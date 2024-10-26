#ifndef LUASCRIPTADAPTER_H
#define LUASCRIPTADAPTER_H

#include <QObject>
#include <QList>
#include <QMap>
#include <QVariant>

#include "backend/luascript.h"
#include "model/luaeditormodelitem.h"

class LuaScriptAdapter : public QObject
{
    Q_OBJECT
public:
    // Structure to hold class data
    struct MethodData
    {
        QString type;
        QString description;
        QString args;
        QString returns;
        QString valuetype;
    };

    // Structure to hold class data
    struct ClassData
    {
        QString type;
        QString description;
        QString inherits;
        QMap<QString, MethodData> methods;
    };
public:
    explicit LuaScriptAdapter(QObject* parent = Q_NULLPTR);

    virtual ~LuaScriptAdapter();

    QPair<int, LuaEditorModelItem*> createLuaScript(const QString& filePathName);

    bool removeLuaScript(const QString& filePathName);

    bool saveLuaScript(const QString& filePathName, const QString& content);

public Q_SLOTS:
    void generateIntellisense(const QString& filePathName, const QString& currentText);  // Generate intellisense based on current text

    void checkSyntax(const QString& filePathName, const QString& luaCode);

    QMap<QString, ClassData> prepareLuaApi(const QString& filePathName, bool parseSilent);
Q_SIGNALS:
    void signal_luaApiPrepareInitial(const QString& filePathName, bool parseSilent);
    void signal_luaScriptReady(const QString& filePathName, const QString& content);
    void signal_intellisenseReady(const QString& filePathName, const QStringList& suggestions);
    void signal_syntaxCheckResult(const QString& filePathName, bool valid, int line, int start, int end, const QString& message);
    void signal_luaScriptSaved();
    void signal_luaApiPrepareResult(bool silent, bool success, const QString& message);
private:
    int findLuaScript(const QString& filePathName);

    void appendInheritedMethods(QMap<QString, ClassData>& apiData, const QString& className, QSet<QString>& visited);
private:
    QList<LuaScript*> luaScripts;
    bool luaApiCreatedIntially;
};

#endif // LUASCRIPTADAPTER_H
