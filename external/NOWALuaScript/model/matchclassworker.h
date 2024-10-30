#ifndef MATCHCLASSWORKER_H
#define MATCHCLASSWORKER_H

#include <QObject>

class LuaEditorModelItem;

class MatchClassWorker : public QObject
{
    Q_OBJECT
public:
    MatchClassWorker(LuaEditorModelItem* luaEditorModelItem, const QString& currentText, const QString& typedAfterColon, int cursorPosition, int mouseX, int mouseY);

    void setParameters(const QString& currentText, const QString& textAfterColon, int cursorPos, int mouseX, int mouseY);

    // Method to stop the processing
    void stopProcessing(void);
public slots:
    void process(void);

Q_SIGNALS:
    void finished();

    void signal_requestProcess();

    void signal_deliverData(const QString& matchedClassName, const QString& matchedMethodName, const QString& textAfterColon, int cursorPos, int mouseX, int mouseY);
private:
    LuaEditorModelItem* luaEditorModelItem;
    QString currentText;
    QString typedAfterColon;
    int cursorPosition;
    int mouseX;
    int mouseY;
    QString matchedClassName;
    bool isProcessing; // Flag to track processing state
    bool isStopped;    // Flag to signal processing should stop
};

#endif // MATCHCLASSWORKER_H
