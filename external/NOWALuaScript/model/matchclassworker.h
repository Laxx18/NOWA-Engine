#ifndef MATCHCLASSWORKER_H
#define MATCHCLASSWORKER_H

#include <QObject>

class LuaEditorModelItem;

class MatchClassWorker : public QObject
{
    Q_OBJECT
public:
    MatchClassWorker(LuaEditorModelItem* luaEditorModelItem, bool forConstant, const QString& currentText, const QString& textAfterKeyword, int cursorPosition, int mouseX, int mouseY);

    void setParameters(bool forConstant, const QString& currentText, const QString& textAfterKeyword, int cursorPos, int mouseX, int mouseY);

    // Method to stop the processing
    void stopProcessing(void);
public slots:
    void process(void);

Q_SIGNALS:
    void finished();

    void signal_requestProcess();

    void signal_deliverData(const QString& matchedClassName, const QString& matchedMethodName, const QString& textAfterKeyword, int cursorPos, int mouseX, int mouseY);
private:
    QString handleMethods(void);

    void handleConstantsResults(void);
private:
    LuaEditorModelItem* luaEditorModelItem;
    QString currentText;
    QString typedAfterKeyword;
    int cursorPosition;
    int mouseX;
    int mouseY;
    bool forConstant;
    QString matchedClassName;
    bool isProcessing; // Flag to track processing state
    bool isStopped;    // Flag to signal processing should stop
};

#endif // MATCHCLASSWORKER_H
