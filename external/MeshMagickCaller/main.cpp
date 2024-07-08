#include <windows.h>
#include <iostream>
#include <string>
#include <DbgHelp.h>

using namespace std;

// Function to get the directory of the executable
std::string getExecutablePath()
{
    char buffer[MAX_PATH];
    GetModuleFileName(NULL, buffer, MAX_PATH);
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");
    return std::string(buffer).substr(0, pos);
}

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

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Set up the environment block to use the old OgreMain.dll path
    std::string env = "PATH=" + getExecutablePath() + ";" + getenv("PATH");
    LPSTR envBlock = const_cast<char*>(env.c_str());

    std::string tempParameters = "C:/Users/lukas/Documents/NOWA-Engine/development/MeshUtils/1.4.1/MeshMagick.exe transform -rotate=\"90 0 0\" -in=C:/Users/lukas/Documents/NOWA-Engine/media/models/Vehicles/Viper.mesh out=C:/Users/lukas/Documents/NOWA-Engine/media/models/// Vehicles/Viper.mesh_rotated.mesh";

    // std::string meshMagickCmd = "..\\old\\MeshMagick.exe transform -rotate=\"90 0 0\" -in=..\\meshes\\your_mesh.mesh -out=..\\meshes\\your_mesh_rotated.mesh";

#if 1
    // Create the process
    if (!CreateProcess(applicationFilePathName.data(), const_cast<char*>(parameters.data()), NULL, NULL, FALSE, CREATE_UNICODE_ENVIRONMENT, envBlock, getExecutablePath().data(), &si, &pi))
    // if (!CreateProcess("MeshMagick.exe", const_cast<char*>(parameters.data()), NULL, NULL, FALSE, CREATE_UNICODE_ENVIRONMENT, envBlock, NULL, &si, &pi))
    {
        std::string message = "Unable to open file: " + applicationFilePathName;
        MessageBox(0, message.data(), "Error", MB_OK);
        return false;
}

    // Wait for the process to complete
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

#else

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
#endif
    return 0;
}