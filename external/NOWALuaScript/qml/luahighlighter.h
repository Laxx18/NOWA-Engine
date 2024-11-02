#ifndef LUAHIGHLIGHTER_H
#define LUAHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QRegularExpression>
#include <QTextCharFormat>
#include <QQuickItem>

class LuaHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit LuaHighlighter(QQuickItem* luaEditorTextEdit, QObject* parent = Q_NULLPTR);

    QTextCursor getCursor();

    void setErrorLine(int line, int start, int end);

    void clearErrors();

    void commentSelection();

    void uncommentSelection();

    void setCursorPosition(int cursorPosition);

    void addTabToSelection(void);

    void removeTabFromSelection(void);

    void breakLine(void);

    void searchInTextEdit(const QString& searchText, bool wholeWord, bool caseSensitive);

    void replaceInTextEdit(const QString& searchText, const QString& replaceText);

    void clearSearch(void);

    void undo(void);

    void redo(void);

    void insertSentText(int sizeToReplace, const QString& text);
Q_SIGNALS:
    void insertingNewLineChanged(bool isInserting);
protected:
    void highlightBlock(const QString& text) override;

    void highlightMatchingBrackets();

    QPair<int, int> findMatchingBrackets(const QString& text, int cursorPos);

    void removeMatchingBracketsBold();

    void replaceInBlock(int searchStart);

    bool isWholeWord(const QString &text, int startIndex, int length);
private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    QQuickItem* luaEditorTextEdit;

    QVector<HighlightingRule> highlightingRules;
    int errorLine;
    int oldErrorLine;
    int errorStart;
    int errorEnd;
    QTextCharFormat errorFormat;

    bool errorAlreadyCleared;
    QTextCursor cursor;
    QString searchText;
    QString replaceText;
    bool wholeWord;
    bool caseSensitiv;
};

#endif // LUAHIGHLIGHTER_H
