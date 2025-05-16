#include "OgreNewt_Stdafx.h"
#include "OgreNewt_World.h"
#include "OgreNewt_MaterialID.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_BodyInAABBIterator.h"
#include "OgreNewt_CollisionPrimitives.h"

#pragma warning(disable:4355)

namespace OgreNewt
{
	/*WorldTriggerManager::WorldTriggerManager(World* const world)
		: dCustomTriggerManager(world->getNewtonWorld())
	{
	}


	void WorldTriggerManager::TriggerCallback::setTriggerEnterCallback(WorldTriggerEnterCallback callback)
	{
		m_EnterCallback = callback;
	}

	void WorldTriggerManager::TriggerCallback::setTriggerInsideCallback(WorldTriggerInsideCallback callback)
	{
		m_InsideCallback = callback;
	}

	void WorldTriggerManager::TriggerCallback::setTriggerExitCallback(WorldTriggerExitCallback callback)
	{
		m_ExitCallback = callback;
	}*/

	std::map<Ogre::String, World*> World::worlds = std::map<Ogre::String, World*>();

	// Constructor
	World::World(Ogre::Real desiredFps, int maxUpdatesPerFrames, Ogre::String name)
		: m_bodyInAABBIterator(this),
		solverModel(1),
		desiredFps(desiredFps),
		name(name),
		m_threadCount(1),
		m_triggerManager(nullptr),
		m_debugger(nullptr)
		, m_framesCount(0)
		, m_physicsFramesCount(0)
		, m_fps(0.0f)
		, m_timestepAcc(0.0f)
		, m_currentListenerTimestep(0.0f)
		, m_mainThreadPhysicsTime(0.0f)
		, m_mainThreadPhysicsTimeAcc(0.0f)
	{
#ifndef WIN32
		pthread_mutex_init(&m_ogreMutex, 0);
#endif

		m_world = NewtonCreate();

		if (!m_world)
		{
			// world not created!
		}

		setUpdateFPS(this->desiredFps, maxUpdatesPerFrames);

		NewtonWorldSetUserData(m_world, this);


		// create the default ID.
		m_defaultMatID = new OgreNewt::MaterialID(this, NewtonMaterialGetDefaultGroupID(m_world));

		m_leaveCallback = nullptr;

		m_defaultAngularDamping = Ogre::Vector3(0.001f, 0.001f, 0.001f);
		m_defaultLinearDamping = 0.001f;

		m_debugger = new Debugger(this);
		// m_triggerManager = new WorldTriggerManager(this);
		// set the default solve mode to be iterative the fastest
		setSolverModel(solverModel);

		// set one tread by default
		setThreadCount(m_threadCount);

		NewtonSelectBroadphaseAlgorithm (m_world, 0);

		NewtonMaterialSetDefaultSoftness(m_world, m_defaultMatID->getID(), m_defaultMatID->getID(), 0.1f);
		NewtonMaterialSetDefaultElasticity(m_world, m_defaultMatID->getID(), m_defaultMatID->getID(), 0.0f);
		NewtonMaterialSetDefaultCollidable(m_world, m_defaultMatID->getID(), m_defaultMatID->getID(), 1);
		NewtonMaterialSetDefaultFriction(m_world, m_defaultMatID->getID(), m_defaultMatID->getID(), 0.9f, 0.5f);

		// store the world by name in a static map
		worlds[name] = this;
	}

	// Destructor
	World::~World()
	{
		const auto& it = World::worlds.find(this->name);
		if (it !=  World::worlds.cend())
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
			NewtonDestroy(m_world);
			m_world = nullptr;
		}

		// Only check assert, if all newton worlds are destroyed!
		if (true == World::worlds.empty())
		{
			dAssert(NewtonGetMemoryUsed() == 0);
		}
	}

	/*dCustomTriggerController* World::CreateTriggerController(Body* body)
	{
		dFloat m[16];
		OgreNewt::Converters::QuatPosToMatrix(body->getOrientation(), body->getPosition(), m);

		if (m_triggerManager)
			return m_triggerManager->CreateController(&m[0], body->getNewtonCollision());

		return 0;
	}

	void World::DestroyTriggerController(dCustomTriggerController* const controller)
	{
		m_triggerManager->DestroyController(controller);
	}*/

	void World::setPreListener(void* const listenerUserData, NewtonWorldUpdateListenerCallback update)
	{
		NewtonWorldListenerSetPreUpdateCallback(m_world, listenerUserData, update);
	}

	void World::setPostListener(void* const listenerUserData, NewtonWorldUpdateListenerCallback update)
	{
		NewtonWorldListenerSetPostUpdateCallback(m_world, listenerUserData, update);
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

// Attention with sub steps, do not use it, because else each force is half strong and movement becomes jittery
		// if (desiredFps <= 30.0f)
		// 	NewtonSetNumberOfSubsteps(m_world, 2);

		if (m_timestep > 1.0f / 10.0f)
		{
			// recalculate the iteration count to met the desire fps 
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
		NewtonSetThreadsCount(m_world, threads);
	}

	void World::cleanUp(void)
	{
		// if we are run asynchronous we need make sure no update in on flight.
		NewtonWaitForUpdateToFinish(m_world);
		// clean up all caches the engine have saved
		NewtonInvalidateCache(m_world);
	}

	// update
	int World::update(Ogre::Real t_step)
	{
		int realUpdates = 0;

		// Clamps the step if necessary
		if (t_step > (m_timestep * m_maxTicksPerFrames))
		{
			t_step = m_timestep * m_maxTicksPerFrames;
		}

		// Advances the accumulator;
		m_timeAcumulator += t_step;

		while (m_timeAcumulator >= m_timestep)
		{
			if (m_threadCount > 1)
			{
				NewtonUpdateAsync(m_world, m_timestep);
				NewtonWaitForUpdateToFinish(m_world);
			}
			else
			{
				NewtonUpdate(m_world, m_timestep);
			}

			m_timeAcumulator -= m_timestep;

			realUpdates++;
		}
		// NewtonWaitForUpdateToFinish(m_world);

		// Changing something here is useless!
		Ogre::Real param = m_timeAcumulator * m_invTimestep;
		// param = 1.0f;
		for (Body* body = getFirstBody(); body; body = body->getNext())
		{
			body->updateNode(param/*1.0f*/);
		}

		return realUpdates;
	}

	Body* World::getFirstBody() const
	{
		NewtonBody* body = NewtonWorldGetFirstBody(m_world);
		if (body)
			return (Body*)NewtonBodyGetUserData(body);

		return nullptr;
	}

	CustomTriggerManager* World::getTriggerManager(void)
	{
		if (nullptr == m_triggerManager)
			m_triggerManager = new CustomTriggerManager(this);

		return m_triggerManager;
	}

	void World::AddSceneCollision(CollisionPtr collision, int id)
	{
		if (nullptr == m_sceneCollisionPtr)
		{
			m_sceneCollisionPtr = CollisionPtr(new CollisionPrimitives::SceneCollision(this));
		}
		std::dynamic_pointer_cast<CollisionPrimitives::SceneCollision>(m_sceneCollisionPtr)->AddCollision(collision, id);
	}

	void World::RemoveSceneCollision(CollisionPtr collision)
	{
		if (nullptr != m_sceneCollisionPtr)
		{
			std::dynamic_pointer_cast<CollisionPrimitives::SceneCollision>(m_sceneCollisionPtr)->RemoveCollision(collision);
		}
	}

	void World::RemoveSceneCollision(NewtonCollision* collision)
	{
		if (nullptr != m_sceneCollisionPtr)
		{
			std::dynamic_pointer_cast<CollisionPrimitives::SceneCollision>(m_sceneCollisionPtr)->RemoveCollision(collision);
		}
	}
}

