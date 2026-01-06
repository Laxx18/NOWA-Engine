#include "NOWAPrecompiled.h"
#include "NOWAPlatform.h"

#include <filesystem>
#include <system_error>
#include <cstdlib>

#if defined(_WIN32)
#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

namespace NOWA::Platform
{
    std::string getUserDataDir()
    {
#if defined(_WIN32)
        char path[MAX_PATH]{};
        if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, path)))
            return std::string(path);
        return ".";
#elif defined(__linux__)
        if (const char* xdg = std::getenv("XDG_DATA_HOME"))
            return std::string(xdg);
        if (const char* home = std::getenv("HOME"))
            return std::string(home) + "/.local/share";
        return ".";
#else
        return ".";
#endif
    }

    bool ensureDirExists(const std::string& absDir)
    {
        namespace fs = std::filesystem;
        std::error_code ec;
        fs::create_directories(fs::u8path(absDir), ec);
        return !ec;
    }

    bool openFolderInFileManager(const std::string& absDir)
    {
#if defined(_WIN32)
        return (reinterpret_cast<intptr_t>(
            ShellExecuteA(nullptr, "open", absDir.c_str(), nullptr, nullptr, SW_SHOWNORMAL)
            ) > 32);
#elif defined(__linux__)
        // Works on most desktops
        std::string cmd = "xdg-open \"" + absDir + "\" >/dev/null 2>&1 &";
        return std::system(cmd.c_str()) == 0;
#else
        (void)absDir;
        return false;
#endif
    }

    void setOsCursorVisible(bool visible)
    {
#if defined(_WIN32)
        if (visible) { while (ShowCursor(TRUE) < 0) {} }
        else { while (ShowCursor(FALSE) >= 0) {} }
#else
        // Start with a no-op; later you can implement via SDL/X11 depending on your windowing backend.
        (void)visible;
#endif
    }
}

