#ifndef LUASCRIPTQMLADAPTER_H
#define LUASCRIPTQMLADAPTER_H

#include <QObject>
#include <QQmlComponent>
#include <QQuickItem>
#include <QMutex>

class LuaScriptQmlAdapter : public QObject
{
    Q_OBJECT
public:
    explicit LuaScriptQmlAdapter(QObject* parent = Q_NULLPTR);

    Q_INVOKABLE void checkSyntax(const QString& filePathName, const QString& luaCode);

    Q_INVOKABLE void changeTab(int newTabIndex);

    Q_INVOKABLE void requestSetLuaApi(const QString& filePathName, bool parseSilent);

    Q_INVOKABLE void relayKeyPress(int key);
public:
    /**
     * @brief instance is the getter used to receive the object of this singleton implementation.
     * @returns singleton instance of this
     */
    static LuaScriptQmlAdapter* instance();

    /**
     * @brief The singleton type provider is needed by the Qt(-meta)-system to register this singleton instace in qml world
     * @param pEngine not used but needed by function base
     * @param pSriptEngine not used but needed by function base
     * @returns singleton instance of this
     */
    static QObject* getSingletonTypeProvider(QQmlEngine* pEngine, QJSEngine* pScriptEngine);

Q_SIGNALS:
    void signal_requestSyntaxCheck(const QString& filePathName, const QString& luaCode);
    void signal_syntaxCheckResult(const QString& filePathName, bool valid, int line, int start, int end, const QString& message);
    void signal_changeTab(int newTabIndex);
    void signal_requestSetLuaApi(const QString& filePathName, bool parseSilent);
    void signal_luaApiPreparationResult(bool parseSilent, bool success, const QString& message);
    void signal_relayKeyPress(int key);
public Q_SLOTS:
    void syntaxCheckResult(const QString& filePathName, bool valid, int line, int start, int end, const QString& message);

    void luaApiPreparationResult(bool parseSilent, bool success, const QString& message);
private:
    static LuaScriptQmlAdapter* ms_pInstance;
    static QMutex ms_mutex;
};

#endif // LUASCRIPTQMLADAPTER_H
