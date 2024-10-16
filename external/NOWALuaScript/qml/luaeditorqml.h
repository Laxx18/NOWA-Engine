#ifndef LUAEDITORQML_H
#define LUAEDITORQML_H

#include <QQuickItem>
#include <QtQml>

class LuaEditorModelItem;

class LuaEditorQml : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
public:

     Q_PROPERTY(LuaEditorModelItem* model READ model NOTIFY modelChanged)

public:
    // IMPORANT: parent MUST be by default Q_NULLPTR! Else: If creating for QML: Element is not createable will occur!!!
    // See: https://forum.qt.io/topic/155986/qt6-qabstractlistmodel-constructor-with-two-arguments-qml-element-is-not-creatable/2
    explicit LuaEditorQml(QQuickItem* parent = Q_NULLPTR);

    /**
     * @brief Gets the lua editor model item..
     * @returns                 The lua seditor model item..
     */
    LuaEditorModelItem* model() const;

    /**
     * @brief Sets the lua editor model item.
     * @param pModel             The lua editor model item. to set.
     */
    void setModel(LuaEditorModelItem* luaEditorModelItem);
public slots:
    void onParentChanged(QQuickItem* newParent);
Q_SIGNALS:
    void modelChanged();
private:
    LuaEditorModelItem* luaEditorModelItem;
    QQuickItem* lineNumbersEdit;
    QQuickItem* luaEditorTextEdit;
};

#endif // LUAEDITORQML_H
