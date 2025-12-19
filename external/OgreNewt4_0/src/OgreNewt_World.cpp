#include "OgreNewt_Stdafx.h"
#include "OgreNewt_World.h"
#include "OgreNewt_BodyNotify.h"
#include "OgreNewt_ContactNotify.h"

#include "OgreNewt_ConcurrentQueue.h"

using namespace OgreNewt;

struct World::CmdQueueImpl
{
	moodycamel::ConcurrentQueue<ICommand*> q;
};

World::World(Ogre::Real desiredFps, int maxUpdatesPerFrames, const Ogre::String& name)
	: ndWorld(),
	m_impl(std::make_unique<CmdQueueImpl>())
	, m_name(name)
	, m_updateFPS(desiredFps > 1.0f ? desiredFps : 120.0f)
	, m_fixedTimestep(1.0f / (m_updateFPS > 1.0f ? m_updateFPS : 100.0f))
	, m_timeAccumulator(0.0f)
	, m_maxTicksPerFrames(maxUpdatesPerFrames > 0 ? maxUpdatesPerFrames : 5)
	, m_invFixedTimestep(1.0f / m_fixedTimestep)
	, m_desiredFps(desiredFps)
	, m_solverMode(6)
	, m_threadsRequested(std::max(1u, std::thread::hardware_concurrency()))
	, m_defaultLinearDamping(0.01f)
	, m_defaultAngularDamping(0.05f, 0.05f, 0.05f)
	, m_gravity(0.0f, -19.81f, 0.0f)
	, m_debugger(nullptr)
	, m_mainThreadId()
{
	SetThreadCount(1);
	setSolverModel(m_solverMode);
	SetSubSteps(2);

	m_defaultMatID = new OgreNewt::MaterialID(this, 0);

	m_debugger = new Debugger(this);

	OgreNewt::ContactNotify* notify = new OgreNewt::ContactNotify(this);
	SetContactNotify(notify);

	if (m_mainThreadId == std::thread::id())
	{
		m_mainThreadId = std::this_thread::get_id();
	}
}

World::~World()
{
	Sync();

	ClearCache();

	if (m_debugger)
	{
		delete m_debugger;
		m_debugger = nullptr;
	}

	if (m_defaultMatID)
	{
		delete m_defaultMatID;
		m_defaultMatID = nullptr;
	}

	m_materialPairs.clear();
}

int World::getVersion() const
{
	return 400;
}

void World::cleanUp()
{
	// Thread-agnostic public API
	enqueuePhysicsAndWait([](World& w)
		{
			w.internalCleanUp();
		});
}

void World::internalCleanUp()
{
	// MUST be on world thread (or not simulating)
	// put your real cleanup logic here:
	Sync();
	// drain pending remove joints/bodies etc
	ClearCache();
}

void World::waitForUpdateToFinish()
{
	Sync();
}

void World::setUpdateFPS(Ogre::Real desiredFps, int maxUpdatesPerFrames)
{
	m_updateFPS = std::max<Ogre::Real>(1.0f, desiredFps);
	m_fixedTimestep = 1.0f / m_updateFPS;
	m_invFixedTimestep = 1.0f / m_fixedTimestep;
	m_maxTicksPerFrames = std::max(1, maxUpdatesPerFrames);
}

void World::setSolverModel(int mode)
{
	m_solverMode = mode;
	SetSolverIterations(m_solverMode);
}

void World::setThreadCount(int threads)
{
	m_threadsRequested = std::max(1, threads);
	ndWorld::SetThreadCount(m_threadsRequested);
}

void World::setGravity(const Ogre::Vector3& g)
{
	m_gravity = g;
}

Ogre::Vector3 World::getGravity() const
{
	return m_gravity;
}

int World::getMemoryUsed(void) const
{
	return 0;
}

int World::getBodyCount() const
{
	return GetBodyList().GetCount();
}

int World::getConstraintCount() const
{
	return GetJointList().GetCount();
}

void World::registerMaterialPair(MaterialPair* pair)
{
	int id0 = pair->getMaterial0()->getID();
	int id1 = pair->getMaterial1()->getID();
	if (id0 > id1) std::swap(id0, id1);
	m_materialPairs[std::make_pair(id0, id1)] = pair;
}

void World::unregisterMaterialPair(MaterialPair* pair)
{
	if (!pair) return;

	int id0 = pair->getMaterial0()->getID();
	int id1 = pair->getMaterial1()->getID();
	if (id0 > id1) std::swap(id0, id1);

	auto it = m_materialPairs.find(std::make_pair(id0, id1));
	if (it != m_materialPairs.end())
	{
		m_materialPairs.erase(it);
	}
}

MaterialPair* World::findMaterialPair(int id0, int id1) const
{
	if (id0 > id1) std::swap(id0, id1);
	auto it = m_materialPairs.find(std::make_pair(id0, id1));
	return (it != m_materialPairs.end()) ? it->second : nullptr;
}

bool World::isMainThread(void) const
{
	return std::this_thread::get_id() == m_mainThreadId;
}

bool OgreNewt::World::isSimulating() const
{
	return m_isSimulating.load(std::memory_order_acquire);
}

void World::assertMainThread(void) const
{
	ndAssert(isMainThread());
}

void World::assertWritableNow(void) const
{
	assertMainThread();

	if (isSimulating())
	{
		ndAssert(isMainThread());
	}
}

void World::addBody(ndBodyKinematic* body)
{
	if (!body)
		return;

	AddBody(body);
}

void World::destroyBody(ndBodyKinematic* body)
{
	if (!body)
		return;

	body->SetNotifyCallback(nullptr);

	Sync();
	RemoveBody(body);
}

void World::addJoint(const ndSharedPtr<ndJointBilateralConstraint>& joint)
{
	if (!joint)
		return;

	AddJoint(joint);
}

void World::destroyJoint(ndJointBilateralConstraint* joint)
{
	if (!joint)
		return;

	RemoveJoint(joint);
}

// -----------------------------------------------------------------------------
// Update loop: main-thread driven, ND4 uses internal worker threads.
// Pattern: catch-up with Update(), then Sync(), then safe point work.
// -----------------------------------------------------------------------------
#if 0
void World::update(Ogre::Real timestep)
{
	// optional: remember the physics thread id (main thread in your case)
	if (m_mainThreadId == std::thread::id())
		m_mainThreadId = std::this_thread::get_id();

	m_isSimulating.store(true, std::memory_order_release);

	// Pause / single step
	if (m_paused.load(std::memory_order_acquire))
	{
		if (!m_doSingleStep.exchange(false, std::memory_order_acq_rel))
		{
			// Still allow queued lifetime ops + editor queries while paused
			processPhysicsQueue();
			processRaycastJobs();
			processConvexcastJobs();

			m_isSimulating.store(false, std::memory_order_release);
			return;
		}
	}

	const Ogre::Real dt = m_fixedTimestep;
	const int maxSteps = m_maxTicksPerFrames;

	// Clamp / drop excessive time (same as your old loop)
	if (timestep > (dt * Ogre::Real(maxSteps)))
		timestep = dt * Ogre::Real(maxSteps);

	m_timeAccumulator += timestep;

	// Safe point BEFORE stepping: apply cross-thread add/destroy, etc.
	processPhysicsQueue();

	// Catch-up: restore old behavior (>=)
	bool didStep = false;
	while (m_timeAccumulator > dt)
	{
		ndWorld::Update(static_cast<ndFloat32>(dt));
		m_timeAccumulator -= dt;
		didStep = true;

		if (m_paused.load(std::memory_order_acquire))
			break;
	}

	// Interpolation factor for rendering
	const Ogre::Real interp = m_timeAccumulator * m_invFixedTimestep;

	// One sync per frame (like demo when synchronous)
	if (didStep)
		ndWorld::Sync();
	// else
		// ndWorld::Sync(); // optional: keep it if you rely on a stable world even with 0 steps

	// Safe point AFTER sync: now workers are idle, safe to mutate/query
	processPhysicsQueue();
	processRaycastJobs();
	processConvexcastJobs();

	postUpdate(1.0f/*interp*/);

	m_isSimulating.store(false, std::memory_order_release);
}
#endif

#if 0
void World::update(Ogre::Real timestep)
{
	m_isSimulating.store(true, std::memory_order_release);

	// Work in DOUBLE for all stepping math.
	const double dtFixed = static_cast<double>(m_fixedTimestep);
	const int    maxSteps = m_maxTicksPerFrames;

	// Clamp incoming dt to avoid spikes/spiral
	double dt = static_cast<double>(timestep);
	const double maxDelta = dtFixed * double(maxSteps);
	if (dt > maxDelta)
		dt = maxDelta;

	m_timeAccumulator += dt;

	// Safe point before stepping
	processPhysicsQueue();
	processRaycastJobs();
	processConvexcastJobs();

	// Drop steps if too far behind (keep accumulator bounded)
	const double maxAccum = dtFixed * double(maxSteps);
	if (m_timeAccumulator > maxAccum)
	{
		const double stepsToDrop = floor(m_timeAccumulator / dtFixed) - double(maxSteps);
		if (stepsToDrop > 0.0)
			m_timeAccumulator -= dtFixed * stepsToDrop;
	}

	bool didStep = false;

	// IMPORTANT: use > not >= to avoid “exact boundary” jitter.
	while (m_timeAccumulator > dtFixed)
	{
		ndWorld::Update(static_cast<ndFloat32>(dtFixed));
		m_timeAccumulator -= dtFixed;
		didStep = true;
	}

	// Kill tiny drift so it can't accumulate into a rare extra/missed step
	if (m_timeAccumulator < 1e-9)
		m_timeAccumulator = 0.0;

	const float interp = (dtFixed > 0.0) ? float(m_timeAccumulator / dtFixed) : 0.0f;

	if (didStep)
		ndWorld::Sync();

	// Safe point after stepping
	processPhysicsQueue();
	processRaycastJobs();
	processConvexcastJobs();

	postUpdate(interp); // or postUpdate(1.0f) if you let your engine handle interpolation

	m_isSimulating.store(false, std::memory_order_release);
}
#endif

#if 0
void World::update(Ogre::Real t_step)
{
	// Remember the thread that drives physics (your logic thread)
	if (m_mainThreadId == std::thread::id())
		m_mainThreadId = std::this_thread::get_id();

	m_isSimulating.store(true, std::memory_order_release);

	// Pause / single step
	if (m_paused.load(std::memory_order_acquire))
	{
		if (!m_doSingleStep.exchange(false, std::memory_order_acq_rel))
		{
			// Still allow queued lifetime ops + editor queries while paused
			processPhysicsQueue();
			processRaycastJobs();
			processConvexcastJobs();

			m_isSimulating.store(false, std::memory_order_release);
			return;
		}
		// else: allow exactly one fixed step below
		t_step = m_fixedTimestep;
	}

	const Ogre::Real dt = m_fixedTimestep;
	const int maxSteps = m_maxTicksPerFrames;

	// Clamp / drop excessive time (same as OgreNewt3)
	if (t_step > (dt * Ogre::Real(maxSteps)))
		t_step = dt * Ogre::Real(maxSteps);

	m_timeAccumulator += t_step;

	// Safe point BEFORE stepping: apply cross-thread add/destroy, etc.
	processPhysicsQueue();

	int realUpdates = 0;

	// Like OgreNewt3: >= to run exact boundary steps too
	while (m_timeAccumulator >= dt)
	{
		ndWorld::Update(static_cast<ndFloat32>(dt));
		m_timeAccumulator -= dt;
		++realUpdates;

		// If pause toggled while stepping, stop after this step
		if (m_paused.load(std::memory_order_acquire))
			break;
	}

	// In OgreNewt3: param = accumulator / dt
	const Ogre::Real param = m_timeAccumulator * m_invFixedTimestep;

	// ND4: Sync only if we actually stepped this frame (mirrors "wait finished")
	if (realUpdates > 0)
		ndWorld::Sync();

	// Safe point AFTER sync: now workers are idle, safe to mutate/query
	processPhysicsQueue();
	processRaycastJobs();
	processConvexcastJobs();

	// Your equivalent of body->updateNode(param)
	postUpdate(param);

	m_isSimulating.store(false, std::memory_order_release);
}
#endif

#if 1
// Prevents the following issues:
// The sim runs slow because you’re feeding OgreNewt a fixed 1 / 60 and OgreNewt is also using its own fixed timestep accumulator
// So you re effectively doing fixed - step on fixed - step, and the inner one can easily end up stepping less often than you think
// (especially when dt == dtFixed and you use while (accum > dtFixed)), which yields 0 physics steps some frames -> slow motion / stutter.
void World::update(Ogre::Real timestep)
{
	m_isSimulating.store(true, std::memory_order_release);

	// You are already calling this with fixedDt from AppStateManager.
	const ndFloat32 dt = static_cast<ndFloat32>(timestep);

	// Safe point before stepping
	processPhysicsQueue();
	processRaycastJobs();
	processConvexcastJobs();

	// Step exactly once (or multiple times if you want substeps explicitly)
	ndWorld::Update(dt);
	ndWorld::Sync();

	// Safe point after stepping
	processPhysicsQueue();
	processRaycastJobs();
	processConvexcastJobs();

	// Publish discrete transforms (let your engine interpolation handle visuals)
	postUpdate(1.0f);

	m_isSimulating.store(false, std::memory_order_release);
}

#endif

void World::postUpdate(Ogre::Real interp)
{
	const ndBodyListView& bodyList = GetBodyList();
	const ndArray<ndBodyKinematic*>& view = bodyList.GetView();

	for (ndInt32 i = ndInt32(view.GetCount()) - 1; i >= 0; --i)
	{
		ndBodyKinematic* const ndBody = view[i];
		if (!ndBody)
			continue;

		if (!ndBody->GetSleepState())
		{
			if (auto* notify = ndBody->GetNotifyCallback())
			{
				if (auto* ogreNotify = dynamic_cast<BodyNotify*>(notify))
				{
					if (auto* ogreBody = ogreNotify->GetOgreNewtBody())
					{
						ogreBody->updateNode(interp);
					}
				}
			}
		}
	}
}

void World::recoverInternal()
{
	for (auto node = GetBodyList().GetFirst(); node; node = node->GetNext())
	{
		// IMPORTANT: keep reference, do not copy shared ptr
		auto& bodySp = node->GetInfo();              // bodySp is ndSharedPtr<ndBody>&
		ndBodyKinematic* const b = bodySp->GetAsBodyKinematic();  // raw pointer, no refcount change

		if (!b || b == GetSentinelBody())
			continue;

		if (ndBodyDynamic* dyn = b->GetAsBodyDynamic())
		{
			const ndMatrix m = b->GetMatrix();
			dyn->SetMatrixUpdateScene(m);
			dyn->SetAutoSleep(true);
			dyn->SetSleepState(false);
		}
	}
}

void World::recover()
{
	// Thread-agnostic public API
	enqueuePhysicsAndWait([](World& w)
		{
			w.recoverInternal();
		});
}

// -----------------------------------------------------------------------------
// ND4 hooks (minimal)
// -----------------------------------------------------------------------------
void World::PreUpdate(ndFloat32 /*timestep*/)
{
}

void World::OnSubStepPreUpdate(ndFloat32 /*timestep*/)
{
}

void World::OnSubStepPostUpdate(ndFloat32 /*timestep*/)
{
}

void World::PostUpdate(ndFloat32 /*timestep*/)
{
}

// -----------------------------------------------------------------------------
// Blocking raycast job system (safe from any thread)
// -----------------------------------------------------------------------------
void World::processRaycastJobs(void)
{
	std::vector<std::shared_ptr<RaycastJob>> jobs;
	{
		std::lock_guard<std::mutex> lock(m_jobsMutex);
		jobs.swap(m_pendingRaycastJobs);
	}

	for (auto& job : jobs)
	{
		OgreNewt::BasicRaycast ray(this, job->mFrom, job->mTo, true);
		OgreNewt::BasicRaycast::BasicRaycastInfo info = ray.getFirstHit();
		job->mResult.mInfo = info;

		{
			std::lock_guard<std::mutex> guard(job->mMutex);
			job->mDone = true;
		}
		job->mCondVar.notify_one();
	}
}

World::RaycastResult World::raycastBlocking(const Ogre::Vector3& fromPosition, const Ogre::Vector3& toPosition, OgreNewt::Body* /*ignoreBody*/)
{
	if (isMainThread())
	{
		World::RaycastResult result;
		OgreNewt::BasicRaycast ray(this, fromPosition, toPosition, true);
		result.mInfo = ray.getFirstHit();
		return result;
	}

	std::shared_ptr<RaycastJob> job = std::make_shared<RaycastJob>();
	job->mFrom = fromPosition;
	job->mTo = toPosition;

	{
		std::lock_guard<std::mutex> lock(m_jobsMutex);
		m_pendingRaycastJobs.push_back(job);
	}

	std::unique_lock<std::mutex> lock(job->mMutex);
	job->mCondVar.wait(lock, [job]()
		{
			return job->mDone;
		});

	return job->mResult;
}

// -----------------------------------------------------------------------------
// Blocking convexcast job system (safe from any thread)
// -----------------------------------------------------------------------------
void World::processConvexcastJobs(void)
{
	std::deque<ConvexcastJob*> jobs;
	{
		std::lock_guard<std::mutex> lock(m_jobsMutex);
		jobs.swap(m_pendingConvexcastJobs);
	}

	for (ConvexcastJob* job : jobs)
	{
		if (!job || !job->shape)
		{
			if (job)
			{
				std::lock_guard<std::mutex> lk(job->mtx);
				job->hasHit = false;
				job->done = true;
				job->cv.notify_one();
			}
			continue;
		}

		BasicConvexcast convex(
			this,
			*job->shape,
			job->startpt,
			job->orientation,
			job->endpt,
			job->maxContactsCount,
			job->threadIndex,
			job->ignoreBody);

		if (convex.getContactsCount() > 0)
		{
			job->result = convex.getInfoAt(0);
			job->hasHit = (job->result.mBody != nullptr);
		}
		else
		{
			job->hasHit = false;
		}

		{
			std::lock_guard<std::mutex> lk(job->mtx);
			job->done = true;
		}
		job->cv.notify_one();
	}
}

BasicConvexcast::ConvexcastContactInfo World::convexcastBlocking(
	const ndShapeInstance& shape,
	const Ogre::Vector3& startpt,
	const Ogre::Quaternion& orientation,
	const Ogre::Vector3& endpt,
	OgreNewt::Body* ignoreBody,
	int maxContactsCount,
	int threadIndex)
{
	BasicConvexcast::ConvexcastContactInfo info;

	// Main thread or newton work thread, execute immediately
	if (isMainThread() || threadIndex >= 0)
	{
		BasicConvexcast convex(this, shape, startpt, orientation, endpt, maxContactsCount, threadIndex, ignoreBody);
		if (convex.getContactsCount() > 0)
			info = convex.getInfoAt(0);
		return info;
	}

	ConvexcastJob job;
	job.shape = &shape;
	job.startpt = startpt;
	job.orientation = orientation;
	job.endpt = endpt;
	job.ignoreBody = ignoreBody;
	job.maxContactsCount = maxContactsCount;
	job.threadIndex = threadIndex;
	job.hasHit = false;
	job.done = false;

	{
		std::lock_guard<std::mutex> lock(m_jobsMutex);
		m_pendingConvexcastJobs.push_back(&job);
	}

	{
		std::unique_lock<std::mutex> lock(job.mtx);
		while (!job.done)
		{
			job.cv.wait(lock);
		}
	}

	if (job.hasHit)
	{
		info = job.result;
	}
	return info;
}

void World::setJointRecursiveCollision(const OgreNewt::Body* root, bool enable)
{
	if (!root) return;

	// must be on your world thread / safe point
	enqueuePhysicsAndWait([root, enable](World& w)
		{
			ndBodyKinematic* start = root->getNewtonBody();
			if (!start || start == w.GetSentinelBody())
				return;

			const unsigned int group = enable ? w.m_nextSelfCollisionGroup.fetch_add(1) : 0;
			w.applySelfCollisionGroup(start, group);
		});
}

void World::applySelfCollisionGroup(ndBodyKinematic* start, unsigned int group)
{
	// Build adjacency from current joint graph (no copies of ndSharedPtr!)
	std::unordered_map<ndBodyKinematic*, std::vector<ndBodyKinematic*>> adj;
	adj.reserve(128);

	const auto& joints = GetJointList();
	for (auto node = joints.GetFirst(); node; node = node->GetNext())
	{
		auto& jSp = node->GetInfo();                      // IMPORTANT: reference, not copy
		ndJointBilateralConstraint* j = jSp.operator->(); // raw joint
		if (!j) continue;

		ndBodyKinematic* b0 = j->GetBody0();
		ndBodyKinematic* b1 = j->GetBody1();

		if (!b0 || !b1) continue;
		if (b0 == GetSentinelBody() || b1 == GetSentinelBody()) continue;

		adj[b0].push_back(b1);
		adj[b1].push_back(b0);
	}

	// BFS over connected component
	std::unordered_set<ndBodyKinematic*> visited;
	visited.reserve(adj.size() + 8);

	std::vector<ndBodyKinematic*> stack;
	stack.reserve(adj.size() + 8);
	stack.push_back(start);

	while (!stack.empty())
	{
		ndBodyKinematic* b = stack.back();
		stack.pop_back();

		if (!b || b == GetSentinelBody()) continue;
		if (!visited.insert(b).second) continue;

		// set group on OgreNewt::Body
		if (auto* n = dynamic_cast<OgreNewt::BodyNotify*>(b->GetNotifyCallback()))
		{
			if (auto* ob = n->GetOgreNewtBody())
			{
				ob->setSelfCollisionGroup(group);
			}
		}

		// traverse neighbors
		auto it = adj.find(b);
		if (it != adj.end())
		{
			for (ndBodyKinematic* nb : it->second)
				stack.push_back(nb);
		}
	}
}

void World::processPhysicsQueue()
{
	// called from World::update() on world thread
	ICommand* cmd = nullptr;
	while (m_impl->q.try_dequeue(cmd))
	{
		if (cmd)
		{
			cmd->run(*this);
			delete cmd;
		}
	}
}

void World::enqueueCommandInternal(ICommand* cmd)
{
	m_impl->q.enqueue(cmd);
}