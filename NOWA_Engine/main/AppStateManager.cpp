#include "NOWAPrecompiled.h"
#include "AppStateManager.h"
#include "Core.h"
#include "InputDeviceCore.h"
#include "Events.h"
#include "modules/GameProgressModule.h"
#include "modules/InputDeviceModule.h"
#include "ProcessManager.h"

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
		gameLoopMode(ADAPTIVE),
		lastTime(0),
		desiredUpdates(60),
		slowMotionMS(0),
		renderDelta(0),
		vsyncOn(true),
		bShutdown(false),
		bStall(false)
	{
	}

	AppStateManager::~AppStateManager()
	{
		
	}

	AppStateManager* AppStateManager::getSingletonPtr(void)
	{
		return msSingleton;
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

	void AppStateManager::start(const Ogre::String& stateName, bool renderWhenInactive, GameLoopMode gameLoopMode)
	{
		this->renderWhenInactive = renderWhenInactive;
		this->gameLoopMode = gameLoopMode;

		AppState* appState = this->findByName(stateName);
		if (nullptr != appState)
		{
			this->internalChangeAppState(appState);

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

			if (GameLoopMode::FPS_INDEPENDENT == this->gameLoopMode)
			{
				this->fpsIndependentRendering();
			}
			else if (GameLoopMode::ADAPTIVE == this->gameLoopMode)
			{
				this->adaptiveFPSRendering();
			}
			else
			{
				this->restrictedInterpolatedFPSRendering();
			}

			// end all present states
			while (false == this->activeStateStack.empty())
			{
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

	void AppStateManager::restrictedInterpolatedFPSRendering(void)
	{
		// Core::getSingletonPtr()->getOgreRoot()->getRenderSystem()->_initRenderTargets();

		this->setDesiredUpdates(Core::getSingletonPtr()->getOptionDesiredFramesUpdates());

		Ogre::Window* renderWindow = Core::getSingletonPtr()->getOgreRenderWindow();

		Ogre::Timer timer;

		Ogre::uint64 startTime = timer.getMicroseconds();

		double frameTime = 1.0 / this->desiredUpdates;
		double accumulator = frameTime;
		double timeSinceLast = 1.0 / static_cast<double>(Core::getSingletonPtr()->getScreenRefreshRate());

		double currentTime = static_cast<Ogre::Real>(Core::getSingletonPtr()->getOgreTimer()->getMilliseconds()) * 0.001;

		while (false == this->bShutdown)
		{
			// Process signals from system
			Ogre::WindowEventUtilities::messagePump();
			
			while (accumulator >= frameTime)
			{
				// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "cFrametime: " + Ogre::StringConverter::toString(cFrametime));

				// update input devices
				if (false == this->bStall && false == this->activeStateStack.back()->gameProgressModule->stallUpdates())
				{
					InputDeviceCore::getSingletonPtr()->capture(frameTime);

					// Updates the active state
					this->activeStateStack.back()->update(static_cast<Ogre::Real>(frameTime));
				}

				// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "Logic dt: " + Ogre::StringConverter::toString(dt));

				// Update core
				Core::getSingletonPtr()->updateFrameStats(static_cast<Ogre::Real>(frameTime));
				Core::getSingletonPtr()->update(static_cast<Ogre::Real>(frameTime));

				accumulator -= frameTime;

				if (false == this->bStall && false == this->activeStateStack.back()->gameProgressModule->stallUpdates())
				{
					this->activeStateStack.back()->lateUpdate(static_cast<Ogre::Real>(frameTime));
				}

				// Does not work?
				// if (this->slowMotionMS > 0)
				// 	Ogre::Threads::Sleep(this->slowMotionMS);
			}

			this->bShutdown |= !Ogre::Root::getSingletonPtr()->renderOneFrame(/*static_cast<Ogre::Real>(timeSinceLast)*/);

			/*if (gFakeFrameskip)
				Ogre::Threads::Sleep(40);*/

			Ogre::uint64 endTime = timer.getMicroseconds();
			timeSinceLast = (endTime - startTime) / 1000000.0;
			timeSinceLast = std::min(1.0, timeSinceLast); //Prevent from going haywire.
			accumulator += timeSinceLast;
			startTime = endTime;

			// Check if window is active or in task bar
			HWND temp = GetActiveWindow();
			HWND hWnd;
			Core::getSingletonPtr()->getOgreRenderWindow()->getCustomAttribute("WINDOW", &hWnd);
			
			// Update the renderwindow if the window is not active too, for server/client analysis
			if (temp != hWnd && false == this->renderWhenInactive)
			{
				// Core::getSingletonPtr()->getOgreRenderWindow()->update();
				// Do not burn CPU cycles unnecessary when minimized etc.
				Ogre::Threads::Sleep(500);
			}
		}
	}

	void AppStateManager::adaptiveFPSRendering(void)
	{
		// really important:
		// http://docs.unity3d.com/Manual/ExecutionOrder.html
		// update: Input, Gameslogic -> as often as possible
		// FixedUpdate: Physics because of deterministics, always at constant rate

		Ogre::Window* renderWindow = Core::getSingletonPtr()->getOgreRenderWindow();
		
		// this keeps track of time (seconds to keep it easy)
		double currentTime = static_cast<Ogre::Real>(Core::getSingletonPtr()->getOgreTimer()->getMilliseconds()) * 0.001;

		while (false == this->bShutdown)
		{
			// Process signals from system
			Ogre::WindowEventUtilities::messagePump();
			// calculate time between 2 frames
			// this keeps track of time (seconds to keep it easy)
			double dt = (static_cast<double>(Core::getSingletonPtr()->getOgreTimer()->getMilliseconds()) * 0.001) - currentTime;
			currentTime += dt;

			// update input devices
			if (false == this->bStall && false == this->activeStateStack.back()->gameProgressModule->stallUpdates())
			{
				InputDeviceCore::getSingletonPtr()->capture(static_cast<Ogre::Real>(dt));

				// update the active state
				this->activeStateStack.back()->update(static_cast<float>(dt));
			}

			// Update core
			Core::getSingletonPtr()->updateFrameStats(static_cast<Ogre::Real>(dt));
			Core::getSingletonPtr()->update(static_cast<Ogre::Real>(dt));

			/******rendering comes after update, so its late update*****/
			if (false == this->bStall && false == this->activeStateStack.back()->gameProgressModule->stallUpdates())
			{
				this->activeStateStack.back()->lateUpdate(static_cast<Ogre::Real>(dt));
			}

			// Ogre::Root::getSingletonPtr()->_fireFrameRenderingQueued();
			this->bShutdown |= !Ogre::Root::getSingletonPtr()->renderOneFrame(static_cast<float>(dt));

			// is better because everything necessary is done, and hydrax works properly then

			// Update the renderwindow if the window is not active too, for server/client analysis
			if (false == renderWindow->isVisible() && this->renderWhenInactive)
			{
				// Core::getSingletonPtr()->getOgreRenderWindow()->update();
				// Do not burn CPU cycles unnecessary when minimized etc.
				Ogre::Threads::Sleep(500);
				// Sleep(500);
			}
			// http://stackoverflow.com/questions/3727420/significance-of-sleep0))
			// This will have the frame rate e.g. from 800 to 400 and give other thread a change to react
			// Sleep(0);
		}
	}

	void AppStateManager::fpsIndependentRendering(void)
	{
		std::chrono::nanoseconds lag(0);
		std::chrono::time_point<std::chrono::high_resolution_clock> endTime = std::chrono::high_resolution_clock::now();
		std::chrono::time_point<std::chrono::high_resolution_clock> fpsStartTime = endTime;

		int frameCount = 0;
		int tickCount = 0;

		/*this->getCameraManager()->getActiveCameraBehavior()->setMoveSpeed(20.0f);
		this->getCameraManager()->getActiveCameraBehavior()->setRotationSpeed(20.0f);
		this->getCameraManager()->getActiveCameraBehavior()->setSmoothValue(0.05f);*/

		Ogre::Window* renderWindow = Core::getSingletonPtr()->getOgreRenderWindow();
		this->setDesiredUpdates(Core::getSingletonPtr()->getOptionDesiredFramesUpdates());

		// Default 30 ticks per second
		const unsigned int simulationTickCount = Core::getSingletonPtr()->getOptionDesiredSimulationUpdates();

		Ogre::Real monitorRefreshRate = static_cast<Ogre::Real>(Core::getSingletonPtr()->getScreenRefreshRate());

		if (0 == monitorRefreshRate)
		{
			monitorRefreshRate = static_cast<Ogre::Real>(simulationTickCount);
		}

		if (simulationTickCount <= 30)
		{
			this->getCameraManager()->getActiveCameraBehavior()->setSmoothValue(0.05f);
		}
		else if (simulationTickCount <= 60)
		{
			this->getCameraManager()->getActiveCameraBehavior()->setSmoothValue(0.075f);
		}
		else if (simulationTickCount <= 144)
		{
			this->getCameraManager()->getActiveCameraBehavior()->setSmoothValue(0.05f);
		}

		if (this->desiredUpdates != 0 && this->desiredUpdates <= static_cast<unsigned int>(monitorRefreshRate))
		{
			monitorRefreshRate = static_cast<Ogre::Real>(this->desiredUpdates);
		}

		// Whole engine/game is processed constantly at 60 ticks per second, but rendering as fast graphics card speed!
		const std::chrono::nanoseconds lengthOfFrame(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(1)) / simulationTickCount);
		const Ogre::Real tickCountDt = 1.0f / simulationTickCount;
		const Ogre::Real framesDt = 1.0f / monitorRefreshRate;

		while (false == this->bShutdown)
		{
			// Process signals from system
			Ogre::WindowEventUtilities::messagePump();

			std::chrono::time_point<std::chrono::high_resolution_clock> endTime2 = std::chrono::high_resolution_clock::now();
			std::chrono::nanoseconds fpsElapsedTime(std::chrono::duration_cast<std::chrono::nanoseconds>(endTime2 - fpsStartTime));

			if (fpsElapsedTime >= std::chrono::seconds(1))
			{
				// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "frameCount: " + Ogre::StringConverter::toString(frameCount) + " tickCount: " + Ogre::StringConverter::toString(tickCount));

				// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "Screen Refresh Rate: " + Ogre::StringConverter::toString(Core::getSingletonPtr()->getScreenRefreshRate()));
				// getScreenRefreshRate = 144
				
				/*
				FrameCount: Graphics FPC, TickCount = Processed ticks for dt and updates of engine
				23:38:12: frameCount: 79 tickCount: 57
				23:38:13: frameCount: 1 tickCount: 157
				23:38:14: frameCount: 15 tickCount: 129
				23:38:15: frameCount: 84 tickCount: 61
				23:38:16: frameCount: 84 tickCount: 60
				23:38:17: frameCount: 84 tickCount: 61
				23:38:18: frameCount: 84 tickCount: 61
				23:38:19: frameCount: 84 tickCount: 60
				23:38:20: frameCount: 84 tickCount: 61
				23:38:21: frameCount: 84 tickCount: 61
				23:38:22: frameCount: 84 tickCount: 60
				23:38:23: frameCount: 84 tickCount: 61
				23:38:24: frameCount: 84 tickCount: 61
				23:38:25: frameCount: 83 tickCount: 60
				23:38:26: frameCount: 84 tickCount: 61
				23:38:27: frameCount: 84 tickCount: 60
				23:38:28: frameCount: 84 tickCount: 61
				23:38:29: frameCount: 84 tickCount: 61
				23:38:30: frameCount: 84 tickCount: 60
				*/
		

				Core::getSingletonPtr()->updateFrameStats(tickCountDt);

				fpsStartTime = std::chrono::high_resolution_clock::now();
				frameCount = 0;
				tickCount = 0;
			}

			std::chrono::time_point<std::chrono::high_resolution_clock> startTime = std::chrono::high_resolution_clock::now();

			std::chrono::nanoseconds elapsedTime(std::chrono::duration_cast<std::chrono::nanoseconds>(startTime - endTime));
			endTime = startTime;

			lag += elapsedTime;

			// Updates inputs
			InputDeviceCore::getSingletonPtr()->capture(tickCountDt);

			// Game processing is independent of rendering
			while (lag >= lengthOfFrame && false == this->bShutdown)
			{
				// update input devices
				if (false == this->bStall && false == this->activeStateStack.back()->gameProgressModule->stallUpdates())
				{
					// Updates the active state
					this->activeStateStack.back()->update(tickCountDt);
				}

				// Update core
				Core::getSingletonPtr()->update(tickCountDt);

				// Update the renderwindow if the window is not active too, for server/client analysis
				if (false == renderWindow->isVisible() && this->renderWhenInactive)
				{
					break;
				}

				//finish tick
				lag -= lengthOfFrame;

				// Keeps track of how many ticks have been done this second
				tickCount++;
			}

			// Update the renderwindow if the window is not active too, for server/client analysis
			if (false == renderWindow->isVisible() && this->renderWhenInactive)
			{
				// Do not burn CPU cycles unnecessary when minimized etc.
				Ogre::Threads::Sleep(500);
				// Sleep(500);
			}

			/******rendering comes after update, so its late update*****/
			if (false == this->bStall && false == this->activeStateStack.back()->gameProgressModule->stallUpdates())
			{
				// Note: dt is always constant 1 / 60 = 0.0167
				this->activeStateStack.back()->lateUpdate(tickCountDt);
			}

			// Controls the shown fps in game!
			this->bShutdown |= !Ogre::Root::getSingletonPtr()->renderOneFrame(framesDt);

			// Keeps track of how many frames are done this second
			frameCount++;
		}
	}

	void AppStateManager::internalChangeAppState(AppState* state)
	{
		AppState* oldState = nullptr;
		// end the state if present
		if (false == this->activeStateStack.empty())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AppStateManager] Exiting " + this->activeStateStack.back()->getName());
			this->activeStateStack.back()->exit();

			oldState = this->activeStateStack.back();
			
			this->activeStateStack.pop_back();
		}

		this->activeStateStack.push_back(state);

		// link input devices and gui with core
		this->linkInputWithCore(oldState, state);

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AppStateManager] Entering " + this->activeStateStack.back()->getName());
		// enter the new state
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
		// Links input devices and gui with core
		this->linkInputWithCore(oldState, state);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AppStateManager] Entering " + this->activeStateStack.back()->getName());
		// Enters the new state
		this->activeStateStack.back()->enter();

		this->bStall = false;

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
			this->activeStateStack.back()->exit();
			this->activeStateStack.pop_back();
		}

		// if there is a state left, continue
		if (false == this->activeStateStack.empty())
		{
			this->linkInputWithCore(oldState, this->activeStateStack.back());
			// if a state is paused, resume the state, in order to be able to continue
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AppStateManager] Resuming " + this->activeStateStack.back()->getName());
			this->activeStateStack.back()->resume();
		}
		else
		{
			this->shutdown();
		}
		this->bStall = false;
	}

	void AppStateManager::internalPopAllAndPushAppState(AppState* state)
	{
		while (false == this->activeStateStack.empty())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AppStateManager] Exiting " + this->activeStateStack.back()->getName());
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
			}
		}
		else
		{
			this->shutdown();
		}
		this->bStall = false;
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
			// Creates the delay process and changes the world at another tick. Note, this is necessary
			// because changing the world destroys all game objects and its components.
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
			// Creates the delay process and changes the world at another tick. Note, this is necessary
			// because changing the world destroys all game objects and its components.
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
			// Creates the delay process and changes the world at another tick. Note, this is necessary
			// because changing the world destroys all game objects and its components.
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
			// Creates the delay process and changes the world at another tick. Note, this is necessary
			// because changing the world destroys all game objects and its components.
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
			// NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.2f));
			// Creates the delay process and changes the world at another tick. Note, this is necessary
			// because changing the world destroys all game objects and its components.
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
	}

	bool AppStateManager::getIsShutdown(void) const
	{
		return this->bShutdown;
	}

	bool AppStateManager::getIsStalled(void) const
	{
		return this->bStall;
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

	MeshDecalGeneratorModule* AppStateManager::getMeshDecalGeneratorModule(void) const
	{
		if (true == this->activeStateStack.empty())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AppStateManager] Error getting mesh decal generator module, because at this time no application state (AppState) has been created! Maybe this call was to early.");
			throw Ogre::Exception(Ogre::Exception::ERR_INTERNAL_ERROR, "[AppStateManager] Error getting mesh decal generator module, because at this time no application state (AppState) has been created! Maybe this call was to early.", "NOWA");
		}
		return this->activeStateStack.back()->meshDecalGeneratorModule;
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

	ParticleUniverseModule* AppStateManager::getParticleUniverseModule(void) const
	{
		if (true == this->activeStateStack.empty())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AppStateManager] Error getting particle universe module, because at this time no application state (AppState) has been created! Maybe this call was to early.");
			throw Ogre::Exception(Ogre::Exception::ERR_INTERNAL_ERROR, "[AppStateManager] Error getting particle universe module, because at this time no application state (AppState) has been created! Maybe this call was to early.", "NOWA");
		}
		return this->activeStateStack.back()->particleUniverseModule;
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

	MeshDecalGeneratorModule* AppStateManager::getMeshDecalGeneratorModule(const Ogre::String& stateName)
	{
		AppState* appState = this->findByName(stateName);
		if (nullptr != appState)
			return appState->meshDecalGeneratorModule;

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

	ParticleUniverseModule* AppStateManager::getParticleUniverseModule(const Ogre::String& stateName)
	{
		AppState* appState = this->findByName(stateName);
		if (nullptr != appState)
			return appState->particleUniverseModule;

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

}; // namespace end
