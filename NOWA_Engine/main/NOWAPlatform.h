#ifndef NOWA_PLATFORM_H
#define CORE_H

#include <string>

namespace NOWA::Platform
{
    // Paths
    std::string getUserDataDir();      // e.g. %APPDATA%/NOWA or ~/.local/share/NOWA
    bool ensureDirExists(const std::string& absDir);

    // Shell
    bool openFolderInFileManager(const std::string& absDir);

    // Cursor / window (start with no-op on Linux if needed)
    void setOsCursorVisible(bool visible);
}

#endif
