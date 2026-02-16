#include "NOWAPrecompiled.h"
#include "WorkspaceComponents.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "Modules/WorkspaceModule.h"
#include "main/Core.h"
#include "main/AppStateManager.h"
#include "CameraComponent.h"
#include "OceanComponent.h"
#include "camera/BaseCamera.h"
#include "PlanarReflectionComponent.h"
#include "ReflectionCameraComponent.h"
#include "LightDirectionalComponent.h"
#include "CompositorEffectComponents.h"
#include "DatablockPbsComponent.h"
#include "Compositor/Pass/PassScene/OgreCompositorPassScene.h"
#include "Compositor/Pass/PassQuad/OgreCompositorPassQuad.h"
#include "Compositor/Pass/PassQuad/OgreCompositorPassQuadDef.h"
#include "Compositor/Pass/PassMipmap/OgreCompositorPassMipmapDef.h"
#include "OgreHlmsListener.h"
#include "OgreFrameListener.h"
#include "Cubemaps/OgreParallaxCorrectedCubemapAuto.h"
#include "OgreBitwise.h"

#include "Compositor/OgreCompositorShadowNodeDef.h"
#include "Compositor/OgreCompositorWorkspaceListener.h"
#include "Compositor/Pass/PassIblSpecular/OgreCompositorPassIblSpecularDef.h"

#include "TerraShadowMapper.h"
#include "ocean/OgreHlmsOcean.h"

// #define GPU_PARTICLES

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

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
			Ogre::String id = "PlanarReflectionsWorkspaceListener_ParticleFxModule::workspacePreUpdate";
			NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);
		}

		virtual void workspacePreUpdate(Ogre::CompositorWorkspace* workspace)
		{
			auto closureFunction = [this](Ogre::Real renderDt)
			{
				this->planarReflections->beginFrame();
			};
			Ogre::String id = "PlanarReflectionsWorkspaceListener_ParticleFxModule::workspacePreUpdate";
			NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
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

			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("PlanarReflectionsWorkspaceListener::passEarlyPreExecute", _2(camera, pass),
			{
					// Note: The Aspect Ratio must match that of the camera we're reflecting.
					this->planarReflections->update(camera, camera->getAutoAspectRatio()
											   ? pass->getViewportAspectRatio(0u)
											   : camera->getAspectRatio());
			});
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
		useSSAO(new Variant(WorkspaceBaseComponent::AttrUseSSAO(), false, this->attributes)),
		useDistortion(new Variant(WorkspaceBaseComponent::AttrUseDistortion(), false, this->attributes)),
		useMSAA(new Variant(WorkspaceBaseComponent::AttrUseMSAA(), false, this->attributes)),
		usePCC(new Variant(WorkspaceBaseComponent::AttrUsePCC(), false, this->attributes)),
		shadowGlobalBias(new Variant(WorkspaceBaseComponent::AttrShadowGlobalBias(), Ogre::Real(1.0f), this->attributes)),
		shadowGlobalNormalOffset(new Variant(WorkspaceBaseComponent::AttrShadowGlobalNormalOffset(), Ogre::Real(168.0f), this->attributes)),
		shadowPSSMLambda(new Variant(WorkspaceBaseComponent::AttrShadowPSSMLambda(), Ogre::Real(0.95f), this->attributes)),
		shadowSplitBlend(new Variant(WorkspaceBaseComponent::AttrShadowSplitBlend(), Ogre::Real(0.125f), this->attributes)),
		shadowSplitFade(new Variant(WorkspaceBaseComponent::AttrShadowSplitFade(), Ogre::Real(0.313f), this->attributes)),
		shadowSplitPadding(new Variant(WorkspaceBaseComponent::AttrShadowSplitPadding(), Ogre::Real(1.0f), this->attributes)),
		cameraComponent(nullptr),
		oceanComponent(nullptr),
		workspace(nullptr),
		msaaLevel(1),
		oldBackgroundColor(backgroundColor->getVector3()),
		hlms(nullptr),
		pbs(nullptr),
		unlit(nullptr),
		hlmsManager(nullptr),
		compositorManager(nullptr),
		workspaceCubemap(nullptr),
		cubemapTexture(nullptr),
		canUseReflection(false),
		planarReflections(nullptr),
		planarReflectionsWorkspaceListener(nullptr),
		useTerra(false),
		terra(nullptr),
		useOcean(false),
		canUseOcean(false),
		oceanUnderwaterActive(false),
		hlmsListener(nullptr),
		hlmsWind(nullptr),
		involvedInSplitScreen(false),
		parallaxCorrectedCubemap(nullptr),
		workspacePccProbes(nullptr)
	{
		this->backgroundColor->addUserData(GameObject::AttrActionColorDialog());
		this->superSampling->setDescription("Sets the supersampling for whole scene texture rendering. E.g. a value of 0.25 will pixelize the scene for retro Game experience.");
		this->useReflection->setDescription("Activates reflection, which is rendered by a dynamic cubemap. This component works in conjunction with a ReflectionCameraComponent. Attention: Activating this mode decreases the application's performance considerably!");
		this->usePlanarReflection->setDescription("Activates planar reflection, which is used for mirror planes in conjunction with a PlanarReflectionComponent.");
		this->useDistortion->setDescription("Activates distortion. Distortion setup can be used to create blastwave effects, mix with fire particle effects "
            "to get heat distortion etc. This component works in conjunction with a DistortionComponent.");
		this->referenceId->setVisible(false);
		this->reflectionCameraGameObjectId->setVisible(false);

		this->useMSAA->setDescription("Sets whether to use MSAA (Enhanced Subpixel Morphological Antialiasing). It can also only be used if the ogre.cfg configured anti aliasing level is higher as 1.");

		this->useReflection->addUserData(GameObject::AttrActionNeedRefresh());

		this->usePCC->setDescription("Enable Parallax Corrected Cubemap (PCC) system for automatic reflection probe placement. "
			"This provides better quality reflections in interior spaces. "
			"Cannot be used with manual reflection cameras.");
		this->usePCC->setReadOnly(true);
		// this->usePCC->addUserData(GameObject::AttrActionNeedRefresh());
	}

	WorkspaceBaseComponent::~WorkspaceBaseComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[WorkspaceBaseComponent] Destructor workspace base component for game object: " + this->gameObjectPtr->getName());
		
		this->cameraComponent = nullptr;
		this->oceanComponent = nullptr;
	}

	bool WorkspaceBaseComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BackgroundColor")
		{
			this->backgroundColor->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ViewportRect")
		{
			this->setViewportRect(XMLConverter::getAttribVector4(propertyElement, "data"));
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
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseSSAO")
		{
			this->useSSAO->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseDistortion")
		{
			this->useDistortion->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseMSAA")
		{
			this->useMSAA->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UsePCC")
		{
			this->usePCC->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
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

		// Not active, skip workspace creation
		// TODO: What todo on split screen?
		/*if (false == this->cameraComponent->isActivated())
		{
            return true;
		}*/

		bool hasAnyMirror = this->hasAnyMirrorForPlanarReflections();

		this->usePlanarReflection->setValue(hasAnyMirror);

		if (true == this->useSSAO->getBool())
		{
			this->createSSAONoiseTexture();
		}

		return this->createWorkspace();
	}

	bool WorkspaceBaseComponent::connect(void)
	{
		// Only create workspace for active camera
		if (nullptr != this->cameraComponent && false == this->cameraComponent->isActivated() && false == this->involvedInSplitScreen)
		{
			return true;
		}

		GraphicsModule::getInstance()->beginWorkspaceTransition();

		this->canUseOcean = this->useOcean;

		NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
		{
			if (nullptr != this->workspace)
			{
				Ogre::CompositorWorkspaceDef* workspaceDef = this->workspace->getCompositorManager()->getWorkspaceDefinition(this->workspaceName);
				if (nullptr != workspaceDef)
				{
					// Use clearAll() instead of clearAllInterNodeConnections() 
					// to also clear external connections
					workspaceDef->clearAll();
					workspaceDef->clearOutputConnections();
				}
			}
			auto compositorEffectComponents = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectComponents<CompositorEffectBaseComponent>();
			for (size_t i = 0; i < compositorEffectComponents.size(); i++)
			{
				compositorEffectComponents[i]->enableEffect(compositorEffectComponents[i]->effectName, compositorEffectComponents[i]->isActivated());
			}
			this->reconnectAllNodes();
		};
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "GameObject::connect");

		GraphicsModule::getInstance()->endWorkspaceTransition();
		return true;
	}

	bool WorkspaceBaseComponent::disconnect(void)
	{
		NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
		{
			this->canUseOcean = false;
			if (nullptr != this->workspace)
			{
				Ogre::CompositorWorkspaceDef* workspaceDef = this->workspace->getCompositorManager()->getWorkspaceDefinitionNoThrow(this->workspaceName);
				if (nullptr != workspaceDef)
				{
					workspaceDef->clearAll();
					workspaceDef->clearOutputConnections();
				}
			}

			auto compositorEffectComponents = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectComponents<CompositorEffectBaseComponent>();

			for (size_t i = 0; i < compositorEffectComponents.size(); i++)
			{
				compositorEffectComponents[i]->enableEffect(compositorEffectComponents[i]->effectName, false);
			}

			this->reconnectAllNodes();
		};
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "GameObject::disconnect");

		return true;
	}

	void WorkspaceBaseComponent::internalInitWorkspaceData(void)
	{
		Ogre::String workspacePostfix = Ogre::StringConverter::toString(this->gameObjectPtr->getId());
		this->workspaceName = "NOWAWorkspace" + workspacePostfix;
		this->renderingNodeName = "NOWARenderingNode" + workspacePostfix;
		this->finalRenderingNodeName = "NOWAFinalCompositionNode" + workspacePostfix;

		this->underwaterNodeName = "NOWAUnderwaterNode" + workspacePostfix;
		if (true == this->usePlanarReflection->getBool())
		{
			this->planarReflectionReflectiveRenderingNode = "PlanarReflectionsReflectiveRenderingNode" + workspacePostfix;
			this->planarReflectionReflectiveWorkspaceName = "NOWAPlanarReflectionsReflectiveWorkspace" + workspacePostfix;
		}

		if (true == this->useDistortion->getBool())
		{
			this->distortionNode = "DistortionNode" + workspacePostfix;
		}
	}

	bool WorkspaceBaseComponent::createWorkspace(void)
	{
		if (nullptr == this->gameObjectPtr)
		{
			return true;
		}

		Ogre::String name = this->gameObjectPtr->getName();

		// NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.5f));
		// auto ptrFunction = [this]() {
		if (nullptr == this->compositorManager)
		{
			return true;
		}

		// Only create workspace if the camera is activated and there are no custom external channels (from split screen component)
		if (false == this->cameraComponent->isActivated() && false == this->involvedInSplitScreen)
		{
			return true;
		}

		this->internalInitWorkspaceData();

		this->removeWorkspace();

		GraphicsModule::RenderCommand renderCommand = [this]()
		{
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

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[WorkspaceBaseComponent] Creating workspace: " + this->workspaceName);

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

						this->cubemapTexture = textureManager->createOrRetrieveTexture("cubemap",
																				Ogre::GpuPageOutStrategy::Discard, Ogre::TextureFlags::RenderToTexture | Ogre::TextureFlags::AllowAutomipmaps |
																				iblSpecularFlag, Ogre::TextureTypes::TypeCube);
						this->cubemapTexture->scheduleTransitionTo(Ogre::GpuResidency::OnStorage);

						unsigned int cubeTextureSize = Ogre::StringConverter::parseUnsignedInt(reflectionCameraCompPtr->getCubeTextureSize());

						this->cubemapTexture->setResolution(cubeTextureSize, cubeTextureSize);
						this->cubemapTexture->setNumMipmaps(Ogre::PixelFormatGpuUtils::getMaxMipmapCount(cubeTextureSize, cubeTextureSize));
						this->cubemapTexture->setPixelFormat(Ogre::PFG_RGBA8_UNORM_SRGB);
						this->cubemapTexture->_transitionTo(Ogre::GpuResidency::Resident, (Ogre::uint8*)0);

						this->pbs->resetIblSpecMipmap(0u);

						//Setup the cubemap's compositor.
						Ogre::CompositorChannelVec cubemapExternalChannels(1);
						//Any of the cubemap's render targets will do
						cubemapExternalChannels[0] = this->cubemapTexture;

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
						this->externalChannels[1] = this->cubemapTexture;
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

			if (true == this->usePCC->getBool())
			{
				// PCC doesn't need a separate cubemap texture initially
				// because ParallaxCorrectedCubemapAuto manages its own textures

				// Create the probe renderer node and workspace definitions
				this->createLocalCubemapProbeRendererNode();
				this->createLocalCubemapsProbeWorkspace();

				// Note: The actual PCC system will be created by 
				// PccPerPixelGridPlacementComponent after workspace is ready
			}


			this->internalCreateCompositorNode();

			if (true == this->useTerra || true == this->canUseOcean)
			{
				unsigned char channelCount = 1;
				if (true == this->canUseReflection)
				{
					channelCount++;
				}

				this->externalChannels.resize(channelCount);
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
								planarReflectionCompPtr->destroyPlane();
								planarReflectionCompPtr->createPlane();
							}
						}

						j++;
					}
				}
			}

			this->hlmsWind = dynamic_cast<HlmsWind*>(Ogre::Root::getSingleton().getHlmsManager()->getHlms(Ogre::HLMS_USER0));
			hlmsWind->setup(this->gameObjectPtr->getSceneManager());

			boost::shared_ptr<EventDataHdrActivated> eventDataHdrActivated(new EventDataHdrActivated(gameObjectPtr->getId(), false));
			AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataHdrActivated);

			// };
			// NOWA::ProcessPtr closureProcess(new NOWA::ClosureProcess(ptrFunction));
			// delayProcess->attachChild(closureProcess);
			// NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);

			this->updateShadowGlobalBias();
		};

		Ogre::String message = "WorkspaceBaseComponent::createWorkspace for: " + name;
		GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), message.data());

		return true;
	}

	void WorkspaceBaseComponent::removeWorkspace(void)
	{
		Ogre::String name = this->gameObjectPtr->getName();

		if (nullptr == this->workspace)
		{
			return;
		}

		GraphicsModule::RenderCommand renderCommand = [this]()
		{
			this->resetReflectionForAllEntities();

			this->destroyPccSystem();

			Ogre::TextureGpuManager * textureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();

			if (nullptr != this->workspaceCubemap)
			{
				this->compositorManager->removeWorkspace(this->workspaceCubemap);
				this->workspaceCubemap = nullptr;
			}
			if (nullptr != this->cubemapTexture)
			{
				textureManager->destroyTexture(this->cubemapTexture);
				this->cubemapTexture = nullptr;
			}
			if (nullptr != this->workspace)
			{
				if (nullptr != this->planarReflectionsWorkspaceListener)
				{
					this->workspace->removeListener(this->planarReflectionsWorkspaceListener);
				}

				if (true == this->useTerra || true == this->canUseOcean)
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

				if (true == this->compositorManager->hasNodeDefinition(this->underwaterNodeName))
				{
					this->compositorManager->removeNodeDefinition(this->underwaterNodeName);
				}

				if (true == this->compositorManager->hasNodeDefinition(this->renderingNodeName))
				{
					this->compositorManager->removeNodeDefinition(this->renderingNodeName);
				}
				this->workspace = nullptr;
				this->externalChannels.clear();

				this->customExternalChannels.clear();

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
				if (true == this->compositorManager->hasNodeDefinition(this->distortionNode))
				{
					this->compositorManager->removeNodeDefinition(this->distortionNode);
				}
				if (true == this->compositorManager->hasNodeDefinition("AutoGen " + Ogre::IdString(this->planarReflectionReflectiveWorkspaceName + "/Node").getReleaseText()))
				{
					this->compositorManager->removeNodeDefinition("AutoGen " + Ogre::IdString(this->planarReflectionReflectiveWorkspaceName + "/Node").getReleaseText());
				}

				if (true == this->compositorManager->hasWorkspaceDefinition(this->planarReflectionReflectiveWorkspaceName))
				{
					this->compositorManager->removeWorkspaceDefinition(this->planarReflectionReflectiveWorkspaceName);
				}

				if (true == this->compositorManager->hasNodeDefinition("LocalCubemapProbeRendererNode"))
				{
					this->compositorManager->removeNodeDefinition("LocalCubemapProbeRendererNode");
				}

				if (true == this->compositorManager->hasWorkspaceDefinition("LocalCubemapsProbeWorkspace"))
				{
					this->compositorManager->removeWorkspaceDefinition("LocalCubemapsProbeWorkspace");
				}

				if (nullptr != this->hlmsListener)
				{
					delete this->hlmsListener;
					this->hlmsListener = nullptr;
				}
			}
		};

		Ogre::String message = "WorkspaceBaseComponent::removeWorkspace for: " + name;
		GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), message.data());
	}

	void WorkspaceBaseComponent::nullWorkspace(void)
	{
		if (nullptr != this->workspace)
		{
			ENQUEUE_RENDER_COMMAND_WAIT("WorkspaceBaseComponent::nullWorkspace",
			{
				this->compositorManager->removeWorkspace(this->workspace);
				this->workspace = nullptr;
			});
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

		Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
		NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

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

		// Send event, that component has been deleted
		boost::shared_ptr<EventDataDeleteWorkspaceComponent> eventDataDeleteWorkspaceComponent(new EventDataDeleteWorkspaceComponent(this->gameObjectPtr->getId()));
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataDeleteWorkspaceComponent);

		this->removeWorkspace();
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
		else if (WorkspaceBaseComponent::AttrUseSSAO() == attribute->getName())
		{
			this->setUseSSAO(attribute->getBool());
		}
		else if (WorkspaceBaseComponent::AttrUseDistortion() == attribute->getName())
		{
			this->setUseDistortion(attribute->getBool());
		}
		else if (WorkspaceBaseComponent::AttrUseMSAA() == attribute->getName())
		{
			this->setUseMSAA(attribute->getBool());
		}
		else if (WorkspaceBaseComponent::AttrUsePCC() == attribute->getName())
		{
			// Manipulated by PccPerPixelGridPlacementComponent
			// this->setUsePCC(attribute->getBool());
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

	void WorkspaceBaseComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		GameObjectComponent::writeXML(propertiesXML, doc);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BackgroundColor"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->backgroundColor->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ViewportRect"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->viewportRect->getVector4())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SuperSampling"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->superSampling->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UseHdr"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useHdr->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UseReflection"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useReflection->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ReflectionCameraGameObjectId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->reflectionCameraGameObjectId->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UsePlanarReflection"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->usePlanarReflection->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UseSSAO"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useSSAO->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UseDistortion"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useDistortion->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UseMSAA"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useMSAA->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UsePCC"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->usePCC->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UseTerra"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useTerra)));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UseOcean"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useOcean)));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ShadowGlobalBias"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->shadowGlobalBias->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ShadowGlobalNormalOffset"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->shadowGlobalNormalOffset->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ShadowPSSMLambda"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->shadowPSSMLambda->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ShadowSplitBlend"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->shadowSplitBlend->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ShadowSplitFade"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->shadowSplitFade->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ShadowSplitPadding"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->shadowSplitPadding->getReal())));
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
		if (false == notSimulating)
		{
			auto closureFunction = [this](Ogre::Real renderDt)
			{
				if (nullptr != this->hlmsWind)
				{
					this->hlmsWind->addTime(renderDt);
				}

				// Switch workspace graph when the camera crosses the water line.
				// We only rebuild when the state toggles to avoid heavy work every frame.
				const bool underwaterNow = this->canUseOcean && this->oceanComponent && this->oceanComponent->isCameraUnderwater();
				if (underwaterNow != this->oceanUnderwaterActive)
				{
					this->oceanUnderwaterActive = underwaterNow;
					this->createWorkspace();
				}

				if (true == underwaterNow)
				{
					if (this->underwaterMaterial.isNull())
					{
						return;
					}

					Ogre::Pass* pass = this->underwaterMaterial->getTechnique(0)->getPass(0);
					Ogre::GpuProgramParametersSharedPtr psParams = pass->getFragmentProgramParameters();

					// Update projection params from camera
					Ogre::Vector2 projectionAB = this->cameraComponent->getCamera()->getProjectionParamsAB();
					projectionAB.y /= this->cameraComponent->getFarClipDistance();

					psParams->setNamedConstant("projectionParams", projectionAB);
				}
			};
			Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
			NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
		}
	}

	void WorkspaceBaseComponent::reconnectAllNodes(void)
	{
		if (nullptr != this->workspace)
		{
			if (true == AppStateManager::getSingletonPtr()->bShutdown)
			{
				return;
			}

			Ogre::CompositorWorkspaceDef* workspaceDef = this->workspace->getCompositorManager()->getWorkspaceDefinition(this->workspaceName);

			workspaceDef->clearAllInterNodeConnections();

			unsigned short channelRT0 = 0;
			unsigned short channelRT1 = 1;
			unsigned short channel = 2;

			unsigned short channelOldLumRt = 0;
			unsigned short channelDistortion = 0;
			unsigned short channelGBufferNormals = 0;
			unsigned short channelDepthTexture = 0;

			bool oceanUnderwater = this->canUseOcean && this->oceanComponent && this->oceanComponent->isCameraUnderwater();

			if (true == this->useHdr->getBool())
			{
				channelOldLumRt = channel++;
			}

			if (true == this->useDistortion->getBool())
			{
				channelDistortion = channel++;
			}

			if (true == this->useSSAO->getBool())
			{
				channelGBufferNormals = channel++;
			}

			// Depth texture is always the last channel
			channelDepthTexture = channel++;

			Ogre::IdString msaaNodeName = "NOWAHdrMsaaResolve";
			Ogre::IdString hdrPostProcessingNodeName;
			Ogre::IdString distortionNodeName = this->getDistortionNode();
			Ogre::IdString underwaterNodeName = this->underwaterNodeName;

			if (true == this->useHdr->getBool())
			{
				hdrPostProcessingNodeName = "NOWAHdrPostprocessingNode";
			}

			Ogre::IdString finalRenderNodeName = this->getFinalRenderingNodeName();

			const Ogre::CompositorNodeVec& nodes = this->workspace->getNodeSequence();

			Ogre::IdString lastInNode;
			Ogre::CompositorNodeVec::const_iterator it = nodes.begin();
			Ogre::CompositorNodeVec::const_iterator en = nodes.end();

			while (it != en)
			{
				Ogre::CompositorNode* outNode = *it;
				Ogre::IdString outNodeName = outNode->getName();

				if (outNode->getEnabled() &&
					outNodeName != finalRenderNodeName &&
					outNodeName != hdrPostProcessingNodeName &&
					outNodeName != msaaNodeName &&
					outNodeName != distortionNodeName &&
					outNodeName != underwaterNodeName)
				{
					Ogre::CompositorNodeVec::const_iterator it2 = it + 1;

					while (it2 != en &&
						(false == (*it2)->getEnabled() ||
							(*it2)->getName() == finalRenderNodeName ||
							(*it2)->getName() == hdrPostProcessingNodeName ||
							(*it2)->getName() == msaaNodeName ||
							(*it2)->getName() == distortionNodeName ||
							(*it2)->getName() == underwaterNodeName))
					{
						it2++;
						if (it2 == en)
						{
							break;
						}
					}

					if (it2 != en)
					{
						lastInNode = (*it2)->getName();
						workspaceDef->connect(outNodeName, 0, lastInNode, 0);
						workspaceDef->connect(outNodeName, 1, lastInNode, 1);
					}

					it = it2 - 1;
				}

				it++;
			}

			if (lastInNode == Ogre::IdString())
			{
				lastInNode = this->renderingNodeName;
			}

			// Distortion connections
			if (true == this->useDistortion->getBool())
			{
				workspaceDef->connect(lastInNode, 0, distortionNodeName, 0);
				workspaceDef->connect(this->renderingNodeName, channelDistortion, distortionNodeName, 1);
			}

			// =====================================================================
			// SSAO and depthTexture connections to finalRenderNode
			// =====================================================================
			if (true == this->useSSAO->getBool())
			{
				// SSAO needs both gBufferNormals (channel 2) and depthTexture (channel 3)
				workspaceDef->connect(this->renderingNodeName, channelGBufferNormals, finalRenderNodeName, 2);
				workspaceDef->connect(this->renderingNodeName, channelDepthTexture, finalRenderNodeName, 3);
			}
			else
			{
				// No SSAO, but finalRenderNode still expects depthTexture on channel 2
				workspaceDef->connect(this->renderingNodeName, channelDepthTexture, finalRenderNodeName, 2);
			}

			// =====================================================================
			// Main pipeline connections
			// =====================================================================
			if (false == this->useHdr->getBool())
			{
				const Ogre::IdString& sceneOutputNode = (true == this->useDistortion->getBool()) ? distortionNodeName : lastInNode;

				if (true == oceanUnderwater)
				{
					workspaceDef->connect(sceneOutputNode, 0, underwaterNodeName, 0);
					workspaceDef->connect(this->renderingNodeName, channelDepthTexture, underwaterNodeName, 1);
					workspaceDef->connect(underwaterNodeName, 0, finalRenderNodeName, 1);
				}
				else
				{
					workspaceDef->connect(sceneOutputNode, 0, finalRenderNodeName, 1);
				}
			}
			else
			{
				if (1 == this->msaaLevel)
				{
					workspaceDef->connect(this->renderingNodeName, channelOldLumRt, hdrPostProcessingNodeName, 1);

					if (true == this->useDistortion->getBool())
					{
						workspaceDef->connect(distortionNodeName, 0, hdrPostProcessingNodeName, 0);
					}
					else
					{
						workspaceDef->connect(lastInNode, 0, hdrPostProcessingNodeName, 0);
					}

					workspaceDef->connect(this->renderingNodeName, channelRT1, hdrPostProcessingNodeName, 2);

					if (true == oceanUnderwater)
					{
						workspaceDef->connect(hdrPostProcessingNodeName, 0, underwaterNodeName, 0);
						workspaceDef->connect(this->renderingNodeName, channelDepthTexture, underwaterNodeName, 1);
						workspaceDef->connect(underwaterNodeName, 0, finalRenderNodeName, 1);
					}
					else
					{
						workspaceDef->connect(hdrPostProcessingNodeName, 0, finalRenderNodeName, 1);
					}
				}
				else
				{
					if (true == this->useDistortion->getBool())
					{
						workspaceDef->connect(distortionNodeName, 0, msaaNodeName, 0);
					}
					else
					{
						workspaceDef->connect(lastInNode, 0, msaaNodeName, 0);
					}

					workspaceDef->connect(this->renderingNodeName, channelOldLumRt, msaaNodeName, 1);
					workspaceDef->connect("NOWAHdrMsaaResolve", 0, hdrPostProcessingNodeName, 0);
					workspaceDef->connect(this->renderingNodeName, channelOldLumRt, hdrPostProcessingNodeName, 1);
					workspaceDef->connect(this->renderingNodeName, channelRT1, hdrPostProcessingNodeName, 2);

					if (true == oceanUnderwater)
					{
						workspaceDef->connect(hdrPostProcessingNodeName, 0, underwaterNodeName, 0);
						workspaceDef->connect(this->renderingNodeName, channelDepthTexture, underwaterNodeName, 1);
						workspaceDef->connect(underwaterNodeName, 0, finalRenderNodeName, 1);
					}
					else
					{
						workspaceDef->connect(hdrPostProcessingNodeName, 0, finalRenderNodeName, 1);
					}
				}
			}

			workspaceDef->connectExternal(0, finalRenderNodeName, 0);

			this->workspace->reconnectAllNodes();
		}
	}

	void WorkspaceBaseComponent::baseCreateWorkspace(Ogre::CompositorWorkspaceDef* workspaceDef)
	{
		if (true == this->useMSAA->getBool())
		{
			this->msaaLevel = this->getMSAA();
		}
		else
		{
			this->msaaLevel = 1;
		}

		bool oceanUnderwater = this->canUseOcean && this->oceanComponent && this->oceanComponent->isCameraUnderwater();

		if (true == oceanUnderwater)
		{
			this->createUnderwaterNode();
		}

		// Calculate output channel indices based on enabled features
		unsigned short channelRT0 = 0;
		unsigned short channelRT1 = 1;
		unsigned short channel = 2;

		unsigned short channelOldLumRt = 0;
		unsigned short channelDistortion = 0;
		unsigned short channelGBufferNormals = 0;
		unsigned short channelDepthTexture = 0;

		if (true == this->useHdr->getBool())
		{
			channelOldLumRt = channel++;
		}

		if (true == this->useDistortion->getBool())
		{
			channelDistortion = channel++;
		}

		if (true == this->useSSAO->getBool())
		{
			channelGBufferNormals = channel++;
		}

		// Depth texture is always the last channel
		channelDepthTexture = channel++;

		// Distortion connections
		if (true == this->useDistortion->getBool())
		{
			workspaceDef->connect(this->renderingNodeName, channelRT0, this->distortionNode, 0);
			workspaceDef->connect(this->renderingNodeName, channelDistortion, this->distortionNode, 1);
		}

		// =========================================================================
		// SSAO and depthTexture connections to finalRenderNode
		// =========================================================================
		if (true == this->useSSAO->getBool())
		{
			// SSAO needs both gBufferNormals (channel 2) and depthTexture (channel 3)
			workspaceDef->connect(this->renderingNodeName, channelGBufferNormals, this->finalRenderingNodeName, 2);
			workspaceDef->connect(this->renderingNodeName, channelDepthTexture, this->finalRenderingNodeName, 3);
		}
		else
		{
			// No SSAO, but finalRenderNode still expects depthTexture on channel 2
			// (because createFinalRenderNode always adds it as input)
			workspaceDef->connect(this->renderingNodeName, channelDepthTexture, this->finalRenderingNodeName, 2);
		}

		// Without Hdr
		if (false == this->useHdr->getBool())
		{
			const Ogre::String& sceneOutputNode = (true == this->useDistortion->getBool()) ? this->distortionNode : this->renderingNodeName;

			if (true == oceanUnderwater)
			{
				workspaceDef->connect(sceneOutputNode, 0, this->underwaterNodeName, 0);
				workspaceDef->connect(this->renderingNodeName, channelDepthTexture, this->underwaterNodeName, 1);
				workspaceDef->connect(this->underwaterNodeName, 0, this->finalRenderingNodeName, 1);
			}
			else
			{
				workspaceDef->connect(sceneOutputNode, 0, this->finalRenderingNodeName, 1);
			}

			workspaceDef->connectExternal(0, this->finalRenderingNodeName, 0);
		}
		else
		{
			if (1 == this->msaaLevel)
			{
				if (true == this->useDistortion->getBool())
				{
					workspaceDef->connect(this->distortionNode, 0, "NOWAHdrPostprocessingNode", 0);
				}
				else
				{
					workspaceDef->connect(this->renderingNodeName, channelRT0, "NOWAHdrPostprocessingNode", 0);
				}

				workspaceDef->connect(this->renderingNodeName, channelOldLumRt, "NOWAHdrPostprocessingNode", 1);
				workspaceDef->connect(this->renderingNodeName, channelRT1, "NOWAHdrPostprocessingNode", 2);

				if (true == oceanUnderwater)
				{
					workspaceDef->connect("NOWAHdrPostprocessingNode", 0, this->underwaterNodeName, 0);
					workspaceDef->connect(this->renderingNodeName, channelDepthTexture, this->underwaterNodeName, 1);
					workspaceDef->connect(this->underwaterNodeName, 0, this->finalRenderingNodeName, 1);
				}
				else
				{
					workspaceDef->connect("NOWAHdrPostprocessingNode", 0, this->finalRenderingNodeName, 1);
				}

				workspaceDef->connectExternal(0, this->finalRenderingNodeName, 0);
			}
			else
			{
				if (true == this->useDistortion->getBool())
				{
					workspaceDef->connect(this->distortionNode, 0, "NOWAHdrMsaaResolve", 0);
				}
				else
				{
					workspaceDef->connect(this->renderingNodeName, channelRT0, "NOWAHdrMsaaResolve", 0);
				}

				workspaceDef->connect(this->renderingNodeName, channelOldLumRt, "NOWAHdrMsaaResolve", 1);
				workspaceDef->connect("NOWAHdrMsaaResolve", 0, "NOWAHdrPostprocessingNode", 0);
				workspaceDef->connect(this->renderingNodeName, channelOldLumRt, "NOWAHdrPostprocessingNode", 1);
				workspaceDef->connect(this->renderingNodeName, channelRT1, "NOWAHdrPostprocessingNode", 2);

				if (true == oceanUnderwater)
				{
					workspaceDef->connect("NOWAHdrPostprocessingNode", 0, this->underwaterNodeName, 0);
					workspaceDef->connect(this->renderingNodeName, channelDepthTexture, this->underwaterNodeName, 1);
					workspaceDef->connect(this->underwaterNodeName, 0, this->finalRenderingNodeName, 1);
				}
				else
				{
					workspaceDef->connect("NOWAHdrPostprocessingNode", 0, this->finalRenderingNodeName, 1);
				}

				workspaceDef->connectExternal(0, this->finalRenderingNodeName, 0);
			}
		}
	}

	void WorkspaceBaseComponent::createFinalRenderNode(void)
	{
		if (true == this->compositorManager->hasNodeDefinition(this->finalRenderingNodeName))
		{
			this->compositorManager->removeNodeDefinition(this->finalRenderingNodeName);
		}

		if (false == this->compositorManager->hasNodeDefinition(this->finalRenderingNodeName))
		{
			Ogre::CompositorNodeDef* finalNodeDef = compositorManager->addNodeDefinition(this->finalRenderingNodeName);

			unsigned short numLocalTextures = 0;

			// SSAO Texture definitions:
			if (true == this->useSSAO->getBool())
			{
				numLocalTextures = 4; // depthTextureCopy, ssaoTexture, blurH, blurV
			}

			if (numLocalTextures > 0)
			{
				finalNodeDef->setNumLocalTextureDefinitions(numLocalTextures);
			}

			if (true == this->useSSAO->getBool())
			{
				auto* depthCopyDef = finalNodeDef->addTextureDefinition("depthTextureCopy");
				depthCopyDef->width = 0;
				depthCopyDef->height = 0;
				depthCopyDef->widthFactor = 0.5f;
				depthCopyDef->heightFactor = 0.5f;
				depthCopyDef->format = Ogre::PFG_R16_FLOAT;
				depthCopyDef->depthBufferId = Ogre::DepthBuffer::POOL_NO_DEPTH;
				depthCopyDef->textureFlags = Ogre::TextureFlags::RenderToTexture;

				auto* ssaoDef = finalNodeDef->addTextureDefinition("ssaoTexture");
				ssaoDef->width = 0;
				ssaoDef->height = 0;
				ssaoDef->widthFactor = 0.5f;
				ssaoDef->heightFactor = 0.5f;
				ssaoDef->format = Ogre::PFG_R16_FLOAT;
				ssaoDef->depthBufferId = Ogre::DepthBuffer::POOL_NO_DEPTH;
				ssaoDef->textureFlags = Ogre::TextureFlags::RenderToTexture;

				auto* blurHDef = finalNodeDef->addTextureDefinition("blurTextureHorizontal");
				blurHDef->width = 0;
				blurHDef->height = 0;
				blurHDef->widthFactor = 1.0f;
				blurHDef->heightFactor = 1.0f;
				blurHDef->format = Ogre::PFG_R16_FLOAT;
				blurHDef->depthBufferId = Ogre::DepthBuffer::POOL_NO_DEPTH;
				blurHDef->textureFlags = Ogre::TextureFlags::RenderToTexture;

				auto* blurVDef = finalNodeDef->addTextureDefinition("blurTextureVertical");
				blurVDef->width = 0;
				blurVDef->height = 0;
				blurVDef->widthFactor = 1.0f;
				blurVDef->heightFactor = 1.0f;
				blurVDef->format = Ogre::PFG_R16_FLOAT;
				blurVDef->depthBufferId = Ogre::DepthBuffer::POOL_NO_DEPTH;
				blurVDef->textureFlags = Ogre::TextureFlags::RenderToTexture;
			}

			// Input textures
			unsigned int textureIndex = 0;

			finalNodeDef->addTextureSourceName("rt_output", textureIndex++, Ogre::TextureDefinitionBase::TEXTURE_INPUT);
			finalNodeDef->addTextureSourceName("rtN", textureIndex++, Ogre::TextureDefinitionBase::TEXTURE_INPUT);

			if (true == this->useSSAO->getBool())
			{
				finalNodeDef->addTextureSourceName("gBufferNormals", textureIndex++, Ogre::TextureDefinitionBase::TEXTURE_INPUT);
			}

			finalNodeDef->addTextureSourceName("depthTexture", textureIndex++, Ogre::TextureDefinitionBase::TEXTURE_INPUT);

			if (true == this->useSSAO->getBool())
			{
				Ogre::RenderTargetViewDef* rtv;
				Ogre::RenderTargetViewEntry attachment;

				rtv = finalNodeDef->addRenderTextureView("depthTextureCopy");
				attachment.textureName = "depthTextureCopy";
				rtv->colourAttachments.push_back(attachment);
				rtv->depthBufferId = Ogre::DepthBuffer::POOL_NO_DEPTH;

				rtv = finalNodeDef->addRenderTextureView("ssaoTexture");
				attachment.textureName = "ssaoTexture";
				rtv->colourAttachments.push_back(attachment);
				rtv->depthBufferId = Ogre::DepthBuffer::POOL_NO_DEPTH;

				rtv = finalNodeDef->addRenderTextureView("blurTextureHorizontal");
				attachment.textureName = "blurTextureHorizontal";
				rtv->colourAttachments.push_back(attachment);
				rtv->depthBufferId = Ogre::DepthBuffer::POOL_NO_DEPTH;

				rtv = finalNodeDef->addRenderTextureView("blurTextureVertical");
				attachment.textureName = "blurTextureVertical";
				rtv->colourAttachments.push_back(attachment);
				rtv->depthBufferId = Ogre::DepthBuffer::POOL_NO_DEPTH;
			}

			// Target passes
			unsigned short numTargetPass = 1;

			if (true == this->useSSAO->getBool())
			{
				numTargetPass += 4;
			}

			finalNodeDef->setNumTargetPass(numTargetPass);

			if (true == this->useSSAO->getBool())
			{
				// 1. Depth downsampling
				{
					Ogre::CompositorTargetDef* targetDef = finalNodeDef->addTargetPass("depthTextureCopy");

					Ogre::CompositorPassQuadDef* passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));

					passQuad->setAllLoadActions(Ogre::LoadAction::DontCare);
					passQuad->mMaterialName = (this->msaaLevel > 1)
						? "Ogre/Depth/DownscaleMax_Subsample0"
						: "Ogre/Depth/DownscaleMax";
					passQuad->addQuadTextureSource(0, "depthTexture");
					passQuad->mProfilingId = "NOWA_Final_SSAO_DepthDownscale_Pass_Quad";
				}

				// 2. SSAO generation
				{
					Ogre::CompositorTargetDef* targetDef = finalNodeDef->addTargetPass("ssaoTexture");

					Ogre::CompositorPassQuadDef* passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));

					passQuad->setAllLoadActions(Ogre::LoadAction::Clear);
					passQuad->mClearColour[0] = Ogre::ColourValue::White;
					passQuad->mMaterialName = "SSAO/HS";
					passQuad->addQuadTextureSource(0, "depthTextureCopy");
					passQuad->addQuadTextureSource(1, "gBufferNormals");
					passQuad->mFrustumCorners = Ogre::CompositorPassQuadDef::VIEW_SPACE_CORNERS;
					passQuad->mProfilingId = "NOWA_Final_SSAO_Generation_Pass_Quad";
				}

				// 3. Horizontal blur
				{
					Ogre::CompositorTargetDef* targetDef = finalNodeDef->addTargetPass("blurTextureHorizontal");

					Ogre::CompositorPassQuadDef* passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));

					passQuad->setAllLoadActions(Ogre::LoadAction::DontCare);
					passQuad->mMaterialName = "SSAO/BlurH";
					passQuad->addQuadTextureSource(0, "ssaoTexture");
					passQuad->addQuadTextureSource(1, "depthTextureCopy");
					passQuad->mProfilingId = "NOWA_Final_SSAO_BlurH_Pass_Quad";
				}

				// 4. Vertical blur
				{
					Ogre::CompositorTargetDef* targetDef = finalNodeDef->addTargetPass("blurTextureVertical");

					Ogre::CompositorPassQuadDef* passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));

					passQuad->setAllLoadActions(Ogre::LoadAction::DontCare);
					passQuad->mMaterialName = "SSAO/BlurV";
					passQuad->addQuadTextureSource(0, "blurTextureHorizontal");
					passQuad->addQuadTextureSource(1, "depthTextureCopy");
					passQuad->mProfilingId = "NOWA_Final_SSAO_BlurV_Pass_Quad";
				}
			}

			// rt_output target
			{
				Ogre::CompositorTargetDef* targetDef = finalNodeDef->addTargetPass("rt_output");

				if (true == this->useSSAO->getBool())
				{
					// Apply SSAO directly
					Ogre::CompositorPassQuadDef* passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));

					passQuad->setAllLoadActions(Ogre::LoadAction::DontCare);
					passQuad->mStoreActionDepth = Ogre::StoreAction::DontCare;
					passQuad->mStoreActionStencil = Ogre::StoreAction::DontCare;
					passQuad->mMaterialName = "SSAO/Apply";
					passQuad->addQuadTextureSource(0, "blurTextureVertical");
					passQuad->addQuadTextureSource(1, "rtN");
					passQuad->mProfilingId = "NOWA_Final_SSAO_Apply_Pass_Quad";
				}
				else
				{
					// Copy rtN to rt_output
					Ogre::CompositorPassQuadDef* passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));

					passQuad->setAllLoadActions(Ogre::LoadAction::DontCare);
					passQuad->mMaterialName = "Ogre/Copy/4xFP32";
					passQuad->mStoreActionDepth = Ogre::StoreAction::DontCare;
					passQuad->mStoreActionStencil = Ogre::StoreAction::DontCare;
					passQuad->addQuadTextureSource(0, "rtN");
					passQuad->mProfilingId = "NOWA_Final_rtN_Pass_Quad";
				}

				if (false == this->involvedInSplitScreen)
				{
					Ogre::CompositorPassDef* passMyGUI = static_cast<Ogre::CompositorPassDef*>(targetDef->addPass(Ogre::PASS_CUSTOM, "MYGUI"));
					passMyGUI->mProfilingId = "NOWA_Final_Render_MyGUI_Pass_Custom";
				}

				{
					Ogre::CompositorPassSceneDef* passScene = static_cast<Ogre::CompositorPassSceneDef*>(targetDef->addPass(Ogre::PASS_SCENE));

					passScene->mProfilingId = "NOWA_Final_Render_Overlay_Pass_Scene";
					passScene->mUpdateLodLists = false;
					passScene->mIncludeOverlays = true;
					passScene->mStoreActionDepth = Ogre::StoreAction::DontCare;
					passScene->mStoreActionStencil = Ogre::StoreAction::DontCare;
					passScene->mFirstRQ = 254;
					passScene->mLastRQ = 255;
				}
			}
		}
	}

	void WorkspaceBaseComponent::createDistortionNode(void)
	{
		if (true == this->useDistortion->getBool())
		{
			if (false == this->compositorManager->hasNodeDefinition(this->distortionNode))
			{
				ENQUEUE_RENDER_COMMAND_WAIT("WorkspaceBaseComponent::createDistortionNode",
				{
					Ogre::CompositorNodeDef * compositorNodeDefinition = compositorManager->addNodeDefinition(this->distortionNode);

					// rt_output is the new in texture, instead of like in the script in 2 rt_output whichs is externally connected
					Ogre::TextureDefinitionBase::TextureDefinition * distortionTexDef = compositorNodeDefinition->addTextureDefinition("rt_output");
					distortionTexDef->width = 0.0f; // target_width
					distortionTexDef->height = 0.0f; // target_height
					distortionTexDef->format = Ogre::PFG_RGBA16_FLOAT;
					distortionTexDef->depthBufferFormat = Ogre::PFG_D32_FLOAT;
					distortionTexDef->preferDepthTexture = true;
					// Attention depth_pool?
					distortionTexDef->depthBufferId = 2;
					distortionTexDef->textureFlags = Ogre::TextureFlags::RenderToTexture;

					compositorNodeDefinition->addTextureSourceName("rt0", 0, Ogre::TextureDefinitionBase::TEXTURE_INPUT);
					compositorNodeDefinition->addTextureSourceName("rt_distortion", 1, Ogre::TextureDefinitionBase::TEXTURE_INPUT);

					Ogre::RenderTargetViewDef * rtv = compositorNodeDefinition->addRenderTextureView("rt_output");
					Ogre::RenderTargetViewEntry attachment;
					attachment.textureName = "rt_output";
					rtv->colourAttachments.push_back(attachment);

					unsigned short numTargetPass = 1;
					compositorNodeDefinition->setNumTargetPass(numTargetPass);

					{
						Ogre::CompositorTargetDef* targetDef = compositorNodeDefinition->addTargetPass("rt_output");

						// Render Quad
						{
							Ogre::CompositorPassQuadDef* passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));
							passQuad->setAllLoadActions(Ogre::LoadAction::DontCare);
							passQuad->mMaterialName = "Distortion/Quad";
							passQuad->addQuadTextureSource(0, "rt0");
							passQuad->addQuadTextureSource(1, "rt_distortion");

							passQuad->mProfilingId = "NOWA_Distortion_Renter_Target_Pass_Quad";
						}
					}

					compositorNodeDefinition->mapOutputChannel(0, "rt_output");
					compositorNodeDefinition->mapOutputChannel(1, "rt0");
				});
			}
		}
	}

	void WorkspaceBaseComponent::createUnderwaterNode(void)
	{
		// This node is only needed when an OceanComponent is active and the camera is underwater.
		if (false == this->canUseOcean || nullptr == this->oceanComponent)
			return;
		if (false == this->oceanComponent->isCameraUnderwater())
			return;
		if (this->underwaterNodeName.empty())
			return;

		if (true == this->compositorManager->hasNodeDefinition(this->underwaterNodeName))
		{
			this->compositorManager->removeNodeDefinition(this->underwaterNodeName);
		}

		Ogre::CompositorNodeDef* compositorNodeDefinition = compositorManager->addNodeDefinition(this->underwaterNodeName);

		// =========================================================================
		// INPUT CHANNELS - We need 2 inputs: rt0 (color) and depthTexture
		// =========================================================================
		unsigned int textureIndex = 0;
		compositorNodeDefinition->addTextureSourceName("rt0", textureIndex++, Ogre::TextureDefinitionBase::TEXTURE_INPUT);
		compositorNodeDefinition->addTextureSourceName("depthTexture", textureIndex++, Ogre::TextureDefinitionBase::TEXTURE_INPUT);

		// Output texture
		Ogre::TextureDefinitionBase::TextureDefinition* outTexDef = compositorNodeDefinition->addTextureDefinition("rt_output");
		outTexDef->width = 0.0f;  // target_width
		outTexDef->height = 0.0f; // target_height
		outTexDef->format = Ogre::PFG_RGBA16_FLOAT;
		outTexDef->textureFlags = Ogre::TextureFlags::RenderToTexture;

		// Add a RenderTargetViewDef for rt_output
		Ogre::RenderTargetViewDef* rtv = compositorNodeDefinition->addRenderTextureView("rt_output");
		Ogre::RenderTargetViewEntry attachment;
		attachment.textureName = "rt_output";
		rtv->colourAttachments.push_back(attachment);
		rtv->depthBufferId = Ogre::DepthBuffer::POOL_NO_DEPTH;  // Post-process doesn't need depth

		compositorNodeDefinition->setNumTargetPass(1u);
		Ogre::CompositorTargetDef* targetDef = compositorNodeDefinition->addTargetPass("rt_output");

		// Render Quad with underwater post-process
		{
			Ogre::CompositorPassQuadDef* passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));
			passQuad->setAllLoadActions(Ogre::LoadAction::DontCare);
			passQuad->mMaterialName = "Ocean/UnderwaterPost";

			// Texture inputs: rt0 (color) at slot 0, depthTexture at slot 1
			passQuad->addQuadTextureSource(0u, "rt0");
			passQuad->addQuadTextureSource(1u, "depthTexture");

			passQuad->mProfilingId = "NOWA_Underwater_Post_Pass_Quad";
		}

		// Output channel
		compositorNodeDefinition->setNumOutputChannels(1u);
		compositorNodeDefinition->mapOutputChannel(0u, "rt_output");

		// =========================================================================
		// SET projectionParams for depth linearization
		// =========================================================================
		this->underwaterMaterial = std::static_pointer_cast<Ogre::Material>(Ogre::MaterialManager::getSingleton().load("Ocean/UnderwaterPost",Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME));

		if (false == this->underwaterMaterial.isNull())
		{
			Ogre::Pass* pass = this->underwaterMaterial->getTechnique(0)->getPass(0);
			Ogre::GpuProgramParametersSharedPtr psParams = pass->getFragmentProgramParameters();

			// Get projection params from camera - SAME AS SSAO!
			Ogre::Vector2 projectionAB = this->cameraComponent->getCamera()->getProjectionParamsAB();
			// The division keeps "linearDepth" in a normalized range
			projectionAB.y /= this->cameraComponent->getFarClipDistance();

			psParams->setNamedConstant("projectionParams", projectionAB);
		}
	}

	void WorkspaceBaseComponent::addWorkspace(Ogre::CompositorWorkspaceDef* workspaceDef)
	{
		// Threadsafe from the outside
		if (true == this->externalChannels.empty())
		{
			this->externalChannels.resize(1);
			this->externalChannels[0] = Core::getSingletonPtr()->getOgreRenderWindow()->getTexture();
		}

		if (false == this->customExternalChannels.empty() && true == this->involvedInSplitScreen)
		{
			size_t priorCount = 0;
			// Note: Just eat the Core::getSingletonPtr()->getOgreRenderWindow()->getTexture(), because its surely used differently via customExternalChannels, but take all other existing channels, (e.g. cubeMap, terra etc.)
			if (this->externalChannels.size() > 0)
			{
				priorCount = this->externalChannels.size() - 1;
			}
			this->externalChannels.resize(priorCount + this->customExternalChannels.size());
			for (size_t i = 0; i < this->customExternalChannels.size(); i++)
			{
				if (i >= priorCount)
				{
					this->externalChannels[i] = this->customExternalChannels[i];
				}
			}
		}

		int position = -1;

		if (true == this->involvedInSplitScreen)
		{
			position = 0;
		}
 		this->workspace = WorkspaceModule::getInstance()->getCompositorManager()->addWorkspace(this->gameObjectPtr->getSceneManager(), this->externalChannels, this->cameraComponent->getCamera(), this->workspaceName, true, position);
	}

	Ogre::String WorkspaceBaseComponent::getDistortionNode(void) const
	{
		return this->distortionNode;
	}

	Ogre::String WorkspaceBaseComponent::getUnderwaterNode(void) const
	{
		return this->underwaterNodeName;
	}

	void WorkspaceBaseComponent::changeBackgroundColor(const Ogre::ColourValue& backgroundColor)
	{
		// Attention: Does not work, because clearDef->mColourValue = backgroundColor; is no more available
		if (nullptr == this->workspace)
		{
			return;
		}

		Ogre::CompositorManager2::CompositorNodeDefMap nodeDefinitions = this->compositorManager->getNodeDefinitions();
		Ogre::CompositorNodeDef * nodeDef;
		Ogre::CompositorTargetDef * targetDef;
		Ogre::CompositorPassDef * passDef;
		Ogre::CompositorPassClearDef * clearDef;

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

	unsigned char WorkspaceBaseComponent::getMSAA(void)
	{
		const Ogre::RenderSystemCapabilities* caps = Core::getSingletonPtr()->getOgreRoot()->getRenderSystem()->getCapabilities();

		// If MSAA is set more than default in ogre.cfg and graphics card has (MultiSampleAntiAliasing) support, use it!
		// Possible values are 1,2,4,8
		// 1 is used when its disabled
		unsigned int multiSample = Core::getSingletonPtr()->getOgreRenderWindow()->getSampleDescription().getColourSamples();
		if (multiSample > 1u && caps->hasCapability(Ogre::RSC_EXPLICIT_FSAA_RESOLVE))
		{
			return multiSample;
		}

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

		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("WorkspaceBaseComponent::initializeHdr", _1(fsaa),
		{
			Ogre::String preprocessorDefines = "MSAA_INITIALIZED=1,";

			preprocessorDefines += "MSAA_SUBSAMPLE_WEIGHT=";
			preprocessorDefines += Ogre::StringConverter::toString(1.0f / (float)fsaa);
			preprocessorDefines += ",MSAA_NUM_SUBSAMPLES=";
			preprocessorDefines += Ogre::StringConverter::toString(fsaa);

			Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().load(
				"HDR/Resolve_4xFP32_HDR_Box",
				Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME).
				staticCast<Ogre::Material>();

			Ogre::Pass * pass = material->getTechnique(0)->getPass(0);

			Ogre::GpuProgram * shader = 0;
			Ogre::GpuProgramParametersSharedPtr oldParams;

			//Save old manual & auto params
			oldParams = pass->getFragmentProgramParameters();
			//Retrieve the HLSL/GLSL/Metal shader and rebuild it with the right settings.
			shader = pass->getFragmentProgram()->_getBindingDelegate();
			shader->setParameter("preprocessor_defines", preprocessorDefines);
			pass->getFragmentProgram()->reload();
			//Restore manual & auto params to the newly compiled shader
			pass->getFragmentProgramParameters()->copyConstantsFrom(*oldParams);
		});
	}

	void WorkspaceBaseComponent::createCompositorEffectsFromCode(void)
	{
		ENQUEUE_RENDER_COMMAND_WAIT("WorkspaceBaseComponent::createCompositorEffectsFromCode",
		{
			Ogre::Root * root = Core::getSingletonPtr()->getOgreRoot();

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
						auto pass = targetDef->addPass(Ogre::PASS_QUAD);
						Ogre::CompositorPassQuadDef* passQuad = static_cast<Ogre::CompositorPassQuadDef*>(pass);

						passQuad->setAllLoadActions(Ogre::LoadAction::DontCare);
						passQuad->mMaterialName = "Postprocess/BrightPass2";
						passQuad->addQuadTextureSource(0, "rt_input");
						passQuad->mProfilingId = "NOWA_Post_Effect_Bright_Pass2_Pass_Quad";
					}
				}
				{
					Ogre::CompositorTargetDef* targetDef = bloomDef->addTargetPass("rt1");

					{
						auto pass = targetDef->addPass(Ogre::PASS_QUAD);
						Ogre::CompositorPassQuadDef* passQuad = static_cast<Ogre::CompositorPassQuadDef*>(pass);

						passQuad->setAllLoadActions(Ogre::LoadAction::DontCare);
						passQuad->mMaterialName = "Postprocess/BlurV";
						passQuad->addQuadTextureSource(0, "rt0");

						passQuad->mProfilingId = "NOWA_Post_Effect_BlurV_Pass_Quad";
					}
				}
				{
					Ogre::CompositorTargetDef* targetDef = bloomDef->addTargetPass("rt0");

					{
						auto pass = targetDef->addPass(Ogre::PASS_QUAD);
						Ogre::CompositorPassQuadDef* passQuad = static_cast<Ogre::CompositorPassQuadDef*>(pass);

						passQuad->setAllLoadActions(Ogre::LoadAction::DontCare);
						passQuad->mMaterialName = "Postprocess/BlurH";
						passQuad->addQuadTextureSource(0, "rt1");

						passQuad->mProfilingId = "NOWA_Post_Effect_BlurH_Pass_Quad";
					}
				}
				{
					Ogre::CompositorTargetDef* targetDef = bloomDef->addTargetPass("rt_output");

					{
						auto pass = targetDef->addPass(Ogre::PASS_QUAD);
						Ogre::CompositorPassQuadDef* passQuad = static_cast<Ogre::CompositorPassQuadDef*>(pass);

						passQuad->setAllLoadActions(Ogre::LoadAction::DontCare);
						passQuad->mMaterialName = "Postprocess/BloomBlend2";
						passQuad->addQuadTextureSource(0, "rt_input");
						passQuad->addQuadTextureSource(1, "rt0");

						passQuad->mProfilingId = "NOWA_Post_Effect_Bloom_Blend2_Pass_Quad";
					}
				}

				// Output channels
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
						auto pass = targetDef->addPass(Ogre::PASS_QUAD);
						Ogre::CompositorPassQuadDef* passQuad = static_cast<Ogre::CompositorPassQuadDef*>(pass);

						passQuad->setAllLoadActions(Ogre::LoadAction::DontCare);
						passQuad->mMaterialName = "Postprocess/Glass";
						passQuad->addQuadTextureSource(0, "rt_input");

						passQuad->mProfilingId = "NOWA_Post_Effect_Glass_Pass_Quad";
					}
				}

				// Output channels
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
						auto pass = targetDef->addPass(Ogre::PASS_QUAD);
						Ogre::CompositorPassQuadDef* passQuad = static_cast<Ogre::CompositorPassQuadDef*>(pass);

						passQuad->setAllLoadActions(Ogre::LoadAction::DontCare);
						passQuad->mNumInitialPasses = 1;
						passQuad->mMaterialName = "Ogre/Copy/4xFP32";
						passQuad->addQuadTextureSource(0, "rt_input");

						passQuad->mStoreActionDepth = Ogre::StoreAction::DontCare;
						passQuad->mStoreActionStencil = Ogre::StoreAction::DontCare;

						passQuad->mProfilingId = "NOWA_Post_Effect_Copy_4xFP32_Input_Pass_Quad";
					}
				}
				/// Do the motion blur
				{
					Ogre::CompositorTargetDef* targetDef = motionBlurDef->addTargetPass("rt_output");

					{
						auto pass = targetDef->addPass(Ogre::PASS_QUAD);
						Ogre::CompositorPassQuadDef* passQuad = static_cast<Ogre::CompositorPassQuadDef*>(pass);

						passQuad->setAllLoadActions(Ogre::LoadAction::DontCare);
						passQuad->mMaterialName = "Postprocess/Combine";
						passQuad->addQuadTextureSource(0, "rt_input");
						passQuad->addQuadTextureSource(1, "sum");

						passQuad->mProfilingId = "NOWA_Post_Effect_Combine_Pass_Quad";
					}
				}
				/// Copy back sum texture for the next frame
				{
					Ogre::CompositorTargetDef* targetDef = motionBlurDef->addTargetPass("sum");

					{
						auto pass = targetDef->addPass(Ogre::PASS_QUAD);
						Ogre::CompositorPassQuadDef* passQuad = static_cast<Ogre::CompositorPassQuadDef*>(pass);

						passQuad->setAllLoadActions(Ogre::LoadAction::DontCare);
						passQuad->mMaterialName = "Ogre/Copy/4xFP32";
						passQuad->addQuadTextureSource(0, "rt_output");

						passQuad->mStoreActionDepth = Ogre::StoreAction::DontCare;
						passQuad->mStoreActionStencil = Ogre::StoreAction::DontCare;

						passQuad->mProfilingId = "NOWA_Post_Effect_Copy_4xFP32_Output_Pass_Quad";
					}
				}

				// Output channels
				motionBlurDef->setNumOutputChannels(2);
				motionBlurDef->mapOutputChannel(0, "rt_output");
				motionBlurDef->mapOutputChannel(1, "rt_input");
			}
		});
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
		// Threadsafe from the outside
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
		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("WorkspaceBaseComponent::setDataBlockPbsReflectionTextureName", _2(gameObject, textureName),
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
		});
	}

	void WorkspaceBaseComponent::setPlanarMaxReflections(unsigned long gameObjectId, bool useAccurateLighting, unsigned int width, unsigned int height, bool withMipmaps, bool useMipmapMethodCompute,
		const Ogre::Vector3& position, const Ogre::Quaternion& orientation, const Ogre::Vector2& mirrorSize)
	{
		if (nullptr != this->planarReflections && false == this->planarReflectionReflectiveWorkspaceName.empty())
		{
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("WorkspaceBaseComponent::setPlanarMaxReflections", _9(gameObjectId, useAccurateLighting, width, height, withMipmaps, useMipmapMethodCompute, position, orientation, mirrorSize),
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

				this->planarReflections->setMaxActiveActors(planarReflectionActorIndex, this->planarReflectionReflectiveWorkspaceName, 
					useAccurateLighting, width, height, withMipmaps, Ogre::PFG_RGBA8_UNORM_SRGB, useMipmapMethodCompute);
			});
		}
	}

	void WorkspaceBaseComponent::addPlanarReflectionsActor(unsigned long gameObjectId, bool useAccurateLighting, unsigned int width, unsigned int height, bool withMipmaps, bool useMipmapMethodCompute,
		const Ogre::Vector3& position, const Ogre::Quaternion& orientation, const Ogre::Vector2& mirrorSize)
	{
		if (nullptr != this->planarReflections && false == this->planarReflectionReflectiveWorkspaceName.empty())
		{
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("WorkspaceBaseComponent::addPlanarReflectionsActor", _9(gameObjectId, useAccurateLighting, width, height, withMipmaps, useMipmapMethodCompute, position, orientation, mirrorSize),
			{
				bool foundGameObjectId = false;
				unsigned int planarReflectionActorIndex = 1;
				Ogre::PlanarReflectionActor * existingActor = nullptr;
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

						// Not necessary yet, but just for the future: https://forums.ogre3d.org/viewtopic.php?t=97019
						/*GameObjectPtr reflectionCameraGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->reflectionCameraGameObjectId->getULong());
						if (nullptr != reflectionCameraGameObjectPtr)
						{
							auto& reflectionCameraCompPtr = NOWA::makeStrongPtr(reflectionCameraGameObjectPtr->getComponent<ReflectionCameraComponent>());

							/*if (nullptr != reflectionCameraCompPtr)
							{
								Ogre::UserObjectBindings& cameraBindings(static_cast<Ogre::MovableObject*>(reflectionCameraCompPtr->getCamera())->getUserObjectBindings());
								cameraBindings.setUserAny("Slot", Ogre::Any(actor->getCurrentBoundSlot()));
							}*/

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
			});
		}
	}

	void WorkspaceBaseComponent::removePlanarReflectionsActor(unsigned long gameObjectId)
	{
		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("WorkspaceBaseComponent::removePlanarReflectionsActor", _1(gameObjectId),
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
		});
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
		
		// this->changeViewportRect(0, viewportRect);
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


		this->oceanUnderwaterActive = this->canUseOcean && this->oceanComponent && this->oceanComponent->isCameraUnderwater();

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
			boost::shared_ptr<EventDataHdrActivated> eventDataHdrActivated(new EventDataHdrActivated(gameObjectPtr->getId(), useHdr));
			AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataHdrActivated);

			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("WorkspaceBaseComponent::setUseHdr", _1(useHdr),
			{
				this->gameObjectPtr->getSceneManager()->setAmbientLight(Ogre::ColourValue(0.3f, 0.5f, 0.7f), Ogre::ColourValue(0.6f, 0.45f, 0.3f), this->gameObjectPtr->getSceneManager()->getAmbientLightHemisphereDir());

				// Get the sun light (directional light for sun power setting)
				GameObjectPtr lightGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(GameObjectController::MAIN_LIGHT_ID);

				if (nullptr == lightGameObjectPtr)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[WorkspaceBaseComponent] Could not find 'SunLight' for this component! Affected game object: " + this->gameObjectPtr->getName());
					return;
				}

				// TODO: Test this!
				/*auto& lightDirectionalCompPtr = NOWA::makeStrongPtr(lightGameObjectPtr->getComponent<LightDirectionalComponent>());
				if (nullptr != lightDirectionalCompPtr)
				{
					lightDirectionalCompPtr->setPowerScale(3.14159f);
				}*/
			});
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

	void WorkspaceBaseComponent::setUseSSAO(bool useSSAO)
	{
		this->useSSAO->setValue(useSSAO);

		if (true == useSSAO)
		{
			this->createSSAONoiseTexture();
		}
		
		this->createWorkspace();
	}

	bool WorkspaceBaseComponent::getUseSSAO(void) const
	{
		return this->useSSAO->getBool();
	}

	void WorkspaceBaseComponent::setUseDistortion(bool useDistortion)
	{
		this->useDistortion->setValue(useDistortion);

		this->createWorkspace();
	}

	bool WorkspaceBaseComponent::getUseDistortion(void) const
	{
		return this->useDistortion->getBool();
	}

	void WorkspaceBaseComponent::setUseMSAA(bool useMSAA)
	{
		this->useMSAA->setValue(useMSAA);

		this->createWorkspace();
	}

	bool WorkspaceBaseComponent::getUseMSAA(void) const
	{
		return this->useMSAA->getBool();
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

	void WorkspaceBaseComponent::setUseOcean(bool useOcean, OceanComponent* oceanComponent)
	{
		this->useOcean = useOcean;

		if (true == useOcean)
		{
			this->oceanComponent = oceanComponent;
		}
		else
		{
			this->oceanComponent = nullptr;
		}

		this->createWorkspace();
	}

	Ogre::ParallaxCorrectedCubemapAuto* WorkspaceBaseComponent::getParallaxCorrectedCubemap(void) const
	{
		return this->parallaxCorrectedCubemap;
	}

	void WorkspaceBaseComponent::setParallaxCorrectedCubemap(Ogre::ParallaxCorrectedCubemapAuto* pcc)
	{
		this->parallaxCorrectedCubemap = pcc;
	}

	void WorkspaceBaseComponent::createSSAONoiseTexture(void)
	{
		NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
		{
			//We need to create SSAO kernel samples and noise texture
			//Generate kernel samples first
			float kernelSamples[64][4];
			for (size_t i = 0; i < 64u; ++i)
			{
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

			Ogre::TextureGpu* noiseTexture = nullptr;

			if (false == textureManager->hasTextureResource("noiseTexture", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME))
			{
				noiseTexture = textureManager->createTexture("noiseTexture", Ogre::GpuPageOutStrategy::SaveToSystemRam, 0, Ogre::TextureTypes::Type2D);
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
			}
			else
			{
				noiseTexture = textureManager->findTextureNoThrow("noiseTexture");
			}

			//---------------------------------------------------------------------------------
			//Get GpuProgramParametersSharedPtr to set uniforms that we need
			Ogre::MaterialPtr material = std::static_pointer_cast<Ogre::Material>(Ogre::MaterialManager::getSingleton().load("SSAO/HS", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME));

			Ogre::Pass* pass = material->getTechnique(0)->getPass(0);
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
			projectionAB.y /= this->cameraComponent->getFarClipDistance();
			psParams->setNamedConstant("projectionParams", projectionAB);

			//Set other uniforms
			psParams->setNamedConstant("kernelRadius", 3.0f);

			psParams->setNamedConstant("noiseScale", Ogre::Vector2((Core::getSingletonPtr()->getOgreRenderWindow()->getWidth() * 0.5f) / 2.0f,
										(Core::getSingletonPtr()->getOgreRenderWindow()->getHeight() * 0.5f) / 2.0f));
			psParams->setNamedConstant("invKernelSize", 1.0f / 64.0f);
			psParams->setNamedConstant("sampleDirs", (float*)kernelSamples, 64, 4);

			//Set blur shader uniforms
			Ogre::MaterialPtr materialBlurH = std::static_pointer_cast<Ogre::Material>(Ogre::MaterialManager::getSingleton().load("SSAO/BlurH", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME));

			Ogre::Pass* passBlurH = materialBlurH->getTechnique(0)->getPass(0);
			Ogre::GpuProgramParametersSharedPtr psParamsBlurH = passBlurH->getFragmentProgramParameters();
			psParamsBlurH->setNamedConstant("projectionParams", projectionAB);

			Ogre::MaterialPtr materialBlurV = std::static_pointer_cast<Ogre::Material>(Ogre::MaterialManager::getSingleton().load("SSAO/BlurV", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME));

			Ogre::Pass* passBlurV = materialBlurV->getTechnique(0)->getPass(0);
			Ogre::GpuProgramParametersSharedPtr psParamsBlurV = passBlurV->getFragmentProgramParameters();
			psParamsBlurV->setNamedConstant("projectionParams", projectionAB);

			//Set apply shader uniforms
			Ogre::MaterialPtr materialApply = std::static_pointer_cast<Ogre::Material>(Ogre::MaterialManager::getSingleton().load("SSAO/Apply", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME));

			Ogre::Pass* passApply = materialApply->getTechnique(0)->getPass(0);
			Ogre::GpuProgramParametersSharedPtr psParamsApply = passApply->getFragmentProgramParameters();
			psParamsApply->setNamedConstant("powerScale", 3.0f);
		};
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "WorkspaceBaseComponent::createSSAONoiseTexture");
	}

	void WorkspaceBaseComponent::createLocalCubemapProbeRendererNode(void)
	{
		if (false == this->compositorManager->hasNodeDefinition("LocalCubemapProbeRendererNode"))
		{
			Ogre::CompositorNodeDef* nodeDef = compositorManager->addNodeDefinition("LocalCubemapProbeRendererNode");

			// ===== INPUT CHANNELS =====
			// Channel 0: cubemap texture (from external)
			// Channel 1: ibl_output (from external)
			nodeDef->addTextureSourceName("cubemap", 0, Ogre::TextureDefinitionBase::TEXTURE_INPUT);
			nodeDef->addTextureSourceName("ibl_output", 1, Ogre::TextureDefinitionBase::TEXTURE_INPUT);

			// ===== LOCAL TEXTURES =====
			nodeDef->setNumLocalTextureDefinitions(1);

			auto* depthTexDef = nodeDef->addTextureDefinition("depthTexture");
			depthTexDef->width = 0;  // Will be set by PCC system
			depthTexDef->height = 0;
			depthTexDef->format = Ogre::PFG_D32_FLOAT;
			depthTexDef->textureFlags = Ogre::TextureFlags::RenderToTexture;
			depthTexDef->depthBufferId = Ogre::DepthBuffer::POOL_DEFAULT;

			// ===== RENDER TARGET VIEWS =====
			// RTV 1: cubemap with depth attachment
			Ogre::RenderTargetViewDef* rtvWithDepth = nodeDef->addRenderTextureView("cubemap_customRtv");
			Ogre::RenderTargetViewEntry colourAttachment;
			colourAttachment.textureName = "cubemap";
			rtvWithDepth->colourAttachments.push_back(colourAttachment);
			rtvWithDepth->depthAttachment.textureName = "depthTexture";

			// RTV 2: cubemap without depth (for depth compression pass)
			Ogre::RenderTargetViewDef* rtvNoDepth = nodeDef->addRenderTextureView("cubemap_noDepth");
			colourAttachment.textureName = "cubemap";
			rtvNoDepth->colourAttachments.push_back(colourAttachment);
			rtvNoDepth->depthBufferId = Ogre::DepthBuffer::POOL_NO_DEPTH;

			// ===== TARGET PASSES =====
			// 6 faces × 2 passes each + 1 final IBL pass = 13 passes
			nodeDef->setNumTargetPass(13);

			const Ogre::uint8 cubemapFaces[6] =
			{
				Ogre::CubemapSide::PX, Ogre::CubemapSide::NX,
				Ogre::CubemapSide::PY, Ogre::CubemapSide::NY,
				Ogre::CubemapSide::PZ, Ogre::CubemapSide::NZ
			};

			unsigned short passIndex = 0;

			for (int face = 0; face < 6; ++face)
			{
				// ===== PASS 1: Render scene to cubemap face =====
				{
					Ogre::CompositorTargetDef* targetDef = nodeDef->addTargetPass("cubemap_customRtv", cubemapFaces[face]);

					Ogre::CompositorPassSceneDef* passScene = static_cast<Ogre::CompositorPassSceneDef*>( targetDef->addPass(Ogre::PASS_SCENE));

					passScene->setAllLoadActions(Ogre::LoadAction::Clear);
					passScene->mClearColour[0] = Ogre::ColourValue(0.2f, 0.4f, 0.6f, 1.0f);

					passScene->mStoreActionColour[0] = Ogre::StoreAction::Store;
					passScene->mStoreActionDepth = Ogre::StoreAction::Store;
					passScene->mStoreActionStencil = Ogre::StoreAction::DontCare;

					// Reorient camera for cubemap face
					passScene->mCameraCubemapReorient = true;

					// Filter what gets reflected (visibility mask 0x00000005)
					// This prevents certain objects from appearing in reflections
					passScene->mVisibilityMask = 0x00000005;

					passScene->mIncludeOverlays = false;
					passScene->mUpdateLodLists = true;

					// Render queues 0-250 (RQ 250 is where PCC helper geometry lives)
					passScene->mFirstRQ = 0;
					passScene->mLastRQ = 250;

					// Use shadow node with recalculation for accurate shadows
					passScene->mShadowNode = WorkspaceModule::getInstance()->shadowNodeName;
					passScene->mShadowNodeRecalculation = Ogre::ShadowNodeRecalculation::SHADOW_NODE_RECALCULATE;

					passScene->mProfilingId = "NOWA_PCC_Cubemap_Face_Pass_Scene";
				}

				// ===== PASS 2: Depth compression =====
				{
					Ogre::CompositorTargetDef* targetDef = nodeDef->addTargetPass("cubemap_noDepth", cubemapFaces[face]);

					Ogre::CompositorPassQuadDef* passQuad = static_cast<Ogre::CompositorPassQuadDef*>( targetDef->addPass(Ogre::PASS_QUAD));

					passQuad->mStoreActionDepth = Ogre::StoreAction::DontCare;
					passQuad->mStoreActionStencil = Ogre::StoreAction::DontCare;

					// passQuad->mFrustumCorners = Ogre::CompositorPassQuadDef::VIEW_SPACE_CORNERS_FAR;
					passQuad->mFrustumCorners = Ogre::CompositorPassQuadDef::VIEW_SPACE_CORNERS;
					passQuad->mMaterialName = "PCC/DepthCompressor";
					passQuad->addQuadTextureSource(0, "depthTexture");

					// CRITICAL: Also reorient for cubemap face
					passQuad->mCameraCubemapReorient = true;

					passQuad->mProfilingId = "NOWA_PCC_Depth_Compress_Pass_Quad";

					// On the LAST face (-Z), add IBL specular generation
					if (face == 5) // -Z is the last face
					{
						Ogre::CompositorPassIblSpecularDef* iblPass = static_cast<Ogre::CompositorPassIblSpecularDef*>(targetDef->addPass(Ogre::PASS_IBL_SPECULAR));

						iblPass->setCubemapInput("cubemap");
						iblPass->setCubemapOutput("ibl_output");
						iblPass->mProfilingId = "NOWA_PCC_IBL_Specular_Gen";
					}
				}
			}
		}
	}

	void WorkspaceBaseComponent::createLocalCubemapsProbeWorkspace(void)
	{
		if (false == this->compositorManager->hasWorkspaceDefinition("LocalCubemapsProbeWorkspace"))
		{
			Ogre::CompositorWorkspaceDef* workspaceDef = this->compositorManager->addWorkspaceDefinition("LocalCubemapsProbeWorkspace");

			// Connect external channels to the probe renderer node
			// Channel 0: cubemap texture
			workspaceDef->connectExternal(0, "LocalCubemapProbeRendererNode", 0);
			// Channel 1: IBL output texture
			workspaceDef->connectExternal(1, "LocalCubemapProbeRendererNode", 1);
		}
	}

	void WorkspaceBaseComponent::destroyPccSystem()
	{
		ENQUEUE_RENDER_COMMAND_WAIT("WorkspaceBaseComponent::destroyPccSystem",
		{
			if (nullptr != this->parallaxCorrectedCubemap)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,"[WorkspaceBaseComponent] Destroying PCC system");

				// Clear from HlmsPbs
				Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingletonPtr()->getHlmsManager();
				Ogre::HlmsPbs* hlmsPbs = static_cast<Ogre::HlmsPbs*>(
					hlmsManager->getHlms(Ogre::HLMS_PBS));
				hlmsPbs->setParallaxCorrectedCubemap(nullptr);

				// Cast to FrameListener* before unregistering
				Ogre::Root::getSingleton().removeFrameListener(static_cast<Ogre::FrameListener*>(this->parallaxCorrectedCubemap));

				delete this->parallaxCorrectedCubemap;
				this->parallaxCorrectedCubemap = nullptr;

				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,"[WorkspaceBaseComponent] PCC frame listener unregistered and object deleted");
			}

			if (nullptr != this->workspacePccProbes)
			{
				this->compositorManager->removeWorkspace(this->workspacePccProbes);
				this->workspacePccProbes = nullptr;
			}
		});
	}

	bool WorkspaceBaseComponent::getUseOcean(void) const
	{
		return this->useOcean;
	}

	void WorkspaceBaseComponent::setUsePCC(bool usePCC)
	{
		this->usePCC->setValue(usePCC);

		if (nullptr != this->workspace)
		{
			this->createWorkspace();
		}
	}

	bool WorkspaceBaseComponent::getUsePCC(void) const
	{
		return this->usePCC->getBool();
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
		return this->cubemapTexture;
	}

	Ogre::PlanarReflections* WorkspaceBaseComponent::getPlanarReflections(void) const
	{
		return this->planarReflections;
	}

	void WorkspaceBaseComponent::setShadowGlobalBias(Ogre::Real shadowGlobalBias)
	{
		this->shadowGlobalBias->setValue(shadowGlobalBias); 
		ENQUEUE_RENDER_COMMAND_WAIT("WorkspaceBaseComponent::setShadowGlobalBias",
		{
			this->updateShadowGlobalBias();
		});
	}

	Ogre::Real WorkspaceBaseComponent::getShadowGlobalBias(void) const
	{
		return this->shadowGlobalBias->getReal();
	}

	void WorkspaceBaseComponent::setShadowGlobalNormalOffset(Ogre::Real shadowGlobalNormalOffset)
	{
		this->shadowGlobalNormalOffset->setValue(shadowGlobalNormalOffset);
		ENQUEUE_RENDER_COMMAND_WAIT("WorkspaceBaseComponent::setShadowGlobalNormalOffset",
		{
			this->updateShadowGlobalBias();
		});
	}

	Ogre::Real WorkspaceBaseComponent::getShadowGlobalNormalOffset(void) const
	{
		return this->shadowGlobalNormalOffset->getReal();
	}

	void WorkspaceBaseComponent::setShadowPSSMLambda(Ogre::Real shadowPssmLambda)
	{
		this->shadowPSSMLambda->setValue(shadowPssmLambda);
		ENQUEUE_RENDER_COMMAND_WAIT("WorkspaceBaseComponent::setShadowPSSMLambda",
		{
			this->updateShadowGlobalBias();
		});
	}

	Ogre::Real WorkspaceBaseComponent::getShadowPSSMLambda(void) const
	{
		return this->shadowPSSMLambda->getReal();
	}

	void WorkspaceBaseComponent::setShadowSplitBlend(Ogre::Real shadowSplitBlend)
	{
		this->shadowSplitBlend->setValue(shadowSplitBlend);
		ENQUEUE_RENDER_COMMAND_WAIT("WorkspaceBaseComponent::setShadowSplitBlend",
		{
			this->updateShadowGlobalBias();
		});
	}

	Ogre::Real WorkspaceBaseComponent::getShadowSplitBlend(void) const
	{
		return this->shadowSplitBlend->getReal();
	}

	void WorkspaceBaseComponent::setShadowSplitFade(Ogre::Real shadowSplitFade)
	{
		this->shadowSplitFade->setValue(shadowSplitFade);
		ENQUEUE_RENDER_COMMAND_WAIT("WorkspaceBaseComponent::setShadowSplitFade",
		{
			this->updateShadowGlobalBias();
		});
	}

	Ogre::Real WorkspaceBaseComponent::getShadowSplitFade(void) const
	{
		return this->shadowSplitFade->getReal();
	}

	void WorkspaceBaseComponent::setShadowSplitPadding(Ogre::Real shadowSplitPadding)
	{
		this->shadowSplitPadding->setValue(shadowSplitPadding);
		ENQUEUE_RENDER_COMMAND_WAIT("WorkspaceBaseComponent::setShadowSplitPadding",
		{
			this->updateShadowGlobalBias();
		});
	}

	Ogre::Real WorkspaceBaseComponent::getShadowSplitPadding(void) const
	{
		return this->shadowSplitPadding->getReal();
	}

	void WorkspaceBaseComponent::setCustomExternalChannels(const Ogre::CompositorChannelVec& customExternalChannels)
	{
		this->customExternalChannels = customExternalChannels;
	}

	void WorkspaceBaseComponent::setInvolvedInSplitScreen(bool involvedInSplitScreen)
	{
		this->involvedInSplitScreen = involvedInSplitScreen;
	}

	bool WorkspaceBaseComponent::getInvolvedInSplitScreen(void) const
	{
		return this->involvedInSplitScreen;
	}

	void WorkspaceBaseComponent::updateShadowGlobalBias(void)
	{
		Ogre::CompositorShadowNodeDef * node = this->compositorManager->getShadowNodeDefinitionNonConst(WorkspaceModule::getInstance()->shadowNodeName);
		size_t numShadowDefinitions = node->getNumShadowTextureDefinitions();
		for (size_t i = 0; i < numShadowDefinitions; i++)
		{
			Ogre::ShadowTextureDefinition* texture = node->getShadowTextureDefinitionNonConst(i);
			texture->normalOffsetBias = this->shadowGlobalNormalOffset->getReal();
			texture->constantBiasScale = this->shadowGlobalBias->getReal();
			texture->pssmLambda = this->shadowPSSMLambda->getReal();
			texture->splitBlend = this->shadowSplitBlend->getReal();
			texture->splitFade = this->shadowSplitFade->getReal();
			texture->splitPadding = this->shadowSplitPadding->getReal();
			texture->autoConstantBiasScale = 100.0f;
			texture->autoNormalOffsetBiasScale = 4.0f;
			// texture->numStableSplits = 3;
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

	bool WorkspacePbsComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		bool success = WorkspaceBaseComponent::init(propertyElement);

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

	void WorkspacePbsComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		WorkspaceBaseComponent::writeXML(propertiesXML, doc);
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
		// Threadsafe from the outside
		// https://forums.ogre3d.org/viewtopic.php?t=94748
		if (false == this->compositorManager->hasNodeDefinition(this->renderingNodeName))
		{
			Ogre::CompositorNodeDef* compositorNodeDefinition = compositorManager->addNodeDefinition(this->renderingNodeName);

			unsigned short numTexturesDefinitions = 2;

			if (true == this->useHdr->getBool())
			{
				numTexturesDefinitions++;
			}

			if (true == this->useDistortion->getBool())
			{
				numTexturesDefinitions++;
			}

			if (true == this->useSSAO->getBool())
			{
				numTexturesDefinitions++; // gBufferNormals (only for SSAO)
			}

			numTexturesDefinitions++; // depthTexture (for SSAO or generic use)
			
			compositorNodeDefinition->setNumLocalTextureDefinitions(numTexturesDefinitions);

			Ogre::TextureDefinitionBase::TextureDefinition* texDef = compositorNodeDefinition->addTextureDefinition("rt0");

			texDef->width = 0; // target_width
			texDef->height = 0; // target_height

			if (this->superSampling->getReal() <= 0.0f)
			{
				this->superSampling->setValue(1.0f);
			}

			texDef->widthFactor = this->superSampling->getReal();
			texDef->heightFactor = this->superSampling->getReal();
			texDef->textureFlags = Ogre::TextureFlags::RenderToTexture | Ogre::TextureFlags::MsaaExplicitResolve;
			texDef->format = Ogre::PFG_RGBA16_FLOAT;

			if (this->msaaLevel > 1)
			{
				texDef->fsaa = Ogre::StringConverter::toString(this->msaaLevel);
			}

			if (true == this->useDistortion->getBool())
			{
				// texDef->depthBufferFormat = Ogre::PFG_D32_FLOAT;
				// texDef->depthBufferId = 2;
				// texDef->preferDepthTexture = true;

				Ogre::TextureDefinitionBase::TextureDefinition* distortionTexDef = compositorNodeDefinition->addTextureDefinition("rt_distortion");
				distortionTexDef->width = 0.0f; // target_width
				distortionTexDef->height = 0.0f; // target_height
				distortionTexDef->format = Ogre::PFG_RGBA16_FLOAT;
				distortionTexDef->depthBufferFormat = Ogre::PFG_D32_FLOAT;
				distortionTexDef->preferDepthTexture = true;
				// Attention depth_pool?
				distortionTexDef->depthBufferId = 2;
				distortionTexDef->textureFlags = Ogre::TextureFlags::RenderToTexture;
			}

			if (true == this->useSSAO->getBool())
			{
				// gBufferNormals - with explicit_resolve
				Ogre::TextureDefinitionBase::TextureDefinition* gBufferNormalsTexDef = compositorNodeDefinition->addTextureDefinition("gBufferNormals");
				gBufferNormalsTexDef->width = 0;
				gBufferNormalsTexDef->height = 0;
				gBufferNormalsTexDef->widthFactor = this->superSampling->getReal();
				gBufferNormalsTexDef->heightFactor = this->superSampling->getReal();
				gBufferNormalsTexDef->format = Ogre::PFG_R10G10B10A2_UNORM;
				gBufferNormalsTexDef->textureFlags = Ogre::TextureFlags::RenderToTexture;
				if (this->msaaLevel > 1)
				{
					gBufferNormalsTexDef->fsaa = Ogre::StringConverter::toString(this->msaaLevel);
					gBufferNormalsTexDef->textureFlags |= Ogre::TextureFlags::MsaaExplicitResolve;
				}
			}

			// Depth texture for SSAO or generic depth effects
			Ogre::TextureDefinitionBase::TextureDefinition* depthTextureTexDef = compositorNodeDefinition->addTextureDefinition("depthTexture");
			depthTextureTexDef->width = 0;
			depthTextureTexDef->height = 0;
			depthTextureTexDef->widthFactor = this->superSampling->getReal();
			depthTextureTexDef->heightFactor = this->superSampling->getReal();
			depthTextureTexDef->format = Ogre::PFG_D32_FLOAT;
			depthTextureTexDef->textureFlags = Ogre::TextureFlags::RenderToTexture;
			if (this->msaaLevel > 1)
			{
				depthTextureTexDef->fsaa = Ogre::StringConverter::toString(this->msaaLevel);
			}

			Ogre::RenderTargetViewDef* rtv = compositorNodeDefinition->addRenderTextureView("rt0");
			Ogre::RenderTargetViewEntry attachment;
			attachment.textureName = "rt0";
			rtv->colourAttachments.push_back(attachment);

			if (true == this->useSSAO->getBool())
			{
				// MRT: rt0 + gBufferNormals
				Ogre::RenderTargetViewEntry normalsAttachment;
				normalsAttachment.textureName = "gBufferNormals";
				rtv->colourAttachments.push_back(normalsAttachment);
			}

			// Attach depth texture
			rtv->depthAttachment.textureName = "depthTexture";

			texDef = compositorNodeDefinition->addTextureDefinition("rt1");

			texDef->width = 0; // target_width
			texDef->height = 0; // target_height

			texDef->widthFactor = this->superSampling->getReal();
			texDef->heightFactor = this->superSampling->getReal();
			texDef->format = Ogre::PFG_RGBA16_FLOAT;
			// texDef->msaa = this->msaaLevel;
			texDef->textureFlags = Ogre::TextureFlags::RenderToTexture | Ogre::TextureFlags::MsaaExplicitResolve;

			if (this->msaaLevel > 1)
			{
				texDef->fsaa = Ogre::StringConverter::toString(this->msaaLevel);
			}

			rtv = compositorNodeDefinition->addRenderTextureView("rt1");
			attachment.textureName = "rt1";
			rtv->colourAttachments.push_back(attachment);
			// Note: When set to POOL_NO_DEPTH, no renderqueues are used, everything is rendered without depth!
			rtv->depthBufferId = Ogre::DepthBuffer::POOL_DEFAULT;

			if (true == this->useHdr->getBool())
			{
				texDef = compositorNodeDefinition->addTextureDefinition("oldLumRt");

				texDef->width = 0; // target_width
				texDef->height = 0; // target_height

				texDef->format = Ogre::PFG_R16_FLOAT;
				// ?? Is this necessary?
				texDef->textureFlags = Ogre::TextureFlags::RenderToTexture; // Here also | Ogre::TextureFlags::MsaaExplicitResolve;??
				// texDef->depthBufferId = Ogre::DepthBuffer::POOL_DEFAULT;

				rtv = compositorNodeDefinition->addRenderTextureView("oldLumRt");
				attachment.textureName = "oldLumRt";
				rtv->colourAttachments.push_back(attachment);
				// Compositor script: depth_pool 0  ?
				rtv->depthBufferId = Ogre::DepthBuffer::POOL_NO_DEPTH;
			}

			if (true == this->useDistortion->getBool())
			{
				rtv = compositorNodeDefinition->addRenderTextureView("rt_distortion");
				Ogre::RenderTargetViewEntry attachment;
				attachment.textureName = "rt_distortion";
				rtv->colourAttachments.push_back(attachment);
				rtv->depthBufferId = 2;
			}

			unsigned short numTargetPass = 1;
			if (true == this->useHdr->getBool())
			{
				numTargetPass++;
			}
			if (true == this->useDistortion->getBool())
			{
				numTargetPass++;
			}
			if (true == this->useSSAO->getBool())
			{
				numTargetPass += 4;
			}

			compositorNodeDefinition->setNumTargetPass(numTargetPass);


			// rt0 target
			{
				Ogre::CompositorTargetDef* targetDef = compositorNodeDefinition->addTargetPass("rt0");

				{
					// Render Scene OPAQUE V2 (RQ 0-94)
					{
						Ogre::CompositorPassSceneDef* passScene;
						auto pass = targetDef->addPass(Ogre::PASS_SCENE);
						passScene = static_cast<Ogre::CompositorPassSceneDef*>(pass);

						if (true == this->canUseReflection)
						{
							// Needed for Vulkan & D3D12
							passScene->mExposedTextures.emplace_back(Ogre::IdString("rt1"));
						}

						Ogre::ColourValue color(this->backgroundColor->getVector3().x, this->backgroundColor->getVector3().y, this->backgroundColor->getVector3().z);
						passScene->mClearColour[0] = color;
						passScene->mStoreActionColour[0] = Ogre::StoreAction::StoreOrResolve;

						if (true == this->useSSAO->getBool())
						{
							passScene->mClearColour[1] = Ogre::ColourValue(0.5f, 0.5f, 1.0f, 1.0f);
							passScene->mStoreActionColour[1] = Ogre::StoreAction::StoreOrResolve;
						}

						passScene->setAllLoadActions(Ogre::LoadAction::Clear);

						// Store depth if we need depth texture (for SSAO or generic use)
						passScene->mStoreActionDepth = Ogre::StoreAction::Store;
						passScene->mStoreActionStencil = Ogre::StoreAction::DontCare;

						if (true == this->usePlanarReflection->getBool())
						{
							// Used to identify this pass wants planar reflections
							passScene->mIdentifier = 25001;
						}

						// IMPORTANT: First pass that uses shadows must NOT be set to REUSE
						// otherwise shadow atlas may be read as Undefined at startup / after invalidation.
						passScene->mShadowNode = WorkspaceModule::getInstance()->shadowNodeName;

						passScene->mProfilingId = "NOWA_Pbs_Render_Scene_Opaque_V2_Pass_Scene";
						passScene->mCameraName = this->cameraComponent->getCamera()->getName();

						if (true == this->useSSAO->getBool())
						{
							// Generate normals only for opaque geometry (recommended)
							passScene->mGenNormalsGBuf = true;
						}

						passScene->mIncludeOverlays = false;
						passScene->mUpdateLodLists = true;

						// Render only v2 opaque range in this pass
						passScene->mFirstRQ = 0;
						passScene->mLastRQ = 99;
					}

					// Render Scene OPAQUE V1 (RQ 100-199) so v1::Entity is NOT drawn in transparent pass
					{
						Ogre::CompositorPassSceneDef* passScene;
						auto pass = targetDef->addPass(Ogre::PASS_SCENE);
						passScene = static_cast<Ogre::CompositorPassSceneDef*>(pass);

						passScene->mIncludeOverlays = false;

						passScene->mShadowNode = WorkspaceModule::getInstance()->shadowNodeName;
						passScene->mShadowNodeRecalculation = Ogre::ShadowNodeRecalculation::SHADOW_NODE_REUSE;

						passScene->mCameraName = this->cameraComponent->getCamera()->getName();

						passScene->mFirstRQ = 100;
						passScene->mLastRQ = 199;

						if (true == this->useSSAO->getBool())
						{
							passScene->mGenNormalsGBuf = true;
							passScene->mStoreActionColour[1] = Ogre::StoreAction::StoreOrResolve;
						}

						passScene->mUpdateLodLists = false;

						passScene->setAllLoadActions(Ogre::LoadAction::Load);
						passScene->mStoreActionColour[0] = Ogre::StoreAction::StoreOrResolve;
						passScene->mStoreActionDepth = Ogre::StoreAction::Store;
						passScene->mStoreActionStencil = Ogre::StoreAction::DontCare;

						passScene->mProfilingId = "NOWA_Pbs_Render_Scene_Opaque_V1_Pass_Scene";

						if (true == this->usePlanarReflection->getBool())
						{
							passScene->mIdentifier = 25001;
						}
					}

					// Render Scene TRANSPARENT V2 (RQ 200-224) for alpha-blended ParticleFX2 etc.
					{
						Ogre::CompositorPassSceneDef* passScene;
						auto pass = targetDef->addPass(Ogre::PASS_SCENE);
						passScene = static_cast<Ogre::CompositorPassSceneDef*>(pass);

						passScene->mIncludeOverlays = false;

						passScene->mShadowNode = WorkspaceModule::getInstance()->shadowNodeName;
						passScene->mShadowNodeRecalculation = Ogre::ShadowNodeRecalculation::SHADOW_NODE_REUSE;

						passScene->mCameraName = this->cameraComponent->getCamera()->getName();

						// FAST-high range. Render after v1 opaque
						passScene->mFirstRQ = 200;
						passScene->mLastRQ = 224;

						// Do NOT generate normals for transparency; it can corrupt SSAO / normals buffer.
						passScene->mGenNormalsGBuf = false;

						passScene->mUpdateLodLists = false;

						passScene->setAllLoadActions(Ogre::LoadAction::Load);
						passScene->mStoreActionColour[0] = Ogre::StoreAction::StoreOrResolve;
						passScene->mStoreActionDepth = Ogre::StoreAction::Store;
						passScene->mStoreActionStencil = Ogre::StoreAction::DontCare;

						passScene->mProfilingId = "NOWA_Pbs_Render_Scene_Transparent_V2_Pass_Scene";

						if (true == this->usePlanarReflection->getBool())
						{
							passScene->mIdentifier = 25001;
						}
					}

					// Render Scene TRANSPARENT V1 (RQ 225-255, after sky)
					{
						Ogre::CompositorPassSceneDef* passScene;
						auto pass = targetDef->addPass(Ogre::PASS_SCENE);
						passScene = static_cast<Ogre::CompositorPassSceneDef*>(pass);

						passScene->mIncludeOverlays = false;

						passScene->mShadowNode = WorkspaceModule::getInstance()->shadowNodeName;
						passScene->mShadowNodeRecalculation = Ogre::ShadowNodeRecalculation::SHADOW_NODE_REUSE;

						passScene->mCameraName = this->cameraComponent->getCamera()->getName();

						passScene->mFirstRQ = 225;
						passScene->mLastRQ = 255;  // Covers all remaining queues including GIZMO and MAX

						passScene->mGenNormalsGBuf = false;
						passScene->mUpdateLodLists = false;

						passScene->setAllLoadActions(Ogre::LoadAction::Load);
						passScene->mStoreActionColour[0] = Ogre::StoreAction::StoreOrResolve;
						passScene->mStoreActionDepth = Ogre::StoreAction::Store;
						passScene->mStoreActionStencil = Ogre::StoreAction::DontCare;

						passScene->mProfilingId = "NOWA_Sky_After_Pbs_Transparent_V1_Pass_Scene";

						if (true == this->usePlanarReflection->getBool())
						{
							passScene->mIdentifier = 25001;
						}
					}

					// ===== ADD OVERLAY PASS FOR PCC =====
					// When PCC is active, we need a separate pass for overlays (RQ 251+)
					if (true == this->usePCC->getBool())
					{
						Ogre::CompositorPassSceneDef* overlayPass = static_cast<Ogre::CompositorPassSceneDef*>(targetDef->addPass(Ogre::PASS_SCENE));

						overlayPass->setAllLoadActions(Ogre::LoadAction::Load);
						overlayPass->mStoreActionColour[0] = Ogre::StoreAction::Store;
						overlayPass->mStoreActionDepth = Ogre::StoreAction::DontCare;
						overlayPass->mStoreActionStencil = Ogre::StoreAction::DontCare;

						overlayPass->mFirstRQ = 251;

						overlayPass->mIncludeOverlays = false;
						overlayPass->mUpdateLodLists = false;

						overlayPass->mShadowNode = WorkspaceModule::getInstance()->shadowNodeName;
						overlayPass->mShadowNodeRecalculation = Ogre::ShadowNodeRecalculation::SHADOW_NODE_REUSE;

						overlayPass->mProfilingId = "NOWA_Pbs_PCC_Overlays_Pass_Scene";

						if (true == this->usePlanarReflection->getBool())
						{
							overlayPass->mIdentifier = 25001;
						}
					}
				}
			}

			if (true == this->useHdr->getBool())
			{
				Ogre::CompositorTargetDef* targetDef = compositorNodeDefinition->addTargetPass("oldLumRt");

				// Clear
				{
					Ogre::CompositorPassClearDef* passClear;
					auto pass = targetDef->addPass(Ogre::PASS_CLEAR);
					passClear = static_cast<Ogre::CompositorPassClearDef*>(pass);

					passClear->mNumInitialPasses = 1;
					passClear->mClearColour[0] = Ogre::ColourValue(0.01f, 0.01f, 0.01f, 1.0f);

					passClear->mProfilingId = "NOWA_Pbs_Hdr_Pass_Clear";
				}
			}

			if (true == this->useDistortion->getBool())
			{
				Ogre::CompositorTargetDef* targetDef = compositorNodeDefinition->addTargetPass("rt_distortion");

				// Render Scene for distortion
				{
					Ogre::CompositorPassSceneDef* passScene;
					auto pass = targetDef->addPass(Ogre::PASS_SCENE);
					passScene = static_cast<Ogre::CompositorPassSceneDef*>(pass);

					passScene->mProfilingId = "NOWA_Pbs_Distortion_Pass_Scene";

					passScene->setAllLoadActions(Ogre::LoadAction::Clear);

					passScene->mClearColour[0] = Ogre::ColourValue(0.5f, 0.5f, 0.0f, 0.0f);

					passScene->mUpdateLodLists = false;

					passScene->mIncludeOverlays = false;
					passScene->mFirstRQ = 16;
					passScene->mLastRQ = 17;
				}

				this->createDistortionNode();
			}

			// ===== Output channels =====
			unsigned short outputChannelCount = 2; // rt0, rt1

			if (true == this->useHdr->getBool())
			{
				outputChannelCount++; // oldLumRt
			}

			if (true == this->useDistortion->getBool())
			{
				outputChannelCount++; // rt_distortion
			}

			if (true == this->useSSAO->getBool())
			{
				outputChannelCount++; // gBufferNormals
			}

			outputChannelCount++; // depthTexture

			compositorNodeDefinition->setNumOutputChannels(outputChannelCount);

			unsigned short outputChannel = 0;

			compositorNodeDefinition->mapOutputChannel(outputChannel++, "rt0"); // 0
			compositorNodeDefinition->mapOutputChannel(outputChannel++, "rt1"); // 1

			if (true == this->useHdr->getBool())
			{
				compositorNodeDefinition->mapOutputChannel(outputChannel++, "oldLumRt"); // 2
			}
			if (true == this->useDistortion->getBool())
			{
				compositorNodeDefinition->mapOutputChannel(outputChannel++, "rt_distortion"); // 3
			}

			if (true == this->useSSAO->getBool())
			{
				compositorNodeDefinition->mapOutputChannel(outputChannel++, "gBufferNormals");
			}

			compositorNodeDefinition->mapOutputChannel(outputChannel++, "depthTexture");
		}

		this->createFinalRenderNode();

		this->setBackgroundColor(this->backgroundColor->getVector3());
		// For VR: disable this line and use NOWAPbsRenderingNodeVR
		// this->changeViewportRect(0, this->viewportRect->getVector4());

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
							auto pass = targetDef->addPass(Ogre::PASS_SCENE);
							passScene = static_cast<Ogre::CompositorPassSceneDef*>(pass);

							passScene->mProfilingId = "NOWA_Pbs_PlanarReflections_Pass_Scene";

							Ogre::ColourValue color(this->backgroundColor->getVector3().x, this->backgroundColor->getVector3().y, this->backgroundColor->getVector3().z);
							// passScene->setAllClearColours(color);
							passScene->setAllLoadActions(Ogre::LoadAction::Clear);
							passScene->mClearColour[0] = color;

							passScene->setAllStoreActions(Ogre::StoreAction::StoreOrResolve);
							passScene->mStoreActionColour[0] = Ogre::StoreAction::StoreOrResolve; // Ogre::StoreAction::StoreAndMultisampleResolve; causes a crash, why?
							passScene->mStoreActionDepth = Ogre::StoreAction::DontCare;
							passScene->mStoreActionStencil = Ogre::StoreAction::DontCare;

							passScene->mIncludeOverlays = false;

							// passScene->mVisibilityMask = 0xfffffffe;

							//https://forums.ogre3d.org/viewtopic.php?t=93636
							//https://forums.ogre3d.org/viewtopic.php?t=94748
							passScene->mShadowNode = WorkspaceModule::getInstance()->shadowNodeName;
						}

						// Generate Mipmaps
						{
							Ogre::CompositorPassMipmapDef* passMipmap;
							passMipmap = static_cast<Ogre::CompositorPassMipmapDef*>(targetDef->addPass(Ogre::PASS_MIPMAP));

							passMipmap->mMipmapGenerationMethod = Ogre::CompositorPassMipmapDef::ComputeHQ;

							passMipmap->mProfilingId = "NOWA_Pbs_PlanarReflections_Pass_Mipmap";
						}
					}
				}
			}
		}
	}

	bool WorkspacePbsComponent::internalCreateWorkspace(Ogre::CompositorWorkspaceDef* workspaceDef)
	{
		// Call base workspace creation, which does already main node connections
		WorkspaceBaseComponent::baseCreateWorkspace(workspaceDef);

		this->addWorkspace(workspaceDef);

		this->setBackgroundColor(this->backgroundColor->getVector3());
		// For VR: disable this line and use NOWAPbsRenderingNodeVR
		// this->changeViewportRect(0, this->viewportRect->getVector4());

		if (this->msaaLevel == 1)
		{
			this->initializeSmaa(SMAA_PRESET_LOW, EdgeDetectionLuma);
		}
		else if (this->msaaLevel == 2)
		{
			this->initializeSmaa(SMAA_PRESET_MEDIUM, EdgeDetectionLuma);
		}
		else if (this->msaaLevel == 4)
		{
			this->initializeSmaa(SMAA_PRESET_HIGH, EdgeDetectionColor);
		}
		else
		{
			this->initializeSmaa(SMAA_PRESET_ULTRA, EdgeDetectionColor);
		}

		if (true == this->useHdr->getBool())
		{
			this->initializeHdr(this->msaaLevel);
		}

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

	bool WorkspaceSkyComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		bool success = WorkspaceBaseComponent::init(propertyElement);

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

	void WorkspaceSkyComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		WorkspaceBaseComponent::writeXML(propertiesXML, doc);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SkyBoxName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->skyBoxName->getListSelectedValue())));
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

			unsigned short numTexturesDefinitions = 2;

			if (true == this->useHdr->getBool())
			{
				numTexturesDefinitions++;
			}

			if (true == this->useDistortion->getBool())
			{
				numTexturesDefinitions++;
			}

			if (true == this->useSSAO->getBool())
			{
				numTexturesDefinitions++; // gBufferNormals (only for SSAO)
			}

			numTexturesDefinitions++; // depthTexture (for SSAO or generic use)
			
			compositorNodeDefinition->setNumLocalTextureDefinitions(numTexturesDefinitions);

			Ogre::TextureDefinitionBase::TextureDefinition* texDef = compositorNodeDefinition->addTextureDefinition("rt0");

			texDef->width = 0;
			texDef->height = 0;

			if (this->superSampling->getReal() <= 0.0f)
			{
				this->superSampling->setValue(1.0f);
			}

			texDef->widthFactor = this->superSampling->getReal();
			texDef->heightFactor = this->superSampling->getReal();
			texDef->textureFlags = Ogre::TextureFlags::RenderToTexture | Ogre::TextureFlags::MsaaExplicitResolve;
			texDef->format = Ogre::PFG_RGBA16_FLOAT;

			if (this->msaaLevel > 1)
			{
				texDef->fsaa = Ogre::StringConverter::toString(this->msaaLevel);
			}

			if (true == this->useDistortion->getBool())
			{
				Ogre::TextureDefinitionBase::TextureDefinition* distortionTexDef = compositorNodeDefinition->addTextureDefinition("rt_distortion");
				distortionTexDef->width = 0.0f;
				distortionTexDef->height = 0.0f;
				distortionTexDef->format = Ogre::PFG_RGBA16_FLOAT;
				distortionTexDef->depthBufferFormat = Ogre::PFG_D32_FLOAT;
				distortionTexDef->preferDepthTexture = true;
				distortionTexDef->depthBufferId = 2;
				distortionTexDef->textureFlags = Ogre::TextureFlags::RenderToTexture;
			}

			if (true == this->useSSAO->getBool())
			{
				// gBufferNormals - with explicit_resolve
				Ogre::TextureDefinitionBase::TextureDefinition* gBufferNormalsTexDef = compositorNodeDefinition->addTextureDefinition("gBufferNormals");
				gBufferNormalsTexDef->width = 0;
				gBufferNormalsTexDef->height = 0;
				gBufferNormalsTexDef->widthFactor = this->superSampling->getReal();
				gBufferNormalsTexDef->heightFactor = this->superSampling->getReal();
				gBufferNormalsTexDef->format = Ogre::PFG_R10G10B10A2_UNORM;
				gBufferNormalsTexDef->textureFlags = Ogre::TextureFlags::RenderToTexture;
				if (this->msaaLevel > 1)
				{
					gBufferNormalsTexDef->fsaa = Ogre::StringConverter::toString(this->msaaLevel);
					gBufferNormalsTexDef->textureFlags |= Ogre::TextureFlags::MsaaExplicitResolve;
				}
			}

			// Depth texture for SSAO or generic depth effects
			Ogre::TextureDefinitionBase::TextureDefinition* depthTextureTexDef = compositorNodeDefinition->addTextureDefinition("depthTexture");
			depthTextureTexDef->width = 0;
			depthTextureTexDef->height = 0;
			depthTextureTexDef->widthFactor = this->superSampling->getReal();
			depthTextureTexDef->heightFactor = this->superSampling->getReal();
			depthTextureTexDef->format = Ogre::PFG_D32_FLOAT;
			depthTextureTexDef->textureFlags = Ogre::TextureFlags::RenderToTexture;
			if (this->msaaLevel > 1)
			{
				depthTextureTexDef->fsaa = Ogre::StringConverter::toString(this->msaaLevel);
			}

			Ogre::RenderTargetViewDef* rtv = compositorNodeDefinition->addRenderTextureView("rt0");
			Ogre::RenderTargetViewEntry attachment;
			attachment.textureName = "rt0";
			rtv->colourAttachments.push_back(attachment);

			if (true == this->useSSAO->getBool())
			{
				// MRT: rt0 + gBufferNormals
				Ogre::RenderTargetViewEntry normalsAttachment;
				normalsAttachment.textureName = "gBufferNormals";
				rtv->colourAttachments.push_back(normalsAttachment);
			}

			// Attach depth texture
			rtv->depthAttachment.textureName = "depthTexture";

			texDef = compositorNodeDefinition->addTextureDefinition("rt1");
			texDef->width = 0;
			texDef->height = 0;
			texDef->widthFactor = this->superSampling->getReal();
			texDef->heightFactor = this->superSampling->getReal();
			texDef->format = Ogre::PFG_RGBA16_FLOAT;
			texDef->textureFlags = Ogre::TextureFlags::RenderToTexture | Ogre::TextureFlags::MsaaExplicitResolve;

			if (this->msaaLevel > 1)
			{
				texDef->fsaa = Ogre::StringConverter::toString(this->msaaLevel);
			}

			rtv = compositorNodeDefinition->addRenderTextureView("rt1");
			attachment.textureName = "rt1";
			rtv->colourAttachments.push_back(attachment);
			rtv->depthBufferId = Ogre::DepthBuffer::POOL_DEFAULT;

			if (true == this->useHdr->getBool())
			{
				texDef = compositorNodeDefinition->addTextureDefinition("oldLumRt");
				texDef->width = 0;
				texDef->height = 0;
				texDef->format = Ogre::PFG_R16_FLOAT;
				texDef->textureFlags = Ogre::TextureFlags::RenderToTexture;

				rtv = compositorNodeDefinition->addRenderTextureView("oldLumRt");
				attachment.textureName = "oldLumRt";
				rtv->colourAttachments.push_back(attachment);
				rtv->depthBufferId = Ogre::DepthBuffer::POOL_NO_DEPTH;
			}

			if (true == this->useDistortion->getBool())
			{
				rtv = compositorNodeDefinition->addRenderTextureView("rt_distortion");
				Ogre::RenderTargetViewEntry attachment;
				attachment.textureName = "rt_distortion";
				rtv->colourAttachments.push_back(attachment);
				rtv->depthBufferId = 2;
			}

			unsigned short numTargetPass = 1;
			if (true == this->useHdr->getBool())
			{
				numTargetPass++;
			}
			if (true == this->useDistortion->getBool())
			{
				numTargetPass++;
			}
			if (true == this->useSSAO->getBool())
			{
				numTargetPass += 4;
			}

			compositorNodeDefinition->setNumTargetPass(numTargetPass);

			// rt0 target
			{
				Ogre::CompositorTargetDef* targetDef = compositorNodeDefinition->addTargetPass("rt0");

				// Clear Pass
				{
					Ogre::CompositorPassClearDef* passClear;
					auto pass = targetDef->addPass(Ogre::PASS_CLEAR);
					passClear = static_cast<Ogre::CompositorPassClearDef*>(pass);

					passClear->mClearColour[0] = Ogre::ColourValue(0.2f, 0.4f, 0.6f) * 60.0f;

					if (true == this->useSSAO->getBool())
					{
						// Clear the normals buffer to a neutral value
						passClear->mClearColour[1] = Ogre::ColourValue(0.5f, 0.5f, 1.0f, 1.0f);
						passClear->mStoreActionColour[1] = Ogre::StoreAction::Store;
					}

					passClear->mStoreActionColour[0] = Ogre::StoreAction::Store;
					passClear->mStoreActionDepth = Ogre::StoreAction::Store;
					passClear->mStoreActionStencil = Ogre::StoreAction::Store;

					passClear->mProfilingId = "NOWA_Sky_Clear_Pass_Clear";
				}

				// Render Scene (RQ 0-1, before sky)
				{
					Ogre::CompositorPassSceneDef* passScene;
					auto pass = targetDef->addPass(Ogre::PASS_SCENE);
					passScene = static_cast<Ogre::CompositorPassSceneDef*>(pass);

					if (true == this->canUseReflection)
					{
						passScene->mExposedTextures.emplace_back(Ogre::IdString("rt1"));
					}

					// We already cleared in PASS_CLEAR
					passScene->setAllLoadActions(Ogre::LoadAction::Load);

					passScene->mStoreActionColour[0] = Ogre::StoreAction::StoreOrResolve;
					passScene->mStoreActionDepth = Ogre::StoreAction::Store;
					passScene->mStoreActionStencil = Ogre::StoreAction::DontCare;

					if (true == this->useSSAO->getBool())
					{
						passScene->mStoreActionColour[1] = Ogre::StoreAction::StoreOrResolve;
						passScene->mGenNormalsGBuf = true;
					}

					if (true == this->usePlanarReflection->getBool())
					{
						passScene->mIdentifier = 25001;
					}

					passScene->mProfilingId = "NOWA_Sky_Render_Scene_Before_Sky_Pass_Scene";
					passScene->mCameraName = this->cameraComponent->getCamera()->getName();

					passScene->mIncludeOverlays = false;
					passScene->mUpdateLodLists = true;

					passScene->mFirstRQ = 0;
					passScene->mLastRQ = 1;
				}

				// Quad pass (Sky)
				{
					Ogre::CompositorPassQuadDef* passQuad;
					auto pass = targetDef->addPass(Ogre::PASS_QUAD);
					passQuad = static_cast<Ogre::CompositorPassQuadDef*>(pass);

					passQuad->mMaterialName = "NOWASkyPostprocess";
					passQuad->mFrustumCorners = Ogre::CompositorPassQuadDef::CAMERA_DIRECTION;

					passQuad->setAllLoadActions(Ogre::LoadAction::Load);
					passQuad->mStoreActionColour[0] = Ogre::StoreAction::Store;

					if (true == this->useSSAO->getBool())
					{
						// Don't modify the normals buffer - sky is at infinity
						passQuad->mStoreActionColour[1] = Ogre::StoreAction::Store;
					}

					passQuad->mStoreActionDepth = Ogre::StoreAction::Store;

					passQuad->mProfilingId = "NOWA_Sky_Pass_Quad";
				}

				// Render Scene OPAQUE V2 (RQ 2-94, after sky)
				// IMPORTANT: This is the FIRST pass that uses shadows => must NOT be SHADOW_NODE_REUSE
				{
					Ogre::CompositorPassSceneDef* passScene;
					auto pass = targetDef->addPass(Ogre::PASS_SCENE);
					passScene = static_cast<Ogre::CompositorPassSceneDef*>(pass);

					passScene->mIncludeOverlays = false;

					passScene->mShadowNode = WorkspaceModule::getInstance()->shadowNodeName;

					// FIX: do NOT reuse here, atlas may be Undefined at startup / after invalidation
					// If your Ogre has SHADOW_NODE_RECALCULATE, you can set it explicitly.
					// passScene->mShadowNodeRecalculation = Ogre::ShadowNodeRecalculation::SHADOW_NODE_RECALCULATE;

					passScene->mCameraName = this->cameraComponent->getCamera()->getName();

					passScene->mFirstRQ = 2;
					passScene->mLastRQ = 99;

					passScene->setAllLoadActions(Ogre::LoadAction::Load);
					passScene->mStoreActionColour[0] = Ogre::StoreAction::StoreOrResolve;
					passScene->mStoreActionDepth = Ogre::StoreAction::Store;
					passScene->mStoreActionStencil = Ogre::StoreAction::DontCare;

					if (true == this->useSSAO->getBool())
					{
						passScene->mGenNormalsGBuf = true;
						passScene->mStoreActionColour[1] = Ogre::StoreAction::StoreOrResolve;
					}

					passScene->mUpdateLodLists = true;

					passScene->mProfilingId = "NOWA_Sky_After_Sky_Opaque_V2_Pass_Scene";

					if (true == this->usePlanarReflection->getBool())
					{
						passScene->mIdentifier = 25001;
					}
				}

				// Render Scene OPAQUE V1 (RQ 100-199, after sky)
				{
					Ogre::CompositorPassSceneDef* passScene;
					auto pass = targetDef->addPass(Ogre::PASS_SCENE);
					passScene = static_cast<Ogre::CompositorPassSceneDef*>(pass);

					passScene->mIncludeOverlays = false;

					passScene->mShadowNode = WorkspaceModule::getInstance()->shadowNodeName;
					passScene->mShadowNodeRecalculation = Ogre::ShadowNodeRecalculation::SHADOW_NODE_REUSE;

					passScene->mCameraName = this->cameraComponent->getCamera()->getName();

					passScene->mFirstRQ = 100;
					passScene->mLastRQ = 199;

					passScene->setAllLoadActions(Ogre::LoadAction::Load);
					passScene->mStoreActionColour[0] = Ogre::StoreAction::StoreOrResolve;
					passScene->mStoreActionDepth = Ogre::StoreAction::Store;
					passScene->mStoreActionStencil = Ogre::StoreAction::DontCare;

					if (true == this->useSSAO->getBool())
					{
						passScene->mGenNormalsGBuf = true;
						passScene->mStoreActionColour[1] = Ogre::StoreAction::StoreOrResolve;
					}

					passScene->mUpdateLodLists = false;

					passScene->mProfilingId = "NOWA_Sky_After_Sky_Opaque_V1_Pass_Scene";

					if (true == this->usePlanarReflection->getBool())
					{
						passScene->mIdentifier = 25001;
					}
				}

				// Render Scene TRANSPARENT (FAST-high) (RQ 200-224, after sky)
				{
					Ogre::CompositorPassSceneDef* passScene;
					auto pass = targetDef->addPass(Ogre::PASS_SCENE);
					passScene = static_cast<Ogre::CompositorPassSceneDef*>(pass);

					passScene->mIncludeOverlays = false;

					passScene->mShadowNode = WorkspaceModule::getInstance()->shadowNodeName;
					passScene->mShadowNodeRecalculation = Ogre::ShadowNodeRecalculation::SHADOW_NODE_REUSE;

					passScene->mCameraName = this->cameraComponent->getCamera()->getName();

					passScene->mFirstRQ = 200;
					passScene->mLastRQ = 224;

					passScene->mGenNormalsGBuf = false;
					passScene->mUpdateLodLists = false;

					passScene->setAllLoadActions(Ogre::LoadAction::Load);
					passScene->mStoreActionColour[0] = Ogre::StoreAction::StoreOrResolve;
					passScene->mStoreActionDepth = Ogre::StoreAction::Store;
					passScene->mStoreActionStencil = Ogre::StoreAction::DontCare;

					passScene->mProfilingId = "NOWA_Sky_After_Sky_Transparent_Pass_Scene";

					if (true == this->usePlanarReflection->getBool())
					{
						passScene->mIdentifier = 25001;
					}
				}

				// Render Scene TRANSPARENT V1 (RQ 225-255, after sky)
				{
					Ogre::CompositorPassSceneDef* passScene;
					auto pass = targetDef->addPass(Ogre::PASS_SCENE);
					passScene = static_cast<Ogre::CompositorPassSceneDef*>(pass);

					passScene->mIncludeOverlays = false;

					passScene->mShadowNode = WorkspaceModule::getInstance()->shadowNodeName;
					passScene->mShadowNodeRecalculation = Ogre::ShadowNodeRecalculation::SHADOW_NODE_REUSE;

					passScene->mCameraName = this->cameraComponent->getCamera()->getName();

					passScene->mFirstRQ = 225;
					passScene->mLastRQ = 255;  // Covers all remaining queues including GIZMO and MAX

					passScene->mGenNormalsGBuf = false;
					passScene->mUpdateLodLists = false;

					passScene->setAllLoadActions(Ogre::LoadAction::Load);
					passScene->mStoreActionColour[0] = Ogre::StoreAction::StoreOrResolve;
					passScene->mStoreActionDepth = Ogre::StoreAction::Store;
					passScene->mStoreActionStencil = Ogre::StoreAction::DontCare;

					passScene->mProfilingId = "NOWA_Sky_After_Sky_Transparent_V1_Pass_Scene";

					if (true == this->usePlanarReflection->getBool())
					{
						passScene->mIdentifier = 25001;
					}
				}

				// ===== ADD OVERLAY PASS FOR PCC =====
				if (true == this->usePCC->getBool())
				{
					Ogre::CompositorPassSceneDef* overlayPass = static_cast<Ogre::CompositorPassSceneDef*>(targetDef->addPass(Ogre::PASS_SCENE));

					overlayPass->setAllLoadActions(Ogre::LoadAction::Load);
					overlayPass->mStoreActionColour[0] = Ogre::StoreAction::Store;
					overlayPass->mStoreActionDepth = Ogre::StoreAction::DontCare;
					overlayPass->mStoreActionStencil = Ogre::StoreAction::DontCare;

					overlayPass->mFirstRQ = 251;

					overlayPass->mIncludeOverlays = false;
					overlayPass->mUpdateLodLists = false;

					overlayPass->mShadowNode = WorkspaceModule::getInstance()->shadowNodeName;
					overlayPass->mShadowNodeRecalculation = Ogre::ShadowNodeRecalculation::SHADOW_NODE_REUSE;

					overlayPass->mProfilingId = "NOWA_Pbs_PCC_Overlays_Pass_Scene";

					if (true == this->usePlanarReflection->getBool())
					{
						overlayPass->mIdentifier = 25001;
					}
				}
			}

			if (true == this->useHdr->getBool())
			{
				Ogre::CompositorTargetDef* targetDef = compositorNodeDefinition->addTargetPass("oldLumRt");

				Ogre::CompositorPassClearDef* passClear;
				auto pass = targetDef->addPass(Ogre::PASS_CLEAR);
				passClear = static_cast<Ogre::CompositorPassClearDef*>(pass);

				passClear->mNumInitialPasses = 1;
				passClear->mClearColour[0] = Ogre::ColourValue(0.01f, 0.01f, 0.01f, 1.0f);

				passClear->mProfilingId = "NOWA_Sky_Hdr_Pass_Clear";
			}

			if (true == this->useDistortion->getBool())
			{
				Ogre::CompositorTargetDef* targetDef = compositorNodeDefinition->addTargetPass("rt_distortion");

				Ogre::CompositorPassSceneDef* passScene;
				auto pass = targetDef->addPass(Ogre::PASS_SCENE);
				passScene = static_cast<Ogre::CompositorPassSceneDef*>(pass);

				passScene->mProfilingId = "NOWA_Sky_Distortion_Pass_Scene";
				passScene->setAllLoadActions(Ogre::LoadAction::Clear);
				passScene->mClearColour[0] = Ogre::ColourValue(0.5f, 0.5f, 0.0f, 0.0f);
				passScene->mUpdateLodLists = false;
				passScene->mIncludeOverlays = false;
				passScene->mFirstRQ = 16;
				passScene->mLastRQ = 17;

				this->createDistortionNode();
			}

			// ===== Output channels =====
			unsigned short outputChannelCount = 2; // rt0, rt1

			if (true == this->useHdr->getBool())
			{
				outputChannelCount++;
			}

			if (true == this->useDistortion->getBool())
			{
				outputChannelCount++;
			}

			if (true == this->useSSAO->getBool())
			{
				outputChannelCount++; // gBufferNormals
			}

			outputChannelCount++; // depthTexture

			compositorNodeDefinition->setNumOutputChannels(outputChannelCount);

			unsigned short outputChannel = 0;

			compositorNodeDefinition->mapOutputChannel(outputChannel++, "rt0");
			compositorNodeDefinition->mapOutputChannel(outputChannel++, "rt1");

			if (true == this->useHdr->getBool())
			{
				compositorNodeDefinition->mapOutputChannel(outputChannel++, "oldLumRt");
			}
			if (true == this->useDistortion->getBool())
			{
				compositorNodeDefinition->mapOutputChannel(outputChannel++, "rt_distortion");
			}

			if (true == this->useSSAO->getBool())
			{
				compositorNodeDefinition->mapOutputChannel(outputChannel++, "gBufferNormals");
			}

			compositorNodeDefinition->mapOutputChannel(outputChannel++, "depthTexture");
		}

		this->createFinalRenderNode();

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
							auto pass = targetDef->addPass(Ogre::PASS_SCENE);
							passScene = static_cast<Ogre::CompositorPassSceneDef*>(pass);

							passScene->mProfilingId = "NOWA_Sky_PlanarReflections_Pass_Scene";

							Ogre::ColourValue color(0.2f, 0.4f, 0.6f);
							// passScene->setAllClearColours(color);
							passScene->setAllLoadActions(Ogre::LoadAction::Clear);
							passScene->mClearColour[0] = color;

							// passScene->setAllStoreActions(Ogre::StoreAction::StoreOrResolve);
							passScene->mStoreActionColour[0] = Ogre::StoreAction::StoreOrResolve; // Ogre::StoreAction::StoreAndMultisampleResolve; causes a crash, why?
							passScene->mStoreActionDepth = Ogre::StoreAction::DontCare;
							passScene->mStoreActionStencil = Ogre::StoreAction::DontCare;

							passScene->mFirstRQ = 0;
							passScene->mLastRQ = 2;

							passScene->mIncludeOverlays = false;

							// passScene->mVisibilityMask = 0xfffffffe;

							//https://forums.ogre3d.org/viewtopic.php?t=93636
							//https://forums.ogre3d.org/viewtopic.php?t=94748
							passScene->mShadowNode = WorkspaceModule::getInstance()->shadowNodeName;
						}

						// Sky quad
						{
							auto pass = targetDef->addPass(Ogre::PASS_QUAD);
							Ogre::CompositorPassQuadDef* passQuad = static_cast<Ogre::CompositorPassQuadDef*>(pass);

							passQuad->mMaterialName = "NOWASkyPostprocess";
							passQuad->mFrustumCorners = Ogre::CompositorPassQuadDef::CAMERA_DIRECTION;

							passQuad->mProfilingId = "NOWA_Sky_PlanarReflections_Pass_Quad";
						}

						// Render Scene
						{
							Ogre::CompositorPassSceneDef* passScene;
							auto pass = targetDef->addPass(Ogre::PASS_SCENE);
							passScene = static_cast<Ogre::CompositorPassSceneDef*>(pass);
							passScene->mIncludeOverlays = false;
							passScene->mShadowNode = WorkspaceModule::getInstance()->shadowNodeName;
							passScene->mShadowNodeRecalculation = Ogre::ShadowNodeRecalculation::SHADOW_NODE_REUSE;
							passScene->mFirstRQ = 2;

							passScene->mProfilingId = "NOWA_Sky_Before_Sky_PlanarReflections_Pass_Scene";
						}

						// Generate Mipmaps
						{
							Ogre::CompositorPassMipmapDef* passMipmap;
							passMipmap = static_cast<Ogre::CompositorPassMipmapDef*>(targetDef->addPass(Ogre::PASS_MIPMAP));

							passMipmap->mMipmapGenerationMethod = Ogre::CompositorPassMipmapDef::ComputeHQ;

							passMipmap->mProfilingId = "NOWA_Sky_PlanarReflections_Pass_Mipmap";
						}
					}
				}
			}
		}
	}

	bool WorkspaceSkyComponent::internalCreateWorkspace(Ogre::CompositorWorkspaceDef* workspaceDef)
	{
		// Call base workspace creation, which does already main node connections
		WorkspaceBaseComponent::baseCreateWorkspace(workspaceDef);

		this->addWorkspace(workspaceDef);

		this->changeSkyBox(this->skyBoxName->getListSelectedValue());

		// Is added in CameraComponent
		// WorkspaceModule::getInstance()->setPrimaryWorkspace(this->gameObjectPtr->getSceneManager(), this->cameraComponent->getCamera(), this);

		if (this->msaaLevel == 1)
		{
			this->initializeSmaa(SMAA_PRESET_LOW, EdgeDetectionLuma);
		}
		else if (this->msaaLevel == 2)
		{
			this->initializeSmaa(SMAA_PRESET_MEDIUM, EdgeDetectionLuma);
		}
		else if (this->msaaLevel == 4)
		{
			this->initializeSmaa(SMAA_PRESET_HIGH, EdgeDetectionColor);
		}
		else
		{
			this->initializeSmaa(SMAA_PRESET_ULTRA, EdgeDetectionColor);
		}

		if (true == this->useHdr->getBool())
		{
			this->initializeHdr(this->msaaLevel);
		}

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
			tex->setGamma(8.0);
			tex->setHardwareGammaEnabled(true);
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
		: WorkspaceBaseComponent(),
		hardwareGammaEnabled(new Variant(WorkspaceBackgroundComponent::AttrHardwareGammaEnabled(), true, this->attributes)),
		activeLayerCount(0)
	{
		for (size_t i = 0; i < 9; i++)
		{
			this->layerEnabled[i] = 0.0f;
		}
	}

	WorkspaceBackgroundComponent::~WorkspaceBackgroundComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[WorkspaceBackgroundComponent] Destructor workspace sky component for game object: " + this->gameObjectPtr->getName());
	}

	bool WorkspaceBackgroundComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		bool success = WorkspaceBaseComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "HardwareGammaEnabled")
		{
			this->hardwareGammaEnabled->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

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

		if (WorkspaceBackgroundComponent::AttrHardwareGammaEnabled() == attribute->getName())
		{
			this->setHardwareGammaEnabled(attribute->getBool());
		}
	}

	void WorkspaceBackgroundComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		WorkspaceBaseComponent::writeXML(propertiesXML, doc);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "HardwareGammaEnabled"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->hardwareGammaEnabled->getBool())));
		propertiesXML->append_node(propertyXML);
	}

	void WorkspaceBackgroundComponent::setHardwareGammaEnabled(bool hardwareGammaEnabled)
	{
		this->hardwareGammaEnabled->setValue(hardwareGammaEnabled);

		Ogre::String strMaterialName = "NOWABackgroundScroll";

		if (true == this->materialBackgroundPtr.isNull())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[WorkspaceBackgroundComponent] Could not set: " + this->workspaceName + " because the material: '" + strMaterialName + "' does not exist!");
			throw Ogre::Exception(Ogre::Exception::ERR_ITEM_NOT_FOUND, "Could not create: " + this->workspaceName + " because the material: '" + strMaterialName + "' does not exist!", "NOWA");
		}

		Ogre::Material* material = this->materialBackgroundPtr.getPointer();
		this->passBackground = material->getTechnique(0)->getPass(0);

		Ogre::HlmsSamplerblock samplerblock;
		samplerblock.mU = Ogre::TextureAddressingMode::TAM_WRAP;
		samplerblock.mV = Ogre::TextureAddressingMode::TAM_WRAP;
		samplerblock.mW = Ogre::TextureAddressingMode::TAM_WRAP;
		samplerblock.mMaxAnisotropy = 0;
		samplerblock.mMagFilter = Ogre::FO_POINT;
		samplerblock.mMinFilter = Ogre::FO_POINT;
		samplerblock.mMipFilter = Ogre::FO_NONE;
		samplerblock.setFiltering(Ogre::TextureFilterOptions::TFO_NONE);

		this->passBackground->setSamplerblock(samplerblock);

		for (size_t i = 0; i < this->activeLayerCount; i++)
		{
			Ogre::TextureUnitState* tex = this->passBackground->getTextureUnitState(i);
			if (true == this->hardwareGammaEnabled->getBool())
			{
				tex->setGamma(8.0);
			}
			tex->setHardwareGammaEnabled(hardwareGammaEnabled);
			tex->setNumMipmaps(0);
		}

		material->compile();
	}

	bool WorkspaceBackgroundComponent::getHardwareGammaEnabled(void) const
	{
		return this->hardwareGammaEnabled->getBool();
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
			Ogre::CompositorNodeDef* compositorNodeDefinition = this->compositorManager->addNodeDefinition(this->renderingNodeName);

			unsigned short numTexturesDefinitions = 2; // rt0, rt1

			if (true == this->useHdr->getBool())
			{
				numTexturesDefinitions++;
			}

			if (true == this->useDistortion->getBool())
			{
				numTexturesDefinitions++;
			}

			if (true == this->useSSAO->getBool())
			{
				numTexturesDefinitions++; // gBufferNormals (only for SSAO)
			}

			numTexturesDefinitions++; // depthTexture (for SSAO or generic use)

			compositorNodeDefinition->setNumLocalTextureDefinitions(numTexturesDefinitions);

			Ogre::TextureDefinitionBase::TextureDefinition* texDef = compositorNodeDefinition->addTextureDefinition("rt0");

			if (this->superSampling->getReal() <= 0.0f)
			{
				this->superSampling->setValue(1.0f);
			}

			texDef->width = 0;
			texDef->height = 0;
			texDef->widthFactor = this->superSampling->getReal();
			texDef->heightFactor = this->superSampling->getReal();
			texDef->textureFlags = Ogre::TextureFlags::RenderToTexture | Ogre::TextureFlags::MsaaExplicitResolve;
			texDef->format = Ogre::PFG_RGBA16_FLOAT;

			if (this->msaaLevel > 1)
			{
				texDef->fsaa = Ogre::StringConverter::toString(this->msaaLevel);
			}

			if (true == this->useDistortion->getBool())
			{
				Ogre::TextureDefinitionBase::TextureDefinition* distortionTexDef = compositorNodeDefinition->addTextureDefinition("rt_distortion");
				distortionTexDef->width = 0.0f;
				distortionTexDef->height = 0.0f;
				distortionTexDef->format = Ogre::PFG_RGBA16_FLOAT;
				distortionTexDef->depthBufferFormat = Ogre::PFG_D32_FLOAT;
				distortionTexDef->preferDepthTexture = true;
				distortionTexDef->depthBufferId = 2;
				distortionTexDef->textureFlags = Ogre::TextureFlags::RenderToTexture;
			}

			if (true == this->useSSAO->getBool())
			{
				// gBufferNormals - with explicit_resolve
				Ogre::TextureDefinitionBase::TextureDefinition* gBufferNormalsTexDef = compositorNodeDefinition->addTextureDefinition("gBufferNormals");
				gBufferNormalsTexDef->width = 0;
				gBufferNormalsTexDef->height = 0;
				gBufferNormalsTexDef->widthFactor = this->superSampling->getReal();
				gBufferNormalsTexDef->heightFactor = this->superSampling->getReal();
				gBufferNormalsTexDef->format = Ogre::PFG_R10G10B10A2_UNORM;
				gBufferNormalsTexDef->textureFlags = Ogre::TextureFlags::RenderToTexture;
				if (this->msaaLevel > 1)
				{
					gBufferNormalsTexDef->fsaa = Ogre::StringConverter::toString(this->msaaLevel);
					gBufferNormalsTexDef->textureFlags |= Ogre::TextureFlags::MsaaExplicitResolve;
				}
			}

			Ogre::TextureDefinitionBase::TextureDefinition* depthTextureTexDef = compositorNodeDefinition->addTextureDefinition("depthTexture");
			depthTextureTexDef->width = 0;
			depthTextureTexDef->height = 0;
			depthTextureTexDef->widthFactor = this->superSampling->getReal();
			depthTextureTexDef->heightFactor = this->superSampling->getReal();
			depthTextureTexDef->format = Ogre::PFG_D32_FLOAT;
			depthTextureTexDef->textureFlags = Ogre::TextureFlags::RenderToTexture;
			if (this->msaaLevel > 1)
			{
				depthTextureTexDef->fsaa = Ogre::StringConverter::toString(this->msaaLevel);
			}
			
			Ogre::RenderTargetViewDef* rtv = compositorNodeDefinition->addRenderTextureView("rt0");
			Ogre::RenderTargetViewEntry attachment;
			attachment.textureName = "rt0";
			rtv->colourAttachments.push_back(attachment);

			if (true == this->useSSAO->getBool())
			{
				// MRT: rt0 + gBufferNormals
				Ogre::RenderTargetViewEntry normalsAttachment;
				normalsAttachment.textureName = "gBufferNormals";
				rtv->colourAttachments.push_back(normalsAttachment);
			}

			// Attach depth texture
			rtv->depthAttachment.textureName = "depthTexture";

			texDef = compositorNodeDefinition->addTextureDefinition("rt1");
			texDef->width = 0;
			texDef->height = 0;
			texDef->widthFactor = this->superSampling->getReal();
			texDef->heightFactor = this->superSampling->getReal();
			texDef->format = Ogre::PFG_RGBA16_FLOAT;
			texDef->textureFlags = Ogre::TextureFlags::RenderToTexture | Ogre::TextureFlags::MsaaExplicitResolve;

			if (this->msaaLevel > 1)
			{
				texDef->fsaa = Ogre::StringConverter::toString(this->msaaLevel);
			}

			rtv = compositorNodeDefinition->addRenderTextureView("rt1");
			attachment.textureName = "rt1";
			rtv->colourAttachments.push_back(attachment);
			rtv->depthBufferId = Ogre::DepthBuffer::POOL_DEFAULT;

			if (true == this->useHdr->getBool())
			{
				texDef = compositorNodeDefinition->addTextureDefinition("oldLumRt");
				texDef->width = 0.0f;
				texDef->height = 0.0f;
				texDef->format = Ogre::PFG_R16_FLOAT;
				texDef->textureFlags = Ogre::TextureFlags::RenderToTexture;

				rtv = compositorNodeDefinition->addRenderTextureView("oldLumRt");
				attachment.textureName = "oldLumRt";
				rtv->colourAttachments.push_back(attachment);
				rtv->depthBufferId = Ogre::DepthBuffer::POOL_NO_DEPTH;
			}

			if (true == this->useDistortion->getBool())
			{
				rtv = compositorNodeDefinition->addRenderTextureView("rt_distortion");
				Ogre::RenderTargetViewEntry attachment;
				attachment.textureName = "rt_distortion";
				rtv->colourAttachments.push_back(attachment);
				rtv->depthBufferId = 2;
			}

			unsigned short numTargetPass = 1;
			if (true == this->useHdr->getBool())
			{
				numTargetPass++;
			}
			if (true == this->useDistortion->getBool())
			{
				numTargetPass++;
			}
			if (true == this->useSSAO->getBool())
			{
				numTargetPass += 4;
			}

			compositorNodeDefinition->setNumTargetPass(numTargetPass);

			// rt0 target
			{
				Ogre::CompositorTargetDef* targetDef = compositorNodeDefinition->addTargetPass("rt0");

				// Clear Pass
				{
					Ogre::CompositorPassClearDef* passClear;
					auto pass = targetDef->addPass(Ogre::PASS_CLEAR);
					passClear = static_cast<Ogre::CompositorPassClearDef*>(pass);

					passClear->mClearColour[0] = Ogre::ColourValue(0.2f, 0.4f, 0.6f) * 60.0f;

					if (true == this->useSSAO->getBool())
					{
						// Clear the normals buffer to a neutral value
						passClear->mClearColour[1] = Ogre::ColourValue(0.5f, 0.5f, 1.0f, 1.0f);
						passClear->mStoreActionColour[1] = Ogre::StoreAction::Store;
					}

					passClear->mStoreActionColour[0] = Ogre::StoreAction::Store;
					passClear->mStoreActionDepth = Ogre::StoreAction::Store;
					passClear->mStoreActionStencil = Ogre::StoreAction::Store;

					passClear->mProfilingId = "NOWA_Background_Clear_Pass_Clear";
				}

				// Render Scene (RQ 0-1, before background) - LOAD, do NOT clear again
				{
					Ogre::CompositorPassSceneDef* passScene;
					auto pass = targetDef->addPass(Ogre::PASS_SCENE);
					passScene = static_cast<Ogre::CompositorPassSceneDef*>(pass);

					if (true == this->canUseReflection)
					{
						passScene->mExposedTextures.emplace_back(Ogre::IdString("rt1"));
					}

					passScene->setAllLoadActions(Ogre::LoadAction::Load);

					passScene->mStoreActionColour[0] = Ogre::StoreAction::StoreOrResolve;

					if (true == this->useSSAO->getBool())
					{
						passScene->mStoreActionColour[1] = Ogre::StoreAction::StoreOrResolve;
						passScene->mGenNormalsGBuf = true;
					}

					passScene->mStoreActionDepth = Ogre::StoreAction::Store;
					passScene->mStoreActionStencil = Ogre::StoreAction::DontCare;

					passScene->mFirstRQ = 0;
					passScene->mLastRQ = 1;

					if (true == this->usePlanarReflection->getBool())
					{
						passScene->mIdentifier = 25001;
					}

					passScene->mProfilingId = "NOWA_Background_Before_Background_Pass_Scene";
					passScene->mCameraName = this->cameraComponent->getCamera()->getName();

					passScene->mIncludeOverlays = false;
					passScene->mUpdateLodLists = true;
				}

				// Quad pass Background
				{
					Ogre::CompositorPassQuadDef* passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));
					passQuad->mMaterialName = "NOWABackgroundScroll";
					passQuad->mFrustumCorners = Ogre::CompositorPassQuadDef::NO_CORNERS;

					passQuad->setAllLoadActions(Ogre::LoadAction::Load);
					passQuad->mStoreActionColour[0] = Ogre::StoreAction::Store;

					if (true == this->useSSAO->getBool())
					{
						// Don't modify the normals buffer - background is at infinity
						passQuad->mStoreActionColour[1] = Ogre::StoreAction::Store;
					}

					passQuad->mStoreActionDepth = Ogre::StoreAction::Store;

					passQuad->mProfilingId = "NOWABackgroundScroll_Pass_Quad";
				}

				// Render Scene OPAQUE V2 (RQ 2-94, after background)
				{
					Ogre::CompositorPassSceneDef* passScene;
					auto pass = targetDef->addPass(Ogre::PASS_SCENE);
					passScene = static_cast<Ogre::CompositorPassSceneDef*>(pass);

					passScene->mIncludeOverlays = false;

					passScene->mShadowNode = WorkspaceModule::getInstance()->shadowNodeName;
					passScene->mShadowNodeRecalculation = Ogre::ShadowNodeRecalculation::SHADOW_NODE_REUSE;

					passScene->mCameraName = this->cameraComponent->getCamera()->getName();

					passScene->mFirstRQ = 2;
					passScene->mLastRQ = 99;

					if (true == this->useSSAO->getBool())
					{
						passScene->mGenNormalsGBuf = true;
						passScene->mStoreActionColour[1] = Ogre::StoreAction::StoreOrResolve;
					}

					passScene->mUpdateLodLists = true;

					passScene->setAllLoadActions(Ogre::LoadAction::Load);
					passScene->mStoreActionColour[0] = Ogre::StoreAction::StoreOrResolve;
					passScene->mStoreActionDepth = Ogre::StoreAction::Store;
					passScene->mStoreActionStencil = Ogre::StoreAction::DontCare;

					passScene->mProfilingId = "NOWA_Background_After_Background_Opaque_V2_Pass_Scene";

					if (true == this->usePlanarReflection->getBool())
					{
						passScene->mIdentifier = 25001;
					}
				}

				// Render Scene OPAQUE V1 (RQ 100-199, after background)
				{
					Ogre::CompositorPassSceneDef* passScene;
					auto pass = targetDef->addPass(Ogre::PASS_SCENE);
					passScene = static_cast<Ogre::CompositorPassSceneDef*>(pass);

					passScene->mIncludeOverlays = false;

					passScene->mShadowNode = WorkspaceModule::getInstance()->shadowNodeName;
					passScene->mShadowNodeRecalculation = Ogre::ShadowNodeRecalculation::SHADOW_NODE_REUSE;

					passScene->mCameraName = this->cameraComponent->getCamera()->getName();

					passScene->mFirstRQ = 100;
					passScene->mLastRQ = 199;

					if (true == this->useSSAO->getBool())
					{
						passScene->mGenNormalsGBuf = true;
						passScene->mStoreActionColour[1] = Ogre::StoreAction::StoreOrResolve;
					}

					passScene->mUpdateLodLists = false;

					passScene->setAllLoadActions(Ogre::LoadAction::Load);
					passScene->mStoreActionColour[0] = Ogre::StoreAction::StoreOrResolve;
					passScene->mStoreActionDepth = Ogre::StoreAction::Store;
					passScene->mStoreActionStencil = Ogre::StoreAction::DontCare;

					passScene->mProfilingId = "NOWA_Background_After_Background_Opaque_V1_Pass_Scene";

					if (true == this->usePlanarReflection->getBool())
					{
						passScene->mIdentifier = 25001;
					}
				}

				// Render Scene TRANSPARENT (FAST-high) (RQ 200-249, after background)
				{
					Ogre::CompositorPassSceneDef* passScene;
					auto pass = targetDef->addPass(Ogre::PASS_SCENE);
					passScene = static_cast<Ogre::CompositorPassSceneDef*>(pass);

					passScene->mIncludeOverlays = false;

					passScene->mShadowNode = WorkspaceModule::getInstance()->shadowNodeName;
					passScene->mShadowNodeRecalculation = Ogre::ShadowNodeRecalculation::SHADOW_NODE_REUSE;

					passScene->mCameraName = this->cameraComponent->getCamera()->getName();

					passScene->mFirstRQ = 200;
					passScene->mLastRQ = 224;

					passScene->mGenNormalsGBuf = false;
					passScene->mUpdateLodLists = false;

					passScene->setAllLoadActions(Ogre::LoadAction::Load);
					passScene->mStoreActionColour[0] = Ogre::StoreAction::StoreOrResolve;
					passScene->mStoreActionDepth = Ogre::StoreAction::Store;
					passScene->mStoreActionStencil = Ogre::StoreAction::DontCare;

					passScene->mProfilingId = "NOWA_Background_After_Background_Transparent_Pass_Scene";

					if (true == this->usePlanarReflection->getBool())
					{
						passScene->mIdentifier = 25001;
					}
				}

				// Render Scene TRANSPARENT V1 (RQ 225-224, after sky)
				{
					Ogre::CompositorPassSceneDef* passScene;
					auto pass = targetDef->addPass(Ogre::PASS_SCENE);
					passScene = static_cast<Ogre::CompositorPassSceneDef*>(pass);

					passScene->mIncludeOverlays = false;

					passScene->mShadowNode = WorkspaceModule::getInstance()->shadowNodeName;
					passScene->mShadowNodeRecalculation = Ogre::ShadowNodeRecalculation::SHADOW_NODE_REUSE;

					passScene->mCameraName = this->cameraComponent->getCamera()->getName();

					passScene->mFirstRQ = 225;
					passScene->mLastRQ = 255;  // Covers all remaining queues including GIZMO and MAX

					passScene->mGenNormalsGBuf = false;
					passScene->mUpdateLodLists = false;

					passScene->setAllLoadActions(Ogre::LoadAction::Load);
					passScene->mStoreActionColour[0] = Ogre::StoreAction::StoreOrResolve;
					passScene->mStoreActionDepth = Ogre::StoreAction::Store;
					passScene->mStoreActionStencil = Ogre::StoreAction::DontCare;

					passScene->mProfilingId = "NOWA_Sky_After_Background_Transparent_V1_Pass_Scene";

					if (true == this->usePlanarReflection->getBool())
					{
						passScene->mIdentifier = 25001;
					}
				}

				// ===== ADD OVERLAY PASS FOR PCC =====
				if (true == this->usePCC->getBool())
				{
					Ogre::CompositorPassSceneDef* overlayPass = static_cast<Ogre::CompositorPassSceneDef*>(targetDef->addPass(Ogre::PASS_SCENE));

					overlayPass->setAllLoadActions(Ogre::LoadAction::Load);
					overlayPass->mStoreActionColour[0] = Ogre::StoreAction::Store;
					overlayPass->mStoreActionDepth = Ogre::StoreAction::DontCare;
					overlayPass->mStoreActionStencil = Ogre::StoreAction::DontCare;

					overlayPass->mFirstRQ = 251;

					overlayPass->mIncludeOverlays = false;
					overlayPass->mUpdateLodLists = false;

					overlayPass->mShadowNode = WorkspaceModule::getInstance()->shadowNodeName;
					overlayPass->mShadowNodeRecalculation = Ogre::ShadowNodeRecalculation::SHADOW_NODE_REUSE;

					overlayPass->mProfilingId = "NOWA_Pbs_PCC_Overlays_Pass_Scene";

					if (true == this->usePlanarReflection->getBool())
					{
						overlayPass->mIdentifier = 25001;
					}
				}
			}

			if (true == this->useHdr->getBool())
			{
				Ogre::CompositorTargetDef* targetDef = compositorNodeDefinition->addTargetPass("oldLumRt");

				Ogre::CompositorPassClearDef* passClear;
				auto pass = targetDef->addPass(Ogre::PASS_CLEAR);
				passClear = static_cast<Ogre::CompositorPassClearDef*>(pass);

				passClear->mNumInitialPasses = 1;
				passClear->mClearColour[0] = Ogre::ColourValue(0.01f, 0.01f, 0.01f, 1.0f);

				passClear->mProfilingId = "NOWA_Background_Hdr_Pass_Clear";
			}

			if (true == this->useDistortion->getBool())
			{
				Ogre::CompositorTargetDef* targetDef = compositorNodeDefinition->addTargetPass("rt_distortion");

				Ogre::CompositorPassSceneDef* passScene;
				auto pass = targetDef->addPass(Ogre::PASS_SCENE);
				passScene = static_cast<Ogre::CompositorPassSceneDef*>(pass);

				passScene->mProfilingId = "NOWA_Background_Distortion_Pass_Scene";
				passScene->setAllLoadActions(Ogre::LoadAction::Clear);
				passScene->mClearColour[0] = Ogre::ColourValue(0.5f, 0.5f, 0.0f, 0.0f);
				passScene->mUpdateLodLists = false;
				passScene->mIncludeOverlays = false;
				passScene->mFirstRQ = 16;
				passScene->mLastRQ = 17;

				this->createDistortionNode();
			}

			// ===== Output channels =====
			unsigned short outputChannelCount = 2; // rt0, rt1

			if (true == this->useHdr->getBool())
			{
				outputChannelCount++;
			}

			if (true == this->useDistortion->getBool())
			{
				outputChannelCount++;
			}

			if (true == this->useSSAO->getBool())
			{
				outputChannelCount++; // gBufferNormals
			}

			outputChannelCount++; // depthTexture

			compositorNodeDefinition->setNumOutputChannels(outputChannelCount);

			unsigned short outputChannel = 0;

			compositorNodeDefinition->mapOutputChannel(outputChannel++, "rt0");
			compositorNodeDefinition->mapOutputChannel(outputChannel++, "rt1");

			if (true == this->useHdr->getBool())
			{
				compositorNodeDefinition->mapOutputChannel(outputChannel++, "oldLumRt");
			}
			if (true == this->useDistortion->getBool())
			{
				compositorNodeDefinition->mapOutputChannel(outputChannel++, "rt_distortion");
			}

			if (true == this->useSSAO->getBool())
			{
				compositorNodeDefinition->mapOutputChannel(outputChannel++, "gBufferNormals");
			}

			compositorNodeDefinition->mapOutputChannel(outputChannel++, "depthTexture");

			this->createFinalRenderNode();

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

						// Render Scene
						{
							Ogre::CompositorPassSceneDef* passScene;
							auto pass = targetDef->addPass(Ogre::PASS_SCENE);
							passScene = static_cast<Ogre::CompositorPassSceneDef*>(pass);
							passScene->mProfilingId = "NOWA_Background_PlanarReflection_Pass_Scene";

							Ogre::ColourValue color(0.2f, 0.4f, 0.6f);
							passScene->setAllLoadActions(Ogre::LoadAction::Clear);
							passScene->mClearColour[0] = color;

							passScene->mStoreActionColour[0] = Ogre::StoreAction::StoreOrResolve;
							passScene->mStoreActionDepth = Ogre::StoreAction::DontCare;
							passScene->mStoreActionStencil = Ogre::StoreAction::DontCare;

							passScene->mFirstRQ = 0;
							passScene->mLastRQ = 2;

							passScene->mIncludeOverlays = false;
							passScene->mShadowNode = WorkspaceModule::getInstance()->shadowNodeName;
						}

						// Background quad
						{
							Ogre::CompositorPassQuadDef* passQuad;
							auto pass = targetDef->addPass(Ogre::PASS_QUAD);
							passQuad = static_cast<Ogre::CompositorPassQuadDef*>(pass);

							passQuad->mMaterialName = "NOWABackgroundScroll";
							passQuad->mProfilingId = "NOWA_Background_Background_Pass_Quad";
						}

						// Render Scene (after background)
						{
							Ogre::CompositorPassSceneDef* passScene;
							auto pass = targetDef->addPass(Ogre::PASS_SCENE);
							passScene = static_cast<Ogre::CompositorPassSceneDef*>(pass);

							passScene->mIncludeOverlays = false;
							passScene->mShadowNode = WorkspaceModule::getInstance()->shadowNodeName;
							passScene->mShadowNodeRecalculation = Ogre::ShadowNodeRecalculation::SHADOW_NODE_REUSE;
							passScene->mFirstRQ = 2;
							passScene->mProfilingId = "NOWA_Background_After_Background_Pass_Scene";
						}

						// Generate Mipmaps
						{
							Ogre::CompositorPassMipmapDef* passMipmap;
							passMipmap = static_cast<Ogre::CompositorPassMipmapDef*>(targetDef->addPass(Ogre::PASS_MIPMAP));

							passMipmap->mMipmapGenerationMethod = Ogre::CompositorPassMipmapDef::ComputeHQ;

							passMipmap->mProfilingId = "NOWA_Background_Pass_Mipmap";
						}
					}
				}
			}
		}

		Ogre::String strMaterialName = "NOWABackgroundScroll";
		this->materialBackgroundPtr = Ogre::MaterialManager::getSingletonPtr()->getByName(strMaterialName);

		if (true == this->materialBackgroundPtr.isNull())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[WorkspaceBackgroundComponent] Could not set: " + this->workspaceName + " because the material: '" + strMaterialName + "' does not exist!");
			throw Ogre::Exception(Ogre::Exception::ERR_ITEM_NOT_FOUND, "Could not create: " + this->workspaceName + " because the material: '" + strMaterialName + "' does not exist!", "NOWA");
		}

		Ogre::Material* material = this->materialBackgroundPtr.getPointer();
		this->passBackground = material->getTechnique(0)->getPass(0);
	}

	bool WorkspaceBackgroundComponent::internalCreateWorkspace(Ogre::CompositorWorkspaceDef* workspaceDef)
	{
		// Call base workspace creation, which does already main node connections
		WorkspaceBaseComponent::baseCreateWorkspace(workspaceDef);

		Ogre::String strMaterialName = "NOWABackgroundScroll";
		this->materialBackgroundPtr = Ogre::MaterialManager::getSingletonPtr()->getByName(strMaterialName);

		if (true == this->materialBackgroundPtr.isNull())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[WorkspaceBackgroundComponent] Could not set: " + this->workspaceName + " because the material: '" + strMaterialName + "' does not exist!");
			throw Ogre::Exception(Ogre::Exception::ERR_ITEM_NOT_FOUND, "Could not create: " + this->workspaceName + " because the material: '" + strMaterialName + "' does not exist!", "NOWA");
		}

		Ogre::Material* material = this->materialBackgroundPtr.getPointer();
		this->passBackground = material->getTechnique(0)->getPass(0);

		Ogre::HlmsSamplerblock samplerblock;
		samplerblock.mU = Ogre::TextureAddressingMode::TAM_WRAP;
		samplerblock.mV = Ogre::TextureAddressingMode::TAM_WRAP;
		samplerblock.mW = Ogre::TextureAddressingMode::TAM_WRAP;
		samplerblock.mMaxAnisotropy = 1;
		samplerblock.mMagFilter = Ogre::FO_POINT;
		samplerblock.mMinFilter = Ogre::FO_POINT;
		samplerblock.mMipFilter = Ogre::FO_NONE;
		samplerblock.setFiltering(Ogre::TextureFilterOptions::TFO_NONE);

		this->passBackground->setSamplerblock(samplerblock);

		for (size_t i = 0; i < 9; i++)
		{
			Ogre::TextureUnitState* tex = this->passBackground->getTextureUnitState(i);

			if (true == this->hardwareGammaEnabled->getBool())
			{
				tex->setGamma(8.0);
			}
			tex->setHardwareGammaEnabled(this->hardwareGammaEnabled->getBool());
			tex->setNumMipmaps(0);
		}

		material->compile();

		this->addWorkspace(workspaceDef);

		if (this->msaaLevel == 1)
		{
			this->initializeSmaa(SMAA_PRESET_LOW, EdgeDetectionLuma);
		}
		else if (this->msaaLevel == 2)
		{
			this->initializeSmaa(SMAA_PRESET_MEDIUM, EdgeDetectionLuma);
		}
		else if (this->msaaLevel == 4)
		{
			this->initializeSmaa(SMAA_PRESET_HIGH, EdgeDetectionColor);
		}
		else
		{
			this->initializeSmaa(SMAA_PRESET_ULTRA, EdgeDetectionColor);
		}

		if (true == this->useHdr->getBool())
		{
			this->initializeHdr(this->msaaLevel);
		}

		return nullptr != this->workspace;
	}

	void WorkspaceBackgroundComponent::removeWorkspace(void)
	{
		WorkspaceBaseComponent::removeWorkspace();

		if (false == this->materialBackgroundPtr.isNull())
		{
			this->materialBackgroundPtr.setNull();
		}

		NOWA::GraphicsModule::getInstance()->removeTrackedPass(this->passBackground);

		this->passBackground = nullptr;
	}

	void WorkspaceBackgroundComponent::changeBackground(unsigned short index, const Ogre::String& backgroundTextureName)
	{
		if (index >= 9)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
				"[WorkspaceBackgroundComponent] Invalid layer index: " + Ogre::StringConverter::toString(index));
			return;
		}

		this->layerEnabled[index] = 1.0f;

		Ogre::String strMaterialName = "NOWABackgroundScroll";
		this->materialBackgroundPtr = Ogre::MaterialManager::getSingletonPtr()->getByName(strMaterialName);

		if (this->materialBackgroundPtr.isNull())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
				"[WorkspaceBackgroundComponent] Could not set: " + this->workspaceName +
				" because the material: '" + strMaterialName + "' does not exist!");
			throw Ogre::Exception(Ogre::Exception::ERR_ITEM_NOT_FOUND,
				"Could not create: " + this->workspaceName +
				" because the material: '" + strMaterialName + "' does not exist!", "NOWA");
		}

		Ogre::Material* material = this->materialBackgroundPtr.getPointer();
		this->passBackground = material->getTechnique(0)->getPass(0);

		// Update layer enabled flags
		this->passBackground->getFragmentProgramParameters()->setNamedConstant("layerEnabled", this->layerEnabled, 9, 1);

		// Change background texture
		Ogre::TextureUnitState* tex = this->passBackground->getTextureUnitState(index);
		tex->setNumMipmaps(0);
		tex->setTextureName(backgroundTextureName);

		// Set proper texture addressing for scrolling
		// tex->setTextureAddressingMode(Ogre::TextureUnitState::TAM_WRAP);

		if (this->hardwareGammaEnabled->getBool())
		{
			tex->setGamma(8.0);
		}
		tex->setHardwareGammaEnabled(this->hardwareGammaEnabled->getBool());

		this->materialBackgroundPtr->compile();
	}

	void WorkspaceBackgroundComponent::setBackgroundScrollSpeedX(unsigned short index, Ogre::Real speedX)
	{
		if (index >= 9)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
				"[WorkspaceBackgroundComponent] Invalid layer index for X speed: " + Ogre::StringConverter::toString(index));
			return;
		}

		// Update shader parameters
		if (this->passBackground)
		{
			NOWA::GraphicsModule::getInstance()->updatePassSpeedsX(this->passBackground, index, speedX);
		}
	}

	void WorkspaceBackgroundComponent::setBackgroundScrollSpeedY(unsigned short index, Ogre::Real speedY)
	{
		if (index >= 9)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
				"[WorkspaceBackgroundComponent] Invalid layer index for Y speed: " + Ogre::StringConverter::toString(index));
			return;
		}

		// Update shader parameters
		if (this->passBackground)
		{
			NOWA::GraphicsModule::getInstance()->updatePassSpeedsY(this->passBackground, index, speedY);
		}
	}

	void WorkspaceBackgroundComponent::compileBackgroundMaterial(void)
	{
		auto materialBackgroundPtr = this->materialBackgroundPtr;
		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("BackgroundScrollComponent::compileBackgroundMaterial", _1(materialBackgroundPtr),
		{
			materialBackgroundPtr->compile();
		});
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

	bool WorkspaceCustomComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		bool success = WorkspaceBaseComponent::init(propertyElement);

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

	void WorkspaceCustomComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		WorkspaceBaseComponent::writeXML(propertiesXML, doc);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CustomWorkspaceName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->customWorkspaceName->getString())));
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
		// Do not call base workspace creation, which does already main node connections, because just a custom script is used
		// WorkspaceBaseComponent::baseCreateWorkspace(workspaceDef);

		if (false == this->customWorkspaceName->getString().empty())
		{
			const Ogre::IdString workspaceNameId(this->customWorkspaceName->getString());

			if (false == WorkspaceModule::getInstance()->getCompositorManager()->hasWorkspaceDefinition(workspaceNameId))
			{
				Ogre::ColourValue color(this->backgroundColor->getVector3().x, this->backgroundColor->getVector3().y, this->backgroundColor->getVector3().z);
				this->compositorManager->createBasicWorkspaceDef(this->customWorkspaceName->getString(), color, Ogre::IdString());
			}

			// If prior ws component has some channels defined
			if (true == this->externalChannels.empty())
			{
				this->externalChannels.resize(1);
				this->externalChannels[0] = Core::getSingletonPtr()->getOgreRenderWindow()->getTexture();
			}

			this->workspace = WorkspaceModule::getInstance()->getCompositorManager()->addWorkspace(this->gameObjectPtr->getSceneManager(), externalChannels, this->cameraComponent->getCamera(), this->customWorkspaceName->getString(), true);
		}

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