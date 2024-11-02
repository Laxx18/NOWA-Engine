#include "qml/luahighlighter.h"

#include <QtConcurrent>
#include <QQuickTextDocument>
#include <QDebug>

LuaHighlighter::LuaHighlighter(QQuickItem* luaEditorTextEdit, QObject* parent)
    : QSyntaxHighlighter{parent},
    luaEditorTextEdit(luaEditorTextEdit),
    errorLine(-1),
    oldErrorLine(-2),
    errorStart(-1),
    errorEnd(-1),
    errorAlreadyCleared(true),
    wholeWord(false),
    caseSensitiv(false)
{
    this->errorFormat.setBackground(Qt::transparent); // No background
    this->errorFormat.setForeground(Qt::red); // Set error color to red
    this->errorFormat.setFontUnderline(true); // Underline the text
    // // Does not work
    // // this->errorFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);  // Squiggle underline

    HighlightingRule rule;

    // Define keyword formats
    QTextCharFormat keywordFormat;
    keywordFormat.setForeground(QColor("#00008B"));  // Navy Blue

    // Keywords pattern
    rule.pattern = QRegularExpression("\\b(if|else|while|for|end|function|local|return|do|then|break|in|repeat|until)\\b");
    rule.format = keywordFormat;
    this->highlightingRules.append(rule);

    // Single-line comment format
    QTextCharFormat commentFormat;
    // commentFormat.setForeground(QColor("#696969"));  // Dim Gray
    commentFormat.setForeground(Qt::darkGreen);

    rule.pattern = QRegularExpression("(--[^\n]*)");
    rule.format = commentFormat;
    this->highlightingRules.append(rule);

    // Quotation format for single and double quotes
    QTextCharFormat quotationFormat;
    quotationFormat.setForeground(QColor("#008B8B"));  // Dark Cyan
    rule.pattern = QRegularExpression("\"[^\"]*\"|'[^']*'");
    rule.format = quotationFormat;
    this->highlightingRules.append(rule);
}

void LuaHighlighter::setErrorLine(int line, int start, int end)
{
    if (this->oldErrorLine != this->errorLine)
    {
        this->errorLine = line;
        this->errorStart = start;
        this->errorEnd = end;
        this->rehighlight(); // Rehighlight the document to apply changes
        this->oldErrorLine = this->errorLine;
    }
}

void LuaHighlighter::clearErrors()
{
    if (false == this->errorAlreadyCleared)
    {
        // Clear highlight for the specific line
        QTextCharFormat format; // Reset to default format
        format.setBackground(Qt::transparent); // Set to transparent to clear highlight

        QTextBlock block = document()->findBlockByLineNumber(this->errorLine);
        if (block.isValid())
        {
            // Clear formatting for the entire block (line)
            setFormat(block.firstLineNumber(), block.length(), format);
        }

        this->errorLine = -1;
        this->errorStart = -1;
        this->errorEnd = -1;

        this->rehighlight(); // Rehighlight the document to clear errors

        this->errorAlreadyCleared = true;
    }
}

void LuaHighlighter::commentSelection()
{
    this->cursor.beginEditBlock();
    QTextBlock startBlock = this->cursor.block();
    QTextBlock endBlock = this->cursor.block();  // End block is the same as start for single-line selection

    if (this->cursor.hasSelection())
    {
        startBlock = this->cursor.document()->findBlock(this->cursor.selectionStart());
        endBlock = this->cursor.document()->findBlock(this->cursor.selectionEnd());
    }

    for (QTextBlock block = startBlock; block != endBlock.next(); block = block.next())
    {
        QString blockText = block.text();

        // Only comment if the line is not already commented
        if (!blockText.trimmed().isEmpty() && !blockText.trimmed().startsWith("--"))
        {
            QTextCursor blockCursor(block);
            blockCursor.movePosition(QTextCursor::StartOfBlock);

            // Get the leading whitespace
            QString leadingWhitespace = blockText.left(blockText.indexOf(blockText.trimmed()));

            // Select the entire line to replace it
            blockCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, blockText.length());

            // Remove the original line
            blockCursor.removeSelectedText();
            // Insert the comment marker without additional space, followed by the original text
            blockCursor.insertText(leadingWhitespace + "--" + blockText.trimmed()); // Ensure no extra spaces
        }
    }

    this->cursor.endEditBlock();
}

void LuaHighlighter::uncommentSelection()
{
    this->cursor.beginEditBlock();
    QTextBlock startBlock = this->cursor.block();
    QTextBlock endBlock = this->cursor.block();  // End block is the same as start for single-line selection

    if (cursor.hasSelection())
    {
        startBlock = this->cursor.document()->findBlock(this->cursor.selectionStart());
        endBlock = this->cursor.document()->findBlock(this->cursor.selectionEnd());
    }

    for (QTextBlock block = startBlock; block != endBlock.next(); block = block.next())
    {
        QString blockText = block.text();

        // Use regex to match lines that start with optional spaces followed by "--"
        QRegularExpression commentRegex("^\\s*--");

        // If the line starts with "--", uncomment it
        if (commentRegex.match(blockText).hasMatch())
        {
            // Find the index of the comment marker
            int commentIndex = blockText.indexOf("--");
            // Get the leading whitespace
            QString leadingWhitespace = blockText.left(commentIndex);
            // Get the uncommented text, preserving original spacing
            QString uncommentedText = blockText.mid(commentIndex + 2).trimmed(); // Get the rest of the line after "--"

            // Create a cursor for the current block
            QTextCursor blockCursor(block);
            blockCursor.movePosition(QTextCursor::StartOfBlock);
            // Select the entire line including the leading whitespace and comment
            blockCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, blockText.length()); // Select entire line

            // Remove the selected text
            blockCursor.removeSelectedText();

            // Insert the uncommented text with leading whitespace, preserving original alignment
            blockCursor.insertText(leadingWhitespace + uncommentedText); // Insert leading whitespace followed by uncommented text
        }
    }

    this->cursor.endEditBlock();
}

void LuaHighlighter::highlightMatchingBrackets()
{
    // Get the position of the cursor relative to the current block.
    int cursorPos = this->cursor.position() - this->cursor.block().position();  // Position relative to the block
    if (cursorPos < 0 || cursorPos >= this->cursor.block().length())
    {
        return;
    }

    QString text = this->cursor.block().text();

    // Find the matching brackets, if any.
    QPair<int, int> bracketPair = this->findMatchingBrackets(text, cursorPos);

    if (bracketPair.first != -1 && bracketPair.second != -1)
    {
        QTextCharFormat matchFormat;
        matchFormat.setBackground(QColor("#377dbd"));  // Customize the appearance of matched brackets.
        matchFormat.setFontWeight(QFont::Bold);  // Set the matching brackets to bold.

        // Highlight the matching brackets.
        setFormat(bracketPair.first, 1, matchFormat);   // Highlight the opening bracket
        setFormat(bracketPair.second, 1, matchFormat);  // Highlight the closing bracket
    }
    else
    {
        // No matching brackets found, reset any bold formatting.
        this->removeMatchingBracketsBold();
    }
}

void LuaHighlighter::removeMatchingBracketsBold()
{
    // Iterate over each character in the current block to check and reset bold formatting only.
    for (int i = 0; i < currentBlock().length(); ++i)
    {
        QTextCharFormat charFormat = format(i);  // Get the current character format.

        if (charFormat.fontWeight() == QFont::Bold)
        {
            // If the character is bold, reset the boldness without affecting other styles.
            charFormat.setFontWeight(QFont::Normal);
            setFormat(i, 1, charFormat);
        }
    }
}

void LuaHighlighter::clearSearch()
{
    this->searchText = "";
    this->replaceText = "";
    this->rehighlight();
}

void LuaHighlighter::undo()
{
    // Must be called, else crash does occur because of illegal state of document due to highlighting
    this->clearSearch();
    if (!this->cursor.isNull() && this->cursor.document())
    {
        this->cursor.document()->undo();
    }
}

void LuaHighlighter::redo()
{
    this->clearSearch();
    if (!this->cursor.isNull() && this->cursor.document())
    {
        this->cursor.document()->redo();
    }
}

void LuaHighlighter::insertSentText(int sizeToReplace, const QString& text)
{
    // Begin an edit block to group changes for undo/redo
    this->cursor.beginEditBlock();

    QTextCursor cursor = this->cursor;
    QTextBlock block = currentBlock();  // Get the current block

    // Move cursor backwards by sizeToReplace characters to select them
    cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, sizeToReplace);

    // Insert the new text, replacing the selected characters
    cursor.insertText(text);

    this->cursor = cursor;  // Update the cursor after the replacement

    // End the edit block to finalize changes (allows for undo/redo)
    this->cursor.endEditBlock();
}

QPair<int, int> LuaHighlighter::findMatchingBrackets(const QString& text, int cursorPos)
{
    if (cursorPos < 0 || cursorPos >= text.length()) return QPair<int, int>(-1, -1);

    QChar currentChar = text.at(cursorPos);

    if (currentChar == '(')
    {
        int depth = 1;
        for (int i = cursorPos + 1; i < text.length(); ++i)
        {
            QChar ch = text.at(i);
            if (ch == '(') depth++;
            if (ch == ')') depth--;
            if (depth == 0) return QPair<int, int>(cursorPos, i);  // Match found
        }
    }
    else if (currentChar == ')')
    {
        int depth = 1;
        for (int i = cursorPos - 1; i >= 0; --i)
        {
            QChar ch = text.at(i);
            if (ch == ')') depth++;
            if (ch == '(') depth--;
            if (depth == 0) return QPair<int, int>(i, cursorPos);  // Match found
        }
    }
    return QPair<int, int>(-1, -1);  // No match found
}

QTextCursor LuaHighlighter::getCursor()
{
    // Access the text edit object from the Lua editor to get the current cursor position
    QVariant cursorVariant = this->luaEditorTextEdit->property("cursorPosition");
    int cursorPosition = cursorVariant.toInt();

    // Set up QTextCursor from the current text document and set its position
    QTextCursor cursor(document());

    if (-1 == cursorPosition)
    {
        return cursor;
    }
    cursor.setPosition(cursorPosition);

    // If selection exists, adjust the cursor accordingly
    QVariant selectionStartVariant = this->luaEditorTextEdit->property("selectionStart");
    QVariant selectionEndVariant = this->luaEditorTextEdit->property("selectionEnd");

    if (selectionStartVariant.isValid() && selectionEndVariant.isValid())
    {
        int selectionStart = selectionStartVariant.toInt();
        int selectionEnd = selectionEndVariant.toInt();
        cursor.setPosition(selectionStart);
        cursor.setPosition(selectionEnd, QTextCursor::KeepAnchor);
    }

    return cursor;
}

void LuaHighlighter::setCursorPosition(int cursorPosition)
{
    // Q_UNUSED(cursorPosition);
    this->cursor = this->getCursor();

    QString text = this->cursor.block().text();

    int cursorPos = this->cursor.position() - this->cursor.block().position();  // Position relative to the block
    if (cursorPos < 0 || cursorPos >= this->cursor.block().length() || cursorPos >= text.length())
    {
        return;
    }

    QChar currentChar = text.at(cursorPos);

    // Performance optimization
    if (currentChar == '(' || currentChar == ')')
    {
        this->rehighlight();  // Reapply syntax highlighting
    }
}

void LuaHighlighter::addTabToSelection()
{
    this->cursor.beginEditBlock();
    QTextBlock startBlock = this->cursor.block();
    QTextBlock endBlock = this->cursor.block();  // Default to the current block

    if (this->cursor.hasSelection())
    {
        startBlock = this->cursor.document()->findBlock(this->cursor.selectionStart());
        endBlock = this->cursor.document()->findBlock(this->cursor.selectionEnd());
    }

    QString indent = QString(4, ' ');  // Create a string with 4 spaces

    // Iterate over each selected block and add 4 spaces at the start of each block
    for (QTextBlock block = startBlock; block != endBlock.next(); block = block.next())
    {
        QTextCursor blockCursor(block);
        blockCursor.movePosition(QTextCursor::StartOfBlock);

        // Select the entire block and add 4 spaces
        blockCursor.insertText(indent);
    }

    // Ensure cursor is restored back to where it was before the operation
    if (this->cursor.hasSelection())
    {
        this->cursor.setPosition(this->cursor.selectionStart());
        this->cursor.setPosition(this->cursor.selectionEnd(), QTextCursor::KeepAnchor);
    }

    this->cursor.endEditBlock();
}

void LuaHighlighter::removeTabFromSelection()
{
    this->cursor.beginEditBlock();

    // Determine if there's a selection, get start and end blocks
    QTextBlock startBlock = this->cursor.block();
    QTextBlock endBlock = this->cursor.block();

    if (this->cursor.hasSelection())
    {
        startBlock = this->cursor.document()->findBlock(cursor.selectionStart());
        endBlock = this->cursor.document()->findBlock(cursor.selectionEnd());
    }

    QString indent = QString(4, ' ');  // Create a string with 4 spaces

    // Iterate over each selected block and remove up to `indentSpaces` spaces
    for (QTextBlock block = startBlock; block != endBlock.next(); block = block.next())
    {
        QString blockText = block.text();
        int leadingSpacesCount = 0;

        // Count how many leading spaces the block has
        while (leadingSpacesCount < blockText.length() && blockText.at(leadingSpacesCount) == ' ' && leadingSpacesCount < 4)
        {
            leadingSpacesCount++;
        }

        if (leadingSpacesCount > 0)
        {
            // Remove the leading spaces from the block
            QTextCursor blockCursor(block);
            blockCursor.movePosition(QTextCursor::StartOfBlock);
            blockCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, leadingSpacesCount);
            blockCursor.removeSelectedText();  // Remove the leading spaces
        }
        else if (blockText.isEmpty() && block == endBlock)
        {
            // If the block is empty, simply add indentation
            QTextCursor blockCursor(block);
            blockCursor.movePosition(QTextCursor::StartOfBlock);
            blockCursor.insertText(indent);  // Insert indentation if the line is empty
        }
    }

    this->cursor.endEditBlock();
}

void LuaHighlighter::breakLine()
{
    Q_EMIT insertingNewLineChanged(true); // Emit signal before inserting a new line

    this->cursor.beginEditBlock();

    // Get the current block and determine the cursor's position within it
    QTextBlock currentBlock = this->cursor.block();
    int positionInBlock = this->cursor.position() - currentBlock.position();

    // Get the text of the current line up to the cursor's position
    QString currentBlockText = currentBlock.text().left(positionInBlock);

    // Insert a line break at the cursor's current position
    this->cursor.insertText("\n");

    // Determine indentation level by counting leading spaces in the current line up to the cursor position
    int indentationLevel = 0;
    while (indentationLevel < currentBlockText.length() && currentBlockText[indentationLevel] == ' ')
    {
        indentationLevel++;
    }

    // Insert the same number of spaces at the start of the new line
    if (indentationLevel > 0)
    {
        this->cursor.insertText(QString(indentationLevel, ' '));
    }

    this->cursor.endEditBlock();

    Q_EMIT insertingNewLineChanged(false); // Emit signal after the new line is inserted
}

void LuaHighlighter::searchInTextEdit(const QString& searchText, bool wholeWord, bool caseSensitve)
{
    this->searchText = searchText; // Store the search text as a member variable
    this->wholeWord = wholeWord;
    this->caseSensitiv = caseSensitve;

    this->rehighlight();
}

void LuaHighlighter::replaceInTextEdit(const QString& searchText, const QString& replaceText)
{
    if (searchText.isEmpty())
    {
        return; // Exit if search text is empty
    }

    this->searchText = searchText;
    this->replaceText = replaceText;

    // Rehighlight after making replacements to refresh the syntax highlighting
    rehighlight();
}

void LuaHighlighter::replaceInBlock(int searchStart)
{
    // Begin an edit block to group changes for undo/redo
    this->cursor.beginEditBlock();

    QTextCursor cursor = this->cursor;
    QTextBlock block = currentBlock();  // Get the current block

    // Calculate the correct position within the block for replacing
    cursor.setPosition(block.position() + searchStart, QTextCursor::MoveAnchor);
    cursor.setPosition(block.position() + searchStart + searchText.length(), QTextCursor::KeepAnchor);

    // Ensure we are within the valid range of the document
    if (cursor.hasSelection() && cursor.selectedText() == searchText)
    {
        cursor.insertText(replaceText);  // Replace the selected text
    }

    this->cursor = cursor;  // Update the cursor after the replacement

    // End the edit block to finalize changes (allows for undo/redo)
    this->cursor.endEditBlock();
}

void LuaHighlighter::highlightBlock(const QString& text)
{
    // Apply highlighting rules first
    foreach (const HighlightingRule &rule, this->highlightingRules)
    {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext())
        {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    this->errorAlreadyCleared = false;

    // Only apply error formatting if we are on the error line
    if (this->errorLine != -1)
    {
        // Get the current block number to check if it matches errorLine
        int currentBlockNumber = currentBlock().blockNumber();

        // If current block number matches the error line, apply the error format
        if (currentBlockNumber == this->errorLine - 1)
        {
            // Format is per block! so the current block starts with 0, the next one too!
            // Set the format for the entire block (line)
            setFormat(0, currentBlock().text().length(), this->errorFormat);  // 0 = start of line, text.length() = end of line
        }
    }

    // Bracket highlighting logic

    QStack<int> bracketStack;

    // Loop over the text to check for unbalanced brackets
    for (int i = 0; i < text.length(); ++i)
    {
        QChar ch = text.at(i);

        // Track opening brackets
        if (ch == '(')
        {
            bracketStack.push(i);  // Store the position of opening brackets
        }
        // Match closing brackets
        else if (ch == ')')
        {
            if (!bracketStack.isEmpty())
            {
                bracketStack.pop();  // Match found, pop the corresponding opening bracket
            }
        }
    }

    // If there are unmatched opening brackets, mark them red
    while (!bracketStack.isEmpty())
    {
        int pos = bracketStack.pop();  // Get the position of the unbalanced opening bracket
        setFormat(pos, 1, this->errorFormat);  // Highlight the opening bracket in red
    }

    // Only match for the current block (line)
    if (cursor.block() == currentBlock())
    {
        this->highlightMatchingBrackets();
    }

    // Search highlight logic (for highlighting search terms)
    if (!this->searchText.isEmpty())
    {
        int searchStart = 0;
        QTextCharFormat searchFormat;
        searchFormat.setBackground(Qt::yellow);  // Highlight format for search results

        auto searchFlags = Qt::CaseInsensitive;
        if (true == this->caseSensitiv)
        {
            searchFlags = Qt::CaseSensitive;
        }

        // Loop through the current block to find all matches for the search term
        while ((searchStart = text.indexOf(this->searchText, searchStart, searchFlags)) != -1)
        {
            // Check if "whole word" match is enabled
            if (true == this->wholeWord && false == this->isWholeWord(text, searchStart, this->searchText.length()))
            {
                // If it's not a whole word match, skip this occurrence
                searchStart += this->searchText.length();
                continue;
            }

            // Apply the format to highlight the search text
            setFormat(searchStart, this->searchText.length(), searchFormat);

            // If replaceText is not empty, replace the found text
            if (!this->replaceText.isEmpty())
            {
                this->replaceInBlock(searchStart);
            }

            // Move the start position to after the current match to avoid infinite loop
            searchStart += this->searchText.length();
        }
    }
}

// Function to check if the search result is a whole word
bool LuaHighlighter::isWholeWord(const QString &text, int startIndex, int length)
{
    // Get characters before and after the search result
    QChar beforeChar = startIndex > 0 ? text.at(startIndex - 1) : QChar();
    QChar afterChar = (startIndex + length < text.length()) ? text.at(startIndex + length) : QChar();

    // Check if they are word boundaries (non-alphanumeric characters or start/end of text)
    bool isWordBoundaryBefore = beforeChar.isNull() || !beforeChar.isLetterOrNumber();
    bool isWordBoundaryAfter = afterChar.isNull() || !afterChar.isLetterOrNumber();

    return isWordBoundaryBefore && isWordBoundaryAfter;
}
