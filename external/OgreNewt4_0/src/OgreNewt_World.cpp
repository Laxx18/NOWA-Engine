#include "OgreNewt_Stdafx.h"
#include "OgreNewt_World.h"
#include "OgreNewt_MaterialID.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_BodyNotify.h"
#include "OgreNewt_ContactNotify.h"

namespace OgreNewt
{
	std::map<Ogre::String, World*> World::worlds = std::map<Ogre::String, World*>();

	World::World(Ogre::Real desiredFps, int maxUpdatesPerFrames, Ogre::String name)
		: solverModel(6),
		desiredFps(desiredFps),
		name(name),
		m_threadCount(1),
		m_debugger(nullptr),
		m_framesCount(0),
		m_physicsFramesCount(0),
		m_fps(0.0f),
		m_timestepAcc(0.0f),
		m_currentListenerTimestep(0.0f),
		m_mainThreadPhysicsTime(0.0f),
		m_mainThreadPhysicsTimeAcc(0.0f)
	{
		m_world = new ndWorld();

		if (!m_world)
		{
			// world not created!
		}

		setUpdateFPS(this->desiredFps, maxUpdatesPerFrames);

		m_defaultMatID = new OgreNewt::MaterialID(this, 0);

		m_leaveCallback = nullptr;

		m_defaultAngularDamping = Ogre::Vector3(0.001f, 0.001f, 0.001f);
		m_defaultLinearDamping = 0.001f;

		m_debugger = new Debugger(this);

		setSolverModel(solverModel);
		setThreadCount(m_threadCount);

		worlds[name] = this;

		OgreNewt::ContactNotify* notify = new OgreNewt::ContactNotify(this);
		m_world->SetContactNotify(notify);
	}

	World::~World()
	{
		const auto& it = World::worlds.find(this->name);
		if (it != World::worlds.cend())
		{
			World::worlds.erase(it);
		}

		cleanUp();

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

		if (m_world)
		{
			delete m_world;
			m_world = nullptr;
		}
	}

	World* World::get(Ogre::String name)
	{
		std::map<Ogre::String, World*>::const_iterator search = worlds.find(name);

		if (search != worlds.end())
		{
			return search->second;
		}

		return nullptr;
	}

	void World::setUpdateFPS(Ogre::Real desiredFps, int maxUpdatesPerFrames)
	{
		m_world->SetSubSteps(maxUpdatesPerFrames);
		if (maxUpdatesPerFrames < 1)
		{
			maxUpdatesPerFrames = 1;
		}

		if (maxUpdatesPerFrames > 10)
		{
			maxUpdatesPerFrames = 10;
		}

		m_maxTicksPerFrames = maxUpdatesPerFrames;
		m_updateFPS = desiredFps;
		this->desiredFps = desiredFps;

		m_timestep = 1.0f / desiredFps;

		if (m_timestep > 1.0f / 10.0f)
		{
			m_maxTicksPerFrames += (int)ceilf(m_timestep / (1.0f / 10.0f));
			m_timestep = 1.0f / 10.0f;
			m_updateFPS = 10.0f;
		}

		if (m_timestep < 1.0f / 1000.0f)
		{
			m_timestep = 1.0f / 1000.0f;
			m_updateFPS = 1000.0f;
		}

		m_invTimestep = 1.0f / m_timestep;
		m_timeAcumulator = m_timestep;
	}

	void World::setThreadCount(int threads)
	{
		m_threadCount = threads;
		m_world->SetThreadCount(threads);
	}

	void World::cleanUp(void)
	{
		m_world->Sync();
		// Causes crash
		// m_world->CleanUp();
		invalidateCache();
		m_timeAcumulator = 0.0f;
	}

	void World::recover(void)
	{
		if (!m_world)
		{
			return;
		}

		// No CleanUp / ClearCache here — just wake & invalidate per-body.
		const ndBodyListView& bodyList = m_world->GetBodyList();
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

	int World::update(Ogre::Real t_step)
	{
		int realUpdates = 0;

		if (t_step > (m_timestep * m_maxTicksPerFrames))
			t_step = m_timestep * m_maxTicksPerFrames;

		m_timeAcumulator += t_step;

		while (m_timeAcumulator >= m_timestep)
		{
			m_world->Update(m_timestep);
			m_timeAcumulator -= m_timestep;
			realUpdates++;
		}

		const Ogre::Real interp = m_timeAcumulator * m_invTimestep;
		postUpdate(interp);
		return realUpdates;
	}

	void World::postUpdate(Ogre::Real interp)
	{
		const ndBodyListView& bodyList = m_world->GetBodyList();
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

	void World::invalidateCache()
	{
		m_world->ClearCache();
	}

	// OgreNewt_World.cpp
	void World::destroyAllBodies()
	{
		ndWorld* const w = m_world;
		if (!w)
			return;

		// Ensure nobody is stepping
		w->Sync();

		// Remove all joints first (safer if you have constraints between bodies)
		{
			const ndJointList& jlist = w->GetJointList();
			for (ndJointList::ndNode* node = jlist.GetFirst(); node; )
			{
				ndJointList::ndNode* next = node->GetNext();
				auto j = node->GetInfo();
				w->RemoveJoint(j.operator->());
				node = next;
			}
		}

		// Remove all bodies
		{
			const ndBodyListView& blist = w->GetBodyList();
			const ndArray<ndBodyKinematic*>& view = blist.GetView();
			// removing by pointer is safe; removing backwards avoids reindex costs
			for (ndInt32 i = ndInt32(view.GetCount()) - 1; i >= 0; --i)
			{
				ndBodyKinematic* const b = view[i];
				w->RemoveBody(b->GetAsBody());
			}
		}
	}


	void World::setSolverModel(int model)
	{
		this->solverModel = model;
		m_world->SetSolverIterations(model);
	}

	int World::getMemoryUsed(void) const
	{
		return 0; // TODO: Implement if Newton 4.0 provides memory tracking
	}

	int World::getBodyCount() const
	{
		return m_world->GetBodyList().GetCount();
	}

	int World::getConstraintCount() const
	{
		return m_world->GetJointList().GetCount();
	}

	void World::setMultithreadSolverOnSingleIsland(int mode)
	{
		// Implementation depends on Newton 4.0 API
	}

	int World::getMultithreadSolverOnSingleIsland() const
	{
		return 0;
	}

	void World::waitForUpdateToFinish(void)
	{
		m_world->Sync();
	}

	void World::criticalSectionLock(int threadIndex) const
	{
		m_world->Sync();
	}

	void World::criticalSectionUnlock() const
	{
	}

	int World::getThreadCount() const
	{
		return m_world->GetThreadCount();
	}

	int World::getVersion() const
	{
		return 400; // Newton 4.0
	}
}
