#include "NOWAPrecompiled.h"
#include "OgreNewtModule.h"

namespace NOWA
{
	OgreNewtModule::OgreNewtModule(const Ogre::String& appStateName)
		: appStateName(appStateName),
		ogreNewt(nullptr), // at this time, only one ogrenewt instance is supported
		showText(true),
		globalGravity(Ogre::Vector3(0.0f, -19.8f, 0.0f))
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[OgreNewtModule] Module created");
	}

	OgreNewtModule::~OgreNewtModule()
	{

	}

	OgreNewt::World* OgreNewtModule::createPhysics(const Ogre::String& name, int solverModel, int broadPhaseAlgorithm, int multithreadSolverOnSingleIsland,
		int threadCount, Ogre::Real updateRate, Ogre::Real defaultLinearDamping, Ogre::Vector3 defaultAngularDamping)
	{
		if (nullptr != this->ogreNewt)
		{
			this->destroyContent();
		}

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[OgreNewtModule] Initializing OgreNewt");

		this->ogreNewt = new OgreNewt::World(updateRate, 5, name);
		this->ogreNewt->setSolverModel(solverModel);
		this->ogreNewt->setBroadPhaseAlgorithm(broadPhaseAlgorithm);
		this->ogreNewt->setDefaultLinearDamping(defaultLinearDamping);
		this->ogreNewt->setDefaultAngularDamping(defaultAngularDamping);
		this->ogreNewt->setMultithreadSolverOnSingleIsland(multithreadSolverOnSingleIsland);
		//Ogrenewt mit 2 Thread einstellen: nicht sicher ob OgreNewt diese Funktionalitaet von Newtondynamics unterstuetzt!
#ifdef _WIN32
		SYSTEM_INFO sysinfo;
		GetSystemInfo(&sysinfo);

		DWORD numCPU = sysinfo.dwNumberOfProcessors;
		if (numCPU >= 2)
		{
			numCPU /= 2;
			if (threadCount > numCPU)
				threadCount = numCPU;
		}
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[OgreNewtModule] Using: " + Ogre::StringConverter::toString(threadCount) + " cores for physics simulation");
// #ifndef _DEBUG
		this->ogreNewt->setThreadCount(threadCount);
//#else
//		this->ogreNewt->setThreadCount(1);
//#endif
#else
		int numCPU = sysconf(_SC_NPROCESSORS_ONLN);
		if (numCPU >= 2)
		{
			numCPU /= 2;
		}
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[OgreNewtModule] Using: " + Ogre::StringConverter::toString(numCPU) + " cores for physics simulation");
		this->ogreNewt->setThreadCount(numCPU);
#endif

		return this->ogreNewt;
	}

	OgreNewt::World* OgreNewtModule::createPerformantPhysics(const Ogre::String& name, Ogre::Real updateRate)
	{
		if (nullptr != this->ogreNewt)
		{
			this->destroyContent();
		}

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[OgreNewtModule] Initializing OgreNewt (performant)");
		this->ogreNewt = new OgreNewt::World(updateRate, 5, name);
		// this->ogreNewt->setWorldSize(leftCorner, rightCorner);
		//this->ogreNewt->setWorldSize(Vector3(-500,-500,-500), Vector3(500,500,500));
		// OgreNewt::World::SM_FASTEST and OgreNewt::World::FM_ADAPTIVE are not precise but sufficient for Games see NewtonWiki
		//ogreNewt->setSolverModel(OgreNewt::World::SM_EXACT);
		//ogreNewt->setFrictionModel(OgreNewt::World::FM_EXACT);
		this->ogreNewt->setSolverModel(OgreNewt::World::SM_MEDIUM);
		// this->ogreNewt->setFrictionModel(OgreNewt::World::FM_ADAPTIVE);
		//PF_COMMON_OPTIMIZED --> nachschauen wenn es Probleme gibt
		//ogreNewt->setPlatformArchitecture(OgreNewt::World::PF_COMMON_OPTIMIZED);
		// this->ogreNewt->setPlatformArchitecture(OgreNewt::World::PF_BEST_POSSIBLE + 1);
		this->ogreNewt->setDefaultLinearDamping(0.1f);
		this->ogreNewt->setDefaultAngularDamping(Ogre::Vector3(0.1f, 0.1f, 0.1f));
		// this->ogreNewt->setMultithreadSolverOnSingleIsland(1);
#ifdef _WIN32
		SYSTEM_INFO sysinfo;
		GetSystemInfo(&sysinfo);

		DWORD numCPU = sysinfo.dwNumberOfProcessors;
		if (numCPU >= 2)
		{
			numCPU /= 2;
		}
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[OgreNewtModule] Using: " + Ogre::StringConverter::toString(numCPU) + " cores for physics simulation");
		this->ogreNewt->setThreadCount(numCPU);
#else
		int numCPU = sysconf(_SC_NPROCESSORS_ONLN);
		if (numCPU >= 2)
		{
			numCPU /= 2;
		}
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[OgreNewtModule] Using: " + Ogre::StringConverter::toString(numCPU) + " cores for physics simulation");
		this->ogreNewt->setThreadCount(numCPU);
#endif

		return this->ogreNewt;
	}

	OgreNewt::World* OgreNewtModule::createQualityPhysics(const Ogre::String& name, Ogre::Real updateRate)
	{
		if (nullptr != this->ogreNewt)
		{
			this->destroyContent();
		}

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[OgreNewtModule] Initializing OgreNewt (quality)");

		this->ogreNewt = new OgreNewt::World(updateRate, 5, name);
		// this->ogreNewt->setWorldSize(leftCorner, rightCorner);
		//this->ogreNewt->setWorldSize(Vector3(-500,-500,-500), Vector3(500,500,500));
		this->ogreNewt->setSolverModel(OgreNewt::World::SM_EXACT);
		//ogreNewt->setFrictionModel(OgreNewt::World::FM_EXACT);
		//ogreNewt->setSolverModel(OgreNewt::World::SM_FASTEST);
		// this->ogreNewt->setFrictionModel(OgreNewt::World::FM_EXACT);
		//PF_COMMON_OPTIMIZED --> nachschauen wenn es Probleme gibt
		//ogreNewt->setPlatformArchitecture(OgreNewt::World::PF_COMMON_OPTIMIZED);
		// this->ogreNewt->setPlatformArchitecture(OgreNewt::World::PF_BEST_POSSIBLE + 1);
		this->ogreNewt->setDefaultLinearDamping(0.1f);
		this->ogreNewt->setDefaultAngularDamping(Ogre::Vector3(0.1f, 0.1f, 0.1f));
		// this->ogreNewt->setMultithreadSolverOnSingleIsland(1);

		//NewtonWaitForUpdateToFinish (m_world);
		//SetAutoSleepMode (m_world, !m_autoSleepState);
		//NewtonSetSolverModel (m_world, m_solverModes[m_solverModeIndex]);
		//NewtonSetSolverConvergenceQuality (m_world, m_solverModeQuality ? 1 : 0);
		//NewtonSetMultiThreadSolverOnSingleIsland (m_world, m_useParallelSolver ? 1 : 0);	
		//NewtonSelectBroadphaseAlgorithm (m_world, m_broadPhaseType);

#ifdef _WIN32
		SYSTEM_INFO sysinfo;
		GetSystemInfo(&sysinfo);
		
		DWORD numCPU = sysinfo.dwNumberOfProcessors;
		if (numCPU >= 2)
		{
			numCPU /= 2;
		}
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[OgreNewtModule] Using: " + Ogre::StringConverter::toString(numCPU) + " cores for simulation");
		this->ogreNewt->setThreadCount(1);
#else
		int numCPU = sysconf(_SC_NPROCESSORS_ONLN);
		if (numCPU >= 2)
		{
			numCPU /= 2;
		}
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[OgreNewtModule] Using: " + Ogre::StringConverter::toString(numCPU) + " cores for simulation");
		this->ogreNewt->setThreadCount(numCPU);
#endif

		return ogreNewt;
	}

	OgreNewt::World* OgreNewtModule::getOgreNewt(void) const
	{
		return this->ogreNewt;
	}

	void OgreNewtModule::enableOgreNewtCollisionLines(Ogre::SceneManager* sceneManager, bool showText)
	{
		this->showText = showText;
		if (!this->ogreNewt)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[OgreNewtModule] Collision lines cannot be enabled, because OgreNewt has not been initialized.");
			return;
		}
		OgreNewt::Debugger& debug = this->ogreNewt->getDebugger();
		debug.init(sceneManager, this->showText);
	}

	void OgreNewtModule::destroyContent(void)
	{
		if (this->ogreNewt)
		{
			delete this->ogreNewt;
			this->ogreNewt = 0;
		}
	}

	void OgreNewtModule::showOgreNewtCollisionLines(bool enabled)
	{
		if (this->ogreNewt)
		{
			OgreNewt::Debugger& debug = this->ogreNewt->getDebugger();
			if (enabled)
			{
				debug.showDebugInformation();
				debug.startRaycastRecording();
				debug.clearRaycastsRecorded();
			}
			else
			{
				debug.hideDebugInformation();
				debug.clearRaycastsRecorded();
				debug.stopRaycastRecording();
			}
		}
	}

	void OgreNewtModule::setMaterialIdForDebugger(const OgreNewt::MaterialID* material, const Ogre::ColourValue& colour)
	{
		if (this->ogreNewt)
		{
			OgreNewt::Debugger& debug = this->ogreNewt->getDebugger();

			Ogre::ColourValue tempColour = colour;
			if (Ogre::ColourValue::White == tempColour)
			{
				// Generate a small random offset for each RGB component
				Ogre::Real randOffset = 0.5f; // You can adjust this value to control how much the color changes
				tempColour.r = Ogre::Math::Clamp(tempColour.r + ((rand() % 100) / 100.0f - 0.5f) * randOffset, 0.0f, 1.0f);
				tempColour.g = Ogre::Math::Clamp(tempColour.g + ((rand() % 100) / 100.0f - 0.5f) * randOffset, 0.0f, 1.0f);
				tempColour.b = Ogre::Math::Clamp(tempColour.b + ((rand() % 100) / 100.0f - 0.5f) * randOffset, 0.0f, 1.0f);
			}

			debug.setMaterialColor(material, tempColour);
		}
	}

	void OgreNewtModule::update(Ogre::Real dt)
	{
		if (nullptr != this->ogreNewt)
			this->ogreNewt->update(dt);
	}

	void OgreNewtModule::setGlobalGravity(const Ogre::Vector3& globalGravity)
	{
		this->globalGravity = globalGravity;
	}

	Ogre::Vector3 OgreNewtModule::getGlobalGravity(void) const
	{
		return this->globalGravity;
	}

} // namespace end