#include "NOWAPrecompiled.h"
#include "MainApplication.h"
#include <string>
#include <exception>

// #define MEMORY_LEAK_DETECTION

#ifdef MEMORY_LEAK_DETECTION
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <iostream>
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#pragma comment(lib, "dbghelp.lib")
#include <dbghelp.h>

#include <windows.h>
#include <dbghelp.h>
#include <iostream>
#include <sstream>


#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

void printStackTrace(const Ogre::String& errorMessage, const Ogre::String& title)
{
	const int max_frames = 128;
	void* stack[max_frames];
	HANDLE process = GetCurrentProcess();
	SymInitialize(process, NULL, TRUE);

	USHORT frames = CaptureStackBackTrace(0, max_frames, stack, NULL);

	SYMBOL_INFO* symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
	symbol->MaxNameLen = 255;
	symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

	IMAGEHLP_LINE64 line;
	line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
	DWORD displacement;

	std::ostringstream oss;
	oss << "Error: " << errorMessage << "\nStacktrace:\n";

	for (USHORT i = 0; i < frames; ++i)
	{
		DWORD64 addr = (DWORD64)(stack[i]);

		if (SymFromAddr(process, addr, 0, symbol))
		{
			oss << symbol->Name;
		}
		else
		{
			oss << "[unknown symbol]";
		}

		if (SymGetLineFromAddr64(process, addr, &displacement, &line))
		{
			oss << " - 0x" << std::hex << addr
				<< " in " << line.FileName
				<< ":" << std::dec << line.LineNumber << "\n";
		}
		else
		{
			oss << " - 0x" << std::hex << addr << " (no line info)\n";
		}
	}

	free(symbol);
	SymCleanup(process);

	Ogre::String message = oss.str();

	MessageBoxA(0, message.c_str(), title.c_str(), MB_OK | MB_ICONERROR | MB_TASKMODAL);
}

INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
{
#ifdef MEMORY_LEAK_DETECTION
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	// Create the application
	MainApplication app;
	try	
	{
		// Check if some args have been transmitted
		if (nullptr != strCmdLine)
		{
			Ogre::String strConfigName(strCmdLine);
			app.startSimulation(strConfigName);
		}
		else
		{
			app.startSimulation();
		}
	}
	catch(Ogre::Exception& e)
	{
		// Destroys ogrenewt and newton before throwing, as else it will cause trouble in a thread deep inside newton.
		try
		{
			NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->destroyContent();
		}
		catch (...)
		{
		}
		ShowCursor(true);

		printStackTrace(e.getFullDescription(), "An Ogre exception has occured!");
	}
	catch (MyGUI::Exception& e)
	{
		try
		{
			NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->destroyContent();
		}
		catch (...)
		{
		}
		ShowCursor(true);

		printStackTrace(e.getFullDescription(), "An MyGUI exception has occured!");
	}
	catch (const std::exception& e)
	{
		try
		{
			NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->destroyContent();
		}
		catch (...)
		{
		}
		ShowCursor(true);

		printStackTrace(e.what(), "An std exception has occured!");
	}
	catch (...)
	{
		// Destroys ogrenewt and newton before throwing, as else it will cause trouble in a thread deep inside newton.
		try
		{
			NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->destroyContent();
		}
		catch (...)
		{
		}
		ShowCursor(true);

		printStackTrace("Unknown", "An unknown exception has occured!");
	}

#ifdef MEMORY_LEAK_DETECTION
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	_CrtDumpMemoryLeaks();
#endif

	return 0;
}
#else
int main(int argc, char **argv)
{
    MainApplication app;
    try
    {
        Ogre::String configFileName;

        if (argc > 1)
        {
            configFileName = argv[1];
			app.startSimulation(configFileName);
        }
        else
        {
			app.startSimulation();
        }
    }
    catch(Ogre::Exception& e)
    {
		// Destroys ogrenewt and newton before throwing, as else it will cause trouble in a thread deep inside newton.
		NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->destroyContent();
        std::cerr << "An Ogre exception has occured: " << e.getFullDescription();
    }
	catch (MyGUI::Exception& e)
	{
		try
		{
			NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->destroyContent();
		}
		catch (...)
		{
		}
		ShowCursor(true);
		std::cerr << e.getFullDescription().c_str() << "An Ogre exception has occured!";
	}
	catch (std::exception& e)
	{
		try
		{
			NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->destroyContent();
		}
		catch (...)
		{
		}
		ShowCursor(true);
		std::cerr << e.getFullDescription().c_str() << "An std exception has occured!";
	}
	catch (...)
	{
		// Destroys ogrenewt and newton before throwing, as else it will cause trouble in a thread deep inside newton.
		NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->destroyContent();
		std::cerr << "An unknown exception has occured.";
	}

    return 0;
}
#endif