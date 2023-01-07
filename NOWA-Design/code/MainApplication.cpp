#include "NOWAPrecompiled.h"
#include "MainApplication.h"
#include "main/AppStateManager.h"
#include "DesignState.h"
#include "main/Core.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include "../res/resource.h"
#endif

MainApplication::MainApplication()
{
}

MainApplication::~MainApplication()
{
	// Applictation state manager must always be deleted before core will be deleted in order to avoid ugly side effects
	if (NOWA::AppStateManager::getSingletonPtr())
	{
		// delete core singleton
		delete NOWA::AppStateManager::getSingletonPtr();
	}
	if (NOWA::Core::getSingletonPtr())
	{
		// delete core singleton
		delete NOWA::Core::getSingletonPtr();
	}
}

void MainApplication::startSimulation(const Ogre::String& configName)
{
	// Create framework singleton
	new NOWA::Core();

	// Second the application state manager
	new NOWA::AppStateManager();
	// Note: This are special case singletons, since they are created and deleted manually, to avoid side effects when e.g. core would be else maybe deleted before AppStateManager

	// Feed core configuration with some information
	NOWA::CoreConfiguration coreConfiguration;
	coreConfiguration.graphicsConfigName = configName; // transmitted via args in main, since may variate when used network scenario
	coreConfiguration.wndTitle = "NOWA_Design";
	coreConfiguration.resourcesName = "NOWA_Design.cfg";
	coreConfiguration.customConfigName = "defaultConfig.xml";
	coreConfiguration.isGame = false;

	// Initialize ogre
	bool isInitializedCorrectly = NOWA::Core::getSingletonPtr()->initialize(coreConfiguration);
	if (false == isInitializedCorrectly)
	{
		return;
	}

	// Add a window icon
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	HWND hwnd;
	NOWA::Core::getSingletonPtr()->getOgreRenderWindow()->getCustomAttribute("WINDOW", (void*)&hwnd);
	LONG iconID = (LONG)LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(IDI_ICON1));
#if OGRE_ARCH_TYPE == OGRE_ARCHITECTURE_64
	SetClassLong(hwnd, GCLP_HICON, iconID);
#else
	SetClassLong(hwnd, GCL_HICON, iconID);
#endif
#endif

	Ogre::LogManager::getSingletonPtr()->logMessage("Simulation initialized!");

	// Create states
	// Attention: always use NOWA:: + name of the state
	DesignState::create(NOWA::AppStateManager::getSingletonPtr(), "DesignState", "DesignState");

	// Lets start with the Design state
	NOWA::AppStateManager::getSingletonPtr()->start("DesignState", false, NOWA::AppStateManager::ADAPTIVE /*NOWA::AppStateManager::RESTRICTED_INTERPOLATED*/);
}