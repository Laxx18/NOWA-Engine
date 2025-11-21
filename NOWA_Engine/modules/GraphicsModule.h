#ifndef GRPAHICS_MODULE
#define GRPAHICS_MODULE

#include "defines.h"

#include <functional>
#include <queue>
#include <mutex>
#include <future>
#include <chrono>
#include <atomic>
#include <unordered_map>
#include <vector>
#include <array>
#include <thread>
#include <atomic>

#include "utilities/MathHelper.h"
#include "utilities/concurrentqueue.h"

#define NUM_TRANSFORM_BUFFERS 4

namespace NOWA
{
    namespace RenderGlobals
    {
        // Marked as inline (C++17), so each translation unit gets its own copy, but all copies are treated as one entity by the linker.
        inline thread_local int g_renderCommandDepth;
        inline thread_local int g_waitDepth;
        inline thread_local bool g_inLogicCommand = false;
    }

    /*
    *    @brief How the command queue does work:
    *    Main thread must wait, until the closure command is finished. If the closure command is called and inside that, there is another inner function, which also calls ENQUEUE_RENDER_COMMAND_WAIT, 
    *    it must not wait anymore, because the outer does already etc.
    *    Its necessary that wait macros do wait until the command in the front of the queue is processed completely! I must prevent, that queue has several waiting commands, 
    *    which have not been processed, because then hell breaks loose.
    *
    *    How This Buffer-Based Approach Works
    *    This implementation follows Ogre-Next's approach very closely. Here's how it works:
    *
    *    Multiple Transform Buffers: Each tracked node has multiple transform buffers (4 in this case), allowing for smooth transitions between logic frames.
    *    Buffer Advancement: The logic thread advances the current buffer before updating game logic. This preserves the previous frame's data for interpolation.
    *    Transform Copying: When advancing to a new buffer, the current transform values are copied to the new buffer. This ensures that if an object doesn't move during a frame, it maintains its position.
    *    Interpolation: The render thread calculates an interpolation weight based on time since the last logic frame and uses it to interpolate between the previous and current transform buffers.
    *
    *    Key Advantages
    *
    *    Thread Safety: Logic writes to one buffer, rendering reads from between two buffers. No locks needed!
    *    Triple Buffering Effect: With 4 buffers, we effectively have a triple-buffering approach (previous, current, and next), allowing for smooth transitions even with variable frame rates.
    *    Predictable Memory Usage: Unlike the previous approach, memory usage is fixed per node and doesn't grow over time.
    *    Efficient Updates: You only update the transforms that change, not every active transform every frame.
    *    Smooth Interpolation: Like Ogre-Next, this approach provides smooth interpolation between frames.
    *
    *    Implementation Tips
    *
    *    Add Node Tracking: The addTrackedNode method should be called early when nodes are created, so they're ready for interpolation when needed.
    *    Buffer Size: 4 buffers is typically enough, but you can adjust based on your needs.
    *    Transform Updates: Only update the components that change to minimize data movement.
    *    Memory Management: Consider using a custom allocator for the NodeTransforms vector if you have many objects.
    *
    * How the graphics resources destruction does work:
    * Logic thread (for physics/game logic).
    * Render thread (for reading transform data and drawing).
    * If an entity/item is deleted immediately on the logic thread, the render thread might still access its transform — leading to crashes or undefined behavior.
    * 
    * - Logic thread writes to logicWriteSlot.
    * - Render thread reads from (logicWriteSlot + NUM_SLOTS - 2) % NUM_SLOTS to destroy.
    * - Always skip 1–2 slots between write and destroy to ensure safety.
    * - No cross-thread access = no mutex needed.
    * 
    * A fixed-size ring buffer (e.g., 3 or 4 slots) is used to delay destruction by a few frames. Each frame:
    * The logic thread enqueues entities/items/resources to be destroyed in its current slot.
    * The render thread destroys the contents of a slot from two frames ago.
    * This guarantees no overlapping access, so no mutex is needed.
    * 
    * Why no mutex is needed?
    * Each thread only touches its own slot.
    * Communication is linear and strictly ordered per frame.
    * Delayed destruction happens in slots that are guaranteed to be unused by any thread.
    * Only one thread owns and modifies a slot at any time.
    */

	class EXPORTED GraphicsModule
	{
    public:

        friend class AppState;

        // Transform data structure - just stores the raw transform data
        struct TransformData
        {
            // Absolute components (interpolated)
            Ogre::Vector3     position = Ogre::Vector3::ZERO;
            Ogre::Quaternion  orientation = Ogre::Quaternion::IDENTITY;
            Ogre::Vector3     scale = Ogre::Vector3::UNIT_SCALE;
        };

        struct PassSpeedData
        {
            Ogre::Real speedsX[9] = {};
            Ogre::Real speedsY[9] = {};
        };

        struct CameraTransformData
        {
            // Absolute components (interpolated)
            Ogre::Vector3    position = Ogre::Vector3::ZERO;
            Ogre::Quaternion orientation = Ogre::Quaternion::IDENTITY;
        };

        // Node and its transform buffers
        struct NodeTransforms
        {
            Ogre::Node* node;
            TransformData transforms[NUM_TRANSFORM_BUFFERS];
            bool active = false;
            bool isNew = false;
            bool useDerived = false;
            bool updatedThisFrame = false;
            int stableFrames = 0;
        };

        struct CameraTransforms
        {
            Ogre::Camera* camera;
            CameraTransformData transforms[NUM_TRANSFORM_BUFFERS];
            bool active = false;
            bool isNew = false;
        };

        struct OldBoneTransforms
        {
            Ogre::v1::OldBone* oldBone;
            TransformData transforms[NUM_TRANSFORM_BUFFERS];
            bool active = false;
            bool isNew = false;
        };

        struct PassTransforms
        {
            Ogre::Pass* pass = nullptr;
            PassSpeedData transforms[NUM_TRANSFORM_BUFFERS];
            bool active = false;
            bool isNew = false;
        };

        struct TrackedDatablock
        {
            Ogre::HlmsDatablock* datablock;

            // Each buffer holds a value (e.g. ColourValue)
            Ogre::ColourValue values[NUM_TRANSFORM_BUFFERS];

            // The interpolation application function
            std::function<void(Ogre::ColourValue)> applyFunc;

            // Interpolation function between two values
            std::function<Ogre::ColourValue(const Ogre::ColourValue&, const Ogre::ColourValue&, Ogre::Real)> interpolateFunc;

            bool active = false;
            bool isNew = false;
        };

        // Closure command structure for the queue
        struct ClosureCommand
        {
            Ogre::String uniqueName;
            std::function<void(Ogre::Real)> closureFunc;
            bool fireAndForget = true;
            bool isUpdate = false;  // true for updates, false for new closures
            bool isRemoval = false; // true for removal commands

            ClosureCommand() = default;

            ClosureCommand(const Ogre::String& name, std::function<void(Ogre::Real)> func, bool fireForget = true, bool update = false, bool removal = false)
                : uniqueName(name)
                , closureFunc(std::move(func))
                , fireAndForget(fireForget)
                , isUpdate(update)
                , isRemoval(removal)
            {

            }
        };

        // Persistent closure for non-fire-and-forget closures
        struct PersistentClosure
        {
            Ogre::String uniqueName;
            std::function<void(Ogre::Real)> closureFunc;
            bool active = true;

            PersistentClosure() = default;

            PersistentClosure(const Ogre::String& name, std::function<void(Ogre::Real)> func)
                : uniqueName(name), closureFunc(std::move(func))
            {

            }
        };

        ////////////////////////////////////////////////////////////////

        using RenderCommand = std::function<void()>;

        using DestroyCommand = std::function<void()>;

        // For pull-based systems (e.g., render thread pops commands)
        struct CommandEntry
        {
            RenderCommand command;
            std::shared_ptr<std::promise<void>> completionPromise; // nullptr for fire-and-forget
            bool promiseAlreadyFulfilled = false;
        };
    public:
        void clearAllClosures(void);

        void clearSceneResources(void);

        void doCleanup(void);

        bool getIsRunning(void) const;

        void enqueue(RenderCommand&& command, const char* commandName, std::shared_ptr<std::promise<void>> promise = nullptr);

        bool pop(CommandEntry& commandEntry);

        bool hasPendingRenderCommands(void) const;

        void markPromiseFulfilled(const std::shared_ptr<std::promise<void>>& promise);

        bool wasPromiseFulfilled(const std::shared_ptr<std::promise<void>>& promise);

        void processAllCommands(void);

        void waitForRenderCompletion(void);

        void cleanupFulfilledPromises(void);

        void enqueueAndWait(RenderCommand&& command, const char* commandName);

        template <typename T>
        T enqueueAndWaitWithResult(std::function<T()> command, const char* commandName)
        {
            if (this->isRenderThread())
            {
                try
                {
                    this->logCommandEvent(std::string("Executing command '") + commandName + "' directly on render thread", Ogre::LML_TRIVIAL);
                    auto result = command();
                    this->isRunningWaitClosure = false;
                    return result;
                }
                catch (const std::exception& e)
                {
                    this->logCommandEvent(std::string("Exception in direct execution of '") + commandName + "': " + e.what(), Ogre::LML_CRITICAL);
                    throw;
                }
                catch (...)
                {
                    this->logCommandEvent(std::string("Unknown exception in direct execution of '") + commandName + "'", Ogre::LML_CRITICAL);
                    throw;
                }
            }

            this->incrementWaitDepth();

            try
            {
                auto promise = std::make_shared<std::promise<T>>();
                auto future = promise->get_future();

                bool isNested = isInNestedWait() && this->getWaitDepth() > 1;

                this->logCommandEvent(std::string("Enqueueing command '") + commandName + "' with wait and result", Ogre::LML_NORMAL);

                if (isNested)
                {
                    this->logCommandEvent(std::string("Command '") + commandName + "' is a nested wait (depth: " + std::to_string(this->getWaitDepth()) + ")", Ogre::LML_NORMAL);
                }

                // Enqueue the command with the promise - adjust if your enqueue supports passing promise
                this->enqueue([command, promise]()
                {
                    try
                    {
                        promise->set_value(command());
                    }
                    catch (...)
                    {
                        promise->set_exception(std::current_exception());
                    }
                }, commandName);

                if (!isNested)
                {
                    if (this->isTimeoutEnabled())
                    {
                        auto timeout = getTimeoutDuration();
                        auto status = future.wait_for(timeout);

                        if (status == std::future_status::timeout)
                        {
                            this->logCommandEvent(std::string("Timeout occurred waiting for command '") + commandName + "'", Ogre::LML_CRITICAL);
                            this->processQueueSync();
                        }
                    }
                    else
                    {
                        future.wait();
                    }
                }
                else
                {
                    this->logCommandEvent(std::string("Nested command '") + commandName + "' enqueued without blocking wait", Ogre::LML_NORMAL);
                    // For nested waits: we don't wait here, outer wait will handle it.
                }

                // Get the result after waiting (or immediately if nested)
                T result = future.get();

                this->decrementWaitDepth();
                this->logCommandEvent(std::string("Command '") + commandName + "' completed with result", Ogre::LML_TRIVIAL);
                this->isRunningWaitClosure = false;

                return result;
            }
            catch (...)
            {
                this->decrementWaitDepth();
                throw;
            }
        }

        void processQueueSync(void);

        /*
        * Bulletproof solution for nested render thread commands — in a large codebase (400,000+ LOC), accidental misuse of a promise-based render command inside another queued render command can easily cause deadlocks.
        * The Problem:
        *   When entry.command() is running on the render thread, and inside that command, another command is enqueued with a promise, then wait() is called from the render thread. The render thread waits for itself => deadlock.
        *   The Solution: Detect and immediately execute nested commands if already on the render thread. 
        *   You need to make your queue system smart enough to detect that a command is enqueued from the render thread itself, and then run it directly synchronously, bypassing the queue and the promise.
        */

        // Sets the render thread ID
        void markCurrentThreadAsRenderThread(void);

        // Checks if we are on the render thread
        bool isRenderThread(void) const;
       
        void incrementWaitDepth(void);

        void decrementWaitDepth(void);

        int getWaitDepth(void) const;

        bool isInNestedWait(void) const;

        // Timeout configuration methods
        void enableTimeout(bool enable);

        bool isTimeoutEnabled(void) const;

        void setTimeoutDuration(std::chrono::milliseconds duration);

        std::chrono::milliseconds getTimeoutDuration(void) const;

        void setLogLevel(Ogre::LogMessageLevel level);

        Ogre::LogMessageLevel getLogLevel(void) const;

        void logCommandEvent(const Ogre::String& message, Ogre::LogMessageLevel level) const;

        void logCommandState(const char* commandName, bool willWait) const;

        void recoverFromTimeout(void);

        bool waitForFutureWithTimeout(std::future<void>& future, const std::chrono::milliseconds& timeout, const char* commandName);

        // Add a node to be tracked and transformed
        void addTrackedNode(Ogre::Node* node);

        // Remove a node from tracking
        void removeTrackedNode(Ogre::Node* node);

        // Update position for a node in the current buffer
        void updateNodePosition(Ogre::Node* node, const Ogre::Vector3& position, bool useDerived = false);

        // Update orientation for a node in the current buffer
        void updateNodeOrientation(Ogre::Node* node, const Ogre::Quaternion& orientation, bool useDerived = false);

        // Update scale for a node in the current buffer
        void updateNodeScale(Ogre::Node* node, const Ogre::Vector3& scale, bool useDerived = false);

        // Update full transform for a node in the current buffer
        void updateNodeTransform(Ogre::Node* node, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, const Ogre::Vector3& scale = Ogre::Vector3::UNIT_SCALE, bool useDerived = false);

        // Add a Camera to be tracked and transformed
        void addTrackedCamera(Ogre::Camera* camera);

        // Remove a Camera from tracking
        void removeTrackedCamera(Ogre::Camera* camera);

        // Update position for a Camera in the current buffer
        void updateCameraPosition(Ogre::Camera* camera, const Ogre::Vector3& position);

        // Update orientation for a Camera in the current buffer
        void updateCameraOrientation(Ogre::Camera* camera, const Ogre::Quaternion& orientation);

        // Update full transform for a Camera in the current buffer
        void updateCameraTransform(Ogre::Camera* camera, const Ogre::Vector3& position, const Ogre::Quaternion& orientation);

        // Add a OldBone to be tracked and transformed
        void addTrackedOldBone(Ogre::v1::OldBone* oldBone);

        // Remove a OldBone from tracking
        void removeTrackedOldBone(Ogre::v1::OldBone* oldBone);

        // Update position for a OldBone in the current buffer
        void updateOldBonePosition(Ogre::v1::OldBone* oldBone, const Ogre::Vector3& position);

        // Update orientation for a OldBone in the current buffer
        void updateOldBoneOrientation(Ogre::v1::OldBone* oldBone, const Ogre::Quaternion& orientation);

        // Update full transform for a OldBone in the current buffer
        void updateOldBoneTransform(Ogre::v1::OldBone* oldBone, const Ogre::Vector3& position, const Ogre::Quaternion& orientation);

        // Add a Pass to be tracked and transformed
        void addTrackedPass(Ogre::Pass* pass);

        // Remove a Pass from tracking
        void removeTrackedPass(Ogre::Pass* pass);

        // Update speed x for a Pass in the current buffer
        void updatePassSpeedsX(Ogre::Pass* pass, unsigned short index, Ogre::Real speedX);

        // Update speed y for a Pass in the current buffer
        void updatePassSpeedsY(Ogre::Pass* pass, unsigned short index, Ogre::Real speedY);

        void addTrackedDatablock(Ogre::HlmsDatablock* datablock, const Ogre::ColourValue& initialValue, std::function<void(Ogre::ColourValue)> applyFunc,
            std::function<Ogre::ColourValue(const Ogre::ColourValue&, const Ogre::ColourValue&, Ogre::Real)> interpFunc);

        // Remove a Datablock from tracking
        void removeTrackedDatablock(Ogre::HlmsDatablock* datablock);

        // Update colour changes on datablock
        void updateTrackedDatablockValue(Ogre::HlmsDatablock* datablock, const Ogre::ColourValue& initialValue, const Ogre::ColourValue& targetValue, std::function<void(Ogre::ColourValue)> applyFunc,
            std::function<Ogre::ColourValue(const Ogre::ColourValue&, const Ogre::ColourValue&, Ogre::Real)> interpFunc);

        // Advance to the next transform buffer (called by logic system)
        void advanceTransformBuffer(void);

        // Update all transforms with interpolation weight (called by render thread)
        void updateAllTransforms(void);

        // Calculate interpolation weight based on time since last logic frame
        void calculateInterpolationWeight(void);

        // Set the accumulated time since the last logic frame
        void setAccumTimeSinceLastLogicFrame(Ogre::Real time);

        // Get the accumulated time since the last logic frame
        Ogre::Real getAccumTimeSinceLastLogicFrame(void) const;

        void setFrameTime(Ogre::Real frameTime);

        void beginLogicFrame(void);

        void endLogicFrame(void);

        size_t getPreviousTransformNodeIdx(void) const;

        size_t getPreviousTransformCameraIdx(void) const;

        size_t getPreviousTransformOldBoneIdx(void) const;

        size_t getPreviousTransformPassIdx(void) const;

        size_t getPreviousTrackedDatablockIdx(void) const;

        void enableDebugVisualization(bool enable);

        void dumpBufferState(void) const;

        void enqueueDestroy(DestroyCommand destroyCommand, const char* commandName);

        void advanceFrameAndDestroyOld(void);

        bool hasPendingDestroyCommands(void) const;

        /*
        * // Example of persistent closure (non-fire-and-forget)
        *    void SomeClass::setupPersistentEffect()
        *    {
        *        auto persistentClosure = [this](Ogre::Real weight)
        *        {
        *            // This closure will be executed every frame until removed
        *            this->updateSomeEffect(weight);
        *        };
        *
        *        Ogre::String id = "SomeClass::persistentEffect";
        *        // false = not fire-and-forget, will persist until explicitly removed
        *        NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, persistentClosure, false);
        *    }
        *
        *    // Remove persistent closure when no longer needed
        *    void SomeClass::cleanup()
        *    {
        *        NOWA::GraphicsModule::getInstance()->removeTrackedClosure("SomeClass::persistentEffect");
        *    }
        */
        // Called from logic thread - completely lock-free
        void updateTrackedClosure(const Ogre::String& uniqueName, std::function<void(Ogre::Real)> closureFunc, bool fireAndForget = true);

        // Called from logic thread - completely lock-free
        void removeTrackedClosure(const Ogre::String& uniqueName);

        // Called from render thread - processes all queued commands
        void processClosureCommands(Ogre::Real interpolationWeight);

        // Called from render thread - executes all active closures
        void executeActiveClosures(Ogre::Real interpolationWeight);

        // Combined method called from render thread
        void updateAndExecuteClosures(Ogre::Real interpolationWeight);
	public:
        static constexpr size_t NUM_DESTROY_SLOTS = 4;

        static GraphicsModule* getInstance();
    private:
        GraphicsModule();

        ~GraphicsModule();

        void startRendering(void);

        void stopRendering(void);

        void renderThreadFunction(void);

        // Find a node's transform record
        NodeTransforms* findNodeTransforms(Ogre::Node* node);

        CameraTransforms* findCameraTransforms(Ogre::Camera* camera);

        OldBoneTransforms* findOldBoneTransforms(Ogre::v1::OldBone* oldBone);

        PassTransforms* findPassTransforms(Ogre::Pass* pass);

        TrackedDatablock* findTrackedDatablock(Ogre::HlmsDatablock* datablock);

        // Internal helper methods (render thread only)
        void processSingleCommand(const ClosureCommand& command, Ogre::Real interpolationWeight);
        void addPersistentClosure(const Ogre::String& uniqueName, std::function<void(Ogre::Real)> closureFunc);
        void updatePersistentClosure(const Ogre::String& uniqueName, std::function<void(Ogre::Real)> closureFunc);
        void removePersistentClosure(const Ogre::String& uniqueName);

	private:
        std::thread renderThread;
        bool bRunning;

        std::atomic<std::thread::id> renderThreadId;
        std::mutex mutex;
        moodycamel::ConcurrentQueue<CommandEntry> queue;
        std::mutex fulfilledMutex;
        std::set<std::shared_ptr<std::promise<void>>> fulfilledPromises;

        std::atomic<bool> timeoutEnabled;
        std::atomic<std::chrono::milliseconds::rep> timeoutDuration;
        std::atomic<Ogre::LogMessageLevel> logLevel;

        std::vector<NodeTransforms> trackedNodes;
        std::unordered_map<Ogre::Node*, size_t> nodeToIndexMap;

        std::vector<CameraTransforms> trackedCameras;
        std::unordered_map<Ogre::Camera*, size_t> cameraToIndexMap;

        std::vector<OldBoneTransforms> trackedOldBones;
        std::unordered_map<Ogre::v1::OldBone*, size_t> oldBoneToIndexMap;

        std::vector<PassTransforms> trackedPasses;
        std::unordered_map<Ogre::Pass*, size_t> passToIndexMap;

        std::vector<TrackedDatablock> trackedDatablocks;
        std::unordered_map<Ogre::HlmsDatablock*, size_t> datablockToIndexMap;

        // Lock-free concurrent queue for closure commands
        moodycamel::ConcurrentQueue<ClosureCommand> closureQueue;

        // Storage for persistent closures (only accessed by render thread)
        std::unordered_map<Ogre::String, PersistentClosure> persistentClosures;

        // Producer token for better performance (used by logic thread)
        moodycamel::ProducerToken producerToken;

        // Consumer token for better performance (used by render thread)  
        moodycamel::ConsumerToken consumerToken;

        // Atomic flag to ensure proper initialization
        std::atomic<bool> queueInitialized{ false };

        size_t currentTransformNodeIdx;
        size_t currentTransformCameraIdx;
        size_t currentTransformOldBoneIdx;
        size_t currentTransformPassIdx;
        size_t currentTrackedDatablockIdx;
        Ogre::Real interpolationWeight;

        Ogre::Real accumTimeSinceLastLogicFrame;
        Ogre::Real frameTime;
        bool debugVisualization;

        size_t currentDestroySlot;
        std::array<std::vector<DestroyCommand>, NUM_DESTROY_SLOTS> destroySlots;
        std::atomic<bool> isRunningWaitClosure;
	};

}; // namespace end

// Defines macros for the capture list
#define _1(var1) var1
#define _2(var1, var2) var1, var2
#define _3(var1, var2, var3) var1, var2, var3
#define _4(var1, var2, var3, var4) var1, var2, var3, var4
#define _5(var1, var2, var3, var4, var5) var1, var2, var3, var4, var5
#define _6(var1, var2, var3, var4, var5, var6) var1, var2, var3, var4, var5, var6
#define _7(var1, var2, var3, var4, var5, var6, var7) var1, var2, var3, var4, var5, var6, var7
#define _8(var1, var2, var3, var4, var5, var6, var7, var8) var1, var2, var3, var4, var5, var6, var7, var8
#define _9(var1, var2, var3, var4, var5, var6, var7, var8, var9) var1, var2, var3, var4, var5, var6, var7, var8, var9
#define _10(var1, var2, var3, var4, var5, var6, var7, var8, var9, var10) var1, var2, var3, var4, var5, var6, var7, var8, var9, var10
#define _11(var1, var2, var3, var4, var5, var6, var7, var8, var9, var10, var11) var1, var2, var3, var4, var5, var6, var7, var8, var9, var10, var11
#define _12(var1, var2, var3, var4, var5, var6, var7, var8, var9, var10, var11, var12) var1, var2, var3, var4, var5, var6, var7, var8, var9, var10, var11, var12
#define _13(var1, var2, var3, var4, var5, var6, var7, var8, var9, var10, var11, var12, var13) var1, var2, var3, var4, var5, var6, var7, var8, var9, var10, var11, var12, var13
#define _14(var1, var2, var3, var4, var5, var6, var7, var8, var9, var10, var11, var12, var13, var14) var1, var2, var3, var4, var5, var6, var7, var8, var9, var10, var11, var12, var13, var14
#define _15(var1, var2, var3, var4, var5, var6, var7, var8, var9, var10, var11, var12, var13, var14, var15) var1, var2, var3, var4, var5, var6, var7, var8, var9, var10, var11, var12, var13, var14, var15
#define _16(var1, var2, var3, var4, var5, var6, var7, var8, var9, var10, var11, var12, var13, var14, var15, var16) var1, var2, var3, var4, var5, var6, var7, var8, var9, var10, var11, var12, var13, var14, var15, var16
#define _17(var1, var2, var3, var4, var5, var6, var7, var8, var9, var10, var11, var12, var13, var14, var15, var16, var17) var1, var2, var3, var4, var5, var6, var7, var8, var9, var10, var11, var12, var13, var14, var15, var16, var17
#define _18(var1, var2, var3, var4, var5, var6, var7, var8, var9, var10, var11, var12, var13, var14, var15, var16, var17, var18) var1, var2, var3, var4, var5, var6, var7, var8, var9, var10, var11, var12, var13, var14, var15, var16, var17, var18
#define _19(var1, var2, var3, var4, var5, var6, var7, var8, var9, var10, var11, var12, var13, var14, var15, var16, var17, var18, var19) var1, var2, var3, var4, var5, var6, var7, var8, var9, var10, var11, var12, var13, var14, var15, var16, var17, var18, var19

/*
* Info:
* For non-blocking commands, the logic is a bit different since these macros are not waiting for a promise to resolve. 
* However, to ensure safety and correctness, it's still beneficial to check if we're already on the render thread and execute the command immediately, 
* as in the previous case. This would avoid unnecessary queuing and improve performance if the logic is already running on the render thread.
*/
//
///* Example usage:
//* 		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("Class::yourFunctionForDebugLogging", _2(position, rotation), {
//*			this->sceneNode->_setDerivedPosition(position);
//*			this->sceneNode->_setDerivedOrientation(rotation);
//*		});
//*
//* Its not blocking the logic thread
//*/
//
//
//
///**
// *
// * Usage examples:
// *   ENQUEUE_RENDER_COMMAND("Class::yourFunctionForDebugLogging",
// *       this->sceneNode->updateSomething();
// *       this->performRenderOperation();
// *   });
// *
// *   ENQUEUE_RENDER_COMMAND_NO_CAPTURE({
// *      // Static functions or operations that don't require object state
// *      SomeClass::staticMethod();
// *      performGlobalOperation();
// *   });
// */
//
///*
//*  Note about blocking, wait (promise):
//* 
//* - Immediate execution when already on the render thread.
//* - Queued + waited execution when on any other thread.
//* - Nested safety by design (no deadlock).
//* - Exception propagation works as before.
//*/
//

 /**
 * Example Usage:
 * ENQUEUE_RENDER_COMMAND_MULTI_WAIT("", _6(name, templateName, playTimeMS, orientation, position, scale),
 *  {
 *    ...
 *  });
 *
 * It will block this code til finished on render thread and then the logic thread goes on
 */

#define ENQUEUE_RENDER_COMMAND_WAIT(command_name, lambda_body) \
{ \
    NOWA::GraphicsModule::RenderCommand renderCommand = [this]() { \
        try { \
            lambda_body \
        } catch (const std::exception& e) { \
            NOWA::GraphicsModule::getInstance()->logCommandEvent( \
                Ogre::String("Exception in command '") + command_name + "': " + e.what(), \
                Ogre::LML_CRITICAL \
            ); \
            throw; /* Re-throw to propagate to the promise */ \
        } catch (...) { \
            NOWA::GraphicsModule::getInstance()->logCommandEvent( \
                Ogre::String("Unknown exception in command '") + command_name + "'", \
                Ogre::LML_CRITICAL \
            ); \
            throw; /* Re-throw to propagate to the promise */ \
        } \
    }; \
    \
    NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), command_name); \
}

 // Multi-capture wait macro with timeout, logging, and depth tracking
#define ENQUEUE_RENDER_COMMAND_MULTI_WAIT(command_name, capture_macro, lambda_body) \
{ \
    NOWA::GraphicsModule::RenderCommand renderCommand = [this, capture_macro]() { \
        try { \
            lambda_body \
        } catch (const std::exception& e) { \
            NOWA::GraphicsModule::getInstance()->logCommandEvent( \
                Ogre::String("Exception in command '") + command_name + "': " + e.what(), \
                Ogre::LML_CRITICAL \
            ); \
            throw; /* Re-throw to propagate to the promise */ \
        } catch (...) { \
            NOWA::GraphicsModule::getInstance()->logCommandEvent( \
                Ogre::String("Unknown exception in command '") + command_name + "'", \
                Ogre::LML_CRITICAL \
            ); \
            throw; /* Re-throw to propagate to the promise */ \
        } \
    }; \
    \
    NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), command_name); \
}

// Non-waiting command macros (for completeness)
#define ENQUEUE_RENDER_COMMAND(command_name, lambda_body) \
{ \
    NOWA::GraphicsModule::RenderCommand renderCommand = [this]() { \
        try { \
            lambda_body \
        } catch (const std::exception& e) { \
            NOWA::GraphicsModule::getInstance()->logCommandEvent( \
                Ogre::String("Exception in command '") + command_name + "': " + e.what(), \
                Ogre::LML_CRITICAL \
            ); \
            throw; /* Re-throw to propagate to the promise */ \
        } catch (...) { \
            NOWA::GraphicsModule::getInstance()->logCommandEvent( \
                Ogre::String("Unknown exception in command '") + command_name + "'", \
                Ogre::LML_CRITICAL \
            ); \
            throw; /* Re-throw to propagate to the promise */ \
        } \
    }; \
    \
    NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), command_name); \
}


#define ENQUEUE_RENDER_COMMAND_MULTI(command_name, capture_macro, lambda_body) \
{ \
    NOWA::GraphicsModule::RenderCommand renderCommand = [this, capture_macro]() { \
        try { \
            lambda_body \
        } catch (const std::exception& e) { \
            NOWA::GraphicsModule::getInstance()->logCommandEvent( \
                Ogre::String("Exception in command '") + command_name + "': " + e.what(), \
                Ogre::LML_CRITICAL \
            ); \
            throw; /* Re-throw to propagate to the promise */ \
        } catch (...) { \
            NOWA::GraphicsModule::getInstance()->logCommandEvent( \
                Ogre::String("Unknown exception in command '") + command_name + "'", \
                Ogre::LML_CRITICAL \
            ); \
            throw; /* Re-throw to propagate to the promise */ \
        } \
    }; \
    \
    NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), command_name); \
}

/////////////////No This//////////////////////////////////////////////////////

 // Multi-capture wait macro with timeout, logging, and depth tracking
#define ENQUEUE_RENDER_COMMAND_MULTI_WAIT_NO_THIS(command_name, capture_macro, lambda_body) \
{ \
    NOWA::GraphicsModule::RenderCommand renderCommand = [capture_macro]() { \
        try { \
            lambda_body \
        } catch (const std::exception& e) { \
            NOWA::GraphicsModule::getInstance()->logCommandEvent( \
                Ogre::String("Exception in command '") + command_name + "': " + e.what(), \
                Ogre::LML_CRITICAL \
            ); \
            throw; /* Re-throw to propagate to the promise */ \
        } catch (...) { \
            NOWA::GraphicsModule::getInstance()->logCommandEvent( \
                Ogre::String("Unknown exception in command '") + command_name + "'", \
                Ogre::LML_CRITICAL \
            ); \
            throw; /* Re-throw to propagate to the promise */ \
        } \
    }; \
    \
    NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), command_name); \
}

#define ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS(command_name, capture_macro, lambda_body) \
{ \
    NOWA::GraphicsModule::RenderCommand renderCommand = [capture_macro]() { \
        try { \
            lambda_body \
        } catch (const std::exception& e) { \
            NOWA::GraphicsModule::getInstance()->logCommandEvent( \
                Ogre::String("Exception in command '") + command_name + "': " + e.what(), \
                Ogre::LML_CRITICAL \
            ); \
            throw; /* Re-throw to propagate to the promise */ \
        } catch (...) { \
            NOWA::GraphicsModule::getInstance()->logCommandEvent( \
                Ogre::String("Unknown exception in command '") + command_name + "'", \
                Ogre::LML_CRITICAL \
            ); \
            throw; /* Re-throw to propagate to the promise */ \
        } \
    }; \
    \
    NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), command_name); \
}


#define ENQUEUE_DESTROY_COMMAND(command_name, capture_macro, lambda_body) \
{ \
    NOWA::GraphicsModule::DestroyCommand destroyCommand = [capture_macro]() mutable { \
        try { \
            lambda_body \
        } catch (const std::exception& e) { \
            NOWA::GraphicsModule::getInstance()->logCommandEvent( \
                Ogre::String("Exception in destroy command '") + command_name + "': " + e.what(), \
                Ogre::LML_CRITICAL \
            ); \
            throw; /* Re-throw to propagate to the promise */ \
        } catch (...) { \
            NOWA::GraphicsModule::getInstance()->logCommandEvent( \
                Ogre::String("Unknown exception in destroy command '") + command_name + "'", \
                Ogre::LML_CRITICAL \
            ); \
        } \
    }; \
    \
    auto* graphicsModule = NOWA::GraphicsModule::getInstance(); \
    if (graphicsModule->isRenderThread()) { \
        destroyCommand(); \
    } else { \
        graphicsModule->enqueueDestroy(destroyCommand, command_name); \
    } \
}
/*
* Usage:
* ENQUEUE_DESTROY_COMMAND("DestroySceneNode", _2(sceneManager, sceneNode), {
*    sceneManager->destroySceneNode(sceneNode);
* });
* 
* Note: Take care in using this! E.g. if you are in a destructor, create copies and use them in closure and do not use this, 
* as your parent class gone after the enqueue, but the destroy command is called in another thread later.
* Only use this at most atomic level! For directly destroying Ogre resources and not on function, which do call other functions,
* in which you cannot quarantee, that there is not also a cascading wait command or other destroy command!
*/

#define ENQUEUE_RAYCAST1(raySceneQuery, camera, resultPosition, excludeObjects, successVar) \
    bool successVar = NOWA::GraphicsModule::getInstance()->enqueueAndWaitWithResult<bool>([&]() -> bool \
    { \
        auto mathHelper = MathHelper::getInstance(); \
        return mathHelper->getRaycastFromPoint(raySceneQuery, camera, resultPosition, excludeObjects); \
    }, "ENQUEUE_RAYCAST1");

// The do { ... } while(0) trick is mostly a C/C++ macro idiom to ensure that the macro behaves like a single statement 
// (so you can safely put a semicolon after it and use it inside if statements without braces). But if your style prefers no do-while, that’s totally fine.
#define ENQUEUE_RAYCAST2(mouseX, mouseY, excludeVec, resultPos, successVar) \
    do { \
        Ogre::Vector3 resultPos##Local = Ogre::Vector3::ZERO; \
        int mouseX##Local = (mouseX); \
        int mouseY##Local = (mouseY); \
        successVar = NOWA::GraphicsModule::getInstance()->enqueueAndWaitWithResult<bool>( \
            [=, &resultPos##Local, &excludeVec]() -> bool { \
                auto camera = NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera(); \
                auto renderWindow = NOWA::Core::getSingletonPtr()->getOgreRenderWindow(); \
                auto raySceneQuery = this->raySceneQuery; \
                auto mathHelper = NOWA::MathHelper::getInstance(); \
                return mathHelper->getRaycastFromPoint(mouseX##Local, mouseY##Local, camera, renderWindow, raySceneQuery, resultPos##Local, &excludeVec); \
            }, "ENQUEUE_RAYCAST2"); \
        if (successVar) { \
            (resultPos) = resultPos##Local; \
        } \
    } while(0)

#define ENQUEUE_RAYCAST3(raySceneQuery, camera, excludeVec, clickedPos, movableObj, closestDist, normalVec, successVar) \
    Ogre::Vector3 clickedPos##Local = Ogre::Vector3::ZERO; \
    size_t movableObj##Local = 0; \
    float closestDist##Local = 0.0f; \
    Ogre::Vector3 normalVec##Local = Ogre::Vector3::ZERO; \
    bool successVar = NOWA::GraphicsModule::getInstance()->enqueueAndWaitWithResult<bool>( \
        [=, &clickedPos##Local, &movableObj##Local, &closestDist##Local, &normalVec##Local, &excludeVec]() -> bool { \
            auto mathHelper = NOWA::MathHelper::getInstance(); \
            return mathHelper->getRaycastFromPoint(raySceneQuery, camera, clickedPos##Local, movableObj##Local, closestDist##Local, normalVec##Local, &excludeVec, false); \
        }, "ENQUEUE_RAYCAST3"); \
    if (successVar) { \
        (clickedPos) = clickedPos##Local; \
        (movableObj) = movableObj##Local; \
        (closestDist) = closestDist##Local; \
        (normalVec) = normalVec##Local; \
    }

#define ENQUEUE_RAYCAST4(mouseX, mouseY, excludeVec, clickedPos, closestDist, normalVec, movableObj, successVar) \
    Ogre::Vector3 clickedPos##Local = Ogre::Vector3::ZERO; \
    Ogre::Vector3 normal##Local = Ogre::Vector3::ZERO; \
    float closestDist##Local = 0.0f; \
    size_t movableObj##Local = 0; \
    int mouseX##Local = (mouseX); \
    int mouseY##Local = (mouseY); \
    bool successVar = NOWA::GraphicsModule::getInstance()->enqueueAndWaitWithResult<bool>( \
        [=, &clickedPos##Local, &normal##Local, &closestDist##Local, &movableObj##Local, &excludeVec]() -> bool { \
            auto camera = NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera(); \
            auto renderWindow = NOWA::Core::getSingletonPtr()->getOgreRenderWindow(); \
            auto raySceneQuery = this->raySceneQuery; \
            auto mathHelper = NOWA::MathHelper::getInstance(); \
            return mathHelper->getRaycastFromPoint(mouseX##Local, mouseY##Local, camera, renderWindow, raySceneQuery, clickedPos##Local, movableObj##Local, closestDist##Local, normal##Local, &excludeVec, false); \
        }, "ENQUEUE_RAYCAST4"); \
    if (successVar) { \
        (clickedPos) = clickedPos##Local; \
        (closestDist) = closestDist##Local; \
        (normalVec) = normal##Local; \
        (movableObj) = reinterpret_cast<Ogre::MovableObject*>(movableObj##Local); \
    }

#define ENQUEUE_RAYCAST5(mouseX, mouseY, camera, renderWindow, raySceneQuery, excludeVec, \
                        outResultPos, outMovableObj, outClosestDist, outNormalVec, successVar) \
    Ogre::Vector3 outResultPos##Local = Ogre::Vector3::ZERO; \
    size_t outMovableObj##Local = 0; \
    float outClosestDist##Local = 0.0f; \
    Ogre::Vector3 outNormalVec##Local = Ogre::Vector3::ZERO; \
    int mouseX##Local = (mouseX); \
    int mouseY##Local = (mouseY); \
    successVar = NOWA::GraphicsModule::getInstance()->enqueueAndWaitWithResult<bool>( \
        [=, &outResultPos##Local, &outMovableObj##Local, &outClosestDist##Local, &outNormalVec##Local, &excludeVec]() -> bool { \
            return NOWA::MathHelper::getInstance()->getRaycastFromPoint( \
                mouseX##Local, mouseY##Local, camera, renderWindow, raySceneQuery, \
                outResultPos##Local, outMovableObj##Local, outClosestDist##Local, outNormalVec##Local, &excludeVec, false); \
        }, "ENQUEUE_RAYCAST5"); \
    if (successVar) { \
        (outResultPos) = outResultPos##Local; \
        (outMovableObj) = reinterpret_cast<Ogre::MovableObject*>(outMovableObj##Local); \
        (outClosestDist) = outClosestDist##Local; \
        (outNormalVec) = outNormalVec##Local; \
    }

#define ENQUEUE_GET_MESH_INFO_WITH_SKELETON(entity, vertexCount, vertices, indexCount, indices, position, orientation, scale) \
    NOWA::GraphicsModule::getInstance()->enqueueAndWait( \
        [=, &vertexCount, &vertices, &indexCount, &indices]() { \
            NOWA::MathHelper::getInstance()->getMeshInformationWithSkeleton( \
                entity, vertexCount, vertices, indexCount, indices, position, orientation, scale); \
        }, "ENQUEUE_GET_MESH_INFO_WITH_SKELETON");

#define ENQUEUE_GET_MESH_INFO(meshPtr, vertexCount, vertexBuffer, indexCount, indexBuffer, position, orientation, scale) \
    NOWA::GraphicsModule::getInstance()->enqueueAndWait( \
        [=, &vertexCount, &vertexBuffer, &indexCount, &indexBuffer]() { \
            NOWA::MathHelper::getInstance()->getMeshInformation( \
                meshPtr, vertexCount, vertexBuffer, indexCount, indexBuffer, position, orientation, scale); \
        }, "ENQUEUE_GET_MESH_INFO");

/*
* Usage:
* size_t vertexCount = 0;
* Ogre::Vector3* vertexBuffer = nullptr;
* size_t indexCount = 0;
* unsigned long* indexBuffer = nullptr;
*
* Ogre::v1::MeshPtr mesh = someEntity->getMesh();
* Ogre::Vector3 pos = ...;
* Ogre::Quaternion rot = ...;
* Ogre::Vector3 scale = ...;
*
* ENQUEUE_GET_MESH_INFO(mesh, vertexCount, vertexBuffer, indexCount, indexBuffer, pos, rot, scale);
*
*/

#define ENQUEUE_GET_MESH_INFO2(meshPtr, vertexCount, vertexBuffer, indexCount, indexBuffer, position, orientation, scale) \
    NOWA::GraphicsModule::getInstance()->enqueueAndWait( \
        [=, &vertexCount, &vertexBuffer, &indexCount, &indexBuffer]() { \
            NOWA::MathHelper::getInstance()->getMeshInformation2( \
                meshPtr, vertexCount, vertexBuffer, indexCount, indexBuffer, position, orientation, scale); \
        }, "ENQUEUE_GET_MESH_INFO2");

#define ENQUEUE_GET_DETAILED_MESH_INFO2(meshPtr, vertexCount, vertexBuffer, indexCount, indexBuffer, position, orientation, scale, isHalf4, isIdx32) \
    NOWA::GraphicsModule::getInstance()->enqueueAndWait( \
        [=, &vertexCount, &vertexBuffer, &indexCount, &indexBuffer, &isHalf4, &isIdx32]() { \
            NOWA::MathHelper::getInstance()->getDetailedMeshInformation2( \
                meshPtr, vertexCount, vertexBuffer, indexCount, indexBuffer, position, orientation, scale, isHalf4, isIdx32); \
        }, "ENQUEUE_GET_DETAILED_MESH_INFO2");

#define ENQUEUE_GET_DETAILED_MESH_INFO2_FULL(meshPtr, vertexCount, vertexBuffer, normalBuffer, texCoordBuffer, indexCount, indexBuffer, position, orientation, scale, isHalf4, isIdx32) \
    NOWA::GraphicsModule::getInstance()->enqueueAndWait( \
        [=, &vertexCount, &vertexBuffer, &normalBuffer, &texCoordBuffer, &indexCount, &indexBuffer, &isHalf4, &isIdx32]() { \
            NOWA::MathHelper::getInstance()->getDetailedMeshInformation2( \
                meshPtr, vertexCount, vertexBuffer, normalBuffer, texCoordBuffer, \
                indexCount, indexBuffer, position, orientation, scale, isHalf4, isIdx32); \
        }, "ENQUEUE_GET_DETAILED_MESH_INFO2_FULL");

#define ENQUEUE_GET_MANUAL_MESH_INFO(manualObj, vertexCount, vertexBuffer, indexCount, indexBuffer, position, orientation, scale) \
    NOWA::GraphicsModule::getInstance()->enqueueAndWait( \
        [=, &vertexCount, &vertexBuffer, &indexCount, &indexBuffer]() { \
            NOWA::MathHelper::getInstance()->getManualMeshInformation( \
                manualObj, vertexCount, vertexBuffer, indexCount, indexBuffer, \
                position, orientation, scale); \
        }, "ENQUEUE_GET_MANUAL_MESH_INFO");

#define ENQUEUE_GET_MANUAL_MESH_INFO2(manualObj, vertexCount, vertexBuffer, indexCount, indexBuffer, position, orientation, scale) \
    NOWA::GraphicsModule::getInstance()->enqueueAndWait( \
        [=, &vertexCount, &vertexBuffer, &indexCount, &indexBuffer]() { \
            NOWA::MathHelper::getInstance()->getManualMeshInformation2( \
                manualObj, vertexCount, vertexBuffer, indexCount, indexBuffer, \
                position, orientation, scale); \
        }, "ENQUEUE_GET_MANUAL_MESH_INFO2");


#define ENQUEUE_GET_RAYCAST_HEIGHT(xPos, zPos, excludeVec, heightVar, successVar) \
    Ogre::Real height##Local = 0.0f; \
    Ogre::Real x##Local = (xPos); \
    Ogre::Real z##Local = (zPos); \
    bool successVar = NOWA::GraphicsModule::getInstance()->enqueueAndWaitWithResult<bool>( \
        [=, &height##Local, &excludeVec]() -> bool { \
            auto raySceneQuery = this->raySceneQuery; \
            return NOWA::MathHelper::getInstance()->getRaycastHeight( \
                x##Local, z##Local, raySceneQuery, height##Local, &excludeVec); \
        }, "getRaycastHeight::enqueueAndWaitWithResult"); \
    if (successVar) { \
        (heightVar) = height##Local; \
    }


/*
* Usage:
* Ogre::Real height = 0.0f;
* bool success = false;
*
* ENQUEUE_GET_RAYCAST_HEIGHT(100.0f, 200.0f, myExcludeVec, height, success);
* 
* if (success) {
*     // Use `height`
* }
*/ 

#define ENQUEUE_GET_RAYCAST_RESULT(originVec, directionVec, excludeVec, resultVec, movableObj, successVar) \
    Ogre::Vector3 resultVec##Local = Ogre::Vector3::ZERO; \
    size_t movableObj##Local = 0; \
    Ogre::Vector3 origin##Local = (originVec); \
    Ogre::Vector3 direction##Local = (directionVec); \
    bool successVar = NOWA::GraphicsModule::getInstance()->enqueueAndWaitWithResult<bool>( \
        [=, &resultVec##Local, &movableObj##Local, &excludeVec]() -> bool { \
            auto raySceneQuery = this->raySceneQuery; \
            return NOWA::MathHelper::getInstance()->getRaycastResult( \
                origin##Local, direction##Local, raySceneQuery, resultVec##Local, movableObj##Local, &excludeVec); \
        }, "ENQUEUE_GET_RAYCAST_RESULT"); \
    if (successVar) { \
        (resultVec) = resultVec##Local; \
        (movableObj) = reinterpret_cast<Ogre::MovableObject*>(movableObj##Local); \
    }

/*
* Usage:
* Ogre::Vector3 origin = ...;
* Ogre::Vector3 direction = ...;
* Ogre::Vector3 hitPoint;
* Ogre::MovableObject* hitObject = nullptr;
* bool raycastHit = false;
*
* ENQUEUE_GET_RAYCAST_RESULT(origin, direction, myExcludeVec, hitPoint, hitObject, raycastHit);
*
* if (raycastHit) {
*     // use hitPoint and hitObject
* }
*/ 

#define ENQUEUE_GET_RAYCAST_DETAILS_RESULT(rayInput, resultVec, movableObj, vertexBuf, vertexCountVar, indexBuf, indexCountVar, successVar) \
    Ogre::Vector3 resultVec##Local = Ogre::Vector3::ZERO; \
    size_t movableObj##Local = 0; \
    Ogre::Vector3* vertexBuf##Local = nullptr; \
    size_t vertexCountVar##Local = 0; \
    unsigned long* indexBuf##Local = nullptr; \
    size_t indexCountVar##Local = 0; \
    Ogre::Ray ray##Local = (rayInput); \
    bool successVar = NOWA::GraphicsModule::getInstance()->enqueueAndWaitWithResult<bool>( \
        [=, &resultVec##Local, &movableObj##Local, &vertexBuf##Local, &vertexCountVar##Local, &indexBuf##Local, &indexCountVar##Local]() -> bool { \
            auto raySceneQuery = this->raySceneQuery; \
            return NOWA::MathHelper::getInstance()->getRaycastDetailsResult(ray##Local, raySceneQuery, \
                resultVec##Local, movableObj##Local, vertexBuf##Local, vertexCountVar##Local, indexBuf##Local, indexCountVar##Local); \
        }, "ENQUEUE_GET_RAYCAST_DETAILS_RESULT"); \
    if (successVar) { \
        (resultVec) = resultVec##Local; \
        (movableObj) = reinterpret_cast<Ogre::MovableObject*>(movableObj##Local); \
        (vertexBuf) = vertexBuf##Local; \
        (vertexCountVar) = vertexCountVar##Local; \
        (indexBuf) = indexBuf##Local; \
        (indexCountVar) = indexCountVar##Local; \
    }

/*
* Usage:
* Ogre::Ray ray = ...;
* Ogre::Vector3 hitPoint;
* Ogre::MovableObject* hitObject = nullptr;
* Ogre::Vector3* verts = nullptr;
* unsigned long* inds = nullptr;
* size_t vertCount = 0;
* size_t indCount = 0;
* bool raycastSuccess = false;
* 
* ENQUEUE_GET_RAYCAST_DETAILS_RESULT(ray, hitPoint, hitObject, verts, vertCount, inds, indCount, raycastSuccess);
*
* if (raycastSuccess) {
*     // use hitPoint, hitObject, verts, inds, vertCount, indCount
* }
*/

#define ENQUEUE_GET_RAYCAST_HEIGHT_AND_NORMAL(xPos, zPos, heightVar, normalVar, successVar) \
    Ogre::Real heightVar##Local = 0.0f; \
    Ogre::Vector3 normalVar##Local = Ogre::Vector3::ZERO; \
    Ogre::Real xPos##Local = (xPos); \
    Ogre::Real zPos##Local = (zPos); \
    bool successVar = NOWA::GraphicsModule::getInstance()->enqueueAndWaitWithResult<bool>( \
        [=, &heightVar##Local, &normalVar##Local]() -> bool { \
            auto raySceneQuery = this->raySceneQuery; \
            return NOWA::MathHelper::getInstance()->getRaycastHeightAndNormal(xPos##Local, zPos##Local, raySceneQuery, heightVar##Local, normalVar##Local); \
        }, "ENQUEUE_GET_RAYCAST_HEIGHT_AND_NORMAL"); \
    if (successVar) { \
        (heightVar) = heightVar##Local; \
        (normalVar) = normalVar##Local; \
    }

/*
* Usage: Ogre::Ray ray;
*   ENQUEUE_GET_CAMERA_TO_VIEWPORT_RAY(this->camera, mx, my, ray);
*
*   return ray;
*/

#define ENQUEUE_GET_CAMERA_TO_VIEWPORT_RAY(cameraPtr, mx, my, resultRay) \
    Ogre::Ray resultRay##Local; \
    Ogre::Real mx##Local = (mx); \
    Ogre::Real my##Local = (my); \
    Ogre::Camera* camera##Local = (cameraPtr); \
    NOWA::GraphicsModule::getInstance()->enqueueAndWaitWithResult<bool>( \
        [=, &resultRay##Local]() -> bool { \
            resultRay##Local = camera##Local->getCameraToViewportRay(mx##Local, my##Local); \
            return true; \
        }, "ENQUEUE_GET_CAMERA_TO_VIEWPORT_RAY"); \
    (resultRay) = resultRay##Local;

#endif