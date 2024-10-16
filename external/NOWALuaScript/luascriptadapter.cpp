#include "luascriptadapter.h"

#include <QThread>
#include <QDebug>

#include <QFileInfo>

LuaScriptAdapter::LuaScriptAdapter(QObject *parent)
    : QObject{parent}
{

}

LuaScriptAdapter::~LuaScriptAdapter()
{
    for (const auto& luaScript : this->luaScripts)
    {
        luaScript->deleteLater();
    }
    this->luaScripts.clear();
}

QPair<int, LuaEditorModelItem*> LuaScriptAdapter::createLuaScript(const QString& filePathName)
{
    int index = this->findLuaScript(filePathName);

    if (index >= 0)
    {
        return qMakePair(index, Q_NULLPTR);
    }

    // TODO: Set file name
    LuaScript* luaScript = new LuaScript(filePathName);
    bool success = luaScript->loadScriptFromFile(filePathName);
    if (false == success)
    {
        return qMakePair(-1, Q_NULLPTR);
    }

    // Create a new thread
    QThread* thread = new QThread(this);

    // Move the backend to the new thread
    luaScript->moveToThread(thread);

    this->luaScripts.append(luaScript);

    // Connecting backend response to QML
    connect(luaScript, &LuaScript::signal_intellisenseReady, this, [this, luaScript](const QStringList& suggestions)
    {
                 Q_EMIT signal_intellisenseReady(luaScript->getFilePathName(), suggestions);
    }, Qt::QueuedConnection);

    connect(luaScript, &LuaScript::signal_syntaxCheckResult, this, [this, luaScript](bool valid, int line, int start, int end, const QString& message)
    {
                 Q_EMIT signal_syntaxCheckResult(luaScript->getFilePathName(), valid, line, start, end, message);
    }, Qt::QueuedConnection);


    // Start the thread
    thread->start();

    // Make sure to quit the thread properly when the UmlDotController is destroyed
    QObject::connect(this, &QObject::destroyed, thread, [thread]()
    {
        thread->quit();
        thread->wait();  // Wait for the thread to finish
        thread->deleteLater();  // Safe deletion after the thread is done
    });
    QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    LuaEditorModelItem* luaEditorModelItem = new LuaEditorModelItem(this);

    luaEditorModelItem->setFilePathName(filePathName);
    luaEditorModelItem->setContent(luaScript->getContent());

    QFileInfo fileInfo(filePathName);
    QString fileName = fileInfo.completeBaseName(); // Extracts filename without extension
    luaEditorModelItem->setTitle(fileName);

    return qMakePair(-1, luaEditorModelItem);
}

bool LuaScriptAdapter::removeLuaScript(const QString& filePathName)
{
    // Check if the luaScript with this filePathName exists in the QHash
    int index = this->findLuaScript(filePathName);
    if (-1 == index)
    {
        qWarning() << "LuaScript with filePathName" << filePathName << "not found.";
        return false;
    }

    // Retrieve the LuaScript instance
    LuaScript* luaScript = this->luaScripts.at(index);

    if (Q_NULLPTR == luaScript)
    {
        qWarning() << "LuaScript for filePathName" << filePathName << "is null.";
        return false;
    }

    // Retrieve the thread that the LuaScript is running on
    QThread* thread = luaScript->thread();

    if (Q_NULLPTR == thread)
    {
        qWarning() << "Thread for LuaScript with filePathName" << filePathName << "is null.";
        return false;
    }

    // Clean up connections to the LuaScript and stop the thread
    disconnect(luaScript, Q_NULLPTR, this, Q_NULLPTR);

    // Quit the thread safely and remove LuaScript after it has stopped
    QObject::connect(thread, &QThread::finished, luaScript, &QObject::deleteLater);

    // Quit the thread and wait for it to finish
    thread->quit();
    thread->wait();

    // Now that the LuaScript has been cleaned up, remove it from the QHash
    this->luaScripts.takeAt(index);

    // Clean up the thread itself
    thread->deleteLater();

    qDebug() << "LuaScript with filePathName" << filePathName << "has been removed.";

    return true;
}

bool LuaScriptAdapter::saveLuaScript(const QString& filePathName, const QString& content)
{
    // Check if the luaScript with this filePathName exists in the QHash
    int index = this->findLuaScript(filePathName);
    if (-1 == index)
    {
        qWarning() << "LuaScript with filePathName" << filePathName << "not found.";
        return false;
    }

    // Retrieve the LuaScript instance
    LuaScript* luaScript = this->luaScripts.at(index);

    if (Q_NULLPTR == luaScript)
    {
        qWarning() << "LuaScript for filePathName" << filePathName << "is null.";
        return false;
    }

    bool success = luaScript->saveLuaScript(content);

    if (true == success)
    {
        Q_EMIT signal_luaScriptSaved();
    }

    return success;
}

void LuaScriptAdapter::generateIntellisense(const QString& filePathName, const QString& currentText)
{
    int index = this->findLuaScript(filePathName);
    if (-1 != index)
    {
        LuaScript* luaScript = this->luaScripts.at(index);
        luaScript->generateIntellisense(currentText);
    }
}

void LuaScriptAdapter::checkSyntax(const QString& filePathName, const QString& luaCode)
{
    int index = this->findLuaScript(filePathName);
    if (-1 != index)
    {
        LuaScript* luaScript = this->luaScripts.at(index);
        luaScript->checkSyntax(luaCode);
    }
}

int LuaScriptAdapter::findLuaScript(const QString &filePathName)
{
    bool scriptDoesExist = false;
    int index = -1;
    for (const auto& luaScript : this->luaScripts)
    {
        index++;
        if (luaScript->getFilePathName() == filePathName)
        {
            scriptDoesExist = true;
            break;
        }
    }

    if (true == scriptDoesExist)
    {
        return index;
    }
    else
    {
        return -1;
    }
}
