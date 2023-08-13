#include "NOWAPrecompiled.h"
#include "MainApplication.h"
#include <string>

// #define MEMORY_LEAK_DETECTION

#ifdef MEMORY_LEAK_DETECTION
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <iostream>
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"

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
		MessageBoxA(0, e.getFullDescription().c_str(), "An Ogre exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
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
		MessageBoxA(0, e.getFullDescription().c_str(), "An Ogre exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
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
		MessageBoxEx(0, "An unknown exception has occured!", "Unknown exception", MB_OK, MB_OK | MB_ICONERROR | MB_TASKMODAL);
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
	catch (...)
	{
		// Destroys ogrenewt and newton before throwing, as else it will cause trouble in a thread deep inside newton.
		NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->destroyContent();
		std::cerr << "An unknown exception has occured.");
	}

    return 0;
}
#endif