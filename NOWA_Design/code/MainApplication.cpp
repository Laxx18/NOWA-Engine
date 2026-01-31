#include "NOWAPrecompiled.h"
#include "MainApplication.h"
#include "main/AppStateManager.h"
#include "DesignState.h"
#include "main/Core.h"
#include "modules/GraphicsModule.h"

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
	NOWA::GraphicsModule::getInstance()->doCleanup();
}

void MainApplication::startSimulation(const Ogre::String& configName)
{
	this->configName = configName;

	// Create framework singleton
	new NOWA::Core();

	// Second the application state manager
	new NOWA::AppStateManager();
	// Note: This are special case singletons, since they are created and deleted manually, to avoid side effects when e.g. core would be else maybe deleted before AppStateManager

	// Feed core configuration with some information
	NOWA::CoreConfiguration coreConfiguration;
	coreConfiguration.graphicsConfigName = this->configName; // transmitted via args in main, since may variate when used network scenario
	coreConfiguration.wndTitle = "NOWA_Design";
	coreConfiguration.resourcesName = "NOWA_Design.cfg";
	coreConfiguration.customConfigName = "defaultConfig.xml";
	coreConfiguration.isGame = false;

	NOWA::Core::getSingletonPtr()->setRenderThreadId(std::this_thread::get_id());

	bool isInitializedCorrectly = NOWA::Core::getSingletonPtr()->initialize(coreConfiguration);
	if (false == isInitializedCorrectly)
	{
		return;
	}

	NOWA::Core::getSingletonPtr()->createApplicationIcon(IDI_ICON1);

	Ogre::LogManager::getSingletonPtr()->logMessage("Simulation initialized!");
	
	// Create states
	// Attention: always use NOWA:: + name of the state
	DesignState::create(NOWA::AppStateManager::getSingletonPtr(), "DesignState", "DesignState");

	// Lets start with the Design state
	NOWA::AppStateManager::getSingletonPtr()->start("DesignState", false);
}