#ifndef _INCLUDE_OGRENEWT_WORLD
#define _INCLUDE_OGRENEWT_WORLD

#include "OgreNewt_Prerequisites.h"
#include "OgreNewt_Body.h"
#include "OgreVector3.h"
#include "OgreNewt_Debugger.h"
#include "OgreNewt_MaterialPair.h"
#include "OgreNewt_MaterialID.h"
#include "OgreNewt_RayCast.h"
#include "OgreNewt_ConvexCast.h"
#include "ndWorld.h"
#include <functional>
#include <mutex>
#include <vector>

namespace OgreNewt
{
    class _OgreNewtExport World : public ndWorld
    {
    public:
        typedef OgreNewt::function<void(OgreNewt::Body*, int threadIndex)> LeaveWorldCallback;
    public:
        World(Ogre::Real desiredFps = 100.0f,
            int maxUpdatesPerFrames = 5,
            const Ogre::String& name = "main");
        ~World() override;

        // --- Legacy/engine-facing API (unchanged names) ---
        void update(Ogre::Real t_step);
        void postUpdate(Ogre::Real interp);  // called by your render loop after update()
        void recover();                      // wake bodies & force transforms refresh
        int getVersion() const;

        void cleanUp();                      // clear caches, end step safely
        void clearCache();                   // ndWorld::ClearCache
        void waitForUpdateToFinish();        // ndWorld::Sync

        void setUpdateFPS(Ogre::Real desiredFps, int maxUpdatesPerFrames);
        Ogre::Real getUpdateFPS() const { return m_updateFPS; }
        Ogre::Real getDesiredFps(void) const { return m_desiredFps; }

        // Solver / threads / gravity / damping controls
        void setSolverModel(int mode);
        int getSolverModel() const { return m_solverMode; }

        void setThreadCount(int threads);
        int  getThreadCount() const { return m_threadsRequested; }

        void setGravity(const Ogre::Vector3& g);
        Ogre::Vector3 getGravity() const;

        int getMemoryUsed(void) const;

        int getBodyCount() const;
        int getConstraintCount() const;

        void registerMaterialPair(MaterialPair* pair);

        void unregisterMaterialPair(MaterialPair* pair);

        MaterialPair* World::findMaterialPair(int id0, int id1) const;

        const MaterialID* getDefaultMaterialID() const { return m_defaultMatID; }

        void setPaused(bool p) { m_paused.store(p); }
        bool isPaused() const { return m_paused.load(); }
        void singleStep() { m_doSingleStep.store(true); }

        void setDefaultLinearDamping(Ogre::Real v) { m_defaultLinearDamping = v; }
        void setDefaultAngularDamping(const Ogre::Vector3 &v) { m_defaultAngularDamping = v; }
        Ogre::Real getDefaultLinearDamping()  const { return m_defaultLinearDamping; }
        Ogre::Vector3 getDefaultAngularDamping() const { return m_defaultAngularDamping; }

        bool isSimulating() const { return m_isSimulating.load(std::memory_order_relaxed); }

        void setSimulating(bool state) { m_isSimulating.store(state); }

        Debugger& getDebugger() const { return *m_debugger; }

        // Some engines rely on accessing the raw ndWorld*
        ndWorld* getNewtonWorld() const { return const_cast<World*>(this); }

        // Safe deferrals from physics step to main thread (e.g. picker callback swaps)
        void deferAfterPhysics(std::function<void()> fn);

        // --- ndWorld overrides/hooks (optional but aligned with ND4) ---
        void PreUpdate(ndFloat32 timestep) override;
        void PostUpdate(ndFloat32 timestep) override;
        void OnSubStepPreUpdate(ndFloat32 timestep) override;
        void OnSubStepPostUpdate(ndFloat32 timestep) override;

        // Synchronous raycast that is safe from *any* thread.
        // Internally it runs the actual raycast on the physics/update thread.
        struct RaycastResult
        {
            OgreNewt::BasicRaycast::BasicRaycastInfo mInfo;

            RaycastResult(void)
            {
                mInfo.mBody = nullptr;
                mInfo.mDistance = 0.0f;
                mInfo.mNormal = Ogre::Vector3::ZERO;
            }
        };

        RaycastResult raycastBlocking(const Ogre::Vector3& fromPosition, const Ogre::Vector3& toPosition, OgreNewt::Body* ignoreBody = nullptr);

        // Call from World::update on the physics thread
        void processRaycastJobs(void);

        // Blocking convexcast API, like blockingRayCast
        BasicConvexcast::ConvexcastContactInfo blockingConvexcast(
            const ndShapeInstance& shape,
            const Ogre::Vector3& startpt,
            const Ogre::Quaternion& orientation,
            const Ogre::Vector3& endpt,
            OgreNewt::Body* ignoreBody,
            int maxContactsCount = 16,
            int threadIndex = 0);




        // Optional helper to detect physics thread
        bool isPhysicsThread(void) const;

        void deferRemoveJoint(ndJointBilateralConstraint* joint);
    private:
        struct RaycastJob
        {
            Ogre::Vector3 mFrom;
            Ogre::Vector3 mTo;
            OgreNewt::Body* mIgnoreBody;

            RaycastResult mResult;
            bool mDone;

            std::mutex mMutex;
            std::condition_variable mCondVar;

            RaycastJob(void) :
                mFrom(Ogre::Vector3::ZERO),
                mTo(Ogre::Vector3::ZERO),
                mIgnoreBody(nullptr),
                mDone(false)
            {
            }
        };

        struct ConvexcastJob
        {
            const ndShapeInstance* shape;
            Ogre::Vector3 startpt;
            Ogre::Quaternion orientation;
            Ogre::Vector3 endpt;
            OgreNewt::Body* ignoreBody;
            int maxContactsCount;
            int threadIndex;

            BasicConvexcast::ConvexcastContactInfo result;
            bool hasHit;

            bool done;
            std::mutex mtx;
            std::condition_variable cv;

            ConvexcastJob()
                : shape(nullptr)
                , ignoreBody(nullptr)
                , maxContactsCount(0)
                , threadIndex(0)
                , hasHit(false)
                , done(false)
            {
            }
        };

    private:
        void recoverInternal();

        void flushDeferred();

        void enqueueRaycastJob(const std::shared_ptr<RaycastJob>& job);

        void enqueueConvexcastJob(ConvexcastJob* job);

        void processConvexcastJobs(void);
    private:

        // Timing / stepping state
        Ogre::String m_name;
        Ogre::Real   m_updateFPS;         // user-requested FPS
        Ogre::Real   m_fixedTimestep;     // 1 / m_updateFPS
        Ogre::Real   m_timeAccumulator;   // fixed-step accumulator
        int          m_maxTicksPerFrames; // clamp for accumulator
        Ogre::Real   m_invFixedTimestep;
		Ogre::Real   m_desiredFps;

        // Book-keeping for threads/solver
        int  m_solverMode;
        int             m_threadsRequested;

        MaterialID*     m_defaultMatID;
        std::map<std::pair<int, int>, MaterialPair*> m_materialPairs;

        // Defaults used by new bodies (your API preserved)
        Ogre::Real m_defaultLinearDamping;
        Ogre::Vector3 m_defaultAngularDamping;

        // World gravity shadow (for quick get/set in Ogre math)
        ndVector   m_gravity;
        std::atomic<bool> m_isSimulating = false;

        mutable Debugger* m_debugger;

        std::atomic<bool> m_paused{ false };
        std::atomic<bool> m_doSingleStep{ false };

        // ND4 sync helpers
        mutable ndSpinLock m_lock;  // protects mutations/reads across threads

        // Deferral queue (from worker to main)
        std::mutex m_deferMutex;
        std::vector<std::function<void()>> m_deferred;
        LeaveWorldCallback m_leaveCallback;


        std::vector<std::shared_ptr<RaycastJob>> mPendingRaycastJobs;
        std::mutex m_jobsMutex;
        std::vector<ndJointBilateralConstraint*> mDeferredJointRemovals;
        std::mutex mDeferredMutex;
        std::thread::id m_physicsThreadId;
        std::deque<ConvexcastJob*> m_convexcastJobs;
    };
}

#define SAFE_DEFER(world, code) \
    if (world && world->isSimulating()) { \
        world->deferAfterPhysics([=]() { code; }); \
    } else { \
        code; \
    }

#endif
