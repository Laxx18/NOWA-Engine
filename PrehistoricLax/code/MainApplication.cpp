#include "NOWAPrecompiled.h"
#include "MainApplication.h"
#include "main/AppStateManager.h"
#include "IntroState.h"
#include "MenuState.h"
#include "LoadMenuState.h"
#include "ConfigurationState.h"
#include "PrehistoryState.h"
#include "GameState.h"
#include "main/Core.h"
#include "modules/GraphicsModule.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include "../res/resource.h"
#endif

MainApplication::MainApplication()
	: renderInitialized(false)
{
}

MainApplication::~MainApplication()
{
	if (NOWA::AppStateManager::getSingletonPtr())
	{
		delete NOWA::AppStateManager::getSingletonPtr();
	}

	NOWA::GraphicsModule::getInstance()->doCleanup();

	if (NOWA::Core::getSingletonPtr())
	{
		delete NOWA::Core::getSingletonPtr();
	}
}

void MainApplication::startSimulation(const Ogre::String& graphicsConfigName)
{
	// Create framework singleton
	new NOWA::Core();

	// Second the application state manager
	new NOWA::AppStateManager();
	
	// Feed core configuration with some information
	NOWA::CoreConfiguration coreConfiguration;
	if (true == graphicsConfigName.empty())
	{
		coreConfiguration.graphicsConfigName = "PrehistoricLaxGraphics.cfg";
	}
	else
	{
		coreConfiguration.graphicsConfigName = graphicsConfigName;  // transmitted via args in main, since may variate when used network scenario
	}
	coreConfiguration.wndTitle = "PrehistoricLax";
	// NOWADesign -> Edit -> Deploy, then use this line:
	// coreConfiguration.resourcesName = "PrehistoricLaxDeployed.cfg";
	coreConfiguration.resourcesName = "PrehistoricLax.cfg";
	coreConfiguration.customConfigName = "PrehistoricLaxConfig.xml";
	coreConfiguration.isGame = true;
	
	bool isInitializedCorrectly = NOWA::Core::getSingletonPtr()->initialize(coreConfiguration);
	if (false == isInitializedCorrectly)
	{
		return;
	}
	// Add application icon
	NOWA::Core::getSingletonPtr()->createApplicationIcon(IDI_ICON1);

	Ogre::LogManager::getSingletonPtr()->logMessage("TemplateName initialized!");

	// Create states
	// Attention: always use NOWA:: + name of the state if the class is located in the NOWA workspace.
	// If another state shall be used, do e.g.:
	IntroState::create(NOWA::AppStateManager::getSingletonPtr(), "IntroState", "IntroState");
	MenuState::create(NOWA::AppStateManager::getSingletonPtr(), "MenuState", "MenuState");
	LoadMenuState::create(NOWA::AppStateManager::getSingletonPtr(), "LoadMenuState", "LoadMenuState");
	ConfigurationState::create(NOWA::AppStateManager::getSingletonPtr(), "ConfigurationState", "ConfigurationState");
	// LoadMenuState::create(NOWA::AppStateManager::getSingletonPtr(), "LoadMenuState", "LoadMenuState");
	// SaveMenuState::create(NOWA::AppStateManager::getSingletonPtr(), "SaveMenuState", "SaveMenuState");
	PrehistoryState::create(NOWA::AppStateManager::getSingletonPtr(), "PrehistoryState", "PrehistoryState");
	// GameState::create(NOWA::AppStateManager::getSingletonPtr(), "GameState", "GameState");

	// Lets start with the Game
	NOWA::AppStateManager::getSingletonPtr()->start("IntroState", false);
}