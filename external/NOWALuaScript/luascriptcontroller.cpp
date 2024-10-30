#include "luascriptcontroller.h"
#include "luascriptadapter.h"

#include "model/luaeditormodelitem.h"
#include "model/apimodel.h"

#include <QDebug>

LuaScriptController::LuaScriptController(QQmlApplicationEngine* qmlEngine, QSharedPointer<LuaScriptAdapter> ptrLuaScriptAdapter, QObject* parent)
    : QObject{parent},
      qmlEngine(qmlEngine),
      ptrLuaScriptAdapter(ptrLuaScriptAdapter),
      luaScriptQmlAdapter(LuaScriptQmlAdapter::instance()),
      luaEditorModel(LuaEditorModel::instance()),
      luaScriptQmlComponent(Q_NULLPTR),
      luaEditorContainer(Q_NULLPTR)
{
    connect(luaEditorModel, &LuaEditorModel::signal_requestAddLuaScript, this, &LuaScriptController::slot_createLuaScript);
    connect(luaEditorModel, &LuaEditorModel::signal_requestRemoveLuaScript, this, &LuaScriptController::slot_removeLuaScript);
    connect(luaEditorModel, &LuaEditorModel::signal_requestSaveLuaScript, this, &LuaScriptController::slot_saveLuaScript);
    connect(ptrLuaScriptAdapter.data(), &LuaScriptAdapter::signal_luaScriptSaved, this, &LuaScriptController::slot_luaScriptSaved);

    connect(this->luaScriptQmlAdapter, &LuaScriptQmlAdapter::signal_requestSyntaxCheck, ptrLuaScriptAdapter.data(), &LuaScriptAdapter::checkSyntax);

    connect(ptrLuaScriptAdapter.data(), &LuaScriptAdapter::signal_luaApiPrepareInitial, this, &LuaScriptController::prepareLuaApi);

    connect(this->luaScriptQmlAdapter, &LuaScriptQmlAdapter::signal_requestSetLuaApi, this, &LuaScriptController::prepareLuaApi);

    // Connecting backend response to QML

    connect(ptrLuaScriptAdapter.data(), &LuaScriptAdapter::signal_syntaxCheckResult, this->luaScriptQmlAdapter, &LuaScriptQmlAdapter::syntaxCheckResult);

    connect(ptrLuaScriptAdapter.data(), &LuaScriptAdapter::signal_luaApiPrepareResult, this->luaScriptQmlAdapter, &LuaScriptQmlAdapter::luaApiPreparationResult);

}

LuaScriptController::~LuaScriptController()
{
    // Removes child pin qml objects first (don't make the lose their parent!)
    for (const auto& luaEditorQml : this->luaEditorQmls)
    {
        luaEditorQml->deleteLater();
    }
    this->luaEditorQmls.clear();
}

void LuaScriptController::slot_createLuaScript(const QString& filePathName)
{
    if (Q_NULLPTR == this->ptrLuaScriptAdapter)
    {
        return;
    }

    QString tempFilePathName = filePathName;

    tempFilePathName = tempFilePathName.replace("\\", "/");

    // Try creating a node model and associate it with backendmodel, let it be filled and configured
    QPair<int, LuaEditorModelItem*> resultData = this->ptrLuaScriptAdapter->createLuaScript(tempFilePathName);

    LuaEditorModelItem* luaEditorModelItem = resultData.second;

    if (Q_NULLPTR == luaEditorModelItem)
    {
        if (resultData.first >= 0)
        {
            // There is already this lua script, so send event with index to set current tab.
            this->luaScriptQmlAdapter->changeTab(resultData.first);
        }

        return;
    }

    QQmlEngine::setObjectOwnership(luaEditorModel, QQmlEngine::CppOwnership);
#if 0
    connect(pNodeModel, &Workbench_Internal__NodeModel::sig_toggleBreakPointOnNode, this, &CanvasItemController::slot_toggleNodeBreak);
    connect(pNodeModel, &Workbench_Internal__NodeModel::sig_toggleMute, this, &CanvasItemController::slot_toggleNodeMute);
    connect(pNodeModel, &Workbench_Internal__NodeModel::sig_rename, this, &CanvasItemController::slot_renameNode);
    connect(pNodeModel, &Workbench_Internal__NodeModel::sig_rename, MeasurementProvider::instance(), &MeasurementProvider::renameMeasurementNode);

    // Is just called, after dragging is completed and will not overflow the event system
    connect(pNodeModel, &Workbench_Internal__NodeModel::xChanged, this, &CanvasItemController::slot_sendChangesFlag);
    connect(pNodeModel, &Workbench_Internal__NodeModel::yChanged, this, &CanvasItemController::slot_sendChangesFlag);
#endif

#if 0
    // Create the corresponding LuaEditorQml item
    LuaEditorQml* luaEditorQml = new LuaEditorQml();
    QQmlEngine::setObjectOwnership(luaEditorQml, QQmlEngine::CppOwnership);

    // You can initialize properties here if needed (e.g., tempFilePathName)
    luaEditorQml->setProperty("filePathName", tempFilePathName);
    luaEditorQml->setModel(luaEditorModelItem);

    this->luaEditorQmls.append(luaEditorQml);
#endif

    LuaEditorQml* luaEditorQml = this->createNewLuaEditorQml();
    if (Q_NULLPTR == luaEditorQml)
    {
        return;
    }

    this->luaEditorQmls.append(luaEditorQml);

    // this->luaEditorContainer->setProperty("currentIndex", this->luaEditorQmls.size());

    // luaEditorQml->setProperty("filePathName", tempFilePathName);
    luaEditorQml->setModel(luaEditorModelItem);

    // connect(luaEditorQml, &LuaEditorQml::currentTextChanged, ApiModel::instance(), &ApiModel::onCurrentTextChanged);

    this->luaEditorModel->addLuaScript(luaEditorModelItem);

    // Sets the new script as active one
    this->luaScriptQmlAdapter->changeTab(this->luaEditorModel->getSize() - 1);
}

void LuaScriptController::slot_removeLuaScript(int index)
{
    LuaEditorQml* luaEditorQml = Q_NULLPTR;
#if 0
    for (const auto& currentLuaEditorQml : this->luaEditorQmls)
    {
        if (currentLuaEditorQml->property("filePathName") == filePathName)
        {
            luaEditorQml = currentLuaEditorQml;
            break;
        }
    }
#endif

    luaEditorQml = this->luaEditorQmls.at(index);

    if (Q_NULLPTR == luaEditorQml)
    {
        qDebug() << "Error on removing lua script";
        return;
    }

    LuaEditorModelItem* luaEditorModelItem = luaEditorQml->model();

    bool success = this->ptrLuaScriptAdapter->removeLuaScript(luaEditorModelItem->getFilePathName());
    if (false == success)
    {
        qDebug() << "Error on removing lua script for file path name: " << luaEditorModelItem->getFilePathName();
        return;
    }

    luaEditorQml->deleteLater();

    this->luaEditorQmls.removeOne(luaEditorQml);

    this->luaEditorModel->removeLuaScript(luaEditorModelItem);
}

void LuaScriptController::slot_saveLuaScript(const QString& filePathName, const QString& content)
{
    bool success = this->ptrLuaScriptAdapter->saveLuaScript(filePathName, content);
    if (false == success)
    {
        qDebug() << "Error on saving lua script for file path name: " << filePathName;
        return;
    }
}

void LuaScriptController::slot_luaScriptSaved()
{
    this->luaEditorModel->luaScriptSaved();
}

void LuaScriptController::prepareLuaApi(const QString& filePathName, bool parseSilent)
{
    const auto& apiData = this->ptrLuaScriptAdapter->prepareLuaApi(filePathName, parseSilent);
    if (true == apiData.empty())
    {
        qDebug() << "Error parsing lua api for file path name: " << filePathName;
        return;
    }

    ApiModel::instance()->setApiData(apiData);
}

LuaEditorQml* LuaScriptController::createNewLuaEditorQml(void)
{
    if (Q_NULLPTR == this->luaScriptQmlComponent)
    {
        if (Q_NULLPTR == this->qmlEngine)
        {
            return Q_NULLPTR;
        }

        if (true == this->qmlEngine->rootObjects().isEmpty())
        {
            return Q_NULLPTR;
        }

        this->luaScriptQmlComponent = new QQmlComponent(this->qmlEngine, QUrl(QStringLiteral("qrc:/qml_files/LuaEditor.qml")), this);
    }

    // Find the root object (the ApplicationWindow)
    this->luaEditorContainer = this->qmlEngine->rootObjects()[0]->findChild<QQuickItem*>("luaEditorContainer");
    if (Q_NULLPTR == this->luaEditorContainer)
    {
        qWarning() << "Could not find luaEditorContainer!";
        return Q_NULLPTR;
    }
    else
    {
        qDebug() << "luaEditorContainer found!";
    }

    // Initialize node qml item
    LuaEditorQml* luaEditorQml;
    QObject* object = this->luaScriptQmlComponent->create();

    luaEditorQml = qobject_cast<LuaEditorQml*>(object);
    if (Q_NULLPTR != luaEditorQml)
    {
        QQmlEngine::setObjectOwnership(luaEditorQml, QQmlEngine::CppOwnership);
    }
    else
    {
        qDebug() << "Error on creating LuaEditorQml";
        for (auto& error : this->luaScriptQmlComponent->errors())
        {
            qDebug() << error.description();
        }
        return Q_NULLPTR;
    }

    luaEditorQml->setParentItem(this->luaEditorContainer);
    luaEditorQml->setParent(this->qmlEngine);
    luaEditorQml->setWidth(this->luaEditorContainer->width() - 12);
    luaEditorQml->setHeight(800.0f);

    return luaEditorQml;
}

QQuickItem *LuaScriptController::findChildRecursive(QObject* parent, const QString& childName)
{
    // Check if the current parent is valid
    if (Q_NULLPTR == parent)
    {
        return Q_NULLPTR;
    }

    // Iterate through all children of the parent
    for (QObject* child : parent->children())
    {
        QQuickItem* childItem = qobject_cast<QQuickItem*>(child);
        if (childItem)
        {
            // Print the current child name for debugging
            qDebug() << "Checking child:" << childItem->objectName() << ", type:" << childItem->metaObject()->className();

            // Check if the child matches the desired name
            if (childItem->objectName() == childName)
            {
                return childItem;
            }

            // Recursively search in the child
            QQuickItem* foundChild = this->findChildRecursive(childItem, childName);
            if (Q_NULLPTR != foundChild)
            {
                return foundChild;
            }
        }
    }
    return Q_NULLPTR;
}
