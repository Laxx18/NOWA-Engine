#include "OgreNewt_Stdafx.h"
#include "OgreNewt_World.h"
#include "OgreNewt_BodyNotify.h"
#include "OgreNewt_ContactNotify.h"

#include "OgreNewt_ConcurrentQueue.h"

using namespace OgreNewt;

struct World::CmdQueueImpl
{
    moodycamel::ConcurrentQueue<ICommand*> q;
    moodycamel::ConcurrentQueue<ndSharedPtr<ndBody>> deadBodies;
};

World::World(Ogre::Real desiredFps, int maxUpdatesPerFrames, const Ogre::String& name) :
    ndWorld(),
    m_impl(std::make_unique<CmdQueueImpl>()),
    m_name(name),
    m_updateFPS(desiredFps > 1.0f ? desiredFps : 120.0f),
    m_fixedTimestep(1.0f / (m_updateFPS > 1.0f ? m_updateFPS : 100.0f)),
    m_timeAccumulator(0.0f),
    m_maxTicksPerFrames(maxUpdatesPerFrames > 0 ? maxUpdatesPerFrames : 5),
    m_invFixedTimestep(1.0f / m_fixedTimestep),
    m_desiredFps(desiredFps),
    m_solverMode(4),
    m_threadsRequested(std::max(1u, std::thread::hardware_concurrency())),
    m_defaultLinearDamping(0.01f),
    m_defaultAngularDamping(0.05f, 0.05f, 0.05f),
    m_gravity(0.0f, -19.81f, 0.0f),
    m_debugger(nullptr),
    m_mainThreadId()
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

    // Drain dead-body queues BEFORE CleanUp() -> ndFreeListAlloc::Flush().
    //
    // m_deadBodiesFree holds bodies already RemoveBody'd — we own the last ref.
    // Clearing here returns their memory to Newton's pool first; Flush() then
    // walks it cleanly.  Without this, Flush() frees them while our shared_ptrs
    // still exist -> the Free() is called again when the vectors destruct -> crash.
    //
    // deadBodies queue holds bodies not yet RemoveBody'd — dropping the ref here
    // leaves Newton's scene list as the last owner; CleanUp()'s while-loop removes
    // and frees them correctly.
    {
        ndSharedPtr<ndBody> bodyPtr;
        while (m_impl->deadBodies.try_dequeue(bodyPtr))
        { /* drop ref */
        }
    }
    m_deadBodiesFree.clear();

    CleanUp();

    if (m_debugger)
    {
        delete m_debugger;
        m_debugger = nullptr;
    }
    m_materialPairs.clear();
}

int World::getVersion() const
{
    return 400;
}

void World::cleanUp()
{
    enqueuePhysicsAndWait(
        [](World& w)
        {
            w.internalCleanUp();
        });
}

void World::internalCleanUp()
{
    Sync();
    // ClearCache();
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
    if (id0 > id1)
    {
        std::swap(id0, id1);
    }
    m_materialPairs[std::make_pair(id0, id1)] = pair;
}

void World::unregisterMaterialPair(MaterialPair* pair)
{
    if (!pair)
    {
        return;
    }
    int id0 = pair->getMaterial0()->getID();
    int id1 = pair->getMaterial1()->getID();
    if (id0 > id1)
    {
        std::swap(id0, id1);
    }
    auto it = m_materialPairs.find(std::make_pair(id0, id1));
    if (it != m_materialPairs.end())
    {
        m_materialPairs.erase(it);
    }
}

MaterialPair* World::findMaterialPair(int id0, int id1) const
{
    if (id0 > id1)
    {
        std::swap(id0, id1);
    }
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

void OgreNewt::World::processPhysicsQueue()
{
    // Remove any bodies enqueued for deferred deletion but never processed by
    // PostUpdate (happens when update() was not called, e.g. editor stop/restart).
    // Must run BEFORE any ndWorld::Update() step so Newton never steps with zombie
    // bodies whose internal state (m_sceneNode, notify ptrs) is already dead.
    flushDeadBodies();

    // Body lifetime is owned exclusively by destroyBody() -> PostUpdate().
    // m_pendingBodyFree is gone.
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

void World::addBody(const ndSharedPtr<ndBody>& bodyPtr)
{
    if (!bodyPtr)
    {
        return;
    }
    // Passes the SAME control block Newton already knows about.
    // Never construct ndSharedPtr<ndBody>(rawPtr) — that makes a second
    // independent control block -> double-free.
    AddBody(bodyPtr);
}

// destroyBody now takes a shared_ptr and enqueues it.
// Called from Body::~Body() — any thread, wait-free.
// PostUpdate() drains the queue and calls RemoveBody at the only safe moment:
// after DeleteDeadContacts, on Newton's own thread.
void World::destroyBody(ndSharedPtr<ndBody> bodyPtr)
{
    if (!bodyPtr)
    {
        return;
    }
    m_impl->deadBodies.enqueue(std::move(bodyPtr));
}

void World::addJoint(const ndSharedPtr<ndJointBilateralConstraint>& joint)
{
    if (!joint/* || (*joint)->IsInWorld()*/)
    {
        return;
    }
    AddJoint(joint); // calls patched ndWorld::AddJoint
}

void World::destroyJoint(ndSharedPtr<ndJointBilateralConstraint> joint)
{
    if (!joint || !(*joint)->IsInWorld())
    {
        return;
    }
    RemoveJoint(*joint); // calls patched ndWorld::RemoveJoint
}

void World::update(Ogre::Real timestep)
{
    struct SimGuard
    {
        std::atomic<bool>& f;
        SimGuard(std::atomic<bool>& f) : f(f)
        {
            f.store(true, std::memory_order_release);
        }
        ~SimGuard()
        {
            f.store(false, std::memory_order_release);
        }
    } guard(m_isSimulating);

    processPhysicsQueue();
    ndWorld::Update(static_cast<ndFloat32>(timestep));
    Sync();
    processPhysicsQueue();
    interalPostUpdate(1.0f);
}

void World::updateFixed(Ogre::Real timestep)
{
    // RAII guard so m_isSimulating is ALWAYS reset, even if an exception fires
    struct SimGuard
    {
        std::atomic<bool>& f;
        SimGuard(std::atomic<bool>& f) : f(f)
        {
            f.store(true, std::memory_order_release);
        }
        ~SimGuard()
        {
            f.store(false, std::memory_order_release);
        }
    } guard(m_isSimulating);

    const double dtFixed = double(m_fixedTimestep);

    const int maxSteps = m_maxTicksPerFrames;

    double dt = double(timestep);
    if (dt > dtFixed * double(maxSteps))
    {
        dt = dtFixed * double(maxSteps);
    }

    m_timeAccumulator += dt;

    processPhysicsQueue();

    constexpr double eps = 1e-12;

    int pendingSteps = int(std::floor((m_timeAccumulator + eps) / dtFixed));
    if (pendingSteps > maxSteps)
    {
        m_timeAccumulator -= dtFixed * double(pendingSteps - maxSteps);
        pendingSteps = maxSteps;
    }

    while (m_timeAccumulator + eps >= dtFixed)
    {
        ndWorld::Update((ndFloat32)dtFixed);
        m_timeAccumulator -= dtFixed;
    }

    if (m_timeAccumulator < eps)
    {
        m_timeAccumulator = 0.0;
    }

    // *** THE CRITICAL FIX ***
    // Last ndWorld::Update() fired an async tick that is still running.
    // Must wait before touching ndFreeListAlloc in processPhysicsQueue().
    Sync();

    processPhysicsQueue();

    const float interp = (dtFixed > 0.0) ? float(m_timeAccumulator / dtFixed) : 0.0f;
    interalPostUpdate(interp);
}

void World::interalPostUpdate(Ogre::Real interp)
{
    const ndArray<ndBodyKinematic*>& view = GetBodyList().GetView();

    for (ndInt32 i = ndInt32(view.GetCount()) - 1; i >= 0; --i)
    {
        ndBodyKinematic* const ndBody = view[i];
        if (!ndBody || !ndBody->GetScene() || ndBody->GetSleepState())
        {
            continue;
        }

        auto notifyPtr = ndBody->GetNotifyCallback();
        if (!notifyPtr)
        {
            continue;
        }

        auto* notify = dynamic_cast<OgreNewt::BodyNotify*>(*notifyPtr);
        if (!notify)
        {
            continue;
        }

        auto* ogreBody = notify->GetOgreNewtBody();
        if (!ogreBody)
        {
            continue;
        }

        // One pass: update transform AND dispatch contacts
        ogreBody->updateNode(interp, false);
        ogreBody->dispatchContacts();
    }
}

void World::recoverInternal()
{
    flushDeadBodies();

    for (auto node = GetBodyList().GetFirst(); node; node = node->GetNext())
    {
        auto bodySp = node->GetInfo();
        ndBodyKinematic* const b = bodySp->GetAsBodyKinematic();

        if (!b || b == GetSentinelBody())
        {
            continue;
        }

        // Skip static/infinite-mass bodies — they never moved
        if (b->GetInvMass() == 0.0f)
        {
            continue;
        }

        if (ndBodyDynamic* dyn = b->GetAsBodyDynamic())
        {
            const ndMatrix m = b->GetMatrix();
            dyn->SetMatrixUpdateScene(m);
            dyn->SetVelocity(ndVector::m_zero);
            dyn->SetOmega(ndVector::m_zero);
            dyn->SetAutoSleep(true);
            dyn->SetSleepState(false);
        }
    }
}

void World::recover()
{
    enqueuePhysicsAndWait(
        [](World& w)
        {
            w.recoverInternal();
        });
}

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
    // Runs on Newton's own physics thread, AFTER all substep workers have finished
    // and AFTER DeleteDeadContacts. No worker threads active. No external locks needed.
    //
    // This mirrors ndPhysicsWorld::PostUpdate from the ND4 demo: all transform
    // publication that the main thread will later read MUST happen here, not in
    // interalPostUpdate() (main thread), because by the time interalPostUpdate()
    // runs, the NEXT step's workers may already be writing body state.

    // ── Step 1: free bodies removed in the previous PostUpdate ───────────────────
    // They have now survived one full CalculateContacts pass; any contacts that
    // referenced them have been expired by the LRU check. Safe to release.
    m_deadBodiesFree.clear();

    // ── Step 2: drain new arrivals enqueued by Body::~Body() from any thread ─────
    ndSharedPtr<ndBody> bodyPtr;
    while (m_impl->deadBodies.try_dequeue(bodyPtr))
    {
        ndBodyKinematic* kb = (*bodyPtr)->GetAsBodyKinematic();
        if (kb && kb->GetScene()) // guard: may already be removed during CleanUp
        {
            RemoveBody(*bodyPtr);
        }
        // Keep the shared_ptr alive one more PostUpdate cycle for the LRU window.
        m_deadBodiesFree.push_back(std::move(bodyPtr));
    }
}

void World::setJointRecursiveCollision(const OgreNewt::Body* root, bool enable)
{
    if (!root)
    {
        return;
    }

    enqueuePhysicsAndWait(
        [root, enable](World& w)
        {
            ndBodyKinematic* start = root->getNewtonBody();
            if (!start || start == w.GetSentinelBody())
            {
                return;
            }

            const unsigned int group = enable ? w.m_nextSelfCollisionGroup.fetch_add(1) : 0;
            w.applySelfCollisionGroup(start, group);
        });
}

void World::applySelfCollisionGroup(ndBodyKinematic* start, unsigned int group)
{
    std::unordered_map<ndBodyKinematic*, std::vector<ndBodyKinematic*>> adj;
    adj.reserve(128);

    const auto joints = GetJointList();
    for (auto node = joints.GetFirst(); node; node = node->GetNext())
    {
        auto jSp = node->GetInfo();
        ndJointBilateralConstraint* j = jSp.operator->();
        if (!j)
        {
            continue;
        }

        ndBodyKinematic* b0 = j->GetBody0();
        ndBodyKinematic* b1 = j->GetBody1();
        if (!b0 || !b1)
        {
            continue;
        }
        if (b0 == GetSentinelBody() || b1 == GetSentinelBody())
        {
            continue;
        }

        adj[b0].push_back(b1);
        adj[b1].push_back(b0);
    }

    std::unordered_set<ndBodyKinematic*> visited;
    visited.reserve(adj.size() + 8);

    std::vector<ndBodyKinematic*> stack;
    stack.reserve(adj.size() + 8);
    stack.push_back(start);

    while (!stack.empty())
    {
        ndBodyKinematic* b = stack.back();
        stack.pop_back();

        if (!b || b == GetSentinelBody())
        {
            continue;
        }
        if (!visited.insert(b).second)
        {
            continue;
        }

        ndSharedPtr<ndBodyNotify>& notifyPtr = b->GetNotifyCallback();
        if (notifyPtr)
        {
            if (auto* ogreNotify = dynamic_cast<BodyNotify*>(*notifyPtr))
            {
                if (OgreNewt::Body* body = ogreNotify->GetOgreNewtBody())
                {
                    body->setSelfCollisionGroup(group);
                }
            }
        }

        auto it = adj.find(b);
        if (it != adj.end())
        {
            for (ndBodyKinematic* nb : it->second)
            {
                stack.push_back(nb);
            }
        }
    }
}

void World::flushDeadBodies()
{
    // Synchronously remove all bodies that were enqueued for deferred removal
    // but have not yet been processed by PostUpdate (e.g. because update() was
    // never called after the simulation was stopped in the editor).
    //
    // Must be called while Newton is NOT in the middle of a step (i.e. before
    // ndWorld::Update() fires, or from within PostUpdate / processPhysicsQueue).
    //
    // Without this, zombie bodies from a previous simulation run stay physically
    // inside Newton's scene across a stop/restart cycle. When the new run's first
    // step calls CalculateContacts on the stale contacts those zombies own, Newton
    // accesses their dead internal state (m_sceneNode, notify ptrs) -> crash.
    ndSharedPtr<ndBody> bodyPtr;
    while (m_impl->deadBodies.try_dequeue(bodyPtr))
    {
        ndBodyKinematic* kb = (*bodyPtr)->GetAsBodyKinematic();
        if (kb && kb->GetScene())
        {
            RemoveBody(*bodyPtr);
        }
        // No LRU window needed here: we are removing BEFORE any step runs, so
        // no fresh contacts referencing these bodies exist yet.
    }
    // Also clear any bodies from the previous PostUpdate's LRU hold.
    m_deadBodiesFree.clear();
}

void World::enqueueCommandInternal(ICommand* cmd)
{
    m_impl->q.enqueue(cmd);
}