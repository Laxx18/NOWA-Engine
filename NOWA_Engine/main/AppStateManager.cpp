#include "NOWAPrecompiled.h"
#include "AppStateManager.h"
#include "Core.h"
#include "InputDeviceCore.h"
#include "Events.h"
#include "modules/GameProgressModule.h"
#include "modules/InputDeviceModule.h"
#include "modules/GraphicsModule.h"
#include "ProcessManager.h"
#include "gameobject/AttributesComponent.h"

namespace
{
	enum eAppStateOperation
	{
		ChangeAppState = 0,
		PushAppState = 1,
		PopAppState = 2,
		PopAllAndPushAppState = 3,
		ExitGame = 4
	};
}

namespace NOWA
{

	class Timer
	{
	public:
		Timer()
			: lastTime(std::chrono::high_resolution_clock::now())
		{

		}

		// Get delta time between the last frame and the current one
		Ogre::Real getDeltaTime(void)
		{
			auto currentTime = std::chrono::high_resolution_clock::now();
			std::chrono::duration<Ogre::Real> delta = currentTime - lastTime;
			lastTime = currentTime;
			return delta.count();  // Return the delta time in seconds
		}

	private:
		std::chrono::high_resolution_clock::time_point lastTime;
	};

	//////////////////////////////////////////////////////////////

	class ChangeAppStateProcess : public NOWA::Process
	{
	public:
		explicit ChangeAppStateProcess(AppState* state, eAppStateOperation stateOperation)
			: state(state),
			stateOperation(stateOperation)
		{

		}

		explicit ChangeAppStateProcess(eAppStateOperation stateOperation)
			: state(nullptr),
			stateOperation(stateOperation)
		{

		}

	protected:
		virtual void onInit(void) override
		{
			this->succeed();

			boost::shared_ptr<EventDataLuaScriptModfied> eventDataLuaScriptModified(new EventDataLuaScriptModfied(0L, ""));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataLuaScriptModified);

			// Must abort all processes, if state is about to being changed, not that a prior state make e.g. delayed undo for all game objects at the same time
			ProcessManager::getInstance()->abortAllProcesses(true);

			GraphicsModule::getInstance()->clearSceneResources();

			// NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.25f));
			
			switch(this->stateOperation)
			{
				case eAppStateOperation::ChangeAppState:
				{
					AppStateManager::getSingletonPtr()->internalChangeAppState(this->state);
					break;
				}
				case eAppStateOperation::PushAppState:
				{
					AppStateManager::getSingletonPtr()->internalPushAppState(this->state);
					break;
				}
				case eAppStateOperation::PopAppState:
				{
					AppStateManager::getSingletonPtr()->internalPopAppState();
					break;
				}
				case eAppStateOperation::PopAllAndPushAppState:
				{
					AppStateManager::getSingletonPtr()->internalPopAllAndPushAppState(this->state);
					break;
				}
				case eAppStateOperation::ExitGame:
				{
					AppStateManager::getSingletonPtr()->internalExitGame();
					break;
				}
			}
		}

		virtual void onUpdate(float dt) override
		{
			this->succeed();
		}
	private:
		AppState* state;
		eAppStateOperation stateOperation;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	AppStateManager::AppStateManager()
		: renderWhenInactive(false),
		lastTime(0),
		desiredUpdates(60),
		slowMotionMS(0),
		renderDelta(0),
		vsyncOn(true),
		bShutdown(false),
		bStall(false),
		bCanProcessRenderQueue(true)
	{
	}

	AppStateManager::~AppStateManager()
	{
		// Delete all global attributes
		auto& it = this->globalAttributesMap.begin();

		while (it != this->globalAttributesMap.end())
		{
			Variant* globalAttribute = it->second;
			delete globalAttribute;
			globalAttribute = nullptr;
			++it;
		}
		this->globalAttributesMap.clear();
	}

	AppStateManager* AppStateManager::getSingletonPtr(void)
	{
		return msSingleton;
	}

	GameProgressModule* AppStateManager::getActiveGameProgressModuleSafe(void) const
	{
		return this->activeGameProgressModule.load();
	}

	AppStateManager& AppStateManager::getSingleton(void)
	{
		assert(msSingleton);
		return (*msSingleton);
	}

	void AppStateManager::manageAppState(Ogre::String stateName, AppState* state)
	{
		try
		{
			// Creates new state
			StateInfo newStateInfo;
			newStateInfo.name = stateName;
			newStateInfo.state = state;
			this->states.push_back(newStateInfo);
		}
		catch (std::exception &e)
		{
			delete state;
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AppStateManager] Error on managing a new state\n" + Ogre::String(e.what()));
			throw Ogre::Exception(Ogre::Exception::ERR_INTERNAL_ERROR, "[AppStateManager] Error on managing a new state\n" + Ogre::String(e.what()), "NOWA");
		}
	}

	void AppStateManager::start(const Ogre::String& stateName, bool renderWhenInactive)
	{
		this->renderWhenInactive = renderWhenInactive;

		AppState* appState = this->findByName(stateName);
		if (nullptr != appState)
		{
			this->internalChangeAppState(appState, true);

			auto options = Core::getSingletonPtr()->getOgreRoot()->getRenderSystem()->getConfigOptions();
			auto option = options.find("VSync");
			if (option != options.end())
			{
				if ("Yes" == option->second.currentValue)
				{
					this->vsyncOn = true;
				}
				else
				{
					this->vsyncOn = false;
				}
			}

			this->multiThreadedRendering();

			// end all present states
			while (false == this->activeStateStack.empty())
			{
				NOWA::GraphicsModule::getInstance()->clearAllClosures();

				this->bStall = true;
				this->activeStateStack.back()->exit();

				AppState* oldState = this->activeStateStack.back();
				InputDeviceCore::getSingletonPtr()->removeKeyListener(oldState);
				InputDeviceCore::getSingletonPtr()->removeMouseListener(oldState);
				InputDeviceCore::getSingletonPtr()->removeJoystickListener(oldState);

				this->activeStateStack.pop_back();
			}

			// remove all present states
			while (false == states.empty())
			{
				StateInfo si = this->states.back();
				si.state->destroy();
				this->states.pop_back();
			}

			Core::getSingletonPtr()->setShutdown(true);
		}
		else
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[AppStateManager] Error: Cannot start application state: '" + stateName
				+ "' because it does not exist.");
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[AppStateManager] Error: Cannot start application state: '" + stateName
				+ "' because it does not exist.\n", "NOWA");
		}
	}

	AppState* AppStateManager::findByName(const Ogre::String& stateName)
	{
		for (auto& it = this->states.cbegin(); it != this->states.cend(); ++it)
		{
			if (it->name == stateName)
			{
				return it->state;
			}
		}

		if (true == Core::getSingletonPtr()->getIsGame())
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[AppStateManager] Error: Cannot find the given app state name: '" + stateName + "'. Check if there is a typo or the app state name has changed.");
		}
		return nullptr;
	}

	AppState* AppStateManager::getNextState(AppState* currentAppState)
	{
		AppState* nextState = nullptr;
		for (auto& it = this->states.begin(); it != this->states.end(); ++it)
		{
			if (it->state == currentAppState)
			{
				nextState = (++it)->state;
			}
		}
		return nextState;
	}

	void AppStateManager::setDesiredUpdates(unsigned int desiredUpdates)
	{
		this->desiredUpdates = desiredUpdates;
		this->renderDelta = 0;
		Ogre::ConfigOptionMap& cfgOpts = Ogre::Root::getSingletonPtr()->getRenderSystem()->getConfigOptions();
		Core::getSingletonPtr()->getOgreRenderWindow()->setVSync(this->vsyncOn, Ogre::StringConverter::parseUnsignedInt(cfgOpts["VSync Interval"].currentValue));
	}

	void AppStateManager::enqueue(LogicCommand&& command)
	{
		if (true == this->isLogicThread())
		{
			command();
			return;
		}
		this->queue.enqueue(std::move(command));
	}

	void AppStateManager::enqueueAndWait(LogicCommand&& command)
	{
		// With this code its possible to call from render thread logic stuff with wait and from logic thread commands, which have inside other commands for render thread with wait!

		// If we are already on the logic thread, just run it directly
		if (this->isLogicThread())
		{
			command();
			return;
		}

		auto* graphics = NOWA::GraphicsModule::getInstance();
		const bool calledFromRenderThread = graphics && graphics->isRenderThread();

		// We can't use a std::promise with a reference here safely from multiple threads,
		// so we use flags + exception_ptr stored in this scope.
		std::atomic<bool> done{ false };
		std::exception_ptr exceptionPtr = nullptr;

		// This runs on the logic thread
		LogicCommand wrappedCommand = [cmd = std::move(command), &done, &exceptionPtr]() mutable
		{
			try
			{
				cmd();
			}
			catch (...)
			{
				exceptionPtr = std::current_exception();
			}

			done.store(true, std::memory_order_release);
		};

		// Enqueue command for the logic thread
		this->queue.enqueue(std::move(wrappedCommand));

		// CASE 1: Caller is a normal thread (not render thread)
		if (!calledFromRenderThread)
		{
			// Simple blocking wait for 'done'
			while (!done.load(std::memory_order_acquire))
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}

			if (exceptionPtr)
				std::rethrow_exception(exceptionPtr);

			return;
		}

		// CASE 2: Caller is the RENDER THREAD:
		// We MUST keep processing render commands while waiting,
		// because the logic command may call ENQUEUE_RENDER_COMMAND_*_WAIT

		while (!done.load(std::memory_order_acquire))
		{
			// 1) Process any pending render commands
			graphics->processAllCommands();

			// 2) Pump window events so OS stays happy
			Ogre::WindowEventUtilities::messagePump();

			// 3) Don't burn 100% CPU
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		if (exceptionPtr)
		{
			std::rethrow_exception(exceptionPtr);
		}
	}

	void AppStateManager::clearLogicQueue(void)
	{
		LogicCommand commandEntry;
		while (this->queue.try_dequeue(commandEntry))
		{

		}
	}

	void AppStateManager::markCurrentThreadAsLogicThread(void)
	{
		this->logicThreadId.store(std::this_thread::get_id());
	}

	bool AppStateManager::isLogicThread(void) const
	{
		return std::this_thread::get_id() == this->logicThreadId.load();
	}

	void AppStateManager::processAll(void)
	{
		LogicCommand commandEntry;
		while (this->queue.try_dequeue(commandEntry))
		{
			RenderGlobals::g_inLogicCommand = true;
			commandEntry(); // Execute the logic command
			RenderGlobals::g_inLogicCommand = false;
		}
	}

	void AppStateManager::multiThreadedRendering(void)
	{
		this->markCurrentThreadAsLogicThread();

		const double fixedDt = 1.0 / static_cast<double>(Core::getSingletonPtr()->getOptionDesiredSimulationUpdates());
		const double maxDeltaTime = 0.25;

		// How many fixed steps we're allowed to run per loop iteration before we drop backlog.
		// Use your existing "maxTicksPerFrames" concept if you have one; 5-8 is typical.
		const int maxStepsPerFrame = 5;

		Ogre::Window* renderWindow = Core::getSingletonPtr()->getOgreRenderWindow();
		this->setDesiredUpdates(Core::getSingletonPtr()->getOptionDesiredFramesUpdates());

		// Time in seconds
		double currentTime = static_cast<double>(Core::getSingletonPtr()->getOgreTimer()->getMilliseconds()) * 0.001;
		double accumulator = 0.0;

		NOWA::GraphicsModule* gfx = NOWA::GraphicsModule::getInstance();
		gfx->setFrameTime(static_cast<Ogre::Real>(fixedDt)); // seconds per logic tick

		while (false == this->bShutdown)
		{
			Ogre::WindowEventUtilities::messagePump();

			// Measure real dt
			const double newTime = static_cast<double>(Core::getSingletonPtr()->getOgreTimer()->getMilliseconds()) * 0.001;
			double frameTime = newTime - currentTime;
			currentTime = newTime;

			// Prevent spiral-of-death on big hitches
			frameTime = std::min(frameTime, maxDeltaTime);
			accumulator += frameTime;

			// Variable-rate render-side update (UI/editor etc.)
			if (false == this->bStall && false == this->activeStateStack.back()->gameProgressModule->bSceneLoading)
			{
				this->activeStateStack.back()->renderUpdate(static_cast<Ogre::Real>(frameTime));
			}

			bool didUpdate = false;
			int steps = 0;

			// Run at most maxStepsPerFrame fixed steps
			while (accumulator >= fixedDt && steps < maxStepsPerFrame)
			{
				if (!didUpdate)
				{
					gfx->beginLogicFrame();
					didUpdate = true;
				}

				this->processAll();

				if (false == this->bStall && false == this->activeStateStack.back()->gameProgressModule->bSceneLoading)
				{
					// Fixed-step update (DesignState -> ogreNewt->updateFixed(dt) etc.)
					this->activeStateStack.back()->update(static_cast<Ogre::Real>(fixedDt));
				}

				Core::getSingletonPtr()->updateFrameStats(static_cast<Ogre::Real>(fixedDt));
				Core::getSingletonPtr()->update(static_cast<Ogre::Real>(fixedDt));

				accumulator -= fixedDt;
				++steps;
			}

			// If we're still behind after max steps, DROP backlog to avoid slow-motion.
			// This keeps the game responsive under heavy physics load.
			//if (accumulator >= fixedDt)
			//{
			//	// keep only the remainder for alpha, drop the rest
			//	accumulator = std::fmod(accumulator, fixedDt);
			//}

			if (accumulator >= fixedDt)
			{
				// Drop backlog hard: keep at most one tick remainder for alpha stability
				accumulator = fixedDt * 0.5; // or 0.0 if you prefer snappier behavior
			}

			// Publish alpha for render interpolation every frame
			const float alpha = (fixedDt > 0.0) ? static_cast<float>(accumulator / fixedDt) : 0.0f;
			gfx->publishInterpolationAlpha(alpha);

			if (didUpdate)
			{
				gfx->endLogicFrame();
				gfx->publishLogicFrame(); // optional debug/telemetry
			}

			if (false == renderWindow->isVisible() && this->renderWhenInactive)
			{
				Ogre::Threads::Sleep(500);
			}
		}

		this->bStall = true;
	}

	void AppStateManager::internalChangeAppState(AppState* state, bool initial)
	{
		AppState* oldState = nullptr;
		// end the state if present
		if (false == this->activeStateStack.empty())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AppStateManager] Exiting " + this->activeStateStack.back()->getName());
			NOWA::GraphicsModule::getInstance()->clearAllClosures();
			this->activeStateStack.back()->exit();

			this->bCanProcessRenderQueue = false;

			oldState = this->activeStateStack.back();
			
			this->activeStateStack.pop_back();
		}

		this->activeStateStack.push_back(state);

		// Set the cached pointer on the Logic Thread
		this->activeGameProgressModule.store(state->gameProgressModule);

		// link input devices and gui with core
		this->linkInputWithCore(oldState, state);

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AppStateManager] Entering " + this->activeStateStack.back()->getName());
		// enter the new state
		if (true == initial)
		{
			this->activeStateStack.back()->startRendering();
		}
		this->bCanProcessRenderQueue = true;
		this->activeStateStack.back()->enter();

		if (nullptr == this->activeStateStack.back()->eventManager)
		{
			Ogre::String errorMessage = "[AppStateManager] Error on change to new state: '" + this->activeStateStack.back()->appStateName 
				+ "' because the event manager and other modules are null. Maybe the modules have not been initialized in the enter method. See: 'initializeModules(true, true);'";

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, errorMessage);
			throw Ogre::Exception(Ogre::Exception::ERR_INTERNAL_ERROR, errorMessage, "NOWA");
		}

		this->bStall = false;
	}

	bool AppStateManager::internalPushAppState(AppState* state)
	{
		AppState* oldState = nullptr;

		if (nullptr == state)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AppStateManager] Error: The desired app state is invalid! (nullptr)");

			Ogre::String message = "[AppStateManager] Error: The desired app state is invalid (nullptr)!\n";
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, message);
			// throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, message + "\n", "NOWA");
			return false;
		}
		// If a state is paused, no other state should be pushed
		if (false == this->activeStateStack.empty())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AppStateManager] Can not push appstate, because the "
				+ this->activeStateStack.back()->getName() + " state is paused");
			if (false == this->activeStateStack.back()->pause())
			{
				return false;
			}
		}

		if (false == this->activeStateStack.empty())
		{
			oldState = this->activeStateStack.back();
		}
		this->activeStateStack.push_back(state);

		// Set the cached pointer on the Logic Thread
		this->activeGameProgressModule.store(state->gameProgressModule);

		// Links input devices and gui with core
		this->linkInputWithCore(oldState, state);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AppStateManager] Entering " + this->activeStateStack.back()->getName());
		// Enters the new state
		this->activeStateStack.back()->enter();

		this->bStall = false;
		this->bCanProcessRenderQueue = true;

		return true;
	}

	void AppStateManager::internalPopAppState(void)
	{
		// Removes the present state
		AppState* oldState = nullptr;
		if (false == this->activeStateStack.empty())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AppStateManager] Exiting " + this->activeStateStack.back()->getName());

			oldState = this->activeStateStack.back();
			NOWA::GraphicsModule::getInstance()->clearAllClosures();
			this->activeStateStack.back()->exit();
			this->activeStateStack.pop_back();
		}

		// if there is a state left, continue
		if (false == this->activeStateStack.empty())
		{
			this->linkInputWithCore(oldState, this->activeStateStack.back());
			// if a state is paused, resume the state, in order to be able to continue
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AppStateManager] Resuming " + this->activeStateStack.back()->getName());

			// Set the cached pointer on the Logic Thread
			this->activeGameProgressModule.store(this->activeStateStack.back()->gameProgressModule);
			this->activeStateStack.back()->resume();
		}
		else
		{
			this->shutdown();
		}
		this->bStall = false;
		this->bCanProcessRenderQueue = true;
	}

	void AppStateManager::internalPopAllAndPushAppState(AppState* state)
	{
		while (false == this->activeStateStack.empty())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AppStateManager] Exiting " + this->activeStateStack.back()->getName());
			NOWA::GraphicsModule::getInstance()->clearAllClosures();
			this->activeStateStack.back()->exit();

			AppState* oldState = this->activeStateStack.back();
			InputDeviceCore::getSingletonPtr()->removeKeyListener(oldState);
			InputDeviceCore::getSingletonPtr()->removeMouseListener(oldState);
			InputDeviceCore::getSingletonPtr()->removeJoystickListener(oldState);

			this->activeStateStack.pop_back();
		}

		bool stateAlreadyExists = false;
		for (auto it = this->activeStateStack.begin(); it != this->activeStateStack.end(); )
		{
			if (state != *it)
			{
				it = this->activeStateStack.erase(it);
			}
			else
			{
				stateAlreadyExists = true;
				++it;
			}
		}

		if (nullptr != state)
		{
			if (false == stateAlreadyExists)
			{
				this->internalPushAppState(state);
			}
			else
			{
				this->linkInputWithCore(nullptr, state);
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AppStateManager] Entering " + state->getName());
				// Enter the new state
				state->enter();

				this->bStall = false;
				this->bCanProcessRenderQueue = true;
			}
		}
		else
		{
			this->shutdown();
		}
		this->bStall = false;
		this->bCanProcessRenderQueue = true;
	}

	void AppStateManager::internalExitGame(void)
	{
		if (false == Core::getSingletonPtr()->getIsGame())
		{
			boost::shared_ptr<EventDataStopSimulation> eventDataStopSimulation(new EventDataStopSimulation(""));
			this->activeStateStack.back()->eventManager->queueEvent(eventDataStopSimulation);
		}
		else
		{
			this->shutdown();
		}
	}

	void AppStateManager::signalLogicFrameFinished(void)
	{
		{
			std::lock_guard<std::mutex> lock(this->logicFrameMutex);
			this->logicFrameFinished = true;
		}
		this->logicFrameCondVar.notify_all(); // Notify the rendering thread
	}

	bool AppStateManager::getRenderWhenInactive(void) const
	{
		return this->renderWhenInactive;
	}

#if 1
	void AppStateManager::waitForLogicFrameFinish()
	{
		std::unique_lock<std::mutex> lock(logicFrameMutex);
		bool wasSignaled = logicFrameCondVar.wait_for(lock, std::chrono::milliseconds(100), [this]
		{
			return this->logicFrameFinished;
		});

		if (wasSignaled)
		{
			this->logicFrameFinished = false; // Reset for next frame
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "WARNING: Logic frame sync timeout.Proceeding without signal.");
		}
	}
#else
	void AppStateManager::waitForLogicFrameFinish()
	{
		std::unique_lock<std::mutex> lock(logicFrameMutex);
		logicFrameCondVar.wait(lock, [this] { return logicFrameFinished; });
		logicFrameFinished = false; // Reset the flag for the next frame
	}
#endif


	void AppStateManager::changeAppState(AppState* state)
	{
		if (nullptr == state)
		{
			if (true == Core::getSingletonPtr()->getIsGame())
			{
				Ogre::String errorMessage = "[AppStateManager] Error: Cannot change appstate, because the new app state is null!";
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, errorMessage);
				throw Ogre::Exception(Ogre::Exception::ERR_INTERNAL_ERROR, errorMessage, "NOWA");
			}
			return;
		}

		if (true == Core::getSingletonPtr()->getIsGame())
		{
			this->bStall = true;
			// NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.2f));
			// Creates the delay process and changes the scene at another tick. Note, this is necessary
			// because changing the scene destroys all game objects and its components.
			// So changing the state directly inside a component would create a mess, since everything will be destroyed
			// and the game object map in update loop becomes invalid while its iterating
			// delayProcess->attachChild(NOWA::ProcessPtr(new ChangeAppStateProcess(state, eAppStateOperation::ChangeAppState)));
			// NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
			NOWA::ProcessManager::getInstance()->attachProcess(NOWA::ProcessPtr(new ChangeAppStateProcess(state, eAppStateOperation::ChangeAppState)));
		}
	}

	bool AppStateManager::pushAppState(AppState* state)
	{
		if (nullptr == state)
		{
			if (true == Core::getSingletonPtr()->getIsGame())
			{
				Ogre::String errorMessage = "[AppStateManager] Error: Cannot push appstate, because the new app state is null!";
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, errorMessage);
				throw Ogre::Exception(Ogre::Exception::ERR_INTERNAL_ERROR, errorMessage, "NOWA");
			}
			return false;
		}

		if (true == Core::getSingletonPtr()->getIsGame())
		{
			this->bStall = true;
			// NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.2f));
			// Creates the delay process and changes the scene at another tick. Note, this is necessary
			// because changing the scene destroys all game objects and its components.
			// So changing the state directly inside a component would create a mess, since everything will be destroyed
			// and the game object map in update loop becomes invalid while its iterating
			// delayProcess->attachChild(NOWA::ProcessPtr(new ChangeAppStateProcess(state, eAppStateOperation::PushAppState)));
			// NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
			NOWA::ProcessManager::getInstance()->attachProcess(NOWA::ProcessPtr(new ChangeAppStateProcess(state, eAppStateOperation::PushAppState)));
		}

		return true;
	}

	void AppStateManager::popAppState(void)
	{
		if (true == Core::getSingletonPtr()->getIsGame())
		{
			this->bStall = true;
			// NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.2f));
			// Creates the delay process and changes the scene at another tick. Note, this is necessary
			// because changing the scene destroys all game objects and its components.
			// So changing the state directly inside a component would create a mess, since everything will be destroyed
			// and the game object map in update loop becomes invalid while its iterating
			// delayProcess->attachChild(NOWA::ProcessPtr(new ChangeAppStateProcess(eAppStateOperation::PopAppState)));
			// NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
			NOWA::ProcessManager::getInstance()->attachProcess(NOWA::ProcessPtr(new ChangeAppStateProcess(eAppStateOperation::PopAppState)));
		}
	}

	void AppStateManager::popAllAndPushAppState(AppState* state)
	{
		if (nullptr == state)
		{
			if (true == Core::getSingletonPtr()->getIsGame())
			{
				Ogre::String errorMessage = "[AppStateManager] Error: Cannot pop all and push appstate, because the new app state is null!";
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, errorMessage);
				throw Ogre::Exception(Ogre::Exception::ERR_INTERNAL_ERROR, errorMessage, "NOWA");
			}
			return;
		}

		if (true == Core::getSingletonPtr()->getIsGame())
		{
			this->bStall = true;
			// NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.2f));
			// Creates the delay process and changes the scene at another tick. Note, this is necessary
			// because changing the scene destroys all game objects and its components.
			// So changing the state directly inside a component would create a mess, since everything will be destroyed
			// and the game object map in update loop becomes invalid while its iterating
			// delayProcess->attachChild(NOWA::ProcessPtr(new ChangeAppStateProcess(state, eAppStateOperation::PopAllAndPushAppState)));
			// NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
			NOWA::ProcessManager::getInstance()->attachProcess(NOWA::ProcessPtr(new ChangeAppStateProcess(state, eAppStateOperation::PopAllAndPushAppState)));
		}
	}

	void AppStateManager::exitGame(void)
	{
		if (true == Core::getSingletonPtr()->getIsGame())
		{
			this->bStall = true;
			// NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.2f));
			// Creates the delay process and changes the scene at another tick. Note, this is necessary
			// because changing the scene destroys all game objects and its components.
			// So changing the state directly inside a component would create a mess, since everything will be destroyed
			// and the game object map in update loop becomes invalid while its iterating
			// delayProcess->attachChild(NOWA::ProcessPtr(new ChangeAppStateProcess(eAppStateOperation::ExitGame)));
			// NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
			NOWA::ProcessManager::getInstance()->attachProcess(NOWA::ProcessPtr(new ChangeAppStateProcess(eAppStateOperation::ExitGame)));
		}
	}

	void AppStateManager::linkInputWithCore(AppState* oldState, AppState* state)
	{
		if (nullptr != oldState)
		{
			InputDeviceCore::getSingletonPtr()->removeKeyListener(oldState);
			InputDeviceCore::getSingletonPtr()->removeMouseListener(oldState);
			InputDeviceCore::getSingletonPtr()->removeJoystickListener(oldState);
		}

		// If a listener has been added via key/mouse/joystick pressed, a new listener would be inserted during this iteration, which would cause a crash in mouse/key/button release iterator, hence add in next frame
		NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.25f));
		auto ptrFunction = [this, state]()
		{
			// Remove first for precaution, in order to prevent duplicate state
			InputDeviceCore::getSingletonPtr()->removeKeyListener(state);
			InputDeviceCore::getSingletonPtr()->removeMouseListener(state);
			InputDeviceCore::getSingletonPtr()->removeJoystickListener(state);
			InputDeviceCore::getSingletonPtr()->addKeyListener(state, state->getName());
			InputDeviceCore::getSingletonPtr()->addMouseListener(state, state->getName());
			InputDeviceCore::getSingletonPtr()->addJoystickListener(state, state->getName());
		};
		NOWA::ProcessPtr closureProcess(new NOWA::ClosureProcess(ptrFunction));
		delayProcess->attachChild(closureProcess);
		NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
	}

	void AppStateManager::changeAppState(const Ogre::String& stateName)
	{
		this->changeAppState(this->findByName(stateName));
	}

	bool AppStateManager::pushAppState(const Ogre::String& stateName)
	{
		return this->pushAppState(this->findByName(stateName));
	}

	void AppStateManager::popAllAndPushAppState(const Ogre::String& stateName)
	{
		this->popAllAndPushAppState(this->findByName(stateName));
	}

	bool AppStateManager::hasAppStateStarted(const Ogre::String& stateName)
	{
		AppState* appState = this->findByName(stateName);
		if (nullptr != appState)
		{
			return appState->getHasStarted();
		}
		return false;
	}

	bool AppStateManager::isInAppstate(const Ogre::String& stateName)
	{
		if (true == this->activeStateStack.empty())
		{
			return false;
		}
		return this->activeStateStack.back()->appStateName == stateName;
	}

	Ogre::String AppStateManager::getCurrentAppStateName(void) const
	{
		return this->activeStateStack.back()->getName();
	}

	AppState* AppStateManager::getCurrentAppState(void) const
	{
		if (false == this->activeStateStack.empty())
			return this->activeStateStack.back();
		return nullptr;
	}

	void AppStateManager::setSlowMotion(unsigned int slowMotionMS)
	{
		this->slowMotionMS = slowMotionMS;
	}

	void AppStateManager::shutdown(void)
	{
		this->bShutdown = true;
		if (false == this->activeStateStack.empty())
		{
			this->activeStateStack.back()->stopRendering();
		}
	}

	size_t AppStateManager::getAppStatesCount(void) const
	{
		return this->activeStateStack.size();
	}

	GameObjectController* AppStateManager::getGameObjectController(void) const
	{
		if (true == this->activeStateStack.empty())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AppStateManager] Error getting game object controller, because at this time no application state (AppState) has been created! Maybe this call was to early.");
			throw Ogre::Exception(Ogre::Exception::ERR_INTERNAL_ERROR, "[AppStateManager] Error getting game object controller, because at this time no application state (AppState) has been created! Maybe this call was to early.", "NOWA");
		}
		return this->activeStateStack.back()->gameObjectController;
	}

	GameProgressModule* AppStateManager::getGameProgressModule(void) const
	{
		if (true == this->activeStateStack.empty())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AppStateManager] Error getting game progress module, because at this time no application state (AppState) has been created! Maybe this call was to early.");
			throw Ogre::Exception(Ogre::Exception::ERR_INTERNAL_ERROR, "[AppStateManager] Error getting game progress module, because at this time no application state (AppState) has been created! Maybe this call was to early.", "NOWA");
		}
		return this->activeStateStack.back()->gameProgressModule;
	}

	RakNetModule* AppStateManager::getRakNetModule(void) const
	{
		if (true == this->activeStateStack.empty())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AppStateManager] Error getting RakNet module, because at this time no application state (AppState) has been created! Maybe this call was to early.");
			throw Ogre::Exception(Ogre::Exception::ERR_INTERNAL_ERROR, "[AppStateManager] Error getting RakNet module, because at this time no application state (AppState) has been created! Maybe this call was to early.", "NOWA");
		}
		return this->activeStateStack.back()->rakNetModule;
	}

	MiniMapModule* AppStateManager::getMiniMapModule(void) const
	{
		if (true == this->activeStateStack.empty())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AppStateManager] Error getting minmap module, because at this time no application state (AppState) has been created! Maybe this call was to early.");
			throw Ogre::Exception(Ogre::Exception::ERR_INTERNAL_ERROR, "[AppStateManager] Error getting minimap module, because at this time no application state (AppState) has been created! Maybe this call was to early.", "NOWA");
		}
		return this->activeStateStack.back()->miniMapModule;
	}

	OgreNewtModule* AppStateManager::getOgreNewtModule(void) const
	{
		if (true == this->activeStateStack.empty())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AppStateManager] Error getting OgreNewt module, because at this time no application state (AppState) has been created! Maybe this call was to early.");
			throw Ogre::Exception(Ogre::Exception::ERR_INTERNAL_ERROR, "[AppStateManager] Error getting OgreNewt module, because at this time no application state (AppState) has been created! Maybe this call was to early.", "NOWA");
		}
		return this->activeStateStack.back()->ogreNewtModule;
	}

	DecalsModule* AppStateManager::getDecalsModule(void) const
	{
		if (true == this->activeStateStack.empty())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AppStateManager] Error getting mesh decals module, because at this time no application state (AppState) has been created! Maybe this call was to early.");
			throw Ogre::Exception(Ogre::Exception::ERR_INTERNAL_ERROR, "[AppStateManager] Error getting mesh decals module, because at this time no application state (AppState) has been created! Maybe this call was to early.", "NOWA");
		}
		return this->activeStateStack.back()->decalsModule;
	}

	CameraManager* AppStateManager::getCameraManager(void) const
	{
		if (true == this->activeStateStack.empty())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AppStateManager] Error getting camera manager, because at this time no application state (AppState) has been created! Maybe this call was to early.");
			throw Ogre::Exception(Ogre::Exception::ERR_INTERNAL_ERROR, "[AppStateManager] Error getting camera manager, because at this time no application state (AppState) has been created! Maybe this call was to early.", "NOWA");
		}
		return this->activeStateStack.back()->cameraManager;
	}

	OgreRecastModule* AppStateManager::getOgreRecastModule(void) const
	{
		if (true == this->activeStateStack.empty())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AppStateManager] Error getting OgreRecast module, because at this time no application state (AppState) has been created! Maybe this call was to early.");
			throw Ogre::Exception(Ogre::Exception::ERR_INTERNAL_ERROR, "[AppStateManager] Error getting OgreRecast module, because at this time no application state (AppState) has been created! Maybe this call was to early.", "NOWA");
		}
		return this->activeStateStack.back()->ogreRecastModule;
	}

	ParticleFxModule* AppStateManager::getParticleFxModule(void) const
	{
		if (true == this->activeStateStack.empty())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AppStateManager] Error getting particle fx module, because at this time no application state (AppState) has been created! Maybe this call was to early.");
			throw Ogre::Exception(Ogre::Exception::ERR_INTERNAL_ERROR, "[AppStateManager] Error getting particle fx module, because at this time no application state (AppState) has been created! Maybe this call was to early.", "NOWA");
		}
		return this->activeStateStack.back()->particleFxModule;
	}

	LuaScriptModule* AppStateManager::getLuaScriptModule(void) const
	{
		if (true == this->activeStateStack.empty())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AppStateManager] Error getting lua script module, because at this time no application state (AppState) has been created! Maybe this call was to early.");
			throw Ogre::Exception(Ogre::Exception::ERR_INTERNAL_ERROR, "[AppStateManager] Error getting lua script module, because at this time no application state (AppState) has been created! Maybe this call was to early.", "NOWA");
		}
		return this->activeStateStack.back()->luaScriptModule;
	}

	EventManager* AppStateManager::getEventManager(void) const
	{
		if (true == this->activeStateStack.empty())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AppStateManager] Error getting event manager, because at this time no application state (AppState) has been created! Maybe this call was to early.");
			throw Ogre::Exception(Ogre::Exception::ERR_INTERNAL_ERROR, "[AppStateManager] Error getting event manager, because at this time no application state (AppState) has been created! Maybe this call was to early.", "NOWA");
		}
		return this->activeStateStack.back()->eventManager;
	}

	ScriptEventManager* AppStateManager::getScriptEventManager(void) const
	{
		if (true == this->activeStateStack.empty())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AppStateManager] Error getting script event manager, because at this time no application state (AppState) has been created! Maybe this call was to early.");
			throw Ogre::Exception(Ogre::Exception::ERR_INTERNAL_ERROR, "[AppStateManager] Error getting script event manager, because at this time no application state (AppState) has been created! Maybe this call was to early.", "NOWA");
		}
		return this->activeStateStack.back()->scriptEventManager;
	}

	GameObjectController* AppStateManager::getGameObjectController(const Ogre::String& stateName)
	{
		AppState* appState = this->findByName(stateName);
		if (nullptr != appState)
			return appState->gameObjectController;

		return nullptr;
	}

	GameProgressModule* AppStateManager::getGameProgressModule(const Ogre::String& stateName)
	{
		AppState* appState = this->findByName(stateName);
		if (nullptr != appState)
			return appState->gameProgressModule;

		return nullptr;
	}

	RakNetModule* AppStateManager::getRakNetModule(const Ogre::String& stateName)
	{
		AppState* appState = this->findByName(stateName);
		if (nullptr != appState)
			return appState->rakNetModule;

		return nullptr;
	}

	MiniMapModule* AppStateManager::getMiniMapModule(const Ogre::String& stateName)
	{
		AppState* appState = this->findByName(stateName);
		if (nullptr != appState)
			return appState->miniMapModule;

		return nullptr;
	}

	OgreNewtModule* AppStateManager::getOgreNewtModule(const Ogre::String& stateName)
	{
		AppState* appState = this->findByName(stateName);
		if (nullptr != appState)
			return appState->ogreNewtModule;

		return nullptr;
	}

	DecalsModule* AppStateManager::getDecalsModule(const Ogre::String& stateName)
	{
		AppState* appState = this->findByName(stateName);
		if (nullptr != appState)
			return appState->decalsModule;

		return nullptr;
	}

	CameraManager* AppStateManager::getCameraManager(const Ogre::String& stateName)
	{
		AppState* appState = this->findByName(stateName);
		if (nullptr != appState)
			return appState->cameraManager;

		return nullptr;
	}

	OgreRecastModule* AppStateManager::getOgreRecastModule(const Ogre::String& stateName)
	{
		AppState* appState = this->findByName(stateName);
		if (nullptr != appState)
			return appState->ogreRecastModule;

		return nullptr;
	}

	ParticleFxModule* AppStateManager::getParticleFxModule(const Ogre::String& stateName)
	{
		AppState* appState = this->findByName(stateName);
		if (nullptr != appState)
			return appState->particleFxModule;
		return nullptr;
	}

	LuaScriptModule* AppStateManager::getLuaScriptModule(const Ogre::String& stateName)
	{
		AppState* appState = this->findByName(stateName);
		if (nullptr != appState)
			return appState->luaScriptModule;

		return nullptr;
	}

	EventManager* AppStateManager::getEventManager(const Ogre::String& stateName)
	{
		AppState* appState = this->findByName(stateName);
		if (nullptr != appState)
			return appState->eventManager;

		return nullptr;
	}

	ScriptEventManager* AppStateManager::getScriptEventManager(const Ogre::String& stateName)
	{
		AppState* appState = this->findByName(stateName);
		if (nullptr != appState)
			return appState->scriptEventManager;

		return nullptr;
	}

	Variant* AppStateManager::getGlobalValue(const Ogre::String& attributeName)
	{
		Variant* globalAttribute = nullptr;
		auto& it = this->globalAttributesMap.find(attributeName);
		if (it != this->globalAttributesMap.cend())
		{
			globalAttribute = it->second;
		}
		return globalAttribute;
	}

	Variant* AppStateManager::setGlobalBoolValue(const Ogre::String& attributeName, bool value)
	{
		Variant* globalAttribute = nullptr;
		auto& it = this->globalAttributesMap.find(attributeName);
		if (it != this->globalAttributesMap.cend())
		{
			globalAttribute = it->second;
			globalAttribute->setValue(value);
		}
		else
		{
			globalAttribute = new Variant(attributeName);
			globalAttribute->setValue(value);
			this->globalAttributesMap.emplace(attributeName, globalAttribute);
		}
		return globalAttribute;
	}

	Variant* AppStateManager::setGlobalIntValue(const Ogre::String& attributeName, int value)
	{
		Variant* globalAttribute = nullptr;
		auto& it = this->globalAttributesMap.find(attributeName);
		if (it != this->globalAttributesMap.cend())
		{
			globalAttribute = it->second;
			globalAttribute->setValue(value);
		}
		else
		{
			globalAttribute = new Variant(attributeName);
			globalAttribute->setValue(value);
			this->globalAttributesMap.emplace(attributeName, globalAttribute);
		}
		return globalAttribute;
	}

	Variant* AppStateManager::setGlobalUIntValue(const Ogre::String& attributeName, unsigned int value)
	{
		Variant* globalAttribute = nullptr;
		auto& it = this->globalAttributesMap.find(attributeName);
		if (it != this->globalAttributesMap.cend())
		{
			globalAttribute = it->second;
			globalAttribute->setValue(value);
		}
		else
		{
			globalAttribute = new Variant(attributeName);
			globalAttribute->setValue(value);
			this->globalAttributesMap.emplace(attributeName, globalAttribute);
		}
		return globalAttribute;
	}

	Variant* AppStateManager::setGlobalULongValue(const Ogre::String& attributeName, unsigned long value)
	{
		Variant* globalAttribute = nullptr;
		auto& it = this->globalAttributesMap.find(attributeName);
		if (it != this->globalAttributesMap.cend())
		{
			globalAttribute = it->second;
			globalAttribute->setValue(value);
		}
		else
		{
			globalAttribute = new Variant(attributeName);
			globalAttribute->setValue(value);
			this->globalAttributesMap.emplace(attributeName, globalAttribute);
		}
		return globalAttribute;
	}

	Variant* AppStateManager::setGlobalRealValue(const Ogre::String& attributeName, Ogre::Real value)
	{
		Variant* globalAttribute = nullptr;
		auto& it = this->globalAttributesMap.find(attributeName);
		if (it != this->globalAttributesMap.cend())
		{
			globalAttribute = it->second;
			globalAttribute->setValue(value);
		}
		else
		{
			globalAttribute = new Variant(attributeName);
			globalAttribute->setValue(value);
			this->globalAttributesMap.emplace(attributeName, globalAttribute);
		}
		return globalAttribute;
	}

	Variant* AppStateManager::setGlobalStringValue(const Ogre::String& attributeName, Ogre::String value)
	{
		Variant* globalAttribute = nullptr;
		auto& it = this->globalAttributesMap.find(attributeName);
		if (it != this->globalAttributesMap.cend())
		{
			globalAttribute = it->second;
			globalAttribute->setValue(value);
		}
		else
		{
			globalAttribute = new Variant(attributeName);
			globalAttribute->setValue(value);
			this->globalAttributesMap.emplace(attributeName, globalAttribute);
		}
		return globalAttribute;
	}

	Variant* AppStateManager::setGlobalVector2Value(const Ogre::String& attributeName, Ogre::Vector2 value)
	{
		Variant* globalAttribute = nullptr;
		auto& it = this->globalAttributesMap.find(attributeName);
		if (it != this->globalAttributesMap.cend())
		{
			globalAttribute = it->second;
			globalAttribute->setValue(value);
		}
		else
		{
			globalAttribute = new Variant(attributeName);
			globalAttribute->setValue(value);
			this->globalAttributesMap.emplace(attributeName, globalAttribute);
		}
		return globalAttribute;
	}

	Variant* AppStateManager::setGlobalVector3Value(const Ogre::String& attributeName, Ogre::Vector3 value)
	{
		Variant* globalAttribute = nullptr;
		auto& it = this->globalAttributesMap.find(attributeName);
		if (it != this->globalAttributesMap.cend())
		{
			globalAttribute = it->second;
			globalAttribute->setValue(value);
		}
		else
		{
			globalAttribute = new Variant(attributeName);
			globalAttribute->setValue(value);
			this->globalAttributesMap.emplace(attributeName, globalAttribute);
		}
		return globalAttribute;
	}

	Variant* AppStateManager::setGlobalVector4Value(const Ogre::String& attributeName, Ogre::Vector4 value)
	{
		Variant* globalAttribute = nullptr;
		auto& it = this->globalAttributesMap.find(attributeName);
		if (it != this->globalAttributesMap.cend())
		{
			globalAttribute = it->second;
			globalAttribute->setValue(value);
		}
		else
		{
			globalAttribute = new Variant(attributeName);
			globalAttribute->setValue(value);
			this->globalAttributesMap.emplace(attributeName, globalAttribute);
		}
		return globalAttribute;
	}

	bool AppStateManager::internalReadGlobalAttributes(const Ogre::String& globalAttributesStream)
	{
		bool success = true;
		std::istringstream inStream(globalAttributesStream);

		Ogre::String line;

		while (std::getline(inStream, line)) // This is used, because white spaces are also read
		{
			if (true == line.empty())
				continue;

			// Read till global attributes section
			size_t foundGlobalAttributeSection = line.find("[GlobalAttributes]");
			if (foundGlobalAttributeSection != Ogre::String::npos)
			{
				break;
			}

			// GameObject
			Ogre::String gameObjectId = line.substr(1, line.size() - 2);
			unsigned long id;
			std::istringstream(gameObjectId) >> id;

			// Get the game object controller for this app state name
			auto& gameObjectPtr = this->getGameObjectController(this->activeStateStack.back()->getName())->getGameObjectFromId(id);
			if (nullptr != gameObjectPtr)
			{
				boost::shared_ptr<AttributesComponent> attributesCompPtr = NOWA::makeStrongPtr(gameObjectPtr->getComponent<AttributesComponent>());
				if (nullptr != attributesCompPtr)
				{
					// Read data and set for attributes component
					success = attributesCompPtr->internalRead(inStream);
				}
			}
		}

		// Read possible global attributes
		Ogre::StringVector data;

		// parse til eof
		while (std::getline(inStream, line)) // This is used, because white spaces are also read
		{
			// Parse til next game object
			if (line.find("[") != Ogre::String::npos)
				break;

			data = Ogre::StringUtil::split(line, "=");
			if (data.size() < 3)
				continue;

			Variant* globalAttribute = nullptr;
			auto& it = this->globalAttributesMap.find(data[0]);
			if (it != this->globalAttributesMap.cend())
			{
				globalAttribute = it->second;
			}
			else
			{
				globalAttribute = new Variant(data[0]);
				globalAttribute->setValue(data[1]);
				this->globalAttributesMap.emplace(data[0], globalAttribute);
			}

			if (nullptr != globalAttribute)
			{
				if ("Bool" == data[1])
				{
					globalAttribute->setValue(Ogre::StringConverter::parseBool(data[2]));
					success = true;
				}
				else if ("Int" == data[1])
				{
					globalAttribute->setValue(Ogre::StringConverter::parseInt(data[2]));
					success = true;
				}
				else if ("UInt" == data[1])
				{
					globalAttribute->setValue(Ogre::StringConverter::parseUnsignedInt(data[2]));
					success = true;
				}
				else if ("ULong" == data[1])
				{
					globalAttribute->setValue(Ogre::StringConverter::parseUnsignedLong(data[2]));
					success = true;
				}
				else if ("Real" == data[1])
				{
					globalAttribute->setValue(Ogre::StringConverter::parseReal(data[2]));
					success = true;
				}
				else if ("String" == data[1])
				{
					globalAttribute->setValue(data[2]);
					success = true;
				}
				else if ("Vector2" == data[1])
				{
					globalAttribute->setValue(Ogre::StringConverter::parseVector2(data[2]));
					success = true;
				}
				else if ("Vector3" == data[1])
				{
					globalAttribute->setValue(Ogre::StringConverter::parseVector3(data[2]));
					success = true;
				}
				else if ("Vector4" == data[1])
				{
					globalAttribute->setValue(Ogre::StringConverter::parseVector4(data[2]));
					success = true;
				}
			}
		}
		return success;
	}
	void AppStateManager::saveProgress(const Ogre::String& saveFilePathName, bool crypted, bool sceneSnapshot)
	{
		if (false == saveFilePathName.empty())
		{
			Ogre::String strStream;
			std::ofstream outFile;
			outFile.open(saveFilePathName.c_str());
			if (!outFile)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameProgressModule] ERROR: Could not create file for path: "
					+ saveFilePathName + "'");
				return;
			}

			// Store whether the file should be crypted or not
			outFile << crypted << "\n";

			// Get the game object controller for this app state name
			auto gameObjects = this->getGameObjectController(this->activeStateStack.back()->getName())->getGameObjects();

			std::ostringstream oStream;

			for (auto& it = gameObjects->cbegin(); it != gameObjects->cend(); ++it)
			{
				const auto& gameObjectPtr = it->second;

				// https://thinkcpp.wordpress.com/2012/04/16/file-to-map-inputoutput/

				// First save the game object id, if it does have an attributes component
				boost::shared_ptr<AttributesComponent> attributesCompPtr = NOWA::makeStrongPtr(gameObjectPtr->getComponent<AttributesComponent>());
				if (nullptr != attributesCompPtr)
				{
					oStream << "[" << Ogre::StringConverter::toString(gameObjectPtr->getId()) << "]\n";
					attributesCompPtr->internalSave(oStream);
				}
			}

			// Store global defined attributes
			oStream << "[GlobalAttributes]" << "\n";
			for (auto& it = this->globalAttributesMap.cbegin(); it != this->globalAttributesMap.cend(); ++it)
			{
				Variant* globalAttribute = it->second;
				int type = globalAttribute->getType();
				Ogre::String name = globalAttribute->getName();
				if (Variant::VAR_BOOL == type)
				{
					oStream << globalAttribute->getName() << "=Bool=" << globalAttribute->getBool() << "\n";
				}
				else if (Variant::VAR_INT == type)
				{
					oStream << globalAttribute->getName() << "=Int=" << globalAttribute->getInt() << "\n";
				}
				else if (Variant::VAR_UINT == type)
				{
					oStream << globalAttribute->getName() << "=UInt=" << globalAttribute->getUInt() << "\n";
				}
				else if (Variant::VAR_ULONG == type)
				{
					oStream << globalAttribute->getName() << "=ULong=" << globalAttribute->getULong() << "\n";
				}
				else if (Variant::VAR_REAL == type)
				{
					oStream << globalAttribute->getName() << "=Real=" << globalAttribute->getReal() << "\n";
				}
				else if (Variant::VAR_STRING == type)
				{
					oStream << globalAttribute->getName() << "=String=" << globalAttribute->getString() << "\n";
				}
				else if (Variant::VAR_VEC2 == type)
				{
					oStream << globalAttribute->getName() << "=Vector2=" << globalAttribute->getVector2() << "\n";
				}
				else if (Variant::VAR_VEC3 == type)
				{
					oStream << globalAttribute->getName() << "=Vector3=" << globalAttribute->getVector3() << "\n";
				}
				else if (Variant::VAR_VEC4 == type)
				{
					oStream << globalAttribute->getName() << "=Vector4=" << globalAttribute->getVector4() << "\n";
				}
				/*else if (VAR_LIST == type)
				{
					oStream << this->attributeNames[i]->getString() << "=StringList="<< this->attributeValues[i]->getVector4() << "\n";
				}*/
			}

			// If crypted then encode
			if (true == crypted)
			{
				strStream = Core::getSingletonPtr()->encode64(oStream.str(), true);
			}
			else
			{
				strStream = oStream.str();
			}

			outFile << strStream;
			outFile.close();
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameProgressModule] ERROR: Could not get file path name for saving data: "
				+ saveFilePathName + "'");
		}
	}
}; // namespace end
