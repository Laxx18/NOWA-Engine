#include <windows.h>
#include <iostream>
#include <string>
#include <DbgHelp.h>

using namespace std;

int main(int argc, char* argv[])
{
    std::string applicationFilePathName;
    std::string parameters;
    for (size_t i = 1; i < argc; i++)
    {
        if (i == 1)
        { 
            applicationFilePathName = argv[i];
        }
        else
        {
            parameters += argv[i] + std::string(" ");
        }
    }

    std::string info = "Using '" + applicationFilePathName + "' with parameters : '" + parameters + "'\n";
    cout << info;

    MessageBox(0, info.data(), "Info", MB_OK);

    SHELLEXECUTEINFO shRun = { 0 };
    shRun.cbSize = sizeof(SHELLEXECUTEINFO);
    // shRun.fMask = SEE_MASK_NOCLOSEPROCESS;
    shRun.fMask = SEE_MASK_NOASYNC | SEE_MASK_NO_CONSOLE;
    shRun.hwnd = NULL;
    shRun.lpVerb = NULL;
    shRun.lpFile = applicationFilePathName.data();
    // shRun.lpFile = "./OgreMeshMagick.exe";
    shRun.lpParameters = parameters.data();
    // shRun.lpDirectory = "C:/Users/lukas/Documents/NOWA-Engine/development/MeshUtils/1.8.1";
    shRun.nShow = SW_HIDE;
    shRun.hInstApp = NULL;

    // Execute the file with the parameters
    if (!ShellExecuteEx(&shRun))
    {
        std::string message = "Unable to open file: " + applicationFilePathName;
        MessageBox(0, message.data(), "Error", MB_OK);
    }
    return 0;
}