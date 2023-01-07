#include "NOWAPrecompiled.h"
#include "WorkspaceComponents.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "Modules/WorkspaceModule.h"
#include "main/Core.h"
#include "CameraComponent.h"
#include "ReflectionCameraComponent.h"
#include "PlanarReflectionComponent.h"
#include "HdrEffectComponent.h"
#include "DatablockPbsComponent.h"
#include "Compositor/Pass/PassQuad/OgreCompositorPassQuadDef.h"
#include "Compositor/Pass/PassMipmap/OgreCompositorPassMipmapDef.h"
#include "Compositor/Pass/PassScene/OgreCompositorPassScene.h"

// #include "Compositor/Pass/PassScene/OgreCompositorShadowNodeDef.h"
#include "Compositor/OgreCompositorWorkspaceListener.h"

#include "terra/TerraShadowMapper.h"

namespace NOWA
{
	using namespace rapidxml;

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
		backgroundColor(new Variant(WorkspaceBaseComponent::AttrBackgroundColor(), Ogre::Vector3(0.2f, 0.4f, 0.6f), this->attributes)),
		viewportRect(new Variant(WorkspaceBaseComponent::AttrViewportRect(), Ogre::Vector4(0.0f, 0.0f, 1.0f, 1.0f), this->attributes)),
		superSampling(new Variant(WorkspaceBaseComponent::AttrSuperSampling(), Ogre::Real(1.0f), this->attributes)),
		useHdr(new Variant(WorkspaceBaseComponent::AttrUseHdr(), false, this->attributes)),
		useReflection(new Variant(WorkspaceBaseComponent::AttrUseReflection(), false, this->attributes)),
		reflectionCameraGameObjectId(new Variant(WorkspaceBaseComponent::AttrReflectionCameraGameObjectId(), static_cast<unsigned long>(0), this->attributes, true)),
		usePlanarReflection(new Variant(WorkspaceBaseComponent::AttrUsePlanarReflection(), false, this->attributes)),
		cameraComponent(nullptr),
		workspace(nullptr),
		msaaLevel(1u),
		hlms(nullptr),
		pbs(nullptr),
		unlit(nullptr),
		hlmsManager(nullptr),
		compositorManager(nullptr),
		workspaceCubemap(nullptr),
		cubemap(nullptr),
		planarReflections(nullptr),
		planarReflectionsWorkspaceListener(nullptr),
		useTerra(false),
		terra(nullptr)
	{
		this->backgroundColor->setUserData(GameObject::AttrActionColorDialog());
		this->superSampling->setDescription("Sets the supersampling for whole scene texture rendering. E.g. a value of 0.25 will pixelize the scene for retro Game experience.");
		this->useReflection->setDescription("Activates reflection, which is rendered by a dynamic cubemap. This component works in conjunction with a ReflectionCameraComponent. Attention: Acitvating this mode decreases the application's performance considerably!");
		this->usePlanarReflection->setDescription("Activates planar reflection, which is used for mirror planes in conjunction with a PlanarReflectionComponent.");
		this->referenceId->setVisible(false);

		this->useReflection->setUserData(GameObject::AttrActionNeedRefresh());
	}

	WorkspaceBaseComponent::~WorkspaceBaseComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[WorkspaceBaseComponent] Destructor workspace base component for game object: " + this->gameObjectPtr->getName());
		
		this->cameraComponent = nullptr;

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
			this->useReflection->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ReflectionCameraGameObjectId")
		{
			this->reflectionCameraGameObjectId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UsePlanarReflection")
		{
			this->usePlanarReflection->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseTerra")
		{
			this->useTerra = XMLConverter::getAttribBool(propertyElement, "data");
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
		this->compositorManager = Core::getSingletonPtr()->getOgreRoot()->getCompositorManager2();

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

		std::vector<bool> reflections(GameObjectController::getInstance()->getGameObjects()->size());
		auto gameObjects = GameObjectController::getInstance()->getGameObjects();

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
					gameObjectPtr->getAttribute(GameObject::AttrUseReflection())->setVisible(true);
					gameObjectPtr->setUseReflection(false);
				}
			}

			WorkspacePbsComponent* workspacePbsComponent = dynamic_cast<WorkspacePbsComponent*>(this);
			WorkspaceSkyComponent* workspaceSkyComponent = nullptr;
			WorkspaceBackgroundComponent* workspaceBackgroundComponent = nullptr;
			WorkspaceCustomComponent* workspaceCustomComponent = nullptr;
			
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
		else
		{
			for (auto& it = gameObjects->begin(); it != gameObjects->end(); ++it)
			{
				auto& gameObjectPtr = it->second;
				if (nullptr != gameObjectPtr)
				{
					gameObjectPtr->getAttribute(GameObject::AttrUseReflection())->setVisible(false);
				}
			}
		}

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[WorkspacePbsComponent] Creating workspace: " + this->workspaceName);

		const Ogre::IdString workspaceNameId(this->workspaceName);
		if (false == this->compositorManager->hasWorkspaceDefinition(workspaceNameId))
		{
			Ogre::ColourValue color(this->backgroundColor->getVector3().x, this->backgroundColor->getVector3().y, this->backgroundColor->getVector3().z);
			this->compositorManager->createBasicWorkspaceDef(this->workspaceName, color, Ogre::IdString());
		}

		Ogre::CompositorWorkspaceDef* workspaceDef = WorkspaceModules::getInstance()->getCompositorManager()->getWorkspaceDefinition(this->workspaceName);
		workspaceDef->clearAll();

		Ogre::CompositorManager2::CompositorNodeDefMap nodeDefs = WorkspaceModules::getInstance()->getCompositorManager()->getNodeDefinitions();

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
			GameObjectPtr reflectionCameraGameObjectPtr = GameObjectController::getInstance()->getGameObjectFromId(this->reflectionCameraGameObjectId->getULong());
			if (nullptr != reflectionCameraGameObjectPtr)
			{
				auto& reflectionCameraCompPtr = NOWA::makeStrongPtr(reflectionCameraGameObjectPtr->getComponent<ReflectionCameraComponent>());
				if (nullptr != reflectionCameraCompPtr)
				{
					// Set this component as back reference, because when reflection camera is deleted, reflection must be disabled immediately, else a render crash occurs
					reflectionCameraCompPtr->workspaceBaseComponent = this;

					// We first create the Cubemap workspace and pass it to the final workspace
					// that does the real rendering.
					//
					// If in your application you need to create a workspace but don't have a cubemap yet,
					// you can either programatically modify the workspace definition (which is cumbersome)
					// or just pass a PF_NULL texture that works as a dud and barely consumes any memory.
					// See Tutorial_Terrain for an example of PF_NULL dud.

					//A RenderTarget created with TU_AUTOMIPMAP means the compositor still needs to
					//explicitly generate the mipmaps by calling generate_mipmaps. It's just an API
					//hint to tell the GPU we will be using the mipmaps auto generation routines.
					Ogre::TextureGpuManager* textureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();
					this->cubemap = textureManager->createTexture("cubemap", Ogre::GpuPageOutStrategy::Discard,
						Ogre::TextureFlags::RenderToTexture | Ogre::TextureFlags::AllowAutomipmaps, Ogre::TextureTypes::TypeCube);

					unsigned int cubeTextureSize = Ogre::StringConverter::parseUnsignedInt(reflectionCameraCompPtr->getCubeTextureSize());

					this->cubemap->setResolution(cubeTextureSize, cubeTextureSize);
					this->cubemap->setNumMipmaps(Ogre::PixelFormatGpuUtils::getMaxMipmapCount(cubeTextureSize, cubeTextureSize));
					this->cubemap->setPixelFormat(Ogre::PFG_RGBA8_UNORM_SRGB);
					this->cubemap->_transitionTo(Ogre::GpuResidency::Resident, (Ogre::uint8*)0);

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

					this->workspaceCubemap = this->compositorManager->addWorkspace(this->gameObjectPtr->getSceneManager(), cubemapExternalChannels, reflectionCameraCompPtr->getCamera(),
						workspaceCubemapName, true, -1, (Ogre::UavBufferPackedVec*)0, &this->initialCubemapLayouts, &this->initialCubemapUavAccess);

					// Now setup the regular renderer
					this->externalChannels.resize(2);
					// Render window
					this->externalChannels[0] = Core::getSingletonPtr()->getOgreRenderWindow()->getTexture();
					this->externalChannels[1] = this->cubemap;
				}
			}
		}

		this->internalCreateCompositorNode();

		if (true == this->useTerra)
		{
			unsigned short externalInputTextureId = 1;

			if (false == this->useReflection->getBool())
			{
				this->externalChannels.resize(2);
			}
			else
			{
				this->externalChannels.resize(3);
				externalInputTextureId = 2;
			}

			//Terra's Shadow texture
			if (nullptr != terra)
			{
				//Terra is initialized
				const Ogre::ShadowMapper* shadowMapper = terra->getShadowMapper();
				shadowMapper->fillUavDataForCompositorChannel(&this->externalChannels[externalInputTextureId], this->initialCubemapLayouts, this->initialCubemapUavAccess);
			}
			else
			{
				//The texture is not available. Create a dummy dud.
				Ogre::TextureGpuManager* textureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();
				Ogre::TextureGpu* nullTex = textureManager->createOrRetrieveTexture("DummyNull", Ogre::GpuPageOutStrategy::Discard, Ogre::TextureFlags::ManualTexture, Ogre::TextureTypes::Type2D);
				nullTex->setResolution(1u, 1u);
				nullTex->setPixelFormat(Ogre::PFG_R10G10B10A2_UNORM);
				nullTex->scheduleTransitionTo(Ogre::GpuResidency::OnSystemRam);
				this->externalChannels[externalInputTextureId] = nullTex;
			}
		}

		if (true == this->usePlanarReflection->getBool())
		{
			if (false == this->compositorManager->hasWorkspaceDefinition(this->planarReflectionReflectiveWorkspaceName))
			{
				Ogre::ColourValue color(this->backgroundColor->getVector3().x, this->backgroundColor->getVector3().y, this->backgroundColor->getVector3().z);
				this->compositorManager->createBasicWorkspaceDef(this->planarReflectionReflectiveWorkspaceName, color, Ogre::IdString());

				Ogre::CompositorWorkspaceDef* workspaceDef = WorkspaceModules::getInstance()->getCompositorManager()->getWorkspaceDefinition(this->planarReflectionReflectiveWorkspaceName);

				workspaceDef->connectExternal(0, this->planarReflectionReflectiveRenderingNode, 0);
			}
		}

		bool success = this->internalCreateWorkspace(workspaceDef);

		if (true == this->usePlanarReflection->getBool() && true == success)
		{
			// Setup PlanarReflections, 1.0 = max distance
			this->planarReflections = new Ogre::PlanarReflections(this->gameObjectPtr->getSceneManager(), this->compositorManager, 1.0f, nullptr);
			this->planarReflectionsWorkspaceListener = new PlanarReflectionsWorkspaceListener(this->planarReflections);
			this->workspace->setListener(this->planarReflectionsWorkspaceListener);

			// Attention: Only one planar reflection setting is possible!
			this->pbs->setPlanarReflections(this->planarReflections);
		}

		if (true == this->useReflection->getBool() || true == this->usePlanarReflection->getBool())
		{
			// Restore the reflections
			unsigned int j = 0;
			for (auto& it = gameObjects->begin(); it != gameObjects->end(); ++it)
			{
				auto& gameObjectPtr = it->second;
				if (nullptr != gameObjectPtr)
				{
					gameObjectPtr->setUseReflection(reflections[j]);

					if (true == this->usePlanarReflection->getBool())
					{
						auto planarReflectionCompPtr = NOWA::makeStrongPtr(gameObjectPtr->getComponent<PlanarReflectionComponent>());
						if (nullptr != planarReflectionCompPtr)
						{
							planarReflectionCompPtr->setDatablockName(planarReflectionCompPtr->getDatablockName());
						}
					}

					j++;
				}
			}
		}

		return success;
	}

	void WorkspaceBaseComponent::removeWorkspace(void)
	{
		this->resetReflectionForAllEntities();

		if (nullptr != this->workspaceCubemap)
		{
			this->compositorManager->removeWorkspace(this->workspaceCubemap);
			this->workspaceCubemap = nullptr;
		}
		if (nullptr != this->cubemap)
		{
			Ogre::TextureGpuManager* textureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();
			textureManager->destroyTexture(this->cubemap);
			this->cubemap = nullptr;
		}
		
		if (nullptr != this->workspace)
		{
			this->workspace->setListener(nullptr);

			if (true == this->useTerra)
			{
				unsigned short externalInputTextureId = 1;
				if (true == this->useReflection->getBool())
				{
					externalInputTextureId = 2;
				}

				if (this->workspace->getExternalRenderTargets().size() > externalInputTextureId)
				{
					Ogre::TextureGpu* terraShadowTex = this->workspace->getExternalRenderTargets()[externalInputTextureId];
					if (terraShadowTex->getPixelFormat() == Ogre::PFG_NULL)
					{
						Ogre::TextureGpuManager* textureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();
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

		if (nullptr != this->cameraComponent)
		{
			WorkspaceModules::getInstance()->removeWorkspace(this->cameraComponent->getCamera());
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
		
	}

	Ogre::String WorkspaceBaseComponent::getClassName(void) const
	{
		return "WorkspaceBaseComponent";
	}

	Ogre::String WorkspaceBaseComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
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
							passDef->mClearColour[0] = backgroundColor;
							// clearDef->mColourValue = backgroundColor;
						}
					}
				}
			}
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
		if (Core::getSingletonPtr()->getOgreRenderWindow()->getMsaa() > 1u && caps->hasCapability(Ogre::RSC_EXPLICIT_FSAA_RESOLVE))
			return Core::getSingletonPtr()->getOgreRenderWindow()->getMsaa();

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

	void WorkspaceBaseComponent::resetReflectionForAllEntities(void)
	{
		Ogre::TextureGpuManager* hlmsTextureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();

		auto gameObjects = GameObjectController::getInstance()->getGameObjects();
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

	void WorkspaceBaseComponent::setPlanarReflectionsActor(unsigned long gameObjectId, bool useAccurateLighting, unsigned int width, unsigned int height, bool withMipmaps, bool useMipmapMethodCompute,
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

				this->planarReflections->setMaxActiveActors(planarReflectionActorIndex, this->planarReflectionReflectiveWorkspaceName, useAccurateLighting, width, height, withMipmaps, Ogre::PFG_RGBA8_UNORM_SRGB, useMipmapMethodCompute);

				Ogre::PlanarReflectionActor* actor = this->planarReflections->addActor(Ogre::PlanarReflectionActor(position, mirrorSize, orientation));

				// Make sure it's always activated (i.e. always win against other actors) unless it's not visible by the camera.
				actor->mActivationPriority = 0;
				this->planarReflectionActors.emplace_back(gameObjectId, planarReflectionActorIndex, actor);
			}
			else
			{
				// Just change values for the existing actor
				this->planarReflections->setMaxActiveActors(planarReflectionActorIndex, this->planarReflectionReflectiveWorkspaceName, useAccurateLighting, width, height, withMipmaps, Ogre::PFG_RGBA8_UNORM_SRGB, useMipmapMethodCompute);
			}
		}
	}

	void WorkspaceBaseComponent::removePlanarReflectionsActor(unsigned long gameObjectId)
	{
		for (size_t i = 0; i < this->planarReflectionActors.size(); i++)
		{
			if (std::get<0>(this->planarReflectionActors[i]) == gameObjectId)
			{
				Ogre::PlanarReflectionActor* actor = std::get<2>(this->planarReflectionActors[i]);
				this->planarReflections->destroyActor(actor);
				this->planarReflectionActors.erase(this->planarReflectionActors.begin() + i);
			}
		}
	}


	void WorkspaceBaseComponent::setBackgroundColor(const Ogre::Vector3& backgroundColor)
	{
		this->backgroundColor->setValue(backgroundColor);
		
		Ogre::ColourValue color(backgroundColor.x, backgroundColor.y, backgroundColor.z);
		this->changeBackgroundColor(color);
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
				hdrEffectCompPtr->applyHdrSkyColour(Ogre::ColourValue(0.2f, 0.4f, 0.6f), 1.0f);
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
		this->useReflection->setValue(useReflection);

		this->reflectionCameraGameObjectId->setVisible(useReflection);

		if (false == useReflection)
		{
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

		this->createWorkspace();
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

	void WorkspaceBaseComponent::setReflectionCameraGameObjectId(unsigned long reflectionCameraGameObjectId)
	{
		this->reflectionCameraGameObjectId->setValue(reflectionCameraGameObjectId);

		this->createWorkspace();
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

			if (true == this->useTerra)
			{
				compositorNodeDefinition->addTextureSourceName("TerraShadowTexture", 0, Ogre::TextureDefinitionBase::TEXTURE_INPUT);
			}

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
			texDef->msaa = this->msaaLevel;
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
			texDef->msaa = this->msaaLevel;
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

						if (true == this->useReflection->getBool())
						{
							//Our materials in this pass will be using this cubemap,
							//so we need to expose it to the pass.
							//Note: Even if it "just works" without exposing, the reason for
							//exposing is to ensure compatibility with Vulkan & D3D12.

							passScene->mExposedTextures.emplace_back(Ogre::IdString("rt1"));
						}

						if (true == this->useTerra)
						{
							passScene->mExposedTextures.emplace_back(Ogre::IdString("TerraShadowTexture"));
						}

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
						if (false == this->useTerra)
						{
							passScene->mShadowNode = "NOWAShadowNode";
						}
						else
						{
							passScene->mShadowNode = "NOWATerraShadowNode";
						}
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
							passScene->mShadowNode = "NOWAShadowNode";
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
	
			if (true == this->useTerra)
			{
				workspaceDef->connectExternal(1, this->renderingNodeName, 0);
			}
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

		if (true == this->externalChannels.empty())
		{
			this->externalChannels.resize(1);
			this->externalChannels[0] = Core::getSingletonPtr()->getOgreRenderWindow()->getTexture();
		}

		this->workspace = WorkspaceModules::getInstance()->getCompositorManager()->addWorkspace(this->gameObjectPtr->getSceneManager(), externalChannels,
			this->cameraComponent->getCamera(), this->workspaceName, true, -1, (Ogre::UavBufferPackedVec*)0,
			&this->initialCubemapLayouts, &this->initialCubemapUavAccess);

		this->setBackgroundColor(this->backgroundColor->getVector3());
		// For VR: disable this line and use NOWAPbsRenderingNodeVR
		this->changeViewportRect(0, this->viewportRect->getVector4());

		WorkspaceModules::getInstance()->setPrimaryWorkspace(this->cameraComponent->getCamera(), this);

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
			texDef->msaa = this->msaaLevel;
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
			texDef->msaa = this->msaaLevel;
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

						passScene->mShadowNode = "NOWAShadowNode";

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

						if (true == this->useReflection->getBool())
						{
							//Our materials in this pass will be using this cubemap,
							//so we need to expose it to the pass.
							//Note: Even if it "just works" without exposing, the reason for
							//exposing is to ensure compatibility with Vulkan & D3D12.

							passScene->mExposedTextures.emplace_back(Ogre::IdString("rt1"));
						}

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
						passScene->mShadowNode = "NOWAShadowNode";
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
							passScene->mShadowNode = "NOWAShadowNode";
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
							passScene->mShadowNode = "NOWAShadowNode";
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

		this->workspace = WorkspaceModules::getInstance()->getCompositorManager()->addWorkspace(sceneManager, Core::getSingletonPtr()->getOgreRenderWindow()->getTexture(), camera, workspaceName, true);

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

		// If prior ws component has some channels defined
		if (true == this->externalChannels.empty())
		{
			this->externalChannels.resize(1);
			this->externalChannels[0] = Core::getSingletonPtr()->getOgreRenderWindow()->getTexture();
		}

		this->workspace = WorkspaceModules::getInstance()->getCompositorManager()->addWorkspace(this->gameObjectPtr->getSceneManager(), externalChannels,
			this->cameraComponent->getCamera(), this->workspaceName, true, -1, (Ogre::UavBufferPackedVec*)0,
			&this->initialCubemapLayouts, &this->initialCubemapUavAccess);

		this->changeSkyBox(this->skyBoxName->getListSelectedValue());

		// this->initializeSmaa(WorkspaceModule::SMAA_PRESET_ULTRA, WorkspaceModule::EdgeDetectionColour);

		// this->initializeHdr(msaaLevel);

		WorkspaceModules::getInstance()->setPrimaryWorkspace(this->cameraComponent->getCamera(), this);

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
		: WorkspaceBaseComponent(),
		passBackgroundFar(nullptr),
		passBackgroundMiddle(nullptr),
		passBackgroundNear(nullptr)
	{
		
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
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[WorkspaceBackgroundComponent] Init workspace sky component for game object: " + this->gameObjectPtr->getName());

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

			//Input channels (has none)
			// compositorNodeDefinition->addTextureSourceName("rt0", 0, Ogre::TextureDefinitionBase::TEXTURE_INPUT);
			// compositorNodeDefinition->addTextureSourceName("rt1", 1, Ogre::TextureDefinitionBase::TEXTURE_INPUT);
				/*{
					Ogre::CompositorPassQuadDef* passQuad;
					passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));
					passQuad->mMaterialName = "Postprocess/Glass";
					passQuad->addQuadTextureSource(0, "rt_input");
				}*/


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
			texDef->msaa = this->msaaLevel;
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
			texDef->msaa = this->msaaLevel;
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

						passScene->mShadowNode = "NOWAShadowNode";

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

						if (true == this->useReflection->getBool())
						{
							//Our materials in this pass will be using this cubemap,
							//so we need to expose it to the pass.
							//Note: Even if it "just works" without exposing, the reason for
							//exposing is to ensure compatibility with Vulkan & D3D12.

							passScene->mExposedTextures.emplace_back(Ogre::IdString("rt1"));
						}

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

					// Render Scene
					{
						Ogre::CompositorPassSceneDef* passScene;
						passScene = static_cast<Ogre::CompositorPassSceneDef*>(targetDef->addPass(Ogre::PASS_SCENE));
						passScene->mIncludeOverlays = false;
						passScene->mShadowNode = "NOWAShadowNode";
						passScene->mShadowNodeRecalculation = Ogre::ShadowNodeRecalculation::SHADOW_NODE_REUSE;
						passScene->mFirstRQ = 2;
					}
				}

				//targetDef = compositorNodeDefinition->addTargetPass("rt1");

				//{
				//	//Play nice with Multi-GPU setups by telling the API there is no inter-frame dependency.
				//	//On APIs that support discard_only (e.g. DX11) the driver may just ignore the colour value,
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
								passScene->mShadowNode = "NOWAShadowNode";
							}

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

							// Render Scene
							{
								Ogre::CompositorPassSceneDef* passScene;
								passScene = static_cast<Ogre::CompositorPassSceneDef*>(targetDef->addPass(Ogre::PASS_SCENE));
								passScene->mIncludeOverlays = false;
								passScene->mShadowNode = "NOWAShadowNode";
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

		this->materialBackgroundFarPtr = Ogre::MaterialManager::getSingletonPtr()->getByName("NOWABackgroundPostprocess");
		if (true == this->materialBackgroundFarPtr.isNull())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[WorkspaceBackgroundComponent] Could not set: " + this->workspaceName + " because the material: 'NOWABackgroundPostprocess' does not exist!");
			throw Ogre::Exception(Ogre::Exception::ERR_ITEM_NOT_FOUND, "Could not create: " + this->workspaceName + " because the material: 'NOWABackgroundPostprocess' does not exist!", "NOWA");
		}

		this->materialBackgroundMiddlePtr = Ogre::MaterialManager::getSingletonPtr()->getByName("NOWABackgroundPostprocess2");
		if (true == this->materialBackgroundMiddlePtr.isNull())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[WorkspaceBackgroundComponent] Could not set: " + this->workspaceName + " because the material: 'NOWABackgroundPostprocess2' does not exist!");
			throw Ogre::Exception(Ogre::Exception::ERR_ITEM_NOT_FOUND, "Could not create: " + this->workspaceName + " because the material: 'NOWABackgroundPostprocess2' does not exist!", "NOWA");
		}

		this->materialBackgroundNearPtr = Ogre::MaterialManager::getSingletonPtr()->getByName("NOWABackgroundPostprocess3");
		if (true == this->materialBackgroundNearPtr.isNull())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[WorkspaceBackgroundComponent] Could not set: " + this->workspaceName + " because the material: 'NOWABackgroundPostprocess3' does not exist!");
			throw Ogre::Exception(Ogre::Exception::ERR_ITEM_NOT_FOUND, "Could not create: " + this->workspaceName + " because the material: 'NOWABackgroundPostprocess3' does not exist!", "NOWA");
		}

		Ogre::Material* material = materialBackgroundFarPtr.getPointer();
		this->passBackgroundFar = material->getTechnique(0)->getPass(0);

		material = materialBackgroundMiddlePtr.getPointer();
		this->passBackgroundMiddle = material->getTechnique(0)->getPass(0);

		material = materialBackgroundNearPtr.getPointer();
		this->passBackgroundNear = material->getTechnique(0)->getPass(0);

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

		this->workspace = WorkspaceModules::getInstance()->getCompositorManager()->addWorkspace(sceneManager, Core::getSingletonPtr()->getOgreRenderWindow()->getTexture(), camera, workspaceName, true);

		this->initializeSmaa(WorkspaceModule::SMAA_PRESET_ULTRA, WorkspaceModule::EdgeDetectionColour);
#endif

		this->materialBackgroundFarPtr = Ogre::MaterialManager::getSingletonPtr()->getByName("NOWABackgroundPostprocess");
		if (true == this->materialBackgroundFarPtr.isNull())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[WorkspaceBackgroundComponent] Could not set: " + this->workspaceName + " because the material: 'NOWABackgroundPostprocess' does not exist!");
			throw Ogre::Exception(Ogre::Exception::ERR_ITEM_NOT_FOUND, "Could not create: " + this->workspaceName + " because the material: 'NOWABackgroundPostprocess' does not exist!", "NOWA");
		}

		this->materialBackgroundMiddlePtr = Ogre::MaterialManager::getSingletonPtr()->getByName("NOWABackgroundPostprocess2");
		if (true == this->materialBackgroundMiddlePtr.isNull())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[WorkspaceBackgroundComponent] Could not set: " + this->workspaceName + " because the material: 'NOWABackgroundPostprocess2' does not exist!");
			throw Ogre::Exception(Ogre::Exception::ERR_ITEM_NOT_FOUND, "Could not create: " + this->workspaceName + " because the material: 'NOWABackgroundPostprocess2' does not exist!", "NOWA");
		}

		this->materialBackgroundNearPtr = Ogre::MaterialManager::getSingletonPtr()->getByName("NOWABackgroundPostprocess3");
		if (true == this->materialBackgroundNearPtr.isNull())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[WorkspaceBackgroundComponent] Could not set: " + this->workspaceName + " because the material: 'NOWABackgroundPostprocess3' does not exist!");
			throw Ogre::Exception(Ogre::Exception::ERR_ITEM_NOT_FOUND, "Could not create: " + this->workspaceName + " because the material: 'NOWABackgroundPostprocess3' does not exist!", "NOWA");
		}

		Ogre::Material* material = materialBackgroundFarPtr.getPointer();
		this->passBackgroundFar = material->getTechnique(0)->getPass(0);

		material = materialBackgroundMiddlePtr.getPointer();
		this->passBackgroundMiddle = material->getTechnique(0)->getPass(0);

		material = materialBackgroundNearPtr.getPointer();
		this->passBackgroundNear = material->getTechnique(0)->getPass(0);

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

		// If prior ws component has some channels defined
		if (true == this->externalChannels.empty())
		{
			this->externalChannels.resize(1);
			this->externalChannels[0] = Core::getSingletonPtr()->getOgreRenderWindow()->getTexture();
		}

		this->workspace = WorkspaceModules::getInstance()->getCompositorManager()->addWorkspace(this->gameObjectPtr->getSceneManager(), externalChannels,
			this->cameraComponent->getCamera(), this->workspaceName, true, -1, (Ogre::UavBufferPackedVec*)0,
			&this->initialCubemapLayouts, &this->initialCubemapUavAccess);
		// this->initializeSmaa(WorkspaceModule::SMAA_PRESET_ULTRA, WorkspaceModule::EdgeDetectionColour);

		// this->initializeHdr(msaaLevel);

		WorkspaceModules::getInstance()->setPrimaryWorkspace(this->cameraComponent->getCamera(), this);

		if (true == this->useHdr->getBool())
		{
			this->initializeHdr(this->msaaLevel);
		}

		return nullptr != this->workspace;
	}

	void WorkspaceBackgroundComponent::removeWorkspace(void)
	{
		WorkspaceBaseComponent::removeWorkspace();

		if (false == this->materialBackgroundFarPtr.isNull())
			this->materialBackgroundFarPtr.setNull();
		if (false == this->materialBackgroundMiddlePtr.isNull())
			this->materialBackgroundMiddlePtr.setNull();
		if (false == this->materialBackgroundNearPtr.isNull())
			this->materialBackgroundNearPtr.setNull();

		this->passBackgroundFar = nullptr;
		this->passBackgroundMiddle = nullptr;
		this->passBackgroundNear = nullptr;
	}

	void WorkspaceBackgroundComponent::changeBackgroundFar(const Ogre::String& backgroundFarTextureName)
	{
		if (nullptr != this->passBackgroundFar)
		{
			// Change background texture
			Ogre::TextureUnitState* tex = this->passBackgroundFar->getTextureUnitState(0);
			tex->setTextureName(backgroundFarTextureName);

			// tex->setGamma(2.0);
			this->materialBackgroundFarPtr->compile();
		}
	}

	void WorkspaceBackgroundComponent::changeBackgroundMiddle(const Ogre::String& backgroundMiddleTextureName)
	{
		// Change background texture
		if (nullptr != this->passBackgroundMiddle)
		{
			Ogre::TextureUnitState* tex = this->passBackgroundMiddle->getTextureUnitState(0);
			tex->setTextureName(backgroundMiddleTextureName);

			// tex->setGamma(2.0);
			this->materialBackgroundMiddlePtr->compile();
		}
	}

	void WorkspaceBackgroundComponent::changeBackgroundNear(const Ogre::String& backgroundNearTextureName)
	{
		// Change background texture
		if (nullptr != this->passBackgroundNear)
		{
			Ogre::TextureUnitState* tex = this->passBackgroundNear->getTextureUnitState(0);
			tex->setTextureName(backgroundNearTextureName);

			// tex->setGamma(2.0);
			this->materialBackgroundNearPtr->compile();
		}
	}

	void WorkspaceBackgroundComponent::setBackgroundScrollFarSpeedX(Ogre::Real backgroundScrollFarSpeedX)
	{
		if (nullptr != this->passBackgroundFar)
		{
			this->passBackgroundFar->getFragmentProgramParameters()->setNamedConstant("speedX", backgroundScrollFarSpeedX);

			// tex->setGamma(2.0);
// Attention: Is that necessary?
			// material->compile();
		}
	}

	void WorkspaceBackgroundComponent::setBackgroundScrollFarSpeedY(Ogre::Real backgroundScrollFarSpeedY)
	{
		if (nullptr != this->passBackgroundFar)
		{
			this->passBackgroundFar->getFragmentProgramParameters()->setNamedConstant("speedY", backgroundScrollFarSpeedY);

			// tex->setGamma(2.0);
			// Attention: Is that necessary?
			// material->compile();
		}
	}

	void WorkspaceBackgroundComponent::setBackgroundScrollMiddleSpeedX(Ogre::Real backgroundScrollMiddleSpeedX)
	{
		if (nullptr != this->passBackgroundMiddle)
		{
			this->passBackgroundMiddle->getFragmentProgramParameters()->setNamedConstant("speedX", backgroundScrollMiddleSpeedX);

			// tex->setGamma(2.0);
			// Attention: Is that necessary?
			// material->compile();
		}
	}

	void WorkspaceBackgroundComponent::setBackgroundScrollMiddleSpeedY(Ogre::Real backgroundScrollMiddleSpeedY)
	{
		if (nullptr != this->passBackgroundMiddle)
		{
			this->passBackgroundMiddle->getFragmentProgramParameters()->setNamedConstant("speedY", backgroundScrollMiddleSpeedY);

			// tex->setGamma(2.0);
			// Attention: Is that necessary?
			// material->compile();
		}
	}

	void WorkspaceBackgroundComponent::setBackgroundScrollNearSpeedX(Ogre::Real backgroundScrollNearSpeedX)
	{
		if (nullptr != this->passBackgroundNear)
		{
			this->passBackgroundNear->getFragmentProgramParameters()->setNamedConstant("speedX", backgroundScrollNearSpeedX);

			// tex->setGamma(2.0);
			// Attention: Is that necessary?
			// material->compile();
		}
	}

	void WorkspaceBackgroundComponent::setBackgroundScrollNearSpeedY(Ogre::Real backgroundScrollNearSpeedY)
	{
		if (nullptr != this->passBackgroundNear)
		{
			this->passBackgroundNear->getFragmentProgramParameters()->setNamedConstant("speedY", backgroundScrollNearSpeedY);

			// tex->setGamma(2.0);
			// Attention: Is that necessary?
			// material->compile();
		}
	}

	void WorkspaceBackgroundComponent::compileBackgroundFarMaterial(void)
	{
		if (nullptr != this->passBackgroundFar)
		{
			this->materialBackgroundFarPtr->compile();
		}
	}

	void WorkspaceBackgroundComponent::compileBackgroundMiddleMaterial(void)
	{
		if (nullptr != this->passBackgroundMiddle)
		{
			this->materialBackgroundMiddlePtr->compile();
		}
	}

	void WorkspaceBackgroundComponent::compileBackgroundNearMaterial(void)
	{
		if (nullptr != this->passBackgroundNear)
		{
			this->materialBackgroundNearPtr->compile();
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	WorkspaceCustomComponent::WorkspaceCustomComponent()
		: WorkspaceBaseComponent(),
		customWorkspaceName(new Variant(WorkspaceCustomComponent::AttrCustomWorkspaceName(), Ogre::String(""), this->attributes))
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

		this->workspace = WorkspaceModules::getInstance()->getCompositorManager()->addWorkspace(sceneManager, Core::getSingletonPtr()->getOgreRenderWindow()->getTexture(), camera, workspaceName, true);

		this->initializeSmaa(WorkspaceModule::SMAA_PRESET_ULTRA, WorkspaceModule::EdgeDetectionColour);
#endif

		if (false == this->customWorkspaceName->getString().empty())
		{
			const Ogre::IdString workspaceNameId(this->customWorkspaceName->getString());

			if (false == WorkspaceModules::getInstance()->getCompositorManager()->hasWorkspaceDefinition(workspaceNameId))
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

			this->workspace = WorkspaceModules::getInstance()->getCompositorManager()->addWorkspace(this->gameObjectPtr->getSceneManager(), externalChannels,
				this->cameraComponent->getCamera(), this->customWorkspaceName->getString(), true, -1, (Ogre::UavBufferPackedVec*)0,
				&this->initialCubemapLayouts, &this->initialCubemapUavAccess);
	}

		WorkspaceModules::getInstance()->setPrimaryWorkspace(this->cameraComponent->getCamera(), this);

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