#include "luascriptqmladapter.h"

LuaScriptQmlAdapter* LuaScriptQmlAdapter::ms_pInstance = Q_NULLPTR;
QMutex LuaScriptQmlAdapter::ms_mutex;

LuaScriptQmlAdapter::LuaScriptQmlAdapter(QObject* parent)
    : QObject{parent}
{

}

LuaScriptQmlAdapter* LuaScriptQmlAdapter::instance()
{
    QMutexLocker lock(&ms_mutex);

    if (ms_pInstance == Q_NULLPTR)
    {
        ms_pInstance = new LuaScriptQmlAdapter();
    }
    return ms_pInstance;
}

QObject* LuaScriptQmlAdapter::getSingletonTypeProvider(QQmlEngine* pEngine, QJSEngine* pScriptEngine)
{
    Q_UNUSED(pEngine)
    Q_UNUSED(pScriptEngine)

    return instance();
}

void LuaScriptQmlAdapter::changeTab(int newTabIndex)
{
    Q_EMIT signal_changeTab(newTabIndex);
}

void LuaScriptQmlAdapter::requestIntellisense(const QString& filePathName, const QString& currentText)
{
    Q_EMIT signal_requestIntellisense(filePathName, currentText);
}

void LuaScriptQmlAdapter::checkSyntax(const QString& filePathName, const QString& luaCode)
{
    Q_EMIT signal_requestSyntaxCheck(filePathName, luaCode);
}

void LuaScriptQmlAdapter::handleIntellisenseResults(const QString& filePathName, const QStringList& suggestions)
{
    Q_EMIT signal_intellisenseReady(filePathName, suggestions);
}

void LuaScriptQmlAdapter::syntaxCheckResult(const QString& filePathName, bool valid, int line, int start, int end, const QString& message)
{
    Q_EMIT signal_syntaxCheckResult(filePathName, valid, line, start, end, message);
}
