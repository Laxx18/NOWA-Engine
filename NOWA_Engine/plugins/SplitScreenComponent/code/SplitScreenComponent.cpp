#include "NOWAPrecompiled.h"
#include "SplitScreenComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "gameobject/GameObjectFactory.h"
#include "modules/WorkspaceModule.h"

#include "gameobject/CameraComponent.h"
#include "gameobject/WorkspaceComponents.h"
#include "gameobject/CameraBehaviorComponents.h"

#include "OgreAbiUtils.h"
#include "OgreWindow.h"
#include "OgreDepthBuffer.h"
#include "Compositor/Pass/PassQuad/OgreCompositorPassQuadDef.h"
#include "Compositor/Pass/PassMipmap/OgreCompositorPassMipmapDef.h"
#include "Compositor/Pass/PassScene/OgreCompositorPassScene.h"
#include "Compositor/OgreCompositorNode.h"
#include "Compositor/OgreCompositorNodeDef.h"

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
		geometry(new Variant(SplitScreenComponent::AttrGeometry(), Ogre::Vector4(0.5f, 0.0f, 0.5f, 1.0f), this->attributes)),
		cameraBehaviorGameObjectId(new Variant(SplitScreenComponent::AttrCameraBehaviorGameObjectId(), static_cast<unsigned long>(0), this->attributes, true)),
        splitPreset(new Variant(SplitScreenComponent::AttrApplyPreset(), std::vector<Ogre::String>{"Custom", "2-Vertical", "2-Horizontal", "3-Vertical", "3-Horizontal", "4-Grid"}, this->attributes))
	{
        this->activated->setDescription("If activated, this camera's view is rendered into its own split screen texture and combined with all other activated split screen cameras into the final image. "
                                        "The split screen scenario starts as soon as the first activated SplitScreenComponent connects (simulation start) and ends once the last one disconnects, at which point the main camera becomes active again.");

        this->textureSize->setDescription("Sets the split screen texture size in pixels. Note: The texture is quadratic. Also note: The higher the texture size, the less performant the application will run.\n"
                                          "Hint: This does not have to match the tile's actual on-screen size (see Geometry) - a smaller texture than the tile saves performance at the cost of a slightly blurrier tile, a larger one costs more "
                                          "performance for extra sharpness.");

        this->geometry->setDescription("Sets the geometry of THIS camera's tile relative to the window, in the format Vector4(pos.x, pos.y, width, height), each component in the range 0..1.\n"
                                       "Example: 2 player vertical split: geometry1 0.5 0 0.5 1 geometry2 0 0 0.5 1\n"
                                       "Example: 2 player horizonal split: geometry1 0 0.5 1 0.5 geometry2 0 0 1 0.5\n"
                                       "Example: 3 player vertical split: geometry1 0 0 0.3333 1 geometry2 0.3333 0 0.3333 1 geometry3 0.6666 0 0.3333 1\n"
                                       "Example: 4 player vertical/horizontal split: geometry1 0 0.5 0.5 0.5 geometry2 0.5 0.5 0.5 0.5 geometry3 0 0 0.5 0.5  geometry4 0.5 0 0.5 0.5\n"
                                       "Note: All values are FRACTIONS of the window size, not pixels, so this stays correct at any monitor resolution or window size. See also SplitPreset below to avoid computing these values by hand.\n");

        this->cameraBehaviorGameObjectId->setDescription("Sets the game object id whose CameraBehaviorComponent (FirstPersonCamera, ThirdPersonCamera etc.) shall drive this split screen camera. "
                                                         "This is OPTIONAL: leave it at 0 for a statically placed split screen camera that uses no behavior at all.");

        this->splitPreset->setDescription("Optional convenience preset to avoid computing the Geometry fractions by hand. Applied IMMEDIATELY when changed (no extra button). "
                                          "Select the SAME preset on every split screen camera that shall participate - selecting it on any ONE of them recomputes the Geometry of ALL currently activated split screen cameras in the scene at once. "
                                          "'Custom' does nothing (default, keeps whatever Geometry is currently set). '2-Vertical'/'2-Horizontal' need exactly 2 currently activated split screen cameras, '3-Vertical'/'3-Horizontal' need exactly 3, "
                                          "'4-Grid' needs exactly 4. "
                                          "If the selected preset does not match the actual number of currently activated split screen cameras, nothing is changed and a message is logged. "
                                          "Which tile a camera gets within the preset depends on the order in which its game object was created (first created camera -> first/top-left tile). "
                                          "This selection itself is NOT saved to the scene file - only the resulting Geometry value is (exactly like editing Geometry by hand).");
        this->splitPreset->addUserData(GameObject::AttrActionNoUndo());
        this->splitPreset->addUserData(GameObject::AttrActionNeedRefresh());
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

	bool SplitScreenComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

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
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CameraBehaviorGameObjectId")
		{
			this->cameraBehaviorGameObjectId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
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
			Ogre::Real windowWidth = Core::getSingletonPtr()->getOgreRenderWindow()->getWidth() * 0.5f;
			Ogre::Real windowHeight = Core::getSingletonPtr()->getOgreRenderWindow()->getHeight() /** 1.0f*/;

			this->textureSize->setValue(Ogre::Vector2(windowWidth, windowHeight));
		}

		this->componentBeingLoaded = false;

		this->textureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();

		return true;
	}

	bool SplitScreenComponent::connect(void)
	{
		GameObjectComponent::connect();
		this->setActivated(this->activated->getBool());
		return true;
	}

	bool SplitScreenComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();
		this->cleanupSplitScreen();
		return true;
	}

	bool SplitScreenComponent::onCloned(void)
	{

		return true;
	}

	void SplitScreenComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

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
		else if (SplitScreenComponent::AttrCameraBehaviorGameObjectId() == attribute->getName())
		{
			this->setCameraBehaviorGameObjectId(attribute->getULong());
		}
        else if (SplitScreenComponent::AttrApplyPreset() == attribute->getName())
        {
            this->splitPreset->setListSelectedValue(attribute->getListSelectedValue());
            this->applyPreset();
        }
	}

	void SplitScreenComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CameraBehaviorGameObjectId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->cameraBehaviorGameObjectId->getULong())));
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

		if (false == this->bConnected)
		{
			return;
		}

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

        // Keeps CameraManager's screen-space lookup rectangle for this split screen camera in sync, so
        // mouse-based raycasts (CameraManager::getCameraForScreenPosition) immediately use the updated
        // tile. Relevant when SplitPreset recomputes Geometry while already inside the split screen
        // simulation. Harmless no-op registration if the camera has not been created yet at design time
        // (cameraComponent is nullptr then, guarded below); the registration also happens once for real in
        // setupSplitScreen at simulation start.
        if (nullptr != this->cameraComponent)
        {
            AppStateManager::getSingletonPtr()->getCameraManager()->registerSplitScreenCamera(this->cameraComponent->getCamera(), geometry);
        }
    }

	Ogre::Vector4 SplitScreenComponent::getGeometry(void) const
	{
		return this->geometry->getVector4();
	}

	void SplitScreenComponent::setCameraBehaviorGameObjectId(const unsigned long cameraBehaviorGameObjectId)
	{
		this->cameraBehaviorGameObjectId->setValue(cameraBehaviorGameObjectId);
	}

	unsigned long SplitScreenComponent::getCameraBehaviorGameObjectId(void) const
	{
		return this->cameraBehaviorGameObjectId->getULong();
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
            Ogre::TextureGpu* manualTexture = this->textureManager->createTexture("ManualTexture_" + name, Ogre::GpuPageOutStrategy::SaveToSystemRam, Ogre::TextureFlags::ManualTexture, Ogre::TextureTypes::Type2D);

			int windowWidth = Core::getSingletonPtr()->getOgreRenderWindow()->getWidth()/* * this->geometry->getVector4().z*/;
			int windowHeight = Core::getSingletonPtr()->getOgreRenderWindow()->getHeight() /** this->geometry->getVector4().w*/;

			manualTexture->setResolution(windowWidth, windowHeight);
			manualTexture->scheduleTransitionTo(Ogre::GpuResidency::OnStorage);
			manualTexture->setNumMipmaps(1);

			manualTexture->setPixelFormat(Ogre::PFG_RGBA8_UNORM_SRGB);
            manualTexture->scheduleTransitionTo(Ogre::GpuResidency::Resident);

			texture = this->textureManager->createTexture(name, Ogre::GpuPageOutStrategy::Discard, Ogre::TextureFlags::RenderToTexture, Ogre::TextureTypes::Type2D);
			texture->copyParametersFrom(manualTexture);
            texture->scheduleTransitionTo(Ogre::GpuResidency::Resident);
            texture->_setDepthBufferDefaults(Ogre::DepthBuffer::POOL_DEFAULT, false, Ogre::PFG_D32_FLOAT);

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
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                "[SplitScreenComponent] Error setting up split screen workspace, because the game object: " + this->gameObjectPtr->getName() + " is the main camera. Choose a different camera!");
            return;
        }

        this->cameraComponent = cameraCompPtr.get();

        const auto& workspaceBaseCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<WorkspaceBaseComponent>());
        if (nullptr == workspaceBaseCompPtr || this->gameObjectPtr->getId() == GameObjectController::MAIN_CAMERA_ID)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                "[SplitScreenComponent] Error setting up split screen workspace, because the game object: " + this->gameObjectPtr->getName() + " is the main camera. Choose a different camera!");
            return;
        }

        GraphicsModule::RenderCommand renderCommand = [this, workspaceBaseCompPtr]()
        {
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

            // Determines the index of THIS split screen camera within all activated split screen components.
            // This index is the eye id: it identifies which split screen camera this one is (0 = first split
            // camera, 1 = second one etc.). It is deliberately NOT the component index inside the game object,
            // because that one would be 0 for every camera, since each camera game object owns exactly one
            // SplitScreenComponent.
            int splitScreenIndex = -1;
            for (size_t i = 0; i < tempSplitScreenComponents.size(); i++)
            {
                if (this == tempSplitScreenComponents[i].get())
                {
                    splitScreenIndex = static_cast<int>(i);
                    break;
                }
            }

            if (-1 == splitScreenIndex)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                    "[SplitScreenComponent] Error setting up split screen workspace for game object: " + this->gameObjectPtr->getName() + ", because this component could not be found in the list of activated split screen components.");
                return;
            }

            WorkspaceModule::getInstance()->setSplitScreenScenarioActive(true);
            this->cameraComponent->applySplitScreen(true, splitScreenIndex);

            this->workspaceBaseComponent = workspaceBaseCompPtr.get();

            this->splitScreenTexture = this->createSplitScreenTexture("SplitScreenTexture_" + this->gameObjectPtr->getName());

            this->externalChannels.resize(1);
            this->externalChannels[0] = this->splitScreenTexture;

            // Set the this external channels as custom external channels to create custom workspace
            this->workspaceBaseComponent->setCustomExternalChannels(this->externalChannels);
            this->workspaceBaseComponent->setInvolvedInSplitScreen(true);
            this->workspaceBaseComponent->createWorkspace();
            this->workspaceBaseComponent->connect();

            this->externalChannels.clear();
            this->workspaceBaseComponent->setCustomExternalChannels(this->externalChannels);

            Ogre::CompositorManager2::CompositorNodeDefMap nodeDefs = WorkspaceModule::getInstance()->getCompositorManager()->getNodeDefinitions();

            // Iterate through Compositor Managers resources
            auto it = nodeDefs.begin();
            auto end = nodeDefs.end();

            // Goes through all passes for the given workspace and set the corresponding render category. All game objects which do not match that category, will not be rendered for this camera
            // Note: MyGui is added to the final split combined workspace, so it does not make sense to exclude mygui objects from rendering
            while (it != end)
            {
                if (it->second->getNameStr() == this->workspaceBaseComponent->getRenderingNodeName() || it->second->getNameStr() == this->workspaceBaseComponent->getFinalRenderingNodeName())
                {
                    for (size_t i = 0; i < it->second->getNumTargetPasses(); i++)
                    {
                        for (size_t j = 0; j < it->second->getTargetPass(i)->getCompositorPasses().size(); j++)
                        {
                            const auto& pass = it->second->getTargetPass(i)->getCompositorPasses()[j];
                            if (pass->getType() == Ogre::PASS_SCENE)
                            {
                                Ogre::CompositorPassSceneDef* passScene = static_cast<Ogre::CompositorPassSceneDef*>(pass);
                                unsigned int finalRenderMask = AppStateManager::getSingletonPtr()->getGameObjectController()->generateRenderCategoryId(this->gameObjectPtr->getRenderCategory());
                                passScene->setVisibilityMask(finalRenderMask);
                            }
                        }
                    }
                }

                ++it;
            }

            // Registers this split screen camera's screen-space tile at the CameraManager, regardless of
            // whether it has an optional camera behavior. This is what makes mouse-based raycasts
            // (CameraManager::getCameraForScreenPosition) find the correct camera for every split screen
            // camera, including purely statically placed ones that never go through addCamera() below.
            AppStateManager::getSingletonPtr()->getCameraManager()->registerSplitScreenCamera(this->cameraComponent->getCamera(), this->getGeometry());

            // OPTIONAL camera behavior for THIS split screen camera.
            //
            // A split screen camera may be driven by a camera behavior (FirstPersonCamera,
            // ThirdPersonCamera etc.), which lives on another game object referenced by
            // cameraBehaviorGameObjectId. This is completely optional: if the id is 0 or the game object
            // does not exist or has no CameraBehaviorComponent, the camera is just a statically placed
            // split screen camera and nothing is registered at the camera manager at all.
            //
            // ATTENTION: this block must NOT live inside the "is last component" branch below (that was a
            // bug), because every split screen camera may have its own behavior, not only the last one.
            //
            // Note: forSplitScreen is passed as true, so that activating this camera does not deactivate
            // the other split screen cameras inside the camera manager.
            if (0 != this->cameraBehaviorGameObjectId->getULong())
            {
                GameObjectPtr cameraBehaviorGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->cameraBehaviorGameObjectId->getULong());
                if (nullptr != cameraBehaviorGameObjectPtr)
                {
                    const auto cameraBehaviorCompPtr = NOWA::makeStrongPtr(cameraBehaviorGameObjectPtr->getComponent<CameraBehaviorComponent>());
                    if (nullptr != cameraBehaviorCompPtr)
                    {
                        cameraBehaviorCompPtr->setActivated(true);
                        AppStateManager::getSingletonPtr()->getCameraManager()->addCamera(this->cameraComponent->getCamera(), true, true);
                    }
                    else
                    {
                        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[SplitScreenComponent] Warning: The given camera behavior game object id: " + Ogre::StringConverter::toString(this->cameraBehaviorGameObjectId->getULong()) +
                                                                                                " has no CameraBehaviorComponent, hence no camera behavior is used for the split screen camera: " + this->gameObjectPtr->getName());
                    }
                }
                else
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[SplitScreenComponent] Warning: The given camera behavior game object id: " + Ogre::StringConverter::toString(this->cameraBehaviorGameObjectId->getULong()) +
                                                                                            " does not exist, hence no camera behavior is used for the split screen camera: " + this->gameObjectPtr->getName());
                }
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
                this->tempCamera->setFOVy(Ogre::Degree(90.0f));
                this->tempCamera->setNearClipDistance(0.1f);
                this->tempCamera->setFarClipDistance(500.0f);
                this->tempCamera->setQueryFlags(0 << 0);
                this->tempCamera->setPosition(this->tempCamera->getParentSceneNode()->convertLocalToWorldPositionUpdated(Ogre::Vector3(0.0f, 1.0f, -2.0f)));

                this->finalCombinedWorkspace = compositorManager->addWorkspace(this->gameObjectPtr->getSceneManager(), finalExternalChannels, this->tempCamera, finalWorkspaceName, true, -1);

                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[WorkspaceSplitComponent] Creating final combined workspace: " + finalWorkspaceName);

                WorkspaceModule::getInstance()->setPrimaryWorkspace2(this->gameObjectPtr->getSceneManager(), this->tempCamera, this->finalCombinedWorkspace);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "SplitScreenComponent::setupSplitScreen");
    }

	void SplitScreenComponent::cleanupSplitScreen(void)
    {
        WorkspaceModule::getInstance()->setSplitScreenScenarioActive(false);

        GraphicsModule::RenderCommand renderCommand = [this]()
        {
            Ogre::CompositorManager2* compositorManager = WorkspaceModule::getInstance()->getCompositorManager();

            // Stores the own camera before the camera component pointer is reset, because the workspace map
            // entry AND the CameraManager registration of this split screen camera must be removed at the
            // very end, regardless of whether it had an optional camera behavior.
            Ogre::Camera* ownCamera = nullptr;
            if (nullptr != this->cameraComponent)
            {
                ownCamera = this->cameraComponent->getCamera();
            }

            if (nullptr != this->tempCamera)
            {
                if (nullptr != this->finalCombinedWorkspace)
                {
                    // ATTENTION: WorkspaceModule::removeWorkspace MUST NOT be used here!
                    //
                    // The workspace map entry of the tempCamera was registered as a DUMMY entry (see
                    // setPrimaryWorkspace2). WorkspaceModule::removeWorkspace destroys the current workspace
                    // of a dummy entry and immediately creates a REPLACEMENT dummy workspace for the same
                    // camera, keeping the map entry alive - which is correct for cameras that keep living,
                    // but fatal here: the tempCamera is destroyed a few lines below, so that freshly created
                    // dummy workspace would keep pointing at a destroyed camera. The render thread then
                    // crashes in CompositorPassScene::execute at mCamera->getSceneManager() with a freed
                    // camera.
                    //
                    // removeCamera destroys the workspace of the entry and ERASES the map entry without
                    // creating any replacement, which is exactly what a camera about to die needs.
                    WorkspaceModule::getInstance()->removeCamera(this->tempCamera);

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

                    this->finalCombinedWorkspace = nullptr;
                }

                // Only now the camera may die: no workspace references it anymore and no map entry can
                // re-create one for it.
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

            // Removes the CameraManager entry of this split screen camera, whether it went through
            // addCamera() (had an optional behavior) or only through registerSplitScreenCamera() (purely
            // static split screen camera). removeCamera() correctly handles both an active-with-behaviors
            // entry and a plain entry with empty behaviorData (the for-loop over behaviorData simply does
            // not execute in the latter case), and erases the map entry either way. Without this, a
            // non-behavior split screen camera would stay flagged forSplitScreen forever with stale
            // geometry, and getCameraForScreenPosition would keep considering it a candidate.
            if (nullptr != ownCamera)
            {
                WorkspaceModule::getInstance()->getPrimaryCameraComponent();
                AppStateManager::getSingletonPtr()->getCameraManager()->removeCamera(ownCamera);
            }

            this->externalChannels.clear();

            // Re-activates the main camera at the very END, so that its workspace is created only after
            // this split screen workspace has been torn down completely.
            CameraComponent* primaryCameraComponent = WorkspaceModule::getInstance()->getPrimaryCameraComponent();
            if (nullptr != primaryCameraComponent)
            {
                primaryCameraComponent->setActivated(true);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "SplitScreenComponent::cleanupSplitScreen");
    }

    Ogre::Vector4 SplitScreenComponent::computeGeometryFromPreset(const Ogre::String& preset, size_t index, size_t totalCount)
    {
        if ("2-Vertical" == preset && 2 == totalCount)
        {
            if (0 == index)
            {
                return Ogre::Vector4(0.0f, 0.0f, 0.5f, 1.0f);
            }
            else
            {
                return Ogre::Vector4(0.5f, 0.0f, 0.5f, 1.0f);
            }
        }
        else if ("2-Horizontal" == preset && 2 == totalCount)
        {
            if (0 == index)
            {
                return Ogre::Vector4(0.0f, 0.5f, 1.0f, 0.5f);
            }
            else
            {
                return Ogre::Vector4(0.0f, 0.0f, 1.0f, 0.5f);
            }
        }
        else if ("3-Vertical" == preset && 3 == totalCount)
        {
            Ogre::Real thirdWidth = 1.0f / 3.0f;
            return Ogre::Vector4(static_cast<Ogre::Real>(index) * thirdWidth, 0.0f, thirdWidth, 1.0f);
        }
        else if ("3-Horizontal" == preset && 3 == totalCount)
        {
            Ogre::Real thirdHeight = 1.0f / 3.0f;
            return Ogre::Vector4(0.0f, 1.0f - static_cast<Ogre::Real>(index + 1) * thirdHeight, 1.0f, thirdHeight);
        }
        else if ("4-Grid" == preset && 4 == totalCount)
        {
            Ogre::Real halfWidth = 0.5f;
            Ogre::Real halfHeight = 0.5f;
            Ogre::Real posX = (0 == index % 2) ? 0.0f : halfWidth;
            Ogre::Real posY = (index < 2) ? halfHeight : 0.0f;
            return Ogre::Vector4(posX, posY, halfWidth, halfHeight);
        }

        // Preset is "Custom", or the preset name does not match the actual number of currently
        // activated split screen cameras: signal "not applicable" via a negative x component.
        return Ogre::Vector4(-1.0f, -1.0f, -1.0f, -1.0f);
    }

    void SplitScreenComponent::applyPreset(void)
    {
        Ogre::String preset = this->splitPreset->getListSelectedValue();

        if ("Custom" == preset)
        {
            return;
        }

        auto splitScreenComponents = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectComponents<SplitScreenComponent>();

        std::vector<boost::shared_ptr<SplitScreenComponent>> tempSplitScreenComponents;
        for (size_t i = 0; i < splitScreenComponents.size(); i++)
        {
            if (true == splitScreenComponents[i]->isActivated())
            {
                tempSplitScreenComponents.emplace_back(splitScreenComponents[i]);
            }
        }

        if (true == tempSplitScreenComponents.empty())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[SplitScreenComponent] Cannot apply preset '" + preset + "', because there are no activated split screen cameras in the scene.");
            return;
        }

        bool anyMismatch = false;

        for (size_t i = 0; i < tempSplitScreenComponents.size(); i++)
        {
            Ogre::Vector4 presetGeometry = SplitScreenComponent::computeGeometryFromPreset(preset, i, tempSplitScreenComponents.size());
            if (presetGeometry.x >= 0.0f)
            {
                tempSplitScreenComponents[i]->setGeometry(presetGeometry);

                // Keeps the SplitPreset dropdown of EVERY participating split screen camera in sync with the
                // one that was actually selected, so the properties panel does not show a stale "Custom" the
                // next time that other camera game object is clicked in the editor. Written directly on the
                // foreign component's own Variant (like GameObject::getAttribute(...) is used elsewhere for
                // foreign game objects), NOT via setSplitPreset()/actualizeValue(), to avoid re-entering
                // applyPreset() for every single one of them. No explicit GUI refresh event needed here: this
                // game object's own AttrActionNeedRefresh flag on splitPreset already triggers the refresh for
                // the one currently selected in the editor, and the others simply show correct values once
                // clicked, since their Variant state is already correct now.
                NOWA::Variant* foreignSplitPresetAttribute = tempSplitScreenComponents[i]->getAttribute(SplitScreenComponent::AttrApplyPreset());
                if (nullptr != foreignSplitPresetAttribute)
                {
                    foreignSplitPresetAttribute->setListSelectedValue(preset);
                }
            }
            else
            {
                anyMismatch = true;
            }
        }

        if (true == anyMismatch)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                "[SplitScreenComponent] Preset '" + preset + "' does not match the number of currently activated split screen cameras (" + Ogre::StringConverter::toString(tempSplitScreenComponents.size()) + "). Geometry has not been changed.");
        }
    }

	// Lua registration part

	SplitScreenComponent* getSplitScreenComponentFromIndex(GameObject* gameObject, unsigned int occurrenceIndex)
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

	void setCameraBehaviorGameObjectId(SplitScreenComponent* instance, const Ogre::String& cameraBehaviorGameObjectId)
	{
		instance->setCameraBehaviorGameObjectId(Ogre::StringConverter::parseUnsignedLong(cameraBehaviorGameObjectId));
	}

	Ogre::String getCameraBehaviorGameObjectId(SplitScreenComponent* instance)
	{
		return Ogre::StringConverter::toString(instance->getCameraBehaviorGameObjectId());
	}

	void SplitScreenComponent::createStaticApiForLua(lua_State* lua,luabind::class_<GameObject>& gameObjectClass,luabind::class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
			[
				class_<SplitScreenComponent, GameObjectComponent>("SplitScreenComponent")
					.def("setActivated", &SplitScreenComponent::setActivated)
					.def("isActivated", &SplitScreenComponent::isActivated)
					.def("setCameraBehaviorGameObjectId", &setCameraBehaviorGameObjectId)
					.def("getCameraBehaviorGameObjectId", &getCameraBehaviorGameObjectId)
			];

		LuaScriptApi::getInstance()->addClassToCollection("SplitScreenComponent", "class inherits GameObjectComponent", SplitScreenComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("SplitScreenComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
		LuaScriptApi::getInstance()->addClassToCollection("SplitScreenComponent", "bool isActivated()", "Gets whether this component is activated.");
		LuaScriptApi::getInstance()->addClassToCollection("SplitScreenComponent", "void setCameraBehaviorGameObjectId(string cameraBehaviorGameObjectId)", "Sets camera behavior game object id in order if the camera behavior shall be used for an this splitscreen camera. "
			" If 0 (not set), the currently active camera is used.");
		LuaScriptApi::getInstance()->addClassToCollection("SplitScreenComponent", "string getCameraBehaviorGameObjectId()", "Sets the camera behavior game object id in order if the camera behavior shall be used for an this splitscreen camera. "
			" If 0 (not set), the currently active camera is used.");

		gameObjectClass.def("getSplitScreenComponentFromName", &getSplitScreenComponentFromName);
		gameObjectClass.def("getSplitScreenComponent", (SplitScreenComponent * (*)(GameObject*)) & getSplitScreenComponent);
		// If its desired to create several of this components for one game object
		gameObjectClass.def("getSplitScreenComponentFromIndex", (SplitScreenComponent * (*)(GameObject*, unsigned int)) & getSplitScreenComponentFromIndex);

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