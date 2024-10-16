#ifndef LUASCRIPTADAPTER_H
#define LUASCRIPTADAPTER_H

#include <QObject>
#include <QList>

#include "backend/luascript.h"
#include "model/luaeditormodelitem.h"

class LuaScriptAdapter : public QObject
{
    Q_OBJECT
public:
    explicit LuaScriptAdapter(QObject* parent = Q_NULLPTR);

    virtual ~LuaScriptAdapter();

    QPair<int, LuaEditorModelItem*> createLuaScript(const QString& filePathName);

    bool removeLuaScript(const QString& filePathName);

    bool saveLuaScript(const QString& filePathName, const QString& content);

public Q_SLOTS:
    void generateIntellisense(const QString& filePathName, const QString& currentText);  // Generate intellisense based on current text

    void checkSyntax(const QString& filePathName, const QString& luaCode);

Q_SIGNALS:
    void signal_luaScriptReady(const QString& filePathName, const QString& content);
    void signal_intellisenseReady(const QString& filePathName, const QStringList& suggestions);
    void signal_syntaxCheckResult(const QString& filePathName, bool valid, int line, int start, int end, const QString& message);
    void signal_luaScriptSaved();
private:
    int findLuaScript(const QString& filePathName);
private:
    QList<LuaScript*> luaScripts;
};

#endif // LUASCRIPTADAPTER_H
