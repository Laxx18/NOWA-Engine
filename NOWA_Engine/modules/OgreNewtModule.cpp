#include "NOWAPrecompiled.h"
#include "OgreNewtModule.h"
#include "GraphicsModule.h"

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
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[OgreNewtModule] Using: " + Ogre::StringConverter::toString(threadCount) + " cores for physics simulation and updaterate: " + Ogre::StringConverter::toString(updateRate));
		this->ogreNewt->setThreadCount(threadCount);

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
		if (nullptr != this->ogreNewt)
		{
			delete ogreNewt;
			this->ogreNewt = nullptr;
		}
	}

	void OgreNewtModule::showOgreNewtCollisionLines(bool enabled)
	{
		if (this->ogreNewt)
		{
			ENQUEUE_RENDER_COMMAND_MULTI("OgreNewtModule::showOgreNewtCollisionLines", _1(enabled),
			{
				OgreNewt::Debugger & debug = this->ogreNewt->getDebugger();
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
			});
		}
	}

	void OgreNewtModule::setMaterialIdForDebugger(const OgreNewt::MaterialID* material, const Ogre::ColourValue& colour)
	{
		if (this->ogreNewt)
		{
			OgreNewt::Debugger& debug = this->ogreNewt->getDebugger();

			// Predefined color palette with 31 distinct high-contrast colors
			static const std::vector<Ogre::ColourValue> colorPalette = {
				Ogre::ColourValue(1.0f, 0.0f, 0.0f), // 1 - Red
				Ogre::ColourValue(0.0f, 1.0f, 0.0f), // 2 - Green
				Ogre::ColourValue(0.0f, 0.0f, 1.0f), // 3 - Blue
				Ogre::ColourValue(1.0f, 1.0f, 0.0f), // 4 - Yellow
				Ogre::ColourValue(1.0f, 0.5f, 0.0f), // 5 - Orange
				Ogre::ColourValue(0.5f, 0.0f, 0.5f), // 6 - Violet
				Ogre::ColourValue(0.0f, 1.0f, 1.0f), // 7 - Cyan
				Ogre::ColourValue(1.0f, 0.0f, 1.0f), // 8 - Magenta
				Ogre::ColourValue(0.5f, 0.5f, 0.5f), // 9 - Grey
				Ogre::ColourValue(0.3f, 0.3f, 0.3f), // 10 - Dark Grey
				Ogre::ColourValue(0.8f, 0.3f, 0.1f), // 11 - Rust
				Ogre::ColourValue(0.2f, 0.7f, 0.3f), // 12 - Leaf Green
				Ogre::ColourValue(0.1f, 0.3f, 0.8f), // 13 - Deep Blue
				Ogre::ColourValue(0.9f, 0.2f, 0.5f), // 14 - Pink
				Ogre::ColourValue(0.6f, 0.4f, 0.2f), // 15 - Brown
				Ogre::ColourValue(0.7f, 0.8f, 0.3f), // 16 - Lime
				Ogre::ColourValue(0.3f, 0.6f, 0.9f), // 17 - Sky Blue
				Ogre::ColourValue(0.6f, 0.2f, 0.8f), // 18 - Purple
				Ogre::ColourValue(0.2f, 0.9f, 0.8f), // 19 - Turquoise
				Ogre::ColourValue(0.9f, 0.8f, 0.2f), // 20 - Gold
				Ogre::ColourValue(0.4f, 0.7f, 0.4f), // 21 - Forest Green
				Ogre::ColourValue(0.7f, 0.2f, 0.4f), // 22 - Raspberry
				Ogre::ColourValue(0.2f, 0.2f, 0.7f), // 23 - Navy
				Ogre::ColourValue(0.7f, 0.7f, 0.9f), // 24 - Light Lavender
				Ogre::ColourValue(0.9f, 0.6f, 0.2f), // 25 - Amber
				Ogre::ColourValue(0.6f, 0.9f, 0.2f), // 26 - Spring Green
				Ogre::ColourValue(0.2f, 0.9f, 0.6f), // 27 - Seafoam
				Ogre::ColourValue(0.5f, 0.1f, 0.7f), // 28 - Indigo
				Ogre::ColourValue(0.3f, 0.8f, 0.5f), // 29 - Mint
				Ogre::ColourValue(0.9f, 0.3f, 0.7f), // 30 - Rose
				Ogre::ColourValue(0.7f, 0.9f, 0.7f)  // 31 - Pale Green
			};

			// Static index to ensure unique color assignment
			static size_t colorIndex = 0;

			Ogre::ColourValue tempColour = colorPalette[colorIndex];

			// Cycle through colors
			colorIndex = (colorIndex + 1) % colorPalette.size();

			ENQUEUE_RENDER_COMMAND_MULTI("OgreNewtModule::setMaterialIdForDebugger", _3(&debug, material, tempColour),
			{
				debug.setMaterialColor(material, tempColour);
			});
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

	void OgreNewtModule::registerRenderCallbackForBody(OgreNewt::Body* body)
	{
		if (nullptr == body)
		{
			return;
		}

#if 1
		body->setRenderUpdateCallback([](Ogre::SceneNode* node, const Ogre::Vector3& pos, const Ogre::Quaternion& rot, bool updateRot, bool updateStatic)
		{
			if (nullptr == node)
			{
				return;
			}

			Ogre::Node* parent = node->getParent();

			if (nullptr == parent)
			{
				return;
			}

			if (false == node->isStatic())
			{
				NOWA::GraphicsModule::getInstance()->updateNodePosition(node, (parent->_getDerivedOrientation().Inverse() * (pos - parent->_getDerivedPosition())) / parent->_getDerivedScale());
			}
			if (true == updateRot)
			{
				NOWA::GraphicsModule::getInstance()->updateNodeOrientation(node, parent->_getDerivedOrientation().Inverse() * rot);
			}
			else if (true == updateStatic)
			{
				NOWA::GraphicsModule::getInstance()->updateNodePosition(node, (parent->_getDerivedOrientationUpdated().Inverse() * (pos - parent->_getDerivedPositionUpdated())) / parent->_getDerivedScaleUpdated());
				NOWA::GraphicsModule::getInstance()->updateNodeOrientation(node, parent->_getDerivedOrientationUpdated().Inverse() * rot);
			}
		});
#else

		body->setRenderUpdateCallback([](Ogre::SceneNode* node, const Ogre::Vector3& pos, const Ogre::Quaternion& rot, bool updateRot, bool updateStatic)
			{
				if (nullptr == node)
				{
					return;
				}

				Ogre::Node* parent = node->getParent();

				if (nullptr == parent)
				{
					return;
				}

				NOWA::GraphicsModule::getInstance()->enqueue([=]() {

					if (false == node->isStatic())
					{
						NOWA::GraphicsModule::getInstance()->updateNodePosition(node, (parent->_getDerivedOrientation().Inverse() * (pos - parent->_getDerivedPosition())) / parent->_getDerivedScale());
					}
					if (true == updateRot)
					{
						NOWA::GraphicsModule::getInstance()->updateNodeOrientation(node, parent->_getDerivedOrientation().Inverse() * rot);
					}
					else if (true == updateStatic)
					{
						NOWA::GraphicsModule::getInstance()->updateNodePosition(node, (parent->_getDerivedOrientationUpdated().Inverse() * (pos - parent->_getDerivedPositionUpdated())) / parent->_getDerivedScaleUpdated());
						NOWA::GraphicsModule::getInstance()->updateNodeOrientation(node, parent->_getDerivedOrientationUpdated().Inverse() * rot);
					}
				}, "body->setRenderUpdateCallback");
			});
#endif
	}
} // namespace end