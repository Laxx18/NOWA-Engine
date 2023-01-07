#include "NOWAPrecompiled.h"
#include "MainApplication.h"
#include "main/AppStateManager.h"
#include "GameState.h"
#include "main/Core.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include "../res/resource.h"
#endif

MainApplication::MainApplication()
{
}

MainApplication::~MainApplication()
{
	if (NOWA::AppStateManager::getSingletonPtr())
	{
		delete NOWA::AppStateManager::getSingletonPtr();
	}
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
		coreConfiguration.graphicsConfigName = "ProjectTemplateGraphics.cfg";
	else
		coreConfiguration.graphicsConfigName = graphicsConfigName;  // transmitted via args in main, since may variate when used network scenario
	coreConfiguration.wndTitle = "ProjectTemplate";
	coreConfiguration.resourcesName = "ProjectTemplate.cfg";
	coreConfiguration.customConfigName = "ProjectTemplateConfig.xml";
	// Initialize ogre
	bool isInitializedCorrectly = NOWA::Core::getSingletonPtr()->initialize(coreConfiguration);
	if (false == isInitializedCorrectly)
	{
		return;
	}

	// Add application icon
	NOWA::Core::getSingletonPtr()->createApplicationIcon(IDI_ICON1);

	Ogre::LogManager::getSingletonPtr()->logMessage("TemplateName initialized!");

	NOWA::AppStateManager::restrictFPS(true);

	// Create states
	// Attention: always use NOWA:: + name of the state if the class is located in the NOWA workspace.
	GameState::create(NOWA::AppStateManager::getSingletonPtr(), "GameState", "GameState");

	// Lets start with the Game
	NOWA::AppStateManager::getSingletonPtr()->start("GameState", false, NOWA::AppStateManager::ADAPTIVE);
}