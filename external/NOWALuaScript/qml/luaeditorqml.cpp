#include "qml/luaeditorqml.h"

#include "model/luaeditormodelitem.h"

#include <QTextDocument>
#include <QFontMetrics>
#include <QDebug>

LuaEditorQml::LuaEditorQml(QQuickItem* parent)
    : QQuickItem(parent),
    luaEditorModelItem(Q_NULLPTR),
    lineNumbersEdit(Q_NULLPTR),
    luaEditorTextEdit(Q_NULLPTR),
    quickTextDocument(Q_NULLPTR),
    highlighter(Q_NULLPTR),
    scrollY(0.0f),
    isAfterColon(false),
    lastColonIndex(-1),
    oldCursorPosition(-1),
    cursorPosition(0)
{
    connect(this, &QQuickItem::parentChanged, this, &LuaEditorQml::onParentChanged);

    // Make sure the item is focusable and accepts key input
    // setFlag(QQuickItem::ItemHasContents, true);
    // setAcceptedMouseButtons(Qt::AllButtons);
    // setFlag(QQuickItem::ItemAcceptsInputMethod, true);
    // setFocus(true);  // Ensure it can receive key events
    // setFocusPolicy(Qt::StrongFocus);
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

    connect(this->luaEditorModelItem, &LuaEditorModelItem::signal_commentLines, this, [this] {
        this->highlighter->commentSelection();
        this->typedAfterColon.clear();
    });

    connect(this->luaEditorModelItem, &LuaEditorModelItem::signal_unCommentLines, this, [this] {
        this->highlighter->uncommentSelection();
        this->typedAfterColon.clear();
    });

    connect(this->luaEditorModelItem, &LuaEditorModelItem::signal_addTabToSelection, this, [this] {
        this->highlighter->addTabToSelection();
        this->typedAfterColon.clear();
    });

    connect(this->luaEditorModelItem, &LuaEditorModelItem::signal_removeTabFromSelection, this, [this] {
        this->highlighter->removeTabFromSelection();
        this->typedAfterColon.clear();
    });

    connect(this->luaEditorModelItem, &LuaEditorModelItem::signal_breakLine, this, [this] {
        this->highlighter->breakLine();
        this->typedAfterColon.clear();
    });

    connect(this->luaEditorModelItem, &LuaEditorModelItem::signal_searchInTextEdit, this, [this](const QString& searchText, bool wholeWord, bool caseSensitive) {
        this->luaEditorTextEdit->forceActiveFocus();
        this->highlighter->searchInTextEdit(searchText, wholeWord, caseSensitive);
    });

    connect(this->luaEditorModelItem, &LuaEditorModelItem::signal_replaceInTextEdit, this, [this](const QString& searchText, const QString& replaceText) {
        this->luaEditorTextEdit->forceActiveFocus();
        this->highlighter->replaceInTextEdit(searchText, replaceText);
        this->typedAfterColon.clear();
    });

    connect(this->luaEditorModelItem, &LuaEditorModelItem::signal_clearSearch, this, [this] {
        this->luaEditorTextEdit->forceActiveFocus();
        this->highlighter->clearSearch();
    });

    connect(this->luaEditorModelItem, &LuaEditorModelItem::signal_undo, this, [this] {
        this->luaEditorTextEdit->forceActiveFocus();
        this->highlighter->undo();
        this->typedAfterColon.clear();
    });

    connect(this->luaEditorModelItem, &LuaEditorModelItem::signal_redo, this, [this] {
        this->luaEditorTextEdit->forceActiveFocus();
        this->highlighter->redo();
        this->typedAfterColon.clear();
    });

    connect(this->luaEditorModelItem, &LuaEditorModelItem::signal_sendTextToEditor, this, [this](const QString& text) {
        this->luaEditorTextEdit->forceActiveFocus();
        this->highlighter->insertText(text);
        this->typedAfterColon.clear();
    });

    connect(this, &LuaEditorQml::requestIntellisenseProcessing, this->luaEditorModelItem, &LuaEditorModelItem::startIntellisenseProcessing);
    connect(this, &LuaEditorQml::requestCloseIntellisense, this->luaEditorModelItem, &LuaEditorModelItem::closeIntellisense);


    Q_EMIT modelChanged();
}

void LuaEditorQml::onParentChanged(QQuickItem* newParent)
{
    if (Q_NULLPTR == newParent)
    {
        return;
    }

    if (Q_NULLPTR != this->highlighter)
    {
        return;
    }

    this->luaEditorTextEdit = this->findChild<QQuickItem*>("luaEditor");

    if (Q_NULLPTR == this->luaEditorTextEdit)
    {
        qWarning() << "Could not find luaEditorTextEdit!";
    }

    this->lineNumbersEdit = this->findChild<QQuickItem*>("lineNumbersEdit");

    if (Q_NULLPTR == this->lineNumbersEdit)
    {
        qWarning() << "Could not find lineNumbersEdit!";
    }

    // Access the QQuickTextDocument from the TextEdit
    this->quickTextDocument = this->luaEditorTextEdit->property("textDocument").value<QQuickTextDocument*>();

    // Create and set up the LuaHighlighter
    this->highlighter = new LuaHighlighter(this->luaEditorTextEdit, quickTextDocument->textDocument());
    // Make sure to set the document for the highlighter
    this->highlighter->setDocument(quickTextDocument->textDocument());

    this->highlighter->setCursorPosition(0);

    // Connect to the textChanged signal
    // QObject::connect(this->luaEditorTextEdit, SIGNAL(textChanged()), this, SLOT(onTextChanged()));
}

void LuaEditorQml::onTextChanged()
{

}

void LuaEditorQml::handleKeywordPressed(QChar keyword)
{
    // Skip any virtual char like shift, strg, alt etc.
    if (keyword == '\0')
    {
        return;
    }

    this->currentText = this->quickTextDocument->textDocument()->toPlainText();

    // Detect if a colon has been deleted
    bool isColonDeleted = false;
    if (keyword == '\010' || keyword == '\177')  // Handle backspace and delete keys
    {
        if (this->lastColonIndex >= 0 && this->lastColonIndex < this->currentText.size())
        {
            if (this->cursorPosition > 0)
            {
                this->cursorPosition--;
                this->oldCursorPosition--;
                this->cursorPositionChanged(this->cursorPosition);
            }

            if (true == this->isAfterColon)
            {
                this->typedAfterColon.removeLast();

                if (this->typedAfterColon.size() < 3)
                {
                    this->showContextMenu(this->typedAfterColon);
                }
            }

            // Check if the character at lastColonIndex is no longer a colon
            QString textAt = this->currentText[this->cursorPosition];
            if (textAt == ':')
            {
                isColonDeleted = true;
            }
        }
    }

    // Reset context if a colon was deleted
    if (true == isColonDeleted)
    {
        this->isAfterColon = false;
        this->typedAfterColon.clear();
        this->lastColonIndex = -1;  // Reset lastColonIndex since the colon is gone
        Q_EMIT requestCloseIntellisense();
    }

    // Check if we're typing after a colon and sequentially
    if (true == this->isAfterColon)
    {
        if (cursorPosition != this->oldCursorPosition + 1 || keyword == ' ' || keyword == '\n' || keyword == ':' || keyword == '.')
        {
            this->isAfterColon = false;
            this->typedAfterColon.clear();
        }
        else
        {
             // Do not append backspace or delete keys
            if (keyword != '\010' && keyword != '\177')
            {
                this->typedAfterColon.append(keyword);

                // Start autocompletion with at least 3 chars
                if (this->typedAfterColon.size() >= 3)
                {
                    this->showContextMenu(this->typedAfterColon);
                }
            }
        }
    }

    // Detect new colon and update lastColonIndex
    if (keyword == ':')
    {
        this->isAfterColon = true;
        this->typedAfterColon.clear();
        this->lastColonIndex = cursorPosition;

        this->showContextMenu("");
    }

    if (keyword != '\010' && keyword != '\177')
    {
        // Update oldCursorPosition to the current cursor position
        this->oldCursorPosition = cursorPosition;
    }
}


void LuaEditorQml::showContextMenu(const QString& textAfterColon)
{
    QTextCursor cursor = this->highlighter->getCursor();
    int cursorPos = cursor.position();  // Get the cursor position

    // Create modified current text that includes the ":", because at this time, the current text would be without the ":" and that would corrupt the class match.
    QString modifiedText = this->currentText;
    modifiedText.insert(cursorPos, ":"); // Insert ":" at the cursor position

    // qDebug() << "------->currentText: " << currentText;
    // qDebug() << "------->modifiedText: " << modifiedText;

    const auto& cursorGlobalPos = this->cursorAtPosition(modifiedText, cursorPos);

    // this->luaEditorModelItem->matchClass(modifiedText, cursorPos, cursorGlobalPos.x(), cursorGlobalPos.y());

    Q_EMIT requestIntellisenseProcessing(currentText, textAfterColon, cursorPos, cursorGlobalPos.x(), cursorGlobalPos.y());
}

void LuaEditorQml::updateContentY(qreal contentY)
{
    this->scrollY = contentY;
}

QPointF LuaEditorQml::cursorAtPosition(const QString& currentText, int cursorPos)
{
    // Calculates the context menu position, according to the current cursor position
    QFont font = this->quickTextDocument->textDocument()->defaultFont();
    QFontMetrics fontMetrics(font);

    // Calculate the line number and character position in the line
    int line = currentText.left(cursorPos).count('\n');
    int lineStartIndex = currentText.lastIndexOf('\n', cursorPos - 1) + 1;
    int charPosInLine = cursorPos - lineStartIndex;

    // Calculate x position
    qreal x = fontMetrics.horizontalAdvance(currentText.mid(lineStartIndex, charPosInLine)) + fontMetrics.ascent();

    // Calculate y position using ascent
    qreal y = line * fontMetrics.height() + fontMetrics.ascent();

    // Get the position of the text edit in the QML layout
    QPointF textEditPos = this->luaEditorTextEdit->mapToGlobal(QPointF(0, 0));

    // Gets the contentY of the parent row and its parent flickable of the text edit
    // qreal scrollY = this->luaEditorTextEdit->parentItem()->parentItem()->property("contentY").toReal();

    // Adjust y by adding the vertical position of the text editor
    qreal adjustedY = y - textEditPos.y() - this->scrollY + fontMetrics.height();

    // Calculate the global cursor position
    QPointF cursorGlobalPos = textEditPos + QPointF(x, adjustedY);
    return cursorGlobalPos;
}

void LuaEditorQml::highlightError(int line, int start, int end)
{
    if (Q_NULLPTR != this->highlighter)
    {
        this->highlighter->setErrorLine(line, start, end);
    }
}

void LuaEditorQml::clearError()
{
    if (Q_NULLPTR != this->highlighter)
    {
        this->highlighter->clearErrors();
    }
}

void LuaEditorQml::cursorPositionChanged(int cursorPosition)
{
    this->highlighter->setCursorPosition(cursorPosition);
    this->cursorPosition = cursorPosition;
}

// https://www.kdab.com/formatting-selected-text-in-qml/
// https://stackoverflow.com/questions/39128725/how-to-implement-rich-text-logic-on-qml-textedit-with-qsyntaxhighlighter-class-i
