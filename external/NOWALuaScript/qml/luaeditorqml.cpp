#include "qml/luaeditorqml.h"

#include "model/luaeditormodel.h"

LuaEditorQml::LuaEditorQml(QQuickItem* parent)
    : QQuickItem(parent),
      luaEditorModelItem(Q_NULLPTR),
      lineNumbersEdit(Q_NULLPTR),
      luaEditorTextEdit(Q_NULLPTR)
{
     connect(this, &QQuickItem::parentChanged, this, &LuaEditorQml::onParentChanged);
}

LuaEditorModelItem* LuaEditorQml::model() const
{
    return this->luaEditorModelItem;
}

void LuaEditorQml::setModel(LuaEditorModelItem* luaEditorModelItem)
{
    if (this->luaEditorModelItem == luaEditorModelItem)
    {
       return;
    }

    this->luaEditorModelItem = luaEditorModelItem;

    Q_EMIT modelChanged();
}

void LuaEditorQml::onParentChanged(QQuickItem* newParent)
{
    if (Q_NULLPTR == newParent)
    {
        return;
    }

    this->luaEditorTextEdit = this->findChild<QQuickItem*>("luaEditor");

    if (Q_NULLPTR == this->luaEditorTextEdit)
    {
        qWarning() << "Could not find luaEditorTextEdit!";
    }

    // this->luaEditorTextEdit->setProperty("text", "Hello from C++!");

    this->lineNumbersEdit = this->findChild<QQuickItem*>("lineNumbersEdit");

    if (Q_NULLPTR == this->lineNumbersEdit)
    {
        qWarning() << "Could not find lineNumbersEdit!";
    }
}
