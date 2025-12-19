#include "NOWAPrecompiled.h"
#include "GraphicsModule.h"
#include "main/Core.h"
#include "main/AppStateManager.h"
#include "main/InputDeviceCore.h"

#include <sstream>

namespace
{
    std::chrono::milliseconds g_defaultTimeout(5000); // 5 seconds
}

namespace NOWA
{
    using namespace RenderGlobals;

    GraphicsModule::GraphicsModule()
        : bRunning(false),
        timeoutEnabled(false),
        timeoutDuration(g_defaultTimeout.count()),
        logLevel(Ogre::LML_NORMAL),
        currentTransformNodeIdx(0),
        currentTransformCameraIdx(0),
        currentTransformOldBoneIdx(0),
        currentTransformPassIdx(0),
        currentTrackedDatablockIdx(0),
        interpolationWeight(0.0f),
        accumTimeSinceLastLogicFrame(0.0f),
        frameTime(1.0f / 60.0f),
        debugVisualization(false),
        currentDestroySlot(0),
        isRunningWaitClosure(false),
        closureQueue(),
        producerToken(closureQueue),
        consumerToken(closureQueue)
    {
        // Reserve some space for tracked nodes to avoid frequent reallocations
        this->trackedNodes.reserve(100);

        this->queueInitialized.store(true);
    }

    GraphicsModule::~GraphicsModule()
    {
        
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
        this->markCurrentThreadAsRenderThread();

        this->setTimeoutDuration(std::chrono::milliseconds(10000));
        this->setFrameTime(NOWA::Core::getSingletonPtr()->getOptionDesiredSimulationUpdates());

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
            Ogre::WindowEventUtilities::messagePump();

            Ogre::uint64 currentTime = timer.getMicroseconds();
            Ogre::Real deltaTime = (currentTime - lastFrameTime) * 0.000001f;
            lastFrameTime = currentTime;

            GameProgressModule* gameProgressModule = appStateManager->getActiveGameProgressModuleSafe();
            const bool isStalled = appStateManager->bStall.load();
            const bool isSceneLoading = (gameProgressModule != nullptr) ? gameProgressModule->bSceneLoading.load() : false;

            if (!isStalled && !isSceneLoading)
            {
                NOWA::InputDeviceCore::getSingletonPtr()->capture(deltaTime);
            }

            if (!isStalled && !isSceneLoading)
            {
                this->advanceFrameAndDestroyOld();
                this->processAllCommands();

                // >>> FIX: use alpha published by logic thread (single truth)
                const float alpha = this->consumeInterpolationAlpha();
                this->setInterpolationWeight(alpha); // implement/set your internal weight variable

                // No more: accumTimeSinceLastLogicFrame += deltaTime
                // No more: calculateInterpolationWeight() based on render delta

                this->updateAllTransforms();

                if (++frameCount % 300 == 0)
                {
                    this->waitForRenderCompletion();
                    this->dumpBufferState();
                    frameCount = 0;
                }
            }

            if (!isStalled)
            {
                Ogre::Root::getSingletonPtr()->renderOneFrame();
            }
        }

        while (this->hasPendingRenderCommands())
        {
            this->processAllCommands();
        }

        for (size_t i = 0; i < NOWA::GraphicsModule::NUM_DESTROY_SLOTS; ++i)
        {
            this->advanceFrameAndDestroyOld();
        }

        const int i = this->queue.size_approx();
        if (i > 0)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                "[RenderCommandQueueModule]: Illegal state, as there are still: " +
                Ogre::StringConverter::toString(i) + " pending commands!");
            throw;
        }

        this->trackedNodes.clear();
        this->trackedCameras.clear();
        this->trackedOldBones.clear();
        this->trackedPasses.clear();
        this->trackedDatablocks.clear();

		this->bRunning = false;
		this->timeoutEnabled = false;
		this->timeoutDuration = g_defaultTimeout.count();
		this->logLevel = Ogre::LML_NORMAL;
		this->currentTransformNodeIdx = 0;
		this->currentTransformCameraIdx = 0;
        this->currentTransformOldBoneIdx = 0;
        this->currentTransformPassIdx = 0;
        this->currentTrackedDatablockIdx = 0;

		this->accumTimeSinceLastLogicFrame = 0.0f;

		this->frameTime = 1.0f / 60.0f;
		this->debugVisualization = false;
		this->currentDestroySlot = 0;
		this->isRunningWaitClosure = false;
    }

    void GraphicsModule::publishInterpolationAlpha(float alpha)
    {
        // Clamp hard: we never want NaNs or >1 to leak into interpolation
        if (!(alpha == alpha)) // NaN check
            alpha = 0.0f;

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

    void GraphicsModule::clearAllClosures(void)
    {
        // Clear the concurrent queue - drain all pending commands
        ClosureCommand command;
        size_t clearedCommands = 0;
        while (closureQueue.try_dequeue(consumerToken, command))
        {
            ++clearedCommands;
        }

        // Clear all persistent closures
        persistentClosures.clear();

        // Log the cleanup for debugging
        if (clearedCommands > 0)
        {
            Ogre::LogManager::getSingleton().logMessage("[GraphicsModule] Cleared " + Ogre::StringConverter::toString(clearedCommands) + " queued closure commands and " + Ogre::StringConverter::toString(persistentClosures.size()) +
                " persistent closures", Ogre::LML_NORMAL);
        }
    }

    void GraphicsModule::clearSceneResources(void)
    {
      this->trackedCameras.clear();
      this->cameraToIndexMap.clear();

      this->trackedNodes.clear();
      this->nodeToIndexMap.clear();

      this->oldBoneToIndexMap.clear();
      this->trackedOldBones.clear();

      this->passToIndexMap.clear();
      this->trackedPasses.clear();

      this->datablockToIndexMap.clear();
      this->trackedDatablocks.clear();

      this->currentTransformNodeIdx = 0;
      this->currentTransformCameraIdx = 0;
      this->currentTransformOldBoneIdx = 0;
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
        // Make sure we have a valid command
        if (!command)
        {
            // If we have a promise, fulfill it to prevent deadlock
            if (promise && false == this->wasPromiseFulfilled(promise))
            {
                try
                {
                    promise->set_value();
                    this->markPromiseFulfilled(promise);
                }
                catch (...)
                {
                    // Ignore any exceptions from setting the value
                }
            }
            return;
        }

        // If we're already on the render thread AND processing a command, 
        // execute this command immediately rather than enqueuing it

        // Note: RenderGlobals::g_inLogicCommand has been uncommented, because its dangerous, so that a render thread operation would not be executed on render thread, because its part of logic main thread cascade
        if (true == this->isRenderThread()/* || true == RenderGlobals::g_inLogicCommand*/)
        {
            // Log direct execution
            this->logCommandEvent("Executing command directly on render thread", Ogre::LML_TRIVIAL);

            try
            {
                this->logCommandEvent(std::string("Executing command '") + commandName + "' directly on render thread", Ogre::LML_TRIVIAL);
                command();
                // this->isRunningWaitClosure = false;
                if (promise && !wasPromiseFulfilled(promise))
                {
                    promise->set_value();
                    this->markPromiseFulfilled(promise);
                }
            }
            catch (const std::exception& e)
            {
                this->logCommandEvent(std::string("Exception caught in direct command execution: ") + e.what(), Ogre::LML_CRITICAL);
                if (promise && !wasPromiseFulfilled(promise))
                {
                    try
                    {
                        promise->set_exception(std::current_exception());
                        this->markPromiseFulfilled(promise);
                    }
                    catch (...) {
                        // Ignore any exceptions from setting the exception
                    }
                }
                // We don't rethrow here - we contain the exception within the command
            }
            catch (...)
            {
                this->logCommandEvent("Unknown exception caught in direct command execution", Ogre::LML_CRITICAL);
                if (promise && !wasPromiseFulfilled(promise))
                {
                    try
                    {
                        promise->set_exception(std::current_exception());
                        this->markPromiseFulfilled(promise);
                    }
                    catch (...) {
                        // Ignore any exceptions from setting the exception
                    }
                }
                // We don't rethrow here - we contain the exception within the command
            }
        }
        else
        {
            // Normal behavior - enqueue for later execution
            CommandEntry entry;
            entry.command = std::move(command);
            entry.completionPromise = promise;
            entry.promiseAlreadyFulfilled = false;

            this->queue.enqueue(std::move(entry));

            // Log enqueuing
            this->logCommandEvent("Command " + Ogre::String(commandName) + " enqueued for later execution queue size : " + Ogre::StringConverter::toString(this->queue.size_approx()), Ogre::LML_TRIVIAL);
        }
    }

    void GraphicsModule::processAllCommands(void)
    {
        // Safety check that we're on the render thread
        if (false == this->isRenderThread())
        {
            this->logCommandEvent("processAllCommands called from non-render thread!", Ogre::LML_CRITICAL);
            return;
        }

        // Increment render command depth to detect nested command executions
        g_renderCommandDepth++;

        // Process all commands in the queue
        CommandEntry entry;
        bool processedAnyCommands = false;

        while (this->pop(entry))
        {
            processedAnyCommands = true;
            try
            {
                // Execute the command (which may enqueue more commands if inside a render command)
                if (entry.command)
                {
                    int i = this->queue.size_approx();
                    this->logCommandEvent("Executing command from processAllCommands. New queue size: " + Ogre::StringConverter::toString(this->queue.size_approx()), Ogre::LML_TRIVIAL);
                    entry.command();

                    this->isRunningWaitClosure = false;
                }

                // Set the promise value if it exists and hasn't been fulfilled yet
                if (entry.completionPromise && false == this->wasPromiseFulfilled(entry.completionPromise))
                {
                    entry.completionPromise->set_value();
                    this->markPromiseFulfilled(entry.completionPromise);
                }
            }
            catch (const std::exception& e)
            {
                this->logCommandEvent(std::string("Exception in processAllCommands: ") + e.what(), Ogre::LML_CRITICAL);

                // Set exception on the promise if it exists
                if (entry.completionPromise && false == this->wasPromiseFulfilled(entry.completionPromise))
                {
                    try
                    {
                        entry.completionPromise->set_exception(std::current_exception());
                        this->markPromiseFulfilled(entry.completionPromise);
                    }
                    catch (...) { /* Ignore exceptions from setting the exception */ }
                }
            }
            catch (...)
            {
                this->logCommandEvent("Unknown exception in processAllCommands", Ogre::LML_CRITICAL);

                // Set exception on the promise if it exists
                if (entry.completionPromise && false == this->wasPromiseFulfilled(entry.completionPromise))
                {
                    try
                    {
                        entry.completionPromise->set_exception(std::current_exception());
                        this->markPromiseFulfilled(entry.completionPromise);
                    }
                    catch (...) { /* Ignore exceptions from setting the exception */ }
                }
            }
        }

        // Cleanup fulfilled promises occasionally (when we've processed at least one command)
        if (true == processedAnyCommands)
        {
            this->cleanupFulfilledPromises();
        }

        // Decrement render command depth
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
        this->enqueue([this]()
        {
            // This will execute in the render thread and process any commands ahead of it
                this->logCommandEvent("Processing queue from waitForRenderCompletion", Ogre::LML_NORMAL);
        }, "", promise);

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

    void GraphicsModule::cleanupFulfilledPromises()
    {
        // Only clean up if we have a significant number of fulfilled promises stored
        if (this->fulfilledPromises.size() > 1000)
        {
            std::lock_guard<std::mutex> lock(this->fulfilledMutex);

            std::stringstream ss;
            ss << "Cleaning up " << fulfilledPromises.size() << " fulfilled promises";
            this->logCommandEvent(ss.str(), Ogre::LML_NORMAL);

            this->fulfilledPromises.clear();
        }
    }

#if 0
    void GraphicsModule::enqueueAndWait(RenderCommand&& command, const char* commandName)
    {
        this->isRunningWaitClosure = true;

        // If already on the render thread or in the middle of destroy closure, wait must not be called! Else closure of destroy etc. will be interupted! so execute directly without wait in that case.
        // Or detect if already in a logic->render call chain
        // Note: RenderGlobals::g_inLogicCommand has been uncommented, because its dangerous, so that a render thread operation would not be executed on render thread, because its part of logic main thread cascade
        if (true == this->isRenderThread()/* || true == RenderGlobals::g_inLogicCommand*/)
        {
            try
            {
                // Execute immediately on current thread to break cycle
                this->logCommandEvent(std::string("Executing command '") + commandName + "' directly on render thread with **WAIT**", Ogre::LML_TRIVIAL);
                command();

                this->isRunningWaitClosure = false;
                return;
            }
            catch (const std::exception& e)
            {
                this->logCommandEvent(std::string("Exception in direct execution of '") + commandName + "': " + e.what(), Ogre::LML_CRITICAL);
                throw; // Re-throw the exception
            }
            catch (...)
            {
                this->logCommandEvent(std::string("Unknown exception in direct execution of '") + commandName + "'", Ogre::LML_CRITICAL);
                throw; // Re-throw the exception
            }
        }

        // Track that we're in a waiting state on this thread
        this->incrementWaitDepth();

        try
        {
            // Create a promise for this command
            auto promise = std::make_shared<std::promise<void>>();
            auto future = promise->get_future();

            // Log the command
            this->logCommandEvent(std::string("Enqueueing command '") + commandName + "' with **WAIT**", Ogre::LML_NORMAL);

            // If we're in a nested wait, we still enqueue with the promise but we'll use a special flag
            bool isNested = isInNestedWait() && g_waitDepth > 1; // We've already incremented waitDepth

            if (true == isNested)
            {
                this->logCommandEvent(std::string("Command '") + commandName + "' is a nested **WAIT** call (depth: " + std::to_string(g_waitDepth) + ")", Ogre::LML_NORMAL);
            }

            // Always enqueue with a promise, even for nested commands
            this->enqueue(std::move(command), commandName, promise);

            // For the first level wait, process immediately to avoid deadlocks
            if (false == isNested)
            {
                // Wait for the command to complete
                if (true == this->isTimeoutEnabled())
                {
                    auto timeout = getTimeoutDuration();
                    auto status = future.wait_for(timeout);

                    if (status == std::future_status::timeout)
                    {
                        this->logCommandEvent(std::string("Timeout occurred waiting for command '") + commandName + "'", Ogre::LML_CRITICAL);

                        // Force a full queue synchronization to recover
                        this->processQueueSync();
                    }
                }
                else
                {
                    // future.wait();
                    while (future.wait_for(std::chrono::milliseconds(1)) != std::future_status::ready)
                    {
                        std::this_thread::yield();
                    }

                }
            }
            else
            {
                // For nested waits, we still track the promise but don't wait for it
                // The outer wait will ensure all commands get processed eventually
                this->logCommandEvent(std::string("Nested command '") + commandName + "' enqueued without blocking wait", Ogre::LML_NORMAL);

                // Add to tracking for nested commands if needed
                // trackNestedCommand(promise); // Optional: implement this to track nested promises
            }
        }
        catch (const std::exception& e)
        {
            this->logCommandEvent(std::string("Exception waiting for command '") + commandName + "': " + e.what(), Ogre::LML_CRITICAL);
            decrementWaitDepth();
            throw;
        }
        catch (...)
        {
            this->logCommandEvent(std::string("Unknown exception waiting for command '") + commandName + "'", Ogre::LML_CRITICAL);
            decrementWaitDepth();
            throw;
        }

        decrementWaitDepth();
        logCommandEvent(std::string("Command '") + commandName + "' completed", Ogre::LML_TRIVIAL);

        this->isRunningWaitClosure = false;
    }
#else
    void GraphicsModule::enqueueAndWait(RenderCommand&& command, const char* commandName)
    {
        this->isRunningWaitClosure = true;

        // If already on the render thread or in the middle of destroy closure, wait must not be called! Else closure of destroy etc. will be interupted! so execute directly without wait in that case.
        // Or detect if already in a logic->render call chain
        // Note: RenderGlobals::g_inLogicCommand has been uncommented, because its dangerous, so that a render thread operation would not be executed on render thread, because its part of logic main thread cascade
        if (true == this->isRenderThread() && RenderGlobals::g_renderCommandDepth > 0 /*|| true == RenderGlobals::g_inLogicCommand*/)
        {
            try
            {
                // Execute immediately on current thread to break cycle
                this->logCommandEvent(std::string("Executing command '") + commandName + "' directly on render thread with **WAIT** (re-entrant)", Ogre::LML_TRIVIAL);
                command();

                this->isRunningWaitClosure = false;
                return;
            }
            catch (const std::exception& e)
            {
                this->logCommandEvent(std::string("Exception in direct execution of '") + commandName + "': " + e.what(), Ogre::LML_CRITICAL);
                this->isRunningWaitClosure = false;
                throw; // Re-throw the exception
            }
            catch (...)
            {
                this->logCommandEvent(std::string("Unknown exception in direct execution of '") + commandName + "'", Ogre::LML_CRITICAL);
                this->isRunningWaitClosure = false;
                throw; // Re-throw the exception
            }
        }

        // Track that we're in a waiting state on this thread
        this->incrementWaitDepth();

        try
        {
            // Create a promise for this command
            auto promise = std::make_shared<std::promise<void>>();
            auto future = promise->get_future();

            // Log the command
            this->logCommandEvent(std::string("Enqueueing command '") + commandName + "' with **WAIT**", Ogre::LML_NORMAL);

            // If we're in a nested wait, we still enqueue with the promise but we'll use a special flag
            bool isNested = isInNestedWait() && g_waitDepth > 1; // We've already incremented waitDepth

            if (true == isNested)
            {
                this->logCommandEvent(std::string("Command '") + commandName + "' is a nested **WAIT** call (depth: " + std::to_string(g_waitDepth) + ")", Ogre::LML_NORMAL);
            }

            // Always enqueue with a promise, even for nested commands
            this->enqueue(std::move(command), commandName, promise);

            // For the first level wait, process immediately to avoid deadlocks
            if (false == isNested)
            {
                // Wait for the command to complete
                if (true == this->isTimeoutEnabled())
                {
                    auto timeout = getTimeoutDuration();
                    auto status = future.wait_for(timeout);

                    if (status == std::future_status::timeout)
                    {
                        this->logCommandEvent(std::string("Timeout occurred waiting for command '") + commandName + "'", Ogre::LML_CRITICAL);

                        // Force a full queue synchronization to recover
                        this->processQueueSync();
                    }
                }
                else
                {
                    // future.wait();
                    while (future.wait_for(std::chrono::milliseconds(1)) != std::future_status::ready)
                    {
                        std::this_thread::yield();
                    }
                }
            }
            else
            {
                // For nested waits, we still track the promise but don't wait for it
                // The outer wait will ensure all commands get processed eventually
                this->logCommandEvent(std::string("Nested command '") + commandName + "' enqueued without blocking wait", Ogre::LML_NORMAL);

                // Add to tracking for nested commands if needed
                // trackNestedCommand(promise); // Optional: implement this to track nested promises
            }
        }
        catch (const std::exception& e)
        {
            this->logCommandEvent(std::string("Exception waiting for command '") + commandName + "': " + e.what(), Ogre::LML_CRITICAL);
            decrementWaitDepth();
            this->isRunningWaitClosure = false;
            throw;
        }
        catch (...)
        {
            this->logCommandEvent(std::string("Unknown exception waiting for command '") + commandName + "'", Ogre::LML_CRITICAL);
            decrementWaitDepth();
            this->isRunningWaitClosure = false;
            throw;
        }

        decrementWaitDepth();
        logCommandEvent(std::string("Command '") + commandName + "' completed", Ogre::LML_TRIVIAL);

        this->isRunningWaitClosure = false;
    }

#endif

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
        enqueue([this]() 
        {
            // This will execute after all previous commands
            this->logCommandEvent("Sync point reached in queue processing", Ogre::LML_NORMAL);
        }, "", syncPromise);

        // Wait without timeout - we need to ensure this completes
        this->logCommandEvent("Waiting for queue sync point", Ogre::LML_NORMAL);
        syncFuture.wait();
        this->logCommandEvent("Queue sync point completed", Ogre::LML_NORMAL);
    }

    void GraphicsModule::markPromiseFulfilled(const std::shared_ptr<std::promise<void>>& promise)
    {
        if (!promise) return;

        std::lock_guard<std::mutex> lock(this->fulfilledMutex);
        this->fulfilledPromises.insert(promise);
    }

    bool GraphicsModule::wasPromiseFulfilled(const std::shared_ptr<std::promise<void>>& promise)
    {
        if (!promise) return true;

        std::lock_guard<std::mutex> lock(this->fulfilledMutex);
        return fulfilledPromises.find(promise) != this->fulfilledPromises.end();
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

        // Clear all waiting depths that might be preventing command processing
        // This is a last resort recovery mechanism
        std::lock_guard<std::mutex> lock(mutex);

        // Log the number of commands still in the queue
        std::stringstream ss;
        ss << "Queue has " << this->queue.size_approx() << " pending commands during timeout recovery";
        this->logCommandEvent(ss.str(), Ogre::LML_CRITICAL);

        // Reset wait depth if it's non-zero (something might have gone wrong)
        if (g_waitDepth > 0)
        {
            std::stringstream ss;
            ss << "Resetting wait depth from " << g_waitDepth << " to 0 during recovery";
            this->logCommandEvent(ss.str(), Ogre::LML_CRITICAL);
            g_waitDepth = 0;
        }

        // Optionally: process commands here if we're on the render thread
        if (true == this->isRenderThread())
        {
            this->logCommandEvent("Processing command queue during timeout recovery", Ogre::LML_CRITICAL);
            // Release the lock before processing
            lock.~lock_guard();
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

    GraphicsModule::NodeTransforms* GraphicsModule::findNodeTransforms(Ogre::Node* node)
    {
        // O(1) lookup using the hash map
        auto it = this->nodeToIndexMap.find(node);
        if (it != this->nodeToIndexMap.end() && false == this->trackedNodes.empty())
        {
            // Return pointer to the NodeTransforms in the vector
            return &this->trackedNodes[it->second];
        }
        return nullptr;
    }

    GraphicsModule::CameraTransforms* GraphicsModule::findCameraTransforms(Ogre::Camera* camera)
    {
        // O(1) lookup using the hash map
        auto it = this->cameraToIndexMap.find(camera);
        if (it != this->cameraToIndexMap.end() && false == this->trackedCameras.empty())
        {
            // Return pointer to the CameraTransforms in the vector
            return &this->trackedCameras[it->second];
        }
        return nullptr;
    }

    GraphicsModule::OldBoneTransforms* GraphicsModule::findOldBoneTransforms(Ogre::v1::OldBone* oldBone)
    {
        // O(1) lookup using the hash map
        auto it = this->oldBoneToIndexMap.find(oldBone);
        if (it != this->oldBoneToIndexMap.end() && false == this->trackedOldBones.empty())
        {
            // Return pointer to the OldBoneTransforms in the vector
            return &this->trackedOldBones[it->second];
        }
        return nullptr;
    }

    GraphicsModule::PassTransforms* GraphicsModule::findPassTransforms(Ogre::Pass* pass)
    {
        auto it = this->passToIndexMap.find(pass);
        if (it != this->passToIndexMap.end() && false == this->trackedPasses.empty())
        {
            return &this->trackedPasses[it->second];
        }
        return nullptr;
    }

    GraphicsModule::TrackedDatablock* GraphicsModule::findTrackedDatablock(Ogre::HlmsDatablock* datablock)
    {
        // O(1) lookup using the hash map
        auto it = this->datablockToIndexMap.find(datablock);
        if (it != this->datablockToIndexMap.end() && false == this->trackedDatablocks.empty())
        {
            // Return pointer to the tracked datablock in the vector
            return &this->trackedDatablocks[it->second];
        }
        return nullptr;
    }

    void GraphicsModule::addTrackedNode(Ogre::Node* node)
    {
        // Check if node is already tracked
        if (this->findNodeTransforms(node))
        {
            return;
        }

        // Create a new node transform record
        GraphicsModule::NodeTransforms newNodeTransform;
        newNodeTransform.node = node;
        newNodeTransform.active = true;
        newNodeTransform.isNew = true;
        newNodeTransform.useDerived = false;

        // Initialize all buffers with the current node transform
        for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
        {
            newNodeTransform.transforms[i].position = node->getPosition();
            newNodeTransform.transforms[i].orientation = node->getOrientation();
            newNodeTransform.transforms[i].scale = node->getScale();
        }

        // Add to tracked nodes vector
        size_t newIndex = this->trackedNodes.size();
        this->trackedNodes.push_back(std::move(newNodeTransform));

        // Add to hash map for fast lookups
        this->nodeToIndexMap[node] = newIndex;

        if (true == this->debugVisualization)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[RenderCommandQueueModule]: Added tracked node: " + node->getName());
        }
    }

    void GraphicsModule::removeTrackedNode(Ogre::Node* node)
    {
        auto it = this->nodeToIndexMap.find(node);
        if (it != this->nodeToIndexMap.end())
        {
            size_t indexToRemove = it->second;
            size_t lastIndex = this->trackedNodes.size() - 1;

            if (true == this->debugVisualization)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[RenderCommandQueueModule]: Removed tracked node: " + node->getName());
            }

            // If this is not the last element, move the last element to this position
            if (indexToRemove != lastIndex)
            {
                // Move the last element to the position of the removed element
                this->trackedNodes[indexToRemove] = std::move(this->trackedNodes[lastIndex]);

                // Update the index in the hash map for the moved element
                this->nodeToIndexMap[this->trackedNodes[indexToRemove].node] = indexToRemove;
            }

            // Remove the last element (now duplicated or the one we want to remove)
            this->trackedNodes.pop_back();

            // Remove from the hash map
            this->nodeToIndexMap.erase(it);
        }
    }

    void GraphicsModule::updateNodePosition(Ogre::Node* node, const Ogre::Vector3& position, bool useDerived)
    {
        GraphicsModule::NodeTransforms* nodeTransforms = this->findNodeTransforms(node);

        // If node isn't tracked yet, add it
        if (nullptr == nodeTransforms)
        {
            this->addTrackedNode(node);
            nodeTransforms = this->findNodeTransforms(node);
        }

        // Update position in current buffer only
        // This is key to the mutex-free approach - logic thread only writes to current buffer
        nodeTransforms->transforms[this->currentTransformNodeIdx].position = position;
        nodeTransforms->active = true;
        nodeTransforms->useDerived = useDerived;
        nodeTransforms->updatedThisFrame = true;

        if (true == this->debugVisualization)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[RenderCommandQueueModule]: Updated position for node: " + node->getName()
             + " to " + Ogre::StringConverter::toString(position) + " in buffer: " + Ogre::StringConverter::toString(this->currentTransformNodeIdx));
        }
    }

    void GraphicsModule::updateNodeOrientation(Ogre::Node* node, const Ogre::Quaternion& orientation, bool useDerived)
    {
        GraphicsModule::NodeTransforms* nodeTransforms = this->findNodeTransforms(node);

        // If node isn't tracked yet, add it
        if (nullptr == nodeTransforms)
        {
            this->addTrackedNode(node);
            nodeTransforms = this->findNodeTransforms(node);
        }

        // Update position in current buffer only
        // This is key to the mutex-free approach - logic thread only writes to current buffer
        nodeTransforms->transforms[this->currentTransformNodeIdx].orientation = orientation;
        nodeTransforms->active = true;
        nodeTransforms->useDerived = useDerived;
        nodeTransforms->updatedThisFrame = true;

        if (true == this->debugVisualization)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[RenderCommandQueueModule]: Updated orientation for node: " + node->getName()
                + " to " + Ogre::StringConverter::toString(orientation) + " in buffer: " + Ogre::StringConverter::toString(this->currentTransformNodeIdx));
        }
    }

    void GraphicsModule::updateNodeScale(Ogre::Node* node, const Ogre::Vector3& scale, bool useDerived)
    {
        GraphicsModule::NodeTransforms* nodeTransforms = this->findNodeTransforms(node);

        // If node isn't tracked yet, add it
        if (nullptr == nodeTransforms)
        {
            this->addTrackedNode(node);
            nodeTransforms = this->findNodeTransforms(node);
        }

        // Update position in current buffer only
        // This is key to the mutex-free approach - logic thread only writes to current buffer
        nodeTransforms->transforms[this->currentTransformNodeIdx].scale = scale;
        nodeTransforms->active = true;
        nodeTransforms->useDerived = useDerived;
        nodeTransforms->updatedThisFrame = true;

        if (true == this->debugVisualization)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[RenderCommandQueueModule]: Updated scale for node: " + node->getName()
                + " to " + Ogre::StringConverter::toString(scale) + " in buffer: " + Ogre::StringConverter::toString(this->currentTransformNodeIdx));
        }
    }

    void GraphicsModule::updateNodeTransform(Ogre::Node* node, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, const Ogre::Vector3& scale, bool useDerived)
    {
        GraphicsModule::NodeTransforms* nodeTransforms = this->findNodeTransforms(node);

        // If node isn't tracked yet, add it
        if (nullptr == nodeTransforms)
        {
            this->addTrackedNode(node);
            nodeTransforms = this->findNodeTransforms(node);
        }

        // Update all transform components in current buffer
        nodeTransforms->transforms[this->currentTransformNodeIdx].position = position;
        nodeTransforms->transforms[this->currentTransformNodeIdx].orientation = orientation;
        nodeTransforms->transforms[this->currentTransformNodeIdx].scale = scale;
        nodeTransforms->active = true;
        nodeTransforms->useDerived = useDerived;
        nodeTransforms->updatedThisFrame = true;
    }

    void GraphicsModule::addTrackedCamera(Ogre::Camera* camera)
    {
        // Check if camera is already tracked
        if (this->findCameraTransforms(camera))
        {
            return;
        }

        // Create a new camera transform record
        GraphicsModule::CameraTransforms newCameraTransform;
        newCameraTransform.camera = camera;
        newCameraTransform.active = true;
        newCameraTransform.isNew = true;

        // Initialize all buffers with the current camera transform
        for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
        {
            newCameraTransform.transforms[i].position = camera->getPosition();
            newCameraTransform.transforms[i].orientation = camera->getOrientation();
        }

        // Add to tracked cameras vector
        size_t newIndex = this->trackedCameras.size();
        this->trackedCameras.push_back(std::move(newCameraTransform));

        // Add to hash map for fast lookups
        this->cameraToIndexMap[camera] = newIndex;
    }

    void GraphicsModule::removeTrackedCamera(Ogre::Camera* camera)
    {
        if (this->trackedCameras.empty())
        {
            return;
        }

        auto it = this->cameraToIndexMap.find(camera);
        if (it != this->cameraToIndexMap.end())
        {
            size_t indexToRemove = it->second;
            size_t lastIndex = this->trackedCameras.size() - 1;

            // If this is not the last element, move the last element to this position
            if (indexToRemove != lastIndex)
            {
                // Move the last element to the position of the removed element
                this->trackedCameras[indexToRemove] = std::move(this->trackedCameras[lastIndex]);

                // Update the index in the hash map for the moved element
                this->cameraToIndexMap[this->trackedCameras[indexToRemove].camera] = indexToRemove;
            }

            // Remove the last element (now duplicated or the one we want to remove)
            this->trackedCameras.pop_back();

            // Remove from the hash map
            this->cameraToIndexMap.erase(it);
        }
    }

    void GraphicsModule::updateCameraPosition(Ogre::Camera* camera, const Ogre::Vector3& position)
    {
        GraphicsModule::CameraTransforms* cameraTransforms = this->findCameraTransforms(camera);

        // If camera isn't tracked yet, add it
        if (nullptr == cameraTransforms)
        {
            this->addTrackedCamera(camera);
            cameraTransforms = this->findCameraTransforms(camera);
        }

        // Update position in current buffer only
        // This is key to the mutex-free approach - logic thread only writes to current buffer
        cameraTransforms->transforms[this->currentTransformCameraIdx].position = position;
        cameraTransforms->active = true;
    }

    void GraphicsModule::updateCameraOrientation(Ogre::Camera* camera, const Ogre::Quaternion& orientation)
    {
        GraphicsModule::CameraTransforms* cameraTransforms = this->findCameraTransforms(camera);

        // If camera isn't tracked yet, add it
        if (nullptr == cameraTransforms)
        {
            this->addTrackedCamera(camera);
            cameraTransforms = this->findCameraTransforms(camera);
        }

        // Update orientation in current buffer only
        // This is key to the mutex-free approach - logic thread only writes to current buffer
        cameraTransforms->transforms[this->currentTransformCameraIdx].orientation = orientation;
        cameraTransforms->active = true;
    }

    void GraphicsModule::updateCameraTransform(Ogre::Camera* camera, const Ogre::Vector3& position, const Ogre::Quaternion& orientation)
    {
        GraphicsModule::CameraTransforms* cameraTransforms = this->findCameraTransforms(camera);

        // If camera isn't tracked yet, add it
        if (nullptr == cameraTransforms)
        {
            this->addTrackedCamera(camera);
            cameraTransforms = this->findCameraTransforms(camera);
        }

        // Update position in current buffer only
        // This is key to the mutex-free approach - logic thread only writes to current buffer
        cameraTransforms->transforms[this->currentTransformCameraIdx].position = position;
        cameraTransforms->transforms[this->currentTransformCameraIdx].orientation = orientation;
        cameraTransforms->active = true;
    }

    void GraphicsModule::addTrackedOldBone(Ogre::v1::OldBone* oldBone)
    {
        // Check if oldBone is already tracked
        if (this->findOldBoneTransforms(oldBone))
        {
            return;
        }

        // Create a new oldBone transform record
        GraphicsModule::OldBoneTransforms newOldBoneTransform;
        newOldBoneTransform.oldBone = oldBone;
        newOldBoneTransform.active = true;
        newOldBoneTransform.isNew = true;

        // Initialize all buffers with the current oldBone transform
        for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
        {
            newOldBoneTransform.transforms[i].position = oldBone->getPosition();
            newOldBoneTransform.transforms[i].orientation = oldBone->getOrientation();
        }

        // Add to tracked oldBones vector
        size_t newIndex = this->trackedOldBones.size();
        this->trackedOldBones.push_back(std::move(newOldBoneTransform));

        // Add to hash map for fast lookups
        this->oldBoneToIndexMap[oldBone] = newIndex;
    }

    void GraphicsModule::removeTrackedOldBone(Ogre::v1::OldBone* oldBone)
    {
        if (this->trackedOldBones.empty())
        {
            return;
        }

        auto it = this->oldBoneToIndexMap.find(oldBone);
        if (it != this->oldBoneToIndexMap.end())
        {
            size_t indexToRemove = it->second;
            size_t lastIndex = this->trackedOldBones.size() - 1;

            // If this is not the last element, move the last element to this position
            if (indexToRemove != lastIndex)
            {
                // Move the last element to the position of the removed element
                this->trackedOldBones[indexToRemove] = std::move(this->trackedOldBones[lastIndex]);

                // Update the index in the hash map for the moved element
                this->oldBoneToIndexMap[this->trackedOldBones[indexToRemove].oldBone] = indexToRemove;
            }

            // Remove the last element (now duplicated or the one we want to remove)
            this->trackedOldBones.pop_back();

            // Remove from the hash map
            this->oldBoneToIndexMap.erase(it);
        }
    }

    void GraphicsModule::updateOldBonePosition(Ogre::v1::OldBone* oldBone, const Ogre::Vector3& position)
    {
        GraphicsModule::OldBoneTransforms* oldBoneTransforms = this->findOldBoneTransforms(oldBone);

        // If oldBone isn't tracked yet, add it
        if (nullptr == oldBoneTransforms)
        {
            this->addTrackedOldBone(oldBone);
            oldBoneTransforms = this->findOldBoneTransforms(oldBone);
        }

        // Update position in current buffer only
        // This is key to the mutex-free approach - logic thread only writes to current buffer
        oldBoneTransforms->transforms[this->currentTransformOldBoneIdx].position = position;
        oldBoneTransforms->active = true;
    }

    void GraphicsModule::updateOldBoneOrientation(Ogre::v1::OldBone* oldBone, const Ogre::Quaternion& orientation)
    {
        GraphicsModule::OldBoneTransforms* oldBoneTransforms = this->findOldBoneTransforms(oldBone);

        // If oldBone isn't tracked yet, add it
        if (nullptr == oldBoneTransforms)
        {
            this->addTrackedOldBone(oldBone);
            oldBoneTransforms = this->findOldBoneTransforms(oldBone);
        }

        // Update orientation in current buffer only
        // This is key to the mutex-free approach - logic thread only writes to current buffer
        oldBoneTransforms->transforms[this->currentTransformOldBoneIdx].orientation = orientation;
        oldBoneTransforms->active = true;
    }

    void GraphicsModule::updateOldBoneTransform(Ogre::v1::OldBone* oldBone, const Ogre::Vector3& position, const Ogre::Quaternion& orientation)
    {
        GraphicsModule::OldBoneTransforms* oldBoneTransforms = this->findOldBoneTransforms(oldBone);

        // If oldBone isn't tracked yet, add it
        if (nullptr == oldBoneTransforms)
        {
            this->addTrackedOldBone(oldBone);
            oldBoneTransforms = this->findOldBoneTransforms(oldBone);
        }

        // Update position in current buffer only
        // This is key to the mutex-free approach - logic thread only writes to current buffer
        oldBoneTransforms->transforms[this->currentTransformOldBoneIdx].position = position;
        oldBoneTransforms->transforms[this->currentTransformOldBoneIdx].orientation = orientation;
        oldBoneTransforms->active = true;
    }

    void GraphicsModule::addTrackedPass(Ogre::Pass* pass)
    {
        if (this->findPassTransforms(pass))
        {
            return;
        }

        GraphicsModule::PassTransforms newTransform;
        newTransform.pass = pass;
        newTransform.active = true;
        newTransform.isNew = true;

        for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
        {
            std::fill(std::begin(newTransform.transforms[i].speedsX), std::end(newTransform.transforms[i].speedsX), 0.0f);
            std::fill(std::begin(newTransform.transforms[i].speedsY), std::end(newTransform.transforms[i].speedsY), 0.0f);
        }

        size_t newIndex = this->trackedPasses.size();
        this->trackedPasses.push_back(std::move(newTransform));
        this->passToIndexMap[pass] = newIndex;
    }

    void GraphicsModule::removeTrackedPass(Ogre::Pass* pass)
    {
        if (this->trackedPasses.empty()) return;

        auto it = this->passToIndexMap.find(pass);
        if (it != this->passToIndexMap.end())
        {
            size_t indexToRemove = it->second;
            size_t lastIndex = this->trackedPasses.size() - 1;

            if (indexToRemove != lastIndex)
            {
                this->trackedPasses[indexToRemove] = std::move(this->trackedPasses[lastIndex]);
                this->passToIndexMap[this->trackedPasses[indexToRemove].pass] = indexToRemove;
            }

            this->trackedPasses.pop_back();
            this->passToIndexMap.erase(it);
        }
    }

    void GraphicsModule::updatePassSpeedsX(Ogre::Pass* pass, unsigned short index, Ogre::Real speedX)
    {
        GraphicsModule::PassTransforms* passTransform = this->findPassTransforms(pass);
        if (nullptr == passTransform)
        {
            this->addTrackedPass(pass);
            passTransform = this->findPassTransforms(pass);
        }

        passTransform->transforms[this->currentTransformPassIdx].speedsX[index] = speedX;
        passTransform->active = true;
    }

    void GraphicsModule::updatePassSpeedsY(Ogre::Pass* pass, unsigned short index, Ogre::Real speedY)
    {
        GraphicsModule::PassTransforms* passTransform = this->findPassTransforms(pass);
        if (nullptr == passTransform)
        {
            this->addTrackedPass(pass);
            passTransform = this->findPassTransforms(pass);
        }

        passTransform->transforms[this->currentTransformPassIdx].speedsY[index] = speedY;
        passTransform->active = true;
    }

    void GraphicsModule::addTrackedDatablock(Ogre::HlmsDatablock* datablock, const Ogre::ColourValue& initialValue, std::function<void(Ogre::ColourValue)> applyFunc,
        std::function<Ogre::ColourValue(const Ogre::ColourValue&, const Ogre::ColourValue&, Ogre::Real)> interpFunc)
    {
        if (this->datablockToIndexMap.count(datablock))
        {
            return;
        }

        TrackedDatablock tracked;
        tracked.datablock = datablock;
        tracked.applyFunc = std::move(applyFunc);
        tracked.interpolateFunc = std::move(interpFunc);
        tracked.active = true;
        tracked.isNew = true;

        for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
        {
            tracked.values[i] = initialValue;
        }

        size_t newIndex = this->trackedDatablocks.size();
        this->trackedDatablocks.push_back(std::move(tracked));
        this->datablockToIndexMap[datablock] = newIndex;
    }

    void GraphicsModule::removeTrackedDatablock(Ogre::HlmsDatablock* datablock)
    {
        if (this->trackedDatablocks.empty())
        {
            return;
        }

        auto it = this->datablockToIndexMap.find(datablock);
        if (it != this->datablockToIndexMap.end())
        {
            size_t indexToRemove = it->second;
            size_t lastIndex = this->trackedDatablocks.size() - 1;

            // If this is not the last element, move the last element to this position
            if (indexToRemove != lastIndex)
            {
                // Move the last element to the position of the removed element
                this->trackedDatablocks[indexToRemove] = std::move(this->trackedDatablocks[lastIndex]);

                // Update the index in the hash map for the moved element
                this->datablockToIndexMap[this->trackedDatablocks[indexToRemove].datablock] = indexToRemove;
            }

            // Remove the last element (now duplicated or the one we want to remove)
            this->trackedDatablocks.pop_back();

            // Remove from the hash map
            this->datablockToIndexMap.erase(it);
        }
    }

    void GraphicsModule::updateTrackedDatablockValue(Ogre::HlmsDatablock* datablock, const Ogre::ColourValue& initialValue, const Ogre::ColourValue& targetValue, std::function<void(Ogre::ColourValue)> applyFunc,
        std::function<Ogre::ColourValue(const Ogre::ColourValue&, const Ogre::ColourValue&, Ogre::Real)> interpFunc)
    {
        GraphicsModule::TrackedDatablock* trackedDatablock = this->findTrackedDatablock(datablock);

        // If datablock isn't tracked yet, add it
        if (nullptr == trackedDatablock)
        {
            this->addTrackedDatablock(datablock, initialValue, applyFunc, interpFunc);
            trackedDatablock = this->findTrackedDatablock(datablock);
        }

        trackedDatablock->values[this->currentTrackedDatablockIdx] = targetValue;
        //  Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "updateTrackedDatablockValue tracked intex: " + Ogre::StringConverter::toString(this->currentTrackedDatablockIdx)
        //      + " targetValue: " + Ogre::StringConverter::toString(targetValue));
        trackedDatablock->active = true;
    }

    void GraphicsModule::removeTrackedClosure(const Ogre::String& uniqueName)
    {
        // Ensure queue is initialized
        if (!this->queueInitialized.load())
        {
            return;
        }

        // Create removal command
        ClosureCommand command(uniqueName, nullptr, true, false, true);

        // Use producer token for better performance
        bool success = this->closureQueue.enqueue(producerToken, std::move(command));

        if (false == success)
        {
            Ogre::LogManager::getSingleton().logMessage("[GraphicsModule] Warning: Failed to enqueue removal command for: " + uniqueName, Ogre::LML_CRITICAL);
        }
    }

    void GraphicsModule::updateTrackedClosure(const Ogre::String& uniqueName, std::function<void(Ogre::Real)> closureFunc, bool fireAndForget)
    {
        // Ensure queue is initialized
        if (!this->queueInitialized.load())
        {
            return;
        }

        // Create command and enqueue it - completely lock-free
        ClosureCommand command(uniqueName, std::move(closureFunc), fireAndForget, true, false);

        // Use producer token for better performance
        bool success = this->closureQueue.enqueue(producerToken, std::move(command));

        if (!success)
        {
            // Queue is full or memory allocation failed
            // In practice, this should rarely happen with moodycamel::ConcurrentQueue
            Ogre::LogManager::getSingleton().logMessage("[GraphicsModule] Warning: Failed to enqueue closure command for: " + uniqueName, Ogre::LML_CRITICAL);
        }
    }

    void GraphicsModule::processClosureCommands(Ogre::Real interpolationWeight)
    {
        // Process all available commands in the queue
        ClosureCommand command;

        // Use consumer token for better performance
        // Process up to 1000 commands per frame to prevent blocking
        size_t processedCount = 0;
        const size_t maxCommandsPerFrame = 1000;

        while (processedCount < maxCommandsPerFrame && this->closureQueue.try_dequeue(consumerToken, command))
        {
            this->processSingleCommand(command, interpolationWeight);
            ++processedCount;
        }

        // Log if we hit the limit (might indicate a problem)
        if (processedCount >= maxCommandsPerFrame)
        {
            Ogre::LogManager::getSingleton().logMessage("[GraphicsModule] Warning: Processed maximum closure commands per frame (" + Ogre::StringConverter::toString(maxCommandsPerFrame) + ")", Ogre::LML_NORMAL);
        }
    }

    void GraphicsModule::executeActiveClosures(Ogre::Real interpolationWeight)
    {
        // Execute all persistent closures
        auto it = this->persistentClosures.begin();
        while (it != this->persistentClosures.end())
        {
            if (it->second.active && it->second.closureFunc)
            {
                try
                {
                    it->second.closureFunc(interpolationWeight);
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

    void GraphicsModule::updateAndExecuteClosures(Ogre::Real interpolationWeight)
    {
        // First process all queued commands
        this->processClosureCommands(interpolationWeight);

        // Then execute all active closures
        this->executeActiveClosures(interpolationWeight);
    }

    void GraphicsModule::processSingleCommand(const ClosureCommand& command, Ogre::Real interpolationWeight)
    {
        if (command.isRemoval)
        {
            // Handle removal command
            this->removePersistentClosure(command.uniqueName);
        }
        else if (command.fireAndForget)
        {
            // Execute fire-and-forget closure immediately
            if (command.closureFunc)
            {
                try
                {
                    command.closureFunc(interpolationWeight);
                }
                catch (const std::exception& e)
                {
                    Ogre::LogManager::getSingleton().logMessage("[GraphicsModule] Error executing fire-and-forget closure '" + command.uniqueName + "': " + e.what(), Ogre::LML_CRITICAL);
                }
            }
        }
        else
        {
            // Handle persistent closure
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
        // Add or update persistent closure
        this->persistentClosures[uniqueName] = PersistentClosure(uniqueName, std::move(closureFunc));
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
            logManager.logMessage(Ogre::LML_CRITICAL, "Tracked nodes: " + std::to_string(this->trackedNodes.size()), false);

            for (size_t i = 0; i < this->trackedNodes.size(); ++i)
            {
                const auto& nodeTransform = this->trackedNodes[i];
                logManager.logMessage(Ogre::LML_CRITICAL, "Node " + std::to_string(i) + " (" + Ogre::StringConverter::toString(nodeTransform.node) + "):", false);
                logManager.logMessage(Ogre::LML_CRITICAL, "  Active: " + std::string(nodeTransform.active ? "yes" : "no"), false);
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
        // Runs in Main thread
      
        // Move to the next buffer - this is the key operation that separates logic and render threads
        size_t prevIdx = this->currentTransformNodeIdx;
        this->currentTransformNodeIdx = (this->currentTransformNodeIdx + 1) % NUM_TRANSFORM_BUFFERS;

        if (true == this->debugVisualization)
        {
            this->logCommandEvent("[RenderCommandQueueModule]: Advanced buffer from "
                + Ogre::StringConverter::toString(prevIdx) + " to " + Ogre::StringConverter::toString(this->currentTransformNodeIdx), Ogre::LML_TRIVIAL);
        }

        // For newly added nodes, we need to copy the current transform to all buffers
        // For existing nodes, we copy the previous buffer to the new current buffer
        // This ensures that if an object doesn't move during a frame, it maintains its position
        for (auto& nodeTransform : this->trackedNodes)
        {
            if (true == nodeTransform.isNew)
            {
                // For new nodes, initialize all buffers with current transform
                GraphicsModule::TransformData currentTransform;
                currentTransform.position = nodeTransform.node->getPosition();
                currentTransform.orientation = nodeTransform.node->getOrientation();
                currentTransform.scale = nodeTransform.node->getScale();

                for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
                {
                    nodeTransform.transforms[i] = currentTransform;
                }

                nodeTransform.isNew = false;
                nodeTransform.stableFrames = 0;
                nodeTransform.updatedThisFrame = false;
            }
            else if (true == nodeTransform.active)
            {
                if (nodeTransform.updatedThisFrame)
                {
                    // Got a new transform this frame
                    nodeTransform.stableFrames = 0;
                }
                else
                {
                    ++nodeTransform.stableFrames;     // <<< THIS is the increment
                }

                // For existing active nodes, copy from previous buffer to new current buffer
                // This is important - we're not copying from the node's current transform
                // but from the previous buffer, which preserves the transform history
                nodeTransform.transforms[this->currentTransformNodeIdx] = nodeTransform.transforms[prevIdx];

                // Reset update flag for next frame
                nodeTransform.updatedThisFrame = false;
            }
        }

        // Deactivates nodes that have not been updated for a while
        const int maxStableFrames = 4;

        for (int i = static_cast<int>(this->trackedNodes.size()) - 1; i >= 0; --i)
        {
            NodeTransforms& nodeTransform = this->trackedNodes[i];

            // Optionally also check !active, but not needed if you just rely on stableFrames
            if (nodeTransform.stableFrames >= maxStableFrames)
            {
                this->removeTrackedNode(nodeTransform.node);
            }
        }

        // Advance camera transform buffer
        
        // Move to the next buffer - this is the key operation that separates logic and render threads
        size_t prevCameraIdx = this->currentTransformCameraIdx;
        this->currentTransformCameraIdx = (this->currentTransformCameraIdx + 1) % NUM_TRANSFORM_BUFFERS;

        // For newly added cameras, we need to copy the current transform to all buffers
        // For existing cameras, we copy the previous buffer to the new current buffer
        // This ensures that if an object doesn't move during a frame, it maintains its position
        for (auto& cameraTransform : this->trackedCameras)
        {
            if (true == cameraTransform.isNew)
            {
                // For new cameras, initialize all buffers with current transform
                GraphicsModule::CameraTransformData currentTransform;
                currentTransform.position = cameraTransform.camera->getPosition();
                currentTransform.orientation = cameraTransform.camera->getOrientation();

                for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
                {
                    cameraTransform.transforms[i] = currentTransform;
                }

                cameraTransform.isNew = false;
            }
            else if (true == cameraTransform.active)
            {
                // For existing active cameras, copy from previous buffer to new current buffer
                // This is important - we're not copying from the camera's current transform
                // but from the previous buffer, which preserves the transform history
                cameraTransform.transforms[this->currentTransformCameraIdx] = cameraTransform.transforms[prevCameraIdx];
            }
        }

        // Advance oldBone transform buffer

        // Move to the next buffer - this is the key operation that separates logic and render threads
        size_t prevOldBoneIdx = this->currentTransformOldBoneIdx;
        this->currentTransformOldBoneIdx = (this->currentTransformOldBoneIdx + 1) % NUM_TRANSFORM_BUFFERS;

        // For newly added oldBones, we need to copy the current transform to all buffers
        // For existing oldBones, we copy the previous buffer to the new current buffer
        // This ensures that if an object doesn't move during a frame, it maintains its position
        for (auto& oldBoneTransform : this->trackedOldBones)
        {
            if (true == oldBoneTransform.isNew)
            {
                // For new oldBones, initialize all buffers with current transform
                GraphicsModule::TransformData currentTransform;
                currentTransform.position = oldBoneTransform.oldBone->getPosition();
                currentTransform.orientation = oldBoneTransform.oldBone->getOrientation();

                for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
                {
                    oldBoneTransform.transforms[i] = currentTransform;
                }

                oldBoneTransform.isNew = false;
            }
            else if (true == oldBoneTransform.active)
            {
                // For existing active oldBones, copy from previous buffer to new current buffer
                // This is important - we're not copying from the oldBone's current transform
                // but from the previous buffer, which preserves the transform history
                oldBoneTransform.transforms[this->currentTransformOldBoneIdx] = oldBoneTransform.transforms[prevOldBoneIdx];
            }
        }

        size_t prevPassIdx = this->currentTransformPassIdx;
        this->currentTransformPassIdx = (this->currentTransformPassIdx + 1) % NUM_TRANSFORM_BUFFERS;

        for (auto& passTransform : this->trackedPasses)
        {
            if (passTransform.isNew)
            {
                // Initialize all buffers with current pass state
                GraphicsModule::PassSpeedData currentData;

                for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
                {
                    passTransform.transforms[i] = currentData;
                }
                passTransform.isNew = false;
            }
            else if (passTransform.active)
            {
                // Copy previous buffer to current buffer
                passTransform.transforms[this->currentTransformPassIdx] = passTransform.transforms[prevPassIdx];
            }
        }


        // Advance datablock buffer

        size_t prevDatablockIdx = this->currentTrackedDatablockIdx;
        this->currentTrackedDatablockIdx = (this->currentTrackedDatablockIdx + 1) % NUM_TRANSFORM_BUFFERS;

        for (auto& datablock : this->trackedDatablocks)
        {
            if (datablock.isNew)
            {
                for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
                {
                    datablock.values[i] = datablock.values[this->currentTrackedDatablockIdx];
                }
                datablock.isNew = false;
            }
            else if (datablock.active)
            {
                datablock.values[this->currentTrackedDatablockIdx] = datablock.values[prevDatablockIdx];
            }
        }

        // Reset accumulated time for new frame
        this->accumTimeSinceLastLogicFrame = 0.0f;
    }

    void GraphicsModule::updateAllTransforms(void)
    {
        // Get the previous buffer index
        size_t prevIdx = this->getPreviousTransformNodeIdx();

        // Update all active nodes
        for (auto& nodeTransform : this->trackedNodes)
        {
            if (true == nodeTransform.active)
            {
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
                if (false == nodeTransform.useDerived)
                {
                    nodeTransform.node->setOrientation(currTransform.orientation);
                    nodeTransform.node->setPosition(interpPos);
                }
                else
                {
                    nodeTransform.node->_setDerivedOrientation(interpRot);
                    nodeTransform.node->_setDerivedPosition(interpPos);
                }

                nodeTransform.node->setScale(interpScale);
            }
        }

        // Update camera transforms


        // Get the previous buffer index
        size_t prevCameraIdx = this->getPreviousTransformCameraIdx();

        // Update all active cameras
        for (auto& cameraTransform : this->trackedCameras)
        {
            if (true == cameraTransform.active)
            {
                // Get previous and current transforms
                const GraphicsModule::CameraTransformData& prevTransform = cameraTransform.transforms[prevCameraIdx];
                const GraphicsModule::CameraTransformData& currTransform = cameraTransform.transforms[this->currentTransformCameraIdx];

                // Interpolate position
                Ogre::Vector3 interpPos = Ogre::Math::lerp(prevTransform.position, currTransform.position, this->interpolationWeight);

                // Interpolate orientation
                Ogre::Quaternion interpRot = Ogre::Quaternion::nlerp(this->interpolationWeight, prevTransform.orientation, currTransform.orientation, true);

                // Apply to scene camera
                cameraTransform.camera->setOrientation(interpRot);
                cameraTransform.camera->setPosition(interpPos);
            }
        }

        // Update oldBone transforms


        // Get the previous buffer index
        size_t prevOldBoneIdx = this->getPreviousTransformOldBoneIdx();

        // Update all active oldBones
        for (auto& oldBoneTransform : this->trackedOldBones)
        {
            if (true == oldBoneTransform.active)
            {
                // Get previous and current transforms
                const GraphicsModule::TransformData& prevTransform = oldBoneTransform.transforms[prevOldBoneIdx];
                const GraphicsModule::TransformData& currTransform = oldBoneTransform.transforms[this->currentTransformOldBoneIdx];

                // Interpolate position
                Ogre::Vector3 interpPos = Ogre::Math::lerp(prevTransform.position, currTransform.position, this->interpolationWeight);

                // Interpolate orientation
                Ogre::Quaternion interpRot = Ogre::Quaternion::nlerp(this->interpolationWeight, prevTransform.orientation, currTransform.orientation, true);

                // Apply to scene oldBone
                oldBoneTransform.oldBone->setOrientation(interpRot);
                oldBoneTransform.oldBone->setPosition(interpPos);
            }
        }

        size_t prevPassIdx = this->getPreviousTransformPassIdx();

        for (auto& passTransform : this->trackedPasses)
        {
            if (passTransform.active)
            {
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

                passTransform.pass->getFragmentProgramParameters()->setNamedConstant("speedsX", interpX, 9, 1);
                passTransform.pass->getFragmentProgramParameters()->setNamedConstant("speedsY", interpY, 9, 1);
            }
        }

        // Update datablock colours

        // Get the previous buffer index
        size_t prevTrackedDatablockIdx = this->getPreviousTrackedDatablockIdx();

        for (auto& trackedDatablock : this->trackedDatablocks)
        {
            if (false == trackedDatablock.active)
            {
                continue;
            }

            const Ogre::ColourValue& prev = trackedDatablock.values[prevTrackedDatablockIdx];
            const Ogre::ColourValue& curr = trackedDatablock.values[this->currentTrackedDatablockIdx];

            Ogre::ColourValue result = trackedDatablock.interpolateFunc(prev, curr, this->interpolationWeight);

            trackedDatablock.applyFunc(result);
        }

        // Update and execute all closures - completely lock-free
        this->updateAndExecuteClosures(this->interpolationWeight);
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
        return  this->accumTimeSinceLastLogicFrame;
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
        // TODO: REmoved log flooding
        return;

        if (level >= this->logLevel)
        {
            std::stringstream ss;
            ss << "[RenderCommandQueue] "
                << message
                << " (Thread: " << (this->isRenderThread() ? "RENDER" : "MAIN")
                << ", Depth: " << g_renderCommandDepth
                << ", WaitDepth: " << g_waitDepth << ")";

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
        // If about to destroy, to not delay destruction!
        if (false == this->bRunning)
        {
            this->enqueueAndWait(std::move(destroyCommand), commandName);
            return;
        }

        // Important: enqueueAndWait has most priority and my not be interrupted by destroy command, because:
        // For example unloading a whole application, is better todo in a wait command, so that the logic main thread must wait, until the rendering thread has freed all resources
        // So if the flag is set, nothing is enqueued for destruction
        if (true == this->isRunningWaitClosure)
        {
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