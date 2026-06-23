#ifndef GRPAHICS_MODULE
#define GRPAHICS_MODULE

#include "defines.h"

#include <functional>
#include <mutex>
#include <future>
#include <chrono>
#include <atomic>
#include <unordered_map>
#include <vector>
#include <deque>
#include <array>
#include <thread>

#include "MyGUI_InputManager.h"

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
     * If an entity/item is deleted immediately on the logic thread, the render thread might still access its transform � leading to crashes or undefined behavior.
     *
     * - Logic thread writes to logicWriteSlot.
     * - Render thread reads from (logicWriteSlot + NUM_SLOTS - 2) % NUM_SLOTS to destroy.
     * - Always skip 1�2 slots between write and destroy to ensure safety.
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
     *
     * IMPORTANT - Threading model for the tracked-resource containers (nodePool, cameraPool,
     * oldBonePool, bonePool, passPool, datablockPool) and their *ToIndexMap siblings:
     *
     * This mirrors how Ogre-Next's own Tutorial06_Multithreading sample (GameEntityManager)
     * does it: a tracked object is resolved into a stable storage slot EXACTLY ONCE per
     * (thread, object) pair, and after that the calling thread keeps the slot's raw address
     * cached locally (thread_local) and uses it directly forever - no lock, no hash lookup,
     * no per-call overhead. The lock only exists to make that one-time resolution, and the
     * rare removal/eviction of a slot, safe.
     *
     * Why a std::deque, not a std::vector: a vector's swap-and-pop removal pattern moves
     * OTHER elements around in memory, which would invalidate any thread's already-cached
     * pointer to one of those other elements. A std::deque never relocates an existing
     * element - growing it (push_back/emplace_back) does not move elements that are already
     * in it, and it is never erased from, only "tombstoned" in place (see removeTracked*()).
     * A pointer obtained while holding the mutex therefore stays valid for the lifetime of
     * the GraphicsModule, even across later growth or recycling of OTHER slots.
     *
     * Why a slot can still be safely recycled without a lock on the reading side: each slot
     * carries its own identity field (e.g. NodeTransforms::node) as an atomic pointer. Before
     * trusting a cached slot pointer, the fast path always re-checks
     * "does this slot's stored identity still equal the object I'm updating?". If a slot was
     * tombstoned and later handed to a different object, the stored identity no longer
     * matches, the stale cache entry is dropped, and that one thread re-resolves once more
     * (taking the lock again, exactly as on first contact). This is the same kind of
     * ownership check Ogre's message-passing protocol gives it "for free" - NOWA just has to
     * do it explicitly because objects are updated directly rather than via a message queue.
     *
     * Rules:
     * 1. resolveNodeSlotLocked() (and its five siblings) is the ONLY place that may grow a
     *    pool (push_back/emplace_back) or mutate its index map / free-list. It always takes
     *    the category's mutex and is never called while that mutex is already held by the
     *    calling thread (the mutexes are plain std::mutex, not recursive - on purpose, so a
     *    forgotten nested lock fails loudly as a deadlock in testing instead of silently
     *    working "by luck" via recursion).
     * 2. removeTracked*() takes the lock and tombstones a slot in place (identity -> nullptr,
     *    active -> false, index pushed onto the free-list). It never erases from the pool.
     * 3. The once-per-frame bulk passes (advanceTransformBuffer, updateAllTransforms,
     *    flushTransforms, dumpBufferState) take the category's lock for their ENTIRE pass,
     *    not because the per-element work needs it, but because a std::deque's operator[]/
     *    iteration is only guaranteed safe relative to a CONCURRENT push_back if every access
     *    is itself synchronized - the pointer-stability guarantee only protects pointers
     *    obtained *while already holding the lock*, not a fresh unlocked operator[] call.
     *    Where advanceTransformBuffer's eviction sweep needs to tombstone a slot, it does so
     *    inline (under the lock it already holds) rather than calling the public
     *    removeTracked*() function, specifically to avoid needing a recursive mutex.
     * 4. acquireNode/Camera/OldBone/Bone/Pass/DatablockSlot() are the lock-free fast path used
     *    by every update*() function. They consult a thread_local cache first; only a cache
     *    miss (first contact for this (thread, object) pair, or a stale/recycled entry) falls
     *    through to the locked resolve function.
     * 5. clearSceneResources() must NOT call clear() on a pool's underlying deque - that would
     *    physically free chunk memory that another thread's thread_local cache might still
     *    point to, turning a future use of a stale cache entry into a use-after-free instead
     *    of a harmless "stale identity, re-resolve" miss. It tombstones every existing slot in
     *    place instead, exactly like removeTracked*() does for one slot at a time. Only the
     *    final, terminal cleanup in renderThreadFunction() at engine shutdown actually clears
     *    the deques, since no further access can happen after that point.
     */

    class EXPORTED GraphicsModule
    {
    public:
        friend class AppState;

        // Transform data structure - just stores the raw transform data
        struct TransformData
        {
            // Absolute components (interpolated)
            Ogre::Vector3 position = Ogre::Vector3::ZERO;
            Ogre::Quaternion orientation = Ogre::Quaternion::IDENTITY;
            Ogre::Vector3 scale = Ogre::Vector3::UNIT_SCALE;
        };

        struct PassSpeedData
        {
            Ogre::Real speedsX[9] = {};
            Ogre::Real speedsY[9] = {};
        };

        struct CameraTransformData
        {
            // Absolute components (interpolated)
            Ogre::Vector3 position = Ogre::Vector3::ZERO;
            Ogre::Quaternion orientation = Ogre::Quaternion::IDENTITY;
        };

        // Node and its transform buffers.
        //
        // Locking model (see the big comment block above for the full rationale):
        // 'node' is the slot's identity tag. It is written exactly twice in a slot's
        // life - once when resolveNodeSlotLocked() claims/recycles the slot (under
        // nodeMutex), once when removeTrackedNode()/the eviction sweep tombstones it
        // back to nullptr (also under nodeMutex). Every other field a lock-free
        // update*() call can touch (active, useDerived, updatedThisFrame,
        // stableFrames) is atomic so that those writes are well-defined even though
        // they happen without holding nodeMutex. 'isNew' is NOT atomic - it is only
        // ever touched by resolveNodeSlotLocked() and advanceTransformBuffer(), both
        // of which already hold nodeMutex for as long as they touch it.
        // buffer-advance tick).
        //
        // There is deliberately no "last touched N frames ago" bookkeeping here -
        // tracking lifetime is explicit-only: something that starts driving a node
        // through updateNode*()/setNode*() is responsible for calling
        // removeTrackedNode() when it stops. A node that is tracked but genuinely
        // not being updated this tick simply keeps showing its last value (the
        // copy-forward step in advanceTransformBuffer() carries it into the new
        // buffer slot unchanged) - there is no implicit timeout or auto-eviction.
        //
        // Explicit move ctor/assignment: std::atomic<T> has NO move constructor
        // (its deleted copy constructor suppresses the implicit one), so the
        // compiler-generated move for this whole struct would also be implicitly
        // deleted, which then falls back to the (also deleted) copy constructor -
        // exactly the "construct_at: deleted function" error MSVC reports. This is
        // needed even though, at runtime, nodePool is resize()'d exactly once while
        // empty and never moves a real element again afterward: vector's growth
        // path is templated code that the compiler must still be able to
        // INSTANTIATE regardless of whether it is ever actually executed. Moving
        // each atomic field by hand (load from source, store into destination) is
        // exactly the data-preserving behaviour we want anyway. Copy is explicitly
        // deleted - nothing should ever copy one of these.
        struct NodeTransforms
        {
            std::atomic<Ogre::Node*> node{nullptr};
            TransformData transforms[NUM_TRANSFORM_BUFFERS];
            std::atomic<bool> active{false};
            std::atomic<bool> useDerived{false};
            bool isNew = false;

            NodeTransforms() = default;
            NodeTransforms(const NodeTransforms&) = delete;
            NodeTransforms& operator=(const NodeTransforms&) = delete;

            NodeTransforms(NodeTransforms&& other) noexcept
            {
                node.store(other.node.load(std::memory_order_relaxed), std::memory_order_relaxed);
                for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
                {
                    transforms[i] = other.transforms[i];
                }
                active.store(other.active.load(std::memory_order_relaxed), std::memory_order_relaxed);
                useDerived.store(other.useDerived.load(std::memory_order_relaxed), std::memory_order_relaxed);
                isNew = other.isNew;
            }

            NodeTransforms& operator=(NodeTransforms&& other) noexcept
            {
                if (this != &other)
                {
                    node.store(other.node.load(std::memory_order_relaxed), std::memory_order_relaxed);
                    for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
                    {
                        transforms[i] = other.transforms[i];
                    }
                    active.store(other.active.load(std::memory_order_relaxed), std::memory_order_relaxed);
                    useDerived.store(other.useDerived.load(std::memory_order_relaxed), std::memory_order_relaxed);
                    isNew = other.isNew;
                }
                return *this;
            }
        };

        struct CameraTransforms
        {
            std::atomic<Ogre::Camera*> camera{nullptr};
            CameraTransformData transforms[NUM_TRANSFORM_BUFFERS];
            std::atomic<bool> active{false};
            bool isNew = false;

            CameraTransforms() = default;
            CameraTransforms(const CameraTransforms&) = delete;
            CameraTransforms& operator=(const CameraTransforms&) = delete;

            CameraTransforms(CameraTransforms&& other) noexcept
            {
                camera.store(other.camera.load(std::memory_order_relaxed), std::memory_order_relaxed);
                for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
                {
                    transforms[i] = other.transforms[i];
                }
                active.store(other.active.load(std::memory_order_relaxed), std::memory_order_relaxed);
                isNew = other.isNew;
            }

            CameraTransforms& operator=(CameraTransforms&& other) noexcept
            {
                if (this != &other)
                {
                    camera.store(other.camera.load(std::memory_order_relaxed), std::memory_order_relaxed);
                    for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
                    {
                        transforms[i] = other.transforms[i];
                    }
                    active.store(other.active.load(std::memory_order_relaxed), std::memory_order_relaxed);
                    isNew = other.isNew;
                }
                return *this;
            }
        };

        struct OldBoneTransforms
        {
            std::atomic<Ogre::v1::OldBone*> oldBone{nullptr};
            TransformData transforms[NUM_TRANSFORM_BUFFERS];
            std::atomic<bool> active{false};
            bool isNew = false;

            OldBoneTransforms() = default;
            OldBoneTransforms(const OldBoneTransforms&) = delete;
            OldBoneTransforms& operator=(const OldBoneTransforms&) = delete;

            OldBoneTransforms(OldBoneTransforms&& other) noexcept
            {
                oldBone.store(other.oldBone.load(std::memory_order_relaxed), std::memory_order_relaxed);
                for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
                {
                    transforms[i] = other.transforms[i];
                }
                active.store(other.active.load(std::memory_order_relaxed), std::memory_order_relaxed);
                isNew = other.isNew;
            }

            OldBoneTransforms& operator=(OldBoneTransforms&& other) noexcept
            {
                if (this != &other)
                {
                    oldBone.store(other.oldBone.load(std::memory_order_relaxed), std::memory_order_relaxed);
                    for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
                    {
                        transforms[i] = other.transforms[i];
                    }
                    active.store(other.active.load(std::memory_order_relaxed), std::memory_order_relaxed);
                    isNew = other.isNew;
                }
                return *this;
            }
        };

        struct BoneTransforms
        {
            std::atomic<Ogre::Bone*> bone{nullptr};
            TransformData transforms[NUM_TRANSFORM_BUFFERS];
            std::atomic<bool> active{false};
            bool isNew = false;

            BoneTransforms() = default;
            BoneTransforms(const BoneTransforms&) = delete;
            BoneTransforms& operator=(const BoneTransforms&) = delete;

            BoneTransforms(BoneTransforms&& other) noexcept
            {
                bone.store(other.bone.load(std::memory_order_relaxed), std::memory_order_relaxed);
                for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
                {
                    transforms[i] = other.transforms[i];
                }
                active.store(other.active.load(std::memory_order_relaxed), std::memory_order_relaxed);
                isNew = other.isNew;
            }

            BoneTransforms& operator=(BoneTransforms&& other) noexcept
            {
                if (this != &other)
                {
                    bone.store(other.bone.load(std::memory_order_relaxed), std::memory_order_relaxed);
                    for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
                    {
                        transforms[i] = other.transforms[i];
                    }
                    active.store(other.active.load(std::memory_order_relaxed), std::memory_order_relaxed);
                    isNew = other.isNew;
                }
                return *this;
            }
        };

        struct PassTransforms
        {
            std::atomic<Ogre::Pass*> pass{nullptr};
            PassSpeedData transforms[NUM_TRANSFORM_BUFFERS];
            std::atomic<bool> active{false};
            bool isNew = false;

            PassTransforms() = default;
            PassTransforms(const PassTransforms&) = delete;
            PassTransforms& operator=(const PassTransforms&) = delete;

            PassTransforms(PassTransforms&& other) noexcept
            {
                pass.store(other.pass.load(std::memory_order_relaxed), std::memory_order_relaxed);
                for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
                {
                    transforms[i] = other.transforms[i];
                }
                active.store(other.active.load(std::memory_order_relaxed), std::memory_order_relaxed);
                isNew = other.isNew;
            }

            PassTransforms& operator=(PassTransforms&& other) noexcept
            {
                if (this != &other)
                {
                    pass.store(other.pass.load(std::memory_order_relaxed), std::memory_order_relaxed);
                    for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
                    {
                        transforms[i] = other.transforms[i];
                    }
                    active.store(other.active.load(std::memory_order_relaxed), std::memory_order_relaxed);
                    isNew = other.isNew;
                }
                return *this;
            }
        };

        struct TrackedDatablock
        {
            std::atomic<Ogre::HlmsDatablock*> datablock{nullptr};

            // Each buffer holds a value (e.g. ColourValue)
            Ogre::ColourValue values[NUM_TRANSFORM_BUFFERS];

            // The interpolation application function. Only ever set once, under
            // datablockMutex, when the slot is created/recycled - never touched by
            // the lock-free update path - so a plain std::function is fine here.
            std::function<void(Ogre::ColourValue)> applyFunc;

            // Interpolation function between two values. Same lifetime rule as above.
            std::function<Ogre::ColourValue(const Ogre::ColourValue&, const Ogre::ColourValue&, Ogre::Real)> interpolateFunc;

            std::atomic<bool> active{false};
            bool isNew = false;

            TrackedDatablock() = default;
            TrackedDatablock(const TrackedDatablock&) = delete;
            TrackedDatablock& operator=(const TrackedDatablock&) = delete;

            TrackedDatablock(TrackedDatablock&& other) noexcept
            {
                datablock.store(other.datablock.load(std::memory_order_relaxed), std::memory_order_relaxed);
                for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
                {
                    values[i] = other.values[i];
                }
                applyFunc = std::move(other.applyFunc);
                interpolateFunc = std::move(other.interpolateFunc);
                active.store(other.active.load(std::memory_order_relaxed), std::memory_order_relaxed);
                isNew = other.isNew;
            }

            TrackedDatablock& operator=(TrackedDatablock&& other) noexcept
            {
                if (this != &other)
                {
                    datablock.store(other.datablock.load(std::memory_order_relaxed), std::memory_order_relaxed);
                    for (size_t i = 0; i < NUM_TRANSFORM_BUFFERS; ++i)
                    {
                        values[i] = other.values[i];
                    }
                    applyFunc = std::move(other.applyFunc);
                    interpolateFunc = std::move(other.interpolateFunc);
                    active.store(other.active.load(std::memory_order_relaxed), std::memory_order_relaxed);
                    isNew = other.isNew;
                }
                return *this;
            }
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
        };

    public:
        void beginWorkspaceTransition(void);

        void endWorkspaceTransition(void);

        bool isWorkspaceTransitioning(void) const;

        void clearAllClosures(void);

        void clearSceneResources(void);

        void doCleanup(void);

        bool getIsRunning(void) const;

        void enqueue(RenderCommand&& command, const char* commandName, std::shared_ptr<std::promise<void>> promise = nullptr);

        bool pop(CommandEntry& commandEntry);

        bool hasPendingRenderCommands(void) const;

        void processAllCommands(void);

        void waitForRenderCompletion(void);

        void enqueueAndWait(RenderCommand&& command, const char* commandName);

        template <typename T> T enqueueAndWaitWithResult(std::function<T()> command, const char* commandName)
        {
            // ── Render-thread re-entrancy: execute directly to prevent deadlock ───
            // This is the only real re-entrancy scenario: a command running on the
            // render thread calls back into the queue system. Executing directly
            // bypasses the queue and avoids "render thread waits for itself".
            // All other nesting scenarios are impossible: the logic thread blocks
            // immediately on future.wait(), so waitDepth can never exceed 1 on it.
            if (this->isRenderThread())
            {
                try
                {
                    this->logCommandEvent(std::string("Executing '") + commandName + "' directly on render thread (re-entrant)", Ogre::LML_TRIVIAL);
                    return command();
                }
                catch (const std::exception& e)
                {
                    this->logCommandEvent(std::string("Exception in re-entrant execution of '") + commandName + "': " + e.what(), Ogre::LML_CRITICAL);
                    throw;
                }
                catch (...)
                {
                    this->logCommandEvent(std::string("Unknown exception in re-entrant execution of '") + commandName + "'", Ogre::LML_CRITICAL);
                    throw;
                }
            }

            // ── Logic-thread path ─────────────────────────────────────────────────
            // Mark so enqueueDestroy() defers instead of pushing into the ring-buffer
            // while we are blocked below (mirrors enqueueAndWait behaviour).
            this->isRunningWaitClosure = true;

            try
            {
                auto promise = std::make_shared<std::promise<T>>();
                auto future = promise->get_future();

                this->logCommandEvent(std::string("Enqueueing '") + commandName + "' with wait and result", Ogre::LML_NORMAL);

                // Wrap command so the result is published via the promise on the
                // render thread, and any exception is forwarded cleanly.
                this->enqueue(
                    [command, promise]()
                    {
                        try
                        {
                            promise->set_value(command());
                        }
                        catch (...)
                        {
                            promise->set_exception(std::current_exception());
                        }
                    },
                    commandName);

                // Spin-wait: same pattern as enqueueAndWait — yields the CPU slice
                // rather than hard-blocking, consistent with the rest of the queue system.
                if (this->isTimeoutEnabled())
                {
                    auto timeout = this->getTimeoutDuration();
                    if (future.wait_for(timeout) == std::future_status::timeout)
                    {
                        this->logCommandEvent(std::string("Timeout waiting for '") + commandName + "'", Ogre::LML_CRITICAL);
                        this->processQueueSync(); // attempt recovery
                    }
                }
                else
                {
                    while (future.wait_for(std::chrono::milliseconds(1)) != std::future_status::ready)
                    {
                        std::this_thread::yield();
                    }
                }

                // future is guaranteed ready here — get() will not block.
                T result = future.get(); // also rethrows any exception set by the render thread

                this->flushDeferredDestroyCommands(); // flush anything deferred during the wait
                this->isRunningWaitClosure = false;

                this->logCommandEvent(std::string("'") + commandName + "' completed with result", Ogre::LML_TRIVIAL);

                return result;
            }
            catch (...)
            {
                this->flushDeferredDestroyCommands(); // flush even on exception
                this->isRunningWaitClosure = false;
                throw;
            }
        }

        void processQueueSync(void);

        // Called from LOGIC thread once per outer loop iteration
        void publishInterpolationAlpha(float alpha);

        // Called from RENDER thread each frame
        float consumeInterpolationAlpha() const;

        // Optional: for debugging/telemetry
        uint64_t getLogicFrameId() const;

        /*
         * Bulletproof solution for nested render thread commands � in a large codebase (400,000+ LOC), accidental misuse of a promise-based render command inside another queued render command can easily cause deadlocks.
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

        // Called from logic thread before clearSceneResources(). Blocks until render
        // thread has finished its current iteration and confirmed it won't touch vectors.
        void requestStall(void);

        // Called from logic thread after clearSceneResources(). Unblocks render thread.
        void releaseStall(void);

        bool isSceneValid(void);

        // Add a node to be tracked and transformed
        void addTrackedNode(Ogre::Node* node);

        // Remove a node from tracking
        void removeTrackedNode(Ogre::Node* node);

        // Update position for a node in the current buffer
        // use setNodePosition() instead if you need an instant warp.
        void updateNodePosition(Ogre::Node* node, const Ogre::Vector3& position, bool useDerived = false);

        // Update orientation for a node in the current buffer. Always interpolated -
        // use setNodeOrientation() instead if you need an instant warp.
        void updateNodeOrientation(Ogre::Node* node, const Ogre::Quaternion& orientation, bool useDerived = false);

        // Update scale for a node in the current buffer. Always interpolated -
        // use setNodeScale() instead if you need an instant warp.
        void updateNodeScale(Ogre::Node* node, const Ogre::Vector3& scale, bool useDerived = false);

        // Update full transform for a node in the current buffer. Always interpolated -
        // use setNodeTransform() instead if you need an instant warp.
        void updateNodeTransform(Ogre::Node* node, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, const Ogre::Vector3& scale = Ogre::Vector3::UNIT_SCALE, bool useDerived = false);

        // Instant warp: applies directly to the Ogre node THIS call (synchronously,
        // dispatching to the render thread if not already on it) and re-pins the
        // interpolation buffer to match, so it overrides whatever updateAllTransforms()
        // would otherwise have shown - no waiting for the next interpolation pass, no
        // dependency on buffer index/weight timing. If something else keeps calling
        // updateNodePosition()/etc. for this same node afterward (e.g. a still-running
        // vehicle update), THAT will overwrite the warp again on its own next write -
        // setNodePosition() guarantees correctness for the instant it runs, it cannot by
        // itself stop another system from continuing to drive the same node.
        void setNodePosition(Ogre::Node* node, const Ogre::Vector3& position, bool useDerived = false);

        // Update orientation for a node in the current buffer
        void setNodeOrientation(Ogre::Node* node, const Ogre::Quaternion& orientation, bool useDerived = false);

        // Update scale for a node in the current buffer
        void setNodeScale(Ogre::Node* node, const Ogre::Vector3& scale, bool useDerived = false);

        // Update full transform for a node in the current buffer
        void setNodeTransform(Ogre::Node* node, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, const Ogre::Vector3& scale = Ogre::Vector3::UNIT_SCALE, bool useDerived = false);

        // Add a Camera to be tracked and transformed
        void addTrackedCamera(Ogre::Camera* camera);

        // Remove a Camera from tracking
        void removeTrackedCamera(Ogre::Camera* camera);

        // Update position for a Camera in the current buffer
        // use setCameraPosition() instead if you need an instant warp.
        void updateCameraPosition(Ogre::Camera* camera, const Ogre::Vector3& position);

        // Update orientation for a Camera in the current buffer. Always interpolated -
        // use setCameraOrientation() instead if you need an instant warp.
        void updateCameraOrientation(Ogre::Camera* camera, const Ogre::Quaternion& orientation);

        // Update full transform for a Camera in the current buffer. Always interpolated -
        // use setCameraTransform() instead if you need an instant warp.
        void updateCameraTransform(Ogre::Camera* camera, const Ogre::Vector3& position, const Ogre::Quaternion& orientation);

        // Instant warp - see setNodePosition() for the full contract.
        void setCameraPosition(Ogre::Camera* camera, const Ogre::Vector3& position);

        // Update orientation for a Camera in the current buffer
        void setCameraOrientation(Ogre::Camera* camera, const Ogre::Quaternion& orientation);

        // Update full transform for a Camera in the current buffer
        void setCameraTransform(Ogre::Camera* camera, const Ogre::Vector3& position, const Ogre::Quaternion& orientation);

        // Add a OldBone to be tracked and transformed
        void addTrackedOldBone(Ogre::v1::OldBone* oldBone);

        // Remove a OldBone from tracking
        void removeTrackedOldBone(Ogre::v1::OldBone* oldBone);

        // Update position for a OldBone in the current buffer
        // use setOldBonePosition() instead if you need an instant warp.
        void updateOldBonePosition(Ogre::v1::OldBone* oldBone, const Ogre::Vector3& position);

        // Update orientation for a OldBone in the current buffer. Always interpolated -
        // use setOldBoneOrientation() instead if you need an instant warp.
        void updateOldBoneOrientation(Ogre::v1::OldBone* oldBone, const Ogre::Quaternion& orientation);

        // Update full transform for a OldBone in the current buffer. Always interpolated -
        // use setOldBoneTransform() instead if you need an instant warp.
        void updateOldBoneTransform(Ogre::v1::OldBone* oldBone, const Ogre::Vector3& position, const Ogre::Quaternion& orientation);

        // Instant warp - see setNodePosition() for the full contract.
        void setOldBonePosition(Ogre::v1::OldBone* oldBone, const Ogre::Vector3& position);

        // Update orientation for a OldBone in the current buffer
        void setOldBoneOrientation(Ogre::v1::OldBone* oldBone, const Ogre::Quaternion& orientation);

        // Update full transform for a OldBone in the current buffer
        void setOldBoneTransform(Ogre::v1::OldBone* oldBone, const Ogre::Vector3& position, const Ogre::Quaternion& orientation);

        // Add a Bone to be tracked and transformed
        void addTrackedBone(Ogre::Bone* bone);

        // Remove a Bone from tracking
        void removeTrackedBone(Ogre::Bone* bone);

        // Update position for a Bone in the current buffer
        // use setBonePosition() instead if you need an instant warp.
        void updateBonePosition(Ogre::Bone* bone, const Ogre::Vector3& position);

        // Update orientation for a Bone in the current buffer. Always interpolated -
        // use setBoneOrientation() instead if you need an instant warp.
        void updateBoneOrientation(Ogre::Bone* bone, const Ogre::Quaternion& orientation);

        // Update full transform for a Bone in the current buffer. Always interpolated -
        // use setBoneTransform() instead if you need an instant warp.
        void updateBoneTransform(Ogre::Bone* bone, const Ogre::Vector3& position, const Ogre::Quaternion& orientation);

        // Update orientation for a Bone in the current buffer
        void setBonePosition(Ogre::Bone* bone, const Ogre::Vector3& position);

        // Update full transform for a Bone in the current buffer
        void setBoneOrientation(Ogre::Bone* bone, const Ogre::Quaternion& orientation);

        // Instant warp - see setNodePosition() for the full contract.
        void setBoneTransform(Ogre::Bone* bone, const Ogre::Vector3& position, const Ogre::Quaternion& orientation);

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

        void flushTransforms(void);

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

        size_t getPreviousTransformBoneIdx(void) const;

        size_t getPreviousTransformPassIdx(void) const;

        size_t getPreviousTrackedDatablockIdx(void) const;

        void enableDebugVisualization(bool enable);

        void dumpBufferState(void) const;

        /*
        Do only use enqueueDestroy if:
        Situation
        macroGame
        loop: projectile dies, enemy killed, spawned object gone
        ENQUEUE_DESTROY_COMMAND — deferred ring-buffer, no stall

        Else use enqueueAndWait(...):
        Situation:
        Editor action:
        escape, undo, load scene, place mode offe
        */
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
        void processClosureCommands(void);

        // Called from render thread - executes all active closures
        void executeActiveClosures(void);

        // Combined method called from render thread
        void updateAndExecuteClosures();

        /*
         * @brief Gets the MyGUI widget currently under mouse focus, if any. This is used by input handling code to determine whether to block input for gameplay when the mouse is interacting with the UI.
         * @return Pointer to the MyGUI widget currently under mouse focus, or nullptr if no widget is focused.
         */
        MyGUI::Widget* getMyGUIFocusWidget(void);

    public:
        static constexpr size_t NUM_DESTROY_SLOTS = 4;

        static GraphicsModule* getInstance();

    private:
        GraphicsModule();

        ~GraphicsModule();

        void startRendering(void);

        void stopRendering(void);

        void renderThreadFunction(void);

        void setInterpolationWeight(float w);

        // Locked slow path: resolves 'node' to a stable pool slot, creating or
        // recycling one if it isn't tracked yet. Takes nodeMutex. Called at most
        // once per (thread, node) pair by acquireNodeSlot() below - never call
        // this directly from update*()/addTrackedNode() hot paths.
        NodeTransforms* resolveNodeSlotLocked(Ogre::Node* node);

        // Lock-free fast path used by every updateNode*()/addTrackedNode() call.
        // Checks a thread_local cache first; only falls through to
        // resolveNodeSlotLocked() on a cache miss or a stale (recycled) entry.
        NodeTransforms* acquireNodeSlot(Ogre::Node* node);

        CameraTransforms* resolveCameraSlotLocked(Ogre::Camera* camera);
        CameraTransforms* acquireCameraSlot(Ogre::Camera* camera);

        OldBoneTransforms* resolveOldBoneSlotLocked(Ogre::v1::OldBone* oldBone);
        OldBoneTransforms* acquireOldBoneSlot(Ogre::v1::OldBone* oldBone);

        BoneTransforms* resolveBoneSlotLocked(Ogre::Bone* bone);
        BoneTransforms* acquireBoneSlot(Ogre::Bone* bone);

        PassTransforms* resolvePassSlotLocked(Ogre::Pass* pass);
        PassTransforms* acquirePassSlot(Ogre::Pass* pass);

        // Datablock needs the extra creation parameters because - unlike position/
        // orientation for a node - the apply/interpolate functions cannot be derived
        // from the object itself and must come from the caller on first contact.
        TrackedDatablock* resolveDatablockSlotLocked(Ogre::HlmsDatablock* datablock, const Ogre::ColourValue& initialValue, std::function<void(Ogre::ColourValue)> applyFunc,
            std::function<Ogre::ColourValue(const Ogre::ColourValue&, const Ogre::ColourValue&, Ogre::Real)> interpFunc);
        TrackedDatablock* acquireDatablockSlot(Ogre::HlmsDatablock* datablock, const Ogre::ColourValue& initialValue, std::function<void(Ogre::ColourValue)> applyFunc,
            std::function<Ogre::ColourValue(const Ogre::ColourValue&, const Ogre::ColourValue&, Ogre::Real)> interpFunc);

        // Render-thread-only implementations behind setNode*()/setCamera*()/etc. Each
        // one (1) applies directly to the actual Ogre object right now, and (2) pins
        // the interpolation buffer to match. Called either directly (if already on
        // the render thread) or via enqueueAndWait (otherwise) by the public set*()
        // wrappers - see the contract comment on setNodePosition() in the header.
        void setNodePositionOnRenderThread(Ogre::Node* node, const Ogre::Vector3& position, bool useDerived);
        void setNodeOrientationOnRenderThread(Ogre::Node* node, const Ogre::Quaternion& orientation, bool useDerived);
        void setNodeScaleOnRenderThread(Ogre::Node* node, const Ogre::Vector3& scale, bool useDerived);
        void setNodeTransformOnRenderThread(Ogre::Node* node, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, const Ogre::Vector3& scale, bool useDerived);

        void setCameraPositionOnRenderThread(Ogre::Camera* camera, const Ogre::Vector3& position);
        void setCameraOrientationOnRenderThread(Ogre::Camera* camera, const Ogre::Quaternion& orientation);
        void setCameraTransformOnRenderThread(Ogre::Camera* camera, const Ogre::Vector3& position, const Ogre::Quaternion& orientation);

        void setOldBonePositionOnRenderThread(Ogre::v1::OldBone* oldBone, const Ogre::Vector3& position);
        void setOldBoneOrientationOnRenderThread(Ogre::v1::OldBone* oldBone, const Ogre::Quaternion& orientation);
        void setOldBoneTransformOnRenderThread(Ogre::v1::OldBone* oldBone, const Ogre::Vector3& position, const Ogre::Quaternion& orientation);

        void setBonePositionOnRenderThread(Ogre::Bone* bone, const Ogre::Vector3& position);
        void setBoneOrientationOnRenderThread(Ogre::Bone* bone, const Ogre::Quaternion& orientation);
        void setBoneTransformOnRenderThread(Ogre::Bone* bone, const Ogre::Vector3& position, const Ogre::Quaternion& orientation);

        // Internal helper methods (render thread only)
        void processSingleCommand(const ClosureCommand& command, Ogre::Real renderDt);

        void addPersistentClosure(const Ogre::String& uniqueName, std::function<void(Ogre::Real)> closureFunc);

        void updatePersistentClosure(const Ogre::String& uniqueName, std::function<void(Ogre::Real)> closureFunc);

        void removePersistentClosure(const Ogre::String& uniqueName);

        void flushDeferredDestroyCommands()
        {
            if (true == this->deferredDestroyCommands.empty())
            {
                return;
            }

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL,
                "[GraphicsModule] Flushing " + Ogre::StringConverter::toString(this->deferredDestroyCommands.size()) + " deferred destroy commands into slot " + Ogre::StringConverter::toString(this->currentDestroySlot));

            for (const auto& [cmd, name] : this->deferredDestroyCommands)
            {
                this->destroySlots[this->currentDestroySlot].emplace_back(std::move(cmd));
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[GraphicsModule] Flushed deferred destroy: " + name);
            }

            this->deferredDestroyCommands.clear();
        }

        void publishLogicFrame();

    private:
        std::thread renderThread;
        std::atomic<bool> bRunning{false};

        std::atomic<bool> workspaceTransitionInProgress{false};

        // alpha = accumulator / fixedDt  (clamped 0..1)
        std::atomic<float> m_interpolationAlpha{0.0f};

        // increments when logic produces a new snapshot (endLogicFrame)
        std::atomic<uint64_t> m_logicFrameId{0};

        Ogre::Real currentRenderDt;

        std::atomic<std::thread::id> renderThreadId;
        std::mutex mutex;
        moodycamel::ConcurrentQueue<CommandEntry> queue;

        std::atomic<bool> timeoutEnabled;
        std::atomic<std::chrono::milliseconds::rep> timeoutDuration;
        std::atomic<Ogre::LogMessageLevel> logLevel;

        // Pool storage for tracked nodes. See the threading-model comment near the
        // top of this header. Append-only (push_back/emplace_back only ever grows
        // it); slots are tombstoned in place and recycled via freeNodeSlots, never
        // physically erased - that is what makes a cached raw pointer into this
        // deque safe to keep using forever, even across later growth.
        static constexpr size_t NODE_POOL_CAPACITY = 8192;
        static constexpr size_t CAMERA_POOL_CAPACITY = 64;
        static constexpr size_t OLD_BONE_POOL_CAPACITY = 4096;
        static constexpr size_t BONE_POOL_CAPACITY = 4096;
        static constexpr size_t PASS_POOL_CAPACITY = 256;
        static constexpr size_t DATABLOCK_POOL_CAPACITY = 1024;

        // Returned by resolve*SlotLocked() instead of a real pool slot in the
        // (should-never-happen-in-practice) case that a pool is completely full.
        // Writes to these are harmless - they are never part of nodePool/etc. and
        // are therefore never iterated by advanceTransformBuffer/updateAllTransforms,
        // so the caller never gets a null pointer, the object just silently does not
        // interpolate until something frees up a real slot.
        NodeTransforms nodeOverflowSink;
        CameraTransforms cameraOverflowSink;
        OldBoneTransforms oldBoneOverflowSink;
        BoneTransforms boneOverflowSink;
        PassTransforms passOverflowSink;
        TrackedDatablock datablockOverflowSink;

        // Pool storage for tracked nodes. resize()'d to NODE_POOL_CAPACITY exactly
        // once, in the constructor, BEFORE the render/logic threads start - every
        // slot physically exists from that point on, and nodePool.size() never
        // changes again for the lifetime of the GraphicsModule. That is what makes
        // it safe for any thread to read nodePool.size() (or just iterate the whole
        // vector) without any synchronization at all: there is no concurrent writer
        // to race against, because there is no writer, ever, after construction.
        // "Is this slot in use" is carried entirely by each slot's own active/node
        // fields, not by the vector's size - an unused slot is simply a fully
        // constructed NodeTransforms sitting at active==false.
        std::vector<NodeTransforms> nodePool;
        std::unordered_map<Ogre::Node*, size_t> nodeToIndexMap;
        std::vector<size_t> freeNodeSlots;
        // Guards nodePool's structure (growth/recycling) + nodeToIndexMap + freeNodeSlots.
        // Deliberately a plain (non-recursive) mutex - see rule 1 in the threading-model
        // comment. Mutable so the const dumpBufferState() can lock it.
        // advanceTransformBuffer/updateAllTransforms/flushTransforms/
        // dumpBufferState's loops. Mutable so the const dumpBufferState() can lock
        // it if it ever needs to (it currently doesn't, since reads are lock-free).
        mutable std::mutex nodeRegistrationMutex;

        std::vector<CameraTransforms> cameraPool;
        std::unordered_map<Ogre::Camera*, size_t> cameraToIndexMap;
        std::vector<size_t> freeCameraSlots;
        mutable std::mutex cameraRegistrationMutex;

        std::vector<OldBoneTransforms> oldBonePool;
        std::unordered_map<Ogre::v1::OldBone*, size_t> oldBoneToIndexMap;
        std::vector<size_t> freeOldBoneSlots;
        mutable std::mutex oldBoneRegistrationMutex;

        std::vector<BoneTransforms> bonePool;
        std::unordered_map<Ogre::Bone*, size_t> boneToIndexMap;
        std::vector<size_t> freeBoneSlots;
        mutable std::mutex boneRegistrationMutex;

        std::vector<PassTransforms> passPool;
        std::unordered_map<Ogre::Pass*, size_t> passToIndexMap;
        std::vector<size_t> freePassSlots;
        mutable std::mutex passRegistrationMutex;

        std::vector<TrackedDatablock> datablockPool;
        std::unordered_map<Ogre::HlmsDatablock*, size_t> datablockToIndexMap;
        std::vector<size_t> freeDatablockSlots;
        mutable std::mutex datablockRegistrationMutex;

        // Lock-free concurrent queue for closure commands
        moodycamel::ConcurrentQueue<ClosureCommand> closureQueue;

        // Storage for persistent closures (only accessed by render thread)
        std::unordered_map<Ogre::String, PersistentClosure> persistentClosures;

        std::atomic<MyGUI::Widget*> myGUIFocusWidget{nullptr};

        // Producer token for better performance (used by logic thread)
        moodycamel::ProducerToken producerToken;

        // Consumer token for better performance (used by render thread)
        moodycamel::ConsumerToken consumerToken;

        // Atomic flag to ensure proper initialization
        std::atomic<bool> queueInitialized{false};

        std::atomic<size_t> currentTransformNodeIdx;
        std::atomic<size_t> currentTransformCameraIdx;
        std::atomic<size_t> currentTransformOldBoneIdx;
        std::atomic<size_t> currentTransformBoneIdx;
        std::atomic<size_t> currentTransformPassIdx;
        std::atomic<size_t> currentTrackedDatablockIdx;
        Ogre::Real interpolationWeight;

        Ogre::Real accumTimeSinceLastLogicFrame;
        Ogre::Real frameTime;
        bool debugVisualization;

        std::atomic<size_t> currentDestroySlot;
        std::array<std::vector<DestroyCommand>, NUM_DESTROY_SLOTS> destroySlots;
        std::atomic<bool> isRunningWaitClosure;

        // Destroy commands that arrived while a wait closure was in flight.
        // Written and read exclusively on the logic thread — no lock needed.
        std::vector<std::pair<DestroyCommand, std::string>> deferredDestroyCommands;

        std::atomic<bool> stallRequested;
        std::atomic<bool> stallAcknowledged;
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

#endif