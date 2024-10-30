#include "apimodel.h"

ApiModel* ApiModel::ms_pInstance = Q_NULLPTR;
QMutex ApiModel::ms_mutex;

ApiModel::ApiModel(QObject* parent)
    : QAbstractListModel{parent},
    isIntellisenseShown(false),
    isMatchedFunctionShown(false)
{

}

QVariantList ApiModel::getMethodsForClassName(const QString& className)
{
    QVariantList methods;
    if (false == this->isValidClassName(className))
    {
        return methods;
    }

    const LuaScriptAdapter::ClassData& classData = this->apiData[className];

    // Loop over methods in the selected class and convert them to QVariantMap
    for (auto it = classData.methods.begin(); it != classData.methods.end(); ++it)
    {
        QVariantMap methodMap;
        QString methodName = it.key(); // Get the method name from the key of the QMap

        // Value types are constants and no methods
        if (it.value().type == "value")
        {
            continue;
        }

        // Populate methodMap with the correct access pattern
        methodMap["name"] = methodName;      // Assuming you want to include the method name
        methodMap["type"] = it.value().type; // Accessing the MethodData's fields correctly
        methodMap["description"] = it.value().description;
        methodMap["args"] = it.value().args;
        methodMap["returns"] = it.value().returns;
        methodMap["valuetype"] = it.value().valuetype;

        methods.append(methodMap); // Append method to the list
    }

    return methods;
}




QVariantList ApiModel::getConstantsForClassName(const QString& className)
{
    QVariantList constants;
    if (false == this->isValidClassName(className))
    {
        return constants;
    }

    const LuaScriptAdapter::ClassData& classData = this->apiData[className];

    // Loop over methods in the selected class and convert them to QVariantMap
    for (auto it = classData.methods.begin(); it != classData.methods.end(); ++it)
    {
        QVariantMap constantMap;

        // Value types are constants and no methods
        if (it.value().type != "value")
        {
            continue;
        }

        // Populate methodMap with the correct access pattern
        constantMap["name"] = it.key();      // Assuming you want to include the method name

        constants.append(constantMap); // Append method to the list
    }

    return constants;
}

bool ApiModel::getIsIntellisenseShown() const
{
    return this->isIntellisenseShown;
}

void ApiModel::setIsIntellisenseShown(bool isIntellisenseShown)
{
    if (this->isIntellisenseShown == isIntellisenseShown)
    {
        return;
    }

    this->isIntellisenseShown = isIntellisenseShown;

    Q_EMIT isIntellisenseShownChanged();
}

bool ApiModel::updateMethodsForSelectedClass()
{
    bool success = false;
    this->methodsForSelectedClass.clear();  // Clear existing methods

    if (true == this->selectedClassName.isEmpty())
    {
        return success;
    }

    if (this->apiData.contains(this->selectedClassName))
    {
        this->setClassType(this->apiData[this->selectedClassName].type);
        this->setClassDescription(this->apiData[this->selectedClassName].description);
        this->setClassInherits(this->apiData[this->selectedClassName].inherits);

        this->methodsForSelectedClass = this->getMethodsForClassName(this->selectedClassName);

        success = true;
    }

    if (true == success)
    {
        Q_EMIT methodsForSelectedClassChanged();  // Notify QML to refresh the list
    }

    return success;
}

bool ApiModel::updateConstantsForSelectedClass()
{
    bool success = false;
    this->constantsForSelectedClass.clear();  // Clear existing methods

    if (true == this->selectedClassName.isEmpty())
    {
        return success;
    }

    if (this->apiData.contains(this->selectedClassName))
    {
        this->setClassType(this->apiData[this->selectedClassName].type);
        this->setClassDescription(this->apiData[this->selectedClassName].description);
        this->setClassInherits(this->apiData[this->selectedClassName].inherits);

        this->constantsForSelectedClass = this->getConstantsForClassName(this->selectedClassName);

        success = true;
    }

    if (true == success)
    {
        Q_EMIT constantsForSelectedClassChanged();  // Notify QML to refresh the list
    }

    return success;
}

QString ApiModel::getClassInherits() const
{
    return this->classInherits;
}

void ApiModel::setClassInherits(const QString& classInherits)
{
    if (this->classInherits == classInherits)
    {
        return;
    }

    this->classInherits = classInherits;
    Q_EMIT classInheritsChanged();
}

QString ApiModel::getClassDescription() const
{
    return this->classDescription;
}

void ApiModel::setClassDescription(const QString& classDescription)
{
    if (this->classDescription == classDescription)
    {
        return;
    }

    this->classDescription = classDescription;
    Q_EMIT classDescriptionChanged();
}

QString ApiModel::getClassType() const
{
    return classType;
}

void ApiModel::setClassType(const QString& classType)
{
    if (this->classType == classType)
    {
        return;
    }

    this->classType = classType;
    Q_EMIT classTypeChanged();
}

ApiModel* ApiModel::instance()
{
    QMutexLocker lock(&ms_mutex);

    if (ms_pInstance == Q_NULLPTR)
    {
        ms_pInstance = new ApiModel();
    }
    return ms_pInstance;
}

QObject* ApiModel::getSingletonTypeProvider(QQmlEngine* pEngine, QJSEngine* pScriptEngine)
{
    Q_UNUSED(pEngine)
    Q_UNUSED(pScriptEngine)

    return instance();
}

void ApiModel::setApiData(const QMap<QString, LuaScriptAdapter::ClassData>& apiData)
{
    beginResetModel();
    this->apiData = apiData;
    endResetModel();
}

int ApiModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return this->apiData.size();
}

QVariant ApiModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= this->apiData.size())
    {
        return QVariant();
    }

    QString className = this->apiData.keys().at(index.row());
    if (false == this->apiData.contains(className))
    {
        return QVariant();
    }
    const LuaScriptAdapter::ClassData& classData = this->apiData[className];

    switch (role)
    {
    case ClassNameRole:
        return className;
    case ClassTypeRole:
        return classData.type;
    case ClassDescriptionRole:
        return classData.description;
    case ClassInheritsRole:
        return classData.inherits;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ApiModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ClassNameRole] = "className";
    roles[ClassTypeRole] = "classType";
    roles[ClassDescriptionRole] = "classDescription";
    roles[ClassInheritsRole] = "classInherits";
    return roles;
}

QString ApiModel::getSelectedClassName() const
{
    return this->selectedClassName;
}

void ApiModel::setSelectedClassName(const QString& selectedClassName)
{
    // if (this->selectedClassName == selectedClassName)
    // {
    //     return;
    // }
    this->selectedClassName = selectedClassName;

    bool hasUpdated = false;

    if (true == this->updateMethodsForSelectedClass())
    {
        hasUpdated = true;
    }
    else
    {
        this->selectedClassName = "";
    }

    if (false == hasUpdated && true == this->updateConstantsForSelectedClass())
    {
        hasUpdated = true;
    }

    if (true == hasUpdated)
    {
        Q_EMIT selectedClassNameChanged();
    }
    else
    {
        this->selectedClassName = "";
    }
}

QString ApiModel::getSelectedMethodName() const
{
    return this->selectedMethodName;
}

void ApiModel::setSelectedMethodName(const QString& selectedMethodName)
{
    if (this->selectedMethodName == selectedMethodName)
    {
        return;
    }
    this->selectedMethodName = selectedMethodName;

    Q_EMIT selectedMethodNameChanged();

    const auto& methodDetails = this->getMethodDetails(this->selectedClassName, this->selectedMethodName);

    if (false == methodDetails.isEmpty())
    {
        Q_EMIT signal_getSelectedMethodDetails(methodDetails["description"].toString(), methodDetails["returns"].toString(), methodDetails["name"].toString(), methodDetails["args"].toString());
    }
}

QVariantList ApiModel::getMethodsForSelectedClass() const
{
    return this->methodsForSelectedClass;
}

QVariantList ApiModel::getConstantsForSelectedClass() const
{
    return this->constantsForSelectedClass;
}

void ApiModel::processMatchedMethodsForSelectedClass(const QString& selectedClassName, const QString& typedAfterColon)
{
    this->methodsForSelectedClass.clear();

    if (typedAfterColon.size() >= 3)
    {
        // Get all methods for the currently selected class
        const auto& methods = this->getMethodsForClassName(selectedClassName);

        for (const auto& method : methods)
        {
            QVariantMap methodMap = method.toMap();
            QString name = methodMap["name"].toString();

            // Check if the method name contains the typed string
            if (name.contains(typedAfterColon, Qt::CaseInsensitive))
            {
                QString type = methodMap["type"].toString();
                // Value types are constants and no methods
                if (type == "value")
                {
                    continue;
                }

                QString description = methodMap["description"].toString();
                QString args = methodMap["args"].toString();
                QString returns = methodMap["returns"].toString();
                QString valuetype = methodMap["valuetype"].toString();

                QVariantMap matchDetails;
                matchDetails["name"] = name;
                matchDetails["type"] = type;
                matchDetails["description"] = description;
                matchDetails["args"] = args;
                matchDetails["returns"] = returns;
                matchDetails["valuetype"] = valuetype;

                // Find the start and end indices of the match
                int startIndex = name.indexOf(typedAfterColon, 0, Qt::CaseInsensitive);
                int endIndex = startIndex + typedAfterColon.length() - 1;

                matchDetails["startIndex"] = startIndex;
                matchDetails["endIndex"] = endIndex;

                this->methodsForSelectedClass.append(matchDetails); // Append the match details to the list
            }
        }
    }

    // If no match, show the whole list
    if (true == this->methodsForSelectedClass.isEmpty())
    {
        this->setSelectedClassName(this->selectedClassName);
    }
    else
    {
        this->selectedClassName = selectedClassName;
        Q_EMIT methodsForSelectedClassChanged();
    }
}

void ApiModel::processMatchedConstantsForSelectedClass(const QString& selectedClassName, const QString& typedAfterColon)
{
    this->constantsForSelectedClass.clear();

    if (typedAfterColon.size() >= 3)
    {
        // Get all constants for the currently selected class
        const auto& constants = this->getConstantsForClassName(selectedClassName);

        for (const auto& constant : constants)
        {
            QVariantMap constantMap = constant.toMap();
            QString name = constantMap["name"].toString();

            // Check if the method name contains the typed string
            if (name.contains(typedAfterColon, Qt::CaseInsensitive))
            {
                QString type = constantMap["type"].toString();
                // Value types are methods and no constants
                if (type != "value")
                {
                    continue;
                }

                QVariantMap matchDetails;
                matchDetails["name"] = name;

                // Find the start and end indices of the match
                int startIndex = name.indexOf(typedAfterColon, 0, Qt::CaseInsensitive);
                int endIndex = startIndex + typedAfterColon.length() - 1;

                matchDetails["startIndex"] = startIndex;
                matchDetails["endIndex"] = endIndex;

                this->constantsForSelectedClass.append(matchDetails); // Append the match details to the list
            }
        }
    }

    // If no match, show the whole list
    if (true == this->constantsForSelectedClass.isEmpty())
    {
        this->setSelectedClassName(this->selectedClassName);
    }
    else
    {
        this->selectedClassName = selectedClassName;
        Q_EMIT constantsForSelectedClassChanged();
    }
}

QString ApiModel::getClassForMethodName(const QString& className, const QString& methodName)
{
    const auto& methods = this->getMethodsForClassName(className);
    for (const auto& method : methods)
    {
        QVariantMap methodMap = method.toMap();

        // Check if the current method's name matches the requested method name
        if (methodMap["name"].toString() == methodName)
        {
            QString returnedClassName = methodMap["returns"].toString();
            return returnedClassName;
        }
    }
    return "";
}

void ApiModel::showIntelliSenseMenu(const QString& wordBeforeColon, int mouseX, int mouseY)
{
    this->setSelectedClassName(wordBeforeColon);

    bool hasSomeThingToShow = false;

    if (false == this->getMethodsForSelectedClass().isEmpty())
    {
        hasSomeThingToShow = true;
    }

    if (false == hasSomeThingToShow && false == this->getConstantsForSelectedClass().isEmpty())
    {
        hasSomeThingToShow = true;
    }

    if (false == hasSomeThingToShow)
    {
        return;
    }

    Q_EMIT signal_showIntelliSenseMenu(mouseX, mouseY);
}

void ApiModel::showMatchedFunctionMenu(int mouseX, int mouseY)
{
    Q_EMIT signal_showMatchedFunctionMenu(mouseX, mouseY);
}

QVariantMap ApiModel::getMethodDetails(const QString& selectedClassName, const QString& methodName)
{
    const auto& methods = this->getMethodsForClassName(selectedClassName);
    for (const auto& method : methods)
    {
        QVariantMap methodMap = method.toMap();

        // Check if the current method's name matches the requested method name
        if (methodMap["name"].toString() == methodName)
        {
            return methodMap;
        }
    }

    return QVariantMap();
}

bool ApiModel::getIsMatchedFunctionShown() const
{
    return this->isMatchedFunctionShown;
}

void ApiModel::setIsMatchedFunctionShown(bool isMatchedFunctionShown)
{
    if (this->isMatchedFunctionShown == isMatchedFunctionShown)
    {
        return;
    }

    this->isMatchedFunctionShown = isMatchedFunctionShown;
    Q_EMIT isMatchedFunctionShownChanged();
}

bool ApiModel::isValidClassName(const QString& className)
{
    if (this->apiData.contains(className))
    {
        return true;
    }
    return false;
}

bool ApiModel::isValidMethodName(const QString& className, const QString& methodName)
{
    const auto& methodResult = this->getMethodDetails(className, methodName);
    return !methodResult.isEmpty();
}

void ApiModel::closeIntellisense()
{
    this->setIsIntellisenseShown(false);
    Q_EMIT signal_closeIntellisense();
}

void ApiModel::closeMatchedFunction()
{
    this->setIsMatchedFunctionShown(false);
    Q_EMIT signal_closeMatchedFunctionMenu();
}
