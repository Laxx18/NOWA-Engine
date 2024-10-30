#ifndef LUAEDITORMODELITEM_H
#define LUAEDITORMODELITEM_H

#include <QObject>
#include <QMap>
#include <QThread>

#include "matchclassworker.h"
#include "matchmethodworker.h"

class LuaEditorModelItem : public QObject
{
    Q_OBJECT
public:
    Q_PROPERTY(QString filePathName READ getFilePathName NOTIFY filePathNameChanged)

    Q_PROPERTY(QString content READ getContent WRITE setContent NOTIFY contentChanged)

    Q_PROPERTY(QString title READ getTitle NOTIFY titleChanged)
public:
    struct LuaVariableInfo
    {
        QString name;   // Variable name
        QString type;   // Inferred type (initially empty, filled when type can be determined)
        int line = -1;       // Line number where the variable is declared
        QString scope;  // Scope (e.g., global, local)
    };
public:
    explicit LuaEditorModelItem(QObject* parent = Q_NULLPTR);

    virtual ~LuaEditorModelItem();

    QString getFilePathName() const;

    void setFilePathName(const QString& filePathName);

    QString getContent() const;

    void setContent(const QString& content);

    QString getTitle() const;

    void setTitle(const QString& title);

    bool getHasChanges(void) const;

    void setHasChanges(bool hasChanges);

    void restoreContent(void);

    void openProjectFolder(void);

    // void matchClass(const QString& currentText, int cursorPosition, int mouseX, int mouseY);

    QString extractWordBeforeColon(const QString& currentText, int cursorPosition);

    QString extractMethodBeforeColon(const QString& currentText, int cursorPosition);

    void detectVariables(void);

    LuaVariableInfo getClassForVariableName(const QString& variableName);

public slots:
    void startIntellisenseProcessing(const QString& currentText, const QString& textAfterColon, int cursorPos, int mouseX, int mouseY);

    void closeIntellisense(void);

    void startMatchedFunctionProcessing(const QString& textAfterColon, int cursorPos, int mouseX, int mouseY);

    void closeMatchedFunction();
Q_SIGNALS:
    void filePathNameChanged();

    void contentChanged();

    void titleChanged();

    void hasChangesChanged();

    void signal_commentLines();

    void signal_unCommentLines();

    void signal_addTabToSelection();

    void signal_removeTabFromSelection();

    void signal_breakLine();

    void signal_searchInTextEdit(const QString& searchText, bool wholeWord, bool caseSensitive);

    void signal_replaceInTextEdit(const QString& searchText, const QString& replaceText);

    void signal_clearSearch();

    void signal_undo();

    void signal_redo();

    void signal_sendTextToEditor(const QString& text);
private:
    // Helper function to resolve types for method chains
    QString resolveMethodChainType(const QString& objectVar);
private:
    QString filePathName;
    QString content;
    QString oldContent;
    QString title;
    bool hasChanges;
    bool firstTimeContent;
    QMap<QString, LuaVariableInfo> variableMap;

    MatchClassWorker* matchClassWorker;
    QThread* matchClassThread;

    MatchMethodWorker* matchMethodWorker;
    QThread* matchMethodThread;

    QString matchedClassName;
};

#endif // LUAEDITORMODELITEM_H
