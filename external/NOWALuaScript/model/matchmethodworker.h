#ifndef MATCHMETHODWORKER_H
#define MATCHMETHODWORKER_H

#include <QObject>

class LuaEditorModelItem;

class MatchMethodWorker : public QObject
{
    Q_OBJECT
public:
    MatchMethodWorker(LuaEditorModelItem* luaEditorModelItem, const QString& matchedClassName, const QString& typedAfterColon, int cursorPosition, int mouseX, int mouseY);

    void setParameters(const QString& matchedClassName, const QString& textAfterColon, int cursorPos, int mouseX, int mouseY);

    // Method to stop the processing
    void stopProcessing(void);
public slots:
    void process(void);

Q_SIGNALS:
    void finished();

    void signal_requestProcess();
private:
    LuaEditorModelItem* luaEditorModelItem;
    QString typedAfterColon;
    int cursorPosition;
    int mouseX;
    int mouseY;
    QString matchedClassName;
    bool isProcessing; // Flag to track processing state
    bool isStopped;    // Flag to signal processing should stop
};

#endif // MATCHMETHODWORKER_H
