#include "NOWAPrecompiled.h"
#include "GpuParticlesModule.h"

#include "GpuParticles/Hlms/HlmsParticle.h"
#include "WorkspaceModule.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	GpuParticlesModule::GpuParticlesModule(const Ogre::String& appStateName)
		: appStateName(appStateName),
		sceneManager(nullptr),
		gpuParticleSystemWorld(nullptr),
		gpuParticleSystemResourceManager(nullptr),
		gpuParticleSystemJsonManager(nullptr)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[GpuParticlesModule] Module created");
	}

	GpuParticlesModule::~GpuParticlesModule()
	{

	}

	void GpuParticlesModule::init(Ogre::SceneManager* sceneManager, Ogre::uint32 maxParticles, Ogre::uint16 maxEmitterInstances, Ogre::uint16 maxEmitterCores,
								  Ogre::uint16 bucketSize, Ogre::uint16 gpuThreadsPerGroup)
	{
		// if (nullptr != this->gpuParticleSystemResourceManager)
		if (nullptr != this->sceneManager)
		{
			this->destroyContent();
		}

		this->sceneManager = sceneManager;

		Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingleton().getHlmsManager();
		// TODO: Wind or ocean is user0??
		HlmsParticle* hlmsParticle = static_cast<HlmsParticle*>(hlmsManager->getHlms(Ogre::HLMS_USER2));
		HlmsParticleListener* hlmsParticleListener = hlmsParticle->getParticleListener();

		std::vector<GpuParticleAffector*> affectors;

		this->gpuParticleSystemResourceManager = new GpuParticleSystemResourceManager();
		gpuParticleSystemResourceManager->registerCommonAffectors();
		this->gpuParticleSystemJsonManager = new GpuParticleSystemJsonManager();

		const GpuParticleSystemResourceManager::AffectorByIdStringMap& registeredAffectors = gpuParticleSystemResourceManager->getAffectorByPropertyMap();
		for (auto& it = registeredAffectors.begin(); it != registeredAffectors.end(); ++it)
		{
			affectors.push_back(it->second->clone());
		}

#if 0
		// TODO check with true, if compositor does work
		bool useDepthTexture = true;

	// Compositor is needed only in case of useDepthTexture == true.
		Ogre::IdString depthTextureCompositorNode = "NOWAFinalCompositionNode1111111112";
		Ogre::IdString depthTextureId = "depthTextureCopy";

		this->gpuParticleSystemWorld = OGRE_NEW GpuParticleSystemWorld(
			Ogre::Id::generateNewId<Ogre::MovableObject>(),
			&sceneManager->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC),
			sceneManager, 15, hlmsParticleListener, affectors,
			useDepthTexture, NOWA::WorkspaceModule::getInstance()->getPrimaryWorkspace(NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera()), depthTextureCompositorNode, depthTextureId);
#else

		this->gpuParticleSystemWorld = OGRE_NEW GpuParticleSystemWorld(Ogre::Id::generateNewId<Ogre::MovableObject>(), &this->sceneManager->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC),
																	   this->sceneManager, 15, hlmsParticleListener, affectors, false);
#endif


		//        mGpuParticleSystemWorld->init(4096, 64, 256, 64, 64);
	//            mGpuParticleSystemWorld->init(32768, 256, 256, 64, 64);
	//            mGpuParticleSystemWorld->init(131072, 1024, 256, 64, 64);
		this->gpuParticleSystemWorld->init(maxParticles, maxEmitterInstances, maxEmitterCores, bucketSize, gpuThreadsPerGroup);
		//            mGpuParticleSystemWorld->init(2097152, 16384, 16, 64, 64);


		this->sceneNode = this->sceneManager->getRootSceneNode()->createChildSceneNode();
		this->sceneNode->setName("GpuParticlesSystemWorldNode");
		this->sceneNode->attachObject(this->gpuParticleSystemWorld);

		// Loads all available particle scripts
		Ogre::StringVectorPtr names = Ogre::ResourceGroupManager::getSingletonPtr()->findResourceNames("GpuParticles", "*.json");
		for (Ogre::StringVector::iterator itName = names->begin(); itName != names->end(); ++itName)
		{
			Ogre::DataStreamPtr stream = Ogre::ResourceGroupManager::getSingletonPtr()->openResource(*itName, "GpuParticles");
			if (stream != nullptr)
			{
				this->gpuParticleSystemJsonManager->parseScript(stream, "GpuParticles");
			}
		}
	}

	void GpuParticlesModule::destroyContent(void)
	{
		if (nullptr == this->sceneManager)
		{
			return;
		}

		if (nullptr != this->gpuParticleSystemWorld)
		{
			this->sceneNode->detachObject(this->gpuParticleSystemWorld);
			this->sceneManager->destroySceneNode(this->sceneNode);

			delete this->gpuParticleSystemWorld;
			this->gpuParticleSystemWorld = nullptr;

			if (nullptr != this->gpuParticleSystemJsonManager)
			{
				delete this->gpuParticleSystemJsonManager;
				this->gpuParticleSystemJsonManager = nullptr;
			}

			if (nullptr != this->gpuParticleSystemResourceManager)
			{
				delete this->gpuParticleSystemResourceManager;
				this->gpuParticleSystemResourceManager = nullptr;
			}
		}
	}

	GpuParticleSystemWorld* GpuParticlesModule::getGpuParticleSystemWorld(void) const
	{
		return this->gpuParticleSystemWorld;
	}

	GpuParticleSystemResourceManager* GpuParticlesModule::getGpuParticleSystemResourceManager(void) const
	{
		return this->gpuParticleSystemResourceManager;
	}

} // namespace end