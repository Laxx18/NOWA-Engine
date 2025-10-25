/*
	OgreNewt Library 4.0

	Ogre implementation of Newton Game Dynamics SDK 4.0
*/
#ifndef _INCLUDE_OGRENEWT_WORLD
#define _INCLUDE_OGRENEWT_WORLD

#include "OgreNewt_Prerequisites.h"
#include "OgreNewt_Debugger.h"
#include "OgreNewt_MaterialPair.h"

#include <map>

namespace OgreNewt
{
	class World;

	//! represents a physics world for Newton Dynamics 4.0
	class _OgreNewtExport World
	{
	public:
		enum SolverModelMode
		{
			SM_EXACT = 0,
			SM_FASTEST = 1,
			SM_MEDIUM = 4,
			SM_SLOW = 8,
			SM_RIDICULUS = 16
		};

		typedef OgreNewt::function<void(OgreNewt::Body*, int threadIndex)> LeaveWorldCallback;

	public:
		World(Ogre::Real desiredFps = 100.0f, int maxUpdatesPerFrames = 5, Ogre::String name = "main");
		~World();

		static World* get(Ogre::String name = "main");

		int update(Ogre::Real t_step);

		void invalidateCache();

		void setUpdateFPS(Ogre::Real desiredFps, int maxUpdatesPerFrames);

		Ogre::Real getUpdateFPS() const { return m_updateFPS; }

		void setDefaultLinearDamping(Ogre::Real defaultLinearDamping) { m_defaultLinearDamping = defaultLinearDamping; }
		Ogre::Real getDefaultLinearDamping() const { return m_defaultLinearDamping; }

		void setDefaultAngularDamping(Ogre::Vector3 defaultAngularDamping) { m_defaultAngularDamping = defaultAngularDamping; }
		Ogre::Vector3 getDefaultAngularDamping() const { return m_defaultAngularDamping; }

		ndWorld* getNewtonWorld() const { return m_world; }

		const MaterialID* getDefaultMaterialID() const { return m_defaultMatID; }

		void destroyAllBodies();

		void setSolverModel(int model);
		int getSolverModel(void) const { return this->solverModel; }

		int getMemoryUsed(void) const;

		int getBodyCount() const;
		int getConstraintCount() const;

		void setMultithreadSolverOnSingleIsland(int mode);
		int getMultithreadSolverOnSingleIsland() const;

		void setThreadCount(int threads);
		int getThreadCount() const;

		void waitForUpdateToFinish(void);

		void criticalSectionLock(int threadIndex = 0) const;
		void criticalSectionUnlock() const;

		void cleanUp(void);

		int getVersion() const;

		Ogre::Real getDesiredFps(void) const { return this->desiredFps; }

		Body* getFirstBody() const;

		Debugger& getDebugger() const { return *m_debugger; }

		std::mutex m_ogreMutex;

		void registerMaterialPair(MaterialPair* pair);

		void unregisterMaterialPair(MaterialPair* pair);

		MaterialPair* findMaterialPair(int id0, int id1) const;

	protected:
		int m_maxTicksPerFrames;
		Ogre::Real m_timestep;
		Ogre::Real m_invTimestep;
		Ogre::Real m_timeAcumulator;
		Ogre::Real m_updateFPS;

		Ogre::Vector3 m_defaultAngularDamping;
		Ogre::Real m_defaultLinearDamping;

		ndWorld* m_world;
		MaterialID* m_defaultMatID;

		LeaveWorldCallback m_leaveCallback;

		static std::map<Ogre::String, World*> worlds;

		mutable Debugger* m_debugger;

	private:
		int solverModel;
		Ogre::Real desiredFps;
		int m_threadCount;

		int m_framesCount;
		int m_physicsFramesCount;
		ndFloat32 m_fps;
		ndFloat32 m_timestepAcc;
		ndFloat32 m_currentListenerTimestep;
		ndFloat32 m_mainThreadPhysicsTime;
		ndFloat32 m_mainThreadPhysicsTimeAcc;
		__int64 m_microsecunds;
		Ogre::String name;

		std::map<std::pair<int, int>, MaterialPair*> m_materialPairs;
	};
}

#endif
