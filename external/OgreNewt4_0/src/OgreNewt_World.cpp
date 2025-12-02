#include "OgreNewt_Stdafx.h"
#include "OgreNewt_World.h"
#include "OgreNewt_BodyNotify.h"
#include "OgreLogManager.h"
#include "OgreNewt_ContactNotify.h"

using namespace OgreNewt;

World::World(Ogre::Real desiredFps, int maxUpdatesPerFrames, const Ogre::String& name)
    : ndWorld()
    , m_name(name)
    , m_desiredFps(desiredFps)
    , m_updateFPS(desiredFps > 1.0f ? desiredFps : 120.0f)
    , m_fixedTimestep(1.0f / (m_updateFPS > 1.0f ? m_updateFPS : 100.0f))
    , m_timeAccumulator(0.0f)
    , m_maxTicksPerFrames(maxUpdatesPerFrames > 0 ? maxUpdatesPerFrames : 5)
	, m_invFixedTimestep(1.0f / m_fixedTimestep)
    , m_solverMode(6)
    , m_threadsRequested(std::max(1u, std::thread::hardware_concurrency()))
    , m_defaultLinearDamping(0.01f)
    , m_defaultAngularDamping(0.05f)
    , m_gravity(0.0f, -9.81f, 0.0f, 0.0f)
{
    // Bring ND4 defaults to sane values.
    // Threads: you can tweak this externally via setThreadCountExt().
    SetThreadCount(m_threadsRequested);

    // Solver model mapping: we use iterations as a proxy (ND4 changed this surface)
    setSolverModel(m_solverMode);

    // Substeps: keep at 2; we handle fixed stepping in update()
    SetSubSteps(2);

    m_defaultMatID = new OgreNewt::MaterialID(this, 0);

    m_debugger = new Debugger(this);

    OgreNewt::ContactNotify* notify = new OgreNewt::ContactNotify(this);
    SetContactNotify(notify);
}

World::~World()
{
    Sync();       // join worker threads
    ClearCache(); // clears spatial/broadphase caches
}

void World::cleanUp()
{
    Sync();       // join worker threads
    ClearCache(); // clears spatial/broadphase caches

    const ndBodyListView& bodyList = GetBodyList();
    const ndArray<ndBodyKinematic*>& view = bodyList.GetView();

    for (ndInt32 i = ndInt32(view.GetCount()) - 1; i >= 0; --i)
    {
        ndBodyKinematic* const ndBody = view[i];
        if (!ndBody)
            continue;

        // Skip static/kinematic (not dynamic) bodies
        ndBodyDynamic* const dyn = ndBody->GetAsBodyDynamic();
        if (!dyn)
            continue;

        // Reset physical state
        dyn->SetForce(ndVector::m_zero);
        dyn->SetTorque(ndVector::m_zero);
        dyn->SetVelocity(ndVector::m_zero);
        dyn->SetOmega(ndVector::m_zero);

        // Optionally clear sleep flags to make sure state is clean
        // dyn->SetSleepState(true);

        // Sync Ogre side
        if (auto* notify = ndBody->GetNotifyCallback())
        {
            if (auto* ogreNotify = dynamic_cast<OgreNewt::BodyNotify*>(notify))
            {
                if (auto* ogreBody = ogreNotify->GetOgreNewtBody())
                {
                    ogreBody->setForce(Ogre::Vector3::ZERO);
                    ogreBody->setTorque(Ogre::Vector3::ZERO);
                    ogreBody->setVelocity(Ogre::Vector3::ZERO);
                    ogreBody->setOmega(Ogre::Vector3::ZERO);
                }
            }
        }
    }
}

void World::clearCache()
{
    // ndScopeSpinLock lock(m_lock);
    ClearCache();
}

void World::waitForUpdateToFinish()
{
    // Safe point for cross-thread ops
    Sync();
}

void World::setUpdateFPS(Ogre::Real desiredFps, int maxUpdatesPerFrames)
{
    // ndScopeSpinLock lock(m_lock);
    m_updateFPS = std::max<Ogre::Real>(1.0f, desiredFps);
    m_fixedTimestep = 1.0f / m_updateFPS;
    m_invFixedTimestep = 1.0f / m_fixedTimestep;
    m_maxTicksPerFrames = std::max(1, maxUpdatesPerFrames);
}

void World::setSolverModel(int mode)
{
    // ndScopeSpinLock lock(m_lock);
    m_solverMode = mode;

    SetSolverIterations(m_solverMode);
}

void World::setThreadCount(int threads)
{
    // ndScopeSpinLock lock(m_lock);
    m_threadsRequested = std::max(1, threads);
    ndWorld::SetThreadCount(m_threadsRequested);
}

void World::setGravity(const Ogre::Vector3& g)
{
    // ndScopeSpinLock lock(m_lock);
    m_gravity = ndVector(g.x, g.y, g.z, 0.0f);
}

Ogre::Vector3 World::getGravity() const
{
    // No need to lock for a trivial read; keep it safe anyway
    // ndScopeSpinLock lock(m_lock);
    return Ogre::Vector3(m_gravity.m_x, m_gravity.m_y, m_gravity.m_z);
}

int World::getMemoryUsed(void) const
{
    return 0; // TODO: Implement if Newton 4.0 provides memory tracking
}

int World::getBodyCount() const
{
    return GetBodyList().GetCount();
}

int World::getConstraintCount() const
{
    return GetJointList().GetCount();
}

int World::getVersion() const
{
    return 400; // Newton 4.0
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

void World::deferAfterPhysics(std::function<void()> fn)
{
    std::lock_guard<std::mutex> g(m_deferMutex);
    m_deferred.emplace_back(std::move(fn));
}

void World::flushDeferred()
{
    std::vector<std::function<void()>> pending;
    {
        std::lock_guard<std::mutex> g(m_deferMutex);
        pending.swap(m_deferred);
    }

    for (auto& fn : pending)
    {
        if (fn) fn();
    }
}

void World::update(Ogre::Real t_step)
{
    if (m_paused.load())
    {
        if (!m_doSingleStep.exchange(false))
            return;
    }

    // clamp to avoid spiral-of-death
    if (t_step > (m_fixedTimestep * m_maxTicksPerFrames))
        t_step = m_fixedTimestep * m_maxTicksPerFrames;

    m_timeAccumulator += t_step;

    while (m_timeAccumulator >= m_fixedTimestep)
    {
        // 1) Kick the step (ndWorld::Update will first Sync with any *previous* step,
        //    then start this step asynchronously via TickOne()).
        ndWorld::Update(static_cast<ndFloat32>(m_fixedTimestep));  // starts async work

        // 2) Wait for the step we just started to finish -> this makes stepping synchronous.
        ndWorld::Sync();  // <- THIS is the “second Sync” you asked about

        // 3) (Optional) If you kept any deferral queue, you can flush immediately here,
        //    but with a purely synchronous model you won’t need deferral any more.
        // flushDeferred();

        m_timeAccumulator -= m_fixedTimestep;

        if (m_paused.load())
            break;
    }

    const Ogre::Real interp = m_timeAccumulator * m_invFixedTimestep;
    postUpdate(interp);
}

void World::postUpdate(Ogre::Real interp)
{
    const ndBodyListView& bodyList = GetBodyList();
    const ndArray<ndBodyKinematic*>& view = bodyList.GetView();

    for (ndInt32 i = ndInt32(view.GetCount()) - 1; i >= 0; --i)
    {
        ndBodyKinematic* const ndBody = view[i];
        if (!ndBody->GetSleepState())
        {
            if (auto* notify = ndBody->GetNotifyCallback())
            {
                if (auto* ogreNotify = dynamic_cast<BodyNotify*>(notify))
                {
                    if (auto* ogreBody = ogreNotify->GetOgreNewtBody())
                        ogreBody->updateNode(interp);
                }
            }
        }
    }
}

void World::recover()
{
    // ndScopeSpinLock lock(m_lock);

    const ndBodyListView& bodyList = GetBodyList();
    const ndArray<ndBodyKinematic*>& view = bodyList.GetView();

    for (ndInt32 i = ndInt32(view.GetCount()) - 1; i >= 0; --i)
    {
        ndBodyKinematic* const b = view[i];
        if (!b) continue;

        // dynamic only
        if (b->GetInvMass() > ndFloat32(0.0f))
        {


            // mark the body/scene as dirty (invalidates contact cache & aabb)
            //    This is the ND4 way to say "something changed, don’t keep me asleep".
            b->SetMatrixUpdateScene(b->GetMatrix());

            // actually wake it
            b->SetAutoSleep(true);      // keep normal autosleep behavior
            b->SetSleepState(false);    // force out of equilibrium for the next step
        }
    }
}

void World::PreUpdate(ndFloat32 /*timestep*/)
{
    // Called by ND4 just before the internal update work starts.
    // Keep minimal work here; use it if you need to latch external state.
}

void World::OnSubStepPreUpdate(ndFloat32 /*timestep*/)
{
    // Per-substep pre (rarely needed)
}

void World::OnSubStepPostUpdate(ndFloat32 /*timestep*/)
{
    // Per-substep post (rarely needed)
}

void World::PostUpdate(ndFloat32 /*timestep*/)
{
    // ND4 calls this after each Update() completes (while still inside Update()).
    // We do *not* push to Ogre here to avoid render-thread contention.
    // If you want to run something tightly after the step, enqueue via deferAfterPhysics().
    // Example:
    // deferAfterPhysics([this](){ /* safe edits after Sync() in update() */ });
}
