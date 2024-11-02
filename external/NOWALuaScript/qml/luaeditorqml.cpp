#include "qml/luaeditorqml.h"

#include "model/luaeditormodelitem.h"

#include <QTextDocument>
#include <QFontMetrics>
#include <QQuickWindow>
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
    isAfterDot(false),
    lastColonIndex(-1),
    lastDotIndex(-1),
    oldCursorPosition(-1),
    cursorPosition(0),
    isInMatchedFunctionProcessing(false)
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
        this->resetTextAfterColon();
        this->resetTextAfterDot();
    });

    connect(this->luaEditorModelItem, &LuaEditorModelItem::signal_unCommentLines, this, [this] {
        this->highlighter->uncommentSelection();
        this->resetTextAfterColon();
        this->resetTextAfterDot();
    });

    connect(this->luaEditorModelItem, &LuaEditorModelItem::signal_addTabToSelection, this, [this] {
        this->highlighter->addTabToSelection();
        this->resetTextAfterColon();
        this->resetTextAfterDot();
    });

    connect(this->luaEditorModelItem, &LuaEditorModelItem::signal_removeTabFromSelection, this, [this] {
        this->highlighter->removeTabFromSelection();
        this->resetTextAfterColon();
        this->resetTextAfterDot();
    });

    connect(this->luaEditorModelItem, &LuaEditorModelItem::signal_breakLine, this, [this] {
        this->highlighter->breakLine();
        this->resetTextAfterColon();
        this->resetTextAfterDot();
    });

    connect(this->luaEditorModelItem, &LuaEditorModelItem::signal_searchInTextEdit, this, [this](const QString& searchText, bool wholeWord, bool caseSensitive) {
        this->luaEditorTextEdit->forceActiveFocus();
        this->highlighter->searchInTextEdit(searchText, wholeWord, caseSensitive);
    });

    connect(this->luaEditorModelItem, &LuaEditorModelItem::signal_replaceInTextEdit, this, [this](const QString& searchText, const QString& replaceText) {
        this->luaEditorTextEdit->forceActiveFocus();
        this->highlighter->replaceInTextEdit(searchText, replaceText);
        this->resetTextAfterColon();
        this->resetTextAfterDot();
    });

    connect(this->luaEditorModelItem, &LuaEditorModelItem::signal_clearSearch, this, [this] {
        this->luaEditorTextEdit->forceActiveFocus();
        this->highlighter->clearSearch();
    });

    connect(this->luaEditorModelItem, &LuaEditorModelItem::signal_undo, this, [this] {
        this->luaEditorTextEdit->forceActiveFocus();
        this->highlighter->undo();
        this->resetTextAfterColon();
        this->resetTextAfterDot();
    });

    connect(this->luaEditorModelItem, &LuaEditorModelItem::signal_redo, this, [this] {
        this->luaEditorTextEdit->forceActiveFocus();
        this->highlighter->redo();
        this->resetTextAfterColon();
        this->resetTextAfterDot();
    });

    connect(this->luaEditorModelItem, &LuaEditorModelItem::signal_sendTextToEditor, this, [this](const QString& text) {
        this->showIntelliSenseContextMenuAtCursor(false, text);
        this->luaEditorTextEdit->forceActiveFocus();
        if (true == this->isAfterColon)
        {
            this->highlighter->insertSentText(this->typedAfterColon.size(), text);
            // Actualize the typed after colon text, to still show the intelisense for the method
            this->typedAfterColon = text;
        }
        else if (true == this->isAfterDot)
        {
            this->highlighter->insertSentText(this->typedAfterDot.size(), text);
            // Actualize the typed after colon text, to still show the intelisense for the method
            this->typedAfterDot = text;
            Q_EMIT requestCloseIntellisense();
        }

        this->oldCursorPosition = this->cursorPosition - 1;
    });

    connect(this, &LuaEditorQml::requestIntellisenseProcessing, this->luaEditorModelItem, &LuaEditorModelItem::startIntellisenseProcessing);
    connect(this, &LuaEditorQml::requestCloseIntellisense, this->luaEditorModelItem, &LuaEditorModelItem::closeIntellisense);

    connect(this, &LuaEditorQml::requestMatchedFunctionContextMenu, this->luaEditorModelItem, &LuaEditorModelItem::startMatchedFunctionProcessing);
    connect(this, &LuaEditorQml::requestCloseMatchedFunctionContextMenu, this->luaEditorModelItem, &LuaEditorModelItem::closeMatchedFunction);


    connect(this->highlighter, &LuaHighlighter::insertingNewLineChanged, this, [this](bool inserting) {
        Q_EMIT signal_insertingNewLine(inserting);
    });

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

void LuaEditorQml::resetTextAfterColon()
{
    this->isAfterColon = false;
    this->typedAfterColon.clear();
}

void LuaEditorQml::resetTextAfterDot()
{
    this->isAfterDot = false;
    this->typedAfterDot.clear();
}

void LuaEditorQml::handleKeywordPressed(QChar keyword)
{
    // Skip any virtual char like shift, strg, alt etc.
    if (keyword == '\0')
    {
        return;
    }

    this->currentText = this->quickTextDocument->textDocument()->toPlainText();

    // Cursor jump, clear everything
    if (this->cursorPosition != this->oldCursorPosition + 1)
    {
        this->isAfterColon = false;
        this->typedAfterColon.clear();
        this->isInMatchedFunctionProcessing = false;
    }

    // Detect if a colon has been deleted
    bool isColonDeleted = false;
    bool isDotDeleted = false;
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
                // If deleting a char and the opening bracket has been deleted, we are no longer in matched function processing
                if (this->typedAfterColon.contains("("))
                {
                    this->isInMatchedFunctionProcessing = false;
                    Q_EMIT requestCloseMatchedFunctionContextMenu();
                }
                this->typedAfterColon.removeLast();
                this->showIntelliSenseContextMenuAtCursor(false, this->typedAfterColon);
            }

            // Check if the character at lastColonIndex is no longer a colon
            QString textAt = this->currentText[this->cursorPosition];
            if (textAt == ':')
            {
                isColonDeleted = true;
            }
        }
        else if (this->lastDotIndex >= 0 && this->lastDotIndex < this->currentText.size())
        {
            if (this->cursorPosition > 0)
            {
                this->cursorPosition--;
                this->oldCursorPosition--;
                this->cursorPositionChanged(this->cursorPosition);
            }

            if (true == this->isAfterDot)
            {
                // If deleting a char and the opening bracket has been deleted, we are no longer in matched function processing
                if (this->typedAfterColon.contains("("))
                {
                    this->isInMatchedFunctionProcessing = false;
                    Q_EMIT requestCloseMatchedFunctionContextMenu();
                }
                this->typedAfterDot.removeLast();
                this->showIntelliSenseContextMenuAtCursor(true, this->typedAfterDot);
            }

            // Check if the character at lastDotIndex is no longer a dot
            QString textAt = this->currentText[this->cursorPosition];
            if (textAt == '.')
            {
                isDotDeleted = true;
            }
        }
    }

    // Reset context if a colon was deleted
    if (true == isColonDeleted)
    {
        this->resetTextAfterColon();
        this->lastColonIndex = -1;  // Reset lastColonIndex since the colon is gone
        Q_EMIT requestCloseIntellisense();
    }
    else if (true == isDotDeleted)
    {
        this->resetTextAfterDot();
        this->lastDotIndex = -1;  // Reset lastDotIndex since the dot is gone
        Q_EMIT requestCloseIntellisense();
    }

    // Check if we're typing after a colon and sequentially
    if (true == this->isAfterColon)
    {
        if (false == this->isInMatchedFunctionProcessing && (this->cursorPosition != this->oldCursorPosition + 1 || keyword == ' ' || keyword == '\n' || keyword == ':' || keyword == '.'))
        {
            this->resetTextAfterColon();
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
                    this->showIntelliSenseContextMenuAtCursor(false, this->typedAfterColon);
                }
            }
        }
    }
    else if (true == this->isAfterDot)
    {
        if (false == this->isInMatchedFunctionProcessing && (this->cursorPosition != this->oldCursorPosition + 1 || keyword == ' ' || keyword == '\n' || keyword == ':' || keyword == '.'))
        {
            this->resetTextAfterDot();
        }
        else
        {
            // Do not append backspace or delete keys
            if (keyword != '\010' && keyword != '\177')
            {
                this->typedAfterDot.append(keyword);

                // Start autocompletion with at least 3 chars
                if (this->typedAfterDot.size() >= 3)
                {
                    this->showIntelliSenseContextMenuAtCursor(true, this->typedAfterDot);
                }
            }
        }
    }

    // and not removing char
    if (keyword == ')' && keyword != '\010' && keyword != '\177')
    {
        this->processBracket(keyword);
    }

    if (this->typedAfterColon.contains("("))
    {
        this->isInMatchedFunctionProcessing = true;
        this->showMachtedFunctionContextMenuAtCursor(this->typedAfterColon);
    }
    if (this->typedAfterColon.contains(")"))
    {
        this->isInMatchedFunctionProcessing = false;
        Q_EMIT requestCloseMatchedFunctionContextMenu();
    }

    // Detect new colon and update lastColonIndex
    if (keyword == ':')
    {
        this->isAfterColon = true;
        this->typedAfterColon.clear();
        this->lastColonIndex = this->cursorPosition;

        this->showIntelliSenseContextMenu(false);
    }
    // Detect new dot and update lastDotIndex
    else if (keyword == '.')
    {
        this->isAfterDot = true;
        this->typedAfterDot.clear();
        this->lastDotIndex = this->cursorPosition;

        this->showIntelliSenseContextMenu(true);
    }
    else if (keyword == '(' && true == this->isAfterColon)
    {
        this->isInMatchedFunctionProcessing = true;
        Q_EMIT requestCloseIntellisense();

        this->showMachtedFunctionContextMenuAtCursor(this->typedAfterColon);
    }
    else if (keyword == ')' && true == this->isAfterColon)
    {
        this->isInMatchedFunctionProcessing = false;
    }

    // and not removing char
    if (keyword == '(' && keyword != '\010' && keyword != '\177')
    {
        this->processBracket(keyword);
    }

    if (keyword != '\010' && keyword != '\177')
    {
        // Update oldCursorPosition to the current cursor position
        this->oldCursorPosition = this->cursorPosition;
    }
}

void LuaEditorQml::processBracket(QChar keyword)
{
    if (false == this->isInMatchedFunctionProcessing)
    {
        // Find the last colon in the line up to the current cursor position
        int lineStart = this->quickTextDocument->textDocument()->findBlockByLineNumber(this->cursorPosition).position();
        int lastColonInLineIndex = this->currentText.lastIndexOf(':', this->cursorPosition);
        bool isInSameLine = lastColonInLineIndex >= lineStart;

        // Detects just an opening or closing bracket, whether its a valid function to show infos
        if (true == isInSameLine)
        {
            // Extract text from the last colon to the bracket position
            int startIndex = lastColonInLineIndex + 1;
            int endIndex = this->cursorPosition;

            QString textFromColonToBracket = this->currentText.mid(startIndex, endIndex - startIndex).trimmed();
            textFromColonToBracket += keyword;

            // Optionally, skip processing based on extracted text or take further actions here
            if (false == textFromColonToBracket.isEmpty())
            {
                this->showIntelliSenseContextMenu(false);

                this->isInMatchedFunctionProcessing = true;
                Q_EMIT requestCloseIntellisense();

                this->isAfterColon = true;
                this->typedAfterColon = textFromColonToBracket;

                // this->showMachtedFunctionContextMenuAtCursor(this->typedAfterColon);
            }
        }
    }
}

void LuaEditorQml::showIntelliSenseContextMenu(bool forConstant)
{
    if (true == this->isInMatchedFunctionProcessing)
    {
        Q_EMIT requestCloseIntellisense();
        return;
    }

    QTextCursor cursor = this->highlighter->getCursor();
    int cursorPos = cursor.position();  // Get the cursor position

    // Create modified current text that includes the ":", because at this time, the current text would be without the ":" and that would corrupt the class match.
    QString modifiedText = this->currentText;
    if (false == forConstant)
    {
        modifiedText.insert(cursorPos, ":"); // Insert ":" at the cursor position
    }
    else
    {
        modifiedText.insert(cursorPos, "."); // Insert "." at the cursor position
    }

    const auto& cursorGlobalPos = this->cursorAtPosition(modifiedText, cursorPos);

    Q_EMIT requestIntellisenseProcessing(forConstant, this->currentText, "", cursorPos, cursorGlobalPos.x(), cursorGlobalPos.y());
}

void LuaEditorQml::showIntelliSenseContextMenuAtCursor(bool forConstant, const QString& textAfterColon)
{
    if (true == this->isInMatchedFunctionProcessing)
    {
        Q_EMIT requestCloseIntellisense();
        return;
    }

    const auto& cursorGlobalPos = this->cursorAtPosition(this->currentText, this->cursorPosition);

    Q_EMIT requestIntellisenseProcessing(forConstant, this->currentText, textAfterColon, this->cursorPosition, cursorGlobalPos.x(), cursorGlobalPos.y());
}

void LuaEditorQml::showMachtedFunctionContextMenuAtCursor(const QString& textAfterColon)
{
    if (false == this->isInMatchedFunctionProcessing)
    {
        Q_EMIT requestCloseMatchedFunctionContextMenu();
        return;
    }

    Q_EMIT requestCloseIntellisense();

    const auto& cursorGlobalPos = this->cursorAtPosition(this->currentText, this->cursorPosition);

    Q_EMIT requestMatchedFunctionContextMenu(textAfterColon, this->cursorPosition, cursorGlobalPos.x(), cursorGlobalPos.y());
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

    // Get the global window position
    QQuickWindow* window = this->luaEditorTextEdit->window();
    QPoint windowPos = window->mapToGlobal(QPoint(0, 0));

    // Adjust y by adding the vertical position of the text editor (23: size of the caption in application window)
    qreal adjustedY = y - textEditPos.y() - this->scrollY + fontMetrics.height() + 23;

    // Calculate the global cursor position
    QPointF cursorGlobalPos = textEditPos + QPointF(x - windowPos.x(), adjustedY - windowPos.y());
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
