#include "NOWAPrecompiled.h"
#include "MainApplication.h"
#include "main/AppStateManager.h"
#include "GameState.h"
#include "main/Core.h"
#include "modules/RenderCommandQueueModule.h"

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
	if (NOWA::Core::getSingletonPtr())
	{
		delete NOWA::Core::getSingletonPtr();
	}
}

void MainApplication::renderThreadFunction(void)
{
	// Configure the render command queue
	auto* renderQueue = NOWA::RenderCommandQueueModule::getInstance();

	// Mark the current thread as the render thread
	renderQueue->markCurrentThreadAsRenderThread();

	// Feed core configuration with some information
	NOWA::CoreConfiguration coreConfiguration;
	if (true == graphicsConfigName.empty())
		coreConfiguration.graphicsConfigName = "ProjectTemplateGraphics.cfg";
	else
		coreConfiguration.graphicsConfigName = graphicsConfigName;  // transmitted via args in main, since may variate when used network scenario
	coreConfiguration.wndTitle = "ProjectTemplate";
	// NOWADesign -> Edit -> Deploy, then use this line:
	// coreConfiguration.resourcesName = "ProjectTemplateDeployed.cfg";
	coreConfiguration.resourcesName = "ProjectTemplate.cfg";
	coreConfiguration.customConfigName = "ProjectTemplateConfig.xml";

	NOWA::Core::getSingletonPtr()->setRenderThreadId(std::this_thread::get_id());

	bool isInitializedCorrectly = NOWA::Core::getSingletonPtr()->initialize(coreConfiguration);

	// Notify that initialization is complete
	{
		std::lock_guard<std::mutex> lock(this->renderInitMutex);
		this->renderInitialized = true;
	}
	this->renderInitCondition.notify_one();

	if (false == isInitializedCorrectly)
	{
		return;
	}

	// Set timeout duration (e.g., 10 seconds for complex operations)
	renderQueue->setTimeoutDuration(std::chrono::milliseconds(10000));
	renderQueue->setFrameTime(NOWA::Core::getSingletonPtr()->getOptionDesiredSimulationUpdates());

	// Enable or disable timeout based on build configuration
#ifdef _DEBUG
	// In debug builds, you might want to disable timeout for easier debugging
	renderQueue->enableTimeout(false);
	// And set a more verbose log level
	renderQueue->setLogLevel(Ogre::LML_TRIVIAL);
	// renderQueue->enableDebugVisualization(true);
#else
	// In release builds, keep timeout enabled with normal logging
	// renderQueue->enableTimeout(true);
	renderQueue->setLogLevel(Ogre::LML_NORMAL);
#endif

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

	static int frameCount = 0;

	// Render thread loop
	// Initialize timer for frame time calculation
	Ogre::Timer timer;
	Ogre::uint64 lastFrameTime = timer.getMicroseconds();

	// Render thread loop
	while (false == NOWA::Core::getSingletonPtr()->isShutdown())
	{
		// Process signals from system
		Ogre::WindowEventUtilities::messagePump();

		// Process all waiting render commands before rendering
		renderQueue->processAllCommands();

		// Calculate delta time
		Ogre::uint64 currentTime = timer.getMicroseconds();
		Ogre::Real deltaTime = (currentTime - lastFrameTime) * 0.000001f; // Convert to seconds
		lastFrameTime = currentTime;

		// Update accumulated time in render command queue
		// This is critical for proper interpolation
		Ogre::Real currentAccumTime = renderQueue->getAccumTimeSinceLastLogicFrame();
		renderQueue->setAccumTimeSinceLastLogicFrame(currentAccumTime + deltaTime);

		// Calculate interpolation weight based on time since last logic frame
		Ogre::Real interpolationWeight = renderQueue->calculateInterpolationWeight();

		// Update transforms with interpolation
		renderQueue->updateAllTransforms(interpolationWeight);

		// Perform the actual rendering
		Ogre::Root::getSingletonPtr()->renderOneFrame();

		// Periodically dump buffer state for debugging (every 300 frames = 5 seconds at 60 FPS)
		if (++frameCount % 300 == 0)
		{
			renderQueue->waitForRenderCompletion();
			// Uncomment for debugging
			renderQueue->dumpBufferState();
			frameCount = 0;  // Reset counter
		}
	}

	// Process any remaining commands before fully exiting, so that code cleanup like DesignState::exit can be processed on a queue, even the render game loop is already gone
	renderQueue->processAllCommands();
}

void MainApplication::startSimulation(const Ogre::String& graphicsConfigName)
{
	// Create framework singleton
	new NOWA::Core();

	// Second the application state manager
	new NOWA::AppStateManager();
	
	std::thread renderThread(&MainApplication::renderThreadFunction, this);

	// Wait until render thread initializes Ogre/Core
	{
		std::unique_lock<std::mutex> lock(this->renderInitMutex);
		this->renderInitCondition.wait(lock, [this] { return this->renderInitialized; });
	}

	// Add application icon
	NOWA::Core::getSingletonPtr()->createApplicationIcon(IDI_ICON1);

	Ogre::LogManager::getSingletonPtr()->logMessage("TemplateName initialized!");

	// Create states
	// Attention: always use NOWA:: + name of the state if the class is located in the NOWA workspace.
	// If another state shall be used, do e.g.:
	// IntroState::create(NOWA::AppStateManager::getSingletonPtr(), "IntroState", "GameState");
	// NOWA::AppStateManager::getSingletonPtr()->start("IntroState", false, NOWA::AppStateManager::MULTI_THREADED);
	GameState::create(NOWA::AppStateManager::getSingletonPtr(), "GameState", "GameState");

	// Lets start with the Game
	NOWA::AppStateManager::getSingletonPtr()->start("GameState", false, NOWA::AppStateManager::MULTI_THREADED);
	
	renderThread.detach();
}