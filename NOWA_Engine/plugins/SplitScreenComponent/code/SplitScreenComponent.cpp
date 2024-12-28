#include "NOWAPrecompiled.h"
#include "SplitScreenComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"
#include "modules/WorkspaceModule.h"

#include "gameobject/CameraComponent.h"
#include "gameobject/WorkspaceComponents.h"

#include "OgreAbiUtils.h"

#include "Compositor/Pass/PassQuad/OgreCompositorPassQuadDef.h"
#include "Compositor/Pass/PassMipmap/OgreCompositorPassMipmapDef.h"
#include "Compositor/Pass/PassScene/OgreCompositorPassScene.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	SplitScreenComponent::SplitScreenComponent()
		: GameObjectComponent(),
		name("SplitScreenComponent"),
		splitScreenTexture(nullptr),
		textureManager(nullptr),
		cameraComponent(nullptr),
		workspaceBaseComponent(nullptr),
		componentBeingLoaded(false),
		tempCamera(nullptr),
		finalCombinedWorkspace(nullptr),
		activated(new Variant(SplitScreenComponent::AttrActivated(), true, this->attributes)),
		textureSize(new Variant(SplitScreenComponent::AttrTextureSize(), Ogre::Vector2(640.0f, 480.0f), this->attributes)),
		geometry(new Variant(SplitScreenComponent::AttrGeometry(), Ogre::Vector4(0.5f, 0.0f, 0.5f, 1.0f), this->attributes))
	{
		this->activated->setDescription("If activated the scene from a prior workspace component will be rendered into a texture.");
		this->textureSize->setDescription("Sets the split screen texture size in pixels. Note: The texture is quadratic. Also note: The higher the texture size, the less performant the application will run.");
		this->geometry->setDescription("Sets the geometry of the split screen relative to the window screen in the format Vector4(pos.x, pos.y, width, height).\n"
			"Example: 2 player vertical split: geometry1 0.5 0 0.5 1 geometry2 0 0 0.5 1\n"
			"Example: 2 player horizonal split: geometry1 0 0.5 1 0.5 geometry2 0 0 1 0.5\n"
			"Example: 3 player vertical split: geometry1 0 0 0.3333 1 geometry2 0.3333 0 0.3333 1 geometry3 0.6666 0 0.3333 1\n"
			"Example: 4 player vertical/horizontal split: geometry1 0 0.5 0.5 0.5 geometry2 0.5 0.5 0.5 0.5 geometry3 0 0 0.5 0.5  geometry4 0.5 0 0.5 0.5\n"
		);
	}

	SplitScreenComponent::~SplitScreenComponent(void)
	{

	}

	const Ogre::String& SplitScreenComponent::getName() const
	{
		return this->name;
	}

	void SplitScreenComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<SplitScreenComponent>(SplitScreenComponent::getStaticClassId(), SplitScreenComponent::getStaticClassName());
	}

	void SplitScreenComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool SplitScreenComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		// Priority connect!
		this->bConnectPriority = true;

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TextureSize")
		{
			this->textureSize->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Geometry")
		{
			this->geometry->setValue(XMLConverter::getAttribVector4(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		this->componentBeingLoaded = true;

		return true;
	}

	GameObjectCompPtr SplitScreenComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		return SplitScreenCompPtr();
	}

	bool SplitScreenComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[SplitScreenComponent] Init component for game object: " + this->gameObjectPtr->getName());

		if (false == this->componentBeingLoaded)
		{
			Ogre::Real windowWidth = Core::getSingletonPtr()->getOgreRenderWindow()->getWidth()/* * 0.5f*/;
			Ogre::Real windowHeight = Core::getSingletonPtr()->getOgreRenderWindow()->getHeight() /** 1.0f*/;

			this->textureSize->setValue(Ogre::Vector2(windowWidth, windowHeight));
		}

		this->componentBeingLoaded = false;

		this->textureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();

		return true;
	}

	bool SplitScreenComponent::connect(void)
	{
		this->setActivated(this->activated->getBool());
		return true;
	}

	bool SplitScreenComponent::disconnect(void)
	{
		this->cleanupSplitScreen();
		return true;
	}

	bool SplitScreenComponent::onCloned(void)
	{

		return true;
	}

	void SplitScreenComponent::onRemoveComponent(void)
	{
		this->cleanupSplitScreen();
	}

	void SplitScreenComponent::onOtherComponentRemoved(unsigned int index)
	{

	}

	void SplitScreenComponent::onOtherComponentAdded(unsigned int index)
	{

	}

	void SplitScreenComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			// Do something
		}
	}

	void SplitScreenComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (SplitScreenComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (SplitScreenComponent::AttrTextureSize() == attribute->getName())
		{
			this->setTextureSize(attribute->getVector2());
		}
		else if (SplitScreenComponent::AttrGeometry() == attribute->getName())
		{
			this->setGeometry(attribute->getVector4());
		}
	}

	void SplitScreenComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TextureSize"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->textureSize->getVector2())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Geometry"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->geometry->getVector4())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String SplitScreenComponent::getClassName(void) const
	{
		return "SplitScreenComponent";
	}

	Ogre::String SplitScreenComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void SplitScreenComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);

		if (true == activated)
		{
			this->setupSplitScreen();
		}
		else
		{
			this->cleanupSplitScreen();
		}
	}

	bool SplitScreenComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void SplitScreenComponent::setTextureSize(const Ogre::Vector2& textureSize)
	{
		this->textureSize->setValue(textureSize);
	}

	Ogre::Vector2 SplitScreenComponent::getTextureSize(void) const
	{
		return this->textureSize->getVector2();
	}

	void SplitScreenComponent::setGeometry(const Ogre::Vector4& geometry)
	{
		this->geometry->setValue(geometry);
	}

	Ogre::Vector4 SplitScreenComponent::getGeometry(void) const
	{
		return this->geometry->getVector4();
	}

	Ogre::TextureGpu* SplitScreenComponent::getSplitScreenTexture(void) const
	{
		return this->splitScreenTexture;
	}

	Ogre::TextureGpu* SplitScreenComponent::createSplitScreenTexture(const Ogre::String& name)
	{
		Ogre::TextureGpu* texture = nullptr;
		if (false == this->textureManager->hasTextureResource(name, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME))
		{
			Ogre::TextureGpu* manualTexture = this->textureManager->createTexture("ManualTexture_" + name, GpuPageOutStrategy::SaveToSystemRam, TextureFlags::ManualTexture, TextureTypes::Type2D);

			int windowWidth = Core::getSingletonPtr()->getOgreRenderWindow()->getWidth()/* * this->geometry->getVector4().z*/;
			int windowHeight = Core::getSingletonPtr()->getOgreRenderWindow()->getHeight() /** this->geometry->getVector4().w*/;

			manualTexture->setResolution(windowWidth, windowHeight);
			manualTexture->scheduleTransitionTo(GpuResidency::OnStorage);
			manualTexture->setNumMipmaps(1);

			manualTexture->setPixelFormat(PFG_RGBA8_UNORM_SRGB);
			manualTexture->scheduleTransitionTo(GpuResidency::Resident);

			texture = this->textureManager->createTexture(name, GpuPageOutStrategy::Discard, TextureFlags::RenderToTexture, TextureTypes::Type2D);
			texture->copyParametersFrom(manualTexture);
			texture->scheduleTransitionTo(GpuResidency::Resident);
			texture->_setDepthBufferDefaults(DepthBuffer::POOL_DEFAULT, false, PFG_D32_FLOAT);

			return texture;
		}
		else
		{
			texture = this->textureManager->findTextureNoThrow(name);
		}
		return texture;
	}

	void SplitScreenComponent::setupSplitScreen(void)
	{
		const auto& cameraCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<CameraComponent>());
		if (nullptr == cameraCompPtr || this->gameObjectPtr->getId() == GameObjectController::MAIN_CAMERA_ID)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[SplitScreenComponent] Error setting up split screen workspace, because the game object: " + this->gameObjectPtr->getName() + " is the main camera. Choose a different camera!");
			return;
		}

		this->cameraComponent = cameraCompPtr.get();

		const auto& workspaceBaseCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<WorkspaceBaseComponent>());
		if (nullptr == workspaceBaseCompPtr || this->gameObjectPtr->getId() == GameObjectController::MAIN_CAMERA_ID)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[SplitScreenComponent] Error setting up split screen workspace, because the game object: " + this->gameObjectPtr->getName() + " is the main camera. Choose a different camera!");
			return;
		}

		auto splitScreenComponents = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectComponents<SplitScreenComponent>();

		// Only use activated components
		std::vector<boost::shared_ptr<SplitScreenComponent>> tempSplitScreenComponents;
		for (size_t i = 0; i < splitScreenComponents.size(); i++)
		{
			if (true == splitScreenComponents[i]->isActivated())
			{
				tempSplitScreenComponents.emplace_back(splitScreenComponents[i]);
			}
		}

		if (tempSplitScreenComponents.size() > 4)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[SplitScreenComponent] Error setting up split screen workspace, there are more than 4 cameras. Only 4 cameras are supported!");
			return;
		}

		WorkspaceModule::getInstance()->setSplitScreenScenarioActive(true);
		this->cameraComponent->applySplitScreen(true, tempSplitScreenComponents.size());
		
		this->workspaceBaseComponent = workspaceBaseCompPtr.get();

		this->splitScreenTexture = this->createSplitScreenTexture("SplitScreenTexture_" + this->gameObjectPtr->getName());

		this->externalChannels.resize(1);
		this->externalChannels[0] = this->splitScreenTexture;

		// Set the this external channels as custom external channels to create custom workspace
		this->workspaceBaseComponent->setCustomExternalChannels(this->externalChannels);
		this->workspaceBaseComponent->setInvolvedInSplitScreen(true);
		this->workspaceBaseComponent->createWorkspace();

		this->externalChannels.clear();
		this->workspaceBaseComponent->setCustomExternalChannels(this->externalChannels);

		Ogre::CompositorManager2::CompositorNodeDefMap nodeDefs = WorkspaceModule::getInstance()->getCompositorManager()->getNodeDefinitions();

		// Iterate through Compositor Managers resources
		auto& it = nodeDefs.begin();
		auto& end = nodeDefs.end();

		// Goes through all passes for the given workspace and set the corresponding render category. All game objects which do not match that category, will not be rendered for this camera
		// Note: MyGui is added to the final split combined workspace, so it does not make sense to exclude mygui objects from rendering
		while (it != end)
		{
			if (it->second->getNameStr() == this->workspaceBaseComponent->getRenderingNodeName() ||
				it->second->getNameStr() == this->workspaceBaseComponent->getFinalRenderingNodeName())
			{
				for (size_t i = 0; i < it->second->getNumTargetPasses(); i++)
				{
					for (size_t j = 0; j < it->second->getTargetPass(i)->getCompositorPasses().size(); j++)
					{
						const auto& pass = it->second->getTargetPass(i)->getCompositorPasses()[j];
						if (pass->getType() == Ogre::PASS_SCENE)
						{
							Ogre::CompositorPassSceneDef* passScene = static_cast<Ogre::CompositorPassSceneDef*>(pass);
							unsigned int finalRenderMask = AppStateManager::getSingletonPtr()->getGameObjectController()->generateRenderCategoryId(this->cameraComponent->getExcludeRenderCategories());
							passScene->setVisibilityMask(finalRenderMask);
						}
					}
				}
			}

			++it;
		}

		bool isLastComponent = false;
		
		if (tempSplitScreenComponents.size() > 0)
		{
			isLastComponent = this == tempSplitScreenComponents[tempSplitScreenComponents.size() - 1].get();
		}

		if (true == isLastComponent)
		{
			std::vector<Ogre::String> textureNames;
			std::vector<Ogre::Vector4> geometryVectors;

			for (size_t i = 0; i < tempSplitScreenComponents.size(); i++)
			{
				textureNames.emplace_back(tempSplitScreenComponents[i]->getSplitScreenTexture()->getNameStr());
				geometryVectors.emplace_back(tempSplitScreenComponents[i]->getGeometry());
			}

			Ogre::String materialName = "DynamicSplitMaterial";
			auto splitMaterial = Ogre::MaterialManager::getSingletonPtr()->getByName(materialName);
			Ogre::GpuProgramParametersSharedPtr fragmentParams = splitMaterial->getTechnique(0)->getPass(0)->getFragmentProgramParameters();
			for (size_t i = 0; i < geometryVectors.size(); ++i)
			{
				Ogre::String paramName = "geomData" + Ogre::StringConverter::toString(i + 1); // Match shader uniform
				if (fragmentParams->_findNamedConstantDefinition(paramName, false))
				{
					fragmentParams->setNamedConstant(paramName, geometryVectors[i]);
				}
				else
				{
					Ogre::LogManager::getSingleton().logMessage("[SplitScreenComponent]: Shader uniform not found: " + paramName, Ogre::LML_CRITICAL);
				}
			}

			Ogre::CompositorManager2* compositorManager = WorkspaceModule::getInstance()->getCompositorManager();

			Ogre::String finalRenderingNodeName = "FinalSplitScreenCombineNode_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
			Ogre::CompositorNodeDef* finalNodeDef = compositorManager->addNodeDefinition(finalRenderingNodeName);

			// Add render window as input
			finalNodeDef->addTextureSourceName("rt_renderwindow", 0, Ogre::TextureDefinitionBase::TEXTURE_INPUT);

			// Add split textures as inputs
			for (size_t i = 0; i < tempSplitScreenComponents.size(); i++)
			{
				finalNodeDef->addTextureSourceName(tempSplitScreenComponents[i]->getSplitScreenTexture()->getNameStr(), i + 1, Ogre::TextureDefinitionBase::TEXTURE_INPUT);
			}

			finalNodeDef->setNumTargetPass(1);
			Ogre::CompositorTargetDef* targetDef = finalNodeDef->addTargetPass("rt_renderwindow");

			targetDef->setNumPasses(2);

			// Quad pass to combine split textures
			{
				auto* passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));
				passQuad->setAllLoadActions(Ogre::LoadAction::DontCare);
				passQuad->mMaterialName = materialName;
				// passQuad->mMaterialName = "SplitScreen_2h";
				passQuad->mProfilingId = "QuadPass_DynamicSplitMaterial";

				for (size_t i = 0; i < tempSplitScreenComponents.size(); i++)
				{
					passQuad->addQuadTextureSource(i, tempSplitScreenComponents[i]->getSplitScreenTexture()->getNameStr());
				}
			}

			// MyGUI pass
			{
				auto pass = targetDef->addPass(Ogre::PASS_CUSTOM, "MYGUI");
				pass->mProfilingId = "Split_MyGUI_Pass_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
			}

			Ogre::String finalWorkspaceName = "finalCombinedSplitScreenWorkspace_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
			Ogre::CompositorWorkspaceDef* workspaceDef = compositorManager->addWorkspaceDefinition(finalWorkspaceName);

			workspaceDef->connectExternal(0, finalRenderingNodeName, 0);

			// Connect all split textures
			for (size_t i = 0; i < tempSplitScreenComponents.size(); i++)
			{
				workspaceDef->connectExternal(i + 1, finalRenderingNodeName, i + 1);
			}

			Ogre::CompositorChannelVec finalExternalChannels;

			// Add render window as the first channel
			finalExternalChannels.push_back(Core::getSingletonPtr()->getOgreRenderWindow()->getTexture());

			// Add split RTTs
			for (size_t i = 0; i < tempSplitScreenComponents.size(); i++)
			{
				finalExternalChannels.push_back(tempSplitScreenComponents[i]->getSplitScreenTexture());
			}

			// Is not used, just a dummy, also the finalRenderingNodeName is just a node without a scene, which just combines the textures in a shader
			this->tempCamera = this->gameObjectPtr->getSceneManager()->createCamera("FinalSplitScreenDummyCamera_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()));
			NOWA::Core::getSingletonPtr()->setMenuSettingsForCamera(tempCamera);
			this->tempCamera->setFOVy(Ogre::Degree(90.0f));
			this->tempCamera->setNearClipDistance(0.1f);
			this->tempCamera->setFarClipDistance(500.0f);
			this->tempCamera->setQueryFlags(0 << 0);
			this->tempCamera->setPosition(this->tempCamera->getParentSceneNode()->convertLocalToWorldPositionUpdated(Ogre::Vector3(0.0f, 1.0f, -2.0f)));

			AppStateManager::getSingletonPtr()->getCameraManager()->addCamera(this->tempCamera, false);

			this->finalCombinedWorkspace = compositorManager->addWorkspace(this->gameObjectPtr->getSceneManager(), finalExternalChannels, this->tempCamera, finalWorkspaceName, true, -1);

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[WorkspaceSplitComponent] Creating final combined workspace: " + finalWorkspaceName);

			WorkspaceModule::getInstance()->setPrimaryWorkspace2(this->gameObjectPtr->getSceneManager(), this->tempCamera, this->finalCombinedWorkspace);
		}
	}

	void SplitScreenComponent::cleanupSplitScreen(void)
	{
		WorkspaceModule::getInstance()->setSplitScreenScenarioActive(false);

		Ogre::CompositorManager2* compositorManager = WorkspaceModule::getInstance()->getCompositorManager();

		if (nullptr != this->tempCamera)
		{
			AppStateManager::getSingletonPtr()->getCameraManager()->removeCamera(this->tempCamera);

			if (nullptr != this->finalCombinedWorkspace)
			{
				WorkspaceModule::getInstance()->removeWorkspace(this->gameObjectPtr->getSceneManager(), this->tempCamera);

				Ogre::String finalRenderingNodeName = "FinalSplitScreenCombineNode_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
				if (true == compositorManager->hasNodeDefinition(finalRenderingNodeName))
				{
					compositorManager->removeNodeDefinition(finalRenderingNodeName);
				}

				Ogre::String finalWorkspaceName = "finalCombinedSplitScreenWorkspace_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
				if (true == compositorManager->hasWorkspaceDefinition(finalWorkspaceName))
				{
					compositorManager->removeWorkspaceDefinition(finalWorkspaceName);
				}

				WorkspaceModule::getInstance()->getPrimaryCameraComponent()->setActivated(true);

				this->finalCombinedWorkspace = nullptr;
			}

			this->gameObjectPtr->getSceneManager()->destroyCamera(this->tempCamera);
			this->tempCamera = nullptr;
		}

		if (nullptr != this->cameraComponent)
		{
			this->cameraComponent->applySplitScreen(false, -1);
		}

		this->cameraComponent = nullptr;

		if (nullptr != this->workspaceBaseComponent)
		{
			this->workspaceBaseComponent->setInvolvedInSplitScreen(false);
			this->workspaceBaseComponent->removeWorkspace();
			this->workspaceBaseComponent = nullptr;
		}
		this->externalChannels.clear();

		// WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent()->createWorkspace();
	}

	// Lua registration part

	SplitScreenComponent* getSplitScreenComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<SplitScreenComponent>(gameObject->getComponentWithOccurrence<SplitScreenComponent>(occurrenceIndex)).get();
	}

	SplitScreenComponent* getSplitScreenComponent(GameObject* gameObject)
	{
		return makeStrongPtr<SplitScreenComponent>(gameObject->getComponent<SplitScreenComponent>()).get();
	}

	SplitScreenComponent* getSplitScreenComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<SplitScreenComponent>(gameObject->getComponentFromName<SplitScreenComponent>(name)).get();
	}

	void SplitScreenComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
			[
				class_<SplitScreenComponent, GameObjectComponent>("SplitScreenComponent")
					.def("setActivated", &SplitScreenComponent::setActivated)
					.def("isActivated", &SplitScreenComponent::isActivated)
			];

		LuaScriptApi::getInstance()->addClassToCollection("SplitScreenComponent", "class inherits GameObjectComponent", SplitScreenComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("SplitScreenComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
		LuaScriptApi::getInstance()->addClassToCollection("SplitScreenComponent", "bool isActivated()", "Gets whether this component is activated.");

		gameObjectClass.def("getSplitScreenComponentFromName", &getSplitScreenComponentFromName);
		gameObjectClass.def("getSplitScreenComponent", (SplitScreenComponent * (*)(GameObject*)) & getSplitScreenComponent);
		// If its desired to create several of this components for one game object
		gameObjectClass.def("getSplitScreenComponentFromIndex", (SplitScreenComponent * (*)(GameObject*, unsigned int)) & getSplitScreenComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "SplitScreenComponent getSplitScreenComponentFromIndex(unsigned int occurrenceIndex)", "Gets the component by the given occurence index, since a game object may this component maybe several times.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "SplitScreenComponent getSplitScreenComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "SplitScreenComponent getSplitScreenComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castSplitScreenComponent", &GameObjectController::cast<SplitScreenComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "SplitScreenComponent castSplitScreenComponent(SplitScreenComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool SplitScreenComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// Can only be added once
		auto splitScreenCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<SplitScreenComponent>());
		if (nullptr != splitScreenCompPtr)
		{
			return false;
		}

		auto cameraCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<CameraComponent>());
		if (nullptr == cameraCompPtr)
		{
			return false;
		}
		else
		{
			if (cameraCompPtr->getOwner()->getId() == GameObjectController::MAIN_CAMERA_ID)
			{
				return false;
			}
		}

		return true;
	}

}; //namespace end