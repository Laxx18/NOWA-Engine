#ifndef LUAEDITORQML_H
#define LUAEDITORQML_H

#include <QQuickItem>
#include <QtQml>
#include <QQuickTextDocument>

#include "luahighlighter.h"

class LuaEditorModelItem;

class LuaEditorQml : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
public:

     Q_PROPERTY(LuaEditorModelItem* model READ model NOTIFY modelChanged)
public:
     Q_INVOKABLE void highlightError(int line, int start, int end);

     Q_INVOKABLE void clearError();

     Q_INVOKABLE void cursorPositionChanged(int cursorPosition);

     Q_INVOKABLE void handleKeywordPressed(QChar keyword);

     Q_INVOKABLE void updateContentY(qreal contentY);
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
    void onTextChanged(void);
Q_SIGNALS:
    void modelChanged();
    void requestIntellisenseProcessing(const QString& currentText, const QString& textAfterColon, int cursorPos, int mouseX, int mouseY);
    void requestCloseIntellisense();
private:
    void showContextMenu(const QString& textAfterColon);

    // Helper function to get the cursor rectangle
    QPointF cursorAtPosition(const QString& currentText, int cursorPos);
private:
    LuaEditorModelItem* luaEditorModelItem;
    QQuickItem* lineNumbersEdit;
    QQuickItem* luaEditorTextEdit;
    QQuickTextDocument* quickTextDocument;
    LuaHighlighter* highlighter;  // Add a member for the highlighter

    qreal scrollY;

    bool isAfterColon = false; // Track if we are after a colon
    QString typedAfterColon;    // Store text typed after colon
    int cursorPosition;
    int oldCursorPosition;
    int lastColonIndex;
    QString currentText;
};

#endif // LUAEDITORQML_H
