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
	, m_solverMode(4)
	, m_threadsRequested(std::max(1u, std::thread::hardware_concurrency()))
	, m_defaultLinearDamping(0.01f)
	, m_defaultAngularDamping(0.05f, 0.05f, 0.05f)
	, m_gravity(0.0f, -19.81f, 0.0f)
	, m_debugger(nullptr)
	, m_mainThreadId()
{
	SetThreadCount(m_threadsRequested);
	setSolverModel(m_solverMode);
	SetSubSteps(1);
	setUpdateFPS(desiredFps, maxUpdatesPerFrames);

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

	// m_defaultMatID is destroyed elsewhere

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

// Prevents the following issues:
// The sim runs slow because you�re feeding OgreNewt a fixed 1 / 60 and OgreNewt is also using its own fixed timestep accumulator
// So you re effectively doing fixed - step on fixed - step, and the inner one can easily end up stepping less often than you think
// (especially when dt == dtFixed and you use while (accum > dtFixed)), which yields 0 physics steps some frames -> slow motion / stutter.
void World::update(Ogre::Real timestep)
{
	m_isSimulating.store(true, std::memory_order_release);

	// You are already calling this with fixedDt from AppStateManager.
	const ndFloat32 dt = static_cast<ndFloat32>(timestep);

	// Safe point before stepping
	processPhysicsQueue();
	
	// Step exactly once (or multiple times if you want substeps explicitly)
	// ndWorld::Update(dt);
	// ndWorld::Sync();

	/*const int subSteps = 2;
	const ndFloat32 subDt = dt / ndFloat32(subSteps);
	for (int i = 0; i < subSteps; ++i)
	{
		ndWorld::Update(subDt);
	}*/

	ndWorld::Update(dt);

	ndWorld::Sync();

	// Safe point after stepping
	processPhysicsQueue();
	
	// Publish discrete transforms (let your engine interpolation handle visuals)
	postUpdate(1.0f);

	m_isSimulating.store(false, std::memory_order_release);
}

// -----------------------------------------------------------------------------
// Update loop: main-thread driven, ND4 uses internal worker threads.
// Pattern: catch-up with Update(), then Sync(), then safe point work.
// -----------------------------------------------------------------------------

void World::updateFixed(Ogre::Real timestep)
{
	m_isSimulating.store(true, std::memory_order_release);

	const double dtFixed = double(m_fixedTimestep);
	const int    maxSteps = m_maxTicksPerFrames;

	double dt = double(timestep);
	const double maxDelta = dtFixed * double(maxSteps);
	if (dt > maxDelta) dt = maxDelta;

	m_timeAccumulator += dt;

	processPhysicsQueue();
	
	constexpr double eps = 1e-12;

	// bound accumulator
	int pendingSteps = int(std::floor((m_timeAccumulator + eps) / dtFixed));
	if (pendingSteps > maxSteps)
	{
		const int drop = pendingSteps - maxSteps;
		m_timeAccumulator -= dtFixed * double(drop);
		pendingSteps = maxSteps;
	}

	while (m_timeAccumulator + eps >= dtFixed)
	{
		ndWorld::Update((ndFloat32)dtFixed);
		// ndWorld::Sync(); // barrier per step
		m_timeAccumulator -= dtFixed;
	}

	if (m_timeAccumulator < eps) m_timeAccumulator = 0.0;

	const float interp = (dtFixed > 0.0) ? float(m_timeAccumulator / dtFixed) : 0.0f;

	processPhysicsQueue();
	
	postUpdate(interp);   // World interpolates or exposes interpolated transforms
	m_isSimulating.store(false, std::memory_order_release);
}

void World::postUpdate(Ogre::Real interp)
{
	const ndArray<ndBodyKinematic*>& view = GetBodyList().GetView();

	// Pass 1: publish transforms
	for (ndInt32 i = ndInt32(view.GetCount()) - 1; i >= 0; --i)
	{
		ndBodyKinematic* const ndBody = view[i];
		if (!ndBody || ndBody->GetSleepState()) continue;

		if (auto* notify = dynamic_cast<OgreNewt::BodyNotify*>(ndBody->GetNotifyCallback()))
		{
			if (auto* ogreBody = notify->GetOgreNewtBody())
				ogreBody->updateNode(interp);
		}
	}

	// Pass 2: dispatch contacts (game logic, after all transforms are published)
	for (ndInt32 i = ndInt32(view.GetCount()) - 1; i >= 0; --i)
	{
		ndBodyKinematic* const ndBody = view[i];
		if (!ndBody) continue;

		if (auto* notify = dynamic_cast<OgreNewt::BodyNotify*>(ndBody->GetNotifyCallback()))
		{
			if (auto* ogreBody = notify->GetOgreNewtBody())
				ogreBody->dispatchContacts();
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


// -----------------------------------------------------------------------------
// Blocking convexcast job system (safe from any thread)
// -----------------------------------------------------------------------------


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