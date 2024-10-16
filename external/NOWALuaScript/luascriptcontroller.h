#ifndef LUASCRIPTCONTROLLER_H
#define LUASCRIPTCONTROLLER_H

#include <QObject>
#include <QSharedPointer>

#include <QQmlComponent>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QList>

#include "luascriptqmladapter.h"
#include "model/luaeditormodel.h"
#include "qml/luaeditorqml.h"

class LuaScriptAdapter;
class LuaEditorModelItem;

class LuaScriptController : public QObject
{
    Q_OBJECT
public:
    explicit LuaScriptController(QQmlApplicationEngine* qmlEngine, QSharedPointer<LuaScriptAdapter> ptrLuaScriptAdapter, QObject* parent = Q_NULLPTR);

    virtual ~LuaScriptController();
public slots:
    void slot_createLuaScript(const QString& filePathName);

    void slot_removeLuaScript(int index);

    void slot_saveLuaScript(const QString& filePathName, const QString& content);

    void slot_luaScriptSaved();
private:
    LuaEditorQml* createNewLuaEditorQml(void);

    // Function to recursively search for a child item by name
    QQuickItem* findChildRecursive(QObject* parent, const QString& childName);
private:
    QQmlApplicationEngine* qmlEngine;
    LuaScriptQmlAdapter* luaScriptQmlAdapter;
    LuaEditorModel* luaEditorModel;
    QSharedPointer<LuaScriptAdapter> ptrLuaScriptAdapter;

    QQmlComponent* luaScriptQmlComponent;
    QQuickItem* luaEditorContainer;

    QList<LuaEditorQml*> luaEditorQmls;
};

#endif // LUASCRIPTCONTROLLER_H
