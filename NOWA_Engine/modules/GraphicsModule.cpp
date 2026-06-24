#include "NOWAPrecompiled.h"
#include "GraphicsModule.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "main/InputDeviceCore.h"

#include "Animation/OgreBone.h"

#include <sstream>

namespace
{
    std::chrono::milliseconds g_defaultTimeout(5000); // 5 seconds
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
        currentTransformOldBoneIdx(0),
        currentTransformBoneIdx(0),
        currentTransformPassIdx(0),
        currentTrackedDatablockIdx(0),
        interpolationWeight(0.0f),
        accumTimeSinceLastLogicFrame(0.0f),
        frameTime(1.0f / 60.0f),
        currentRenderDt(0.0f),
        debugVisualization(false),
        currentDestroySlot(0),
        isRunningWaitClosure(false),
        closureQueue(),
        producerToken(closureQueue),
        consumerToken(closureQueue),
        stallRequested(false),
        stallAcknowledged(false)
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

        this->oldBonePool.resize(GraphicsModule::OLD_BONE_POOL_CAPACITY);
        this->freeOldBoneSlots.reserve(GraphicsModule::OLD_BONE_POOL_CAPACITY);
        for (size_t i = 0; i < GraphicsModule::OLD_BONE_POOL_CAPACITY; ++i)
        {
            this->freeOldBoneSlots.push_back(i);
        }

        this->bonePool.resize(GraphicsModule::BONE_POOL_CAPACITY);
        this->freeBoneSlots.reserve(GraphicsModule::BONE_POOL_CAPACITY);
        for (size_t i = 0; i < GraphicsModule::BONE_POOL_CAPACITY; ++i)
        {
            this->freeBoneSlots.push_back(i);
        }

        this->passPool.resize(GraphicsModule::PASS_POOL_CAPACITY);
        this->freePassSlots.reserve(GraphicsModule::PASS_POOL_CAPACITY);
        for (size_t i = 0; i < GraphicsModule::PASS_POOL_CAPACITY; ++i)
        {
            this->freePassSlots.push_back(i);
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
        this->renderThread = std::thread(&GraphicsModule::renderThreadFunction, this);
    }

    void GraphicsModule::stopRendering(void)
    {
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

#ifdef _DEBUG
        this->enableTimeout(false);
        this->setLogLevel(Ogre::LML_TRIVIAL);
#else
        this->setLogLevel(Ogre::LML_TRIVIAL);
#endif

        static int frameCount = 0;

        Ogre::Timer timer;
        Ogre::uint64 lastFrameTime = timer.getMicroseconds();

        Ogre::Window* renderWindow = NOWA::Core::getSingletonPtr()->getOgreRenderWindow();
        const auto appStateManager = NOWA::AppStateManager::getSingletonPtr();

        while (true == this->bRunning)
        {
            // ── STALL HANDSHAKE ─────────────────────────────────────────────────────
            // Must be FIRST: logic thread may have called requestStall() and is waiting
            // to finish the previous iteration before it touches vectors.
            if (this->stallRequested.load(std::memory_order_acquire))
            {
                // Acknowledge: we are at a frame boundary, not inside any vector loop
                this->stallAcknowledged.store(true, std::memory_order_release);

                // Park here while logic thread does clearSceneResources() / state switch
                while (this->stallRequested.load(std::memory_order_acquire))
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }

                // Logic thread called releaseStall() — resume normal rendering
                this->stallAcknowledged.store(false, std::memory_order_release);
                continue;
            }

            Ogre::uint64 currentTime = timer.getMicroseconds();
            Ogre::Real deltaTime = (currentTime - lastFrameTime) * 0.000001f;
            lastFrameTime = currentTime;
            this->currentRenderDt = deltaTime;

            GameProgressModule* gameProgressModule = appStateManager->getActiveGameProgressModuleSafe();
            const bool isStalled = appStateManager->bStall.load();
            const bool isSceneLoading = (gameProgressModule != nullptr) ? gameProgressModule->bSceneLoading.load() : false;

            if (false == isStalled && false == isSceneLoading)
            {
                NOWA::InputDeviceCore::getSingletonPtr()->capture(deltaTime);
                this->advanceFrameAndDestroyOld();
                this->processAllCommands();

                const float alpha = this->consumeInterpolationAlpha();
                this->setInterpolationWeight(alpha);
                this->updateAllTransforms();

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
                // Only clear closure state here — it is render-thread-owned.
                this->clearAllClosures();
            }

            if (false == isStalled && false == this->isWorkspaceTransitioning() && false == isSceneLoading)
            {
                Ogre::Root::getSingletonPtr()->renderOneFrame();

                // Execute closures AFTER renderOneFrame so RenderingMetrics are
                // populated when closures read them (e.g. DesignState::updateInfo).
                // Node/bone/datablock interpolation already ran in updateAllTransforms
                // above before renderOneFrame, so visual correctness is preserved.
                this->updateAndExecuteClosures();
            }
        }

        while (this->hasPendingRenderCommands())
        {
            this->processAllCommands();
        }

        // Now it's safe to do frame advancement
        for (size_t i = 0; i < NOWA::GraphicsModule::NUM_DESTROY_SLOTS; ++i)
        {
            this->advanceFrameAndDestroyOld();
        }

        // Check for remaining commands
        const int remainingCommands = this->queue.size_approx();
        if (remainingCommands > 0)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[RenderCommandQueueModule]: Illegal state, as there are still: " + Ogre::StringConverter::toString(remainingCommands) + " pending commands!");
            throw;
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

        this->oldBonePool.clear();
        this->oldBoneToIndexMap.clear();
        this->freeOldBoneSlots.clear();

        this->bonePool.clear();
        this->boneToIndexMap.clear();
        this->freeBoneSlots.clear();

        this->passPool.clear();
        this->passToIndexMap.clear();
        this->freePassSlots.clear();

        this->datablockPool.clear();
        this->datablockToIndexMap.clear();
        this->freeDatablockSlots.clear();

        this->bRunning = false;
        this->timeoutEnabled = false;
        this->timeoutDuration = g_defaultTimeout.count();
        this->logLevel = Ogre::LML_NORMAL;
        this->currentTransformNodeIdx = 0;
        this->currentTransformCameraIdx = 0;
        this->currentTransformOldBoneIdx = 0;
        this->currentTransformBoneIdx = 0;
        this->currentTransformPassIdx = 0;
        this->currentTrackedDatablockIdx = 0;

        this->accumTimeSinceLastLogicFrame = 0.0f;

        this->frameTime = 1.0f / 60.0f;
        this->debugVisualization = false;
        this->currentDestroySlot = 0;
        this->isRunningWaitClosure = false;

        Core::getSingletonPtr()->saveHlmsDiskCache();
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
        // Clear the concurrent queue - drain all pending commands
        ClosureCommand command;
        size_t clearedCommands = 0;
        while (this->closureQueue.try_dequeue(consumerToken, command))
        {
            ++clearedCommands;
        }

        // Clear all persistent closures
        this->persistentClosures.clear();

        // Log the cleanup for debugging
        if (clearedCommands > 0)
        {
            Ogre::LogManager::getSingleton().logMessage("[GraphicsModule] Cleared " + Ogre::StringConverter::toString(clearedCommands) + " queued closure commands and " + Ogre::StringConverter::toString(persistentClosures.size()) +
                                                            " persistent closures",
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
            std::lock_guard<std::mutex> lock(this->oldBoneRegistrationMutex);
            for (auto& slot : this->oldBonePool)
            {
                slot.oldBone.store(nullptr, std::memory_order_relaxed);
                slot.active.store(false, std::memory_order_relaxed);
            }
            this->oldBoneToIndexMap.clear();
            this->freeOldBoneSlots.clear();
            for (size_t i = 0; i < this->oldBonePool.size(); ++i)
            {
                this->freeOldBoneSlots.push_back(i);
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
            std::lock_guard<std::mutex> lock(this->passRegistrationMutex);
            for (auto& slot : this->passPool)
            {
                slot.pass.store(nullptr, std::memory_order_relaxed);
                slot.active.store(false, std::memory_order_relaxed);
            }
            this->passToIndexMap.clear();
            this->freePassSlots.clear();
            for (size_t i = 0; i < this->passPool.size(); ++i)
            {
                this->freePassSlots.push_back(i);
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
        this->currentTransformOldBoneIdx = 0;
        this->currentTransformBoneIdx = 0;
        this->currentTransformPassIdx = 0;
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
        // Wait for render thread to finish before cleanup
        if (this->renderThread.joinable())
        {
            this->renderThread.join();
        }
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
                    this->isRunningWaitClosure = false;
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

        // Submit a command that processes all pending commands and blocks until it's done
        auto promise = std::make_shared<std::promise<void>>();
        auto future = promise->get_future();

        // Create a special sync command that processes everything in the queue
        this->enqueue(
            [this]()
            {
                // This will execute in the render thread and process any commands ahead of it
                this->logCommandEvent("Processing queue from waitForRenderCompletion", Ogre::LML_NORMAL);
            },
            "", promise);

        // Wait for the command to complete
        try
        {
            this->logCommandEvent("Waiting for render queue synchronization", Ogre::LML_NORMAL);
            auto timeout = getTimeoutDuration();
            if (true == this->isTimeoutEnabled())
            {
                if (future.wait_for(timeout) == std::future_status::timeout)
                {
                    this->logCommandEvent("Timeout occurred waiting for render queue synchronization", Ogre::LML_NORMAL);
                }
            }
            else
            {
                future.wait();
            }
            this->isRunningWaitClosure = false;
            this->logCommandEvent("Render queue synchronization complete", Ogre::LML_NORMAL);
        }
        catch (const std::exception& e)
        {
            this->logCommandEvent(std::string("Exception waiting for render completion: ") + e.what(), Ogre::LML_CRITICAL);
        }
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
        this->isRunningWaitClosure = true;

        // ── Render-thread re-entrant path ─────────────────────────────────────
        if (true == this->isRenderThread() && RenderGlobals::g_renderCommandDepth > 0)
        {
            try
            {
                this->logCommandEvent(std::string("Executing '") + commandName + "' directly on render thread with **WAIT** (re-entrant)", Ogre::LML_TRIVIAL);
                command();

                this->flushDeferredDestroyCommands(); // <- flush before clearing flag
                this->isRunningWaitClosure = false;
                return;
            }
            catch (const std::exception& e)
            {
                this->logCommandEvent(std::string("Exception in direct execution of '") + commandName + "': " + e.what(), Ogre::LML_CRITICAL);
                this->flushDeferredDestroyCommands(); // <- flush even on exception
                this->isRunningWaitClosure = false;
                throw;
            }
            catch (...)
            {
                this->logCommandEvent(std::string("Unknown exception in direct execution of '") + commandName + "'", Ogre::LML_CRITICAL);
                this->flushDeferredDestroyCommands(); // <- flush even on exception
                this->isRunningWaitClosure = false;
                throw;
            }
        }

        // ── Normal path (logic thread -> render thread) ────────────────────────
        this->incrementWaitDepth();

        try
        {
            auto promise = std::make_shared<std::promise<void>>();
            auto future = promise->get_future();

            this->logCommandEvent(std::string("Enqueueing '") + commandName + "' with **WAIT**", Ogre::LML_NORMAL);

            bool isNested = isInNestedWait() && g_waitDepth > 1;

            if (isNested)
            {
                this->logCommandEvent(std::string("Command '") + commandName + "' is a nested **WAIT** (depth: " + std::to_string(g_waitDepth) + ")", Ogre::LML_NORMAL);
            }

            this->enqueue(std::move(command), commandName, promise);

            if (false == isNested)
            {
                if (true == this->isTimeoutEnabled())
                {
                    auto timeout = getTimeoutDuration();
                    auto status = future.wait_for(timeout);

                    if (status == std::future_status::timeout)
                    {
                        this->logCommandEvent(std::string("Timeout waiting for '") + commandName + "'", Ogre::LML_CRITICAL);
                        this->processQueueSync();
                    }
                }
                else
                {
                    while (future.wait_for(std::chrono::milliseconds(1)) != std::future_status::ready)
                    {
                        std::this_thread::yield();
                    }
                }
            }
            else
            {
                this->logCommandEvent(std::string("Nested command '") + commandName + "' enqueued without blocking wait", Ogre::LML_NORMAL);
            }
        }
        catch (const std::exception& e)
        {
            this->logCommandEvent(std::string("Exception waiting for '") + commandName + "': " + e.what(), Ogre::LML_CRITICAL);
            this->decrementWaitDepth();
            this->flushDeferredDestroyCommands(); // <- flush even on exception
            this->isRunningWaitClosure = false;
            throw;
        }
        catch (...)
        {
            this->logCommandEvent(std::string("Unknown exception waiting for '") + commandName + "'", Ogre::LML_CRITICAL);
            this->decrementWaitDepth();
            this->flushDeferredDestroyCommands(); // <- flush even on exception
            this->isRunningWaitClosure = false;
            throw;
        }

        this->decrementWaitDepth();
        this->logCommandEvent(std::string("Command '") + commandName + "' completed", Ogre::LML_TRIVIAL);

        this->flushDeferredDestroyCommands(); // <- flush on normal completion
        this->isRunningWaitClosure = false;
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

        // Otherwise, we need to add a sync point and wait for it
        auto syncPromise = std::make_shared<std::promise<void>>();
        auto syncFuture = syncPromise->get_future();

        // Create a special command that just completes the sync
        enqueue(
            [this]()
            {
                // This will execute after all previous commands
                this->logCommandEvent("Sync point reached in queue processing", Ogre::LML_NORMAL);
            },
            "", syncPromise);

        // Wait without timeout - we need to ensure this completes
        this->logCommandEvent("Waiting for queue sync point", Ogre::LML_NORMAL);
        syncFuture.wait();
        this->logCommandEvent("Queue sync point completed", Ogre::LML_NORMAL);
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

            // Reset wait depth if it's non-zero (something might have gone wrong)
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
        if (false == this->timeoutEnabled)
        {
            // If timeout is disabled, wait indefinitely
            future.wait();
            return true;
        }

        auto status = future.wait_for(timeout);
        if (status == std::future_status::timeout)
        {
            std::stringstream ss;
            ss << "Command '" << commandName << "' timed out after " << timeout.count() << "ms";
            this->logCommandEvent(ss.str(), Ogre::LML_CRITICAL);

            // Try to recover from the timeout
            this->recoverFromTimeout();

            return false;
        }
        return true;
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

    GraphicsModule::OldBoneTransforms* GraphicsModule::resolveOldBoneSlotLocked(Ogre::v1::OldBone* oldBone)
    {
        std::lock_guard<std::mutex> lock(this->oldBoneRegistrationMutex);

        auto it = this->oldBoneToIndexMap.find(oldBone);
        if (it != this->oldBoneToIndexMap.end())
        {
            return &this->oldBonePool[it->second];
        }

        size_t index;
        if (true == this->freeOldBoneSlots.empty())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GraphicsModule] OldBone pool exhausted (capacity " + Ogre::StringConverter::toString(GraphicsModule::OLD_BONE_POOL_CAPACITY) + "). Raise OLD_BONE_POOL_CAPACITY.");
            return &this->oldBoneOverflowSink;
        }
        index = this->freeOldBoneSlots.back();
        this->freeOldBoneSlots.pop_back();

        OldBoneTransforms& slot = this->oldBonePool[index];

        GraphicsModule::TransformData baseline;
        baseline.position = oldBone->getPosition();
        baseline.orientation = oldBone->getOrientation();
        for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
        {
            slot.transforms[i] = baseline;
        }

        slot.isNew = true;
        slot.active.store(true, std::memory_order_relaxed);
        slot.oldBone.store(oldBone, std::memory_order_release);

        this->oldBoneToIndexMap[oldBone] = index;

        return &slot;
    }

    GraphicsModule::OldBoneTransforms* GraphicsModule::acquireOldBoneSlot(Ogre::v1::OldBone* oldBone)
    {
        thread_local std::unordered_map<Ogre::v1::OldBone*, OldBoneTransforms*> tlsCache;

        auto cacheIt = tlsCache.find(oldBone);
        if (cacheIt != tlsCache.end())
        {
            OldBoneTransforms* slot = cacheIt->second;
            if (slot->oldBone.load(std::memory_order_acquire) == oldBone)
            {
                return slot;
            }
            tlsCache.erase(cacheIt);
        }

        OldBoneTransforms* slot = this->resolveOldBoneSlotLocked(oldBone);
        tlsCache[oldBone] = slot;
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

    GraphicsModule::PassTransforms* GraphicsModule::resolvePassSlotLocked(Ogre::Pass* pass)
    {
        std::lock_guard<std::mutex> lock(this->passRegistrationMutex);

        auto it = this->passToIndexMap.find(pass);
        if (it != this->passToIndexMap.end())
        {
            return &this->passPool[it->second];
        }

        size_t index;
        if (true == this->freePassSlots.empty())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GraphicsModule] Pass pool exhausted (capacity " + Ogre::StringConverter::toString(GraphicsModule::PASS_POOL_CAPACITY) + "). Raise PASS_POOL_CAPACITY.");
            return &this->passOverflowSink;
        }
        index = this->freePassSlots.back();
        this->freePassSlots.pop_back();

        PassTransforms& slot = this->passPool[index];

        GraphicsModule::PassSpeedData currentData;
        for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
        {
            slot.transforms[i] = currentData;
        }

        slot.isNew = true;
        slot.active.store(true, std::memory_order_relaxed);
        slot.pass.store(pass, std::memory_order_release);

        this->passToIndexMap[pass] = index;

        return &slot;
    }

    GraphicsModule::PassTransforms* GraphicsModule::acquirePassSlot(Ogre::Pass* pass)
    {
        thread_local std::unordered_map<Ogre::Pass*, PassTransforms*> tlsCache;

        auto cacheIt = tlsCache.find(pass);
        if (cacheIt != tlsCache.end())
        {
            PassTransforms* slot = cacheIt->second;
            if (slot->pass.load(std::memory_order_acquire) == pass)
            {
                return slot;
            }
            tlsCache.erase(cacheIt);
        }

        PassTransforms* slot = this->resolvePassSlotLocked(pass);
        tlsCache[pass] = slot;
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

    void GraphicsModule::addTrackedOldBone(Ogre::v1::OldBone* oldBone)
    {
        this->acquireOldBoneSlot(oldBone);
    }

    void GraphicsModule::removeTrackedOldBone(Ogre::v1::OldBone* oldBone)
    {
        std::lock_guard<std::mutex> lock(this->oldBoneRegistrationMutex);

        auto it = this->oldBoneToIndexMap.find(oldBone);
        if (it == this->oldBoneToIndexMap.end())
        {
            return;
        }

        size_t index = it->second;
        OldBoneTransforms& slot = this->oldBonePool[index];

        slot.oldBone.store(nullptr, std::memory_order_release);
        slot.active.store(false, std::memory_order_relaxed);

        this->oldBoneToIndexMap.erase(it);
        this->freeOldBoneSlots.push_back(index);
    }

    void GraphicsModule::updateOldBonePosition(Ogre::v1::OldBone* oldBone, const Ogre::Vector3& position)
    {
        // Always interpolated - call setOldBonePosition() instead for an instant warp.
        GraphicsModule::OldBoneTransforms* oldBoneTransforms = this->acquireOldBoneSlot(oldBone);

        oldBoneTransforms->transforms[this->currentTransformOldBoneIdx].position = position;
        oldBoneTransforms->active.store(true, std::memory_order_relaxed);
    }

    void GraphicsModule::updateOldBoneOrientation(Ogre::v1::OldBone* oldBone, const Ogre::Quaternion& orientation)
    {
        // Always interpolated - call setOldBoneOrientation() instead for an instant warp.
        GraphicsModule::OldBoneTransforms* oldBoneTransforms = this->acquireOldBoneSlot(oldBone);

        oldBoneTransforms->transforms[this->currentTransformOldBoneIdx].orientation = orientation;
        oldBoneTransforms->active.store(true, std::memory_order_relaxed);
    }

    void GraphicsModule::updateOldBoneTransform(Ogre::v1::OldBone* oldBone, const Ogre::Vector3& position, const Ogre::Quaternion& orientation)
    {
        GraphicsModule::OldBoneTransforms* oldBoneTransforms = this->acquireOldBoneSlot(oldBone);

        oldBoneTransforms->transforms[this->currentTransformOldBoneIdx].position = position;
        oldBoneTransforms->transforms[this->currentTransformOldBoneIdx].orientation = orientation;
        oldBoneTransforms->active.store(true, std::memory_order_relaxed);
    }

    // Instant warp - see the contract comment on setNodePosition() in the header.
    void GraphicsModule::setOldBonePosition(Ogre::v1::OldBone* oldBone, const Ogre::Vector3& position)
    {
        if (true == this->isRenderThread())
        {
            this->setOldBonePositionOnRenderThread(oldBone, position);
        }
        else
        {
            NOWA::GraphicsModule::RenderCommand command = [this, oldBone, position]()
            {
                this->setOldBonePositionOnRenderThread(oldBone, position);
            };
            this->enqueueAndWait(std::move(command), "GraphicsModule::setOldBonePosition");
        }
    }

    void GraphicsModule::setOldBonePositionOnRenderThread(Ogre::v1::OldBone* oldBone, const Ogre::Vector3& position)
    {
        oldBone->setPosition(position);

        /*GraphicsModule::OldBoneTransforms* oldBoneTransforms = this->acquireOldBoneSlot(oldBone);
        for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
        {
            oldBoneTransforms->transforms[i].position = position;
        }

        oldBoneTransforms->active.store(false, std::memory_order_relaxed);*/
    }

    void GraphicsModule::setOldBoneOrientation(Ogre::v1::OldBone* oldBone, const Ogre::Quaternion& orientation)
    {
        if (true == this->isRenderThread())
        {
            this->setOldBoneOrientationOnRenderThread(oldBone, orientation);
        }
        else
        {
            NOWA::GraphicsModule::RenderCommand command = [this, oldBone, orientation]()
            {
                this->setOldBoneOrientationOnRenderThread(oldBone, orientation);
            };
            this->enqueueAndWait(std::move(command), "GraphicsModule::setOldBoneOrientation");
        }
    }

    void GraphicsModule::setOldBoneOrientationOnRenderThread(Ogre::v1::OldBone* oldBone, const Ogre::Quaternion& orientation)
    {
        oldBone->setOrientation(orientation);

        /*GraphicsModule::OldBoneTransforms* oldBoneTransforms = this->acquireOldBoneSlot(oldBone);
        for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
        {
            oldBoneTransforms->transforms[i].orientation = orientation;
        }

        oldBoneTransforms->active.store(false, std::memory_order_relaxed);*/
    }

    void GraphicsModule::setOldBoneTransform(Ogre::v1::OldBone* oldBone, const Ogre::Vector3& position, const Ogre::Quaternion& orientation)
    {
        if (true == this->isRenderThread())
        {
            this->setOldBoneTransformOnRenderThread(oldBone, position, orientation);
        }
        else
        {
            NOWA::GraphicsModule::RenderCommand command = [this, oldBone, position, orientation]()
            {
                this->setOldBoneTransformOnRenderThread(oldBone, position, orientation);
            };
            this->enqueueAndWait(std::move(command), "GraphicsModule::setOldBoneTransform");
        }
    }

    void GraphicsModule::setOldBoneTransformOnRenderThread(Ogre::v1::OldBone* oldBone, const Ogre::Vector3& position, const Ogre::Quaternion& orientation)
    {
        oldBone->setPosition(position);
        oldBone->setOrientation(orientation);

        /*GraphicsModule::OldBoneTransforms* oldBoneTransforms = this->acquireOldBoneSlot(oldBone);
        for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
        {
            oldBoneTransforms->transforms[i].position = position;
            oldBoneTransforms->transforms[i].orientation = orientation;
        }

        oldBoneTransforms->active.store(false, std::memory_order_relaxed);*/
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

    void GraphicsModule::addTrackedPass(Ogre::Pass* pass)
    {
        this->acquirePassSlot(pass);
    }

    void GraphicsModule::removeTrackedPass(Ogre::Pass* pass)
    {
        std::lock_guard<std::mutex> lock(this->passRegistrationMutex);

        auto it = this->passToIndexMap.find(pass);
        if (it == this->passToIndexMap.end())
        {
            return;
        }

        size_t index = it->second;
        PassTransforms& slot = this->passPool[index];

        slot.pass.store(nullptr, std::memory_order_release);
        slot.active.store(false, std::memory_order_relaxed);

        this->passToIndexMap.erase(it);
        this->freePassSlots.push_back(index);
    }

    void GraphicsModule::updatePassSpeedsX(Ogre::Pass* pass, unsigned short index, Ogre::Real speedX)
    {
        GraphicsModule::PassTransforms* passTransform = this->acquirePassSlot(pass);

        passTransform->transforms[this->currentTransformPassIdx].speedsX[index] = speedX;
        passTransform->active.store(true, std::memory_order_relaxed);
    }

    void GraphicsModule::updatePassSpeedsY(Ogre::Pass* pass, unsigned short index, Ogre::Real speedY)
    {
        GraphicsModule::PassTransforms* passTransform = this->acquirePassSlot(pass);

        passTransform->transforms[this->currentTransformPassIdx].speedsY[index] = speedY;
        passTransform->active.store(true, std::memory_order_relaxed);
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

        // Post a removal command so the render thread removes it at the next
        // safe point (processClosureCommands), not immediately from main thread.
        ClosureCommand cmd;
        cmd.uniqueName = uniqueName;
        cmd.isRemoval = true;
        cmd.fireAndForget = false;
        cmd.isUpdate = false;
        bool success = this->closureQueue.enqueue(this->producerToken, std::move(cmd));
        ;

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
            this->processSingleCommand(command, this->currentRenderDt);
            ++processedCount;
        }

        // Log if we hit the limit (might indicate a problem)
        if (processedCount >= maxCommandsPerFrame)
        {
            Ogre::LogManager::getSingleton().logMessage("[GraphicsModule] Warning: Processed maximum closure commands per frame (" + Ogre::StringConverter::toString(maxCommandsPerFrame) + ")", Ogre::LML_NORMAL);
        }
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

    size_t GraphicsModule::getPreviousTransformOldBoneIdx(void) const
    {
        return (this->currentTransformOldBoneIdx + NUM_TRANSFORM_BUFFERS - 1) % NUM_TRANSFORM_BUFFERS;
    }

    size_t GraphicsModule::getPreviousTransformBoneIdx(void) const
    {
        return (this->currentTransformBoneIdx + NUM_TRANSFORM_BUFFERS - 1) % NUM_TRANSFORM_BUFFERS;
    }

    size_t GraphicsModule::getPreviousTransformPassIdx(void) const
    {
        return (this->currentTransformPassIdx + NUM_TRANSFORM_BUFFERS - 1) % NUM_TRANSFORM_BUFFERS;
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
        // OldBone transforms
        // =========================================================================
        {
            size_t prevOldBoneIdx = this->currentTransformOldBoneIdx;
            this->currentTransformOldBoneIdx = (this->currentTransformOldBoneIdx + 1) % NUM_TRANSFORM_BUFFERS;

            for (auto& oldBoneTransform : this->oldBonePool)
            {
                Ogre::v1::OldBone* oldBone = oldBoneTransform.oldBone.load(std::memory_order_acquire);

                if (true == oldBoneTransform.isNew)
                {
                    if (nullptr == oldBone)
                    {
                        continue;
                    }

                    GraphicsModule::TransformData currentTransform;
                    currentTransform.position = oldBone->getPosition();
                    currentTransform.orientation = oldBone->getOrientation();

                    for (size_t b = 0; b < NUM_TRANSFORM_BUFFERS; ++b)
                    {
                        oldBoneTransform.transforms[b] = currentTransform;
                    }

                    oldBoneTransform.isNew = false;
                }
                else if (true == oldBoneTransform.active.load(std::memory_order_relaxed))
                {
                    oldBoneTransform.transforms[this->currentTransformOldBoneIdx] = oldBoneTransform.transforms[prevOldBoneIdx];
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
        // Pass transforms (no eviction)
        // =========================================================================
        {
            size_t prevPassIdx = this->currentTransformPassIdx;
            this->currentTransformPassIdx = (this->currentTransformPassIdx + 1) % NUM_TRANSFORM_BUFFERS;

            for (auto& passTransform : this->passPool)
            {
                if (passTransform.isNew)
                {
                    GraphicsModule::PassSpeedData currentData;

                    for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
                    {
                        passTransform.transforms[i] = currentData;
                    }
                    passTransform.isNew = false;
                }
                else if (passTransform.active.load(std::memory_order_relaxed))
                {
                    passTransform.transforms[this->currentTransformPassIdx] = passTransform.transforms[prevPassIdx];
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

        // Update oldBone transforms

        // Get the previous buffer index
        size_t prevOldBoneIdx = this->getPreviousTransformOldBoneIdx();

        // Update all active oldBones
        {
            for (const auto& oldBoneTransform : this->oldBonePool)
            {
                if (true == oldBoneTransform.active.load(std::memory_order_relaxed))
                {
                    Ogre::v1::OldBone* oldBone = oldBoneTransform.oldBone.load(std::memory_order_relaxed);
                    if (nullptr == oldBone)
                    {
                        continue;
                    }

                    // Get previous and current transforms
                    const GraphicsModule::TransformData& prevTransform = oldBoneTransform.transforms[prevOldBoneIdx];
                    const GraphicsModule::TransformData& currTransform = oldBoneTransform.transforms[this->currentTransformOldBoneIdx];

                    // Interpolate position
                    Ogre::Vector3 interpPos = Ogre::Math::lerp(prevTransform.position, currTransform.position, this->interpolationWeight);

                    // Interpolate orientation
                    Ogre::Quaternion interpRot = Ogre::Quaternion::nlerp(this->interpolationWeight, prevTransform.orientation, currTransform.orientation, true);

                    // Apply to scene oldBone
                    oldBone->setOrientation(interpRot);
                    oldBone->setPosition(interpPos);
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

        size_t prevPassIdx = this->getPreviousTransformPassIdx();

        {
            for (const auto& passTransform : this->passPool)
            {
                if (passTransform.active.load(std::memory_order_relaxed))
                {
                    Ogre::Pass* pass = passTransform.pass.load(std::memory_order_relaxed);
                    if (nullptr == pass)
                    {
                        continue;
                    }

                    const GraphicsModule::PassSpeedData& prev = passTransform.transforms[prevPassIdx];
                    const GraphicsModule::PassSpeedData& curr = passTransform.transforms[this->currentTransformPassIdx];

                    Ogre::Real interpX[9];
                    Ogre::Real interpY[9];

                    for (int i = 0; i < 9; ++i)
                    {
                        interpX[i] = Ogre::Math::lerp(prev.speedsX[i], curr.speedsX[i], this->interpolationWeight);
                        interpY[i] = Ogre::Math::lerp(prev.speedsY[i], curr.speedsY[i], this->interpolationWeight);

                        /*Ogre::LogManager::getSingletonPtr()->logMessage(
                            "[Render] Layer " + std::to_string(i) +
                            ": X=" + Ogre::StringConverter::toString(interpX[i]) +
                            " Y=" + Ogre::StringConverter::toString(interpY[i]));*/
                    }

                    pass->getFragmentProgramParameters()->setNamedConstant("speedsX", interpX, 9, 1);
                    pass->getFragmentProgramParameters()->setNamedConstant("speedsY", interpY, 9, 1);
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
        // if (level >= this->logLevel)

        if (level == Ogre::LML_CRITICAL)
        {
            std::stringstream ss;
            ss << "[RenderCommandQueue] " << message << " (Thread: " << (this->isRenderThread() ? "RENDER" : "MAIN") << ", Depth: " << g_renderCommandDepth << ", WaitDepth: " << g_waitDepth << ")";

            Ogre::LogManager::getSingletonPtr()->logMessage(level, ss.str());
        }
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
        // Engine shutting down: bypass ring-buffer, execute immediately with wait.
        if (false == this->bRunning)
        {
            this->enqueueAndWait(std::move(destroyCommand), commandName);
            return;
        }

        // A wait closure is in flight on the render thread. We cannot push into
        // destroySlots right now because the render thread owns the slot state
        // during command execution. Defer until enqueueAndWait() finishes, then
        // flushDeferredDestroyCommands() will move them into the ring-buffer safely.
        if (true == this->isRunningWaitClosure)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL,
                "[GraphicsModule] Deferring destroy '" + Ogre::String(commandName) + "' (wait closure in flight, " + Ogre::StringConverter::toString(this->deferredDestroyCommands.size() + 1) + " total deferred)");

            this->deferredDestroyCommands.emplace_back(std::move(destroyCommand), commandName);
            return;
        }

        this->destroySlots[this->currentDestroySlot].emplace_back(std::move(destroyCommand));
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "Enqueue destroy command: " + Ogre::String(commandName));
    }

    void GraphicsModule::advanceFrameAndDestroyOld(void)
    {
        // this->isRunningDestroyClosure = true;

        // Slot to destroy is two frames behind
        const size_t destroySlot = (this->currentDestroySlot + 1) % GraphicsModule::NUM_DESTROY_SLOTS;

        for (auto& destroyCommand : this->destroySlots[destroySlot])
        {
            // Destroy safely (now guaranteed not in use)
            destroyCommand();
        }
        this->destroySlots[destroySlot].clear();

        // Advance to next logic slot
        this->currentDestroySlot = (this->currentDestroySlot + 1) % GraphicsModule::NUM_DESTROY_SLOTS;

        // this->isRunningDestroyClosure = false;
    }

    bool GraphicsModule::hasPendingDestroyCommands(void) const
    {
        return false == this->destroySlots[this->currentDestroySlot].empty();
    }

}; // namespace end