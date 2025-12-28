#ifndef _INCLUDE_OGRENEWT_WORLD
#define _INCLUDE_OGRENEWT_WORLD

#include "OgreNewt_Prerequisites.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_Debugger.h"
#include "OgreNewt_MaterialPair.h"
#include "OgreNewt_MaterialID.h"
#include "OgreNewt_RayCast.h"
#include "OgreNewt_ConvexCast.h"

#include "ndWorld.h"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <future>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace OgreNewt
{
	class BodyNotify;

	class _OgreNewtExport World : public ndWorld
	{
	public:
		typedef OgreNewt::function<void(OgreNewt::Body*, int threadIndex)> LeaveWorldCallback;

	public:
		World(Ogre::Real desiredFps = 100.0f, int maxUpdatesPerFrames = 5, const Ogre::String& name = "main");
		~World() override;

		void update(Ogre::Real timestep);

		void updateFixed(Ogre::Real timestep);

		void recover();
		int getVersion() const;

		void cleanUp();
		void waitForUpdateToFinish();

		void setUpdateFPS(Ogre::Real desiredFps, int maxUpdatesPerFrames);
		Ogre::Real getUpdateFPS() const { return m_updateFPS; }
		Ogre::Real getDesiredFps(void) const { return m_desiredFps; }

		void setSolverModel(int mode);
		int getSolverModel() const { return m_solverMode; }

		void setThreadCount(int threads);
		int  getThreadCount() const { return m_threadsRequested; }

		void setGravity(const Ogre::Vector3& g);
		Ogre::Vector3 getGravity() const;

		int getMemoryUsed(void) const;

		int getBodyCount() const;
		int getConstraintCount() const;

		void assertMainThread(void) const;

		void registerMaterialPair(MaterialPair* pair);
		void unregisterMaterialPair(MaterialPair* pair);
		MaterialPair* findMaterialPair(int id0, int id1) const;

		const MaterialID* getDefaultMaterialID() const { return m_defaultMatID; }

		void setPaused(bool p) { m_paused.store(p); }
		bool isPaused() const { return m_paused.load(); }
		void singleStep() { m_doSingleStep.store(true); }

		void setDefaultLinearDamping(Ogre::Real v) { m_defaultLinearDamping = v; }
		void setDefaultAngularDamping(const Ogre::Vector3& v) { m_defaultAngularDamping = v; }
		Ogre::Real getDefaultLinearDamping() const { return m_defaultLinearDamping; }
		Ogre::Vector3 getDefaultAngularDamping() const { return m_defaultAngularDamping; }

		Debugger& getDebugger() const { return *m_debugger; }

		ndWorld* getNewtonWorld() const { return const_cast<World*>(this); }

		void setJointRecursiveCollision(const OgreNewt::Body* root, bool enable);

		// ---------------------------------------------------------------------
		// Thread identity / safe point
		// ---------------------------------------------------------------------
		bool isMainThread(void) const;
		bool isSimulating() const;
		// ---------------------------------------------------------------------
		// DIRECT world mutations (NO defer)
		// Rules:
		// - Must be called from update-thread (main thread)
		// - If simulating: only allowed inside safe point (after Sync() in update)
		// ---------------------------------------------------------------------
		void addBody(ndBodyKinematic* body);

		void destroyBody(ndBodyKinematic* body);

		void addJoint(const ndSharedPtr<ndJointBilateralConstraint>& joint);

		void destroyJoint(ndJointBilateralConstraint* joint);

		// ---------------------------------------------------------------------
		// Blocking raycast / convexcast (safe from any thread)
		// These are executed in the safe point inside update().
		// ---------------------------------------------------------------------
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

		BasicConvexcast::ConvexcastContactInfo convexcastBlocking(
			const ndShapeInstance& shape,
			const Ogre::Vector3& startpt,
			const Ogre::Quaternion& orientation,
			const Ogre::Vector3& endpt,
			OgreNewt::Body* ignoreBody,
			int maxContactsCount = 16,
			int threadIndex = 0);

		void processRaycastJobs(void);
		void processConvexcastJobs(void);

		// ND4 hooks (kept minimal)
		void PreUpdate(ndFloat32 timestep) override;
		void PostUpdate(ndFloat32 timestep) override;
		void OnSubStepPreUpdate(ndFloat32 timestep) override;
		void OnSubStepPostUpdate(ndFloat32 timestep) override;

	public:
		template <class Fn>
		void enqueuePhysics(Fn&& fn)
		{
			using FnT = std::decay_t<Fn>;

			// If already on main thread OR not simulating yet: execute immediately.
			// (important for scene load / no pump yet)
			if (isMainThread() || !isSimulating())
			{
				fn(*this);
				return;
			}

			ICommand* cmd = new Command<FnT>(FnT(std::forward<Fn>(fn)), nullptr);
			enqueueCommandInternal(cmd);
		}

		template <class Fn>
		void enqueuePhysicsAndWait(Fn&& fn)
		{
			using FnT = std::decay_t<Fn>;

			if (isMainThread() || !isSimulating())
			{
				fn(*this);
				return;
			}

			auto done = std::make_shared<std::promise<void>>();
			auto fut = done->get_future();

			ICommand* cmd = new Command<FnT>(FnT(std::forward<Fn>(fn)), done);
			enqueueCommandInternal(cmd);

			fut.wait(); // blocks until world thread pumps queue
		}
	private:
		void postUpdate(Ogre::Real interp);

		void recoverInternal();
		void internalCleanUp();

		void assertWritableNow(void) const;

		struct ICommand
		{
			virtual ~ICommand() = default;
			virtual void run(World& world) = 0;
		};

		template <class Fn>
		struct Command final : ICommand
		{
			Fn m_fn;
			std::shared_ptr<std::promise<void>> m_done; // optional

			Command(Fn&& fn, std::shared_ptr<std::promise<void>> done)
				: m_fn(std::move(fn))
				, m_done(std::move(done))
			{
			}

			void run(World& world) override
			{
				m_fn(world);
				if (m_done)
				{
					m_done->set_value();
				}
			}
		};

		void processPhysicsQueue();
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

			RaycastJob(void)
				: mFrom(Ogre::Vector3::ZERO)
				, mTo(Ogre::Vector3::ZERO)
				, mIgnoreBody(nullptr)
				, mDone(false)
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
		// Timing
		Ogre::String m_name;
		Ogre::Real   m_updateFPS;
		Ogre::Real   m_fixedTimestep;
		double   m_timeAccumulator;
		int          m_maxTicksPerFrames;
		Ogre::Real   m_invFixedTimestep;
		Ogre::Real   m_desiredFps;

		// Book-keeping
		int  m_solverMode;
		int  m_threadsRequested;

		MaterialID* m_defaultMatID;
		std::map<std::pair<int, int>, MaterialPair*> m_materialPairs;

		Ogre::Real m_defaultLinearDamping;
		Ogre::Vector3 m_defaultAngularDamping;

		Ogre::Vector3 m_gravity;

		mutable Debugger* m_debugger;

		std::atomic<bool> m_paused{ false };
		std::atomic<bool> m_doSingleStep{ false };

		// Jobs
		std::mutex m_jobsMutex;
		std::vector<std::shared_ptr<RaycastJob>> m_pendingRaycastJobs;
		std::deque<ConvexcastJob*> m_pendingConvexcastJobs;

		// Thread identity: thread that drives update() (main thread)
		std::thread::id m_mainThreadId;
		std::atomic<bool> m_isSimulating{ false };

		std::atomic<unsigned int> m_nextSelfCollisionGroup{ 1 };

		void applySelfCollisionGroup(ndBodyKinematic* start, unsigned int group);
	private:
		// non-template, defined in .cpp
		void enqueueCommandInternal(ICommand* cmd);

		struct CmdQueueImpl;                 // opaque
		std::unique_ptr<CmdQueueImpl> m_impl; // owns queue
	};
}

#endif
