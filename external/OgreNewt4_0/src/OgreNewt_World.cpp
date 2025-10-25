#include "OgreNewt_World.h"
#include "OgreNewt_MaterialID.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_BodyNotify.h"
#include "OgreNewt_ContactNotify.h"

namespace OgreNewt
{
	std::map<Ogre::String, World*> World::worlds = std::map<Ogre::String, World*>();

	World::World(Ogre::Real desiredFps, int maxUpdatesPerFrames, Ogre::String name)
		: solverModel(1),
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
	}

	int World::update(Ogre::Real t_step)
	{
		int realUpdates = 0;

		if (t_step > (m_timestep * m_maxTicksPerFrames))
		{
			t_step = m_timestep * m_maxTicksPerFrames;
		}

		m_timeAcumulator += t_step;

		while (m_timeAcumulator >= m_timestep)
		{
			m_world->Update(m_timestep);

			m_timeAcumulator -= m_timestep;
			realUpdates++;
		}

		Ogre::Real param = m_timeAcumulator * m_invTimestep;

		for (Body* body = getFirstBody(); body; body = body->getNext())
		{
			body->updateNode(param);
		}

		return realUpdates;
	}

	Body* World::getFirstBody() const
	{
		const auto& bodyList = m_world->GetBodyList();
		if (bodyList.GetCount() > 0)
		{
			ndBodyList::ndNode* node = bodyList.GetFirst();
			if (node)
			{
				auto ndBody = node->GetInfo();
				if (ndBody)
				{
					ndBodyNotify* notify = ndBody->GetNotifyCallback();
					if (notify)
					{
						BodyNotify* ogreNotify = static_cast<BodyNotify*>(notify);
						return ogreNotify->GetOgreNewtBody();
					}
				}
			}
		}
		return nullptr;
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
		m_world->Sync();
	}

	void World::destroyAllBodies()
	{
		const ndBodyListView& bodyList = m_world->GetBodyList();
		if (bodyList.GetCount() > 0)
		{
			auto ndBody = bodyList.GetFirst()->GetInfo();
			if (ndBody)
			{
				ndBody->GetAsBody();
				m_world->RemoveBody(*ndBody);
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
