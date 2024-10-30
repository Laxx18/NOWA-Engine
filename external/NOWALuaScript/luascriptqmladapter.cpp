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

void LuaScriptQmlAdapter::requestSetLuaApi(const QString& filePathName, bool parseSilent)
{
    Q_EMIT signal_requestSetLuaApi(filePathName, parseSilent);
}

void LuaScriptQmlAdapter::relayKeyPress(int key)
{
    Q_EMIT signal_relayKeyPress(key);
}

void LuaScriptQmlAdapter::checkSyntax(const QString& filePathName, const QString& luaCode)
{
    Q_EMIT signal_requestSyntaxCheck(filePathName, luaCode);
}

void LuaScriptQmlAdapter::syntaxCheckResult(const QString& filePathName, bool valid, int line, int start, int end, const QString& message)
{
    Q_EMIT signal_syntaxCheckResult(filePathName, valid, line, start, end, message);
}

void LuaScriptQmlAdapter::luaApiPreparationResult(bool parseSilent, bool success, const QString& message)
{
    Q_EMIT signal_luaApiPreparationResult(parseSilent, success, message);
}
