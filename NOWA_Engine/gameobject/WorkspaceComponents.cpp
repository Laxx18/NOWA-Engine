#include "NOWAPrecompiled.h"
#include "WorkspaceComponents.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "Modules/WorkspaceModule.h"
#include "main/Core.h"
#include "main/AppStateManager.h"
#include "CameraComponent.h"
#include "PlanarReflectionComponent.h"
#include "ReflectionCameraComponent.h"
#include "HdrEffectComponent.h"
#include "DatablockPbsComponent.h"
#include "Compositor/Pass/PassQuad/OgreCompositorPassQuadDef.h"
#include "Compositor/Pass/PassMipmap/OgreCompositorPassMipmapDef.h"
#include "Compositor/Pass/PassScene/OgreCompositorPassScene.h"
#include "OgreHlmsListener.h"
#include "OgreBitwise.h"

#include "Compositor/OgreCompositorShadowNodeDef.h"
#include "Compositor/OgreCompositorWorkspaceListener.h"

#include "TerraShadowMapper.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	///////////////////////////////////////////////////////////////////////////////////////////////

	class HlmsDebugLogListener : public Ogre::HlmsListener
	{
	public:

		void propertiesMergedPreGenerationStep(const Ogre::String& shaderProfile, const Ogre::HlmsCache& passCache, const Ogre::HlmsPropertyVec& renderableCacheProperties,
			const Ogre::PiecesMap renderableCachePieces[Ogre::NumShaderTypes], const Ogre::HlmsPropertyVec& properties, const Ogre::QueuedRenderable& queuedRenderable)
		{
			std::stringstream ss;
			ss << "\n";
			ss << "shaderProfile: " << shaderProfile << "\n";
			ss << "renderableCacheProperties:\n";
			for (const auto& p : renderableCacheProperties)
			{
				std::string key = p.keyName.getFriendlyText();
				ss << key << "=" << p.value << " ";
			}
			ss << "\n";
			ss << "properties:\n";
			for (const auto& p : properties)
			{
				std::string key = p.keyName.getFriendlyText();
				ss << key << "=" << p.value << " ";
			}
			ss << "\n";
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[WorkspaceBaseComponent] Hlms Debug Log: " + ss.str());
		}
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////

	class PlanarReflectionsWorkspaceListener : public Ogre::CompositorWorkspaceListener
	{
	public:
		PlanarReflectionsWorkspaceListener(Ogre::PlanarReflections* planarReflections)
			: planarReflections(planarReflections)
		{

		}

		virtual ~PlanarReflectionsWorkspaceListener()
		{

		}

		virtual void workspacePreUpdate(Ogre::CompositorWorkspace* workspace)
		{
			this->planarReflections->beginFrame();
		}

		virtual void passEarlyPreExecute(Ogre::CompositorPass* pass)
		{
			// Ignore non-scene passes
			if (pass->getType() != Ogre::PASS_SCENE)
				return;

			assert(dynamic_cast<const Ogre::CompositorPassSceneDef*>(pass->getDefinition()));

			const Ogre::CompositorPassSceneDef* passDef = static_cast<const Ogre::CompositorPassSceneDef*>(pass->getDefinition());

			// Ignore scene passes that belong to a shadow node.
			if (passDef->mShadowNodeRecalculation == Ogre::SHADOW_NODE_CASTER_PASS)
				return;

			// Ignore scene passes we haven't specifically tagged to receive reflections
			if (passDef->mIdentifier != 25001)
				return;

			Ogre::CompositorPassScene* passScene = static_cast<Ogre::CompositorPassScene*>(pass);
			Ogre::Camera* camera = passScene->getCamera();

			//Note: The Aspect Ratio must match that of the camera we're reflecting.
			this->planarReflections->update(camera, camera->getAspectRatio());
		}
	private:
		Ogre::PlanarReflections* planarReflections;
	};

	/////////////////////////////////////////////////////////////////////////////////////////

	WorkspaceBaseComponent::WorkspaceBaseComponent()
		: GameObjectComponent(),
		backgroundColor(new Variant(WorkspaceBaseComponent::AttrBackgroundColor(), Ogre::Vector3(0.0f, 0.0f, 0.0f), this->attributes)),
		viewportRect(new Variant(WorkspaceBaseComponent::AttrViewportRect(), Ogre::Vector4(0.0f, 0.0f, 1.0f, 1.0f), this->attributes)),
		superSampling(new Variant(WorkspaceBaseComponent::AttrSuperSampling(), Ogre::Real(1.0f), this->attributes)),
		useHdr(new Variant(WorkspaceBaseComponent::AttrUseHdr(), false, this->attributes)),
		useReflection(new Variant(WorkspaceBaseComponent::AttrUseReflection(), false, this->attributes)),
		reflectionCameraGameObjectId(new Variant(WorkspaceBaseComponent::AttrReflectionCameraGameObjectId(), static_cast<unsigned long>(0), this->attributes, true)),
		usePlanarReflection(new Variant(WorkspaceBaseComponent::AttrUsePlanarReflection(), false, this->attributes)),
		shadowGlobalBias(new Variant(WorkspaceBaseComponent::AttrShadowGlobalBias(), Ogre::Real(1.0f), this->attributes)),
		shadowGlobalNormalOffset(new Variant(WorkspaceBaseComponent::AttrShadowGlobalNormalOffset(), Ogre::Real(168.0f), this->attributes)),
		shadowPSSMLambda(new Variant(WorkspaceBaseComponent::AttrShadowPSSMLambda(), Ogre::Real(0.95f), this->attributes)),
		shadowSplitBlend(new Variant(WorkspaceBaseComponent::AttrShadowSplitBlend(), Ogre::Real(0.125f), this->attributes)),
		shadowSplitFade(new Variant(WorkspaceBaseComponent::AttrShadowSplitFade(), Ogre::Real(0.313f), this->attributes)),
		shadowSplitPadding(new Variant(WorkspaceBaseComponent::AttrShadowSplitPadding(), Ogre::Real(1.0f), this->attributes)),
		cameraComponent(nullptr),
		workspace(nullptr),
		msaaLevel(1u),
		oldBackgroundColor(backgroundColor->getVector3()),
		hlms(nullptr),
		pbs(nullptr),
		unlit(nullptr),
		hlmsManager(nullptr),
		compositorManager(nullptr),
		workspaceCubemap(nullptr),
		cubemap(nullptr),
		canUseReflection(false),
		planarReflections(nullptr),
		planarReflectionsWorkspaceListener(nullptr),
		useTerra(false),
		terra(nullptr),
		useOcean(false),
		hlmsListener(nullptr),
		hlmsWind(nullptr)
	{
		/*
		constantBiasScale( 1.0f ),
            normalOffsetBias( 168.0f ),
            autoConstantBiasScale( 100.0f ),
            autoNormalOffsetBiasScale( 4.0f ),
			pssmLambda( 0.95f ),
            splitPadding( 1.0f ),
            splitBlend( 0.125f ),
            splitFade( 0.313f ),
		*/

		this->backgroundColor->addUserData(GameObject::AttrActionColorDialog());
		this->superSampling->setDescription("Sets the supersampling for whole scene texture rendering. E.g. a value of 0.25 will pixelize the scene for retro Game experience.");
		this->useReflection->setDescription("Activates reflection, which is rendered by a dynamic cubemap. This component works in conjunction with a ReflectionCameraComponent. Attention: Acitvating this mode decreases the application's performance considerably!");
		this->usePlanarReflection->setDescription("Activates planar reflection, which is used for mirror planes in conjunction with a PlanarReflectionComponent.");
		this->referenceId->setVisible(false);
		this->reflectionCameraGameObjectId->setVisible(false);

		this->useReflection->addUserData(GameObject::AttrActionNeedRefresh());
	}

	WorkspaceBaseComponent::~WorkspaceBaseComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[WorkspaceBaseComponent] Destructor workspace base component for game object: " + this->gameObjectPtr->getName());
		
		this->cameraComponent = nullptr;

		// Send event, that component has been deleted
		boost::shared_ptr<EventDataDeleteWorkspaceComponent> eventDataDeleteWorkspaceComponent(new EventDataDeleteWorkspaceComponent(this->gameObjectPtr->getId()));
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataDeleteWorkspaceComponent);

		this->removeWorkspace();
	}

	bool WorkspaceBaseComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BackgroundColor")
		{
			this->backgroundColor->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ViewportRect")
		{
			this->viewportRect->setValue(XMLConverter::getAttribVector4(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SuperSampling")
		{
			this->superSampling->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseHdr")
		{
			this->useHdr->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseReflection")
		{
			this->setUseReflection(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ReflectionCameraGameObjectId")
		{
			this->reflectionCameraGameObjectId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UsePlanarReflection")
		{
			bool hasAnyMirror = this->hasAnyMirrorForPlanarReflections();

			if (true == hasAnyMirror)
			{
				this->usePlanarReflection->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			}
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseTerra")
		{
			this->useTerra = XMLConverter::getAttribBool(propertyElement, "data");
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseOcean")
		{
			this->useOcean = XMLConverter::getAttribBool(propertyElement, "data");
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ShadowGlobalBias")
		{
			this->shadowGlobalBias->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ShadowGlobalNormalOffset")
		{
			this->shadowGlobalNormalOffset->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ShadowPSSMLambda")
		{
			this->shadowPSSMLambda->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ShadowSplitBlend")
		{
			this->shadowSplitBlend->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ShadowSplitFade")
		{
			this->shadowSplitFade->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ShadowSplitPadding")
		{
			this->shadowSplitPadding->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr WorkspaceBaseComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		return nullptr;
	}

	bool WorkspaceBaseComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[WorkspaceBaseComponent] Init workspace base component for game object: " + this->gameObjectPtr->getName());
		
		this->msaaLevel = this->getMSAA();
		
		// Get hlms data
		this->hlms = Core::getSingletonPtr()->getOgreRoot()->getHlmsManager()->getHlms(Ogre::HLMS_PBS);
		// assert(dynamic_cast<Ogre::HlmsPbs*>(hlms));
		this->pbs = static_cast<Ogre::HlmsPbs*>(hlms);
		// assert(dynamic_cast<Ogre::HlmsUnlit*>(hlms));
		this->unlit = static_cast<Ogre::HlmsUnlit*>(hlms);
		this->hlmsManager = Core::getSingletonPtr()->getOgreRoot()->getHlmsManager();
		this->compositorManager = WorkspaceModule::getInstance()->getCompositorManager();

		auto& cameraCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<CameraComponent>());
		if (nullptr != cameraCompPtr)
		{
			this->cameraComponent = cameraCompPtr.get();
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[WorkspaceBaseComponent] Could not get prior CameraComponent. This component must be set under the CameraComponent! Affected game object: " + this->gameObjectPtr->getName());
			return false;
		}

		bool hasAnyMirror = this->hasAnyMirrorForPlanarReflections();

		this->usePlanarReflection->setValue(hasAnyMirror);

		return this->createWorkspace();
	}

	void WorkspaceBaseComponent::internalInitWorkspaceData(void)
	{
		this->workspaceName = "NOWAWorkspace" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
		this->renderingNodeName = "NOWARenderingNode" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
		if (false == this->useHdr->getBool())
		{
			this->finalRenderingNodeName = "NOWAPbsFinalCompositionNode";
		}
		else
		{
			this->finalRenderingNodeName = "NOWAHdrFinalCompositionNode";
		}

		if (true == this->usePlanarReflection->getBool())
		{
			this->planarReflectionReflectiveRenderingNode = "PlanarReflectionsReflectiveRenderingNode" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
			this->planarReflectionReflectiveWorkspaceName = "NOWAPlanarReflectionsReflectiveWorkspace" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
		}
	}

	bool WorkspaceBaseComponent::createWorkspace(void)
	{
		if (nullptr == this->compositorManager)
			return true;

		// Only create workspace if the camera is activated
		if (false == this->cameraComponent->isActivated())
			return true;

		this->removeWorkspace();

		this->internalInitWorkspaceData();

		this->resetReflectionForAllEntities();

		std::vector<bool> reflections(AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects()->size());
		auto gameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects();

		Ogre::String cubemapRenderingNode;

		if (true == this->useReflection->getBool())
		{
			// Store, which game object has used reflections

			unsigned int i = 0;
			for (auto& it = gameObjects->begin(); it != gameObjects->end(); ++it)
			{
				auto& gameObjectPtr = it->second;
				if (nullptr != gameObjectPtr)
				{
					reflections[i++] = gameObjectPtr->getUseReflection();
					// gameObjectPtr->getAttribute(GameObject::AttrUseReflection())->setVisible(true);
					gameObjectPtr->setUseReflection(false);
				}
			}

			WorkspacePbsComponent* workspacePbsComponent = dynamic_cast<WorkspacePbsComponent*>(this);
			WorkspaceSkyComponent* workspaceSkyComponent = nullptr;
			WorkspaceBackgroundComponent* workspaceBackgroundComponent = nullptr;
			WorkspaceCustomComponent* workspaceCustomComponent = nullptr;
			

			// TODO:  const IdString cubemapRendererNode = renderWindow->getSampleDescription().isMultisample()
			// ? "CubemapRendererNodeMsaa"
			// 	: "CubemapRendererNode";
			if (nullptr == workspacePbsComponent)
			{
				workspaceSkyComponent = dynamic_cast<WorkspaceSkyComponent*>(this);
				if (nullptr == workspaceSkyComponent)
				{
					workspaceBackgroundComponent = dynamic_cast<WorkspaceBackgroundComponent*>(this);
					if (nullptr != workspaceBackgroundComponent)
					{
						cubemapRenderingNode = "NOWABackgroundDynamicCubemapProbeRendererNode";
					}
					else
					{
						workspaceCustomComponent = dynamic_cast<WorkspaceCustomComponent*>(this);
						if (nullptr != workspaceCustomComponent)
						{
							cubemapRenderingNode = "NOWASkyDynamicCubemapProbeRendererNode";
						}
					}
				}
				else
				{
					cubemapRenderingNode = "NOWASkyDynamicCubemapProbeRendererNode";
				}
			}
			else
			{
				cubemapRenderingNode = "NOWAPbsDynamicCubemapProbeRendererNode";
			}
		}
		/*else
		{
			for (auto& it = gameObjects->begin(); it != gameObjects->end(); ++it)
			{
				auto& gameObjectPtr = it->second;
				if (nullptr != gameObjectPtr)
				{
					gameObjectPtr->getAttribute(GameObject::AttrUseReflection())->setVisible(false);
				}
			}
		}*/

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[WorkspacePbsComponent] Creating workspace: " + this->workspaceName);

		const Ogre::IdString workspaceNameId(this->workspaceName);
		if (false == this->compositorManager->hasWorkspaceDefinition(workspaceNameId))
		{
			Ogre::ColourValue color(this->backgroundColor->getVector3().x, this->backgroundColor->getVector3().y, this->backgroundColor->getVector3().z);
			this->compositorManager->createBasicWorkspaceDef(this->workspaceName, color, Ogre::IdString());
		}

		Ogre::CompositorWorkspaceDef* workspaceDef = WorkspaceModule::getInstance()->getCompositorManager()->getWorkspaceDefinition(this->workspaceName);
		workspaceDef->clearAll();

		Ogre::CompositorManager2::CompositorNodeDefMap nodeDefs = WorkspaceModule::getInstance()->getCompositorManager()->getNodeDefinitions();

		// Iterate through Compositor Managers resources
		Ogre::CompositorManager2::CompositorNodeDefMap::const_iterator it = nodeDefs.begin();
		Ogre::CompositorManager2::CompositorNodeDefMap::const_iterator end = nodeDefs.end();

		Ogre::IdString compositorId = "Ogre/Postprocess";

		// Add all compositor resources to the view container
		while (it != end)
		{
			if (it->second->mCustomIdentifier == compositorId)
			{
				// this->compositorNames.emplace_back(it->second->getNameStr());

				// Manually disable the node and add it to the workspace without any connection
				it->second->setStartEnabled(false);
				workspaceDef->addNodeAlias(it->first, it->first);
			}

			++it;
		}

		if (true == this->useReflection->getBool())
		{
			GameObjectPtr reflectionCameraGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->reflectionCameraGameObjectId->getULong());
			if (nullptr != reflectionCameraGameObjectPtr)
			{
				auto& reflectionCameraCompPtr = NOWA::makeStrongPtr(reflectionCameraGameObjectPtr->getComponent<ReflectionCameraComponent>());
				if (nullptr != reflectionCameraCompPtr)
				{
					// Set this component as back reference, because when reflection camera is deleted, reflection must be disabled immediately, else a render crash occurs
					reflectionCameraCompPtr->workspaceBaseComponent = this;

					this->canUseReflection = true;

					// We first create the Cubemap workspace and pass it to the final workspace
					// that does the real rendering.
					//
					// If in your application you need to create a workspace but don't have a cubemap yet,
					// you can either programatically modify the workspace definition (which is cumbersome)
					// or just pass a PF_NULL texture that works as a dud and barely consumes any memory.
					
					unsigned int iblSpecularFlag = 0;
					if (Ogre::Root::getSingletonPtr()->getRenderSystem()->getCapabilities()->hasCapability(Ogre::RSC_COMPUTE_PROGRAM))
					{
						iblSpecularFlag = Ogre::TextureFlags::Uav | Ogre::TextureFlags::Reinterpretable;
					}

					Ogre::TextureGpuManager* textureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();

					/*
					this->cubemap = textureManager->createTexture("cubemap", Ogre::GpuPageOutStrategy::Discard,
						Ogre::TextureFlags::RenderToTexture | Ogre::TextureFlags::AllowAutomipmaps, Ogre::TextureTypes::TypeCube);*/

					this->cubemap = textureManager->createOrRetrieveTexture("cubemap", 
						Ogre::GpuPageOutStrategy::Discard, Ogre::TextureFlags::RenderToTexture | Ogre::TextureFlags::AllowAutomipmaps | 
						iblSpecularFlag, Ogre::TextureTypes::TypeCube);
					this->cubemap->scheduleTransitionTo(Ogre::GpuResidency::OnStorage);

					unsigned int cubeTextureSize = Ogre::StringConverter::parseUnsignedInt(reflectionCameraCompPtr->getCubeTextureSize());

					this->cubemap->setResolution(cubeTextureSize, cubeTextureSize);
					this->cubemap->setNumMipmaps(Ogre::PixelFormatGpuUtils::getMaxMipmapCount(cubeTextureSize, cubeTextureSize));
					this->cubemap->setPixelFormat(Ogre::PFG_RGBA8_UNORM_SRGB);
					this->cubemap->_transitionTo(Ogre::GpuResidency::Resident, (Ogre::uint8*)0);

					this->pbs->resetIblSpecMipmap(0u);

					//Setup the cubemap's compositor.
					Ogre::CompositorChannelVec cubemapExternalChannels(1);
					//Any of the cubemap's render targets will do
					cubemapExternalChannels[0] = this->cubemap;

					Ogre::String workspaceCubemapName = this->workspaceName + "_cubemap";

					if (false == this->compositorManager->hasWorkspaceDefinition(workspaceCubemapName))
					{
						Ogre::CompositorWorkspaceDef* workspaceDef = this->compositorManager->addWorkspaceDefinition(workspaceCubemapName);
						// "NOWALocalCubemapProbeRendererNode" has been defined in scripts.
						// Very handy (as it 99% the same for everything)
						workspaceDef->connectExternal(0, cubemapRenderingNode, 0);
					}

					// If camera has not been created yet, do it!
					if (nullptr == reflectionCameraCompPtr->getCamera())
					{
						reflectionCameraCompPtr->postInit();
					}

					this->workspaceCubemap = this->compositorManager->addWorkspace(this->gameObjectPtr->getSceneManager(), cubemapExternalChannels, reflectionCameraCompPtr->getCamera(),
						workspaceCubemapName, true);

					/*this->workspaceCubemap = this->compositorManager->addWorkspace(this->gameObjectPtr->getSceneManager(), cubemapExternalChannels, reflectionCameraCompPtr->getCamera(),
						workspaceCubemapName, true, -1, (Ogre::UavBufferPackedVec*)0, &this->initialCubemapLayouts, &this->initialCubemapUavAccess);*/

					// Now setup the regular renderer
					this->externalChannels.resize(2);
					// Render window
					this->externalChannels[0] = Core::getSingletonPtr()->getOgreRenderWindow()->getTexture();
					this->externalChannels[1] = this->cubemap;
				}
				else
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[WorkspaceBaseComponent] Warning reflections will not work, because the given game object id: " + Ogre::StringConverter::toString(this->reflectionCameraGameObjectId->getULong()) +
						" has no " + CameraComponent::getStaticClassName() + ". Affected game object : " + this->gameObjectPtr->getName());
				}
			}
			else
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[WorkspaceBaseComponent] Warning reflections will not work, because the given game object id: " + Ogre::StringConverter::toString(this->reflectionCameraGameObjectId->getULong()) 
					+ " does not exist. Affected game object : " + this->gameObjectPtr->getName());
			}
		}

		this->internalCreateCompositorNode();

		this->updateShadowGlobalBias();

		if (true == this->useTerra)
		{
			unsigned short externalInputTextureId = 1;

			if (false == this->canUseReflection)
			{
#ifdef TERRA_OLD
				this->externalChannels.resize(2);
#else
				this->externalChannels.resize(1);
#endif
			}
			else
			{
#ifdef TERRA_OLD
				this->externalChannels.resize(3);
				externalInputTextureId = 2;
#else
				this->externalChannels.resize(2);
				externalInputTextureId = 1;
#endif
			}
#ifdef TERRA_OLD
			//Terra's Shadow texture
			if (nullptr != terra)
			{
				//Terra is initialized
				const Ogre::ShadowMapper* shadowMapper = terra->getShadowMapper();
				// shadowMapper->fillUavDataForCompositorChannel(&this->externalChannels[externalInputTextureId], this->initialCubemapLayouts, this->initialCubemapUavAccess);
				shadowMapper->fillUavDataForCompositorChannel(&this->externalChannels[externalInputTextureId]);
			}
			else
			{
				//The texture is not available. Create a dummy dud.
				Ogre::TextureGpuManager* textureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();
				Ogre::TextureGpu* nullTex = textureManager->createOrRetrieveTexture("DummyTerraNull", Ogre::GpuPageOutStrategy::Discard, Ogre::TextureFlags::ManualTexture, Ogre::TextureTypes::Type2D);
				nullTex->setResolution(1u, 1u);
				nullTex->setPixelFormat(Ogre::PFG_R10G10B10A2_UNORM);
				nullTex->scheduleTransitionTo(Ogre::GpuResidency::Resident);
				this->externalChannels[externalInputTextureId] = nullTex;
			}
#endif
		}


		if (true == this->usePlanarReflection->getBool())
		{
			if (false == this->compositorManager->hasWorkspaceDefinition(this->planarReflectionReflectiveWorkspaceName))
			{
				Ogre::ColourValue color(this->backgroundColor->getVector3().x, this->backgroundColor->getVector3().y, this->backgroundColor->getVector3().z);
				this->compositorManager->createBasicWorkspaceDef(this->planarReflectionReflectiveWorkspaceName, color, Ogre::IdString());

				Ogre::CompositorWorkspaceDef* workspaceDef = WorkspaceModule::getInstance()->getCompositorManager()->getWorkspaceDefinition(this->planarReflectionReflectiveWorkspaceName);

				workspaceDef->connectExternal(0, this->planarReflectionReflectiveRenderingNode, 0);
			}
		}

		// this->initializeSmaa(WorkspaceBaseComponent::SMAA_PRESET_ULTRA, WorkspaceBaseComponent::EdgeDetectionColour);

		bool success = this->internalCreateWorkspace(workspaceDef);

		if (true == this->usePlanarReflection->getBool() && true == success)
		{
			// Setup PlanarReflections, 1.0 = max distance
			this->planarReflections = new Ogre::PlanarReflections(this->gameObjectPtr->getSceneManager(), this->compositorManager, 1.0f, nullptr);
			this->planarReflectionsWorkspaceListener = new PlanarReflectionsWorkspaceListener(this->planarReflections);
			this->workspace->addListener(this->planarReflectionsWorkspaceListener);

			// Attention: Only one planar reflection setting is possible!
			this->pbs->setPlanarReflections(this->planarReflections);
		}

		if (true == this->canUseReflection || true == this->usePlanarReflection->getBool())
		{
			// Restore the reflections
			unsigned int j = 0;
			for (auto& it = gameObjects->begin(); it != gameObjects->end(); ++it)
			{
				auto& gameObjectPtr = it->second;
				if (nullptr != gameObjectPtr)
				{
					// Refresh reflections
					gameObjectPtr->setUseReflection(reflections[j]);

					if (true == this->usePlanarReflection->getBool())
					{
						auto planarReflectionCompPtr = NOWA::makeStrongPtr(gameObjectPtr->getComponent<PlanarReflectionComponent>());
						if (nullptr != planarReflectionCompPtr)
						{
							planarReflectionCompPtr->createPlane();
						}
					}

					j++;
				}
			}
		}

		this->hlmsWind = dynamic_cast<HlmsWind*>(Ogre::Root::getSingleton().getHlmsManager()->getHlms(Ogre::HLMS_USER0));
		hlmsWind->setup(this->gameObjectPtr->getSceneManager());

		return success;
	}

	void WorkspaceBaseComponent::removeWorkspace(void)
	{
		this->resetReflectionForAllEntities();

		Ogre::TextureGpuManager* textureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();

		if (nullptr != this->workspaceCubemap)
		{
			this->compositorManager->removeWorkspace(this->workspaceCubemap);
			this->workspaceCubemap = nullptr;
		}
		if (nullptr != this->cubemap)
		{
			textureManager->destroyTexture(this->cubemap);
			this->cubemap = nullptr;
		}
		
		if (nullptr != this->workspace)
		{
			if (nullptr != this->planarReflectionsWorkspaceListener)
			{
				this->workspace->removeListener(this->planarReflectionsWorkspaceListener);
			}

			if (true == this->useTerra)
			{
				unsigned short externalInputTextureId = 1;
				if (true == this->canUseReflection)
				{
					externalInputTextureId = 2;
				}

				Ogre::TextureGpu* nullTex = textureManager->findTextureNoThrow("DummyTerraNull");
				if (nullptr != nullTex)
				{
					textureManager->destroyTexture(nullTex);
				}

				if (this->workspace->getExternalRenderTargets().size() > externalInputTextureId)
				{
					Ogre::TextureGpu* terraShadowTex = this->workspace->getExternalRenderTargets()[externalInputTextureId];
					if (terraShadowTex->getPixelFormat() == Ogre::PFG_NULL)
					{
						textureManager->destroyTexture(terraShadowTex);
					}
				}
			}

			this->compositorManager->removeWorkspace(this->workspace);

			if (true == this->compositorManager->hasWorkspaceDefinition(this->workspaceName + "_cubemap"))
			{
				this->compositorManager->removeWorkspaceDefinition(this->workspaceName + "_cubemap");
			}
			// Note: Each time addWorkspaceDefinition is called, an "AutoGen Hash..." node is internally created by Ogre, this one must also be removed, else a crash occurs (bug in Ogre?)
			if (true == this->compositorManager->hasNodeDefinition("AutoGen " + Ogre::IdString(this->workspaceName + "/Node").getReleaseText()))
			{
				this->compositorManager->removeNodeDefinition("AutoGen " + Ogre::IdString(this->workspaceName + "/Node").getReleaseText());
			}
			if (true == this->compositorManager->hasWorkspaceDefinition(this->workspaceName))
			{
				this->compositorManager->removeWorkspaceDefinition(this->workspaceName);
			}
			
			if (true == this->compositorManager->hasNodeDefinition(this->renderingNodeName))
			{
				this->compositorManager->removeNodeDefinition(this->renderingNodeName);
			}
			this->workspace = nullptr;
			this->externalChannels.clear();

			if (nullptr != this->planarReflectionsWorkspaceListener)
			{
				delete this->planarReflectionsWorkspaceListener;
				this->planarReflectionsWorkspaceListener = nullptr;
			}
			if (nullptr != this->planarReflections)
			{
				delete this->planarReflections;
				this->planarReflections = nullptr;
			}
			this->planarReflectionActors.clear();

			if (true == this->compositorManager->hasNodeDefinition(this->planarReflectionReflectiveRenderingNode))
			{
				this->compositorManager->removeNodeDefinition(this->planarReflectionReflectiveRenderingNode);
			}
			if (true == this->compositorManager->hasNodeDefinition("AutoGen " + Ogre::IdString(this->planarReflectionReflectiveWorkspaceName + "/Node").getReleaseText()))
			{
				this->compositorManager->removeNodeDefinition("AutoGen " + Ogre::IdString(this->planarReflectionReflectiveWorkspaceName + "/Node").getReleaseText());
			}
			
			if (true == this->compositorManager->hasWorkspaceDefinition(this->planarReflectionReflectiveWorkspaceName))
			{
				this->compositorManager->removeWorkspaceDefinition(this->planarReflectionReflectiveWorkspaceName);
			}

			if (nullptr != this->hlmsListener)
			{
				delete this->hlmsListener;
				this->hlmsListener = nullptr;
			}
		}
	}

	void WorkspaceBaseComponent::onOtherComponentRemoved(unsigned int index)
	{
		auto& gameObjectCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponentByIndex(index));
		if (nullptr != gameObjectCompPtr)
		{
			auto& cameraCompPtr = boost::dynamic_pointer_cast<CameraComponent>(gameObjectCompPtr);
			if (nullptr != cameraCompPtr)
			{
				// Camera has been removed, delete also this component
				this->gameObjectPtr->deleteComponent(this->getClassName());
			}
		}
	}

	void WorkspaceBaseComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		GameObjectPtr reflectionCameraGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->reflectionCameraGameObjectId->getULong());
		if (nullptr != reflectionCameraGameObjectPtr)
		{
			auto& reflectionCameraCompPtr = NOWA::makeStrongPtr(reflectionCameraGameObjectPtr->getComponent<ReflectionCameraComponent>());
			if (nullptr != reflectionCameraCompPtr)
			{
				reflectionCameraCompPtr->workspaceBaseComponent = nullptr;
			}
		}

		if (nullptr != this->cameraComponent)
		{
			WorkspaceModule::getInstance()->removeWorkspace(this->gameObjectPtr->getSceneManager(), this->cameraComponent->getCamera());
		}
	}

	void WorkspaceBaseComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (WorkspaceBaseComponent::AttrBackgroundColor() == attribute->getName())
		{
			this->setBackgroundColor(attribute->getVector3());
		}
		else if (WorkspaceBaseComponent::AttrViewportRect() == attribute->getName())
		{
			this->setViewportRect(attribute->getVector4());
		}
		else if (WorkspaceBaseComponent::AttrSuperSampling() == attribute->getName())
		{
			this->setSuperSampling(attribute->getReal());
		}
		else if (WorkspaceBaseComponent::AttrUseHdr() == attribute->getName())
		{
			this->setUseHdr(attribute->getBool());
		}
		else if (WorkspaceBaseComponent::AttrUseReflection() == attribute->getName())
		{
			this->setUseReflection(attribute->getBool());
		}
		else if (WorkspaceBaseComponent::AttrReflectionCameraGameObjectId() == attribute->getName())
		{
			this->setReflectionCameraGameObjectId(attribute->getULong());
		}
		else if (WorkspaceBaseComponent::AttrUsePlanarReflection() == attribute->getName())
		{
			this->setUsePlanarReflection(attribute->getBool());
		}
		else if (WorkspaceBaseComponent::AttrShadowGlobalBias() == attribute->getName())
		{
			this->setShadowGlobalBias(attribute->getReal());
		}
		else if (WorkspaceBaseComponent::AttrShadowGlobalNormalOffset() == attribute->getName())
		{
			this->setShadowGlobalNormalOffset(attribute->getReal());
		}
		else if (WorkspaceBaseComponent::AttrShadowPSSMLambda() == attribute->getName())
		{
			this->setShadowPSSMLambda(attribute->getReal());
		}
		else if (WorkspaceBaseComponent::AttrShadowSplitBlend() == attribute->getName())
		{
			this->setShadowSplitBlend(attribute->getReal());
		}
		else if (WorkspaceBaseComponent::AttrShadowSplitFade() == attribute->getName())
		{
			this->setShadowSplitFade(attribute->getReal());
		}
		else if (WorkspaceBaseComponent::AttrShadowSplitPadding() == attribute->getName())
		{
			this->setShadowSplitPadding(attribute->getReal());
		}
	}

	void WorkspaceBaseComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		GameObjectComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BackgroundColor"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->backgroundColor->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ViewportRect"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->viewportRect->getVector4())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SuperSampling"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->superSampling->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UseHdr"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->useHdr->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UseReflection"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->useReflection->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ReflectionCameraGameObjectId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->reflectionCameraGameObjectId->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UsePlanarReflection"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->usePlanarReflection->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UseTerra"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->useTerra)));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UseOcean"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->useOcean)));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ShadowGlobalBias"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->shadowGlobalBias->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ShadowGlobalNormalOffset"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->shadowGlobalNormalOffset->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ShadowPSSMLambda"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->shadowPSSMLambda->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ShadowSplitBlend"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->shadowSplitBlend->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ShadowSplitFade"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->shadowSplitFade->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ShadowSplitPadding"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->shadowSplitPadding->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String WorkspaceBaseComponent::getClassName(void) const
	{
		return "WorkspaceBaseComponent";
	}

	Ogre::String WorkspaceBaseComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void WorkspaceBaseComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (nullptr != this->hlmsWind && false == notSimulating)
		{
			this->hlmsWind->addTime(dt);
		}
	}

	void WorkspaceBaseComponent::changeBackgroundColor(const Ogre::ColourValue& backgroundColor)
	{
		// Attention: Does not work, because clearDef->mColourValue = backgroundColor; is no more available
		if (nullptr == this->workspace)
		{
			return;
		}
		Ogre::CompositorManager2::CompositorNodeDefMap nodeDefinitions = this->compositorManager->getNodeDefinitions();
		Ogre::CompositorNodeDef* nodeDef;
		Ogre::CompositorTargetDef* targetDef;
		Ogre::CompositorPassDef* passDef;
		Ogre::CompositorPassClearDef* clearDef;
		bool foundPass = false;
		for (auto& it = nodeDefinitions.cbegin(); it != nodeDefinitions.cend(); ++it)
		{
			nodeDef = it->second;
			if (nodeDef)
			{
				if (nodeDef->getNumTargetPasses() > 0)
				{
					targetDef = nodeDef->getTargetPass(0);
					Ogre::CompositorPassDefVec passDefs = targetDef->getCompositorPasses();
					Ogre::CompositorPassDefVec::const_iterator iterPass;
					Ogre::CompositorPassDefVec::const_iterator iterPassStart = passDefs.begin();
					Ogre::CompositorPassDefVec::const_iterator iterPassEnd = passDefs.end();
					for (iterPass = iterPassStart; iterPass != iterPassEnd; ++iterPass)
					{
						passDef = *iterPass;
						if (Ogre::PASS_CLEAR == passDef->getType())
						{
							clearDef = static_cast<Ogre::CompositorPassClearDef*>(passDef);
							// Attention: Is this correct?
							for (unsigned short i = 0; i < OGRE_MAX_MULTIPLE_RENDER_TARGETS; i++)
							{
								passDef->mClearColour[i] = backgroundColor;
								// clearDef->mColourValue = backgroundColor;
							}
							foundPass = true;
						}
					}
				}
			}
		}

		if (true == foundPass)
		{
			this->createWorkspace();
		}
	}

	void WorkspaceBaseComponent::changeViewportRect(unsigned short viewportIndex, const Ogre::Vector4& viewportRect)
	{
		if (true == this->renderingNodeName.empty())
		{
			return;
		}

		Ogre::CompositorNodeDef* nodeDef = this->compositorManager->getNodeDefinitionNonConst(this->renderingNodeName);

		assert(nodeDef->getNumTargetPasses() >= 1);

		Ogre::CompositorTargetDef* targetDef = nodeDef->getTargetPass(0);
		Ogre::CompositorPassDefVec& passDefs = targetDef->getCompositorPassesNonConst();
		assert(passDefs.size() >= 1);
		for (size_t i = 0; i < passDefs.size(); i++)
		{
			Ogre::CompositorPassDef*& passDef = passDefs[i];

			Ogre::CompositorPassDef::ViewportRect rect;
			rect.mVpLeft = viewportRect.x;
			rect.mVpTop = viewportRect.y;
			rect.mVpWidth = viewportRect.z;
			rect.mVpHeight = viewportRect.w;
			rect.mVpScissorLeft = rect.mVpLeft;
			rect.mVpScissorTop = rect.mVpTop;
			rect.mVpScissorWidth = rect.mVpWidth;
			rect.mVpScissorHeight = rect.mVpHeight;

			passDef->mVpRect[viewportIndex] = rect;
		}
	}

	unsigned char WorkspaceBaseComponent::getMSAA(void)
	{
		const Ogre::RenderSystemCapabilities* caps = Core::getSingletonPtr()->getOgreRoot()->getRenderSystem()->getCapabilities();

		// If MSAA is set more than default in ogre.cfg and graphics card has (MultiSampleAntiAliasing) support, use it!
		// Possible values are 1,2,4,8
		// 1 is used when its disabled
		if (Core::getSingletonPtr()->getOgreRenderWindow()->isMultisample() > 1u && caps->hasCapability(Ogre::RSC_EXPLICIT_FSAA_RESOLVE))
			return Core::getSingletonPtr()->getOgreRenderWindow()->isMultisample();

		return 1u;
	}

	void WorkspaceBaseComponent::initializeHdr(Ogre::uint8 fsaa)
	{
		if (false == this->useHdr)
		{
			return;
		}

		if (fsaa <= 1)
		{
			return;
		}

		Ogre::String preprocessorDefines = "MSAA_INITIALIZED=1,";

		preprocessorDefines += "MSAA_SUBSAMPLE_WEIGHT=";
		preprocessorDefines += Ogre::StringConverter::toString(1.0f / (float)fsaa);
		preprocessorDefines += ",MSAA_NUM_SUBSAMPLES=";
		preprocessorDefines += Ogre::StringConverter::toString(fsaa);

		Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().load(
			"HDR/Resolve_4xFP32_HDR_Box",
			Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME).
			staticCast<Ogre::Material>();

		Ogre::Pass* pass = material->getTechnique(0)->getPass(0);

		Ogre::GpuProgram* shader = 0;
		Ogre::GpuProgramParametersSharedPtr oldParams;

		//Save old manual & auto params
		oldParams = pass->getFragmentProgramParameters();
		//Retrieve the HLSL/GLSL/Metal shader and rebuild it with the right settings.
		shader = pass->getFragmentProgram()->_getBindingDelegate();
		shader->setParameter("preprocessor_defines", preprocessorDefines);
		pass->getFragmentProgram()->reload();
		//Restore manual & auto params to the newly compiled shader
		pass->getFragmentProgramParameters()->copyConstantsFrom(*oldParams);
	}

	void WorkspaceBaseComponent::createCompositorEffectsFromCode(void)
	{
		Ogre::Root* root = Core::getSingletonPtr()->getOgreRoot();

		// Bloom compositor is loaded from script but here is the hard coded equivalent
		if (!this->compositorManager->hasNodeDefinition("Bloom"))
		{
			Ogre::CompositorNodeDef* bloomDef = compositorManager->addNodeDefinition("Bloom");

			//Input channels
			bloomDef->addTextureSourceName("rt_input", 0, Ogre::TextureDefinitionBase::TEXTURE_INPUT);
			bloomDef->addTextureSourceName("rt_output", 1, Ogre::TextureDefinitionBase::TEXTURE_INPUT);

			bloomDef->mCustomIdentifier = "Ogre/Postprocess";

			//Local textures
			bloomDef->setNumLocalTextureDefinitions(2);
			{
				Ogre::TextureDefinitionBase::TextureDefinition* texDef = bloomDef->addTextureDefinition("rt0");
				texDef->widthFactor = 0.25f;
				texDef->heightFactor = 0.25f;
				texDef->format = Ogre::PFG_RGBA8_UNORM_SRGB;

				Ogre::RenderTargetViewDef* rtv = bloomDef->addRenderTextureView("rt0");
				Ogre::RenderTargetViewEntry attachment;
				attachment.textureName = "rt0";
				rtv->colourAttachments.push_back(attachment);
				rtv->depthBufferId = Ogre::DepthBuffer::POOL_NO_DEPTH;

				texDef = bloomDef->addTextureDefinition("rt1");
				texDef->widthFactor = 0.25f;
				texDef->heightFactor = 0.25f;
				texDef->format = Ogre::PFG_RGBA8_UNORM_SRGB;

				rtv = bloomDef->addRenderTextureView("rt1");
				attachment.textureName = "rt1";
				rtv->colourAttachments.push_back(attachment);
				rtv->depthBufferId = Ogre::DepthBuffer::POOL_NO_DEPTH;
			}

			bloomDef->setNumTargetPass(4);

			{
				Ogre::CompositorTargetDef* targetDef = bloomDef->addTargetPass("rt0");

				{
					Ogre::CompositorPassQuadDef* passQuad;
					passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));
					passQuad->setAllLoadActions(Ogre::LoadAction::DontCare);
					passQuad->mMaterialName = "Postprocess/BrightPass2";
					passQuad->addQuadTextureSource(0, "rt_input");
				}
			}
			{
				Ogre::CompositorTargetDef* targetDef = bloomDef->addTargetPass("rt1");

				{
					Ogre::CompositorPassQuadDef* passQuad;
					passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));
					passQuad->setAllLoadActions(Ogre::LoadAction::DontCare);
					passQuad->mMaterialName = "Postprocess/BlurV";
					passQuad->addQuadTextureSource(0, "rt0");
				}
			}
			{
				Ogre::CompositorTargetDef* targetDef = bloomDef->addTargetPass("rt0");

				{
					Ogre::CompositorPassQuadDef* passQuad;
					passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));
					passQuad->setAllLoadActions(Ogre::LoadAction::DontCare);
					passQuad->mMaterialName = "Postprocess/BlurH";
					passQuad->addQuadTextureSource(0, "rt1");
				}
			}
			{
				Ogre::CompositorTargetDef* targetDef = bloomDef->addTargetPass("rt_output");

				{
					Ogre::CompositorPassQuadDef* passQuad;
					passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));
					passQuad->setAllLoadActions(Ogre::LoadAction::DontCare);
					passQuad->mMaterialName = "Postprocess/BloomBlend2";
					passQuad->addQuadTextureSource(0, "rt_input");
					passQuad->addQuadTextureSource(1, "rt0");
				}
			}

			//Output channels
			bloomDef->setNumOutputChannels(2);
			bloomDef->mapOutputChannel(0, "rt_output");
			bloomDef->mapOutputChannel(1, "rt_input");
		}

		//Glass compositor is loaded from script but here is the hard coded equivalent
		if (!compositorManager->hasNodeDefinition("Glass"))
		{
			Ogre::CompositorNodeDef* glassDef = compositorManager->addNodeDefinition("Glass");

			//Input channels
			glassDef->addTextureSourceName("rt_input", 0, Ogre::TextureDefinitionBase::TEXTURE_INPUT);
			glassDef->addTextureSourceName("rt_output", 1, Ogre::TextureDefinitionBase::TEXTURE_INPUT);

			glassDef->mCustomIdentifier = "Ogre/Postprocess";

			glassDef->setNumTargetPass(1);

			{
				Ogre::CompositorTargetDef* targetDef = glassDef->addTargetPass("rt_output");

				{
					Ogre::CompositorPassQuadDef* passQuad;
					passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));
					passQuad->setAllLoadActions(Ogre::LoadAction::DontCare);
					passQuad->mMaterialName = "Postprocess/Glass";
					passQuad->addQuadTextureSource(0, "rt_input");
				}
			}

			//Output channels
			glassDef->setNumOutputChannels(2);
			glassDef->mapOutputChannel(0, "rt_output");
			glassDef->mapOutputChannel(1, "rt_input");
		}

		if (!compositorManager->hasNodeDefinition("Motion Blur"))
		{
			/// Motion blur effect
			Ogre::CompositorNodeDef* motionBlurDef = compositorManager->addNodeDefinition("Motion Blur");

			//Input channels
			motionBlurDef->addTextureSourceName("rt_input", 0, Ogre::TextureDefinitionBase::TEXTURE_INPUT);
			motionBlurDef->addTextureSourceName("rt_output", 1, Ogre::TextureDefinitionBase::TEXTURE_INPUT);

			motionBlurDef->mCustomIdentifier = "Ogre/Postprocess";

			//Local textures
			motionBlurDef->setNumLocalTextureDefinitions(1);
			{
				Ogre::TextureDefinitionBase::TextureDefinition* texDef = motionBlurDef->addTextureDefinition("sum");
				texDef->width = 0;
				texDef->height = 0;
				texDef->format = Ogre::PFG_RGBA8_UNORM_SRGB;
				texDef->textureFlags &= (Ogre::uint32)~Ogre::TextureFlags::DiscardableContent;

				Ogre::RenderTargetViewDef* rtv = motionBlurDef->addRenderTextureView("sum");
				Ogre::RenderTargetViewEntry attachment;
				attachment.textureName = "sum";
				rtv->colourAttachments.push_back(attachment);
				rtv->depthBufferId = Ogre::DepthBuffer::POOL_NO_DEPTH;
			}

			motionBlurDef->setNumTargetPass(3);

			/// Initialisation pass for sum texture
			{
				Ogre::CompositorTargetDef* targetDef = motionBlurDef->addTargetPass("sum");
				{
					Ogre::CompositorPassQuadDef* passQuad;
					passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));
					passQuad->setAllLoadActions(Ogre::LoadAction::DontCare);
					passQuad->mNumInitialPasses = 1;
					passQuad->mMaterialName = "Ogre/Copy/4xFP32";
					passQuad->addQuadTextureSource(0, "rt_input");
				}
			}
			/// Do the motion blur
			{
				Ogre::CompositorTargetDef* targetDef = motionBlurDef->addTargetPass("rt_output");

				{
					Ogre::CompositorPassQuadDef* passQuad;
					passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));
					passQuad->setAllLoadActions(Ogre::LoadAction::DontCare);
					passQuad->mMaterialName = "Postprocess/Combine";
					passQuad->addQuadTextureSource(0, "rt_input");
					passQuad->addQuadTextureSource(1, "sum");
				}
			}
			/// Copy back sum texture for the next frame
			{
				Ogre::CompositorTargetDef* targetDef = motionBlurDef->addTargetPass("sum");

				{
					Ogre::CompositorPassQuadDef* passQuad;
					passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));
					passQuad->setAllLoadActions(Ogre::LoadAction::DontCare);
					passQuad->mMaterialName = "Ogre/Copy/4xFP32";
					passQuad->addQuadTextureSource(0, "rt_output");
				}
			}

			//Output channels
			motionBlurDef->setNumOutputChannels(2);
			motionBlurDef->mapOutputChannel(0, "rt_output");
			motionBlurDef->mapOutputChannel(1, "rt_input");
		}
	}

	void WorkspaceBaseComponent::createSSAONoiseTexture(void)
	{
		//We need to create SSAO kernel samples and noise texture
		//Generate kernel samples first
		float kernelSamples[64][4];
		for (size_t i = 0; i < 64u; ++i)
		{
			//            Ogre::Vector3 sample( 10, 10, 10 );
			//            while( sample.squaredLength() > 1.0f )
			//            {
			//                sample = Ogre::Vector3( Ogre::Math::RangeRandom(  -1.0f, 1.0f ),
			//                                        Ogre::Math::RangeRandom(  -1.0f, 1.0f ),
			//                                        Ogre::Math::RangeRandom(  0.01f, 1.0f ) );
			////                sample = Ogre::Vector3( Ogre::Math::RangeRandom(  -0.1f, 0.1f ),
			////                                        Ogre::Math::RangeRandom(  -0.1f, 0.1f ),
			////                                        Ogre::Math::RangeRandom(  0.5f, 1.0f ) );
			//            }
			Ogre::Vector3 sample = Ogre::Vector3(Ogre::Math::RangeRandom(-1.0f, 1.0f), Ogre::Math::RangeRandom(-1.0f, 1.0f), Ogre::Math::RangeRandom(0.0f, 1.0f));

			sample.normalise();

			float scale = (float)i / 64.0f;
			scale = Ogre::Math::lerp(0.3f, 1.0f, scale * scale);
			sample = sample * scale;

			kernelSamples[i][0] = sample.x;
			kernelSamples[i][1] = sample.y;
			kernelSamples[i][2] = sample.z;
			kernelSamples[i][3] = 1.0f;
		}

		//Next generate noise texture
		Ogre::Root* root = Core::getSingletonPtr()->getOgreRoot();
		Ogre::TextureGpuManager* textureManager = root->getRenderSystem()->getTextureGpuManager();
		Ogre::TextureGpu* noiseTexture = textureManager->createTexture("noiseTextureSSAO", Ogre::GpuPageOutStrategy::SaveToSystemRam, 0, Ogre::TextureTypes::Type2D);
		noiseTexture->setResolution(2u, 2u);
		noiseTexture->setPixelFormat(Ogre::PFG_RGBA8_SNORM);
		noiseTexture->_transitionTo(Ogre::GpuResidency::Resident, (Ogre::uint8*)0);
		noiseTexture->_setNextResidencyStatus(Ogre::GpuResidency::Resident);

		Ogre::StagingTexture* stagingTexture = textureManager->getStagingTexture(2u, 2u, 1u, 1u, Ogre::PFG_RGBA8_SNORM);
		stagingTexture->startMapRegion();
		Ogre::TextureBox texBox = stagingTexture->mapRegion(2u, 2u, 1u, 1u, Ogre::PFG_RGBA8_SNORM);

		for (size_t j = 0; j < texBox.height; ++j)
		{
			for (size_t i = 0; i < texBox.width; ++i)
			{
				Ogre::Vector3 noise = Ogre::Vector3(Ogre::Math::RangeRandom(-1.0f, 1.0f), Ogre::Math::RangeRandom(-1.0f, 1.0f), 0.0f);
				noise.normalise();

				Ogre::uint8* pixelData = reinterpret_cast<Ogre::uint8*>(texBox.at(i, j, 0));
				pixelData[0] = Ogre::Bitwise::floatToSnorm8(noise.x);
				pixelData[1] = Ogre::Bitwise::floatToSnorm8(noise.y);
				pixelData[2] = Ogre::Bitwise::floatToSnorm8(noise.z);
				pixelData[3] = Ogre::Bitwise::floatToSnorm8(1.0f);
			}
		}

		stagingTexture->stopMapRegion();
		stagingTexture->upload(texBox, noiseTexture, 0, 0, 0);
		textureManager->removeStagingTexture(stagingTexture);
		stagingTexture = 0;
		noiseTexture->notifyDataIsReady();

		//---------------------------------------------------------------------------------
		//Get GpuProgramParametersSharedPtr to set uniforms that we need
		Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().load("SSAO/HS", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME).staticCast<Ogre::Material>();

		Ogre::Pass* pass = material->getTechnique(0)->getPass(0);
		// this->ssaoPass = pass;
		Ogre::GpuProgramParametersSharedPtr psParams = pass->getFragmentProgramParameters();

		//Lets set uniforms for shader
		//Set texture uniform for noise
		Ogre::TextureUnitState* noiseTextureState = pass->getTextureUnitState("noiseTexture");
		noiseTextureState->setTexture(noiseTexture);

		//Reconstruct position from depth. Position is needed in SSAO
		//We need to set the parameters based on camera to the
		//shader so that the un-projection works as expected
		Ogre::Vector2 projectionAB = this->cameraComponent->getCamera()->getProjectionParamsAB();
		//The division will keep "linearDepth" in the shader in the [0; 1] range.
		projectionAB.y /= this->cameraComponent->getCamera()->getFarClipDistance();
		psParams->setNamedConstant("projectionParams", projectionAB);

		//Set other uniforms
		psParams->setNamedConstant("kernelRadius", /*mKernelRadius*/1.0f);

		psParams->setNamedConstant("noiseScale", Ogre::Vector2((Core::getSingletonPtr()->getOgreRenderWindow()->getWidth() * 0.5f) / 2.0f,
			(Core::getSingletonPtr()->getOgreRenderWindow()->getHeight() * 0.5f) / 2.0f));
		psParams->setNamedConstant("invKernelSize", 1.0f / 64.0f);
		psParams->setNamedConstant("sampleDirs", (float*)kernelSamples, 64, 4);

		//Set blur shader uniforms
		Ogre::MaterialPtr materialBlurH = Ogre::MaterialManager::getSingleton().load("SSAO/BlurH", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME).staticCast<Ogre::Material>();

		Ogre::Pass* passBlurH = materialBlurH->getTechnique(0)->getPass(0);
		Ogre::GpuProgramParametersSharedPtr psParamsBlurH = passBlurH->getFragmentProgramParameters();
		psParamsBlurH->setNamedConstant("projectionParams", projectionAB);

		Ogre::MaterialPtr materialBlurV = Ogre::MaterialManager::getSingleton().load("SSAO/BlurV", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME).staticCast<Ogre::Material>();

		Ogre::Pass* passBlurV = materialBlurV->getTechnique(0)->getPass(0);
		Ogre::GpuProgramParametersSharedPtr psParamsBlurV = passBlurV->getFragmentProgramParameters();
		psParamsBlurV->setNamedConstant("projectionParams", projectionAB);

		//Set apply shader uniforms
		Ogre::MaterialPtr materialApply = Ogre::MaterialManager::getSingleton().load("SSAO/Apply", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME).staticCast<Ogre::Material>();

		Ogre::Pass* passApply = materialApply->getTechnique(0)->getPass(0);
		// this->ssaoApplyPass = passApply;
		Ogre::GpuProgramParametersSharedPtr psParamsApply = passApply->getFragmentProgramParameters();
		psParamsApply->setNamedConstant("powerScale", /*mPowerScale*/ 1.5f);
	}

	void WorkspaceBaseComponent::initializeSmaa(PresetQuality quality, EdgeDetectionMode edgeDetectionMode)
	{
		const Ogre::RenderSystemCapabilities* caps = Ogre::Root::getSingletonPtr()->getRenderSystem()->getCapabilities();

		Ogre::String materialNames[3] =
		{
			"SMAA/EdgeDetection",
			"SMAA/BlendingWeightCalculation",
			"SMAA/NeighborhoodBlending"
		};

		Ogre::String preprocessorDefines = "SMAA_INITIALIZED=1,";

		switch (quality)
		{
		case SMAA_PRESET_LOW:
			preprocessorDefines += "SMAA_PRESET_LOW=1,";
			break;
		case SMAA_PRESET_MEDIUM:
			preprocessorDefines += "SMAA_PRESET_MEDIUM=1,";
			break;
		case SMAA_PRESET_HIGH:
			preprocessorDefines += "SMAA_PRESET_HIGH=1,";
			break;
		case SMAA_PRESET_ULTRA:
			preprocessorDefines += "SMAA_PRESET_ULTRA=1,";
			break;
		}

		//Actually these macros (EdgeDetectionMode) only
		//affect pixel shader SMAA/EdgeDetection_ps
		switch (edgeDetectionMode)
		{
		case EdgeDetectionDepth:
			OGRE_EXCEPT(Ogre::Exception::ERR_NOT_IMPLEMENTED,
				"EdgeDetectionDepth not implemented.",
				"SmaaUtils::initialize");
			break;
		case EdgeDetectionLuma:
			preprocessorDefines += "SMAA_EDGE_DETECTION_MODE=1,";
			break;
		case EdgeDetectionColor:
			preprocessorDefines += "SMAA_EDGE_DETECTION_MODE=2,";
			break;
		}

		if (caps->isShaderProfileSupported("ps_4_1"))
			preprocessorDefines += "SMAA_HLSL_4_1=1,";
		else if (caps->isShaderProfileSupported("ps_4_0"))
			preprocessorDefines += "SMAA_HLSL_4=1,";
		else if (caps->isShaderProfileSupported("glsl410") || caps->isShaderProfileSupported("glslvk"))
			preprocessorDefines += "SMAA_GLSL_4=1,";
		else if (caps->isShaderProfileSupported("glsl330"))
			preprocessorDefines += "SMAA_GLSL_3=1,";

		for (size_t i = 0; i < sizeof(materialNames) / sizeof(materialNames[0]); ++i)
		{
			Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().load(materialNames[i], Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME).staticCast<Ogre::Material>();

			Ogre::Pass* pass = material->getTechnique(0)->getPass(0);

			Ogre::GpuProgram* shader = 0;
			Ogre::GpuProgramParametersSharedPtr oldParams;

			//Save old manual & auto params
			oldParams = pass->getVertexProgramParameters();
			//Retrieve the HLSL/GLSL/Metal shader and rebuild it with the right settings.
			shader = pass->getVertexProgram()->_getBindingDelegate();
			shader->setParameter("preprocessor_defines", preprocessorDefines);
			pass->getVertexProgram()->reload();
			//Restore manual & auto params to the newly compiled shader
			pass->getVertexProgramParameters()->copyConstantsFrom(*oldParams);

			//Save old manual & auto params
			oldParams = pass->getFragmentProgramParameters();
			//Retrieve the HLSL/GLSL/Metal shader and rebuild it with the right settings.
			shader = pass->getFragmentProgram()->_getBindingDelegate();
			shader->setParameter("preprocessor_defines", preprocessorDefines);
			pass->getFragmentProgram()->reload();
			//Restore manual & auto params to the newly compiled shader
			pass->getFragmentProgramParameters()->copyConstantsFrom(*oldParams);
		}
	}

	void WorkspaceBaseComponent::resetReflectionForAllEntities(void)
	{
		Ogre::TextureGpuManager* hlmsTextureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();

		auto gameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects();
		for (auto& it = gameObjects->begin(); it != gameObjects->end(); ++it)
		{
			auto& gameObjectPtr = it->second;
			if (nullptr != gameObjectPtr)
			{
				if (true == gameObjectPtr->getUseReflection())
				{
					Ogre::v1::Entity* entity = gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
					if (nullptr != entity)
					{
						for (size_t i = 0; i < entity->getNumSubEntities(); i++)
						{
							Ogre::HlmsPbsDatablock* pbsDatablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(entity->getSubEntity(i)->getDatablock());
							if (nullptr != pbsDatablock)
							{
								// DynamicCubemap
								auto reflectionTexture = pbsDatablock->getTexture(Ogre::PbsTextureTypes::PBSM_REFLECTION);
								if (nullptr != reflectionTexture)
								{
									pbsDatablock->setTexture(static_cast<Ogre::uint8>(Ogre::PBSM_REFLECTION), nullptr);
									this->setDataBlockPbsReflectionTextureName(gameObjectPtr.get(), "");

									/*Ogre::TextureGpu* reflectionTexture = pbsDatablock->getTexture(static_cast<Ogre::uint8>(Ogre::PBSM_REFLECTION));
									if (nullptr != reflectionTexture)
									{
										hlmsTextureManager->destroyTexture(reflectionTexture);
									}*/
								}
							}
						}
					}
					else
					{
						Ogre::Item* item = gameObjectPtr->getMovableObject<Ogre::Item>();
						if (nullptr != item)
						{
							for (size_t i = 0; i < item->getNumSubItems(); i++)
							{
								Ogre::HlmsPbsDatablock* pbsDatablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(item->getSubItem(i)->getDatablock());
								if (nullptr != pbsDatablock)
								{
									// DynamicCubemap
									auto reflectionTexture = pbsDatablock->getTexture(Ogre::PbsTextureTypes::PBSM_REFLECTION);
									if (nullptr != reflectionTexture)
									{
										pbsDatablock->setTexture(static_cast<Ogre::uint8>(Ogre::PBSM_REFLECTION), nullptr);
										this->setDataBlockPbsReflectionTextureName(gameObjectPtr.get(), "");
									}
								}
							}
						}
					}
				}
			}
		}
	}

	void WorkspaceBaseComponent::setDataBlockPbsReflectionTextureName(GameObject* gameObject, const Ogre::String& textureName)
	{
		unsigned int i = 0;
		boost::shared_ptr<DatablockPbsComponent> datablockPbsCompPtr = nullptr;
		do
		{
			datablockPbsCompPtr = NOWA::makeStrongPtr(gameObject->getComponentWithOccurrence<DatablockPbsComponent>(i));
			if (nullptr != datablockPbsCompPtr)
			{
				datablockPbsCompPtr->setReflectionTextureName(textureName);
				i++;
			}
		} while (nullptr != datablockPbsCompPtr);
	}

	void WorkspaceBaseComponent::setPlanarMaxReflections(unsigned long gameObjectId, bool useAccurateLighting, unsigned int width, unsigned int height, bool withMipmaps, bool useMipmapMethodCompute,
		const Ogre::Vector3& position, const Ogre::Quaternion& orientation, const Ogre::Vector2& mirrorSize)
	{
		if (nullptr != this->planarReflections && false == this->planarReflectionReflectiveWorkspaceName.empty())
		{
			bool foundGameObjectId = false;
			unsigned int planarReflectionActorIndex = 1;
			for (size_t i = 0; i < this->planarReflectionActors.size(); i++)
			{
				if (std::get<0>(this->planarReflectionActors[i]) == gameObjectId)
				{
					foundGameObjectId = true;
					planarReflectionActorIndex = std::get<1>(this->planarReflectionActors[i]);
					Ogre::PlanarReflectionActor* actor = std::get<2>(this->planarReflectionActors[i]);
					actor->setPlane(position, mirrorSize, orientation);
				}
			}

			this->planarReflections->setMaxActiveActors(planarReflectionActorIndex, this->planarReflectionReflectiveWorkspaceName, useAccurateLighting, width, height, withMipmaps, Ogre::PFG_RGBA8_UNORM_SRGB, useMipmapMethodCompute);
		}
	}

	void WorkspaceBaseComponent::addPlanarReflectionsActor(unsigned long gameObjectId, bool useAccurateLighting, unsigned int width, unsigned int height, bool withMipmaps, bool useMipmapMethodCompute,
		const Ogre::Vector3& position, const Ogre::Quaternion& orientation, const Ogre::Vector2& mirrorSize)
	{
		if (nullptr != this->planarReflections && false == this->planarReflectionReflectiveWorkspaceName.empty())
		{
			bool foundGameObjectId = false;
			unsigned int planarReflectionActorIndex = 1;
			Ogre::PlanarReflectionActor* existingActor = nullptr;
			for (size_t i = 0; i < this->planarReflectionActors.size(); i++)
			{
				if (std::get<0>(this->planarReflectionActors[i]) == gameObjectId)
				{
					foundGameObjectId = true;
					planarReflectionActorIndex = std::get<1>(this->planarReflectionActors[i]);
					Ogre::PlanarReflectionActor* existingActor = std::get<2>(this->planarReflectionActors[i]);
					this->planarReflections->destroyActor(existingActor);
					// actor->setPlane(position, mirrorSize, orientation);

					Ogre::PlanarReflectionActor* actor = this->planarReflections->addActor(Ogre::PlanarReflectionActor(position, mirrorSize, orientation));

					// Make sure it's always activated (i.e. always win against other actors) unless it's not visible by the camera.
					actor->mActivationPriority = 0;
					this->planarReflectionActors[i] = std::make_tuple(gameObjectId, planarReflectionActorIndex, actor);
				}
			}

			if (false == foundGameObjectId)
			{
				bool foundPlanarReflectionActorIndex = false;
				for (size_t i = 0; i < this->planarReflectionActors.size(); i++)
				{
					if (std::get<1>(this->planarReflectionActors[i]) == planarReflectionActorIndex)
					{
						planarReflectionActorIndex++;
					}
				}

				Ogre::PlanarReflectionActor* actor = this->planarReflections->addActor(Ogre::PlanarReflectionActor(position, mirrorSize, orientation));
				
				// Make sure it's always activated (i.e. always win against other actors) unless it's not visible by the camera.
				actor->mActivationPriority = 0;
				this->planarReflectionActors.emplace_back(gameObjectId, planarReflectionActorIndex, actor);
			}
		}
	}

	void WorkspaceBaseComponent::removePlanarReflectionsActor(unsigned long gameObjectId)
	{
		bool couldRemove = false;
		for (size_t i = 0; i < this->planarReflectionActors.size(); i++)
		{
			if (std::get<0>(this->planarReflectionActors[i]) == gameObjectId)
			{
				Ogre::PlanarReflectionActor* actor = std::get<2>(this->planarReflectionActors[i]);
				this->planarReflections->destroyActor(actor);
				this->planarReflectionActors.erase(this->planarReflectionActors.begin() + i);
				couldRemove = true;
			}
		}

		if (true == couldRemove)
		{
			/*delete this->planarReflections;
			this->workspace->removeListener(this->planarReflectionsWorkspaceListener);
			this->pbs->setPlanarReflections(nullptr);

			delete this->planarReflectionsWorkspaceListener;

			this->planarReflections = new Ogre::PlanarReflections(this->gameObjectPtr->getSceneManager(), this->compositorManager, 1.0f, nullptr);
			this->planarReflectionsWorkspaceListener = new PlanarReflectionsWorkspaceListener(this->planarReflections);
			this->workspace->addListener(this->planarReflectionsWorkspaceListener);

			// Attention: Only one planar reflection setting is possible!
			this->pbs->setPlanarReflections(this->planarReflections);*/

			// this->createWorkspace();
		}
	}

	void WorkspaceBaseComponent::setBackgroundColor(const Ogre::Vector3& backgroundColor)
	{
		if (false == MathHelper::getInstance()->vector3Equals(backgroundColor, this->oldBackgroundColor))
		{
			this->backgroundColor->setValue(backgroundColor);
			this->oldBackgroundColor = backgroundColor;
		
			Ogre::ColourValue color(backgroundColor.x, backgroundColor.y, backgroundColor.z);
			this->changeBackgroundColor(color);
		}
	}

	Ogre::Vector3 WorkspaceBaseComponent::getBackgroundColor(void) const
	{
		return this->backgroundColor->getVector3();
	}

	void WorkspaceBaseComponent::setViewportRect(const Ogre::Vector4& viewportRect)
	{
		this->viewportRect->setValue(viewportRect);
		
		this->changeViewportRect(0, viewportRect);
	}

	Ogre::Vector4 WorkspaceBaseComponent::getViewportRect(void) const
	{
		return this->viewportRect->getVector4();
	}

	void WorkspaceBaseComponent::setSuperSampling(Ogre::Real superSampling)
	{
		if (superSampling <= 0.0f)
			superSampling = 1.0f;

		this->superSampling->setValue(superSampling);

		this->createWorkspace();
	}

	Ogre::Real WorkspaceBaseComponent::getSuperSampling(void) const
	{
		return this->superSampling->getReal();
	}

	void WorkspaceBaseComponent::setUseHdr(bool useHdr)
	{
		this->useHdr->setValue(useHdr);

		if (false == useHdr && nullptr != this->gameObjectPtr)
		{
			auto& hdrEffectCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<HdrEffectComponent>());
			if (nullptr != hdrEffectCompPtr)
			{
				// Reset hdr values
				hdrEffectCompPtr->applyHdrSkyColor(Ogre::ColourValue(0.2f, 0.4f, 0.6f), 1.0f);
				hdrEffectCompPtr->applyExposure(1.0f, 1.0f, 1.0f);
				hdrEffectCompPtr->applyBloomThreshold(0.0f, 0.0f);
			}
		}

		this->createWorkspace();
	}

	bool WorkspaceBaseComponent::getUseHdr(void) const
	{
		return this->useHdr->getBool();
	}

	void WorkspaceBaseComponent::setUseReflection(bool useReflection)
	{
		if (this->useReflection->getBool() == useReflection)
		{
			return;
		}
		this->useReflection->setValue(useReflection);

		this->reflectionCameraGameObjectId->setVisible(useReflection);

		if (false == useReflection)
		{
			this->canUseReflection = false;
			this->reflectionCameraGameObjectId->setValue(static_cast<unsigned long>(0));
		}

		this->createWorkspace();
	}

	bool WorkspaceBaseComponent::getUseReflection(void) const
	{
		return this->useReflection->getBool();
	}

	void WorkspaceBaseComponent::setUsePlanarReflection(bool usePlanarReflection)
	{
		this->usePlanarReflection->setValue(usePlanarReflection);

		bool hasAnyMirror = false;

		if (true == usePlanarReflection)
		{
			hasAnyMirror = this->hasAnyMirrorForPlanarReflections();
		}

		if (false == usePlanarReflection || hasAnyMirror)
		{
			this->createWorkspace();
		}
	}

	bool WorkspaceBaseComponent::getUsePlanarReflection(void) const
	{
		return this->usePlanarReflection->getBool();
	}

	void WorkspaceBaseComponent::setUseTerra(bool useTerra)
	{
		this->useTerra = useTerra;

		this->createWorkspace();
	}

	bool WorkspaceBaseComponent::getUseTerra(void) const
	{
		return this->useTerra;
	}

	void WorkspaceBaseComponent::setUseOcean(bool useOcean)
	{
		this->useOcean = useOcean;

		this->createWorkspace();
	}

	bool WorkspaceBaseComponent::getUseOcean(void) const
	{
		return this->useOcean;
	}

	void WorkspaceBaseComponent::setReflectionCameraGameObjectId(unsigned long reflectionCameraGameObjectId)
	{
		this->reflectionCameraGameObjectId->setValue(reflectionCameraGameObjectId);

		if (true == this->useReflection->getBool())
		{
			this->createWorkspace();
		}
	}

	unsigned long WorkspaceBaseComponent::getReflectionCameraGameObjectId(void) const
	{
		return this->reflectionCameraGameObjectId->getULong();
	}

	Ogre::String WorkspaceBaseComponent::getWorkspaceName(void) const
	{
		return this->workspaceName;
	}

	Ogre::String WorkspaceBaseComponent::getRenderingNodeName(void) const
	{
		return this->renderingNodeName;
	}

	Ogre::String WorkspaceBaseComponent::getFinalRenderingNodeName(void) const
	{
		return this->finalRenderingNodeName;
	}

	void WorkspaceBaseComponent::setWorkspace(Ogre::CompositorWorkspace* workspace)
	{
		this->workspace = workspace;
	}

	Ogre::CompositorWorkspace* WorkspaceBaseComponent::getWorkspace(void) const
	{
		return this->workspace;
	}

	Ogre::TextureGpu* WorkspaceBaseComponent::getDynamicCubemapTexture(void) const
	{
		return this->cubemap;
	}

	Ogre::PlanarReflections* WorkspaceBaseComponent::getPlanarReflections(void) const
	{
		return this->planarReflections;
	}

	void WorkspaceBaseComponent::setShadowGlobalBias(Ogre::Real shadowGlobalBias)
	{
		this->shadowGlobalBias->setValue(shadowGlobalBias); 
		this->updateShadowGlobalBias();
	}

	Ogre::Real WorkspaceBaseComponent::getShadowGlobalBias(void) const
	{
		return this->shadowGlobalBias->getReal();
	}

	void WorkspaceBaseComponent::setShadowGlobalNormalOffset(Ogre::Real shadowGlobalNormalOffset)
	{
		this->shadowGlobalNormalOffset->setValue(shadowGlobalNormalOffset);
		this->updateShadowGlobalBias();
	}

	Ogre::Real WorkspaceBaseComponent::getShadowGlobalNormalOffset(void) const
	{
		return this->shadowGlobalNormalOffset->getReal();
	}

	void WorkspaceBaseComponent::setShadowPSSMLambda(Ogre::Real shadowPssmLambda)
	{
		this->shadowPSSMLambda->setValue(shadowPssmLambda);
		this->updateShadowGlobalBias();
	}

	Ogre::Real WorkspaceBaseComponent::getShadowPSSMLambda(void) const
	{
		return this->shadowPSSMLambda->getReal();
	}

	void WorkspaceBaseComponent::setShadowSplitBlend(Ogre::Real shadowSplitBlend)
	{
		this->shadowSplitBlend->setValue(shadowSplitBlend);
		this->updateShadowGlobalBias();
	}

	Ogre::Real WorkspaceBaseComponent::getShadowSplitBlend(void) const
	{
		return this->shadowSplitBlend->getReal();
	}

	void WorkspaceBaseComponent::setShadowSplitFade(Ogre::Real shadowSplitFade)
	{
		this->shadowSplitFade->setValue(shadowSplitFade);
		this->updateShadowGlobalBias();
	}

	Ogre::Real WorkspaceBaseComponent::getShadowSplitFade(void) const
	{
		return this->shadowSplitFade->getReal();
	}

	void WorkspaceBaseComponent::setShadowSplitPadding(Ogre::Real shadowSplitPadding)
	{
		this->shadowSplitPadding->setValue(shadowSplitPadding);
		this->updateShadowGlobalBias();
	}

	Ogre::Real WorkspaceBaseComponent::getShadowSplitPadding(void) const
	{
		return this->shadowSplitPadding->getReal();
	}

	void WorkspaceBaseComponent::updateShadowGlobalBias(void)
	{
		Ogre::CompositorShadowNodeDef* node = this->compositorManager->getShadowNodeDefinitionNonConst(WorkspaceModule::getInstance()->shadowNodeName);
		size_t numShadowDefinitions = node->getNumShadowTextureDefinitions();
		for (size_t i = 0; i < numShadowDefinitions; i++)
		{
			Ogre::ShadowTextureDefinition* texture = node->getShadowTextureDefinitionNonConst(i);
#if 1
			texture->normalOffsetBias = this->shadowGlobalNormalOffset->getReal();
			texture->constantBiasScale = this->shadowGlobalBias->getReal();
			texture->pssmLambda = this->shadowPSSMLambda->getReal();
			texture->splitBlend = this->shadowSplitBlend->getReal();
			texture->splitFade = this->shadowSplitFade->getReal();
			texture->splitPadding = this->shadowSplitPadding->getReal();
			texture->autoConstantBiasScale = 100.0f;
			texture->autoNormalOffsetBiasScale = 4.0f;
			// texture->numStableSplits = 3;
#endif
			/*
			Edit: Got good results with setting the depth bias on the material between 0.00050 > 0.0010
			Got a good result with normal offset at value: 0.0000040
			Doesnt look too disimilar to what i was playing with my version.
			Shadows also scale well with the object size, my advice check the shadowmap bias on the materials itself.
			*/

			/*
			texture->autoConstantBiasScale = 100.0f;
			texture->autoNormalOffsetBiasScale = 4.0f;
			*/
		}
	}

	bool WorkspaceBaseComponent::hasAnyMirrorForPlanarReflections(void)
	{
		bool hasAnyMirror = false;

		auto gameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects();

		for (auto& it = gameObjects->begin(); it != gameObjects->end(); ++it)
		{
			auto& gameObjectPtr = it->second;
			if (nullptr != gameObjectPtr)
			{

				auto planarReflectionCompPtr = NOWA::makeStrongPtr(gameObjectPtr->getComponent<PlanarReflectionComponent>());
				if (nullptr != planarReflectionCompPtr)
				{
					hasAnyMirror = true;
					break;
				}
			}
		}

		return hasAnyMirror;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	WorkspacePbsComponent::WorkspacePbsComponent()
		: WorkspaceBaseComponent()
	{
		
	}

	WorkspacePbsComponent::~WorkspacePbsComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[WorkspacePbsComponent] Destructor workspace pbs component for game object: " + this->gameObjectPtr->getName());
	}

	bool WorkspacePbsComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = WorkspaceBaseComponent::init(propertyElement, filename);

		return success;
	}

	bool WorkspacePbsComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[WorkspacePbsComponent] Init workspace pbs component for game object: " + this->gameObjectPtr->getName());

		bool success = WorkspaceBaseComponent::postInit();

		return success;
	}

	void WorkspacePbsComponent::actualizeValue(Variant* attribute)
	{
		WorkspaceBaseComponent::actualizeValue(attribute);

	}

	void WorkspacePbsComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		WorkspaceBaseComponent::writeXML(propertiesXML, doc, filePath);
	}

	Ogre::String WorkspacePbsComponent::getClassName(void) const
	{
		return "WorkspacePbsComponent";
	}

	Ogre::String WorkspacePbsComponent::getParentClassName(void) const
	{
		return "WorkspaceBaseComponent";
	}

	void WorkspacePbsComponent::internalCreateCompositorNode(void)
	{
		// https://forums.ogre3d.org/viewtopic.php?t=94748
		if (!this->compositorManager->hasNodeDefinition(this->renderingNodeName))
		{
			Ogre::CompositorNodeDef* compositorNodeDefinition = compositorManager->addNodeDefinition(this->renderingNodeName);
#ifdef TERRA_OLD
			if (true == this->useTerra)
			{
				compositorNodeDefinition->addTextureSourceName("TerraShadowTexture", 0, Ogre::TextureDefinitionBase::TEXTURE_INPUT);
			}
#endif

			unsigned short numTexturesDefinitions = 0;
			
			if (false == this->useHdr->getBool())
			{
				numTexturesDefinitions = 2;
			}
			else
			{
				numTexturesDefinitions = 3;
			}

			compositorNodeDefinition->setNumLocalTextureDefinitions(numTexturesDefinitions);

			Ogre::TextureDefinitionBase::TextureDefinition* texDef = compositorNodeDefinition->addTextureDefinition("rt0");
			texDef->width = 0; // target_width
			texDef->height = 0; // target_height

			if (this->superSampling->getReal() <= 0.0f)
				this->superSampling->setValue(1.0f);

			texDef->widthFactor = this->superSampling->getReal();
			texDef->heightFactor = this->superSampling->getReal();
			texDef->format = Ogre::PFG_RGBA16_FLOAT;
			// texDef->msaa = this->msaaLevel;
			texDef->textureFlags = Ogre::TextureFlags::RenderToTexture | Ogre::TextureFlags::MsaaExplicitResolve;

			Ogre::RenderTargetViewDef* rtv = compositorNodeDefinition->addRenderTextureView("rt0");
			Ogre::RenderTargetViewEntry attachment;
			attachment.textureName = "rt0";
			rtv->colourAttachments.push_back(attachment);
			rtv->depthBufferId = Ogre::DepthBuffer::POOL_DEFAULT;

			texDef = compositorNodeDefinition->addTextureDefinition("rt1");
			texDef->width = 0; // target_width
			texDef->height = 0; // target_height
			texDef->widthFactor = this->superSampling->getReal();
			texDef->heightFactor = this->superSampling->getReal();
			texDef->format = Ogre::PFG_RGBA16_FLOAT;
			// texDef->msaa = this->msaaLevel;
			texDef->textureFlags = Ogre::TextureFlags::RenderToTexture | Ogre::TextureFlags::MsaaExplicitResolve;

			rtv = compositorNodeDefinition->addRenderTextureView("rt1");
			attachment.textureName = "rt1";
			rtv->colourAttachments.push_back(attachment);
			// Note: When set to POOL_NO_DEPTH, no renderqueues are used, everything is rendered without death!
			rtv->depthBufferId = Ogre::DepthBuffer::POOL_DEFAULT;

			if (true == this->useHdr->getBool())
			{
				texDef = compositorNodeDefinition->addTextureDefinition("oldLumRt");
				texDef->width = 1;
				texDef->height = 1;
				texDef->format = Ogre::PFG_RGBA16_FLOAT;
				// ?? Is this necessary?
				texDef->textureFlags = Ogre::TextureFlags::RenderToTexture; // Here also | Ogre::TextureFlags::MsaaExplicitResolve;??
				// texDef->depthBufferId = Ogre::DepthBuffer::POOL_DEFAULT;

				rtv = compositorNodeDefinition->addRenderTextureView("oldLumRt");
				attachment.textureName = "oldLumRt";
				rtv->colourAttachments.push_back(attachment);
				// Compositor script: depth_pool 0  ?
				rtv->depthBufferId = Ogre::DepthBuffer::POOL_NO_DEPTH;
			}

			unsigned short numTargetPass = 0;

			if (false == this->useHdr->getBool())
			{
				numTargetPass = 1;
				compositorNodeDefinition->setNumTargetPass(numTargetPass);
			}
			else
			{
				numTargetPass = 2;
				compositorNodeDefinition->setNumTargetPass(numTargetPass);
			}

			{
				Ogre::CompositorTargetDef* targetDef = compositorNodeDefinition->addTargetPass("rt0");

				{
					// Render Scene
					{
						Ogre::CompositorPassSceneDef* passScene;
						passScene = static_cast<Ogre::CompositorPassSceneDef*>(targetDef->addPass(Ogre::PASS_SCENE));

						if (true == this->canUseReflection)
						{
							//Our materials in this pass will be using this cubemap,
							//so we need to expose it to the pass.
							//Note: Even if it "just works" without exposing, the reason for
							//exposing is to ensure compatibility with Vulkan & D3D12.

							passScene->mExposedTextures.emplace_back(Ogre::IdString("rt1"));
						}
#ifdef TERRA_OLD
						if (true == this->useTerra)
						{
							passScene->mExposedTextures.emplace_back(Ogre::IdString("TerraShadowTexture"));
						}
#endif
						Ogre::ColourValue color(this->backgroundColor->getVector3().x, this->backgroundColor->getVector3().y, this->backgroundColor->getVector3().z);
						// passScene->setAllClearColours(color);
						passScene->setAllLoadActions(Ogre::LoadAction::Clear);
						passScene->mClearColour[0] = color;

						// passScene->setAllStoreActions(Ogre::StoreAction::StoreOrResolve);
						passScene->mStoreActionColour[0] = Ogre::StoreAction::StoreOrResolve; // Ogre::StoreAction::StoreAndMultisampleResolve; causes a crash, why?
						passScene->mStoreActionDepth = Ogre::StoreAction::DontCare;
						passScene->mClearStencil = Ogre::StoreAction::DontCare;

						// passScene->mFirstRQ = 10;
						// passScene->mLastRQ = 253;

						if (true == this->usePlanarReflection->getBool())
						{
							// Used to identify this pass wants planar reflections
							passScene->mIdentifier = 25001;
						}

						passScene->mIncludeOverlays = false;

						//https://forums.ogre3d.org/viewtopic.php?t=93636
						//https://forums.ogre3d.org/viewtopic.php?t=94748

						// Note: Terra uses also this shadow node, so it works for all kinds of lights
						passScene->mShadowNode = WorkspaceModule::getInstance()->shadowNodeName;
					}
				}

				if (true == this->useHdr->getBool())
				{
					Ogre::CompositorTargetDef* targetDef = compositorNodeDefinition->addTargetPass("oldLumRt");

					{
						Ogre::CompositorPassClearDef* passClear;
						passClear = static_cast<Ogre::CompositorPassClearDef*>(targetDef->addPass(Ogre::PASS_CLEAR));
						passClear->mNumInitialPasses = 1;
						passClear->mClearColour[0] = Ogre::ColourValue(0.01f, 0.01f, 0.01f, 1.0f);
					}
				}
			}

			//Output channels
			unsigned short outputChannel = 0;

			if (false == this->useHdr->getBool())
			{
				outputChannel = 2;
				compositorNodeDefinition->setNumOutputChannels(outputChannel);
			}
			else
			{
				outputChannel = 3;
				compositorNodeDefinition->setNumOutputChannels(outputChannel);
			}

			compositorNodeDefinition->mapOutputChannel(0, "rt0");
			compositorNodeDefinition->mapOutputChannel(1, "rt1");

			if (true == this->useHdr->getBool())
			{
				compositorNodeDefinition->mapOutputChannel(2, "oldLumRt");
			}

		}

		this->setBackgroundColor(this->backgroundColor->getVector3());
		// For VR: disable this line and use NOWAPbsRenderingNodeVR
		this->changeViewportRect(0, this->viewportRect->getVector4());

		if (true == this->externalChannels.empty())
		{
			this->externalChannels.resize(1);
			this->externalChannels[0] = Core::getSingletonPtr()->getOgreRenderWindow()->getTexture();
		}

		if (true == this->usePlanarReflection->getBool())
		{
			if (false == this->compositorManager->hasNodeDefinition(this->planarReflectionReflectiveRenderingNode))
			{
				Ogre::CompositorNodeDef* compositorNodeDefinition = compositorManager->addNodeDefinition(this->planarReflectionReflectiveRenderingNode);

				compositorNodeDefinition->addTextureSourceName("rt_renderwindow", 0, Ogre::TextureDefinitionBase::TEXTURE_INPUT);

				unsigned short numTargetPass = 1;
				compositorNodeDefinition->setNumTargetPass(numTargetPass);

				{
					Ogre::CompositorTargetDef* targetDef = compositorNodeDefinition->addTargetPass("rt_renderwindow");

					{
						// Render Scene
						{
							Ogre::CompositorPassSceneDef* passScene;
							passScene = static_cast<Ogre::CompositorPassSceneDef*>(targetDef->addPass(Ogre::PASS_SCENE));

							Ogre::ColourValue color(this->backgroundColor->getVector3().x, this->backgroundColor->getVector3().y, this->backgroundColor->getVector3().z);
							// passScene->setAllClearColours(color);
							passScene->setAllLoadActions(Ogre::LoadAction::Clear);
							passScene->mClearColour[0] = color;

							// passScene->setAllStoreActions(Ogre::StoreAction::StoreOrResolve);
							passScene->mStoreActionColour[0] = Ogre::StoreAction::StoreOrResolve; // Ogre::StoreAction::StoreAndMultisampleResolve; causes a crash, why?
							passScene->mStoreActionDepth = Ogre::StoreAction::DontCare;
							passScene->mClearStencil = Ogre::StoreAction::DontCare;

							// passScene->mFirstRQ = 10;
							// passScene->mLastRQ = 253;

							passScene->mIncludeOverlays = false;

							passScene->mVisibilityMask = 0xfffffffe;

							//https://forums.ogre3d.org/viewtopic.php?t=93636
							//https://forums.ogre3d.org/viewtopic.php?t=94748
							passScene->mShadowNode = WorkspaceModule::getInstance()->shadowNodeName;
						}

						// Generate Mipmaps
						{
							Ogre::CompositorPassMipmapDef* passMipmap;
							passMipmap = static_cast<Ogre::CompositorPassMipmapDef*>(targetDef->addPass(Ogre::PASS_MIPMAP));

							passMipmap->mMipmapGenerationMethod = Ogre::CompositorPassMipmapDef::ComputeHQ;
						}
					}
				}
			}
		}
	}

	bool WorkspacePbsComponent::internalCreateWorkspace(Ogre::CompositorWorkspaceDef* workspaceDef)
	{
		// Without Hdr
		if (false == this->useHdr->getBool())
		{
			workspaceDef->connect(this->renderingNodeName, 0, this->finalRenderingNodeName, 1);
			workspaceDef->connectExternal(0, this->finalRenderingNodeName, 0);
		}
		else
		{
			if (1u == this->msaaLevel)
			{
				workspaceDef->connect(this->renderingNodeName, 0, "NOWAHdrPostprocessingNode", 0);
				workspaceDef->connect(this->renderingNodeName, 2, "NOWAHdrPostprocessingNode", 1);
				workspaceDef->connectExternal(0, "NOWAHdrPostprocessingNode", 2);

				workspaceDef->connect("NOWAHdrPostprocessingNode", 0, this->finalRenderingNodeName, 0);
			}
			else
			{
				workspaceDef->connect(this->renderingNodeName, 0, "NOWAHdrMsaaResolve", 0);
				workspaceDef->connect(this->renderingNodeName, 2, "NOWAHdrMsaaResolve", 1);

				workspaceDef->connect("NOWAHdrMsaaResolve", 0, "NOWAHdrPostprocessingNode", 0);
				workspaceDef->connect(this->renderingNodeName, 2, "NOWAHdrPostprocessingNode", 1);
				workspaceDef->connectExternal(0, "NOWAHdrPostprocessingNode", 2);

				workspaceDef->connect("NOWAHdrPostprocessingNode", 0, this->finalRenderingNodeName, 0);
			}
		}
#ifdef TERRA_OLD
		if (true == this->useTerra)
		{
			workspaceDef->connectExternal(1, this->renderingNodeName, 0);
		}
#endif
		if (true == this->externalChannels.empty())
		{
			this->externalChannels.resize(1);
			this->externalChannels[0] = Core::getSingletonPtr()->getOgreRenderWindow()->getTexture();
		}

		/*this->workspace = WorkspaceModule::getInstance()->getCompositorManager()->addWorkspace(this->gameObjectPtr->getSceneManager(), externalChannels,
			this->cameraComponent->getCamera(), this->workspaceName, true, -1, (Ogre::UavBufferPackedVec*)0,
			&this->initialCubemapLayouts, &this->initialCubemapUavAccess);*/

		this->workspace = WorkspaceModule::getInstance()->getCompositorManager()->addWorkspace(this->gameObjectPtr->getSceneManager(), externalChannels,
			this->cameraComponent->getCamera(), this->workspaceName, true);

		this->setBackgroundColor(this->backgroundColor->getVector3());
		// For VR: disable this line and use NOWAPbsRenderingNodeVR
		this->changeViewportRect(0, this->viewportRect->getVector4());

		// WorkspaceModule::getInstance()->setPrimaryWorkspace(this->gameObjectPtr->getSceneManager(), this->cameraComponent->getCamera(), this);

		if (true == this->useHdr->getBool())
		{
			this->initializeHdr(this->msaaLevel);
		}

		// this->createSSAONoiseTexture();

		return nullptr != this->workspace;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	WorkspaceSkyComponent::WorkspaceSkyComponent()
		: WorkspaceBaseComponent(),
		skyBoxName(new Variant(WorkspaceSkyComponent::AttrSkyBoxName(), this->attributes))
	{
		Ogre::StringVectorPtr skyNames = Ogre::ResourceGroupManager::getSingleton().findResourceNames("Skies", "*.dds");
		std::vector<Ogre::String> compatibleSkyNames(skyNames.getPointer()->size() + 1);
		unsigned int i = 0;
		for (auto& it = skyNames.getPointer()->cbegin(); it != skyNames.getPointer()->cend(); it++)
		{
			compatibleSkyNames[i++] = *it;
		}
		this->skyBoxName->setValue(compatibleSkyNames);
	}

	WorkspaceSkyComponent::~WorkspaceSkyComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[WorkspaceSkyComponent] Destructor workspace sky component for game object: " + this->gameObjectPtr->getName());
	}

	bool WorkspaceSkyComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = WorkspaceBaseComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SkyBoxName")
		{
			this->skyBoxName->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		return success;
	}

	bool WorkspaceSkyComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[WorkspaceSkyComponent] Init workspace sky component for game object: " + this->gameObjectPtr->getName());

		bool success = WorkspaceBaseComponent::postInit();

		return success;
	}

	void WorkspaceSkyComponent::actualizeValue(Variant* attribute)
	{
		WorkspaceBaseComponent::actualizeValue(attribute);

		if (WorkspaceSkyComponent::AttrSkyBoxName() == attribute->getName())
		{
			this->setSkyBoxName(attribute->getListSelectedValue());
		}
	}

	void WorkspaceSkyComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		WorkspaceBaseComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SkyBoxName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->skyBoxName->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String WorkspaceSkyComponent::getClassName(void) const
	{
		return "WorkspaceSkyComponent";
	}

	Ogre::String WorkspaceSkyComponent::getParentClassName(void) const
	{
		return "WorkspaceBaseComponent";
	}

	void WorkspaceSkyComponent::internalCreateCompositorNode(void)
	{
		if (!this->compositorManager->hasNodeDefinition(this->renderingNodeName))
		{
			Ogre::CompositorNodeDef* compositorNodeDefinition = this->compositorManager->addNodeDefinition(this->renderingNodeName);

			//Input channels (has none)
			// compositorNodeDefinition->addTextureSourceName("rt0", 0, Ogre::TextureDefinitionBase::TEXTURE_INPUT);
			// compositorNodeDefinition->addTextureSourceName("rt1", 1, Ogre::TextureDefinitionBase::TEXTURE_INPUT);
				/*{
					Ogre::CompositorPassQuadDef* passQuad;
					passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));
					passQuad->mMaterialName = "Postprocess/Glass";
					passQuad->addQuadTextureSource(0, "rt_input");
				}*/
#ifdef TERRA_OLD
			if (true == this->useTerra)
			{
				compositorNodeDefinition->addTextureSourceName("TerraShadowTexture", 0, Ogre::TextureDefinitionBase::TEXTURE_INPUT);
			}
#endif
			if (false == this->useHdr->getBool())
			{
				compositorNodeDefinition->setNumLocalTextureDefinitions(2);
			}
			else
			{
				compositorNodeDefinition->setNumLocalTextureDefinitions(3);
			}

			Ogre::TextureDefinitionBase::TextureDefinition* texDef = compositorNodeDefinition->addTextureDefinition("rt0");

			if (this->superSampling->getReal() <= 0.0f)
				this->superSampling->setValue(1.0f);

			texDef->width = 0; // target_width
			texDef->height = 0; // target_height
			texDef->widthFactor = this->superSampling->getReal();
			texDef->heightFactor = this->superSampling->getReal();
			texDef->format = Ogre::PFG_RGBA16_FLOAT;
			// texDef->format = Ogre::PFG_RGBA8_UNORM_SRGB;
			// texDef->msaa = this->msaaLevel;
			texDef->textureFlags = Ogre::TextureFlags::RenderToTexture | Ogre::TextureFlags::MsaaExplicitResolve;

			Ogre::RenderTargetViewDef* rtv = compositorNodeDefinition->addRenderTextureView("rt0");
			Ogre::RenderTargetViewEntry attachment;
			attachment.textureName = "rt0";
			rtv->colourAttachments.push_back(attachment);
			rtv->depthBufferId = Ogre::DepthBuffer::POOL_DEFAULT;

			texDef = compositorNodeDefinition->addTextureDefinition("rt1");
			texDef->width = 0; // target_width
			texDef->height = 0; // target_height
			texDef->widthFactor = this->superSampling->getReal();
			texDef->heightFactor = this->superSampling->getReal();
			texDef->format = Ogre::PFG_RGBA16_FLOAT;
			// texDef->format = Ogre::PFG_RGBA8_UNORM_SRGB;
			// texDef->msaa = this->msaaLevel;
			texDef->textureFlags = Ogre::TextureFlags::RenderToTexture | Ogre::TextureFlags::MsaaExplicitResolve;

			rtv = compositorNodeDefinition->addRenderTextureView("rt1");
			attachment.textureName = "rt1";
			rtv->colourAttachments.push_back(attachment);
			rtv->depthBufferId = Ogre::DepthBuffer::POOL_DEFAULT;

			if (true == this->useHdr->getBool())
			{
				texDef = compositorNodeDefinition->addTextureDefinition("oldLumRt");
				texDef->width = 1;
				texDef->height = 1;
				texDef->format = Ogre::PFG_RGBA16_FLOAT;
				texDef->textureFlags = Ogre::TextureFlags::RenderToTexture;
				texDef->depthBufferId = Ogre::DepthBuffer::POOL_DEFAULT;

				rtv = compositorNodeDefinition->addRenderTextureView("oldLumRt");
				attachment.textureName = "oldLumRt";
				rtv->colourAttachments.push_back(attachment);
				rtv->depthBufferId = Ogre::DepthBuffer::POOL_NO_DEPTH;
			}

			if (false == this->useHdr->getBool())
			{
				compositorNodeDefinition->setNumTargetPass(1);
			}
			else
			{
				compositorNodeDefinition->setNumTargetPass(2);
			}

			{
				Ogre::CompositorTargetDef* targetDef = compositorNodeDefinition->addTargetPass("rt0");
				{
					// Render Scene
					{
						Ogre::CompositorPassSceneDef* passScene;
						passScene = static_cast<Ogre::CompositorPassSceneDef*>(targetDef->addPass(Ogre::PASS_SCENE));

						Ogre::ColourValue color(0.2f, 0.4f, 0.6f);
						// passScene->setAllClearColours(color);
						passScene->setAllLoadActions(Ogre::LoadAction::Clear);
						passScene->mClearColour[0] = color;

						// passScene->setAllStoreActions(Ogre::StoreAction::StoreOrResolve);
						passScene->mStoreActionColour[0] = Ogre::StoreAction::StoreOrResolve; // Ogre::StoreAction::StoreAndMultisampleResolve; causes a crash, why?
						passScene->mStoreActionDepth = Ogre::StoreAction::DontCare;
						passScene->mClearStencil = Ogre::StoreAction::DontCare;

// Attention: There is another place with shadow!
						passScene->mShadowNode = WorkspaceModule::getInstance()->shadowNodeName;

						passScene->mFirstRQ = 0;
						passScene->mLastRQ = 2;

						if (true == this->usePlanarReflection->getBool())
						{
							// Used to identify this pass wants planar reflections
							passScene->mIdentifier = 25001;
						}

						// passScene->setAllStoreActions(Ogre::StoreAction::StoreOrResolve);
						/*passScene->mStoreActionColour[0] = Ogre::StoreAction::StoreOrResolve;
						*/

						if (true == this->canUseReflection)
						{
							//Our materials in this pass will be using this cubemap,
							//so we need to expose it to the pass.
							//Note: Even if it "just works" without exposing, the reason for
							//exposing is to ensure compatibility with Vulkan & D3D12.

							passScene->mExposedTextures.emplace_back(Ogre::IdString("rt1"));
						}
#ifdef TERRA_OLD
						if (true == this->useTerra)
						{
							passScene->mExposedTextures.emplace_back(Ogre::IdString("TerraShadowTexture"));
						}
#endif
						passScene->mIncludeOverlays = false;
					}

					//https://forums.ogre3d.org/viewtopic.php?t=93636
					//https://forums.ogre3d.org/viewtopic.php?t=94748

					// Sky quad
					{
						Ogre::CompositorPassQuadDef* passQuad;
						passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));
						passQuad->mMaterialName = "NOWASkyPostprocess";
						passQuad->mFrustumCorners = Ogre::CompositorPassQuadDef::CAMERA_DIRECTION;
					}

					// Render Scene
					{
						Ogre::CompositorPassSceneDef* passScene;
						passScene = static_cast<Ogre::CompositorPassSceneDef*>(targetDef->addPass(Ogre::PASS_SCENE));
						passScene->mIncludeOverlays = false;
						passScene->mShadowNode = WorkspaceModule::getInstance()->shadowNodeName;
						passScene->mShadowNodeRecalculation = Ogre::ShadowNodeRecalculation::SHADOW_NODE_REUSE;
						passScene->mFirstRQ = 2;
					}
				}

				if (true == this->useHdr->getBool())
				{
					Ogre::CompositorTargetDef* targetDef = compositorNodeDefinition->addTargetPass("oldLumRt");

					{
						Ogre::CompositorPassClearDef* passClear;
						passClear = static_cast<Ogre::CompositorPassClearDef*>(targetDef->addPass(Ogre::PASS_CLEAR));
						passClear->mNumInitialPasses = 1;
						passClear->mClearColour[0] = Ogre::ColourValue(0.01f, 0.01f, 0.01f, 1.0f);
					}
				}
			}

			//Output channels

			if (false == this->useHdr->getBool())
			{
				compositorNodeDefinition->setNumOutputChannels(2);
			}
			else
			{
				compositorNodeDefinition->setNumOutputChannels(3);
			}

			compositorNodeDefinition->mapOutputChannel(0, "rt0");
			compositorNodeDefinition->mapOutputChannel(1, "rt1");

			if (true == this->useHdr->getBool())
			{
				compositorNodeDefinition->mapOutputChannel(2, "oldLumRt");
			}

			// For VR: disable this line and use NOWAPbsRenderingNodeVR
			this->changeViewportRect(0, this->viewportRect->getVector4());

			if (true == this->externalChannels.empty())
			{
				this->externalChannels.resize(1);
				this->externalChannels[0] = Core::getSingletonPtr()->getOgreRenderWindow()->getTexture();
			}
		}

		if (true == this->usePlanarReflection->getBool())
		{
			if (false == this->compositorManager->hasNodeDefinition(this->planarReflectionReflectiveRenderingNode))
			{
				Ogre::CompositorNodeDef* compositorNodeDefinition = compositorManager->addNodeDefinition(this->planarReflectionReflectiveRenderingNode);

				compositorNodeDefinition->addTextureSourceName("rt_renderwindow", 0, Ogre::TextureDefinitionBase::TEXTURE_INPUT);

				unsigned short numTargetPass = 1;
				compositorNodeDefinition->setNumTargetPass(numTargetPass);

				{
					Ogre::CompositorTargetDef* targetDef = compositorNodeDefinition->addTargetPass("rt_renderwindow");

					{
						// Render Scene
						{
							Ogre::CompositorPassSceneDef* passScene;
							passScene = static_cast<Ogre::CompositorPassSceneDef*>(targetDef->addPass(Ogre::PASS_SCENE));

							Ogre::ColourValue color(0.2f, 0.4f, 0.6f);
							// passScene->setAllClearColours(color);
							passScene->setAllLoadActions(Ogre::LoadAction::Clear);
							passScene->mClearColour[0] = color;

							// passScene->setAllStoreActions(Ogre::StoreAction::StoreOrResolve);
							passScene->mStoreActionColour[0] = Ogre::StoreAction::StoreOrResolve; // Ogre::StoreAction::StoreAndMultisampleResolve; causes a crash, why?
							passScene->mStoreActionDepth = Ogre::StoreAction::DontCare;
							passScene->mClearStencil = Ogre::StoreAction::DontCare;

							passScene->mFirstRQ = 0;
							passScene->mLastRQ = 2;

							passScene->mIncludeOverlays = false;

							passScene->mVisibilityMask = 0xfffffffe;

							//https://forums.ogre3d.org/viewtopic.php?t=93636
							//https://forums.ogre3d.org/viewtopic.php?t=94748
							passScene->mShadowNode = WorkspaceModule::getInstance()->shadowNodeName;
						}

						// Sky quad
						{
							Ogre::CompositorPassQuadDef* passQuad;
							passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));
							passQuad->mMaterialName = "NOWASkyPostprocess";
							passQuad->mFrustumCorners = Ogre::CompositorPassQuadDef::CAMERA_DIRECTION;
						}

						// Render Scene
						{
							Ogre::CompositorPassSceneDef* passScene;
							passScene = static_cast<Ogre::CompositorPassSceneDef*>(targetDef->addPass(Ogre::PASS_SCENE));
							passScene->mIncludeOverlays = false;
							passScene->mShadowNode = WorkspaceModule::getInstance()->shadowNodeName;
							passScene->mShadowNodeRecalculation = Ogre::ShadowNodeRecalculation::SHADOW_NODE_REUSE;
							passScene->mFirstRQ = 2;
						}

						// Generate Mipmaps
						{
							Ogre::CompositorPassMipmapDef* passMipmap;
							passMipmap = static_cast<Ogre::CompositorPassMipmapDef*>(targetDef->addPass(Ogre::PASS_MIPMAP));

							passMipmap->mMipmapGenerationMethod = Ogre::CompositorPassMipmapDef::ComputeHQ;
						}
					}
				}
			}
		}
	}

	bool WorkspaceSkyComponent::internalCreateWorkspace(Ogre::CompositorWorkspaceDef* workspaceDef)
	{
		// Normal

#if 0
		workspaceDef->connect("NOWASkyRenderingNode", 0, "NOWAPbsFinalCompositionNode", 1);
		workspaceDef->connectExternal(0, "NOWAPbsFinalCompositionNode", 0);
#endif

		// Normal with glass

#if 0
		// Note: Glass must be enabled: it->second->setStartEnabled(true);
		workspaceDef->connect("NOWASkyRenderingNode", "Glass");
		workspaceDef->connectExternal(0, "NOWAPbsFinalCompositionNode", 0);
		workspaceDef->connect("Glass", 0, "NOWAPbsFinalCompositionNode", 1);
#endif

#if 0
		// SMAA
		// Later set for all workspaces and set for configuration via component?
		workspaceDef->connect("NOWASkyRenderingNode", 0, "SmaaNode", 0);
		workspaceDef->connect("NOWASkyRenderingNode", 1, "SmaaNode", 1);

		workspaceDef->connectExternal(0, "NOWAPbsFinalCompositionNode", 0);
		workspaceDef->connect("SmaaNode", 0, "NOWAPbsFinalCompositionNode", 1);

		this->workspace = WorkspaceModule::getInstance()->getCompositorManager()->addWorkspace(sceneManager, Core::getSingletonPtr()->getOgreRenderWindow()->getTexture(), camera, workspaceName, true);

		this->initializeSmaa(WorkspaceModule::SMAA_PRESET_ULTRA, WorkspaceModule::EdgeDetectionColour);
#endif

		this->changeViewportRect(0, this->viewportRect->getVector4());

		if (false == this->useHdr->getBool())
		{
			workspaceDef->connect(this->renderingNodeName, 0, this->finalRenderingNodeName, 1);
			workspaceDef->connectExternal(0, this->finalRenderingNodeName, 0);
		}
		else
		{
			// Hdr with sky
			if (1 == this->msaaLevel)
			{
				workspaceDef->connect(this->renderingNodeName, 0, "NOWAHdrPostprocessingNode", 0);
				workspaceDef->connect(this->renderingNodeName, 2, "NOWAHdrPostprocessingNode", 1);
				workspaceDef->connectExternal(0, "NOWAHdrPostprocessingNode", 2);

				workspaceDef->connect("NOWAHdrPostprocessingNode", 0, this->finalRenderingNodeName, 0);
			}
			else
			{
				workspaceDef->connect(this->renderingNodeName, 0, "NOWAHdrMsaaResolve", 0);
				workspaceDef->connect(this->renderingNodeName, 2, "NOWAHdrMsaaResolve", 1);

				workspaceDef->connect("NOWAHdrMsaaResolve", 0, "NOWAHdrPostprocessingNode", 0);
				workspaceDef->connect(this->renderingNodeName, 2, "NOWAHdrPostprocessingNode", 1);
				workspaceDef->connectExternal(0, "NOWAHdrPostprocessingNode", 2);

				workspaceDef->connect("NOWAHdrPostprocessingNode", 0, this->finalRenderingNodeName, 0);
			}

			// Example Hdr with glass
			// workspaceDef->connect("NOWASkyRenderingNode", 0, "Glass", 0);
			// workspaceDef->connect("NOWASkyRenderingNode", 1, "Glass", 1);
			// workspaceDef->connect("NOWASkyRenderingNode", 2, "NOWAHdrPostprocessingNode", 1);
			// workspaceDef->connect("Glass", 0, "NOWAHdrPostprocessingNode", 0);
			// workspaceDef->connectExternal(0, "NOWAHdrPostprocessingNode", 2);
			// workspaceDef->connect("NOWAHdrPostprocessingNode", 0, "NOWAHdrFinalCompositionNode", 0);
		}
#ifdef TERRA_OLD
		if (true == this->useTerra)
		{
			workspaceDef->connectExternal(1, this->renderingNodeName, 0);
		}
#endif
		// If prior ws component has some channels defined
		if (true == this->externalChannels.empty())
		{
			this->externalChannels.resize(1);
			this->externalChannels[0] = Core::getSingletonPtr()->getOgreRenderWindow()->getTexture();
		}

		/*this->workspace = WorkspaceModule::getInstance()->getCompositorManager()->addWorkspace(this->gameObjectPtr->getSceneManager(), externalChannels,
			this->cameraComponent->getCamera(), this->workspaceName, true, -1, (Ogre::UavBufferPackedVec*)0,
			&this->initialCubemapLayouts, &this->initialCubemapUavAccess);*/

		this->workspace = WorkspaceModule::getInstance()->getCompositorManager()->addWorkspace(this->gameObjectPtr->getSceneManager(), externalChannels,
			this->cameraComponent->getCamera(), this->workspaceName, true);

		this->changeSkyBox(this->skyBoxName->getListSelectedValue());

		// this->initializeSmaa(WorkspaceModule::SMAA_PRESET_ULTRA, WorkspaceModule::EdgeDetectionColour);

		// this->initializeHdr(msaaLevel);

		// Is added in CameraComponent
		// WorkspaceModule::getInstance()->setPrimaryWorkspace(this->gameObjectPtr->getSceneManager(), this->cameraComponent->getCamera(), this);

		if (true == this->useHdr->getBool())
		{
			this->initializeHdr(this->msaaLevel);
		}

		// this->createSSAONoiseTexture(camera);

		return nullptr != this->workspace;
	}

	void WorkspaceSkyComponent::changeSkyBox(const Ogre::String& skyBoxName)
	{
		if (nullptr == this->workspace)
		{
			return;
		}

		Ogre::MaterialPtr materialPtr = Ogre::MaterialManager::getSingletonPtr()->getByName("NOWASkyPostprocess");
		if (false == materialPtr.isNull())
		{
			Ogre::Material* material = materialPtr.getPointer();
			Ogre::TextureUnitState* tex = material->getTechnique(0)->getPass(0)->getTextureUnitState(0);
			tex->setCubicTextureName(skyBoxName, true);
			// tex->setEnvironmentMap(true);
			// tex->setNumMipmaps(1024);
			tex->setGamma(2.0);
			material->compile();
		}
	}

	void WorkspaceSkyComponent::setSkyBoxName(const Ogre::String& skyBoxName)
	{
		this->skyBoxName->setListSelectedValue(skyBoxName);
		this->changeSkyBox(skyBoxName);
	}

	Ogre::String WorkspaceSkyComponent::getSkyBoxName(void) const
	{
		return this->skyBoxName->getListSelectedValue();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	WorkspaceBackgroundComponent::WorkspaceBackgroundComponent()
		: WorkspaceBaseComponent()
	{
		for (size_t i = 0; i < 9; i++)
		{
			passBackground[i] = nullptr;
		}
	}

	WorkspaceBackgroundComponent::~WorkspaceBackgroundComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[WorkspaceBackgroundComponent] Destructor workspace sky component for game object: " + this->gameObjectPtr->getName());
	}

	bool WorkspaceBackgroundComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = WorkspaceBaseComponent::init(propertyElement, filename);

		return success;
	}

	bool WorkspaceBackgroundComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[WorkspaceBackgroundComponent] Init workspace background component for game object: " + this->gameObjectPtr->getName());

		bool success = WorkspaceBaseComponent::postInit();

		return success;
	}

	void WorkspaceBackgroundComponent::actualizeValue(Variant* attribute)
	{
		WorkspaceBaseComponent::actualizeValue(attribute);

	}

	void WorkspaceBackgroundComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		WorkspaceBaseComponent::writeXML(propertiesXML, doc, filePath);
	}

	Ogre::String WorkspaceBackgroundComponent::getClassName(void) const
	{
		return "WorkspaceBackgroundComponent";
	}

	Ogre::String WorkspaceBackgroundComponent::getParentClassName(void) const
	{
		return "WorkspaceBaseComponent";
	}

	void WorkspaceBackgroundComponent::internalCreateCompositorNode(void)
	{
		if (!this->compositorManager->hasNodeDefinition(this->renderingNodeName))
		{
			Ogre::CompositorNodeDef* compositorNodeDefinition = compositorManager->addNodeDefinition(this->renderingNodeName);
#ifdef TERRA_OLD
			if (true == this->useTerra)
			{
				compositorNodeDefinition->addTextureSourceName("TerraShadowTexture", 0, Ogre::TextureDefinitionBase::TEXTURE_INPUT);
			}
#endif
			if (false == this->useHdr->getBool())
			{
				compositorNodeDefinition->setNumLocalTextureDefinitions(2);
			}
			else
			{
				compositorNodeDefinition->setNumLocalTextureDefinitions(3);
			}

			Ogre::TextureDefinitionBase::TextureDefinition* texDef = compositorNodeDefinition->addTextureDefinition("rt0");

			if (this->superSampling->getReal() <= 0.0f)
				this->superSampling->setValue(1.0f);

			texDef->width = 0; // target_width
			texDef->height = 0; // target_height
			texDef->widthFactor = this->superSampling->getReal();
			texDef->heightFactor = this->superSampling->getReal();
			texDef->format = Ogre::PFG_RGBA16_FLOAT;
			// texDef->msaa = this->msaaLevel;
			texDef->textureFlags = Ogre::TextureFlags::RenderToTexture | Ogre::TextureFlags::MsaaExplicitResolve;

			Ogre::RenderTargetViewDef* rtv = compositorNodeDefinition->addRenderTextureView("rt0");
			Ogre::RenderTargetViewEntry attachment;
			attachment.textureName = "rt0";
			rtv->colourAttachments.push_back(attachment);
			rtv->depthBufferId = Ogre::DepthBuffer::POOL_DEFAULT;

			texDef = compositorNodeDefinition->addTextureDefinition("rt1");
			texDef->width = 0; // target_width
			texDef->height = 0; // target_height
			texDef->widthFactor = this->superSampling->getReal();
			texDef->heightFactor = this->superSampling->getReal();
			texDef->format = Ogre::PFG_RGBA16_FLOAT;
			// texDef->msaa = this->msaaLevel;
			texDef->textureFlags = Ogre::TextureFlags::RenderToTexture | Ogre::TextureFlags::MsaaExplicitResolve;

			rtv = compositorNodeDefinition->addRenderTextureView("rt1");
			attachment.textureName = "rt1";
			rtv->colourAttachments.push_back(attachment);
			rtv->depthBufferId = Ogre::DepthBuffer::POOL_DEFAULT;

			if (true == this->useHdr->getBool())
			{
				texDef = compositorNodeDefinition->addTextureDefinition("oldLumRt");
				texDef->width = 1;
				texDef->height = 1;
				texDef->format = Ogre::PFG_RGBA16_FLOAT;
				texDef->textureFlags = Ogre::TextureFlags::RenderToTexture;
				texDef->depthBufferId = Ogre::DepthBuffer::POOL_DEFAULT;

				rtv = compositorNodeDefinition->addRenderTextureView("oldLumRt");
				attachment.textureName = "oldLumRt";
				rtv->colourAttachments.push_back(attachment);
				rtv->depthBufferId = Ogre::DepthBuffer::POOL_NO_DEPTH;
			}

			if (false == this->useHdr->getBool())
			{
				compositorNodeDefinition->setNumTargetPass(1);
			}
			else
			{
				compositorNodeDefinition->setNumTargetPass(2);
			}

			{
				Ogre::CompositorTargetDef* targetDef = compositorNodeDefinition->addTargetPass("rt0");

				{
					// Render Scene
					{
						Ogre::CompositorPassSceneDef* passScene;
						passScene = static_cast<Ogre::CompositorPassSceneDef*>(targetDef->addPass(Ogre::PASS_SCENE));

						Ogre::ColourValue color(0.2f, 0.4f, 0.6f);
						// passScene->setAllClearColours(color);
						passScene->setAllLoadActions(Ogre::LoadAction::Clear);
						passScene->mClearColour[0] = color;

						// passScene->setAllStoreActions(Ogre::StoreAction::StoreOrResolve);
						passScene->mStoreActionColour[0] = Ogre::StoreAction::StoreOrResolve; // Ogre::StoreAction::StoreAndMultisampleResolve; causes a crash, why?
						passScene->mStoreActionDepth = Ogre::StoreAction::DontCare;
						passScene->mClearStencil = Ogre::StoreAction::DontCare;
						passScene->mShadowNode = WorkspaceModule::getInstance()->shadowNodeName;

						passScene->mFirstRQ = 0;
						passScene->mLastRQ = 2;

						if (true == this->usePlanarReflection->getBool())
						{
							// Used to identify this pass wants planar reflections
							passScene->mIdentifier = 25001;
						}

						// passScene->setAllStoreActions(Ogre::StoreAction::StoreOrResolve);
						/*passScene->mStoreActionColour[0] = Ogre::StoreAction::StoreOrResolve;
						*/

						if (true == this->canUseReflection)
						{
							//Our materials in this pass will be using this cubemap,
							//so we need to expose it to the pass.
							//Note: Even if it "just works" without exposing, the reason for
							//exposing is to ensure compatibility with Vulkan & D3D12.

							passScene->mExposedTextures.emplace_back(Ogre::IdString("rt1"));
						}
#ifdef TERRA_OLD
						if (true == this->useTerra)
						{
							passScene->mExposedTextures.emplace_back(Ogre::IdString("TerraShadowTexture"));
						}
#endif
						passScene->mIncludeOverlays = false;
					}

					//https://forums.ogre3d.org/viewtopic.php?t=93636
					//https://forums.ogre3d.org/viewtopic.php?t=94748

					// Background 1 quad
					{
						Ogre::CompositorPassQuadDef* passQuad;
						passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));
						passQuad->mMaterialName = "NOWABackgroundPostprocess";
					}

					// Background 2 quad
					{
						Ogre::CompositorPassQuadDef* passQuad;
						passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));
						passQuad->mMaterialName = "NOWABackgroundPostprocess2";
					}

					// Background 3 quad
					{
						Ogre::CompositorPassQuadDef* passQuad;
						passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));
						passQuad->mMaterialName = "NOWABackgroundPostprocess3";
					}

					// Background 4 quad
					{
						Ogre::CompositorPassQuadDef* passQuad;
						passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));
						passQuad->mMaterialName = "NOWABackgroundPostprocess4";
					}

					// Background 5 quad
					{
						Ogre::CompositorPassQuadDef* passQuad;
						passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));
						passQuad->mMaterialName = "NOWABackgroundPostprocess5";
					}

					// Background 6 quad
					{
						Ogre::CompositorPassQuadDef* passQuad;
						passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));
						passQuad->mMaterialName = "NOWABackgroundPostprocess6";
					}

					// Background 7 quad
					{
						Ogre::CompositorPassQuadDef* passQuad;
						passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));
						passQuad->mMaterialName = "NOWABackgroundPostprocess7";
					}

					// Background 8 quad
					{
						Ogre::CompositorPassQuadDef* passQuad;
						passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));
						passQuad->mMaterialName = "NOWABackgroundPostprocess8";
					}

					// Background 9 quad
					{
						Ogre::CompositorPassQuadDef* passQuad;
						passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));
						passQuad->mMaterialName = "NOWABackgroundPostprocess9";
					}

					// Render Scene
					{
						Ogre::CompositorPassSceneDef* passScene;
						passScene = static_cast<Ogre::CompositorPassSceneDef*>(targetDef->addPass(Ogre::PASS_SCENE));
						passScene->mIncludeOverlays = false;
						passScene->mShadowNode = WorkspaceModule::getInstance()->shadowNodeName;
						passScene->mShadowNodeRecalculation = Ogre::ShadowNodeRecalculation::SHADOW_NODE_REUSE;
						passScene->mFirstRQ = 2;
					}
				}

				//targetDef = compositorNodeDefinition->addTargetPass("rt1");

				//{
				//	//Play nice with Multi-GPU setups by telling the API there is no inter-frame dependency.
				//	//On APIs that support discard_only (e.g. DX11) the driver may just ignore the color value,
				//	//and use a different memory region (i.e. the value read from a pixel of a discarded buffer
				//	//without having written to it is undefined)

				//	// Render Clear
				//	{
				//		Ogre::CompositorPassClearDef* passClear;
				//		passClear = static_cast<Ogre::CompositorPassClearDef*>(targetDef->addPass(Ogre::PASS_CLEAR));

				//		Ogre::ColourValue color(0.0f, 1.0f, 0.0f, 1.0f);
				//		passClear->mClearColour[0] = color;

				//		passClear->setAllLoadActions(Ogre::LoadAction::)

				//		passClear->mIncludeOverlays = false;
				//	}
				//}

				if (true == this->useHdr->getBool())
				{
					Ogre::CompositorTargetDef* targetDef = compositorNodeDefinition->addTargetPass("oldLumRt");

					{
						Ogre::CompositorPassClearDef* passClear;
						passClear = static_cast<Ogre::CompositorPassClearDef*>(targetDef->addPass(Ogre::PASS_CLEAR));
						passClear->mNumInitialPasses = 1;
						passClear->mClearColour[0] = Ogre::ColourValue(0.01f, 0.01f, 0.01f, 1.0f);
					}
				}
			}

			//Output channels

			if (false == this->useHdr->getBool())
			{
				compositorNodeDefinition->setNumOutputChannels(2);
			}
			else
			{
				compositorNodeDefinition->setNumOutputChannels(3);
			}

			compositorNodeDefinition->mapOutputChannel(0, "rt0");
			compositorNodeDefinition->mapOutputChannel(1, "rt1");

			if (true == this->useHdr->getBool())
			{
				compositorNodeDefinition->mapOutputChannel(2, "oldLumRt");
			}

			if (true == this->usePlanarReflection->getBool())
			{
				if (false == this->compositorManager->hasNodeDefinition(this->planarReflectionReflectiveRenderingNode))
				{
					Ogre::CompositorNodeDef* compositorNodeDefinition = compositorManager->addNodeDefinition(this->planarReflectionReflectiveRenderingNode);

					compositorNodeDefinition->addTextureSourceName("rt_renderwindow", 0, Ogre::TextureDefinitionBase::TEXTURE_INPUT);

					unsigned short numTargetPass = 1;
					compositorNodeDefinition->setNumTargetPass(numTargetPass);

					{
						Ogre::CompositorTargetDef* targetDef = compositorNodeDefinition->addTargetPass("rt_renderwindow");

						{
							// Render Scene
							{
								Ogre::CompositorPassSceneDef* passScene;
								passScene = static_cast<Ogre::CompositorPassSceneDef*>(targetDef->addPass(Ogre::PASS_SCENE));

								Ogre::ColourValue color(0.2f, 0.4f, 0.6f);
								// passScene->setAllClearColours(color);
								passScene->setAllLoadActions(Ogre::LoadAction::Clear);
								passScene->mClearColour[0] = color;

								// passScene->setAllStoreActions(Ogre::StoreAction::StoreOrResolve);
								passScene->mStoreActionColour[0] = Ogre::StoreAction::StoreOrResolve; // Ogre::StoreAction::StoreAndMultisampleResolve; causes a crash, why?
								passScene->mStoreActionDepth = Ogre::StoreAction::DontCare;
								passScene->mClearStencil = Ogre::StoreAction::DontCare;

								passScene->mFirstRQ = 0;
								passScene->mLastRQ = 2;

								passScene->mIncludeOverlays = false;

								passScene->mVisibilityMask = 0xfffffffe;

								//https://forums.ogre3d.org/viewtopic.php?t=93636
								//https://forums.ogre3d.org/viewtopic.php?t=94748
								passScene->mShadowNode = WorkspaceModule::getInstance()->shadowNodeName;
							}

							// Add background image passes
							for (size_t i = 0; i < 9; i++)
							{
								Ogre::String strMaterialName = "NOWABackgroundPostprocess";
								if (i > 0)
								{
									strMaterialName = "NOWABackgroundPostprocess" + Ogre::StringConverter::toString(i + 1);
								}
								Ogre::CompositorPassQuadDef* passQuad;
								passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));
								passQuad->mMaterialName = strMaterialName;
							}

							// Render Scene
							{
								Ogre::CompositorPassSceneDef* passScene;
								passScene = static_cast<Ogre::CompositorPassSceneDef*>(targetDef->addPass(Ogre::PASS_SCENE));
								passScene->mIncludeOverlays = false;
								passScene->mShadowNode = WorkspaceModule::getInstance()->shadowNodeName;
								passScene->mShadowNodeRecalculation = Ogre::ShadowNodeRecalculation::SHADOW_NODE_REUSE;
								passScene->mFirstRQ = 2;
							}

							// Generate Mipmaps
							{
								Ogre::CompositorPassMipmapDef* passMipmap;
								passMipmap = static_cast<Ogre::CompositorPassMipmapDef*>(targetDef->addPass(Ogre::PASS_MIPMAP));

								passMipmap->mMipmapGenerationMethod = Ogre::CompositorPassMipmapDef::ComputeHQ;
							}
						}
					}
				}
			}
		}

		for (size_t i = 0; i < 9; i++)
		{
			Ogre::String strMaterialName = "NOWABackgroundPostprocess";
			if (i > 0)
			{
				strMaterialName = "NOWABackgroundPostprocess" + Ogre::StringConverter::toString(i + 1);
			}
			this->materialBackgroundPtr[i] = Ogre::MaterialManager::getSingletonPtr()->getByName(strMaterialName);
			
			if (true == this->materialBackgroundPtr[i].isNull())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[WorkspaceBackgroundComponent] Could not set: " + this->workspaceName + " because the material: '" + strMaterialName + "' does not exist!");
				throw Ogre::Exception(Ogre::Exception::ERR_ITEM_NOT_FOUND, "Could not create: " + this->workspaceName + " because the material: '" + strMaterialName + "' does not exist!", "NOWA");
			}

			Ogre::Material* material = this->materialBackgroundPtr[i].getPointer();
			this->passBackground[i] = material->getTechnique(0)->getPass(0);
		}

		// For VR: disable this line and use NOWAPbsRenderingNodeVR
		this->changeViewportRect(0, this->viewportRect->getVector4());

		this->externalChannels.resize(1);
		this->externalChannels[0] = Core::getSingletonPtr()->getOgreRenderWindow()->getTexture();
	}

	bool WorkspaceBackgroundComponent::internalCreateWorkspace(Ogre::CompositorWorkspaceDef* workspaceDef)
	{
		// Normal

#if 0
		workspaceDef->connect("NOWASkyRenderingNode", 0, "NOWAPbsFinalCompositionNode", 1);
		workspaceDef->connectExternal(0, "NOWAPbsFinalCompositionNode", 0);
#endif

		// Normal with glass

#if 0
		// Note: Glass must be enabled: it->second->setStartEnabled(true);
		workspaceDef->connect("NOWASkyRenderingNode", "Glass");
		workspaceDef->connectExternal(0, "NOWAPbsFinalCompositionNode", 0);
		workspaceDef->connect("Glass", 0, "NOWAPbsFinalCompositionNode", 1);
#endif

#if 0
		// SMAA
		// Later set for all workspaces and set for configuration via component?
		workspaceDef->connect("NOWASkyRenderingNode", 0, "SmaaNode", 0);
		workspaceDef->connect("NOWASkyRenderingNode", 1, "SmaaNode", 1);

		workspaceDef->connectExternal(0, "NOWAPbsFinalCompositionNode", 0);
		workspaceDef->connect("SmaaNode", 0, "NOWAPbsFinalCompositionNode", 1);

		this->workspace = WorkspaceModule::getInstance()->getCompositorManager()->addWorkspace(sceneManager, Core::getSingletonPtr()->getOgreRenderWindow()->getTexture(), camera, workspaceName, true);

		this->initializeSmaa(WorkspaceModule::SMAA_PRESET_ULTRA, WorkspaceModule::EdgeDetectionColour);
#endif

		for (size_t i = 0; i < 9; i++)
		{
			Ogre::String strMaterialName = "NOWABackgroundPostprocess";
			if (i > 0)
			{
				strMaterialName = "NOWABackgroundPostprocess" + Ogre::StringConverter::toString(i + 1);
				
			}
			this->materialBackgroundPtr[i] = Ogre::MaterialManager::getSingletonPtr()->getByName(strMaterialName);
			
			if (true == this->materialBackgroundPtr[i].isNull())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[WorkspaceBackgroundComponent] Could not set: " + this->workspaceName + " because the material: '" + strMaterialName + "' does not exist!");
				throw Ogre::Exception(Ogre::Exception::ERR_ITEM_NOT_FOUND, "Could not create: " + this->workspaceName + " because the material: '" + strMaterialName + "' does not exist!", "NOWA");
			}

			Ogre::Material* material = this->materialBackgroundPtr[i].getPointer();
			this->passBackground[i] = material->getTechnique(0)->getPass(0);
		}

		this->changeViewportRect(0, this->viewportRect->getVector4());

		if (false == this->useHdr->getBool())
		{
			workspaceDef->connect(this->renderingNodeName, 0, this->finalRenderingNodeName, 1);
			workspaceDef->connectExternal(0, this->finalRenderingNodeName, 0);
		}
		else
		{
			// Hdr with sky
			if (1 == this->msaaLevel)
			{
				workspaceDef->connect(this->renderingNodeName, 0, "NOWAHdrPostprocessingNode", 0);
				workspaceDef->connect(this->renderingNodeName, 2, "NOWAHdrPostprocessingNode", 1);
				workspaceDef->connectExternal(0, "NOWAHdrPostprocessingNode", 2);

				workspaceDef->connect("NOWAHdrPostprocessingNode", 0, this->finalRenderingNodeName, 0);
			}
			else
			{
				workspaceDef->connect(this->renderingNodeName, 0, "NOWAHdrMsaaResolve", 0);
				workspaceDef->connect(this->renderingNodeName, 2, "NOWAHdrMsaaResolve", 1);

				workspaceDef->connect("NOWAHdrMsaaResolve", 0, "NOWAHdrPostprocessingNode", 0);
				workspaceDef->connect(this->renderingNodeName, 2, "NOWAHdrPostprocessingNode", 1);
				workspaceDef->connectExternal(0, "NOWAHdrPostprocessingNode", 2);

				workspaceDef->connect("NOWAHdrPostprocessingNode", 0, this->finalRenderingNodeName, 0);
			}

			// Example Hdr with glass
			// workspaceDef->connect("NOWASkyRenderingNode", 0, "Glass", 0);
			// workspaceDef->connect("NOWASkyRenderingNode", 1, "Glass", 1);
			// workspaceDef->connect("NOWASkyRenderingNode", 2, "NOWAHdrPostprocessingNode", 1);
			// workspaceDef->connect("Glass", 0, "NOWAHdrPostprocessingNode", 0);
			// workspaceDef->connectExternal(0, "NOWAHdrPostprocessingNode", 2);
			// workspaceDef->connect("NOWAHdrPostprocessingNode", 0, "NOWAHdrFinalCompositionNode", 0);
		}
#ifdef TERRA_OLD
		if (true == this->useTerra)
		{
			workspaceDef->connectExternal(1, this->renderingNodeName, 0);
		}
#endif
		// If prior ws component has some channels defined
		if (true == this->externalChannels.empty())
		{
			this->externalChannels.resize(1);
			this->externalChannels[0] = Core::getSingletonPtr()->getOgreRenderWindow()->getTexture();
		}

		this->workspace = WorkspaceModule::getInstance()->getCompositorManager()->addWorkspace(this->gameObjectPtr->getSceneManager(), externalChannels,
			this->cameraComponent->getCamera(), this->workspaceName, true);

		/*this->workspace = WorkspaceModule::getInstance()->getCompositorManager()->addWorkspace(this->gameObjectPtr->getSceneManager(), externalChannels,
			this->cameraComponent->getCamera(), this->workspaceName, true, -1, (Ogre::UavBufferPackedVec*)0,
			&this->initialCubemapLayouts, &this->initialCubemapUavAccess);*/
		// this->initializeSmaa(WorkspaceBaseComponent::SMAA_PRESET_ULTRA, WorkspaceBaseComponent::EdgeDetectionColour);

		// this->initializeHdr(msaaLevel);

		// WorkspaceModule::getInstance()->setPrimaryWorkspace(this->gameObjectPtr->getSceneManager(), this->cameraComponent->getCamera(), this);

		if (true == this->useHdr->getBool())
		{
			this->initializeHdr(this->msaaLevel);
		}

		// this->createSSAONoiseTexture(camera);

		return nullptr != this->workspace;
	}

	void WorkspaceBackgroundComponent::removeWorkspace(void)
	{
		WorkspaceBaseComponent::removeWorkspace();

		for (size_t i = 0; i < 9; i++)
		{
			if (false == this->materialBackgroundPtr[i].isNull())
				this->materialBackgroundPtr[i].setNull();

			this->passBackground[i] = nullptr;
		}
	}

	void WorkspaceBackgroundComponent::changeBackground(unsigned short index, const Ogre::String& backgroundTextureName)
	{
		if (nullptr == this->passBackground[index])
		{
			// Create all
			for (size_t i = 0; i < 9; i++)
			{
				Ogre::String strMaterialName = "NOWABackgroundPostprocess";
				if (i > 0)
				{
					strMaterialName = "NOWABackgroundPostprocess" + Ogre::StringConverter::toString(i + 1);
				}
				this->materialBackgroundPtr[i] = Ogre::MaterialManager::getSingletonPtr()->getByName(strMaterialName);

				if (true == this->materialBackgroundPtr[i].isNull())
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[WorkspaceBackgroundComponent] Could not set: " + this->workspaceName + " because the material: '" + strMaterialName + "' does not exist!");
					throw Ogre::Exception(Ogre::Exception::ERR_ITEM_NOT_FOUND, "Could not create: " + this->workspaceName + " because the material: '" + strMaterialName + "' does not exist!", "NOWA");
				}

				Ogre::Material* material = this->materialBackgroundPtr[i].getPointer();
				this->passBackground[i] = material->getTechnique(0)->getPass(0);
			}
		}

		if (nullptr != this->passBackground[index])
		{
			// Change background texture
			Ogre::TextureUnitState* tex = this->passBackground[index]->getTextureUnitState(0);
			tex->setTextureName(backgroundTextureName);

			// tex->setGamma(2.0);
			this->materialBackgroundPtr[index]->compile();
		}
	}

	void WorkspaceBackgroundComponent::setBackgroundScrollSpeedX(unsigned short index, Ogre::Real backgroundScrollSpeedX)
	{
		if (nullptr != this->passBackground[index])
		{
			this->passBackground[index]->getFragmentProgramParameters()->setNamedConstant("speedX", backgroundScrollSpeedX);

			// tex->setGamma(2.0);
// Attention: Is that necessary?
			// material->compile();
		}
	}

	void WorkspaceBackgroundComponent::setBackgroundScrollSpeedY(unsigned short index, Ogre::Real backgroundScrollSpeedY)
	{
		if (nullptr != this->passBackground[index])
		{
			this->passBackground[index]->getFragmentProgramParameters()->setNamedConstant("speedY", backgroundScrollSpeedY);

			// tex->setGamma(2.0);
			// Attention: Is that necessary?
			// material->compile();
		}
	}

	void WorkspaceBackgroundComponent::compileBackgroundMaterial(unsigned short index)
	{
		if (nullptr != this->passBackground[index])
		{
			this->materialBackgroundPtr[index]->compile();
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	WorkspaceCustomComponent::WorkspaceCustomComponent()
		: WorkspaceBaseComponent(),
		customWorkspaceName(new Variant(WorkspaceCustomComponent::AttrCustomWorkspaceName(), Ogre::String("PbsCustomWorkspace"), this->attributes))
	{

	}

	WorkspaceCustomComponent::~WorkspaceCustomComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[WorkspaceCustomComponent] Destructor workspace custom component for game object: " + this->gameObjectPtr->getName());
	}

	bool WorkspaceCustomComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = WorkspaceBaseComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CustomWorkspaceName")
		{
			this->customWorkspaceName->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		return success;
	}

	bool WorkspaceCustomComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[WorkspaceCustomComponent] Init workspace custom component for game object: " + this->gameObjectPtr->getName());

		bool success = WorkspaceBaseComponent::postInit();

		return success;
	}

	void WorkspaceCustomComponent::actualizeValue(Variant* attribute)
	{
		WorkspaceBaseComponent::actualizeValue(attribute);

		if (WorkspaceCustomComponent::AttrCustomWorkspaceName() == attribute->getName())
		{
			this->setCustomWorkspaceName(attribute->getString());
		}
	}

	void WorkspaceCustomComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		WorkspaceBaseComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CustomWorkspaceName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->customWorkspaceName->getString())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String WorkspaceCustomComponent::getClassName(void) const
	{
		return "WorkspaceCustomComponent";
	}

	Ogre::String WorkspaceCustomComponent::getParentClassName(void) const
	{
		return "WorkspaceBaseComponent";
	}

	void WorkspaceCustomComponent::internalInitWorkspaceData(void)
	{

	}

	void WorkspaceCustomComponent::internalCreateCompositorNode(void)
	{
		if (false == this->customWorkspaceName->getString().empty())
		{
			this->workspaceName = this->customWorkspaceName->getString();
		}
	}

	bool WorkspaceCustomComponent::internalCreateWorkspace(Ogre::CompositorWorkspaceDef* workspaceDef)
	{
		// Normal

#if 0
		workspaceDef->connect("NOWASkyRenderingNode", 0, "NOWAPbsFinalCompositionNode", 1);
		workspaceDef->connectExternal(0, "NOWAPbsFinalCompositionNode", 0);
#endif

		// Normal with glass

#if 0
		// Note: Glass must be enabled: it->second->setStartEnabled(true);
		workspaceDef->connect("NOWASkyRenderingNode", "Glass");
		workspaceDef->connectExternal(0, "NOWAPbsFinalCompositionNode", 0);
		workspaceDef->connect("Glass", 0, "NOWAPbsFinalCompositionNode", 1);
#endif

#if 0
		// SMAA
		// Later set for all workspaces and set for configuration via component?
		workspaceDef->connect("NOWASkyRenderingNode", 0, "SmaaNode", 0);
		workspaceDef->connect("NOWASkyRenderingNode", 1, "SmaaNode", 1);

		workspaceDef->connectExternal(0, "NOWAPbsFinalCompositionNode", 0);
		workspaceDef->connect("SmaaNode", 0, "NOWAPbsFinalCompositionNode", 1);

		this->workspace = WorkspaceModule::getInstance()->getCompositorManager()->addWorkspace(sceneManager, Core::getSingletonPtr()->getOgreRenderWindow()->getTexture(), camera, workspaceName, true);

		this->initializeSmaa(WorkspaceModule::SMAA_PRESET_ULTRA, WorkspaceModule::EdgeDetectionColour);
#endif

		if (false == this->customWorkspaceName->getString().empty())
		{
			const Ogre::IdString workspaceNameId(this->customWorkspaceName->getString());

			if (false == WorkspaceModule::getInstance()->getCompositorManager()->hasWorkspaceDefinition(workspaceNameId))
			{
				Ogre::ColourValue color(this->backgroundColor->getVector3().x, this->backgroundColor->getVector3().y, this->backgroundColor->getVector3().z);
				this->compositorManager->createBasicWorkspaceDef(this->customWorkspaceName->getString(), color, Ogre::IdString());
			}

			// Will crash, no node name
			// this->changeViewportRect(0, this->viewportRect->getVector4());

			// If prior ws component has some channels defined
			if (true == this->externalChannels.empty())
			{
				this->externalChannels.resize(1);
				this->externalChannels[0] = Core::getSingletonPtr()->getOgreRenderWindow()->getTexture();
			}

			/*this->workspace = WorkspaceModule::getInstance()->getCompositorManager()->addWorkspace(this->gameObjectPtr->getSceneManager(), externalChannels,
				this->cameraComponent->getCamera(), this->customWorkspaceName->getString(), true, -1, (Ogre::UavBufferPackedVec*)0,
				&this->initialCubemapLayouts, &this->initialCubemapUavAccess);*/

			this->workspace = WorkspaceModule::getInstance()->getCompositorManager()->addWorkspace(this->gameObjectPtr->getSceneManager(), Core::getSingletonPtr()->getOgreRenderWindow()->getTexture(), this->cameraComponent->getCamera(),
                                                    this->customWorkspaceName->getString(), true );
		}

		// WorkspaceModule::getInstance()->setPrimaryWorkspace(this->gameObjectPtr->getSceneManager(), this->cameraComponent->getCamera(), this);

		return nullptr != this->workspace;
	}

	void WorkspaceCustomComponent::setCustomWorkspaceName(const Ogre::String& customWorkspaceName)
	{
		this->customWorkspaceName->setValue(customWorkspaceName);
		this->createWorkspace();
	}

	Ogre::String WorkspaceCustomComponent::getCustomWorkspaceName(void) const
	{
		return this->customWorkspaceName->getString();
	}

}; // namespace end