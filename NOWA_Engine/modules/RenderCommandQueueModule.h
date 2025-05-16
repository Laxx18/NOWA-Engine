#ifndef RENDER_COMMAND_QUEUE_MODULE
#define RENDER_COMMAND_QUEUE_MODULE

#include "defines.h"

#include <functional>
#include <queue>
#include <mutex>
#include <functional>
#include <future>
#include <chrono>
#include <atomic>
#include <unordered_map>

#define NUM_TRANSFORM_BUFFERS 4

namespace NOWA
{
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
    */

	class EXPORTED RenderCommandQueueModule
	{
    public:

        // Transform data structure - just stores the raw transform data
        struct TransformData
        {
            Ogre::Vector3 position = Ogre::Vector3::ZERO;
            Ogre::Quaternion orientation = Ogre::Quaternion::IDENTITY;
            Ogre::Vector3 scale = Ogre::Vector3::UNIT_SCALE;
        };

        struct CameraTransformData
        {
            Ogre::Vector3 position = Ogre::Vector3::ZERO;
            Ogre::Quaternion orientation = Ogre::Quaternion::IDENTITY;
        };

        // Node and its transform buffers
        struct NodeTransforms
        {
            Ogre::Node* node;
            TransformData transforms[NUM_TRANSFORM_BUFFERS];
            bool active = false;
            bool isNew = false;
        };

        struct CameraTransforms
        {
            Ogre::Camera* camera;
            CameraTransformData transforms[NUM_TRANSFORM_BUFFERS];
            bool active = false;
            bool isNew = false;
        };
        ////////////////////////////////////////////////////////////////

        using RenderCommand = std::function<void()>;

        // For pull-based systems (e.g., render thread pops commands)
        struct CommandEntry
        {
            RenderCommand command;
            std::shared_ptr<std::promise<void>> completionPromise; // nullptr for fire-and-forget
            bool promiseAlreadyFulfilled = false;
        };
	public:

        void enqueue(RenderCommand command, std::shared_ptr<std::promise<void>> promise = nullptr);

        bool pop(CommandEntry& commandEntry);

        void markPromiseFulfilled(const std::shared_ptr<std::promise<void>>& promise);

        bool wasPromiseFulfilled(const std::shared_ptr<std::promise<void>>& promise);

        // TODO: Interface virtual 
        void processAllCommands(void);

        void waitForRenderCompletion(void);

        void cleanupFulfilledPromises(void);

        void executeAndWait(RenderCommand command, const char* commandName);

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
        void updateNodePosition(Ogre::Node* node, const Ogre::Vector3& position);

        // Update orientation for a node in the current buffer
        void updateNodeOrientation(Ogre::Node* node, const Ogre::Quaternion& orientation);

        // Update scale for a node in the current buffer
        void updateNodeScale(Ogre::Node* node, const Ogre::Vector3& scale);

        // Update full transform for a node in the current buffer
        void updateNodeTransform(Ogre::Node* node, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, const Ogre::Vector3& scale = Ogre::Vector3::UNIT_SCALE);

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

        // Advance to the next transform buffer (called by logic system)
        void advanceTransformBuffer(void);

        // Update all transforms with interpolation weight (called by render thread)
        void updateAllTransforms(Ogre::Real weight);

        // Calculate interpolation weight based on time since last logic frame
        Ogre::Real calculateInterpolationWeight(void);

        // Set the accumulated time since the last logic frame
        void setAccumTimeSinceLastLogicFrame(Ogre::Real time);

        // Get the accumulated time since the last logic frame
        Ogre::Real getAccumTimeSinceLastLogicFrame(void) const;

        void setFrameTime(Ogre::Real frameTime);

        void beginLogicFrame(void);

        void endLogicFrame(void);

        size_t getCurrentTransformNodeIdx(void) const;

        size_t getPreviousTransformNodeIdx(void) const;

        size_t getCurrentTransformCameraIdx(void) const;

        size_t getPreviousTransformCameraIdx(void) const;

        void enableDebugVisualization(bool enable);

        void dumpBufferState(void) const;

	public:

		static RenderCommandQueueModule* getInstance();

	private:
		RenderCommandQueueModule();

		~RenderCommandQueueModule();

        // Find a node's transform record
        NodeTransforms* findNodeTransforms(Ogre::Node* node);

        CameraTransforms* findCameraTransforms(Ogre::Camera* camera);
	private:
        std::atomic<std::thread::id> renderThreadId;
        std::mutex mutex;
        std::queue<CommandEntry> queue;
        std::mutex fulfilledMutex;
        std::set<std::shared_ptr<std::promise<void>>> fulfilledPromises;

        std::atomic<bool> timeoutEnabled;
        std::atomic<std::chrono::milliseconds::rep> timeoutDuration;
        std::atomic<Ogre::LogMessageLevel> logLevel;

        std::vector<NodeTransforms> trackedNodes;
        std::unordered_map<Ogre::Node*, size_t> nodeToIndexMap;

        std::vector<CameraTransforms> trackedCameras;
        std::unordered_map<Ogre::Camera*, size_t> cameraToIndexMap;


        size_t currentTransformNodeIdx;
        size_t currentTransformCameraIdx;
        Ogre::Real accumTimeSinceLastLogicFrame;
        Ogre::Real frameTime;
        bool debugVisualization;

        // Frame counters for debugging
        unsigned long logicFrameCount;
        unsigned long renderFrameCount;
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
    NOWA::RenderCommandQueueModule::RenderCommand renderCommand = [this]() { \
        try { \
            lambda_body \
        } catch (const std::exception& e) { \
            NOWA::RenderCommandQueueModule::getInstance()->logCommandEvent( \
                Ogre::String("Exception in command '") + command_name + "': " + e.what(), \
                Ogre::LML_CRITICAL \
            ); \
            throw; /* Re-throw to propagate to the promise */ \
        } catch (...) { \
            NOWA::RenderCommandQueueModule::getInstance()->logCommandEvent( \
                Ogre::String("Unknown exception in command '") + command_name + "'", \
                Ogre::LML_CRITICAL \
            ); \
            throw; /* Re-throw to propagate to the promise */ \
        } \
    }; \
    \
    NOWA::RenderCommandQueueModule::getInstance()->executeAndWait(renderCommand, command_name); \
}

 // Multi-capture wait macro with timeout, logging, and depth tracking
#define ENQUEUE_RENDER_COMMAND_MULTI_WAIT(command_name, capture_macro, lambda_body) \
{ \
    NOWA::RenderCommandQueueModule::RenderCommand renderCommand = [this, capture_macro]() { \
        try { \
            lambda_body \
        } catch (const std::exception& e) { \
            NOWA::RenderCommandQueueModule::getInstance()->logCommandEvent( \
                Ogre::String("Exception in command '") + command_name + "': " + e.what(), \
                Ogre::LML_CRITICAL \
            ); \
            throw; /* Re-throw to propagate to the promise */ \
        } catch (...) { \
            NOWA::RenderCommandQueueModule::getInstance()->logCommandEvent( \
                Ogre::String("Unknown exception in command '") + command_name + "'", \
                Ogre::LML_CRITICAL \
            ); \
            throw; /* Re-throw to propagate to the promise */ \
        } \
    }; \
    \
    NOWA::RenderCommandQueueModule::getInstance()->executeAndWait(renderCommand, command_name); \
}

// Non-waiting command macros (for completeness)
#define ENQUEUE_RENDER_COMMAND(command_name, lambda_body) \
{ \
    NOWA::RenderCommandQueueModule::RenderCommand renderCommand = [this]() { \
        try { \
            lambda_body \
        } catch (const std::exception& e) { \
            NOWA::RenderCommandQueueModule::getInstance()->logCommandEvent( \
                Ogre::String("Exception in command '") + command_name + "': " + e.what(), \
                Ogre::LML_CRITICAL \
            ); \
            throw; /* Re-throw to propagate to the promise */ \
        } catch (...) { \
            NOWA::RenderCommandQueueModule::getInstance()->logCommandEvent( \
                Ogre::String("Unknown exception in command '") + command_name + "'", \
                Ogre::LML_CRITICAL \
            ); \
            throw; /* Re-throw to propagate to the promise */ \
        } \
    }; \
    \
    NOWA::RenderCommandQueueModule::getInstance()->enqueue(renderCommand); \
}


#define ENQUEUE_RENDER_COMMAND_MULTI(command_name, capture_macro, lambda_body) \
{ \
    NOWA::RenderCommandQueueModule::RenderCommand renderCommand = [this, capture_macro]() { \
        try { \
            lambda_body \
        } catch (const std::exception& e) { \
            NOWA::RenderCommandQueueModule::getInstance()->logCommandEvent( \
                Ogre::String("Exception in command '") + command_name + "': " + e.what(), \
                Ogre::LML_CRITICAL \
            ); \
            throw; /* Re-throw to propagate to the promise */ \
        } catch (...) { \
            NOWA::RenderCommandQueueModule::getInstance()->logCommandEvent( \
                Ogre::String("Unknown exception in command '") + command_name + "'", \
                Ogre::LML_CRITICAL \
            ); \
            throw; /* Re-throw to propagate to the promise */ \
        } \
    }; \
    \
    NOWA::RenderCommandQueueModule::getInstance()->enqueue(renderCommand); \
}

#endif