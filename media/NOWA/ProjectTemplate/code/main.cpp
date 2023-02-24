#include "NOWAPrecompiled.h"
#include "MainApplication.h"
#include <string>

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"

INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
{
	// Creates the application
	MainApplication app;
	
	try	
	{
		// Checks if some args have been transmitted
		if (nullptr != strCmdLine)
		{
			Ogre::String strGraphicsConfigName(strCmdLine);
			app.startSimulation(strGraphicsConfigName);
		}
		else
		{
			app.startSimulation();
		}
	}
	catch( Ogre::Exception& e )
	{
		// Destroys ogrenewt and newton before throwing, as else it will cause trouble in a thread deep inside newton.
		NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->destroyContent();
		ShowCursor(true);
		MessageBoxA(0, e.getFullDescription().c_str(), "An Ogre exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
	}
	catch (...)
	{
		// Destroys ogrenewt and newton before throwing, as else it will cause trouble in a thread deep inside newton.
		NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->destroyContent();
		MessageBoxEx(0, "An unknown exception has occured!", "Unknown exception", MB_OK, MB_OK | MB_ICONERROR | MB_TASKMODAL);
	}

	return 0;
}

#else

int main(int argc, char **argv)
{
	// Creates the application
    MainApplication app;
    try
    {
        Ogre::String configFileName;

		// Checks if some args have been transmitted
        if (argc > 1)
        {
            configFileName = argv[1];
        }
        else
        {
            configFileName = "ogre.cfg";
        }
        app.startSimulation(configFileName);
    }
    catch( Ogre::Exception& e )
    {
		// Destroys ogrenewt and newton before throwing, as else it will cause trouble in a thread deep inside newton.
		NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->destroyContent();
        std::cerr << "An exception has occured: " << e.getFullDescription();
	}
	catch (...)
	{
		// Destroys ogrenewt and newton before throwing, as else it will cause trouble in a thread deep inside newton.
		NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->destroyContent();
		std::cerr << "An unknown exception has occured!";
	}
    return 0;
}

#endif