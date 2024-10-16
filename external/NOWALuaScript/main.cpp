#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "luascriptcontroller.h"
#include "luascriptqmladapter.h"
#include "luascriptadapter.h"
#include "model/luaeditormodel.h"
#include "qml/luaeditorqml.h"

#include <QDebug>

#ifdef Q_OS_WIN
    #include <Windows.h>
#endif

#ifdef Q_OS_WIN

void showWindowsMessageBox(const QString& title, const QString& message)
{
#ifdef Q_OS_WIN
    MessageBox(
        nullptr,
        reinterpret_cast<LPCWSTR>(message.utf16()),
        reinterpret_cast<LPCWSTR>(title.utf16()),
        MB_OK | MB_ICONINFORMATION
    );
#endif
}

class NativeEventFilter : public QAbstractNativeEventFilter
{
public:
    void setLuaScriptController(QSharedPointer<LuaScriptController> ptrLuaScriptController)
    {
        this->ptrLuaScriptController = ptrLuaScriptController;
    }

    virtual bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override
    {
        Q_UNUSED(result)
        if (eventType == "windows_generic_MSG")
        {
            MSG* msg = static_cast<MSG*>(message);
            if (msg->message == WM_COPYDATA)
            {
                COPYDATASTRUCT* cds = reinterpret_cast<COPYDATASTRUCT*>(msg->lParam);
                if (cds)
                {
                    // Parse the received data
                    QString receivedData = QString::fromUtf8(static_cast<char*>(cds->lpData));

                    // Split the received data into message ID and Lua script path
                    QStringList parts = receivedData.split('|');
                    if (parts.size() == 2)
                    {
                        QString messageId = parts[0];
                        QString receivedLuaScriptPath = parts[1];

                        // Check for your custom message ID
                        if (messageId == "LuaScriptPath")
                        {
                            // Handle the Lua script path
                            qDebug() << "Received Lua script path:" << receivedLuaScriptPath;

                            // Show Windows MessageBox
                            // showWindowsMessageBox("Received via sendMessage", "Received Lua script path: " + receivedLuaScriptPath);

                            // If NOWALuaScript.exe is opened, a message is send out from NOWA-Design with a lua path. Add it!
                            this->ptrLuaScriptController->slot_createLuaScript(receivedLuaScriptPath);

                            // Emit signals or trigger actions in NOWALuaScript
                            return true;  // Message handled
                        }
                    }
                }
            }
        }
        return false;
    }
private:
    QSharedPointer<LuaScriptController> ptrLuaScriptController;
};
#endif


int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName("NOWA");
    app.setOrganizationDomain("https://lukas-kalinowski.com");
    app.setApplicationName("NOWALuaScript");

    QQmlApplicationEngine engine;
    // For custom modules
    engine.addImportPath(QStringLiteral("qrc:/qml_files"));

    qmlRegisterSingletonType<LuaScriptQmlAdapter>("NOWALuaScript", 1, 0, "LuaScriptQmlAdapter", LuaScriptQmlAdapter::getSingletonTypeProvider);
    qmlRegisterSingletonType<LuaEditorModel>("NOWALuaScript", 1, 0, "NOWALuaEditorModel", LuaEditorModel::getSingletonTypeProvider);
    qmlRegisterUncreatableType<LuaEditorModelItem>("NOWALuaScript", 1, 0, "LuaScriptModelItem", "Not ment to be created in qml.");
    // Register LuaEditorQml as a QML type
    qmlRegisterType<LuaEditorQml>("NOWALuaScript", 1, 0, "LuaEditorQml");

    QString initialLuaScriptPath;
    // Handle initial Lua script path from command-line arguments
    if (argc > 1)
    {
        initialLuaScriptPath = argv[1]; // First argument is the Lua script file path
        qDebug() << "Opening Lua script at:" << initialLuaScriptPath;
        // Handle the initial Lua script (open in editor)

        // showWindowsMessageBox("Received via args Lua Script", "Received Lua script at: " + initialLuaScriptPath);
    }

    QSharedPointer<LuaScriptAdapter> ptrLuaScriptAdapter(new LuaScriptAdapter());
    QSharedPointer<LuaScriptController> ptrLuaScriptController(new LuaScriptController(&engine, ptrLuaScriptAdapter, &app));

    const QUrl url(QStringLiteral("qrc:/qml_files/Main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); },
    Qt::QueuedConnection);
    engine.load(url);

#ifdef Q_OS_WIN
    // Install the native event filter
    NativeEventFilter nativeEventFilter;
    nativeEventFilter.setLuaScriptController(ptrLuaScriptController);
    app.installNativeEventFilter(&nativeEventFilter);
#endif

    // After QML is ready, set a potential first lua script and load it, if it comes from the args
    if (false == initialLuaScriptPath.isEmpty())
    {
        ptrLuaScriptController->slot_createLuaScript(initialLuaScriptPath);
    }

    return app.exec();
}
