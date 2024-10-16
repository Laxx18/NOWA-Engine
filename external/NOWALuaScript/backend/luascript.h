#ifndef LUASCRIPT_H
#define LUASCRIPT_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QSharedPointer>
#include <QMap>

extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

class LuaScript : public QObject
{
    Q_OBJECT
public:
    explicit LuaScript(const QString& filePathName, QObject* parent = Q_NULLPTR);

    virtual ~LuaScript();

    QString getFilePathName(void) const;

    QString getContent(void) const;

    void generateIntellisense(const QString& currentText);  // Generate intellisense based on current text

    bool loadScriptFromFile(const QString& filePathName);  // Load and parse a Lua script

    void checkSyntax(const QString& luaCode);

    bool saveLuaScript(const QString& content);
Q_SIGNALS:
    void signal_luaScriptLoaded(const QString& content);
    void signal_intellisenseReady(const QStringList& suggestions);
    void signal_syntaxCheckResult(bool valid, int line, int start, int end, const QString& message);
private:
    // Internal helper methods and members for parsing
    QStringList searchSuggestions(const QString& text);
private:
    lua_State* lua;  // Lua state
    QString filePathName;
    QString content;
};

#endif // LUASCRIPT_H
