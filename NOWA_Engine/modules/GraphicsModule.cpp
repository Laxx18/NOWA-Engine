#include "NOWAPrecompiled.h"
#include "GraphicsModule.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "main/InputDeviceCore.h"
#include "utilities/LoadingIndicator.h"

// Attention: TEMPORARY diagnostic for the loading indicator. Comment out when done.
#define NOWA_LOADING_INDICATOR_TIMING

#include <Animation/OgreBone.h>
#include <chrono>
#include <condition_variable>

// Attention: TEMPORARY diagnostic for the suspended-render wait and for the render loop itself.
// Comment the define out once the measurement is done. Everything it adds is aggregated, never one
// log line per iteration.
// #define NOWA_SUSPEND_WAIT_TIMING
// #define CLOSURE_DEBUG

#ifdef NOWA_SUSPEND_WAIT_TIMING

namespace
{
    // Render thread only - no synchronisation needed.
    size_t g_renderLoopIterations = 0;
    size_t g_renderLoopParkedIterations = 0;
    std::chrono::steady_clock::time_point g_renderLoopLastHeartbeat = std::chrono::steady_clock::now();
}
#endif
#include <mutex>
#include <sstream>

namespace
{
    std::chrono::milliseconds g_defaultTimeout(5000); // 5 seconds

#ifdef CLOSURE_DEBUG
    // --- TEMPORARY closure-flood / slow-render-iteration diagnostics ---
    // Render thread only, no locking needed. Toggle via DEBUG_CLOSURE above.
    Ogre::Real g_renderDt = 0.0f;

    struct ClosureCommandDiag
    {
        size_t count;
        size_t adds;
        size_t updates;
        size_t fireAndForget;
        size_t removals;

        ClosureCommandDiag() : count(0), adds(0), updates(0), fireAndForget(0), removals(0)
        {
        }
    };

    std::unordered_map<Ogre::String, ClosureCommandDiag> g_closureCommandDiagnostics;

    void logClosureFloodDiagnostics(void)
    {
        std::vector<std::pair<Ogre::String, ClosureCommandDiag>> sorted(g_closureCommandDiagnostics.begin(), g_closureCommandDiagnostics.end());

        std::sort(sorted.begin(), sorted.end(),
            [](const std::pair<Ogre::String, ClosureCommandDiag>& a, const std::pair<Ogre::String, ClosureCommandDiag>& b)
            {
                return a.second.count > b.second.count;
            });

        Ogre::LogManager::getSingletonPtr()->logMessage("[GraphicsModule] Closure flood diagnostics - distinct names this frame: " + Ogre::StringConverter::toString(sorted.size()) + ", renderDt: " + Ogre::StringConverter::toString(g_renderDt),
            Ogre::LML_NORMAL);

        const size_t maxNamesToLog = 10;
        size_t namesLogged = 0;
        for (const auto& entry : sorted)
        {
            if (namesLogged >= maxNamesToLog)
            {
                break;
            }

            std::stringstream ss;
            ss << "[GraphicsModule]   '" << entry.first << "' count=" << entry.second.count << " (add=" << entry.second.adds << " update=" << entry.second.updates << " fireAndForget=" << entry.second.fireAndForget
               << " removal=" << entry.second.removals << ")";
            Ogre::LogManager::getSingletonPtr()->logMessage(ss.str(), Ogre::LML_NORMAL);

            ++namesLogged;
        }
    }
#endif
}

namespace NOWA
{
    using namespace RenderGlobals;

    GraphicsModule::GraphicsModule() :
        bRunning(false),
        timeoutEnabled(false),
        timeoutDuration(g_defaultTimeout.count()),
        logLevel(Ogre::LML_NORMAL),
        currentTransformNodeIdx(0),
        currentTransformCameraIdx(0),
        currentTransformBoneIdx(0),
        currentTrackedDatablockIdx(0),
        interpolationWeight(0.0f),
        accumTimeSinceLastLogicFrame(0.0f),
        frameTime(1.0f / 60.0f),
        currentRenderDt(0.0f),
        debugVisualization(false),
        currentDestroySlot(0),
        closureQueue(),
        producerToken(closureQueue),
        consumerToken(closureQueue),
        stallRequested(false),
        stallAcknowledged(false),
        wasStalledOrLoading(false),
        renderingSuspended(false),
        commandPending(false),
        renderThreadParked(false),
        loadingIndicator(nullptr),
        loadingFrameIntervalSeconds(1.0f / 20.0f)
    {
        // Note: nodePool and its five siblings are std::deque, not std::vector - they
        // grow only via push_back/emplace_back under their category mutex (see the
        // threading-model comment in the header) and are never reserved/preallocated
        // up front the way the old vector was; growth is rare in steady state because
        // freed slots are recycled via the free-list, so there is no equivalent
        // "reserve(100)" call needed here.
        this->nodePool.resize(GraphicsModule::NODE_POOL_CAPACITY);
        this->freeNodeSlots.reserve(GraphicsModule::NODE_POOL_CAPACITY);
        for (size_t i = 0; i < GraphicsModule::NODE_POOL_CAPACITY; ++i)
        {
            this->freeNodeSlots.push_back(i);
        }

        this->cameraPool.resize(GraphicsModule::CAMERA_POOL_CAPACITY);
        this->freeCameraSlots.reserve(GraphicsModule::CAMERA_POOL_CAPACITY);
        for (size_t i = 0; i < GraphicsModule::CAMERA_POOL_CAPACITY; ++i)
        {
            this->freeCameraSlots.push_back(i);
        }

        this->bonePool.resize(GraphicsModule::BONE_POOL_CAPACITY);
        this->freeBoneSlots.reserve(GraphicsModule::BONE_POOL_CAPACITY);
        for (size_t i = 0; i < GraphicsModule::BONE_POOL_CAPACITY; ++i)
        {
            this->freeBoneSlots.push_back(i);
        }

        this->datablockPool.resize(GraphicsModule::DATABLOCK_POOL_CAPACITY);
        this->freeDatablockSlots.reserve(GraphicsModule::DATABLOCK_POOL_CAPACITY);
        for (size_t i = 0; i < GraphicsModule::DATABLOCK_POOL_CAPACITY; ++i)
        {
            this->freeDatablockSlots.push_back(i);
        }

        this->queueInitialized.store(true);
    }

    GraphicsModule::~GraphicsModule()
    {
    }

    MyGUI::Widget* GraphicsModule::getMyGUIFocusWidget(void)
    {
        // If called from the render thread, query directly — no stale cached value
        if (true == this->isRenderThread())
        {
            return MyGUI::InputManager::getInstancePtr()->getMouseFocusWidget();
        }
        // Logic thread reads the value cached last render frame
        return this->myGUIFocusWidget.load(std::memory_order_relaxed);
    }

    GraphicsModule* GraphicsModule::getInstance()
    {
        static GraphicsModule instance;
        return &instance;
    }

    void GraphicsModule::startRendering(void)
    {
        this->bRunning = true;

        // Attention: must be set BEFORE the thread is spawned. enqueueAndWait() uses this
        // flag to decide whether a foreign thread may execute a command inline, and a logic
        // thread that gets here first would otherwise run Ogre commands on itself.
        this->renderThreadAlive.store(true, std::memory_order_release);

        this->renderThread = std::thread(&GraphicsModule::renderThreadFunction, this);
    }

    void GraphicsModule::stopRendering(void)
    {
        // Attention: this only asks the render thread to leave its main loop. It does NOT
        // mean the render thread is gone - it still drains the queue, runs the deferred
        // destroy slots and clears the pools afterwards. Use renderThreadAlive (not
        // bRunning) to decide whether inline execution on a foreign thread is safe.
        this->bRunning = false;
    }

    bool GraphicsModule::getIsRunning(void) const
    {
        return this->bRunning;
    }

    void GraphicsModule::renderThreadFunction(void)
    {
        // Advertise this thread's identity to Core so enqueueAndWait thread-ownership assertions work correctly.
        Core::getSingletonPtr()->setRenderThreadId(std::this_thread::get_id());

        this->markCurrentThreadAsRenderThread();

        this->setTimeoutDuration(std::chrono::milliseconds(10000));

        const float fixedDt = 1.0f / float(NOWA::Core::getSingletonPtr()->getOptionDesiredSimulationUpdates());
        this->setFrameTime(fixedDt);

        // Attention: The timeout must stay enabled in debug builds too. Disabling it turns every
        // producer/consumer mismatch into an unbreakable hang instead of a logged warning.
#ifdef _DEBUG
        this->enableTimeout(true);
        this->setLogLevel(Ogre::LML_TRIVIAL);
#else
        this->setLogLevel(Ogre::LML_TRIVIAL);
#endif

        static int frameCount = 0;

        // Attention: std::chrono::steady_clock instead of Ogre::Timer. Ogre's Win32 Timer wraps
        // EVERY QueryPerformanceCounter read in a SetThreadAffinityMask() pair, pinning this
        // thread to a single core (usually core 0) and unpinning it again - two kernel
        // transitions plus, whenever the thread was not already on that core, a real thread
        // migration with the corresponding cache and TLB loss. That happens once per loop
        // iteration, which during a suspended scene import is about a thousand times per second,
        // and in normal operation on the very thread that also runs renderOneFrame().
        //
        // Ogre's Timer additionally MUTATES member state on every read (mStartTime / mLastTime,
        // for its GetTickCount based leap compensation), so sharing one instance across threads
        // is a data race that can make the returned time jump - and a jumping deltaTime is a
        // classic cause of visual stutter.
        auto lastFrameTime = std::chrono::steady_clock::now();

        Ogre::Window* renderWindow = NOWA::Core::getSingletonPtr()->getOgreRenderWindow();
        const auto appStateManager = NOWA::AppStateManager::getSingletonPtr();

        while (true == this->bRunning)
        {
            // -- STALL HANDSHAKE -----------------------------------------------------
            // Must be FIRST: logic thread may have called requestStall() and is waiting
            // to finish the previous iteration before it touches vectors.
            // Attention: The logic thread must NEVER call enqueueAndWait() between
            // requestStall() and releaseStall(), because commands are deliberately not
            // serviced while parked here.
            if (this->stallRequested.load(std::memory_order_acquire))
            {
                // Acknowledge: we are at a frame boundary, not inside any vector loop
                this->stallAcknowledged.store(true, std::memory_order_release);

                // Park here while logic thread does clearSceneResources() / state switch
                while (this->stallRequested.load(std::memory_order_acquire))
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }

                // Logic thread called releaseStall() -- resume normal rendering
                this->stallAcknowledged.store(false, std::memory_order_release);
                continue;
            }

#ifdef CLOSURE_DEBUG
            // TEMPORARY: per-stage timing to find which stage causes slow render
            // iterations. Toggle via DEBUG_CLOSURE above.
            // Attention: steady_clock here too - see the note on lastFrameTime above. The stage
            // timings are in microseconds as before, so the log formatting below is unchanged.
            const auto stageTimerStart = std::chrono::steady_clock::now();
            Ogre::uint64 tCommands = 0;
            Ogre::uint64 tTransforms = 0;
            Ogre::uint64 tRenderOneFrame = 0;
            Ogre::uint64 tClosures = 0;

            auto stageElapsedMicroseconds = [&stageTimerStart]() -> Ogre::uint64
            {
                return static_cast<Ogre::uint64>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - stageTimerStart).count());
            };
#endif

            // -- COMMAND SERVICE -----------------------------------------------------
            // Unconditional and before every early-out below. The logic thread blocks in
            // enqueueAndWait() until these run, so skipping this in ANY branch (stall,
            // scene loading, shutdown drain) deadlocks the logic thread.
            this->processAllCommands();

#ifdef CLOSURE_DEBUG
            tCommands = stageElapsedMicroseconds();
#endif

            // -- SHUTDOWN DRAIN ------------------------------------------------------
            // The logic loop has ended and the teardown (state exit, GameObjectController::stop,
            // scene destruction) is running on the logic thread. No rendering, no closures,
            // no transform interpolation -- only keep the command queue alive.
            if (this->shutdownDrain.load(std::memory_order_acquire))
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            const auto currentTime = std::chrono::steady_clock::now();
            const Ogre::Real deltaTime = std::chrono::duration<Ogre::Real>(currentTime - lastFrameTime).count();
            lastFrameTime = currentTime;
            this->currentRenderDt = deltaTime;

#ifdef CLOSURE_DEBUG
            g_renderDt = deltaTime;
#endif

            GameProgressModule* gameProgressModule = appStateManager->getActiveGameProgressModuleSafe();
            // Attention: renderingSuspended is GraphicsModule's OWN flag, set by
            // suspendRendering() resp. the ScopedRenderSuspend guard around scene loading. It is
            // deliberately independent of AppStateManager::bStall, whose ownership is shared with
            // internalChangeAppState / internalPushAppState / the shutdown path, and independent
            // of GameProgressModule::bSceneLoading, which only the game side maintains. Both were
            // tried first and neither reliably reached this loop, so every enqueueAndWait kept
            // waiting for the running renderOneFrame() - about 16 ms per round trip.
            // Folding it into isStalled here makes BOTH branches below honour it.
            const bool isStalled = appStateManager->bStall.load() || this->renderingSuspended.load(std::memory_order_acquire);
            const bool isSceneLoading = (gameProgressModule != nullptr) ? gameProgressModule->bSceneLoading.load() : false;

#ifdef NOWA_SUSPEND_WAIT_TIMING
            // Attention: TEMPORARY. Unconditional heartbeat so we can see WHERE the render thread
            // actually spends the import, instead of inferring it. Render thread only, so plain
            // file-scope statics are fine.
            {
                ++g_renderLoopIterations;

                const auto heartbeatNow = std::chrono::steady_clock::now();
                if (heartbeatNow - g_renderLoopLastHeartbeat >= std::chrono::milliseconds(500))
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                        "[RENDER-LOOP] iterations=" + Ogre::StringConverter::toString(g_renderLoopIterations) + " parked=" + Ogre::StringConverter::toString(g_renderLoopParkedIterations) + " isStalled=" + Ogre::StringConverter::toString(isStalled) +
                            " isSceneLoading=" + Ogre::StringConverter::toString(isSceneLoading) + " renderingSuspended=" + Ogre::StringConverter::toString(this->renderingSuspended.load(std::memory_order_acquire)) +
                            " workspaceTransitioning=" + Ogre::StringConverter::toString(this->isWorkspaceTransitioning()) + " queueSize=" + Ogre::StringConverter::toString(this->queue.size_approx()));

                    g_renderLoopLastHeartbeat = heartbeatNow;
                    g_renderLoopIterations = 0;
                    g_renderLoopParkedIterations = 0;
                }
            }
#endif

            if (false == isStalled && false == isSceneLoading)
            {
                this->wasStalledOrLoading = false;

                auto* workspaceModule = AppStateManager::getSingletonPtr()->getWorkspaceModule();
                if (nullptr != workspaceModule)
                {
                    workspaceModule->updateAdaptiveQuality(deltaTime);
                }

                NOWA::InputDeviceCore::getSingletonPtr()->capture(deltaTime);
                this->advanceFrameAndDestroyOld();

                const float alpha = this->consumeInterpolationAlpha();
                this->setInterpolationWeight(alpha);
                this->updateAllTransforms();

#ifdef CLOSURE_DEBUG
                tTransforms = stageElapsedMicroseconds();
#endif

                if (++frameCount % 300 == 0)
                {
                    this->waitForRenderCompletion();
                    this->dumpBufferState();
                    frameCount = 0;
                }
            }
            else
            {
                // Stalled or scene loading.
                // Vector clears are now exclusively the logic thread's job,
                // done safely inside requestStall()/clearSceneResources()/releaseStall().
                // Only clear closure state here -- it is render-thread-owned.
                //
                // Attention: clearAllClosures() must NOT run on every spin. This branch has no
                // renderOneFrame() to pace it, so during a scene load it executes thousands of
                // times per second, and each pass drains the closure queue and walks
                // persistentClosures for nothing. Worse, whenever it does clear something it
                // writes an Ogre log line, and Ogre's LogManager flushes to disk per message -
                // which turns the spin into a disk I/O storm exactly while the logic thread is
                // trying to load. Clear ONCE on entering the state instead.
                if (false == this->wasStalledOrLoading)
                {
                    this->clearAllClosures();
                    this->wasStalledOrLoading = true;
                }

                // Attention: This MUST NOT be a plain sleep_for(1ms). On Windows the default
                // scheduler granularity is ~15.6 ms, so sleep_for(1ms) actually parks the thread
                // for a full tick - and every enqueueAndWait round trip then costs those ~15.6 ms
                // instead of microseconds. That is indistinguishable from waiting for a VSync
                // frame and completely defeats the point of suspending the rendering.
                //
                // The condition variable is notified by enqueue(), so a new command wakes this
                // thread immediately. The short timeout only bounds how long it takes to notice
                // that the suspend flag was cleared again; the commandPending predicate closes
                // the race where enqueue() notifies between processAllCommands() above and the
                // wait below.
                // Attention: renderThreadParked is published BEFORE the lock is taken, so the
                // window in which enqueue() could miss the notify is as small as possible. Should
                // it still happen, the commandPending predicate makes the very next wait_for
                // return immediately, and the 2 ms timeout is the hard upper bound.
#ifdef NOWA_SUSPEND_WAIT_TIMING
                const auto suspendWaitStart = std::chrono::steady_clock::now();
                ++g_renderLoopParkedIterations;
#endif

                this->waitForCommandOrSignal(std::chrono::milliseconds(2));

#ifdef NOWA_SUSPEND_WAIT_TIMING
                {
                    // Attention: render thread only, so plain statics are fine here.
                    static double suspendWaitAccumulatedMillis = 0.0;
                    static size_t suspendWaitCount = 0;
                    static size_t suspendCommandsSeen = 0;

                    suspendWaitAccumulatedMillis += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - suspendWaitStart).count();
                    ++suspendWaitCount;
                    suspendCommandsSeen += static_cast<size_t>(this->queue.size_approx());

                    if (suspendWaitCount >= 20)
                    {
                        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[SUSPEND-WAIT] waits=" + Ogre::StringConverter::toString(suspendWaitCount) +
                                                                                                " avgMillis=" + Ogre::StringConverter::toString(suspendWaitAccumulatedMillis / static_cast<double>(suspendWaitCount)) +
                                                                                                " queuedOnWake=" + Ogre::StringConverter::toString(suspendCommandsSeen));

                        suspendWaitAccumulatedMillis = 0.0;
                        suspendWaitCount = 0;
                        suspendCommandsSeen = 0;
                    }
                }
#endif
            }

            if (false == isStalled && false == this->isWorkspaceTransitioning() && false == isSceneLoading)
            {
                Ogre::Root::getSingletonPtr()->renderOneFrame();

#ifdef CLOSURE_DEBUG
                tRenderOneFrame = stageElapsedMicroseconds();
#endif

                // Execute closures AFTER renderOneFrame so RenderingMetrics are
                // populated when closures read them (e.g. DesignState::updateInfo).
                // Node/bone/datablock interpolation already ran in updateAllTransforms
                // above before renderOneFrame, so visual correctness is preserved.
                this->updateAndExecuteClosures();

#ifdef CLOSURE_DEBUG
                tClosures = stageElapsedMicroseconds();

                // TEMPORARY: log a stage breakdown whenever the whole iteration took
                // more than 100ms.
                if (tClosures > 100000)
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage("[GraphicsModule] Slow render iteration - processAllCommands: " + Ogre::StringConverter::toString(tCommands / 1000.0) + "ms, updateAllTransforms: " +
                                                                        Ogre::StringConverter::toString((tTransforms - tCommands) / 1000.0) + "ms, renderOneFrame: " + Ogre::StringConverter::toString((tRenderOneFrame - tTransforms) / 1000.0) +
                                                                        "ms, updateAndExecuteClosures: " + Ogre::StringConverter::toString((tClosures - tRenderOneFrame) / 1000.0) + "ms",
                        Ogre::LML_NORMAL);
                }
#endif
            }
        }

        while (this->hasPendingRenderCommands())
        {
            this->processAllCommands();
        }

        // Now it's safe to do frame advancement.
        // Attention: after beginShutdownDrain() flushed the ring-buffer and enqueueDestroy()
        // stopped deferring, these slots should be empty. Anything still in here was
        // enqueued after the scene was torn down and is very likely to hold dangling Ogre
        // pointers, so make it visible in the log instead of just crashing in the lambda.
        size_t leftoverDestroyCommands = 0;
        for (size_t i = 0; i < NOWA::GraphicsModule::NUM_DESTROY_SLOTS; ++i)
        {
            leftoverDestroyCommands += this->destroySlots[i].size();
        }

        if (leftoverDestroyCommands > 0)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GraphicsModule]: " + Ogre::StringConverter::toString(leftoverDestroyCommands) +
                                                                                    " destroy commands were still deferred at render thread exit. They were enqueued after the shutdown drain began and may reference already destroyed Ogre objects.");
        }

        for (size_t i = 0; i < NOWA::GraphicsModule::NUM_DESTROY_SLOTS; ++i)
        {
            this->advanceFrameAndDestroyOld();
        }

        // The deferred destroy commands above may themselves have enqueued work, so drain
        // one more time before declaring the queue empty.
        while (this->hasPendingRenderCommands())
        {
            this->processAllCommands();
        }

        // Check for remaining commands.
        // Attention: this must NOT throw. A bare 'throw;' outside a catch block calls
        // std::terminate(), and even a real exception is fatal here because nothing on the
        // render thread catches it. Logging is the only useful thing we can do.
        const int remainingCommands = this->queue.size_approx();
        if (remainingCommands > 0)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[RenderCommandQueueModule]: Illegal state, as there are still: " + Ogre::StringConverter::toString(remainingCommands) + " pending commands!");
        }

        this->clearAllClosures();

        // Terminal cleanup only - the engine is shutting down and bRunning has
        // already dropped out of the while-loop above, so no other thread can
        // still be resolving/updating a slot. This is the ONE place it is safe
        // to actually clear() the pools (as opposed to tombstoning, which is
        // what clearSceneResources() must do instead - see the threading-model
        // comment in the header for why).
        this->nodePool.clear();
        this->nodeToIndexMap.clear();
        this->freeNodeSlots.clear();

        this->cameraPool.clear();
        this->cameraToIndexMap.clear();
        this->freeCameraSlots.clear();

        this->bonePool.clear();
        this->boneToIndexMap.clear();
        this->freeBoneSlots.clear();

        this->datablockPool.clear();
        this->datablockToIndexMap.clear();
        this->freeDatablockSlots.clear();

        this->bRunning = false;
        this->shutdownDrain = false;
        this->timeoutEnabled = false;
        this->timeoutDuration = g_defaultTimeout.count();
        this->logLevel = Ogre::LML_NORMAL;
        this->currentTransformNodeIdx = 0;
        this->currentTransformCameraIdx = 0;
        this->currentTransformBoneIdx = 0;
        this->currentTrackedDatablockIdx = 0;

        this->accumTimeSinceLastLogicFrame = 0.0f;

        this->frameTime = 1.0f / 60.0f;
        this->debugVisualization = false;
        this->currentDestroySlot = 0;

        // Attention: MUST be the very last statement of this function. From here on any
        // enqueueAndWait() from a foreign thread executes inline instead of blocking on a
        // consumer that no longer exists. Setting it any earlier would let the logic thread
        // touch the device while we are still draining and clearing the pools above.
        this->renderThreadAlive.store(false, std::memory_order_release);
    }

    void GraphicsModule::beginShutdownDrain(void)
    {
        // Called from the logic thread right after its main loop ended, BEFORE any teardown
        // (state exit, GameObjectController::stop, scene destruction) is executed.
        // The render thread stops rendering but keeps servicing the command queue, so that
        // enqueueAndWait() from the teardown path can still complete.

        // Attention: the deferred destroy ring-buffer MUST be flushed here, while the scene,
        // the SceneManager and everything the destroy commands captured are still alive.
        // The two-frame delay of destroySlots only means anything while frames are actually
        // being rendered. From here on no frame is rendered anymore, so anything left in the
        // ring-buffer would only be executed at the very end of renderThreadFunction - long
        // after Core::destroyScene() killed the SceneManager, which turns every captured
        // Ogre::SceneNode* into a dangling pointer (crash in UserObjectBindings::clear()).
        //
        // The flush runs as a queued command so it executes on the render thread, which is
        // still servicing the queue at this point (shutdownDrain is set only afterwards).
        this->enqueueAndWait(
            [this]()
            {
                for (size_t i = 0; i < GraphicsModule::NUM_DESTROY_SLOTS; ++i)
                {
                    this->advanceFrameAndDestroyOld();
                }
            },
            "GraphicsModule::beginShutdownDrain::flushDestroySlots");

        this->shutdownDrain.store(true, std::memory_order_release);
    }

    void GraphicsModule::publishInterpolationAlpha(float alpha)
    {
        // Clamp hard: we never want NaNs or >1 to leak into interpolation
        if (!(alpha == alpha)) // NaN check
        {
            alpha = 0.0f;
        }

        alpha = std::clamp(alpha, 0.0f, 1.0f);

        // Release so render thread sees this after logic wrote it
        m_interpolationAlpha.store(alpha, std::memory_order_release);
    }

    void GraphicsModule::publishLogicFrame()
    {
        // Release: marks the moment a new snapshot/buffer state is ready
        m_logicFrameId.fetch_add(1, std::memory_order_release);
    }

    float GraphicsModule::consumeInterpolationAlpha() const
    {
        // Acquire pairs with logic release stores
        return m_interpolationAlpha.load(std::memory_order_acquire);
    }

    uint64_t GraphicsModule::getLogicFrameId() const
    {
        return m_logicFrameId.load(std::memory_order_acquire);
    }

    void GraphicsModule::setInterpolationWeight(float w)
    {
        if (!(w == w))
        {
            w = 0.0f;
        }
        w = std::clamp(w, 0.0f, 1.0f);

        // CRITICAL: update the variable updateAllTransforms() actually uses
        this->interpolationWeight = w;
    }

    void GraphicsModule::beginWorkspaceTransition(void)
    {
        workspaceTransitionInProgress = true;
    }

    void GraphicsModule::endWorkspaceTransition(void)
    {
        workspaceTransitionInProgress = false;
    }

    bool GraphicsModule::isWorkspaceTransitioning(void) const
    {
        return workspaceTransitionInProgress;
    }

    void GraphicsModule::clearAllClosures(void)
    {
        // Attention: closureQueue's consumer token and persistentClosures are owned by the
        // RENDER thread exclusively. A moodycamel ConsumerToken is not thread safe, and the
        // render thread iterates persistentClosures in executeActiveClosures(). Calling this
        // from the logic thread (as GameObjectController::stop() does) while the render
        // thread is in its stall branch means two threads dequeue through the same token,
        // which is undefined behaviour. Any foreign thread is therefore routed through the
        // command queue instead.
        if (false == this->isRenderThread())
        {
            if (true == this->renderThreadAlive.load(std::memory_order_acquire))
            {
                this->enqueueAndWait(
                    [this]()
                    {
                        this->clearAllClosures();
                    },
                    "clearAllClosures");
                return;
            }

            // No render thread at all - nobody can race us, so doing it here is safe.
            // Attention: the token-less overload on purpose, the consumer token belongs to
            // the (now dead) render thread.
            ClosureCommand shutdownCommand;
            size_t shutdownClearedCommands = 0;
            while (this->closureQueue.try_dequeue(shutdownCommand))
            {
                ++shutdownClearedCommands;
            }

            const size_t shutdownClearedPersistent = this->persistentClosures.size();
            this->persistentClosures.clear();

            Ogre::LogManager::getSingleton().logMessage("[GraphicsModule] Cleared " + Ogre::StringConverter::toString(shutdownClearedCommands) + " queued closure commands and " + Ogre::StringConverter::toString(shutdownClearedPersistent) +
                                                            " persistent closures (no render thread)",
                Ogre::LML_NORMAL);
            return;
        }

        // Clear the concurrent queue - drain all pending commands
        ClosureCommand command;
        size_t clearedCommands = 0;
        while (this->closureQueue.try_dequeue(consumerToken, command))
        {
            ++clearedCommands;
        }

        // Attention: read the size BEFORE clearing. The original log read it afterwards and
        // therefore always reported zero persistent closures.
        const size_t clearedPersistent = this->persistentClosures.size();

        // Clear all persistent closures
        this->persistentClosures.clear();

        // Log the cleanup for debugging
        if (clearedCommands > 0 || clearedPersistent > 0)
        {
            Ogre::LogManager::getSingleton().logMessage("[GraphicsModule] Cleared " + Ogre::StringConverter::toString(clearedCommands) + " queued closure commands and " + Ogre::StringConverter::toString(clearedPersistent) + " persistent closures",
                Ogre::LML_NORMAL);
        }
    }

    void GraphicsModule::clearSceneResources(void)
    {
        // IMPORTANT: this tombstones every slot in place - it must NOT call clear()
        // on a pool's deque. Doing so would physically free chunk memory that some
        // thread's thread_local cache might still hold a raw pointer into; the next
        // time that thread used the stale pointer it would be a use-after-free
        // instead of a harmless "identity mismatch, re-resolve" miss. See the
        // threading-model comment near the top of the header.
        {
            std::lock_guard<std::mutex> lock(this->nodeRegistrationMutex);
            for (auto& slot : this->nodePool)
            {
                slot.node.store(nullptr, std::memory_order_relaxed);
                slot.active.store(false, std::memory_order_relaxed);
            }
            this->nodeToIndexMap.clear();
            this->freeNodeSlots.clear();
            for (size_t i = 0; i < this->nodePool.size(); ++i)
            {
                this->freeNodeSlots.push_back(i);
            }
        }

        {
            std::lock_guard<std::mutex> lock(this->cameraRegistrationMutex);
            for (auto& slot : this->cameraPool)
            {
                slot.camera.store(nullptr, std::memory_order_relaxed);
                slot.active.store(false, std::memory_order_relaxed);
            }
            this->cameraToIndexMap.clear();
            this->freeCameraSlots.clear();
            for (size_t i = 0; i < this->cameraPool.size(); ++i)
            {
                this->freeCameraSlots.push_back(i);
            }
        }

        {
            std::lock_guard<std::mutex> lock(this->boneRegistrationMutex);
            for (auto& slot : this->bonePool)
            {
                slot.bone.store(nullptr, std::memory_order_relaxed);
                slot.active.store(false, std::memory_order_relaxed);
            }
            this->boneToIndexMap.clear();
            this->freeBoneSlots.clear();
            for (size_t i = 0; i < this->bonePool.size(); ++i)
            {
                this->freeBoneSlots.push_back(i);
            }
        }

        {
            std::lock_guard<std::mutex> lock(this->datablockRegistrationMutex);
            for (auto& slot : this->datablockPool)
            {
                slot.datablock.store(nullptr, std::memory_order_relaxed);
                slot.active.store(false, std::memory_order_relaxed);
            }
            this->datablockToIndexMap.clear();
            this->freeDatablockSlots.clear();
            for (size_t i = 0; i < this->datablockPool.size(); ++i)
            {
                this->freeDatablockSlots.push_back(i);
            }
        }

        this->currentTransformNodeIdx = 0;
        this->currentTransformCameraIdx = 0;
        this->currentTransformBoneIdx = 0;
        this->currentTrackedDatablockIdx = 0;
        this->interpolationWeight = 0.0f;
        this->accumTimeSinceLastLogicFrame = 0.0f;

        // 6. Clear Pending Destruction Commands (destroySlots)
        // The multi-frame delayed destruction queue must be cleared immediately.
        for (size_t i = 0; i < GraphicsModule::NUM_DESTROY_SLOTS; ++i)
        {
            this->destroySlots[i].clear();
        }

        // 7. Reset the internal destroy slot index
        this->currentDestroySlot = 0;
    }

    void GraphicsModule::doCleanup(void)
    {
        // Attention: this must be called AFTER the whole logic side teardown (state exit ->
        // GameObjectController::stop() -> scene destruction) has finished, because all of
        // that still needs the render thread to service the command queue.
        //
        // Attention: bRunning must be cleared HERE. Nothing else in the shutdown path does
        // it, and join() on a render thread whose 'while (true == this->bRunning)' never
        // ends blocks forever.
        this->bRunning = false;
        this->shutdownDrain.store(false, std::memory_order_release);

        // Wait for render thread to finish before cleanup
        if (this->renderThread.joinable())
        {
            this->renderThread.join();
        }

        // Belt and braces: renderThreadFunction() clears this as its last statement, but if
        // the thread was never started (or already joined earlier) it must be false here too,
        // so that late destructors calling enqueueAndWait() execute inline instead of hanging.
        this->renderThreadAlive.store(false, std::memory_order_release);
    }

    void GraphicsModule::enqueue(RenderCommand&& command, const char* commandName, std::shared_ptr<std::promise<void>> promise)
    {
        if (!command)
        {
            // Null command — fulfill the promise immediately so the caller doesn't hang.
            if (promise)
            {
                try
                {
                    promise->set_value();
                }
                catch (const std::future_error&)
                { /* already set, harmless */
                }
            }
            return;
        }

        if (true == this->isRenderThread())
        {
            this->logCommandEvent(std::string("Executing '") + commandName + "' directly on render thread", Ogre::LML_TRIVIAL);
            try
            {
                command();
                if (promise)
                {
                    try
                    {
                        promise->set_value();
                    }
                    catch (const std::future_error&)
                    { /* already set */
                    }
                }
            }
            catch (const std::exception& e)
            {
                this->logCommandEvent(std::string("Exception in direct execution: ") + e.what(), Ogre::LML_CRITICAL);
                if (promise)
                {
                    try
                    {
                        promise->set_exception(std::current_exception());
                    }
                    catch (const std::future_error&)
                    { /* already set */
                    }
                }
            }
            catch (...)
            {
                this->logCommandEvent("Unknown exception in direct execution", Ogre::LML_CRITICAL);
                if (promise)
                {
                    try
                    {
                        promise->set_exception(std::current_exception());
                    }
                    catch (const std::future_error&)
                    { /* already set */
                    }
                }
            }
            return;
        }

        // Normal path — enqueue for render thread to process
        CommandEntry entry;
        entry.command = std::move(command);
        entry.completionPromise = promise;
        this->queue.enqueue(std::move(entry));

        // Attention: Wakes the render thread when it is parked in the suspended / stalled branch
        // of renderThreadFunction. Without this, a suspended render thread only notices the new
        // command when its sleep expires - and on Windows the default scheduler granularity is
        // ~15.6 ms, NOT the 1 ms that std::this_thread::sleep_for(1ms) suggests. Every
        // enqueueAndWait round trip then costs a full scheduler tick, which looks exactly like
        // waiting for a VSync frame and is what made scene loading appear unchanged after the
        // rendering was already suspended.
        //
        // COST IN THE NORMAL PATH: no mutex is taken here, ever. While the render thread is
        // rendering (the overwhelmingly common case) this is one relaxed atomic store plus one
        // acquire load of renderThreadParked, i.e. a couple of nanoseconds and no kernel call.
        // notify_one() - the only part that reaches the OS - runs ONLY while the render thread is
        // actually parked, which happens exclusively during a suspend or a stall.
        this->signalCommandWaiters();

        this->logCommandEvent("Command " + Ogre::String(commandName) + " enqueued, queue size: " + Ogre::StringConverter::toString(this->queue.size_approx()), Ogre::LML_TRIVIAL);
    }

    void GraphicsModule::processAllCommands(void)
    {
        if (false == this->isRenderThread())
        {
            this->logCommandEvent("processAllCommands called from non-render thread!", Ogre::LML_CRITICAL);
            return;
        }

        g_renderCommandDepth++;

        CommandEntry entry;
        while (this->pop(entry))
        {
            try
            {
                if (entry.command)
                {
                    this->logCommandEvent("Executing command, queue size: " + Ogre::StringConverter::toString(this->queue.size_approx()), Ogre::LML_TRIVIAL);
                    entry.command();

                    // Attention: the former 'this->isRunningWaitClosure = false;' was removed
                    // here. That flag is owned by the WAITING thread, not by us. Clearing it
                    // from the render thread cancelled the destroy-deferral in
                    // enqueueDestroy() while the logic thread was still blocked in
                    // enqueueAndWait(). It is now RenderGlobals::g_insideWaitClosure, which
                    // is thread_local and therefore cannot be clobbered across threads.
                }

                if (entry.completionPromise)
                {
                    try
                    {
                        entry.completionPromise->set_value();
                    }
                    catch (const std::future_error&)
                    { /* already set inside command */
                    }
                }
            }
            catch (const std::exception& e)
            {
                this->logCommandEvent(std::string("Exception in processAllCommands: ") + e.what(), Ogre::LML_CRITICAL);
                if (entry.completionPromise)
                {
                    try
                    {
                        entry.completionPromise->set_exception(std::current_exception());
                    }
                    catch (const std::future_error&)
                    { /* already set */
                    }
                }
            }
            catch (...)
            {
                this->logCommandEvent("Unknown exception in processAllCommands", Ogre::LML_CRITICAL);
                if (entry.completionPromise)
                {
                    try
                    {
                        entry.completionPromise->set_exception(std::current_exception());
                    }
                    catch (const std::future_error&)
                    { /* already set */
                    }
                }
            }
        }

        // cleanupFulfilledPromises() call removed — tracking system eliminated
        g_renderCommandDepth--;
    }

    void GraphicsModule::waitForRenderCompletion(void)
    {
        // If we're already on the render thread, just process the queue directly
        if (true == this->isRenderThread())
        {
            this->logCommandEvent("waitForRenderCompletion called from render thread - processing queue directly", Ogre::LML_NORMAL);
            this->processAllCommands();
            return;
        }

        // No consumer -> there is nothing to synchronize against, and waiting would hang.
        if (false == this->renderThreadAlive.load(std::memory_order_acquire))
        {
            this->logCommandEvent("waitForRenderCompletion called without a render thread - draining the queue instead", Ogre::LML_NORMAL);
            this->processQueueSync();
            return;
        }

        // Submit a command that acts as a sync point and wait until it has been executed.
        // Attention: this deliberately goes through enqueueAndWait() instead of duplicating
        // the wait logic. The original code did a bare future.wait() with no timeout and no
        // liveness check, which hangs the logic thread if the render thread stops servicing
        // the queue - exactly the shutdown deadlock this whole path had to be hardened for.
        this->enqueueAndWait(
            [this]()
            {
                this->logCommandEvent("Processing queue from waitForRenderCompletion", Ogre::LML_NORMAL);
            },
            "waitForRenderCompletion");

        this->logCommandEvent("Render queue synchronization complete", Ogre::LML_NORMAL);
    }

    bool GraphicsModule::pop(CommandEntry& commandEntry)
    {
        return this->queue.try_dequeue(commandEntry);
    }

    bool GraphicsModule::hasPendingRenderCommands(void) const
    {
        return this->queue.size_approx() > 0;
    }

    void GraphicsModule::enqueueAndWait(RenderCommand&& command, const char* commandName)
    {
        // Attention: g_insideWaitClosure is thread_local and must be saved and restored, not
        // blindly cleared on exit. A nested call would otherwise clear it while the outer
        // call is still running.
        const bool previousInsideWaitClosure = g_insideWaitClosure;
        g_insideWaitClosure = true;

        // -- Inline execution paths --------------------------------------------------
        // Two cases must never go through the queue:
        // 1) We ARE the render thread. Enqueueing to ourselves and then waiting for
        //    ourselves is a guaranteed self-deadlock. The thread identity decides this,
        //    NOT g_renderCommandDepth: a closure, a MyGUI callback or an Ogre listener
        //    runs on the render thread with depth 0 and must still take this path.
        // 2) There is no render thread (not started yet, or already joined). Note this
        //    checks renderThreadAlive and NOT bRunning: between stopRendering() and the
        //    render thread actually returning it still drains the queue and touches the
        //    device, so inline execution on a foreign thread would be a data race.
        const bool isOnRenderThread = this->isRenderThread();
        const bool hasNoConsumer = (false == this->renderThreadAlive.load(std::memory_order_acquire));

        if (true == isOnRenderThread || true == hasNoConsumer)
        {
            try
            {
                if (true == isOnRenderThread)
                {
                    this->logCommandEvent(std::string("Executing '") + commandName + "' directly on render thread with **WAIT** (re-entrant)", Ogre::LML_TRIVIAL);
                }
                else
                {
                    this->logCommandEvent(std::string("Executing '") + commandName + "' inline with **WAIT**, because there is no render thread", Ogre::LML_NORMAL);
                }

                command();

                this->flushDeferredDestroyCommands();
                g_insideWaitClosure = previousInsideWaitClosure;
                return;
            }
            catch (const std::exception& e)
            {
                this->logCommandEvent(std::string("Exception in direct execution of '") + commandName + "': " + e.what(), Ogre::LML_CRITICAL);
                this->flushDeferredDestroyCommands();
                g_insideWaitClosure = previousInsideWaitClosure;
                throw;
            }
            catch (...)
            {
                this->logCommandEvent(std::string("Unknown exception in direct execution of '") + commandName + "'", Ogre::LML_CRITICAL);
                this->flushDeferredDestroyCommands();
                g_insideWaitClosure = previousInsideWaitClosure;
                throw;
            }
        }

        // -- Normal path (logic thread -> render thread) ------------------------------
        this->incrementWaitDepth();

        try
        {
            auto promise = std::make_shared<std::promise<void>>();
            auto future = promise->get_future();

            this->logCommandEvent(std::string("Enqueueing '") + commandName + "' with **WAIT**", Ogre::LML_TRIVIAL);

            // Attention: with the thread-identity guard above in place this must not happen
            // anymore. g_waitDepth is thread_local, the render thread returns before
            // incrementWaitDepth(), and the logic thread cannot re-enter while it is blocked.
            const bool isNested = this->isInNestedWait() && g_waitDepth > 1;

            if (true == isNested)
            {
                this->logCommandEvent(std::string("Command '") + commandName + "' is a nested **WAIT** (depth: " + std::to_string(g_waitDepth) + ") and will NOT be waited for. This should no longer be reachable", Ogre::LML_CRITICAL);
            }

            this->enqueue(std::move(command), commandName, promise);

            if (false == isNested)
            {
                // Attention: this waits in BLOCKING slices, it does not spin. The former
                // 1ms-slice + std::this_thread::yield() loop burned a full core while the
                // render thread was busy, which on a loaded machine actively starved the
                // very thread we are waiting for. A promise/future wait_for wakes on notify,
                // so a long slice costs no extra latency - it only bounds how often we get
                // to re-check liveness.
                const std::chrono::milliseconds sliceDuration(50);

                // Attention: a timeout must NEVER abandon the wait while the render thread is
                // alive. A render thread that is compiling an Hlms shader or loading a 2.5 MB
                // skeleton is WORKING, not hung - and returning here without having executed
                // the command leaves it to run later against a dead caller stack frame. The
                // timeout is now only a diagnostic: it logs that we are waiting unusually
                // long and keeps waiting. The ONLY exit without execution is a render thread
                // that has really ended.
                const auto waitStart = std::chrono::steady_clock::now();
                const bool timeoutIsEnabled = this->isTimeoutEnabled();
                const auto timeoutValue = this->getTimeoutDuration();

                bool isReady = false;
                bool hasGivenUp = false;
                bool hasWarned = false;

                while (false == isReady && false == hasGivenUp)
                {
                    if (std::future_status::ready == future.wait_for(sliceDuration))
                    {
                        isReady = true;
                        break;
                    }

                    // The render thread vanished while we were waiting. Draining the queue
                    // ourselves is only safe once it has really returned, which is exactly
                    // what renderThreadAlive tells us.
                    if (false == this->renderThreadAlive.load(std::memory_order_acquire))
                    {
                        this->logCommandEvent(std::string("Render thread ended while waiting for '") + commandName + "', draining queue on this thread", Ogre::LML_CRITICAL);
                        this->processQueueSync();
                        hasGivenUp = true;
                        break;
                    }

                    if (true == timeoutIsEnabled && false == hasWarned)
                    {
                        if ((std::chrono::steady_clock::now() - waitStart) >= timeoutValue)
                        {
                            this->logCommandEvent(std::string("Still waiting for '") + commandName + "' after " + Ogre::StringConverter::toString(static_cast<int>(timeoutValue.count())) +
                                                      "ms. The render thread is alive but slow (shader compilation, resource load, long frame). Continuing to wait",
                                Ogre::LML_CRITICAL);
                            hasWarned = true;
                        }
                    }
                }
            }
        }
        catch (const std::exception& e)
        {
            this->logCommandEvent(std::string("Exception waiting for '") + commandName + "': " + e.what(), Ogre::LML_CRITICAL);
            this->decrementWaitDepth();
            this->flushDeferredDestroyCommands();
            g_insideWaitClosure = previousInsideWaitClosure;
            throw;
        }
        catch (...)
        {
            this->logCommandEvent(std::string("Unknown exception waiting for '") + commandName + "'", Ogre::LML_CRITICAL);
            this->decrementWaitDepth();
            this->flushDeferredDestroyCommands();
            g_insideWaitClosure = previousInsideWaitClosure;
            throw;
        }

        this->decrementWaitDepth();
        this->logCommandEvent(std::string("Command '") + commandName + "' completed", Ogre::LML_TRIVIAL);

        this->flushDeferredDestroyCommands();
        g_insideWaitClosure = previousInsideWaitClosure;
    }

    void GraphicsModule::processQueueSync(void)
    {
        // If we're on the render thread, process directly
        if (true == this->isRenderThread())
        {
            this->logCommandEvent("Processing queue synchronously on render thread", Ogre::LML_NORMAL);
            this->processAllCommands();
            return;
        }

        // Attention: this is a LAST RESORT, called from enqueueAndWait() once the render
        // thread has ended. It must NEVER enqueue-and-wait again: the original version
        // pushed a sync command and then did a bare syncFuture.wait(), which hangs forever
        // precisely because nobody is servicing the queue anymore.
        if (true == this->renderThreadAlive.load(std::memory_order_acquire))
        {
            this->logCommandEvent("processQueueSync called while the render thread is still alive - refusing to touch the queue from a foreign thread", Ogre::LML_CRITICAL);
            return;
        }

        this->logCommandEvent("Render thread is gone - draining the command queue on the calling thread", Ogre::LML_CRITICAL);

        CommandEntry entry;
        size_t drainedCommands = 0;

        while (this->pop(entry))
        {
            try
            {
                if (entry.command)
                {
                    entry.command();
                    ++drainedCommands;
                }

                if (entry.completionPromise)
                {
                    try
                    {
                        entry.completionPromise->set_value();
                    }
                    catch (const std::future_error&)
                    { /* already set inside command */
                    }
                }
            }
            catch (const std::exception& e)
            {
                this->logCommandEvent(std::string("Exception while draining the queue: ") + e.what(), Ogre::LML_CRITICAL);
                if (entry.completionPromise)
                {
                    try
                    {
                        entry.completionPromise->set_exception(std::current_exception());
                    }
                    catch (const std::future_error&)
                    { /* already set */
                    }
                }
            }
            catch (...)
            {
                this->logCommandEvent("Unknown exception while draining the queue", Ogre::LML_CRITICAL);
                if (entry.completionPromise)
                {
                    try
                    {
                        entry.completionPromise->set_exception(std::current_exception());
                    }
                    catch (const std::future_error&)
                    { /* already set */
                    }
                }
            }
        }

        this->logCommandEvent("Drained " + Ogre::StringConverter::toString(drainedCommands) + " commands on the calling thread", Ogre::LML_CRITICAL);
    }

    void GraphicsModule::markCurrentThreadAsRenderThread()
    {
        this->renderThreadId.store(std::this_thread::get_id());
    }

    bool GraphicsModule::isRenderThread() const
    {
        return std::this_thread::get_id() == this->renderThreadId.load();
    }

    void GraphicsModule::incrementWaitDepth(void)
    {
        g_waitDepth++;
        this->logCommandEvent("Incremented wait depth", Ogre::LML_TRIVIAL);
    }

    void GraphicsModule::decrementWaitDepth(void)
    {
        if (g_waitDepth > 0)
        {
            g_waitDepth--;
            this->logCommandEvent("Decremented wait depth", Ogre::LML_TRIVIAL);
        }
        else
        {
            this->logCommandEvent("Attempted to decrement wait depth below 0", Ogre::LML_CRITICAL);
        }
    }

    int GraphicsModule::getWaitDepth(void) const
    {
        return g_waitDepth;
    }

    bool GraphicsModule::isInNestedWait(void) const
    {
        return g_waitDepth > 0;
    }

    void GraphicsModule::enableTimeout(bool enable)
    {
        this->timeoutEnabled = enable;
        this->logCommandEvent(std::string("Timeout ") + (enable ? "enabled" : "disabled"), Ogre::LML_NORMAL);
    }

    bool GraphicsModule::isTimeoutEnabled(void) const
    {
        return this->timeoutEnabled;
    }

    void GraphicsModule::setTimeoutDuration(std::chrono::milliseconds duration)
    {
        this->timeoutDuration = duration.count();
        std::stringstream ss;
        ss << "Timeout duration set to " << duration.count() << "ms";
        this->logCommandEvent(ss.str(), Ogre::LML_NORMAL);
    }

    std::chrono::milliseconds GraphicsModule::getTimeoutDuration(void) const
    {
        return std::chrono::milliseconds(this->timeoutDuration);
    }

    void GraphicsModule::recoverFromTimeout(void)
    {
        this->logCommandEvent("Attempting to recover from command timeout", Ogre::LML_CRITICAL);

        bool shouldProcessCommands = false;

        // Clear all waiting depths that might be preventing command processing
        // This is a last resort recovery mechanism
        {
            std::lock_guard<std::mutex> lock(mutex);

            // Log the number of commands still in the queue
            std::stringstream ss;
            ss << "Queue has " << this->queue.size_approx() << " pending commands during timeout recovery";
            this->logCommandEvent(ss.str(), Ogre::LML_CRITICAL);

            // Reset wait depth if it's non-zero (something might have gone wrong).
            // Attention: g_waitDepth is thread_local, so this only ever resets the depth of
            // the thread that calls recoverFromTimeout - it cannot unstick a different one.
            if (g_waitDepth > 0)
            {
                std::stringstream ss2;
                ss2 << "Resetting wait depth from " << g_waitDepth << " to 0 during recovery";
                this->logCommandEvent(ss2.str(), Ogre::LML_CRITICAL);
                g_waitDepth = 0;
            }

            // Check if we should process commands on the render thread
            shouldProcessCommands = this->isRenderThread();
            if (shouldProcessCommands)
            {
                this->logCommandEvent("Processing command queue during timeout recovery", Ogre::LML_CRITICAL);
            }
        } // Lock released here via RAII

        // Process commands outside the lock to avoid holding mutex while processing
        if (shouldProcessCommands)
        {
            this->processAllCommands();
        }
    }

    bool GraphicsModule::waitForFutureWithTimeout(std::future<void>& future, const std::chrono::milliseconds& timeout, const char* commandName)
    {
        // Attention: this must never wait unbounded, not even with the timeout disabled.
        // The former 'future.wait()' hung the calling thread forever as soon as the render
        // thread stopped servicing the queue, which is exactly what happens during shutdown.
        // With the timeout disabled we still poll, but only give up once the render thread
        // has really ended.
        const std::chrono::milliseconds sliceDuration(1);
        const auto waitStart = std::chrono::steady_clock::now();
        const bool timeoutIsEnabled = this->timeoutEnabled;

        while (true)
        {
            if (std::future_status::ready == future.wait_for(sliceDuration))
            {
                return true;
            }

            if (false == this->renderThreadAlive.load(std::memory_order_acquire))
            {
                std::stringstream ssDead;
                ssDead << "Command '" << commandName << "' cannot complete, the render thread has ended";
                this->logCommandEvent(ssDead.str(), Ogre::LML_CRITICAL);
                return false;
            }

            if (true == timeoutIsEnabled)
            {
                if ((std::chrono::steady_clock::now() - waitStart) >= timeout)
                {
                    std::stringstream ss;
                    ss << "Command '" << commandName << "' timed out after " << timeout.count() << "ms";
                    this->logCommandEvent(ss.str(), Ogre::LML_CRITICAL);

                    // Try to recover from the timeout
                    this->recoverFromTimeout();

                    return false;
                }
            }

            std::this_thread::yield();
        }
    }

    void GraphicsModule::setLoadingIndicator(ILoadingIndicator* indicator)
    {
        // Attention: Ownership stays with the caller. This only stores the pointer, and the caller
        // must keep the object alive until it passes nullptr here again.
        this->loadingIndicator = indicator;
    }

    void GraphicsModule::setLoadingFrameRate(Ogre::Real framesPerSecond)
    {
        if (framesPerSecond < 1.0f)
        {
            framesPerSecond = 1.0f;
        }

        this->loadingFrameIntervalSeconds = 1.0f / framesPerSecond;
    }

    void GraphicsModule::renderLoadingFrameThrottled(void)
    {
        // Attention: RE-ENTRANCY GUARD. renderOneFrame() below can run listeners, MyGUI callbacks
        // and compositor passes, and any of those may end up calling AppStateManager::enqueueAndWait
        // again. That lands in the very command pump loop we are called from, which would call this
        // function a second time. thread_local, so it guards the render thread without any
        // synchronisation.
        static thread_local bool insideLoadingFrame = false;

        if (true == insideLoadingFrame)
        {
            return;
        }

#ifdef NOWA_LOADING_INDICATOR_TIMING
        // Attention: TEMPORARY. Counts why this function returns without rendering. Aggregated and
        // logged once per second, so a silent early-out cannot hide.
        static size_t reasonNoIndicator = 0;
        static size_t reasonNotRenderThread = 0;
        static size_t reasonNoWorkspace = 0;
        static size_t reasonThrottled = 0;
        static size_t framesRendered = 0;
        static auto lastReasonLog = std::chrono::steady_clock::now();

        const auto reasonNow = std::chrono::steady_clock::now();
        if (reasonNow - lastReasonLog >= std::chrono::seconds(1))
        {
            lastReasonLog = reasonNow;
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[LOADING-FRAME] rendered=" + Ogre::StringConverter::toString(framesRendered) + " noIndicator=" + Ogre::StringConverter::toString(reasonNoIndicator) +
                                                                                    " notRenderThread=" + Ogre::StringConverter::toString(reasonNotRenderThread) + " noWorkspace=" + Ogre::StringConverter::toString(reasonNoWorkspace) +
                                                                                    " throttled=" + Ogre::StringConverter::toString(reasonThrottled));

            reasonNoIndicator = 0;
            reasonNotRenderThread = 0;
            reasonNoWorkspace = 0;
            reasonThrottled = 0;
            framesRendered = 0;
        }
#endif

        // Attention: This is the ONLY place that renders anything while a scene is loading.
        // renderThreadFunction's own loop is not running then - the render thread sits in the
        // command pump loop of AppStateManager::enqueueAndWait, which is where this is called
        // from. Putting a renderOneFrame() into renderThreadFunction has no effect during a load.
        if (nullptr == this->loadingIndicator)
        {
#ifdef NOWA_LOADING_INDICATOR_TIMING
            ++reasonNoIndicator;
#endif
            return;
        }

        if (false == this->isRenderThread())
        {
#ifdef NOWA_LOADING_INDICATOR_TIMING
            ++reasonNotRenderThread;
#endif
            return;
        }

        // Attention: A workspace is required. ProjectManager::loadProject destroys the scene before
        // parsing, so unless a dummy workspace was created for the load there is nothing to render
        // into and renderOneFrame would draw nothing (or worse). Bail out quietly - logging here
        // would spam, because this runs many times per second.
        if (false == AppStateManager::getSingletonPtr()->getWorkspaceModule()->hasAnyWorkspace())
        {
#ifdef NOWA_LOADING_INDICATOR_TIMING
            ++reasonNoWorkspace;
#endif
            return;
        }

        const auto now = std::chrono::steady_clock::now();
        const Ogre::Real secondsSinceLastFrame = std::chrono::duration<Ogre::Real>(now - this->lastLoadingFrameTime).count();

        if (secondsSinceLastFrame < this->loadingFrameIntervalSeconds)
        {
#ifdef NOWA_LOADING_INDICATOR_TIMING
            ++reasonThrottled;
#endif
            return;
        }

        this->lastLoadingFrameTime = now;

        insideLoadingFrame = true;

        try
        {
            this->loadingIndicator->onUpdate(secondsSinceLastFrame);
            Ogre::Root::getSingletonPtr()->renderOneFrame();

#ifdef NOWA_LOADING_INDICATOR_TIMING
            ++framesRendered;
#endif
        }
        catch (const Ogre::Exception& e)
        {
            // Attention: a failure here must never abort the scene load. Drop the indicator so we
            // do not retry every frame and flood the log.
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GraphicsModule] Loading frame failed, disabling the loading indicator: " + e.getDescription());
            this->loadingIndicator = nullptr;
        }
        catch (...)
        {
            // Attention: MyGUI throws its own exception type, which the handler above would not
            // catch - it would have propagated straight through the command pump loop.
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GraphicsModule] Loading frame threw an unknown exception, disabling the loading indicator.");
            this->loadingIndicator = nullptr;
        }

        insideLoadingFrame = false;
    }

    void GraphicsModule::waitForCommandOrSignal(std::chrono::milliseconds timeout)
    {
        // Parks the calling thread until a new render command arrives (enqueue() signals), until
        // someone calls signalCommandWaiters() explicitly, or until the timeout expires.
        //
        // Attention: This exists because std::this_thread::sleep_for(1ms) does NOT sleep for 1 ms
        // on Windows. The default scheduler granularity is ~15.6 ms, so a polling loop built on
        // sleep_for wakes only once per timer tick. Any thread waiting for that loop to service
        // the command queue therefore pays ~15.6 ms PER round trip - which is exactly what made
        // scene loading take 13 seconds for 133 objects.
        //
        // Attention: renderThreadParked is published BEFORE the lock is taken, so the window in
        // which a signaller could miss us is as small as possible. Should it still happen, the
        // commandPending predicate makes this return immediately, and the timeout is the hard
        // upper bound.
        this->renderThreadParked.store(true, std::memory_order_release);

        {
            std::unique_lock<std::mutex> commandWaitLock(this->commandWaitMutex);
            this->commandWaitCondition.wait_for(commandWaitLock, timeout,
                [this]()
                {
                    return this->commandPending.load(std::memory_order_acquire);
                });
            this->commandPending.store(false, std::memory_order_release);
        }

        this->renderThreadParked.store(false, std::memory_order_release);
    }

    void GraphicsModule::signalCommandWaiters(void)
    {
        // COST IN THE NORMAL PATH: no mutex is taken here, ever. When nobody is parked this is one
        // release store plus one acquire load, i.e. a couple of nanoseconds and no kernel call.
        // notify_one() - the only part that reaches the OS - runs ONLY while someone is parked.
        this->commandPending.store(true, std::memory_order_release);

        if (true == this->renderThreadParked.load(std::memory_order_acquire))
        {
            this->commandWaitCondition.notify_one();
        }
    }

    void GraphicsModule::suspendRendering(bool suspend)
    {
        // Attention: This only stops renderOneFrame() and the transform interpolation. The
        // command queue keeps being serviced, which is the whole point: while suspended, an
        // enqueueAndWait round trip costs microseconds instead of a full frame.
        //
        // Attention: NOT reentrant. Use ScopedRenderSuspend for nesting-safe usage, or make sure
        // suspend/resume are strictly paired.
        const bool wasSuspended = this->renderingSuspended.exchange(suspend, std::memory_order_release);

        if (wasSuspended != suspend)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, Ogre::String("[GraphicsModule] Rendering ") + (suspend ? "SUSPENDED" : "RESUMED"));
        }
    }

    bool GraphicsModule::isRenderingSuspended(void) const
    {
        return this->renderingSuspended.load(std::memory_order_acquire);
    }

    void GraphicsModule::requestStall()
    {
        // Signal the render thread to pause at the next safe boundary (top of loop)
        stallRequested.store(true, std::memory_order_release);

        // Wait until render thread confirms it has exited any vector-touching code
        while (!stallAcknowledged.load(std::memory_order_acquire))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        // Now safe: render thread is parked, won't touch tracked vectors until releaseStall()
    }

    void GraphicsModule::releaseStall()
    {
        stallRequested.store(false, std::memory_order_release);
        // Render thread will see this on its next spin, clear m_stallAcknowledged, and resume
    }

    bool GraphicsModule::isSceneValid(void)
    {
        GameProgressModule* gameProgressModule = AppStateManager::getSingletonPtr()->getActiveGameProgressModuleSafe();
        const bool isStalled = AppStateManager::getSingletonPtr()->bStall.load();
        const bool isSceneLoading = (gameProgressModule != nullptr) ? gameProgressModule->bSceneLoading.load() : true;

        if (false == isStalled && false == isSceneLoading)
        {
            return true;
        }
        return false;
    }

    GraphicsModule::NodeTransforms* GraphicsModule::resolveNodeSlotLocked(Ogre::Node* node)
    {
        std::lock_guard<std::mutex> lock(this->nodeRegistrationMutex);

        auto it = this->nodeToIndexMap.find(node);
        if (it != this->nodeToIndexMap.end())
        {
            return &this->nodePool[it->second];
        }

        if (true == this->freeNodeSlots.empty())
        {
            // Pool exhausted - every one of the NODE_POOL_CAPACITY slots is
            // currently bound to a live node. This should never happen in normal
            // play; if it does, hand back the shared overflow sink instead of a
            // null pointer so no caller crashes - the node simply will not
            // interpolate until something frees up a real slot. Raise
            // NODE_POOL_CAPACITY if you see this in the log.
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                "[GraphicsModule] Node pool exhausted (capacity " + Ogre::StringConverter::toString(GraphicsModule::NODE_POOL_CAPACITY) + "). Raise NODE_POOL_CAPACITY. Node '" + node->getName() + "' will not interpolate.");
            return &this->nodeOverflowSink;
        }

        size_t index = this->freeNodeSlots.back();
        this->freeNodeSlots.pop_back();

        NodeTransforms& slot = this->nodePool[index];

        // Initialize all buffers with the current node transform - same baseline
        // snapshot behaviour as the original addTrackedNode().
        GraphicsModule::TransformData baseline;
        baseline.position = node->getPosition();
        baseline.orientation = node->getOrientation();
        baseline.scale = node->getScale();
        for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
        {
            slot.transforms[i] = baseline;
        }

        slot.isNew = true;
        slot.useDerived.store(false, std::memory_order_relaxed);
        slot.active.store(true, std::memory_order_relaxed);
        // Publish the identity LAST and with release ordering: any thread that
        // later sees this node pointer via the index map or a recycled slot
        // check is guaranteed to also see the baseline data written above.
        slot.node.store(node, std::memory_order_release);

        this->nodeToIndexMap[node] = index;

        if (true == this->debugVisualization)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[RenderCommandQueueModule]: Added tracked node: " + node->getName());
        }

        return &slot;
    }

    GraphicsModule::NodeTransforms* GraphicsModule::acquireNodeSlot(Ogre::Node* node)
    {
        // Lock-free fast path. Each thread that has ever touched 'node' keeps its
        // own cached slot pointer here - no shared state, no lock, no contention
        // with any other thread's cache.
        thread_local std::unordered_map<Ogre::Node*, NodeTransforms*> tlsCache;

        auto cacheIt = tlsCache.find(node);
        if (cacheIt != tlsCache.end())
        {
            NodeTransforms* slot = cacheIt->second;
            if (slot->node.load(std::memory_order_acquire) == node)
            {
                return slot;
            }
            // Stale: this slot has been tombstoned/recycled since we cached it.
            // Drop the entry and fall through to re-resolve, once, below.
            tlsCache.erase(cacheIt);
        }

        NodeTransforms* slot = this->resolveNodeSlotLocked(node);
        tlsCache[node] = slot;
        return slot;
    }

    GraphicsModule::CameraTransforms* GraphicsModule::resolveCameraSlotLocked(Ogre::Camera* camera)
    {
        std::lock_guard<std::mutex> lock(this->cameraRegistrationMutex);

        auto it = this->cameraToIndexMap.find(camera);
        if (it != this->cameraToIndexMap.end())
        {
            return &this->cameraPool[it->second];
        }

        size_t index;
        if (true == this->freeCameraSlots.empty())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GraphicsModule] Camera pool exhausted (capacity " + Ogre::StringConverter::toString(GraphicsModule::CAMERA_POOL_CAPACITY) + "). Raise CAMERA_POOL_CAPACITY.");
            return &this->cameraOverflowSink;
        }
        index = this->freeCameraSlots.back();
        this->freeCameraSlots.pop_back();

        CameraTransforms& slot = this->cameraPool[index];

        GraphicsModule::CameraTransformData baseline;
        baseline.position = camera->getPosition();
        baseline.orientation = camera->getOrientation();
        for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
        {
            slot.transforms[i] = baseline;
        }

        slot.isNew = true;
        slot.active.store(true, std::memory_order_relaxed);
        slot.camera.store(camera, std::memory_order_release);

        this->cameraToIndexMap[camera] = index;

        return &slot;
    }

    GraphicsModule::CameraTransforms* GraphicsModule::acquireCameraSlot(Ogre::Camera* camera)
    {
        thread_local std::unordered_map<Ogre::Camera*, CameraTransforms*> tlsCache;

        auto cacheIt = tlsCache.find(camera);
        if (cacheIt != tlsCache.end())
        {
            CameraTransforms* slot = cacheIt->second;
            if (slot->camera.load(std::memory_order_acquire) == camera)
            {
                return slot;
            }
            tlsCache.erase(cacheIt);
        }

        CameraTransforms* slot = this->resolveCameraSlotLocked(camera);
        tlsCache[camera] = slot;
        return slot;
    }

    GraphicsModule::BoneTransforms* GraphicsModule::resolveBoneSlotLocked(Ogre::Bone* bone)
    {
        std::lock_guard<std::mutex> lock(this->boneRegistrationMutex);

        auto it = this->boneToIndexMap.find(bone);
        if (it != this->boneToIndexMap.end())
        {
            return &this->bonePool[it->second];
        }

        size_t index;
        if (true == this->freeBoneSlots.empty())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GraphicsModule] Bone pool exhausted (capacity " + Ogre::StringConverter::toString(GraphicsModule::BONE_POOL_CAPACITY) + "). Raise BONE_POOL_CAPACITY.");
            return &this->boneOverflowSink;
        }
        index = this->freeBoneSlots.back();
        this->freeBoneSlots.pop_back();

        BoneTransforms& slot = this->bonePool[index];

        GraphicsModule::TransformData baseline;
        baseline.position = bone->getPosition();
        baseline.orientation = bone->getOrientation();
        for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
        {
            slot.transforms[i] = baseline;
        }

        slot.isNew = true;
        slot.active.store(true, std::memory_order_relaxed);
        slot.bone.store(bone, std::memory_order_release);

        this->boneToIndexMap[bone] = index;

        return &slot;
    }

    GraphicsModule::BoneTransforms* GraphicsModule::acquireBoneSlot(Ogre::Bone* bone)
    {
        thread_local std::unordered_map<Ogre::Bone*, BoneTransforms*> tlsCache;

        auto cacheIt = tlsCache.find(bone);
        if (cacheIt != tlsCache.end())
        {
            BoneTransforms* slot = cacheIt->second;
            if (slot->bone.load(std::memory_order_acquire) == bone)
            {
                return slot;
            }
            tlsCache.erase(cacheIt);
        }

        BoneTransforms* slot = this->resolveBoneSlotLocked(bone);
        tlsCache[bone] = slot;
        return slot;
    }

    GraphicsModule::TrackedDatablock* GraphicsModule::resolveDatablockSlotLocked(Ogre::HlmsDatablock* datablock, const Ogre::ColourValue& initialValue, std::function<void(Ogre::ColourValue)> applyFunc,
        std::function<Ogre::ColourValue(const Ogre::ColourValue&, const Ogre::ColourValue&, Ogre::Real)> interpFunc)
    {
        std::lock_guard<std::mutex> lock(this->datablockRegistrationMutex);

        auto it = this->datablockToIndexMap.find(datablock);
        if (it != this->datablockToIndexMap.end())
        {
            return &this->datablockPool[it->second];
        }

        size_t index;
        if (true == this->freeDatablockSlots.empty())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GraphicsModule] Datablock pool exhausted (capacity " + Ogre::StringConverter::toString(GraphicsModule::DATABLOCK_POOL_CAPACITY) + "). Raise DATABLOCK_POOL_CAPACITY.");
            return &this->datablockOverflowSink;
        }
        index = this->freeDatablockSlots.back();
        this->freeDatablockSlots.pop_back();

        TrackedDatablock& slot = this->datablockPool[index];

        for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
        {
            slot.values[i] = initialValue;
        }

        slot.applyFunc = std::move(applyFunc);
        slot.interpolateFunc = std::move(interpFunc);
        slot.isNew = true;
        slot.active.store(true, std::memory_order_relaxed);
        slot.datablock.store(datablock, std::memory_order_release);

        this->datablockToIndexMap[datablock] = index;

        return &slot;
    }

    GraphicsModule::TrackedDatablock* GraphicsModule::acquireDatablockSlot(Ogre::HlmsDatablock* datablock, const Ogre::ColourValue& initialValue, std::function<void(Ogre::ColourValue)> applyFunc,
        std::function<Ogre::ColourValue(const Ogre::ColourValue&, const Ogre::ColourValue&, Ogre::Real)> interpFunc)
    {
        thread_local std::unordered_map<Ogre::HlmsDatablock*, TrackedDatablock*> tlsCache;

        auto cacheIt = tlsCache.find(datablock);
        if (cacheIt != tlsCache.end())
        {
            TrackedDatablock* slot = cacheIt->second;
            if (slot->datablock.load(std::memory_order_acquire) == datablock)
            {
                return slot;
            }
            tlsCache.erase(cacheIt);
        }

        TrackedDatablock* slot = this->resolveDatablockSlotLocked(datablock, initialValue, std::move(applyFunc), std::move(interpFunc));
        tlsCache[datablock] = slot;
        return slot;
    }

    void GraphicsModule::addTrackedNode(Ogre::Node* node)
    {
        // Public, explicit registration. Same one-time-resolution path as the
        // lazy "first update call" path below - acquireNodeSlot() is idempotent,
        // so calling this ahead of time just pre-warms the calling thread's cache.
        this->acquireNodeSlot(node);
    }

    void GraphicsModule::removeTrackedNode(Ogre::Node* node)
    {
        std::lock_guard<std::mutex> lock(this->nodeRegistrationMutex);

        auto it = this->nodeToIndexMap.find(node);
        if (it == this->nodeToIndexMap.end())
        {
            return;
        }

        size_t index = it->second;
        NodeTransforms& slot = this->nodePool[index];

        if (true == this->debugVisualization)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[RenderCommandQueueModule]: Removed tracked node: " + node->getName());
        }

        // Tombstone in place - never erase from the pool. Any thread whose
        // thread_local cache still points at this exact slot will see its
        // identity no longer matches (node load() != its cached node pointer)
        // the next time it tries to use it, and will transparently re-resolve.
        // NOTE: this assumes the caller has already stopped any other thread
        // from calling update*() for this exact node before removing it -
        // the same lifecycle convention the original code relied on (e.g. a
        // component disconnects/stops driving a node before telling
        // GraphicsModule to forget it).
        slot.node.store(nullptr, std::memory_order_release);
        slot.active.store(false, std::memory_order_relaxed);

        this->nodeToIndexMap.erase(it);
        this->freeNodeSlots.push_back(index);
    }

    void GraphicsModule::updateNodePosition(Ogre::Node* node, const Ogre::Vector3& position, bool useDerived)
    {
        // Lock-free after the first call: acquireNodeSlot() only takes nodeMutex
        // the first time THIS thread sees 'node' (or after its cached slot was
        // recycled). Every call after that is a direct pointer dereference.
        //
        GraphicsModule::NodeTransforms* nodeTransforms = this->acquireNodeSlot(node);

        nodeTransforms->transforms[this->currentTransformNodeIdx].position = position;
        nodeTransforms->active.store(true, std::memory_order_relaxed);
        nodeTransforms->useDerived.store(useDerived, std::memory_order_relaxed);

        if (true == this->debugVisualization)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                "[RenderCommandQueueModule]: Updated position for node: " + node->getName() + " to " + Ogre::StringConverter::toString(position) + " in buffer: " + Ogre::StringConverter::toString(this->currentTransformNodeIdx));
        }
    }
    // Transform warp: this becomes the truth for this node right now,
    void GraphicsModule::updateNodeOrientation(Ogre::Node* node, const Ogre::Quaternion& orientation, bool useDerived)
    // from it. Any later call - fireAndForget or not, from any thread -
    // is free to overwrite it again; this is a one-shot snapshot, not a
    // lock on the node.
    {
        // Always interpolated - call setNodeOrientation() instead for an instant warp.
        GraphicsModule::NodeTransforms* nodeTransforms = this->acquireNodeSlot(node);

        nodeTransforms->transforms[this->currentTransformNodeIdx].orientation = orientation;
        nodeTransforms->active.store(true, std::memory_order_relaxed);
        nodeTransforms->useDerived.store(useDerived, std::memory_order_relaxed);

        if (true == this->debugVisualization)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                "[RenderCommandQueueModule]: Updated orientation for node: " + node->getName() + " to " + Ogre::StringConverter::toString(orientation) + " in buffer: " + Ogre::StringConverter::toString(this->currentTransformNodeIdx));
        }
    }

    void GraphicsModule::updateNodeScale(Ogre::Node* node, const Ogre::Vector3& scale, bool useDerived)
    {
        // Always interpolated - call setNodeScale() instead for an instant warp.
        GraphicsModule::NodeTransforms* nodeTransforms = this->acquireNodeSlot(node);

        nodeTransforms->transforms[this->currentTransformNodeIdx].scale = scale;
        nodeTransforms->active.store(true, std::memory_order_relaxed);
        nodeTransforms->useDerived.store(useDerived, std::memory_order_relaxed);

        if (true == this->debugVisualization)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                "[RenderCommandQueueModule]: Updated scale for node: " + node->getName() + " to " + Ogre::StringConverter::toString(scale) + " in buffer: " + Ogre::StringConverter::toString(this->currentTransformNodeIdx));
        }
    }

    void GraphicsModule::updateNodeTransform(Ogre::Node* node, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, const Ogre::Vector3& scale, bool useDerived)
    {
        GraphicsModule::NodeTransforms* nodeTransforms = this->acquireNodeSlot(node);

        nodeTransforms->transforms[this->currentTransformNodeIdx].position = position;
        nodeTransforms->transforms[this->currentTransformNodeIdx].orientation = orientation;
        nodeTransforms->transforms[this->currentTransformNodeIdx].scale = scale;
        nodeTransforms->active.store(true, std::memory_order_relaxed);
        nodeTransforms->useDerived.store(useDerived, std::memory_order_relaxed);
    }

    // =========================================================================
    // setNode*() - instant warp. See the contract comment on setNodePosition()
    // in the header for the full rationale. Each public function here just
    // decides which thread should do the actual work, then delegates to the
    // matching *OnRenderThread() implementation, which is the only place that
    // both touches the real Ogre::Node AND re-pins the interpolation buffer.
    // =========================================================================

    void GraphicsModule::setNodePosition(Ogre::Node* node, const Ogre::Vector3& position, bool useDerived)
    {
        if (true == this->isRenderThread())
        {
            this->setNodePositionOnRenderThread(node, position, useDerived);
        }
        else
        {
            NOWA::GraphicsModule::RenderCommand command = [this, node, position, useDerived]()
            {
                this->setNodePositionOnRenderThread(node, position, useDerived);
            };
            this->enqueueAndWait(std::move(command), "GraphicsModule::setNodePosition");
        }
    }

    void GraphicsModule::setNodePositionOnRenderThread(Ogre::Node* node, const Ogre::Vector3& position, bool useDerived)
    {
        // 1. Apply directly to the real Ogre node RIGHT NOW. This is what makes
        //    this call "kill" any interpolation already in flight: the next
        //    renderOneFrame() in this same iteration renders THIS value, not
        //    whatever updateAllTransforms() would otherwise have blended to.
        if (false == useDerived)
        {
            node->setPosition(position);
        }
        else
        {
            node->_setDerivedPosition(position);
        }

        // 2. Re-pin the interpolation buffer to match, so that if this node is
        //    still tracked, any later updateAllTransforms() pass (this frame or
        //    a future one, as long as nothing else writes to it first) keeps
        //    reaffirming this exact value instead of drifting back toward
        //    whatever was buffered before.
        /*GraphicsModule::NodeTransforms* nodeTransforms = this->acquireNodeSlot(node);
        for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
        {
            nodeTransforms->transforms[i].position = position;
        }

        nodeTransforms->active.store(false, std::memory_order_relaxed);
        nodeTransforms->useDerived.store(useDerived, std::memory_order_relaxed);*/
    }

    void GraphicsModule::setNodeOrientation(Ogre::Node* node, const Ogre::Quaternion& orientation, bool useDerived)
    {
        if (true == this->isRenderThread())
        {
            this->setNodeOrientationOnRenderThread(node, orientation, useDerived);
        }
        else
        {
            NOWA::GraphicsModule::RenderCommand command = [this, node, orientation, useDerived]()
            {
                this->setNodeOrientationOnRenderThread(node, orientation, useDerived);
            };
            this->enqueueAndWait(std::move(command), "GraphicsModule::setNodeOrientation");
        }
    }

    void GraphicsModule::setNodeOrientationOnRenderThread(Ogre::Node* node, const Ogre::Quaternion& orientation, bool useDerived)
    {
        if (false == useDerived)
        {
            node->setOrientation(orientation);
        }
        else
        {
            node->_setDerivedOrientation(orientation);
        }

        /*GraphicsModule::NodeTransforms* nodeTransforms = this->acquireNodeSlot(node);
        for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
        {
            nodeTransforms->transforms[i].orientation = orientation;
        }
        nodeTransforms->active.store(false, std::memory_order_relaxed);
        nodeTransforms->useDerived.store(useDerived, std::memory_order_relaxed);*/
    }

    void GraphicsModule::setNodeScale(Ogre::Node* node, const Ogre::Vector3& scale, bool useDerived)
    {
        if (true == this->isRenderThread())
        {
            this->setNodeScaleOnRenderThread(node, scale, useDerived);
        }
        else
        {
            NOWA::GraphicsModule::RenderCommand command = [this, node, scale, useDerived]()
            {
                this->setNodeScaleOnRenderThread(node, scale, useDerived);
            };
            this->enqueueAndWait(std::move(command), "GraphicsModule::setNodeScale");
        }
    }

    void GraphicsModule::setNodeScaleOnRenderThread(Ogre::Node* node, const Ogre::Vector3& scale, bool useDerived)
    {
        // Note: Ogre::Node has no _setDerivedScale() counterpart - scale is only
        // ever set directly, exactly like the original fireAndForget code path
        // did (it commented "Scale only on non-derived path" for the same reason).
        node->setScale(scale);

        /*GraphicsModule::NodeTransforms* nodeTransforms = this->acquireNodeSlot(node);
        for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
        {
            nodeTransforms->transforms[i].scale = scale;
        }

        nodeTransforms->active.store(false, std::memory_order_relaxed);
        nodeTransforms->useDerived.store(useDerived, std::memory_order_relaxed);*/
    }

    void GraphicsModule::setNodeTransform(Ogre::Node* node, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, const Ogre::Vector3& scale, bool useDerived)
    {
        if (true == this->isRenderThread())
        {
            this->setNodeTransformOnRenderThread(node, position, orientation, scale, useDerived);
        }
        else
        {
            NOWA::GraphicsModule::RenderCommand command = [this, node, position, orientation, scale, useDerived]()
            {
                this->setNodeTransformOnRenderThread(node, position, orientation, scale, useDerived);
            };
            this->enqueueAndWait(std::move(command), "GraphicsModule::setNodeTransform");
        }
    }

    void GraphicsModule::teleportNodePosition(Ogre::Node* node, const Ogre::Vector3& position, bool useDerived)
    {
        GraphicsModule::NodeTransforms* nodeTransforms = this->acquireNodeSlot(node);

        // Writes all values to all buffers and permits interpolation so its a real teleport
        for (size_t b = 0; b < NUM_TRANSFORM_BUFFERS; ++b)
        {
            nodeTransforms->transforms[b].position = position;
        }
        nodeTransforms->active.store(true, std::memory_order_relaxed);
        nodeTransforms->useDerived.store(useDerived, std::memory_order_relaxed);
    }

    void GraphicsModule::teleportNodeOrientation(Ogre::Node* node, const Ogre::Quaternion& orientation)
    {
        GraphicsModule::NodeTransforms* nodeTransforms = this->acquireNodeSlot(node);

        // Writes all values to all buffers and permits interpolation so its a real teleport
        for (size_t b = 0; b < NUM_TRANSFORM_BUFFERS; ++b)
        {
            nodeTransforms->transforms[b].orientation = orientation;
        }
        nodeTransforms->active.store(true, std::memory_order_relaxed);
    }

    void GraphicsModule::setNodeTransformOnRenderThread(Ogre::Node* node, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, const Ogre::Vector3& scale, bool useDerived)
    {
        if (false == useDerived)
        {
            node->setPosition(position);
            node->setOrientation(orientation);
            node->setScale(scale);
        }
        else
        {
            node->_setDerivedPosition(position);
            node->_setDerivedOrientation(orientation);
            // Comment says: "Scale only on non-derived path" - same as the
            // original fireAndForget code and the interpolated path above.
        }

        /*GraphicsModule::NodeTransforms* nodeTransforms = this->acquireNodeSlot(node);
        for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
        {
            nodeTransforms->transforms[i].position = position;
            nodeTransforms->transforms[i].orientation = orientation;
            nodeTransforms->transforms[i].scale = scale;
        }

        nodeTransforms->active.store(false, std::memory_order_relaxed);
        nodeTransforms->useDerived.store(useDerived, std::memory_order_relaxed);*/
    }

    void GraphicsModule::addTrackedCamera(Ogre::Camera* camera)
    {
        this->acquireCameraSlot(camera);
    }

    void GraphicsModule::removeTrackedCamera(Ogre::Camera* camera)
    {
        std::lock_guard<std::mutex> lock(this->cameraRegistrationMutex);

        auto it = this->cameraToIndexMap.find(camera);
        if (it == this->cameraToIndexMap.end())
        {
            return;
        }

        size_t index = it->second;
        CameraTransforms& slot = this->cameraPool[index];

        slot.camera.store(nullptr, std::memory_order_release);
        slot.active.store(false, std::memory_order_relaxed);

        this->cameraToIndexMap.erase(it);
        this->freeCameraSlots.push_back(index);
    }

    void GraphicsModule::updateCameraPosition(Ogre::Camera* camera, const Ogre::Vector3& position)
    {
        // Always interpolated - call setCameraPosition() instead for an instant warp.
        GraphicsModule::CameraTransforms* cameraTransforms = this->acquireCameraSlot(camera);

        cameraTransforms->transforms[this->currentTransformCameraIdx].position = position;
        cameraTransforms->active.store(true, std::memory_order_relaxed);
    }

    void GraphicsModule::updateCameraOrientation(Ogre::Camera* camera, const Ogre::Quaternion& orientation)
    {
        GraphicsModule::CameraTransforms* cameraTransforms = this->acquireCameraSlot(camera);

        cameraTransforms->transforms[this->currentTransformCameraIdx].orientation = orientation;
        cameraTransforms->active.store(true, std::memory_order_relaxed);
    }

    void GraphicsModule::updateCameraTransform(Ogre::Camera* camera, const Ogre::Vector3& position, const Ogre::Quaternion& orientation)
    {
        // Always interpolated - call setCameraTransform() instead for an instant warp.
        GraphicsModule::CameraTransforms* cameraTransforms = this->acquireCameraSlot(camera);

        cameraTransforms->transforms[this->currentTransformCameraIdx].position = position;
        cameraTransforms->transforms[this->currentTransformCameraIdx].orientation = orientation;
        cameraTransforms->active.store(true, std::memory_order_relaxed);
    }

    // Instant warp - see the contract comment on setNodePosition() in the header.
    void GraphicsModule::setCameraPosition(Ogre::Camera* camera, const Ogre::Vector3& position)
    {
        if (true == this->isRenderThread())
        {
            this->setCameraPositionOnRenderThread(camera, position);
        }
        else
        {
            NOWA::GraphicsModule::RenderCommand command = [this, camera, position]()
            {
                this->setCameraPositionOnRenderThread(camera, position);
            };
            this->enqueueAndWait(std::move(command), "GraphicsModule::setCameraPosition");
        }
    }

    void GraphicsModule::setCameraPositionOnRenderThread(Ogre::Camera* camera, const Ogre::Vector3& position)
    {
        camera->setPosition(position);

        /*GraphicsModule::CameraTransforms* cameraTransforms = this->acquireCameraSlot(camera);
        for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
        {
            cameraTransforms->transforms[i].position = position;
        }

        cameraTransforms->active.store(false, std::memory_order_relaxed);*/
    }

    void GraphicsModule::setCameraOrientation(Ogre::Camera* camera, const Ogre::Quaternion& orientation)
    {
        if (true == this->isRenderThread())
        {
            this->setCameraOrientationOnRenderThread(camera, orientation);
        }
        else
        {
            NOWA::GraphicsModule::RenderCommand command = [this, camera, orientation]()
            {
                this->setCameraOrientationOnRenderThread(camera, orientation);
            };
            this->enqueueAndWait(std::move(command), "GraphicsModule::setCameraOrientation");
        }
    }

    void GraphicsModule::setCameraOrientationOnRenderThread(Ogre::Camera* camera, const Ogre::Quaternion& orientation)
    {
        camera->setOrientation(orientation);

        /*GraphicsModule::CameraTransforms* cameraTransforms = this->acquireCameraSlot(camera);
        for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
        {
            cameraTransforms->transforms[i].orientation = orientation;
        }

        cameraTransforms->active.store(false, std::memory_order_relaxed);*/
    }

    void GraphicsModule::setCameraTransform(Ogre::Camera* camera, const Ogre::Vector3& position, const Ogre::Quaternion& orientation)
    {
        if (true == this->isRenderThread())
        {
            this->setCameraTransformOnRenderThread(camera, position, orientation);
        }
        else
        {
            NOWA::GraphicsModule::RenderCommand command = [this, camera, position, orientation]()
            {
                this->setCameraTransformOnRenderThread(camera, position, orientation);
            };
            this->enqueueAndWait(std::move(command), "GraphicsModule::setCameraTransform");
        }
    }

    void GraphicsModule::setCameraTransformOnRenderThread(Ogre::Camera* camera, const Ogre::Vector3& position, const Ogre::Quaternion& orientation)
    {
        camera->setPosition(position);
        camera->setOrientation(orientation);

        /*GraphicsModule::CameraTransforms* cameraTransforms = this->acquireCameraSlot(camera);
        for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
        {
            cameraTransforms->transforms[i].position = position;
            cameraTransforms->transforms[i].orientation = orientation;
        }

        cameraTransforms->active.store(false, std::memory_order_relaxed);*/
    }

    void GraphicsModule::addTrackedBone(Ogre::Bone* bone)
    {
        this->acquireBoneSlot(bone);
    }

    void GraphicsModule::removeTrackedBone(Ogre::Bone* bone)
    {
        std::lock_guard<std::mutex> lock(this->boneRegistrationMutex);

        auto it = this->boneToIndexMap.find(bone);
        if (it == this->boneToIndexMap.end())
        {
            return;
        }

        size_t index = it->second;
        BoneTransforms& slot = this->bonePool[index];

        slot.bone.store(nullptr, std::memory_order_release);
        slot.active.store(false, std::memory_order_relaxed);

        this->boneToIndexMap.erase(it);
        this->freeBoneSlots.push_back(index);
    }

    void GraphicsModule::updateBonePosition(Ogre::Bone* bone, const Ogre::Vector3& position)
    {
        GraphicsModule::BoneTransforms* boneTransforms = this->acquireBoneSlot(bone);

        boneTransforms->transforms[this->currentTransformBoneIdx].position = position;
        boneTransforms->active.store(true, std::memory_order_relaxed);
    }

    void GraphicsModule::updateBoneOrientation(Ogre::Bone* bone, const Ogre::Quaternion& orientation)
    {
        // Always interpolated - call setBoneOrientation() instead for an instant warp.
        GraphicsModule::BoneTransforms* boneTransforms = this->acquireBoneSlot(bone);

        boneTransforms->transforms[this->currentTransformBoneIdx].orientation = orientation;
        boneTransforms->active.store(true, std::memory_order_relaxed);
    }

    void GraphicsModule::updateBoneTransform(Ogre::Bone* bone, const Ogre::Vector3& position, const Ogre::Quaternion& orientation)
    {
        // Always interpolated - call setBoneTransform() instead for an instant warp.
        GraphicsModule::BoneTransforms* boneTransforms = this->acquireBoneSlot(bone);

        boneTransforms->transforms[this->currentTransformBoneIdx].position = position;
        boneTransforms->transforms[this->currentTransformBoneIdx].orientation = orientation;
        boneTransforms->active.store(true, std::memory_order_relaxed);
    }

    // Instant warp - see the contract comment on setNodePosition() in the header.
    void GraphicsModule::setBonePosition(Ogre::Bone* bone, const Ogre::Vector3& position)
    {
        if (true == this->isRenderThread())
        {
            this->setBonePositionOnRenderThread(bone, position);
        }
        else
        {
            NOWA::GraphicsModule::RenderCommand command = [this, bone, position]()
            {
                this->setBonePositionOnRenderThread(bone, position);
            };
            this->enqueueAndWait(std::move(command), "GraphicsModule::setBonePosition");
        }
    }

    void GraphicsModule::setBonePositionOnRenderThread(Ogre::Bone* bone, const Ogre::Vector3& position)
    {
        bone->setPosition(position);

        /*GraphicsModule::BoneTransforms* boneTransforms = this->acquireBoneSlot(bone);
        for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
        {
            boneTransforms->transforms[i].position = position;
        }

        boneTransforms->active.store(false, std::memory_order_relaxed);*/
    }

    void GraphicsModule::setBoneOrientation(Ogre::Bone* bone, const Ogre::Quaternion& orientation)
    {
        if (true == this->isRenderThread())
        {
            this->setBoneOrientationOnRenderThread(bone, orientation);
        }
        else
        {
            NOWA::GraphicsModule::RenderCommand command = [this, bone, orientation]()
            {
                this->setBoneOrientationOnRenderThread(bone, orientation);
            };
            this->enqueueAndWait(std::move(command), "GraphicsModule::setBoneOrientation");
        }
    }

    void GraphicsModule::setBoneOrientationOnRenderThread(Ogre::Bone* bone, const Ogre::Quaternion& orientation)
    {
        bone->setOrientation(orientation);

        /*GraphicsModule::BoneTransforms* boneTransforms = this->acquireBoneSlot(bone);
        for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
        {
            boneTransforms->transforms[i].orientation = orientation;
        }

        boneTransforms->active.store(false, std::memory_order_relaxed);*/
    }

    void GraphicsModule::setBoneTransform(Ogre::Bone* bone, const Ogre::Vector3& position, const Ogre::Quaternion& orientation)
    {
        if (true == this->isRenderThread())
        {
            this->setBoneTransformOnRenderThread(bone, position, orientation);
        }
        else
        {
            NOWA::GraphicsModule::RenderCommand command = [this, bone, position, orientation]()
            {
                this->setBoneTransformOnRenderThread(bone, position, orientation);
            };
            this->enqueueAndWait(std::move(command), "GraphicsModule::setBoneTransform");
        }
    }

    void GraphicsModule::setBoneTransformOnRenderThread(Ogre::Bone* bone, const Ogre::Vector3& position, const Ogre::Quaternion& orientation)
    {
        bone->setPosition(position);
        bone->setOrientation(orientation);

        /*GraphicsModule::BoneTransforms* boneTransforms = this->acquireBoneSlot(bone);
        for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
        {
            boneTransforms->transforms[i].position = position;
            boneTransforms->transforms[i].orientation = orientation;
        }

        boneTransforms->active.store(false, std::memory_order_relaxed);*/
    }

    void GraphicsModule::addTrackedDatablock(Ogre::HlmsDatablock* datablock, const Ogre::ColourValue& initialValue, std::function<void(Ogre::ColourValue)> applyFunc,
        std::function<Ogre::ColourValue(const Ogre::ColourValue&, const Ogre::ColourValue&, Ogre::Real)> interpFunc)
    {
        this->acquireDatablockSlot(datablock, initialValue, std::move(applyFunc), std::move(interpFunc));
    }

    void GraphicsModule::removeTrackedDatablock(Ogre::HlmsDatablock* datablock)
    {
        std::lock_guard<std::mutex> lock(this->datablockRegistrationMutex);

        auto it = this->datablockToIndexMap.find(datablock);
        if (it == this->datablockToIndexMap.end())
        {
            return;
        }

        size_t index = it->second;
        TrackedDatablock& slot = this->datablockPool[index];

        slot.datablock.store(nullptr, std::memory_order_release);
        slot.active.store(false, std::memory_order_relaxed);

        this->datablockToIndexMap.erase(it);
        this->freeDatablockSlots.push_back(index);
    }

    void GraphicsModule::updateTrackedDatablockValue(Ogre::HlmsDatablock* datablock, const Ogre::ColourValue& initialValue, const Ogre::ColourValue& targetValue, std::function<void(Ogre::ColourValue)> applyFunc,
        std::function<Ogre::ColourValue(const Ogre::ColourValue&, const Ogre::ColourValue&, Ogre::Real)> interpFunc)
    {
        // acquireDatablockSlot() only actually consumes initialValue/applyFunc/interpFunc
        // on first contact (when it has to create or recycle a slot); on every later
        // call for the same datablock it is purely a lock-free cache hit, and these
        // arguments are simply ignored - matching the original find-or-create semantics.
        GraphicsModule::TrackedDatablock* trackedDatablock = this->acquireDatablockSlot(datablock, initialValue, std::move(applyFunc), std::move(interpFunc));

        trackedDatablock->values[this->currentTrackedDatablockIdx] = targetValue;
        trackedDatablock->active.store(true, std::memory_order_relaxed);
    }

    void GraphicsModule::flushTransforms(void)
    {
        for (auto& nodeTransform : this->nodePool)
        {
            Ogre::Node* node = nodeTransform.node.load(std::memory_order_acquire);
            if (false == nodeTransform.active.load(std::memory_order_relaxed) || nullptr == node)
            {
                continue;
            }

            const TransformData& t = nodeTransform.transforms[this->currentTransformNodeIdx];

            if (false == nodeTransform.useDerived.load(std::memory_order_relaxed))
            {
                node->setPosition(t.position);
                node->setOrientation(t.orientation);
                node->setScale(t.scale);
            }
            else
            {
                node->_setDerivedPosition(t.position);
                node->_setDerivedOrientation(t.orientation);
            }
        }
    }

    void GraphicsModule::removeTrackedClosure(const Ogre::String& uniqueName)
    {
        // Ensure queue is initialized
        if (!this->queueInitialized.load())
        {
            return;
        }

        // Attention: producerToken is a single moodycamel ProducerToken shared by the whole
        // module and is NOT thread safe. If we are already on the render thread we own
        // persistentClosures anyway, so remove directly instead of pushing through the token
        // (updateTrackedClosure() does the same for the same reason).
        if (true == this->isRenderThread())
        {
            this->removePersistentClosure(uniqueName);
            return;
        }

        // Post a removal command so the render thread removes it at the next
        // safe point (processClosureCommands), not immediately from main thread.
        ClosureCommand cmd;
        cmd.uniqueName = uniqueName;
        cmd.isRemoval = true;
        cmd.fireAndForget = false;
        cmd.isUpdate = false;
        bool success = this->closureQueue.enqueue(this->producerToken, std::move(cmd));

        if (false == success)
        {
            Ogre::LogManager::getSingleton().logMessage("[GraphicsModule] Warning: Failed to enqueue removal command for: " + uniqueName, Ogre::LML_CRITICAL);
        }
    }

    void GraphicsModule::updateTrackedClosure(const Ogre::String& uniqueName, std::function<void(Ogre::Real)> closureFunc, bool fireAndForget)
    {
        // Ensure queue is initialized
        if (false == this->queueInitialized.load())
        {
            return;
        }

        // If already on the render thread, execute the closure immediately.
        // Queuing it would cause a one-frame delay and — in the case of
        // updateTrackedClosure — could stack duplicate entries if called
        // repeatedly from render-thread code (e.g. navmesh redraw callbacks).
        if (true == this->isRenderThread())
        {
            // Execute with dt=0 — tracked closures that run immediately are
            // one-shot context calls, not per-frame updates. Callers that
            // need the real dt should not be calling from the render thread.
            if (nullptr != closureFunc)
            {
                closureFunc(0.0f);
            }
            return;
        }

        // Create command and enqueue it - completely lock-free
        ClosureCommand command(uniqueName, std::move(closureFunc), fireAndForget, true, false);

        // Use producer token for better performance
        bool success = this->closureQueue.enqueue(producerToken, std::move(command));

        if (false == success)
        {
            // Queue is full or memory allocation failed
            // In practice, this should rarely happen with moodycamel::ConcurrentQueue
            Ogre::LogManager::getSingleton().logMessage("[GraphicsModule] Warning: Failed to enqueue closure command for: " + uniqueName, Ogre::LML_CRITICAL);
        }
    }

    void GraphicsModule::processClosureCommands(void)
    {
        // Process all available commands in the queue
        ClosureCommand command;

        // Use consumer token for better performance
        // Process up to 1000 commands per frame to prevent blocking
        size_t processedCount = 0;
        const size_t maxCommandsPerFrame = 1000;

        while (processedCount < maxCommandsPerFrame && this->closureQueue.try_dequeue(consumerToken, command))
        {
#ifdef CLOSURE_DEBUG
            // Only start tallying once we are trending toward an actual overflow -
            // keeps normal frames (a few dozen closures) completely free of the
            // hashmap insert/allocation cost below.
            if (processedCount >= 500)
            {
                ClosureCommandDiag& diag = g_closureCommandDiagnostics[command.uniqueName];
                ++diag.count;
                if (true == command.isRemoval)
                {
                    ++diag.removals;
                }
                else if (true == command.fireAndForget)
                {
                    ++diag.fireAndForget;
                }
                else if (true == command.isUpdate)
                {
                    ++diag.updates;
                }
                else
                {
                    ++diag.adds;
                }
            }
#endif

            this->processSingleCommand(command, this->currentRenderDt);
            ++processedCount;
        }

        // Log if we hit the limit (might indicate a problem)
        if (processedCount >= maxCommandsPerFrame)
        {
            Ogre::LogManager::getSingleton().logMessage("[GraphicsModule] Warning: Processed maximum closure commands per frame (" + Ogre::StringConverter::toString(maxCommandsPerFrame) + ")", Ogre::LML_NORMAL);
#ifdef CLOSURE_DEBUG
            logClosureFloodDiagnostics();
#endif
        }

#ifdef DEBUG_CLOSURE
        if (false == g_closureCommandDiagnostics.empty())
        {
            g_closureCommandDiagnostics.clear();
        }
#endif
    }

    void GraphicsModule::executeActiveClosures(void)
    {
        // Execute all persistent closures
        auto it = this->persistentClosures.begin();
        while (it != this->persistentClosures.end())
        {
            if (it->second.active && it->second.closureFunc)
            {
                try
                {
                    it->second.closureFunc(this->currentRenderDt);
                    ++it;
                }
                catch (const std::exception& e)
                {
                    // Log error and remove problematic closure
                    Ogre::LogManager::getSingleton().logMessage("[GraphicsModule] Error executing closure '" + it->first + "': " + e.what(), Ogre::LML_CRITICAL);
                    it = this->persistentClosures.erase(it);
                }
            }
            else
            {
                ++it;
            }
        }
    }

    void GraphicsModule::updateAndExecuteClosures(void)
    {
        // Cache mouse focus state once per render frame — any thread can read it
        this->myGUIFocusWidget.store(MyGUI::InputManager::getInstancePtr()->getMouseFocusWidget(), std::memory_order_relaxed);

        // First process all queued commands
        this->processClosureCommands();

        // Then execute all active closures
        this->executeActiveClosures();
    }

    void GraphicsModule::processSingleCommand(const ClosureCommand& command, Ogre::Real renderDt)
    {
        if (command.isRemoval)
        {
            // Removal: parameter not used — closure is unregistered, not called
            this->removePersistentClosure(command.uniqueName);
        }
        else if (command.fireAndForget)
        {
            // Execute immediately with the render-thread delta time.
            // renderDt is NOT an interpolation weight — it is elapsed seconds
            // since the last render frame, used by closures for time-based effects
            // (e.g. fading, animation ticking). It has nothing to do with the
            // logic-snapshot interpolation alpha managed by interpolationWeight.
            if (command.closureFunc)
            {
                try
                {
                    command.closureFunc(renderDt);
                }
                catch (const std::exception& e)
                {
                    Ogre::LogManager::getSingleton().logMessage("[GraphicsModule] Error executing fire-and-forget closure '" + command.uniqueName + "': " + e.what(), Ogre::LML_CRITICAL);
                }
            }
        }
        else
        {
            // Persistent closure: parameter not used here either — the closure
            // is only registered/updated now; renderDt is supplied when it is
            // actually *called* each frame inside executeActiveClosures().
            if (command.isUpdate)
            {
                this->updatePersistentClosure(command.uniqueName, command.closureFunc);
            }
            else
            {
                this->addPersistentClosure(command.uniqueName, command.closureFunc);
            }
        }
    }

    void GraphicsModule::addPersistentClosure(const Ogre::String& uniqueName, std::function<void(Ogre::Real)> closureFunc)
    {
        // Only insert if not already present — idempotent.
        // Subsequent calls from the same caller every frame become no-ops here.
        auto it = this->persistentClosures.find(uniqueName);
        if (it == this->persistentClosures.end())
        {
            this->persistentClosures.emplace(uniqueName, PersistentClosure(uniqueName, std::move(closureFunc)));
        }
        // If already present: do nothing. The closure is already running
        // every frame via executeActiveClosures — no re-registration needed.
    }

    void GraphicsModule::updatePersistentClosure(const Ogre::String& uniqueName, std::function<void(Ogre::Real)> closureFunc)
    {
        auto it = this->persistentClosures.find(uniqueName);
        if (it != this->persistentClosures.end())
        {
            it->second.closureFunc = std::move(closureFunc);
            it->second.active = true;
        }
        else
        {
            // If it doesn't exist, create it
            this->addPersistentClosure(uniqueName, std::move(closureFunc));
        }
    }

    void GraphicsModule::removePersistentClosure(const Ogre::String& uniqueName)
    {
        this->persistentClosures.erase(uniqueName);
    }

    size_t GraphicsModule::getPreviousTransformNodeIdx(void) const
    {
        return (this->currentTransformNodeIdx + NUM_TRANSFORM_BUFFERS - 1) % NUM_TRANSFORM_BUFFERS;
    }

    size_t GraphicsModule::getPreviousTransformCameraIdx(void) const
    {
        return (this->currentTransformCameraIdx + NUM_TRANSFORM_BUFFERS - 1) % NUM_TRANSFORM_BUFFERS;
    }

    size_t GraphicsModule::getPreviousTransformBoneIdx(void) const
    {
        return (this->currentTransformBoneIdx + NUM_TRANSFORM_BUFFERS - 1) % NUM_TRANSFORM_BUFFERS;
    }

    size_t GraphicsModule::getPreviousTrackedDatablockIdx(void) const
    {
        return (this->currentTrackedDatablockIdx + NUM_TRANSFORM_BUFFERS - 1) % NUM_TRANSFORM_BUFFERS;
    }

    void GraphicsModule::enableDebugVisualization(bool enable)
    {
        this->debugVisualization = enable;
    }

    void GraphicsModule::dumpBufferState() const
    {
        if (true == this->debugVisualization)
        {
            Ogre::LogManager& logManager = Ogre::LogManager::getSingleton();

            logManager.logMessage(Ogre::LML_CRITICAL, "=== BUFFER STATE DUMP ===", false);
            logManager.logMessage(Ogre::LML_CRITICAL, "Current buffer index: " + std::to_string(this->currentTransformNodeIdx), false);
            logManager.logMessage(Ogre::LML_CRITICAL, "Previous buffer index: " + std::to_string(this->getPreviousTransformNodeIdx()), false);
            logManager.logMessage(Ogre::LML_CRITICAL, "Tracked nodes: " + std::to_string(this->nodePool.size()), false);

            for (size_t i = 0; i < this->nodePool.size(); ++i)
            {
                const NodeTransforms& nodeTransform = this->nodePool[i];
                Ogre::Node* node = nodeTransform.node.load(std::memory_order_relaxed);
                logManager.logMessage(Ogre::LML_CRITICAL, "Node " + std::to_string(i) + " (" + Ogre::StringConverter::toString(node) + "):", false);
                logManager.logMessage(Ogre::LML_CRITICAL, "  Active: " + std::string(nodeTransform.active.load(std::memory_order_relaxed) ? "yes" : "no"), false);
                logManager.logMessage(Ogre::LML_CRITICAL, "  New: " + std::string(nodeTransform.isNew ? "yes" : "no"), false);

                for (size_t j = 0; j < NUM_TRANSFORM_BUFFERS; ++j)
                {
                    const auto& transform = nodeTransform.transforms[j];
                    logManager.logMessage(Ogre::LML_CRITICAL, "  Buffer " + std::to_string(j) + ":", false);
                    logManager.logMessage(Ogre::LML_CRITICAL, "    Position: " + Ogre::StringConverter::toString(transform.position), false);
                    logManager.logMessage(Ogre::LML_CRITICAL, "    Orientation: " + Ogre::StringConverter::toString(transform.orientation), false);
                    logManager.logMessage(Ogre::LML_CRITICAL, "    Scale: " + Ogre::StringConverter::toString(transform.scale), false);
                }
            }

            logManager.logMessage(Ogre::LML_CRITICAL, "========================", false);
        }
    }

    void GraphicsModule::advanceTransformBuffer(void)
    {
        // Runs in Main thread.

        // =========================================================================
        // Node transforms — NO MUTEX in the loop body
        // =========================================================================
        {
            size_t prevIdx = this->currentTransformNodeIdx;
            this->currentTransformNodeIdx = (this->currentTransformNodeIdx + 1) % NUM_TRANSFORM_BUFFERS;

            if (true == this->debugVisualization)
            {
                this->logCommandEvent("[RenderCommandQueueModule]: Advanced buffer from " + Ogre::StringConverter::toString(prevIdx) + " to " + Ogre::StringConverter::toString(this->currentTransformNodeIdx), Ogre::LML_TRIVIAL);
            }

            for (auto& nodeTransform : this->nodePool)
            {
                Ogre::Node* node = nodeTransform.node.load(std::memory_order_acquire);

                if (true == nodeTransform.isNew)
                {
                    if (nullptr == node)
                    {
                        continue;
                    }

                    GraphicsModule::TransformData currentTransform;
                    if (true == nodeTransform.useDerived.load(std::memory_order_relaxed))
                    {
                        currentTransform.position = node->_getDerivedPosition();
                        currentTransform.orientation = node->_getDerivedOrientation();
                        currentTransform.scale = node->_getDerivedScale();
                    }
                    else
                    {
                        currentTransform.position = node->getPosition();
                        currentTransform.orientation = node->getOrientation();
                        currentTransform.scale = node->getScale();
                    }

                    for (size_t b = 0; b < NUM_TRANSFORM_BUFFERS; ++b)
                    {
                        nodeTransform.transforms[b] = currentTransform;
                    }

                    nodeTransform.isNew = false;
                }
                else if (true == nodeTransform.active.load(std::memory_order_relaxed))
                {
                    // Carry the last value forward into the new current slot. A node
                    // that genuinely is not being updated this tick simply keeps

                    nodeTransform.transforms[this->currentTransformNodeIdx] = nodeTransform.transforms[prevIdx];
                }
            }
        }

        // =========================================================================
        // Camera transforms
        // =========================================================================
        {
            size_t prevCameraIdx = this->currentTransformCameraIdx;
            this->currentTransformCameraIdx = (this->currentTransformCameraIdx + 1) % NUM_TRANSFORM_BUFFERS;

            for (auto& cameraTransform : this->cameraPool)
            {
                Ogre::Camera* camera = cameraTransform.camera.load(std::memory_order_acquire);

                if (true == cameraTransform.isNew)
                {
                    if (nullptr == camera)
                    {
                        continue;
                    }

                    GraphicsModule::CameraTransformData currentTransform;
                    currentTransform.position = camera->getPosition();
                    currentTransform.orientation = camera->getOrientation();

                    for (size_t b = 0; b < NUM_TRANSFORM_BUFFERS; ++b)
                    {
                        cameraTransform.transforms[b] = currentTransform;
                    }

                    cameraTransform.isNew = false;
                }
                else if (true == cameraTransform.active.load(std::memory_order_relaxed))
                {
                    cameraTransform.transforms[this->currentTransformCameraIdx] = cameraTransform.transforms[prevCameraIdx];
                }
            }
        }

        // =========================================================================
        // Bone transforms
        // =========================================================================
        {
            size_t prevBoneIdx = this->currentTransformBoneIdx;
            this->currentTransformBoneIdx = (this->currentTransformBoneIdx + 1) % NUM_TRANSFORM_BUFFERS;

            for (auto& boneTransform : this->bonePool)
            {
                Ogre::Bone* bone = boneTransform.bone.load(std::memory_order_acquire);

                if (true == boneTransform.isNew)
                {
                    if (nullptr == bone)
                    {
                        continue;
                    }

                    GraphicsModule::TransformData currentTransform;
                    currentTransform.position = bone->getPosition();
                    currentTransform.orientation = bone->getOrientation();

                    for (size_t b = 0; b < NUM_TRANSFORM_BUFFERS; ++b)
                    {
                        boneTransform.transforms[b] = currentTransform;
                    }

                    boneTransform.isNew = false;
                }
                else if (true == boneTransform.active.load(std::memory_order_relaxed))
                {
                    boneTransform.transforms[this->currentTransformBoneIdx] = boneTransform.transforms[prevBoneIdx];
                }
            }
        }

        // =========================================================================
        // Datablock transforms (no eviction)
        // =========================================================================
        {
            size_t prevDatablockIdx = this->currentTrackedDatablockIdx;
            this->currentTrackedDatablockIdx = (this->currentTrackedDatablockIdx + 1) % NUM_TRANSFORM_BUFFERS;

            for (auto& datablock : this->datablockPool)
            {
                if (datablock.isNew)
                {
                    for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
                    {
                        datablock.values[i] = datablock.values[this->currentTrackedDatablockIdx];
                    }
                    datablock.isNew = false;
                }
                else if (datablock.active.load(std::memory_order_relaxed))
                {
                    datablock.values[this->currentTrackedDatablockIdx] = datablock.values[prevDatablockIdx];
                }
            }
        }

        this->accumTimeSinceLastLogicFrame = 0.0f;
    }

    void GraphicsModule::updateAllTransforms(void)
    {
        // Get the previous buffer index
        size_t prevIdx = this->getPreviousTransformNodeIdx();

        // Update all active nodes
        {
            for (const auto& nodeTransform : this->nodePool)
            {
                if (true == nodeTransform.active.load(std::memory_order_relaxed))
                {
                    Ogre::Node* node = nodeTransform.node.load(std::memory_order_relaxed);
                    if (nullptr == node)
                    {
                        continue;
                    }

                    // Get previous and current transforms
                    const GraphicsModule::TransformData& prevTransform = nodeTransform.transforms[prevIdx];
                    const GraphicsModule::TransformData& currTransform = nodeTransform.transforms[this->currentTransformNodeIdx];

                    // Interpolate position
                    Ogre::Vector3 interpPos = Ogre::Math::lerp(prevTransform.position, currTransform.position, this->interpolationWeight);

                    // Interpolate orientation
                    Ogre::Quaternion interpRot = Ogre::Quaternion::nlerp(this->interpolationWeight, prevTransform.orientation, currTransform.orientation, true);

                    // Interpolate scale
                    Ogre::Vector3 interpScale = Ogre::Math::lerp(prevTransform.scale, currTransform.scale, this->interpolationWeight);

                    // Apply to scene node
                    if (false == nodeTransform.useDerived.load(std::memory_order_relaxed))
                    {
                        node->setPosition(interpPos);
                        node->setOrientation(interpRot);
                        node->setScale(interpScale);
                    }
                    else
                    {
                        node->_setDerivedPosition(interpPos);
                        node->_setDerivedOrientation(interpRot);
                        // Comment says: "Scale only on non-derived path"
                    }
                }
            }
        }

        // Update camera transforms

        // Get the previous buffer index
        size_t prevCameraIdx = this->getPreviousTransformCameraIdx();

        // Update all active cameras
        {
            for (const auto& cameraTransform : this->cameraPool)
            {
                if (true == cameraTransform.active.load(std::memory_order_relaxed))
                {
                    Ogre::Camera* camera = cameraTransform.camera.load(std::memory_order_relaxed);
                    if (nullptr == camera)
                    {
                        continue;
                    }

                    // Get previous and current transforms
                    const GraphicsModule::CameraTransformData& prevTransform = cameraTransform.transforms[prevCameraIdx];
                    const GraphicsModule::CameraTransformData& currTransform = cameraTransform.transforms[this->currentTransformCameraIdx];

                    // Interpolate position
                    Ogre::Vector3 interpPos = Ogre::Math::lerp(prevTransform.position, currTransform.position, this->interpolationWeight);

                    // Interpolate orientation
                    Ogre::Quaternion interpRot = Ogre::Quaternion::nlerp(this->interpolationWeight, prevTransform.orientation, currTransform.orientation, true);

                    // Apply to scene camera
                    camera->setOrientation(interpRot);
                    camera->setPosition(interpPos);
                }
            }
        }

        // Update bone transforms

        size_t prevBoneIdx = this->getPreviousTransformBoneIdx();

        {
            for (const auto& boneTransform : this->bonePool)
            {
                if (true == boneTransform.active.load(std::memory_order_relaxed))
                {
                    Ogre::Bone* bone = boneTransform.bone.load(std::memory_order_relaxed);
                    if (nullptr == bone)
                    {
                        continue;
                    }

                    const GraphicsModule::TransformData& prevTransform = boneTransform.transforms[prevBoneIdx];
                    const GraphicsModule::TransformData& currTransform = boneTransform.transforms[this->currentTransformBoneIdx];

                    Ogre::Vector3 interpPos = Ogre::Math::lerp(prevTransform.position, currTransform.position, this->interpolationWeight);
                    Ogre::Quaternion interpRot = Ogre::Quaternion::nlerp(this->interpolationWeight, prevTransform.orientation, currTransform.orientation, true);

                    bone->setOrientation(interpRot);
                    bone->setPosition(interpPos);
                }
            }
        }

        // Update datablock colours

        // Get the previous buffer index
        size_t prevTrackedDatablockIdx = this->getPreviousTrackedDatablockIdx();

        {
            for (const auto& trackedDatablock : this->datablockPool)
            {
                if (false == trackedDatablock.active.load(std::memory_order_relaxed))
                {
                    continue;
                }

                if (nullptr == trackedDatablock.datablock.load(std::memory_order_relaxed))
                {
                    continue;
                }

                const Ogre::ColourValue& prev = trackedDatablock.values[prevTrackedDatablockIdx];
                const Ogre::ColourValue& curr = trackedDatablock.values[this->currentTrackedDatablockIdx];

                Ogre::ColourValue result = trackedDatablock.interpolateFunc(prev, curr, this->interpolationWeight);

                trackedDatablock.applyFunc(result);
            }
        }

        // Note: updateAndExecuteClosures() is no longer called here.
        // It is called explicitly after renderOneFrame() in the render loop
        // so that closures reading RenderingMetrics see populated values.
    }

    void GraphicsModule::calculateInterpolationWeight(void)
    {
        // Calculate weight based on accumulated time and frame time
        // This is critical for smooth interpolation
        Ogre::Real weight = this->accumTimeSinceLastLogicFrame / this->frameTime;

        // Clamp weight to [0,1]
        this->interpolationWeight = std::min(1.0f, std::max(0.0f, weight));
    }

    void GraphicsModule::setAccumTimeSinceLastLogicFrame(Ogre::Real time)
    {
        this->accumTimeSinceLastLogicFrame = time;
    }

    Ogre::Real GraphicsModule::getAccumTimeSinceLastLogicFrame(void) const
    {
        return this->accumTimeSinceLastLogicFrame;
    }

    void GraphicsModule::setFrameTime(Ogre::Real frameTime)
    {
        this->frameTime = frameTime;
    }

    void GraphicsModule::beginLogicFrame(void)
    {
        // Advance the transform buffer to the next buffer
        // This is called at the start of each logic frame
        this->advanceTransformBuffer();
    }

    void GraphicsModule::endLogicFrame(void)
    {
        // Marks the logic snapshot as complete and announces it to the render thread.
        // Must be called exactly once after all fixed-step updates for a given outer
        // loop iteration have finished — i.e. after the last update(fixedDt) call
        // and after publishInterpolationAlpha() has been called for this iteration.
        //
        // Pair with beginLogicFrame(), which opens the snapshot by advancing the
        // transform buffer. Do NOT call endLogicFrame() without a preceding
        // beginLogicFrame() in the same outer loop iteration.
        //
        // publishInterpolationAlpha() is intentionally NOT called here because alpha
        // must be published every outer loop iteration (even when no fixed steps ran),
        // whereas endLogicFrame() is only called when at least one step ran.
        this->publishLogicFrame();
    }

    void GraphicsModule::setLogLevel(Ogre::LogMessageLevel level)
    {
        this->logLevel = level;
    }

    Ogre::LogMessageLevel GraphicsModule::getLogLevel(void) const
    {
        return this->logLevel;
    }

    void GraphicsModule::logCommandEvent(const Ogre::String& message, Ogre::LogMessageLevel level) const
    {
        // Attention: the early-out must come FIRST. The caller has already paid for building
        // the message string, but everything below (stringstream construction, formatting,
        // the isRenderThread() atomic load) was executed for every TRIVIAL call too. During
        // scene import this runs thousands of times and shows up in the profile as
        // StringConverter::toString.
        if (level != Ogre::LML_CRITICAL)
        {
            return;
        }

        std::stringstream ss;
        ss << "[RenderCommandQueue] " << message << " (Thread: " << (this->isRenderThread() ? "RENDER" : "MAIN") << ", Depth: " << g_renderCommandDepth << ", WaitDepth: " << g_waitDepth << ")";

        Ogre::LogManager::getSingletonPtr()->logMessage(level, ss.str());
    }

    void GraphicsModule::logCommandState(const char* commandName, bool willWait) const
    {
        // TODO: REmoved log flooding
        return;

        std::stringstream ss;
        ss << "Command '" << commandName << "' - Will wait: " << (willWait ? "YES" : "NO");
        this->logCommandEvent(ss.str(), Ogre::LML_NORMAL);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////

    void GraphicsModule::enqueueDestroy(GraphicsModule::DestroyCommand destroyCommand, const char* commandName)
    {
        // No render thread at all: bypass ring-buffer, execute immediately with wait.
        // Attention: checks renderThreadAlive and not bRunning. Between stopRendering() and
        // the render thread actually returning, the destroy slots are still owned and
        // advanced by the render thread, so pushing or executing here would race.
        if (false == this->renderThreadAlive.load(std::memory_order_acquire))
        {
            this->enqueueAndWait(std::move(destroyCommand), commandName);
            return;
        }

        // Shutdown teardown in progress: bypass the ring-buffer as well and destroy right
        // away, on the render thread.
        //
        // Attention: this branch is what makes the teardown safe. The ring-buffer delays a
        // destroy by NUM_DESTROY_SLOTS *rendered frames*, and during the shutdown drain no
        // frame is ever rendered again. A deferred command would therefore only run at the
        // end of renderThreadFunction, i.e. after AppState::destroyModules() already called
        // Core::destroyScene() - every Ogre pointer it captured would be dangling by then.
        // Executing now is safe precisely because rendering has stopped: nothing can still
        // be reading the resource we are about to destroy.
        if (true == this->shutdownDrain.load(std::memory_order_acquire))
        {
            this->enqueueAndWait(std::move(destroyCommand), commandName);
            return;
        }

        // A wait closure is in flight on THIS thread. We cannot push into
        // destroySlots right now because the render thread owns the slot state
        // during command execution. Defer until enqueueAndWait() finishes, then
        // flushDeferredDestroyCommands() will move them into the ring-buffer safely.
        if (true == g_insideWaitClosure)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL,
                "[GraphicsModule] Deferring destroy '" + Ogre::String(commandName) + "' (wait closure in flight, " + Ogre::StringConverter::toString(this->deferredDestroyCommands.size() + 1) + " total deferred)");

            this->deferredDestroyCommands.emplace_back(std::move(destroyCommand), commandName);
            return;
        }

        this->destroySlots[this->currentDestroySlot.load(std::memory_order_acquire)].emplace_back(std::move(destroyCommand));
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "Enqueue destroy command: " + Ogre::String(commandName));
    }

    void GraphicsModule::advanceFrameAndDestroyOld(void)
    {
        // this->isRunningDestroyClosure = true;

        // Slot to destroy is two frames behind
        const size_t currentSlot = this->currentDestroySlot.load(std::memory_order_acquire);
        const size_t destroySlot = (currentSlot + 1) % GraphicsModule::NUM_DESTROY_SLOTS;

        for (auto& destroyCommand : this->destroySlots[destroySlot])
        {
            // Destroy safely (now guaranteed not in use)
            destroyCommand();
        }
        this->destroySlots[destroySlot].clear();

        // Advance to next logic slot
        this->currentDestroySlot.store(destroySlot, std::memory_order_release);

        // this->isRunningDestroyClosure = false;
    }

    bool GraphicsModule::hasPendingDestroyCommands(void) const
    {
        return false == this->destroySlots[this->currentDestroySlot].empty();
    }

    void GraphicsModule::drainAllDestroyCommands(void)
    {
        // Flush anything that is still sitting in the deferred list first, otherwise those
        // commands would be moved into the ring buffer after we already emptied it.
        this->flushDeferredDestroyCommands();

        // Execute every slot, not just the one that is two frames behind. This is the whole
        // point: normally advanceFrameAndDestroyOld() only drains one slot per rendered frame,
        // which is correct while rendering continues, but fatal if the scene manager is about
        // to be destroyed - the captured Ogre pointers would dangle by the time the render
        // thread gets around to those slots.
        auto drainCommand = [this]()
        {
            for (size_t slot = 0; slot < GraphicsModule::NUM_DESTROY_SLOTS; slot++)
            {
                for (auto& destroyCommand : this->destroySlots[slot])
                {
                    destroyCommand();
                }
                this->destroySlots[slot].clear();
            }
        };

        if (true == this->isRenderThread())
        {
            // Already on the render thread - run directly, an enqueueAndWait would deadlock.
            drainCommand();
            return;
        }

        NOWA::GraphicsModule::RenderCommand renderCommand = [drainCommand]()
        {
            drainCommand();
        };
        this->enqueueAndWait(std::move(renderCommand), "GraphicsModule::drainAllDestroyCommands");
    }

}; // namespace end