#include "NOWAPrecompiled.h"
#include "MainApplication.h"
#include <string>

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"

INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
{
	try	
	{
		// Create the application
		MainApplication app;	
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
	catch( Ogre::Exception& e )
	{
		ShowCursor(true);
		MessageBoxA(0, e.getFullDescription().c_str(), "An Ogre exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
	}
	catch (std::exception& e)
	{
		MessageBoxEx(0, e.what(), "An exception has occured!", MB_OK, MB_OK | MB_ICONERROR | MB_TASKMODAL);
	}
	catch (...)
	{
		MessageBoxEx(0, "An unknown exception has occured!", "Unknown exception", MB_OK, MB_OK | MB_ICONERROR | MB_TASKMODAL);
	}

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
        std::cerr << "An Ogre exception has occured: " << e.getFullDescription();
    }
	catch (std::exception& e)
	{
		std::cerr << "An exception has occured!" << e.getFullDescription();
	}
	catch (...)
	{
		std::cerr << "An unknown exception has occured.");
	}

    return 0;
}
#endif