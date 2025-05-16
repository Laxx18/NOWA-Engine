#include "NOWAPrecompiled.h"
#include "RenderCommandQueueModule.h"
#include "main/Core.h"

#include <sstream>

// In the namespace section at the top of your file
namespace
{
    thread_local int g_renderCommandDepth = 0;
    thread_local int g_waitDepth = 0;

    // Default timeout value (can be changed at runtime)
    std::chrono::milliseconds g_defaultTimeout(5000); // 5 seconds
}

namespace NOWA
{

    RenderCommandQueueModule::RenderCommandQueueModule()
        : timeoutEnabled(true),
        timeoutDuration(g_defaultTimeout.count()),
        logLevel(Ogre::LML_NORMAL),
        currentTransformNodeIdx(0),
        currentTransformCameraIdx(0),
        accumTimeSinceLastLogicFrame(0.0f),
        frameTime(1.0f / 30.0f),
        debugVisualization(false),
        logicFrameCount(0),
        renderFrameCount(0)
    {
        // Reserve some space for tracked nodes to avoid frequent reallocations
        this->trackedNodes.reserve(100);
    }

    RenderCommandQueueModule::~RenderCommandQueueModule()
    {
        // Clear tracked nodes
        this->trackedNodes.clear();
        this->trackedCameras.clear();
    }

    void RenderCommandQueueModule::enqueue(RenderCommand command, std::shared_ptr<std::promise<void>> promise)
    {
        // Make sure we have a valid command
        if (!command)
        {
            // If we have a promise, fulfill it to prevent deadlock
            if (promise && !wasPromiseFulfilled(promise))
            {
                try
                {
                    promise->set_value();
                    markPromiseFulfilled(promise);
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
        if (isRenderThread() && g_renderCommandDepth > 0)
        {
            // Log direct execution
            logCommandEvent("Executing command directly on render thread", Ogre::LML_TRIVIAL);

            try
            {
                command();
                if (promise && !wasPromiseFulfilled(promise))
                {
                    promise->set_value();
                    markPromiseFulfilled(promise);
                }
            }
            catch (const std::exception& e)
            {
                logCommandEvent(std::string("Exception caught in direct command execution: ") + e.what(), Ogre::LML_CRITICAL);
                if (promise && !wasPromiseFulfilled(promise))
                {
                    try
                    {
                        promise->set_exception(std::current_exception());
                        markPromiseFulfilled(promise);
                    }
                    catch (...) {
                        // Ignore any exceptions from setting the exception
                    }
                }
                // We don't rethrow here - we contain the exception within the command
            }
            catch (...)
            {
                logCommandEvent("Unknown exception caught in direct command execution", Ogre::LML_CRITICAL);
                if (promise && !wasPromiseFulfilled(promise))
                {
                    try
                    {
                        promise->set_exception(std::current_exception());
                        markPromiseFulfilled(promise);
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

            std::lock_guard<std::mutex> lock(mutex);
            this->queue.push(std::move(entry));

            // Log enqueuing
            logCommandEvent("Command enqueued for later execution", Ogre::LML_TRIVIAL);
        }
    }

    void RenderCommandQueueModule::processAllCommands(void)
    {
        // Safety check that we're on the render thread
        if (!isRenderThread())
        {
            logCommandEvent("processAllCommands called from non-render thread!", Ogre::LML_CRITICAL);
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
                    logCommandEvent("Executing command from processAllCommands", Ogre::LML_TRIVIAL);
                    entry.command();
                }

                // Set the promise value if it exists and hasn't been fulfilled yet
                if (entry.completionPromise && !wasPromiseFulfilled(entry.completionPromise))
                {
                    entry.completionPromise->set_value();
                    markPromiseFulfilled(entry.completionPromise);
                }
            }
            catch (const std::exception& e)
            {
                logCommandEvent(std::string("Exception in processAllCommands: ") + e.what(), Ogre::LML_CRITICAL);

                // Set exception on the promise if it exists
                if (entry.completionPromise && !wasPromiseFulfilled(entry.completionPromise))
                {
                    try
                    {
                        entry.completionPromise->set_exception(std::current_exception());
                        markPromiseFulfilled(entry.completionPromise);
                    }
                    catch (...) { /* Ignore exceptions from setting the exception */ }
                }
            }
            catch (...)
            {
                logCommandEvent("Unknown exception in processAllCommands", Ogre::LML_CRITICAL);

                // Set exception on the promise if it exists
                if (entry.completionPromise && !wasPromiseFulfilled(entry.completionPromise))
                {
                    try
                    {
                        entry.completionPromise->set_exception(std::current_exception());
                        markPromiseFulfilled(entry.completionPromise);
                    }
                    catch (...) { /* Ignore exceptions from setting the exception */ }
                }
            }
        }

        // Cleanup fulfilled promises occasionally (when we've processed at least one command)
        if (processedAnyCommands)
        {
            cleanupFulfilledPromises();
        }

        // Decrement render command depth
        g_renderCommandDepth--;
    }

    void RenderCommandQueueModule::waitForRenderCompletion(void)
    {
        // If we're already on the render thread, just process the queue directly
        if (isRenderThread())
        {
            logCommandEvent("waitForRenderCompletion called from render thread - processing queue directly", Ogre::LML_NORMAL);
            processAllCommands();
            return;
        }

        // Submit a command that processes all pending commands and blocks until it's done
        auto promise = std::make_shared<std::promise<void>>();
        auto future = promise->get_future();

        // Create a special sync command that processes everything in the queue
        this->enqueue([this]()
        {
            // This will execute in the render thread and process any commands ahead of it
            logCommandEvent("Processing queue from waitForRenderCompletion", Ogre::LML_NORMAL);
        }, promise);

        // Wait for the command to complete
        try
        {
            logCommandEvent("Waiting for render queue synchronization", Ogre::LML_NORMAL);
            auto timeout = getTimeoutDuration();
            if (isTimeoutEnabled())
            {
                if (future.wait_for(timeout) == std::future_status::timeout)
                {
                    logCommandEvent("Timeout occurred waiting for render queue synchronization", Ogre::LML_CRITICAL);
                }
            }
            else
            {
                future.wait();
            }
            logCommandEvent("Render queue synchronization complete", Ogre::LML_NORMAL);
        }
        catch (const std::exception& e)
        {
            logCommandEvent(std::string("Exception waiting for render completion: ") + e.what(), Ogre::LML_CRITICAL);
        }
    }

    bool RenderCommandQueueModule::pop(CommandEntry& commandEntry)
    {
        std::lock_guard<std::mutex> lock(this->mutex);
        if (!this->queue.empty())
        {
            commandEntry = this->queue.front();
            this->queue.pop();
            return true;
        }
        return false;
    }

    void RenderCommandQueueModule::cleanupFulfilledPromises()
    {
        std::lock_guard<std::mutex> lock(this->fulfilledMutex);

        // Only clean up if we have a significant number of fulfilled promises stored
        if (fulfilledPromises.size() > 1000)
        {
            std::stringstream ss;
            ss << "Cleaning up " << fulfilledPromises.size() << " fulfilled promises";
            logCommandEvent(ss.str(), Ogre::LML_NORMAL);

            fulfilledPromises.clear();
        }
    }

    void RenderCommandQueueModule::executeAndWait(RenderCommand command, const char* commandName)
    {
        // If we're already on the render thread, execute directly
        if (isRenderThread())
        {
            try
            {
                logCommandEvent(std::string("Executing command '") + commandName + "' directly on render thread", Ogre::LML_TRIVIAL);
                command();
                return;
            }
            catch (const std::exception& e)
            {
                logCommandEvent(std::string("Exception in direct execution of '") + commandName + "': " + e.what(), Ogre::LML_CRITICAL);
                throw; // Re-throw the exception
            }
            catch (...)
            {
                logCommandEvent(std::string("Unknown exception in direct execution of '") + commandName + "'", Ogre::LML_CRITICAL);
                throw; // Re-throw the exception
            }
        }

        // Track that we're in a waiting state on this thread
        incrementWaitDepth();

        try
        {
            // Create a promise for this command
            auto promise = std::make_shared<std::promise<void>>();
            auto future = promise->get_future();

            // Log the command
            logCommandEvent(std::string("Enqueueing command '") + commandName + "' with wait", Ogre::LML_NORMAL);

            // If we're in a nested wait, we still enqueue with the promise but we'll use a special flag
            bool isNested = isInNestedWait() && g_waitDepth > 1; // We've already incremented waitDepth

            if (isNested)
            {
                logCommandEvent(std::string("Command '") + commandName + "' is a nested wait call (depth: " +
                    std::to_string(g_waitDepth) + ")", Ogre::LML_NORMAL);
            }

            // Always enqueue with a promise, even for nested commands
            enqueue(command, promise);

            // For the first level wait, process immediately to avoid deadlocks
            if (!isNested)
            {
                // Wait for the command to complete
                if (isTimeoutEnabled())
                {
                    auto timeout = getTimeoutDuration();
                    auto status = future.wait_for(timeout);

                    if (status == std::future_status::timeout)
                    {
                        logCommandEvent(std::string("Timeout occurred waiting for command '") + commandName + "'", Ogre::LML_CRITICAL);

                        // Force a full queue synchronization to recover
                        processQueueSync();
                    }
                }
                else
                {
                    future.wait();
                }
            }
            else
            {
                // For nested waits, we still track the promise but don't wait for it
                // The outer wait will ensure all commands get processed eventually
                logCommandEvent(std::string("Nested command '") + commandName +
                    "' enqueued without blocking wait", Ogre::LML_NORMAL);

                // Add to tracking for nested commands if needed
                // trackNestedCommand(promise); // Optional: implement this to track nested promises
            }
        }
        catch (const std::exception& e)
        {
            logCommandEvent(std::string("Exception waiting for command '") + commandName + "': " + e.what(), Ogre::LML_CRITICAL);
            decrementWaitDepth();
            throw;
        }
        catch (...)
        {
            logCommandEvent(std::string("Unknown exception waiting for command '") + commandName + "'", Ogre::LML_CRITICAL);
            decrementWaitDepth();
            throw;
        }

        decrementWaitDepth();
        logCommandEvent(std::string("Command '") + commandName + "' completed", Ogre::LML_TRIVIAL);
    }

    void RenderCommandQueueModule::processQueueSync(void)
    {
        // If we're on the render thread, process directly
        if (isRenderThread())
        {
            logCommandEvent("Processing queue synchronously on render thread", Ogre::LML_NORMAL);
            processAllCommands();
            return;
        }

        // Otherwise, we need to add a sync point and wait for it
        auto syncPromise = std::make_shared<std::promise<void>>();
        auto syncFuture = syncPromise->get_future();

        // Create a special command that just completes the sync
        enqueue([this]() {
            // This will execute after all previous commands
            logCommandEvent("Sync point reached in queue processing", Ogre::LML_NORMAL);
            }, syncPromise);

        // Wait without timeout - we need to ensure this completes
        logCommandEvent("Waiting for queue sync point", Ogre::LML_NORMAL);
        syncFuture.wait();
        logCommandEvent("Queue sync point completed", Ogre::LML_NORMAL);
    }

    void RenderCommandQueueModule::markPromiseFulfilled(const std::shared_ptr<std::promise<void>>& promise)
    {
        if (!promise) return;

        std::lock_guard<std::mutex> lock(this->fulfilledMutex);
        fulfilledPromises.insert(promise);
    }

    bool RenderCommandQueueModule::wasPromiseFulfilled(const std::shared_ptr<std::promise<void>>& promise)
    {
        if (!promise) return true;

        std::lock_guard<std::mutex> lock(this->fulfilledMutex);
        return fulfilledPromises.find(promise) != fulfilledPromises.end();
    }

    void RenderCommandQueueModule::markCurrentThreadAsRenderThread()
    {
        this->renderThreadId.store(std::this_thread::get_id());
    }

    bool RenderCommandQueueModule::isRenderThread() const
    {
        return std::this_thread::get_id() == this->renderThreadId.load();
    }

    void RenderCommandQueueModule::incrementWaitDepth(void)
    {
        g_waitDepth++;
        this->logCommandEvent("Incremented wait depth", Ogre::LML_TRIVIAL);
    }

    void RenderCommandQueueModule::decrementWaitDepth(void)
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

    int RenderCommandQueueModule::getWaitDepth(void) const
    {
        return g_waitDepth;
    }

    bool RenderCommandQueueModule::isInNestedWait(void) const
    {
        return g_waitDepth > 0;
    }

    void RenderCommandQueueModule::enableTimeout(bool enable)
    {
        this->timeoutEnabled = enable;
        this->logCommandEvent(std::string("Timeout ") + (enable ? "enabled" : "disabled"), Ogre::LML_NORMAL);
    }

    bool RenderCommandQueueModule::isTimeoutEnabled(void) const
    {
        return this->timeoutEnabled;
    }

    void RenderCommandQueueModule::setTimeoutDuration(std::chrono::milliseconds duration)
    {
        this->timeoutDuration = duration.count();
        std::stringstream ss;
        ss << "Timeout duration set to " << duration.count() << "ms";
        this->logCommandEvent(ss.str(), Ogre::LML_NORMAL);
    }

    std::chrono::milliseconds RenderCommandQueueModule::getTimeoutDuration(void) const
    {
        return std::chrono::milliseconds(this->timeoutDuration);
    }

    void RenderCommandQueueModule::recoverFromTimeout(void)
    {
        logCommandEvent("Attempting to recover from command timeout", Ogre::LML_CRITICAL);

        // Clear all waiting depths that might be preventing command processing
        // This is a last resort recovery mechanism
        std::lock_guard<std::mutex> lock(mutex);

        // Log the number of commands still in the queue
        std::stringstream ss;
        ss << "Queue has " << queue.size() << " pending commands during timeout recovery";
        logCommandEvent(ss.str(), Ogre::LML_CRITICAL);

        // Reset wait depth if it's non-zero (something might have gone wrong)
        if (g_waitDepth > 0) {
            std::stringstream ss;
            ss << "Resetting wait depth from " << g_waitDepth << " to 0 during recovery";
            logCommandEvent(ss.str(), Ogre::LML_CRITICAL);
            g_waitDepth = 0;
        }

        // Optionally: process commands here if we're on the render thread
        if (isRenderThread())
        {
            logCommandEvent("Processing command queue during timeout recovery", Ogre::LML_CRITICAL);
            // Release the lock before processing
            lock.~lock_guard();
            processAllCommands();
        }
    }

    bool RenderCommandQueueModule::waitForFutureWithTimeout(std::future<void>& future, const std::chrono::milliseconds& timeout, const char* commandName)
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
            recoverFromTimeout();

            return false;
        }
        return true;
    }

    RenderCommandQueueModule::NodeTransforms* RenderCommandQueueModule::findNodeTransforms(Ogre::Node* node)
    {
        // O(1) lookup using the hash map
        auto it = this->nodeToIndexMap.find(node);
        if (it != this->nodeToIndexMap.end())
        {
            // Return pointer to the NodeTransforms in the vector
            return &this->trackedNodes[it->second];
        }
        return nullptr;
    }

    RenderCommandQueueModule::CameraTransforms* RenderCommandQueueModule::findCameraTransforms(Ogre::Camera* camera)
    {
        // O(1) lookup using the hash map
        auto it = this->cameraToIndexMap.find(camera);
        if (it != this->cameraToIndexMap.end())
        {
            // Return pointer to the CameraTransforms in the vector
            return &this->trackedCameras[it->second];
        }
        return nullptr;
    }

    void RenderCommandQueueModule::addTrackedNode(Ogre::Node* node)
    {
        // Check if node is already tracked
        if (this->findNodeTransforms(node))
        {
            return;
        }

        // Create a new node transform record
        RenderCommandQueueModule::NodeTransforms newNodeTransform;
        newNodeTransform.node = node;
        newNodeTransform.active = true;
        newNodeTransform.isNew = true;

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

    void RenderCommandQueueModule::removeTrackedNode(Ogre::Node* node)
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

    void RenderCommandQueueModule::updateNodePosition(Ogre::Node* node, const Ogre::Vector3& position)
    {
        RenderCommandQueueModule::NodeTransforms* nodeTransforms = this->findNodeTransforms(node);

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

        if (true == this->debugVisualization)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[RenderCommandQueueModule]: Updated position for node: " + node->getName()
             + " to " + Ogre::StringConverter::toString(position) + " in buffer: " + Ogre::StringConverter::toString(this->currentTransformNodeIdx));
        }
    }

    void RenderCommandQueueModule::updateNodeOrientation(Ogre::Node* node, const Ogre::Quaternion& orientation)
    {
        RenderCommandQueueModule::NodeTransforms* nodeTransforms = this->findNodeTransforms(node);

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

        if (true == this->debugVisualization)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[RenderCommandQueueModule]: Updated orientation for node: " + node->getName()
                + " to " + Ogre::StringConverter::toString(orientation) + " in buffer: " + Ogre::StringConverter::toString(this->currentTransformNodeIdx));
        }
    }

    void RenderCommandQueueModule::updateNodeScale(Ogre::Node* node, const Ogre::Vector3& scale)
    {
        RenderCommandQueueModule::NodeTransforms* nodeTransforms = this->findNodeTransforms(node);

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

        if (true == this->debugVisualization)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[RenderCommandQueueModule]: Updated scale for node: " + node->getName()
                + " to " + Ogre::StringConverter::toString(scale) + " in buffer: " + Ogre::StringConverter::toString(this->currentTransformNodeIdx));
        }
    }

    void RenderCommandQueueModule::updateNodeTransform(Ogre::Node* node, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, const Ogre::Vector3& scale)
    {
        RenderCommandQueueModule::NodeTransforms* nodeTransforms = this->findNodeTransforms(node);

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
    }

    void RenderCommandQueueModule::addTrackedCamera(Ogre::Camera* camera)
    {
        // Check if camera is already tracked
        if (this->findCameraTransforms(camera))
        {
            return;
        }

        // Create a new camera transform record
        RenderCommandQueueModule::CameraTransforms newCameraTransform;
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

    void RenderCommandQueueModule::removeTrackedCamera(Ogre::Camera* camera)
    {
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

    void RenderCommandQueueModule::updateCameraPosition(Ogre::Camera* camera, const Ogre::Vector3& position)
    {
        RenderCommandQueueModule::CameraTransforms* cameraTransforms = this->findCameraTransforms(camera);

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

    void RenderCommandQueueModule::updateCameraOrientation(Ogre::Camera* camera, const Ogre::Quaternion& orientation)
    {
        RenderCommandQueueModule::CameraTransforms* cameraTransforms = this->findCameraTransforms(camera);

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

    void RenderCommandQueueModule::updateCameraTransform(Ogre::Camera* camera, const Ogre::Vector3& position, const Ogre::Quaternion& orientation)
    {
        RenderCommandQueueModule::CameraTransforms* cameraTransforms = this->findCameraTransforms(camera);

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

    size_t RenderCommandQueueModule::getCurrentTransformNodeIdx(void) const
    {
        return this->currentTransformNodeIdx;
    }

    size_t RenderCommandQueueModule::getPreviousTransformNodeIdx(void) const
    {
        return (this->currentTransformNodeIdx + NUM_TRANSFORM_BUFFERS - 1) % NUM_TRANSFORM_BUFFERS;
    }

    size_t RenderCommandQueueModule::getCurrentTransformCameraIdx(void) const
    {
        return this->currentTransformCameraIdx;
    }

    size_t RenderCommandQueueModule::getPreviousTransformCameraIdx(void) const
    {
        return (this->currentTransformCameraIdx + NUM_TRANSFORM_BUFFERS - 1) % NUM_TRANSFORM_BUFFERS;
    }

    void RenderCommandQueueModule::enableDebugVisualization(bool enable)
    {
        this->debugVisualization = enable;
    }

    void RenderCommandQueueModule::dumpBufferState() const
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

    void RenderCommandQueueModule::advanceTransformBuffer(void)
    {
        // Move to the next buffer - this is the key operation that separates logic and render threads
        size_t prevIdx = this->currentTransformNodeIdx;
        this->currentTransformNodeIdx = (this->currentTransformNodeIdx + 1) % NUM_TRANSFORM_BUFFERS;

        if (true == this->debugVisualization)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[RenderCommandQueueModule]: Advanced buffer from "
                + Ogre::StringConverter::toString(prevIdx) + " to " + Ogre::StringConverter::toString(this->currentTransformNodeIdx));
        }

        // For newly added nodes, we need to copy the current transform to all buffers
        // For existing nodes, we copy the previous buffer to the new current buffer
        // This ensures that if an object doesn't move during a frame, it maintains its position
        for (auto& nodeTransform : this->trackedNodes)
        {
            if (true == nodeTransform.isNew)
            {
                // For new nodes, initialize all buffers with current transform
                RenderCommandQueueModule::TransformData currentTransform;
                currentTransform.position = nodeTransform.node->getPosition();
                currentTransform.orientation = nodeTransform.node->getOrientation();
                currentTransform.scale = nodeTransform.node->getScale();

                for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
                {
                    nodeTransform.transforms[i] = currentTransform;
                }

                nodeTransform.isNew = false;
            }
            else if (true == nodeTransform.active)
            {
                // For existing active nodes, copy from previous buffer to new current buffer
                // This is important - we're not copying from the node's current transform
                // but from the previous buffer, which preserves the transform history
                nodeTransform.transforms[this->currentTransformNodeIdx] = nodeTransform.transforms[prevIdx];
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
                RenderCommandQueueModule::CameraTransformData currentTransform;
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

        // Reset accumulated time for new frame
        this->accumTimeSinceLastLogicFrame = 0.0f;

        // Increment logic frame counter
        ++this->logicFrameCount;
    }

    void RenderCommandQueueModule::updateAllTransforms(Ogre::Real weight)
    {
        // Increment render frame counter
        ++this->renderFrameCount;

        // Get the previous buffer index
        size_t prevIdx = this->getPreviousTransformNodeIdx();

        if (true == this->debugVisualization)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[RenderCommandQueueModule]: Updating transforms with weight: "
                + Ogre::StringConverter::toString(weight) + " between buffers " + Ogre::StringConverter::toString(prevIdx) + " and "
                + Ogre::StringConverter::toString(this->currentTransformNodeIdx) + " (Logic frame: " + Ogre::StringConverter::toString(this->logicFrameCount)
                + ", Render frame: " + Ogre::StringConverter::toString(this->renderFrameCount) + ")");
        }

        // Update all active nodes
        for (auto& nodeTransform : this->trackedNodes)
        {
            if (true == nodeTransform.active)
            {
                // Get previous and current transforms
                const RenderCommandQueueModule::TransformData& prevTransform = nodeTransform.transforms[prevIdx];
                const RenderCommandQueueModule::TransformData& currTransform = nodeTransform.transforms[this->currentTransformNodeIdx];

                // Interpolate position
                Ogre::Vector3 interpPos = Ogre::Math::lerp(prevTransform.position, currTransform.position, weight);

                // Interpolate orientation
                Ogre::Quaternion interpRot = Ogre::Quaternion::nlerp(weight, prevTransform.orientation, currTransform.orientation, true);

                // Interpolate scale
                Ogre::Vector3 interpScale = Ogre::Math::lerp(prevTransform.scale, currTransform.scale, weight);

                // Apply to scene node
                nodeTransform.node->setPosition(interpPos);
                nodeTransform.node->setOrientation(interpRot);
                nodeTransform.node->setScale(interpScale);

                if (true == this->debugVisualization && weight > 0.0f && weight < 1.0f)
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[RenderCommandQueueModule]: Node "
                        + nodeTransform.node->getName() + " interpolated pos: " + Ogre::StringConverter::toString(interpPos) + " (prev: "
                        + Ogre::StringConverter::toString(prevTransform.position) + ", curr: " + Ogre::StringConverter::toString(currTransform.position) + ")");
                }
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
                const RenderCommandQueueModule::CameraTransformData& prevTransform = cameraTransform.transforms[prevCameraIdx];
                const RenderCommandQueueModule::CameraTransformData& currTransform = cameraTransform.transforms[this->currentTransformCameraIdx];

                // Interpolate position
                Ogre::Vector3 interpPos = Ogre::Math::lerp(prevTransform.position, currTransform.position, weight);

                // Interpolate orientation
                Ogre::Quaternion interpRot = Ogre::Quaternion::nlerp(weight, prevTransform.orientation, currTransform.orientation, true);

                // Apply to scene camera
                cameraTransform.camera->setPosition(interpPos);
                cameraTransform.camera->setOrientation(interpRot);
            }
        }

    }

    Ogre::Real RenderCommandQueueModule::calculateInterpolationWeight(void)
    {
        // Calculate weight based on accumulated time and frame time
        // This is critical for smooth interpolation
        Ogre::Real weight = this->accumTimeSinceLastLogicFrame / this->frameTime;

        // Clamp weight to [0,1]
        return std::min(1.0f, std::max(0.0f, weight));
    }

    void RenderCommandQueueModule::setAccumTimeSinceLastLogicFrame(Ogre::Real time)
    {
        this->accumTimeSinceLastLogicFrame = time;
    }

    Ogre::Real RenderCommandQueueModule::getAccumTimeSinceLastLogicFrame(void) const
    {
        return  this->accumTimeSinceLastLogicFrame;
    }

    void RenderCommandQueueModule::setFrameTime(Ogre::Real frameTime)
    {
        this->frameTime = frameTime;
    }

    void RenderCommandQueueModule::beginLogicFrame(void)
    {
        // Advance the transform buffer to the next buffer
        // This is called at the start of each logic frame
        this->advanceTransformBuffer();
    }

    void RenderCommandQueueModule::endLogicFrame(void)
    {
        // Currently empty - can be used for frame statistics or other end-of-frame operations
        if (true == this->debugVisualization)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[RenderCommandQueueModule]: Logic frame "
                + Ogre::StringConverter::toString(this->logicFrameCount) + " completed");
        }
    }

    RenderCommandQueueModule* RenderCommandQueueModule::getInstance()
    {
        static RenderCommandQueueModule instance;
        return &instance;
    }

    void RenderCommandQueueModule::setLogLevel(Ogre::LogMessageLevel level)
    {
        this->logLevel = level;
    }

    Ogre::LogMessageLevel RenderCommandQueueModule::getLogLevel(void) const
    {
        return this->logLevel;
    }

    void RenderCommandQueueModule::logCommandEvent(const Ogre::String& message, Ogre::LogMessageLevel level) const
    {
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

    void RenderCommandQueueModule::logCommandState(const char* commandName, bool willWait) const
    {
        std::stringstream ss;
        ss << "Command '" << commandName << "' - Will wait: " << (willWait ? "YES" : "NO");
        this->logCommandEvent(ss.str(), Ogre::LML_NORMAL);
    }

}; // namespace end