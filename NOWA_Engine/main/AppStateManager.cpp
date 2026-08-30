#include "NOWAPrecompiled.h"

#include "AppStateManager.h"
#include "Core.h"
#include "Events.h"
#include "InputDeviceCore.h"
#include "ProcessManager.h"
#include "gameobject/AttributesComponent.h"
#include "modules/GameProgressModule.h"
#include "modules/GraphicsModule.h"
#include "modules/InputDeviceModule.h"
#include "utilities/Timer.h"
#include <chrono>

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
        explicit ChangeAppStateProcess(AppState* state, eAppStateOperation stateOperation) : state(state), stateOperation(stateOperation)
        {
        }

        explicit ChangeAppStateProcess(eAppStateOperation stateOperation) : state(nullptr), stateOperation(stateOperation)
        {
        }

    protected:
        virtual void onInit(void) override
        {
            this->succeed();

            boost::shared_ptr<EventDataLuaScriptModfied> eventDataLuaScriptModified(new EventDataLuaScriptModfied(0L, ""));
            NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataLuaScriptModified);

            ProcessManager::getInstance()->abortAllProcesses(true);

            // ── SAFE VECTOR CLEAR ───────────────────────────────────────────────────
            // requestStall() blocks until the render thread finishes its current
            // iteration and confirms it is not inside updateAllTransforms() or any
            // other vector-reading code. Only then is clearSceneResources() safe.
            auto* graphicsModule = GraphicsModule::getInstance();
            graphicsModule->requestStall();
            graphicsModule->clearSceneResources();
            graphicsModule->releaseStall();

            switch (this->stateOperation)
            {
            case eAppStateOperation::ChangeAppState:
                AppStateManager::getSingletonPtr()->internalChangeAppState(this->state);
                break;
            case eAppStateOperation::PushAppState:
                AppStateManager::getSingletonPtr()->internalPushAppState(this->state);
                break;
            case eAppStateOperation::PopAppState:
                AppStateManager::getSingletonPtr()->internalPopAppState();
                break;
            case eAppStateOperation::PopAllAndPushAppState:
                AppStateManager::getSingletonPtr()->internalPopAllAndPushAppState(this->state);
                break;
            case eAppStateOperation::ExitGame:
                AppStateManager::getSingletonPtr()->internalExitGame();
                break;
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

    AppStateManager::AppStateManager() : renderWhenInactive(false), lastTime(0), desiredUpdates(60), slowMotionMS(0), renderDelta(0), vsyncOn(true), bShutdown(false), bStall(false), bCanProcessRenderQueue(true)
    {
    }

    AppStateManager::~AppStateManager()
    {
        // Delete all global attributes
        auto it = this->globalAttributesMap.begin();

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

    bool AppStateManager::isSafeToDispatchEvents(void) const
    {
        if (true == this->bStall.load())
        {
            return false;
        }

        GameProgressModule* gameProgressModule = this->getActiveGameProgressModuleSafe();
        if (nullptr == gameProgressModule)
        {
            // No active game progress module - treat as "not safe" rather than
            // "safe", matching the existing handler's own default
            // (isSceneLoading defaults to true when gameProgressModule is null).
            return true;
        }

        return false == gameProgressModule->bSceneLoading.load();
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
        catch (std::exception& e)
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

            Core::getSingletonPtr()->initLuaConsole();

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

            // Attention: the render thread must die HERE - after every state has been
            // exited and destroyed (all of which uses enqueueAndWait/enqueueDestroy and
            // therefore needs a live consumer), but BEFORE Core is shut down and Ogre::Root
            // is torn down underneath a thread that may still be draining commands.
            // doCleanup() sets bRunning = false, joins the thread and clears
            // renderThreadAlive. It is idempotent, so MainApplication may call it again.
            NOWA::GraphicsModule::getInstance()->doCleanup();

            Core::getSingletonPtr()->setShutdown(true);
        }
        else
        {
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[AppStateManager] Error: Cannot start application state: '" + stateName + "' because it does not exist.");
            throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[AppStateManager] Error: Cannot start application state: '" + stateName + "' because it does not exist.\n", "NOWA");
        }
    }

    AppState* AppStateManager::findByName(const Ogre::String& stateName)
    {
        for (auto it = this->states.cbegin(); it != this->states.cend(); ++it)
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
        for (auto it = this->states.begin(); it != this->states.end(); ++it)
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
        // Guard against being called during engine teardown. MyGUI fires widget
        // callbacks (e.g. eventRootMouseChangeFocus) during Core::~Core() ->
        // myGui->shutdown() -> _destroyAllChildWidget(). If any MyGUIComponent
        // delegate is still registered at that point, it calls enqueue() which
        // then crashes inside isLogicThread() because the logicThreadId atomic
        // member (at offset 0x390) is already freed memory. Once bShutdown is
        // true, the logic queue is gone and there is nothing left to enqueue into.
        if (true == this->bShutdown)
        {
            return;
        }

        if (true == this->isLogicThread())
        {
            command();
        }
        else
        {
            this->queue.enqueue(std::move(command));
        }
    }

    void AppStateManager::enqueueAndWait(LogicCommand&& command)
    {
        // With this code its possible to call from render thread logic stuff with wait and from logic thread commands, which have inside other commands for render thread with wait!

        // Guard against being called during engine teardown. MyGUI fires widget
        // callbacks (e.g. eventRootMouseChangeFocus) during Core::~Core() ->
        // myGui->shutdown() -> _destroyAllChildWidget(). If any MyGUIComponent
        // delegate is still registered at that point, it calls enqueue() which
        // then crashes inside isLogicThread() because the logicThreadId atomic
        // member (at offset 0x390) is already freed memory. Once bShutdown is
        // true, the logic queue is gone and there is nothing left to enqueue into.
        if (true == this->bShutdown)
        {
            return;
        }

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
        std::atomic<bool> done{false};
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

            // Attention: Wakes a render thread that is parked in the CASE 2 pump loop below.
            // Without this it would only notice that we are finished when its wait times out,
            // which costs one more scheduler tick for every single logic command.
            NOWA::GraphicsModule::getInstance()->signalCommandWaiters();
        };

        // Enqueue command for the logic thread
        this->queue.enqueue(std::move(wrappedCommand));

        // CASE 1: Caller is a normal thread (not render thread)
        if (!calledFromRenderThread)
        {
            // Attention: never wait unbounded. Once the logic loop has ended nobody calls
            // processAll() anymore, so 'done' would never be set and this thread would hang
            // for the rest of the process lifetime.
            while (!done.load(std::memory_order_acquire))
            {
                if (false == this->logicQueueServiced.load(std::memory_order_acquire))
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AppStateManager] Logic command cannot be executed, because the logic loop has already ended. Giving up the wait.");
                    return;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            if (exceptionPtr)
            {
                std::rethrow_exception(exceptionPtr);
            }

            return;
        }

        // CASE 2: Caller is the RENDER THREAD:
        // We MUST keep processing render commands while waiting,
        // because the logic command may call ENQUEUE_RENDER_COMMAND_*_WAIT

        while (!done.load(std::memory_order_acquire))
        {
            // Attention: same as CASE 1 - once the logic loop has ended, nothing will ever
            // execute our command and this loop would spin forever, which during shutdown
            // also keeps the render thread from ever reaching its own join().
            if (false == this->logicQueueServiced.load(std::memory_order_acquire))
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AppStateManager] Logic command from the render thread cannot be executed, because the logic loop has already ended. Giving up the wait.");
                return;
            }

            // 1) Process any pending render commands
            graphics->processAllCommands();

            // 2) Pump window events so OS stays happy
            Ogre::WindowEventUtilities::messagePump();

            // 2b) Draw the loading indicator, if one is set. Throttled internally to a low frame
            //     rate, and a no-op when no indicator is active - so this costs nothing outside a
            //     scene load.
            //
            //     Attention: this loop is where the render thread actually IS during a scene load,
            //     which is why the indicator has to be driven from here and not from
            //     renderThreadFunction.
            graphics->renderLoadingFrameThrottled();

            // 3) Park until the next render command arrives or the logic thread signals that our
            //    command is done - instead of polling.
            //
            //    Attention: This used to be std::this_thread::sleep_for(1ms). On Windows the
            //    default scheduler granularity is ~15.6 ms, so that call parked this thread for a
            //    full timer tick. Every GraphicsModule::enqueueAndWait issued by the logic thread
            //    had to wait for the next pass of THIS loop, so each one cost ~15.6 ms. With about
            //    four render round trips per game object that was ~63 ms per object and 13 seconds
            //    for a 133 object scene - all of it spent sleeping, not working.
            //
            //    This is also the loop the render thread sits in for the whole scene import, which
            //    is why it never reaches renderThreadFunction's own loop during a load: no
            //    [RENDER-LOOP] heartbeat, no suspend branch, and every optimisation applied there
            //    was without effect.
            graphics->waitForCommandOrSignal(std::chrono::milliseconds(2));
        }

        if (exceptionPtr)
        {
            try
            {
                std::rethrow_exception(exceptionPtr);
            }
            catch (const std::exception& e)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AppStateManager] Error on managing a new state\n" + Ogre::String(e.what()));
            }
            catch (...)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AppStateManager] Error on managing a new state (unknown exception)");
            }
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

#if 0
	void AppStateManager::multiThreadedRendering(void)
    {
        this->markCurrentThreadAsLogicThread();

#ifdef _DEBUG
        const double fixedDt = 1.0 / 60.0; // 60Hz in debug — gives more headroom
        const double maxDeltaTime = fixedDt * 8.0; // allow up to 8 steps of catch-up
        const int maxStepsPerFrame = 4;
#else
        const double fixedDt = 1.0 / static_cast<double>(Core::getSingletonPtr()->getOptionDesiredSimulationUpdates());
        const double maxDeltaTime = fixedDt * 2.0;
        const int maxStepsPerFrame = 2;
#endif
        // const double maxDeltaTime = 0.25;
        // const int maxStepsPerFrame = 5;

        Ogre::Window* renderWindow = Core::getSingletonPtr()->getOgreRenderWindow();
        this->setDesiredUpdates(Core::getSingletonPtr()->getOptionDesiredFramesUpdates());

        double currentTime = static_cast<double>(Core::getSingletonPtr()->getOgreTimer()->getMilliseconds()) * 0.001;
        double accumulator = 0.0;

        NOWA::GraphicsModule* graphicsModule = NOWA::GraphicsModule::getInstance();
        graphicsModule->setFrameTime(static_cast<Ogre::Real>(fixedDt));

        while (false == this->bShutdown)
        {
            Ogre::WindowEventUtilities::messagePump();

            const double newTime = static_cast<double>(Core::getSingletonPtr()->getOgreTimer()->getMilliseconds()) * 0.001;
            double frameTime = newTime - currentTime;
            currentTime = newTime;

            frameTime = std::min(frameTime, maxDeltaTime);
            accumulator += frameTime;

            if (false == this->bStall && false == this->activeStateStack.back()->gameProgressModule->bSceneLoading)
            {
                this->activeStateStack.back()->renderUpdate(static_cast<Ogre::Real>(frameTime));
            }

            bool didUpdate = false;
            int steps = 0;

            while (accumulator >= fixedDt && steps < maxStepsPerFrame)
            {
                if (!didUpdate)
                {
                    graphicsModule->beginLogicFrame();
                    didUpdate = true;
                }

                this->processAll();

                if (false == this->bStall && false == this->activeStateStack.back()->gameProgressModule->bSceneLoading)
                {
                    // The outer loop already steps time in fixedDt increments.
                    // AppState::update() must call ogreNewt->update(fixedDt), NOT
                    // updateWithInternalAccumulator() — that function owns its own
                    // accumulator and must never be driven from inside this loop.
                    this->activeStateStack.back()->update(static_cast<Ogre::Real>(fixedDt));
                }

                Core::getSingletonPtr()->updateFrameStats(static_cast<Ogre::Real>(fixedDt));
                Core::getSingletonPtr()->update(static_cast<Ogre::Real>(fixedDt));

                accumulator -= fixedDt;
                ++steps;
            }

//#ifdef _DEBUG
//            // In debug, let accumulator grow freely — don't bleed time
//            // (bleeding causes slow motion when debug overhead is high)
//#else
//            if (accumulator >= fixedDt)
//            {
//                accumulator = fixedDt * 0.5;
//            }
//#endif

            const float alpha = (fixedDt > 0.0) ? static_cast<float>(accumulator / fixedDt) : 0.0f;
            graphicsModule->publishInterpolationAlpha(alpha);

            if (false == renderWindow->isVisible() && this->renderWhenInactive)
            {
                Ogre::Threads::Sleep(500);
            }
        }

        this->bStall = true;

		// Drain the queue before this thread exits.
        // moodycamel::ConcurrentQueue stores per-thread producer state that is
        // cleaned up in ThreadExitNotifier::~ThreadExitNotifier() when this thread
        // exits. If any commands are still enqueued at that point, the cleanup
        // callback tries to return tokens to the queue object which may already
        // be in an inconsistent state — causing a deadlock.
        // Draining here while the thread is still alive and the queue is fully
        // valid avoids that entirely.
        this->clearLogicQueue();
    }
#endif

#if 1
    void AppStateManager::multiThreadedRendering(void)
    {
        this->markCurrentThreadAsLogicThread();

        const double fixedDt = 1.0 / static_cast<double>(Core::getSingletonPtr()->getOptionDesiredSimulationUpdates());
        // Allow real catch-up instead of dropping time on every minor hitch.
        // This must be large enough to consume whatever maxDeltaTime can dump
        // into the accumulator in one frame.
        const int maxStepsPerFrame = 8;
        const double maxDeltaTime = fixedDt * maxStepsPerFrame;

        Ogre::Window* renderWindow = Core::getSingletonPtr()->getOgreRenderWindow();
        this->setDesiredUpdates(Core::getSingletonPtr()->getOptionDesiredFramesUpdates());

        double currentTime = static_cast<double>(Core::getSingletonPtr()->getOgreTimer()->getMilliseconds()) * 0.001;
        double accumulator = 0.0;

        NOWA::GraphicsModule* graphicsModule = NOWA::GraphicsModule::getInstance();
        graphicsModule->setFrameTime(static_cast<Ogre::Real>(fixedDt));

        // Configure fps performance profiles
        WorkspaceModule::getInstance()->configureAdaptiveQuality(
            {
                {/*shadowFarDistance*/ 500.0f, /*foliageDistanceMultiplier*/ 1.0f}, // level 0: best
                {300.0f, 0.9f},                                                     // level 1
                {150.0f, 0.8f},                                                     // level 2
                {80.0f, 0.75f},                                                     // level 3: worst
            },
            /*targetFrameTimeMs*/ 16.6f);

        // From here on processAll() runs every iteration, so enqueueAndWait() from the
        // render thread has a consumer and may block. See the flag's use in enqueueAndWait().
        this->logicQueueServiced.store(true, std::memory_order_release);

        while (false == this->bShutdown)
        {
            Ogre::WindowEventUtilities::messagePump();

            const double newTime = static_cast<double>(Core::getSingletonPtr()->getOgreTimer()->getMilliseconds()) * 0.001;
            double frameTime = newTime - currentTime;
            currentTime = newTime;

            frameTime = std::min(frameTime, maxDeltaTime);
            accumulator += frameTime;

            if (false == this->bStall && false == this->activeStateStack.back()->gameProgressModule->bSceneLoading)
            {
                this->activeStateStack.back()->renderUpdate(static_cast<Ogre::Real>(frameTime));
            }

            int steps = 0;

            while (accumulator >= fixedDt && steps < maxStepsPerFrame)
            {
                graphicsModule->beginLogicFrame(); // advance the buffer for THIS step, every step

                this->processAll();

                Core::getSingletonPtr()->updateFrameStats(static_cast<Ogre::Real>(fixedDt));
                Core::getSingletonPtr()->update(static_cast<Ogre::Real>(fixedDt));

                if (false == this->bStall && false == this->activeStateStack.back()->gameProgressModule->bSceneLoading)
                {
                    this->activeStateStack.back()->update(static_cast<Ogre::Real>(fixedDt));
                }

                accumulator -= fixedDt;
                ++steps;

                graphicsModule->endLogicFrame(); // publish this step's snapshot as ready, every step
            }

            // Spiral-of-death guard -- only trips if we genuinely couldn't catch up
            // even after maxStepsPerFrame steps. This should be rare/never in
            // practice now that maxStepsPerFrame matches maxDeltaTime.
            if (steps == maxStepsPerFrame && accumulator >= fixedDt)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AppStateManager]: Logic thread falling behind, dropping " + Ogre::StringConverter::toString(static_cast<float>(accumulator)) + "s of backlog.");
                accumulator = std::fmod(accumulator, fixedDt); // keep phase instead of an arbitrary half-step
            }

            const float alpha = (fixedDt > 0.0) ? static_cast<float>(accumulator / fixedDt) : 0.0f;
            graphicsModule->publishInterpolationAlpha(alpha);

            if (false == renderWindow->isVisible() && this->renderWhenInactive)
            {
                Ogre::Threads::Sleep(500);
            }
        }

        // Shutdown phase begins here.
        // Attention: bStall alone would make the render thread skip rendering, but the render
        // thread must ALSO keep servicing the command queue, because the whole teardown that
        // follows (state exit -> GameObjectController::stop -> disconnect -> enqueueAndWait)
        // blocks on the render thread executing those commands. beginShutdownDrain() keeps the
        // render loop alive for exactly that purpose. It is stopped later in
        // GraphicsModule::doCleanup(), which must run AFTER the teardown is complete.
        // Attention: from here on nobody calls processAll() anymore, so a logic command
        // enqueued by the render thread would never run. enqueueAndWait() must stop
        // blocking on it - see the flag's use there.
        this->logicQueueServiced.store(false, std::memory_order_release);

        this->bStall = true;
        graphicsModule->beginShutdownDrain();

        // Attention: do NOT clear the logic queue here anymore. GameObjectController::stop()
        // pushes its teardown command into this very queue afterwards.
        // this->clearLogicQueue();
    }
#endif

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
            Ogre::String errorMessage = "[AppStateManager] Error on change to new state: '" + this->activeStateStack.back()->appStateName +
                                        "' because the event manager and other modules are null. Maybe the modules have not been initialized in the enter method. See: 'initializeModules(true, true);'";

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
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AppStateManager] Can not push appstate, because the " + this->activeStateStack.back()->getName() + " state is paused");
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
        for (auto it = this->activeStateStack.begin(); it != this->activeStateStack.end();)
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
        bool wasSignaled = logicFrameCondVar.wait_for(lock, std::chrono::milliseconds(100),
            [this]
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
        logicFrameCondVar.wait(lock,
            [this]
            {
                return logicFrameFinished;
            });
        logicFrameFinished = false; // Reset the flag for the next frame
    }
#endif

    void AppStateManager::changeAppState(AppState* state)
    {
        // Guard against a second transition being requested while the first one is still
        // in flight (e.g. reloadCurrentState() immediately followed by changeAppState() in
        // the same call/turn, before the ChangeAppStateProcess attached by the first call
        // has actually run and reset bStall). Without this, the second call would attach
        // ANOTHER ChangeAppStateProcess; ProcessManager::updateProcesses() runs both in the
        // same pass, so the second transition starts tearing down a half-built state from
        // the first - the crash this was reported for. Silently dropping it here is safe:
        // the caller gets exactly what a normal double-click would have gotten, one transition.
        if (true == this->bStall)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AppStateManager] changeAppState ignored: a state transition is already in progress.");
            return;
        }

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
            // Creates the process and changes the scene at another tick. Note, this is necessary
            // because changing the scene destroys all game objects and its components.
            // So changing the state directly inside a component would create a mess, since everything will be destroyed
            // and the game object map in update loop becomes invalid while its iterating
            NOWA::ProcessManager::getInstance()->attachProcess(NOWA::ProcessPtr(new ChangeAppStateProcess(state, eAppStateOperation::ChangeAppState)));
        }
    }

    bool AppStateManager::pushAppState(AppState* state)
    {
        // See changeAppState() for why this guard exists.
        if (true == this->bStall)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AppStateManager] pushAppState ignored: a state transition is already in progress.");
            return false;
        }

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
        // See changeAppState() for why this guard exists.
        if (true == this->bStall)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AppStateManager] popAppState ignored: a state transition is already in progress.");
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
            // delayProcess->attachChild(NOWA::ProcessPtr(new ChangeAppStateProcess(eAppStateOperation::PopAppState)));
            // NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
            NOWA::ProcessManager::getInstance()->attachProcess(NOWA::ProcessPtr(new ChangeAppStateProcess(eAppStateOperation::PopAppState)));
        }
    }

    void AppStateManager::popAllAndPushAppState(AppState* state)
    {
        // See changeAppState() for why this guard exists.
        if (true == this->bStall)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AppStateManager] popAllAndPushAppState ignored: a state transition is already in progress.");
            return;
        }

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
        // See changeAppState() for why this guard exists.
        if (true == this->bStall)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AppStateManager] exitGame ignored: a state transition is already in progress.");
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
            // delayProcess->attachChild(NOWA::ProcessPtr(new ChangeAppStateProcess(eAppStateOperation::ExitGame)));
            // NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
            NOWA::ProcessManager::getInstance()->attachProcess(NOWA::ProcessPtr(new ChangeAppStateProcess(eAppStateOperation::ExitGame)));
        }
    }

    void AppStateManager::linkInputWithCore(AppState* oldState, AppState* state)
    {
        // Everything that touches InputDeviceCore listener containers must run on render thread.
        auto doLink = [oldState, state]()
        {
            if (nullptr != oldState)
            {
                InputDeviceCore::getSingletonPtr()->removeKeyListener(oldState);
                InputDeviceCore::getSingletonPtr()->removeMouseListener(oldState);
                InputDeviceCore::getSingletonPtr()->removeJoystickListener(oldState);
            }

            // If a listener has been added via key/mouse/joystick pressed, a new listener would be inserted during this iteration,
            // which would cause a crash in mouse/key/button release iterator, hence add in next frame
            NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.25f));

            auto ptrFunction = [state]()
            {
                if (nullptr == state)
                {
                    return;
                }

                // Remove first for precaution, in order to prevent duplicate state
                InputDeviceCore::getSingletonPtr()->removeKeyListener(state);
                InputDeviceCore::getSingletonPtr()->removeMouseListener(state);
                InputDeviceCore::getSingletonPtr()->removeJoystickListener(state);

                // Keep your existing signature/order (as in your codebase)
                InputDeviceCore::getSingletonPtr()->addKeyListener(state, state->getName());
                InputDeviceCore::getSingletonPtr()->addMouseListener(state, state->getName());
                InputDeviceCore::getSingletonPtr()->addJoystickListener(state, state->getName());
            };

            NOWA::ProcessPtr closureProcess(new NOWA::ClosureProcess(ptrFunction));
            delayProcess->attachChild(closureProcess);

            // Attach on render thread (because the closure touches InputDeviceCore)
            NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
        };

        // If we're already on render thread, run directly. Otherwise hop to render thread.
        if (true == NOWA::GraphicsModule::getInstance()->isRenderThread())
        {
            doLink();
        }
        else
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [doLink]()
            {
                doLink();
            };
            NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "AppStateManager::linkInputWithCore");
        }
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
        {
            return this->activeStateStack.back();
        }
        return nullptr;
    }

    void AppStateManager::setSlowMotion(unsigned int slowMotionMS)
    {
        this->slowMotionMS = slowMotionMS;
    }

    void AppStateManager::reloadCurrentState(void)
    {
        if (true == this->activeStateStack.empty())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AppStateManager] reloadCurrentState: no active state to reload.");
            return;
        }

        // Re-use the existing changeAppState path: passing the SAME pointer causes
        // internalChangeAppState to call exit() then enter() on the same object,
        // which destroys all MyGUI widgets and recreates them at the new viewport size.
        this->changeAppState(this->activeStateStack.back());
    }

    void AppStateManager::reloadCurrentStateThenChangeAppState(AppState* nextState)
    {
        // Same guard as every other transition entry point - see changeAppState() for why.
        if (true == this->bStall)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AppStateManager] reloadCurrentStateThenChangeAppState ignored: a state transition is already in progress.");
            return;
        }

        if (true == this->activeStateStack.empty())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AppStateManager] reloadCurrentStateThenChangeAppState: no active state to reload.");
            return;
        }

        if (nullptr == nextState)
        {
            if (true == Core::getSingletonPtr()->getIsGame())
            {
                Ogre::String errorMessage = "[AppStateManager] Error: Cannot reload and then change appstate, because the new app state is null!";
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, errorMessage);
                throw Ogre::Exception(Ogre::Exception::ERR_INTERNAL_ERROR, errorMessage, "NOWA");
            }
            return;
        }

        if (true == Core::getSingletonPtr()->getIsGame())
        {
            this->bStall = true;

            // Two use cases (reloadCurrentState() followed by changeAppState()) used to be
            // fired as two independent top-level calls in the same turn - e.g. apply a new
            // resolution (needs a reload of the CURRENT state so MyGUI picks up the new
            // viewport size), then leave to MenuState. Both attached their own
            // ChangeAppStateProcess; ProcessManager ran them in the same pass, and the second
            // one started tearing down a state the first one had only half rebuilt. The
            // bStall guard above now stops that outright - but it also means the second call
            // is simply dropped, so the state never actually changes (the ordering problem
            // reported after adding the guard).
            //
            // Chain them properly instead: attach the "switch to nextState" step as a CHILD
            // of the "reload current state" step. ChangeAppStateProcess::onInit() runs
            // internalChangeAppState() synchronously to completion (including resetting
            // bStall) before calling succeed() - so by the time the child process starts, the
            // reload is fully finished, MyGUI included. One caller-visible call, one
            // guaranteed order, no manual sequencing needed from Lua/component code.
            NOWA::ProcessPtr reloadProcess(new ChangeAppStateProcess(this->activeStateStack.back(), eAppStateOperation::ChangeAppState));
            reloadProcess->attachChild(NOWA::ProcessPtr(new ChangeAppStateProcess(nextState, eAppStateOperation::ChangeAppState)));
            NOWA::ProcessManager::getInstance()->attachProcess(reloadProcess);
        }
    }

    void AppStateManager::reloadCurrentStateThenChangeAppState(const Ogre::String& nextStateName)
    {
        this->reloadCurrentStateThenChangeAppState(this->findByName(nextStateName));
    }

    void AppStateManager::shutdown(void)
    {
        // Attention: this must ONLY end the logic loop. It must NOT call
        // AppState::stopRendering() anymore.
        //
        // stopRendering() sets GraphicsModule::bRunning to false, which makes the render
        // thread leave its loop, drain the queue, run the deferred destroys and clear the
        // pools - all of that immediately, while the logic thread is still finishing its
        // current iteration and has not even started the teardown yet. The teardown that
        // follows (start() -> AppState::exit() -> GameObjectController::stop() ->
        // GameObject::disconnect()) is full of enqueueAndWait() calls, and every one of
        // them then waits for a consumer that no longer exists. That was the shutdown
        // deadlock.
        //
        // The render thread is now stopped and joined in GraphicsModule::doCleanup(),
        // which runs at the very end of start(), after every state has been destroyed.
        this->bShutdown = true;
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
        {
            return appState->gameObjectController;
        }

        return nullptr;
    }

    GameProgressModule* AppStateManager::getGameProgressModule(const Ogre::String& stateName)
    {
        AppState* appState = this->findByName(stateName);
        if (nullptr != appState)
        {
            return appState->gameProgressModule;
        }

        return nullptr;
    }

    RakNetModule* AppStateManager::getRakNetModule(const Ogre::String& stateName)
    {
        AppState* appState = this->findByName(stateName);
        if (nullptr != appState)
        {
            return appState->rakNetModule;
        }

        return nullptr;
    }

    MiniMapModule* AppStateManager::getMiniMapModule(const Ogre::String& stateName)
    {
        AppState* appState = this->findByName(stateName);
        if (nullptr != appState)
        {
            return appState->miniMapModule;
        }

        return nullptr;
    }

    OgreNewtModule* AppStateManager::getOgreNewtModule(const Ogre::String& stateName)
    {
        AppState* appState = this->findByName(stateName);
        if (nullptr != appState)
        {
            return appState->ogreNewtModule;
        }

        return nullptr;
    }

    DecalsModule* AppStateManager::getDecalsModule(const Ogre::String& stateName)
    {
        AppState* appState = this->findByName(stateName);
        if (nullptr != appState)
        {
            return appState->decalsModule;
        }

        return nullptr;
    }

    CameraManager* AppStateManager::getCameraManager(const Ogre::String& stateName)
    {
        AppState* appState = this->findByName(stateName);
        if (nullptr != appState)
        {
            return appState->cameraManager;
        }

        return nullptr;
    }

    OgreRecastModule* AppStateManager::getOgreRecastModule(const Ogre::String& stateName)
    {
        AppState* appState = this->findByName(stateName);
        if (nullptr != appState)
        {
            return appState->ogreRecastModule;
        }

        return nullptr;
    }

    ParticleFxModule* AppStateManager::getParticleFxModule(const Ogre::String& stateName)
    {
        AppState* appState = this->findByName(stateName);
        if (nullptr != appState)
        {
            return appState->particleFxModule;
        }
        return nullptr;
    }

    LuaScriptModule* AppStateManager::getLuaScriptModule(const Ogre::String& stateName)
    {
        AppState* appState = this->findByName(stateName);
        if (nullptr != appState)
        {
            return appState->luaScriptModule;
        }

        return nullptr;
    }

    EventManager* AppStateManager::getEventManager(const Ogre::String& stateName)
    {
        AppState* appState = this->findByName(stateName);
        if (nullptr != appState)
        {
            return appState->eventManager;
        }

        return nullptr;
    }

    ScriptEventManager* AppStateManager::getScriptEventManager(const Ogre::String& stateName)
    {
        AppState* appState = this->findByName(stateName);
        if (nullptr != appState)
        {
            return appState->scriptEventManager;
        }

        return nullptr;
    }

    Variant* AppStateManager::getGlobalValue(const Ogre::String& attributeName)
    {
        Variant* globalAttribute = nullptr;
        auto it = this->globalAttributesMap.find(attributeName);
        if (it != this->globalAttributesMap.cend())
        {
            globalAttribute = it->second;
        }
        return globalAttribute;
    }

    Variant* AppStateManager::setGlobalBoolValue(const Ogre::String& attributeName, bool value)
    {
        Variant* globalAttribute = nullptr;
        auto it = this->globalAttributesMap.find(attributeName);
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
        auto it = this->globalAttributesMap.find(attributeName);
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
        auto it = this->globalAttributesMap.find(attributeName);
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
        auto it = this->globalAttributesMap.find(attributeName);
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
        auto it = this->globalAttributesMap.find(attributeName);
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
        auto it = this->globalAttributesMap.find(attributeName);
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
        auto it = this->globalAttributesMap.find(attributeName);
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
        auto it = this->globalAttributesMap.find(attributeName);
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
        auto it = this->globalAttributesMap.find(attributeName);
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
            {
                continue;
            }

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
            auto gameObjectPtr = this->getGameObjectController(this->activeStateStack.back()->getName())->getGameObjectFromId(id);
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
            {
                break;
            }

            data = Ogre::StringUtil::split(line, "=");
            if (data.size() < 3)
            {
                continue;
            }

            Variant* globalAttribute = nullptr;
            auto it = this->globalAttributesMap.find(data[0]);
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
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameProgressModule] ERROR: Could not create file for path: " + saveFilePathName + "'");
                return;
            }

            // Store whether the file should be crypted or not
            outFile << crypted << "\n";

            // Get the game object controller for this app state name
            auto gameObjects = this->getGameObjectController(this->activeStateStack.back()->getName())->getGameObjects();

            std::ostringstream oStream;

            for (auto it = gameObjects->cbegin(); it != gameObjects->cend(); ++it)
            {
                const auto gameObjectPtr = it->second;

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
            for (auto it = this->globalAttributesMap.cbegin(); it != this->globalAttributesMap.cend(); ++it)
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
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameProgressModule] ERROR: Could not get file path name for saving data: " + saveFilePathName + "'");
        }
    }
}; // namespace end