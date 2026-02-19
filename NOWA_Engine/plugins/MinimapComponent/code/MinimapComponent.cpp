#include "NOWAPrecompiled.h"
#include "MinimapComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"
#include "gameobject/TerraComponent.h"

#include "Compositor/Pass/PassQuad/OgreCompositorPassQuadDef.h"
#include "Compositor/Pass/PassMipmap/OgreCompositorPassMipmapDef.h"
#include "Compositor/Pass/PassScene/OgreCompositorPassScene.h"

#include "OgreAbiUtils.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	MinimapComponent::MinimapComponent()
		: GameObjectComponent(),
		name("MinimapComponent"),
		targetGameObject(nullptr),
		minimapTexture(nullptr),
		heightMapTexture(nullptr),
		fogOfWarTexture(nullptr),
		fogOfWarStagingTexture(nullptr),
		textureManager(nullptr),
		workspace(nullptr),
		minimapWidget(nullptr),
		maskWidget(nullptr),
		fogOfWarWidget(nullptr),
		minimumBounds(Ogre::Vector3::ZERO),
		maximumBounds(Ogre::Vector3::ZERO),
		cameraComponent(nullptr),
		terraComponent(nullptr),
		timeSinceLastUpdate(0.25f),
		lastMapRotation(0.0f),
		activated(new Variant(MinimapComponent::AttrActivated(), true, this->attributes)),
		targetId(new Variant(MinimapComponent::AttrTargetId(), static_cast<unsigned long>(0), this->attributes, true)),
		textureSize(new Variant(MinimapComponent::AttrTextureSize(), static_cast<unsigned int>(256), this->attributes)),
		minimapGeometry(new Variant(MinimapComponent::AttrMinimapGeometry(), Ogre::Vector4(0.8f, 0.0f, 0.2f, 0.2f), this->attributes)),
		transparent(new Variant(MinimapComponent::AttrTransparent(), false, this->attributes)),
		useFogOfWar(new Variant(MinimapComponent::AttrUseFogOfWar(), false, this->attributes)),
		useDiscovery(new Variant(MinimapComponent::AttrUseDiscovery(), false, this->attributes)),
		persistDiscovery(new Variant(MinimapComponent::AttrPersistDiscovery(), false, this->attributes)),
		visibilityRadius(new Variant(MinimapComponent::AttrVisibilityRadius(), Ogre::Real(5.0f), this->attributes)),
		useVisibilitySpotlight(new Variant(MinimapComponent::AttrUseVisibilitySpotlight(), false, this->attributes)),
		wholeSceneVisible(new Variant(MinimapComponent::AttrWholeSceneVisible(), true, this->attributes)),
		cameraHeight(new Variant(MinimapComponent::AttrCameraHeight(), Ogre::Real(10.0f), this->attributes)),
		minimapMask(new Variant(MinimapComponent::AttrMinimapMask(), "", this->attributes)),
		useRoundMinimap(new Variant(MinimapComponent::AttrUseRoundMinimap(), false, this->attributes)),
		rotateMinimap(new Variant(MinimapComponent::AttrRotateMinimap(), false, this->attributes))
	{
		this->targetId->setDescription("Sets the target id for the game object, which shall be tracked.");
		this->textureSize->setDescription("Sets the minimap texture size in pixels. Note: The texture is quadratic. Also note: The higher the texture size, the less performant the application will run.");
		this->minimapGeometry->setDescription("Sets the geometry of the minimap relative to the window screen in the format Vector4(pos.x, pos.y, width, height).");
		this->transparent->setDescription("Sets whether the minimap shall be somewhat transparent.");
		this->useFogOfWar->setDescription("If whole scene visible is set to true. Sets whether fog of war is used.");
		this->useFogOfWar->addUserData(GameObject::AttrActionNeedRefresh());
		this->useDiscovery->setDescription("If fog of war is used, sets whether places in which the target game object has visited remain visible.");
		this->persistDiscovery->setDescription("If fog of war is used and use discovery, sets whether places in which the target game object has visited remain visible and also stored and loaded.");
		this->visibilityRadius->setDescription("If fog of war is used, sets the visibilty radius which discovers the fog of war.");
		this->useVisibilitySpotlight->setDescription("If fog of war is used, sets a spotlight instead of rounded radius area. The spotlight is as big as the visibility radius.");
		this->wholeSceneVisible->setDescription("Sets whether the whole scene is visible. If set to true, the camera will be positioned in the middle of the whole scene and the height adapted, for complete scene rendering on the minimap. "
												" If set to false, only an area is visible and the minimap is scrolled according to the target game object position.");
		this->wholeSceneVisible->addUserData(GameObject::AttrActionNeedRefresh());
		this->cameraHeight->setDescription("If whole scene visible is set to false, sets the camera height, which is added at the top of the y position of the target game object.");

		this->minimapMask->addUserData(GameObject::AttrActionFileOpenDialog(), "Minimap");
		this->minimapMask->setDescription("Sets a minimap image mask, which can be created in gimp and added to the Minimap.xml in the MyGui/Minimap folder.");

		this->useRoundMinimap->setDescription("If whole scene visible is set to false, sets whether to render a rounded minimap texture, for more sophistacted effects. This can e.g. be used in conjunction with the minimap mask to create a nice minimap overlay.");
		this->rotateMinimap->setDescription("If whole scene visible is set to false, sets whether the minimap is rotated according to the target game object orientation.");
	}

	MinimapComponent::~MinimapComponent(void)
	{
		
	}

	const Ogre::String& MinimapComponent::getName() const
	{
		return this->name;
	}

	void MinimapComponent::install(const Ogre::NameValuePairList* options)
	{
		Ogre::ResourceGroupManager::getSingletonPtr()->addResourceLocation("../../media/MyGUI_Media/minimap", "FileSystem", "Minimap");

		/*Ogre::StringVectorPtr names = Ogre::ResourceGroupManager::getSingletonPtr()->findResourceNames("Minimap", "*.png");
		std::vector<Ogre::String> compatibleNames(names.getPointer()->size() + 1);
		unsigned int i = 0;
		compatibleNames[i++] = "";
		for (auto& it = names.getPointer()->cbegin(); it != names.getPointer()->cend(); it++)
		{
			compatibleNames[i++] = *it;
		}

		this->minimapMask->setValue(compatibleNames);*/

		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<MinimapComponent>(MinimapComponent::getStaticClassId(), MinimapComponent::getStaticClassName());
	}
	
	void MinimapComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool MinimapComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetId")
		{
			this->targetId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TextureSize")
		{
			this->textureSize->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MinimapGeometry")
		{
			this->minimapGeometry->setValue(XMLConverter::getAttribVector4(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Transparent")
		{
			this->transparent->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseFogOfWar")
		{
			this->setUseFogOfWar(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseDiscovery")
		{
			this->useDiscovery->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PersistDiscovery")
		{
			this->persistDiscovery->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "VisibilityRadius")
		{
			this->visibilityRadius->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseVisibilitySpotlight")
		{
			this->useVisibilitySpotlight->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WholeSceneVisible")
		{
			this->setWholeSceneVisible(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CameraHeight")
		{
			this->cameraHeight->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MinimapMask")
		{
			this->minimapMask->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseRoundMinimap")
		{
			this->useRoundMinimap->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RotateMinimap")
		{
			this->rotateMinimap->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		return true;
	}

	GameObjectCompPtr MinimapComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		return nullptr;
	}

	bool MinimapComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MinimapComponent] Init component for game object: " + this->gameObjectPtr->getName());

		AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &MinimapComponent::deleteGameObjectDelegate), EventDataDeleteGameObject::getStaticEventType());
		AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &MinimapComponent::handleUpdateBounds), EventDataBoundsUpdated::getStaticEventType());

		this->textureManager = Ogre::Root::getSingleton().getRenderSystem()->getTextureGpuManager();

		// Removes complexity, by setting default direction to -z, which is Ogre default.
		this->gameObjectPtr->setDefaultDirection(Ogre::Vector3::NEGATIVE_UNIT_Z);
		this->gameObjectPtr->getAttribute(GameObject::AttrDefaultDirection())->setVisible(false);

		return true;
	}

	void MinimapComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating && nullptr != this->targetGameObject)
		{
			this->timeSinceLastUpdate += dt;

			if (this->timeSinceLastUpdate >= 0.25f)
			{
				if (true == this->useFogOfWar->getBool())
				{
					auto closureFunction = [this](Ogre::Real renderDt)
					{
						this->updateFogOfWarTexture(this->targetGameObject->getPosition(), this->visibilityRadius->getReal());
					};
					Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
					NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
				}

				this->timeSinceLastUpdate = 0.0f;
			}

			if (false == this->wholeSceneVisible->getBool())
			{
				this->scrollMinimap(this->targetGameObject->getPosition());
			}
		}
	}

	bool MinimapComponent::connect(void)
	{
		GameObjectComponent::connect();

		const auto& targetGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->targetId->getULong());
		if (nullptr != targetGameObjectPtr)
		{
			this->targetGameObject = targetGameObjectPtr.get();
		}

		this->setActivated(this->activated->getBool());
		
		return true;
	}

	bool MinimapComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();

		Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
		NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

		this->targetGameObject = nullptr;
		this->cameraComponent = nullptr;
		this->terraComponent = nullptr;

		this->timeSinceLastUpdate = 0.25f;
		this->lastMapRotation = 0.0f;

		this->removeWorkspace();

		return true;
	}

	bool MinimapComponent::onCloned(void)
	{
		
		return true;
	}

	void MinimapComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
		NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

		this->cameraComponent = nullptr;

		AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &MinimapComponent::deleteGameObjectDelegate), EventDataDeleteGameObject::getStaticEventType());
		AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &MinimapComponent::handleUpdateBounds), EventDataBoundsUpdated::getStaticEventType());

		if (true == this->persistDiscovery->getBool())
		{
			this->saveDiscoveryState();
		}

		this->removeWorkspace();
	}
	
	void MinimapComponent::onOtherComponentRemoved(unsigned int index)
	{
        if (nullptr != this->cameraComponent && index == this->cameraComponent->getIndex())
        {
            this->cameraComponent = nullptr;
        }
	}
	
	void MinimapComponent::onOtherComponentAdded(unsigned int index)
	{
		
	}

	void MinimapComponent::setupMinimapWithFogOfWar(void)
	{
		const auto& cameraCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<CameraComponent>());
		if (nullptr == cameraCompPtr || this->gameObjectPtr->getId() == GameObjectController::MAIN_CAMERA_ID)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MinimapComponent] Error setting up minimap workspace, because the game object: " + this->gameObjectPtr->getName() + " is the main camera. Choose a different camera!");
			return;
		}

		this->cameraComponent = cameraCompPtr.get();

		ENQUEUE_RENDER_COMMAND_WAIT("MinimapComponent::setupMinimapWithFogOfWar",
		{
			auto gameObjectsWithTerraComponent = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsFromComponent(TerraComponent::getStaticClassName());
			if (gameObjectsWithTerraComponent.size() > 0)
			{
				// For now support just for 1 terra
				const auto& terraCompPtr = NOWA::makeStrongPtr(gameObjectsWithTerraComponent[0]->getComponent<TerraComponent>());
				if (nullptr != terraCompPtr)
				{
					this->terraComponent = terraCompPtr.get();

					/*
					Render the whole terrain from above in Ortho projection with high resolution into a RenderTexture. Keep that texture.
					If the terrain is too big to the point that LOD becomes a problem, render it in chunks (e.g. move the camera to the first quadrant and render 2048x2048 region, then move to the next quadrant; 4 times until you get the whole 4096x4096)
					Alternatively, set m_basePixelDimension to a very high value to have very little LOD.
					*/
					// Adjust this via height of camera?
					this->terraComponent->setBasePixelDimension(128);

					if (this->cameraComponent->getOwner()->getId() != this->terraComponent->geCameraId())
					{
						this->terraComponent->setCameraId(this->cameraComponent->getOwner()->getId());
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MinimapComponent] Attention: Changed camera for terra, in order to get minimap working for game object: " + this->gameObjectPtr->getName());
					}
				}
			}

			// Remove the workspace from the camera, as it is used for a new minimap workspace, which is created here
			const auto& workspaceCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<WorkspaceBaseComponent>());
			if (nullptr != workspaceCompPtr)
			{
				this->gameObjectPtr->deleteComponent(workspaceCompPtr.get());
				return;
			}

			if (true == this->persistDiscovery->getBool())
			{
				this->loadDiscoveryState();
			}

			if (true == this->wholeSceneVisible->getBool())
			{
				this->cameraComponent->getOwner()->setDynamic(true);
				this->adjustMinimapCamera();
				this->cameraComponent->getOwner()->setDynamic(false);
			}
			else
			{
				this->cameraComponent->getOwner()->setDynamic(true);
			}


			Ogre::String minimapTextureName = "MinimapRT";

			if (true == this->useRoundMinimap->getBool())
			{
				minimapTextureName = "MinimapRT_Round";
			}

			this->minimapTexture = this->createMinimapTexture(minimapTextureName, this->textureSize->getUInt(), this->textureSize->getUInt());
			if (true == this->useFogOfWar->getBool())
			{
				this->fogOfWarTexture = this->createFogOfWarTexture("FogOfWarTexture", this->textureSize->getUInt(), this->textureSize->getUInt());
				this->fogOfWarStagingTexture = this->textureManager->getStagingTexture(this->fogOfWarTexture->getWidth(), this->fogOfWarTexture->getHeight(), 1u, 1u, this->fogOfWarTexture->getPixelFormat());
			}

			if (true == this->useDiscovery->getBool())
			{
				this->discoveryState.resize((this->textureSize->getUInt()), std::vector<bool>(this->textureSize->getUInt(), false));
			}

			Ogre::CompositorManager2* compositorManager = WorkspaceModule::getInstance()->getCompositorManager();

			if (false == this->useRoundMinimap->getBool())
			{
				this->createMinimapWorkspace();
			}
			else
			{
				this->createRoundMinimapWorkspace();
			}

			// Test 
			/*Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingletonPtr()->getHlmsManager();
			Ogre::Hlms* hlms = hlmsManager->getHlms(Ogre::HLMS_USER3);
			OGRE_ASSERT_HIGH(dynamic_cast<HlmsTerra*>(hlms));
			this->workspace->addListener(new Ogre::TerraWorkspaceListener((Ogre::HlmsTerra*)hlms));*/


			// Create MyGUI widget for the minimap
			Ogre::Vector4 geometry = this->minimapGeometry->getVector4();
			this->minimapWidget = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::ImageBox>("ImageBox", MyGUI::FloatCoord(geometry.x, geometry.y, geometry.z, geometry.w), MyGUI::Align::HCenter, "Overlapped");
			this->minimapWidget->setImageTexture(this->minimapTexture->getNameStr());

			if (false == this->minimapMask->getString().empty())
			{
				// Overlay the circular mask on top of the minimap
				this->maskWidget = minimapWidget->createWidget<MyGUI::ImageBox>("ImageBox", MyGUI::IntCoord(0, 0, this->minimapWidget->getWidth(), this->minimapWidget->getHeight()), MyGUI::Align::Stretch);
				this->maskWidget->setImageTexture(this->minimapMask->getString());
				// Makes sure the mask doesn't block mouse events
				this->maskWidget->setNeedMouseFocus(false);
				// Higher depth to ensure it's on top
				this->maskWidget->setDepth(1);
			}

			if (true == this->useFogOfWar->getBool())
			{
				Ogre::Vector4 geometry = this->minimapGeometry->getVector4();
				this->fogOfWarWidget = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::ImageBox>("ImageBox", MyGUI::FloatCoord(geometry.x, geometry.y, geometry.z, geometry.w), MyGUI::Align::HCenter, "Overlapped");
				this->fogOfWarWidget->setImageTexture(this->fogOfWarTexture->getNameStr());

				if (false == this->persistDiscovery->getBool())
				{
					this->clearFogOfWar();
				}
			}
		});
	}


	Ogre::TextureGpu* MinimapComponent::createMinimapTexture(const Ogre::String& name, unsigned int width, unsigned int height)
	{
		// Threadsafe from the outside
		Ogre::TextureGpu* texture = nullptr;
		if (false == this->textureManager->hasTextureResource(name, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME))
		{
			if (false == this->transparent->getBool())
			{
				texture = this->textureManager->createTexture(name, Ogre::GpuPageOutStrategy::SaveToSystemRam, Ogre::TextureFlags::RenderToTexture, Ogre::TextureTypes::Type2D);
				texture->setResolution(width, height);
				texture->setPixelFormat(Ogre::PFG_RGBA8_UNORM_SRGB);
				texture->setNumMipmaps(1u);
				texture->scheduleTransitionTo(Ogre::GpuResidency::Resident);
				texture->_setNextResidencyStatus(Ogre::GpuResidency::Resident);
			}
			else
			{
				texture = this->textureManager->createTexture(name, Ogre::GpuPageOutStrategy::Discard, Ogre::TextureFlags::RenderToTexture | Ogre::TextureFlags::AllowAutomipmaps, Ogre::TextureTypes::Type2D);
				texture->setResolution(width, height);
				texture->setNumMipmaps(Ogre::PixelFormatGpuUtils::getMaxMipmapCount(width, height));
				texture->setPixelFormat(Ogre::PFG_RGBA8_UNORM_SRGB);
				texture->scheduleTransitionTo(Ogre::GpuResidency::Resident);
				texture->_setNextResidencyStatus(Ogre::GpuResidency::Resident);
			}
		}
		else
		{
			texture = this->textureManager->findTextureNoThrow(name);
		}
		return texture;
	}

	Ogre::TextureGpu* MinimapComponent::createFogOfWarTexture(const Ogre::String& name, unsigned int width, unsigned int height)
	{
		Ogre::TextureGpu* texture = nullptr;
		if (false == this->textureManager->hasTextureResource(name, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME))
		{
			texture = this->textureManager->createTexture(name, Ogre::GpuPageOutStrategy::SaveToSystemRam, Ogre::TextureFlags::RenderToTexture, Ogre::TextureTypes::Type2D);
			texture->setResolution(width, height);
			texture->setPixelFormat(Ogre::PFG_RGBA8_UNORM_SRGB);
			texture->setNumMipmaps(1u);
			texture->scheduleTransitionTo(GpuResidency::Resident);
			texture->_setNextResidencyStatus(GpuResidency::Resident);
		}
		else
		{
			texture = this->textureManager->findTextureNoThrow(name);
		}
		return texture;
	}

	void MinimapComponent::createRoundMinimapWorkspace(void)
	{
		// Threadsafe from the outside
		Ogre::CompositorManager2* compositorManager = WorkspaceModule::getInstance()->getCompositorManager();

		this->minimapNodeName = "MinimapNode_Round";
		if (false == compositorManager->hasNodeDefinition(this->minimapNodeName))
		{
			Ogre::CompositorNodeDef* nodeDef = compositorManager->addNodeDefinition(this->minimapNodeName);

			nodeDef->addTextureSourceName("rt0", 0, Ogre::TextureDefinitionBase::TEXTURE_INPUT);

			// Texture definition
			Ogre::TextureDefinitionBase::TextureDefinition* texDef = nodeDef->addTextureDefinition("MinimapRT");
			texDef->width = 1024;
			texDef->height = 1024;
			texDef->format = Ogre::PFG_RGBA8_UNORM_SRGB;

			Ogre::RenderTargetViewDef* rtv = nodeDef->addRenderTextureView("MinimapRT");
			Ogre::RenderTargetViewEntry attachment;
			attachment.textureName = "MinimapRT";
			rtv->colourAttachments.push_back(attachment);

			nodeDef->setNumTargetPass(2);
			{
				Ogre::CompositorTargetDef* targetDef = nodeDef->addTargetPass("MinimapRT");
				targetDef->setNumPasses(2);
				{
					// Clear Pass
					{
						Ogre::CompositorPassClearDef* passClear;
						passClear = static_cast<Ogre::CompositorPassClearDef*>(targetDef->addPass(Ogre::PASS_CLEAR));

						passClear->mClearColour[0] = Ogre::ColourValue(0.2f, 0.4f, 0.6f);
						passClear->setAllStoreActions(Ogre::StoreAction::Store);
					}

					{
						// Scene pass for minimap
						Ogre::CompositorPassSceneDef* passScene = static_cast<Ogre::CompositorPassSceneDef*>(
							targetDef->addPass(Ogre::PASS_SCENE));
						passScene->mCameraName = this->cameraComponent->getCamera()->getName();

						passScene->mClearColour[0] = Ogre::ColourValue::Black;

						passScene->setAllStoreActions(Ogre::StoreAction::Store);

						// Sets the corresponding render category. All game objects which do not match that category, will not be rendered for this camera
						// Note: MyGui is added to the final split combined workspace, so it does not make sense to exclude mygui objects from rendering
						unsigned int finalRenderMask = AppStateManager::getSingletonPtr()->getGameObjectController()->generateRenderCategoryId(this->cameraComponent->getExcludeRenderCategories());
						passScene->setVisibilityMask(finalRenderMask);

						passScene->mIncludeOverlays = false;

					}
				}

				targetDef = nodeDef->addTargetPass("rt0");
				targetDef->setNumPasses(1);
				{
					// Render Quad
					{
						Ogre::CompositorPassQuadDef* passQuad;
						passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));

						passQuad->addQuadTextureSource(0, "MinimapRT");
						// passQuad->mNumInitialPasses = 1; // Texture would not be updated
						passQuad->mMaterialName = "MinimapMask";
					}
				}
			}
			nodeDef->setNumOutputChannels(1);
			nodeDef->mapOutputChannel(0, "MinimapRT");
		}

		this->minimapWorkspaceName = "MinimapWorkspaceWithFoW_Round";
		if (false == compositorManager->hasWorkspaceDefinition(this->minimapWorkspaceName))
		{
			Ogre::CompositorWorkspaceDef* workspaceDef = compositorManager->addWorkspaceDefinition(this->minimapWorkspaceName);
			workspaceDef->clearAllInterNodeConnections();

			workspaceDef->connectExternal(0, this->minimapNodeName, 0);
		}

		this->externalChannels.resize(1);
		this->externalChannels[0] = this->minimapTexture;

		this->workspace = compositorManager->addWorkspace(this->gameObjectPtr->getSceneManager(), this->externalChannels, this->cameraComponent->getCamera(), this->minimapWorkspaceName, true);
	}

	void MinimapComponent::createMinimapWorkspace(void)
	{
		// Threadsafe from the outside
		Ogre::CompositorManager2* compositorManager = WorkspaceModule::getInstance()->getCompositorManager();

		this->minimapNodeName = "MinimapNode";
		if (false == compositorManager->hasNodeDefinition(this->minimapNodeName))
		{
			Ogre::CompositorNodeDef* nodeDef = compositorManager->addNodeDefinition(this->minimapNodeName);

			nodeDef->addTextureSourceName("MinimapRT", 0, Ogre::TextureDefinitionBase::TEXTURE_INPUT);

			nodeDef->setNumTargetPass(1);
			{
				Ogre::CompositorTargetDef* targetDef = nodeDef->addTargetPass("MinimapRT");
				targetDef->setNumPasses(2);
				{
					// Clear Pass
					{
						Ogre::CompositorPassClearDef* passClear;
						passClear = static_cast<Ogre::CompositorPassClearDef*>(targetDef->addPass(Ogre::PASS_CLEAR));

						passClear->mClearColour[0] = Ogre::ColourValue(0.2f, 0.4f, 0.6f);
						passClear->setAllStoreActions(Ogre::StoreAction::Store);
					}

					{
						// Scene pass for minimap
						Ogre::CompositorPassSceneDef* passScene = static_cast<Ogre::CompositorPassSceneDef*>(
							targetDef->addPass(Ogre::PASS_SCENE));
						passScene->mCameraName = this->cameraComponent->getCamera()->getName();

						passScene->mClearColour[0] = Ogre::ColourValue::Black;

						passScene->setAllStoreActions(Ogre::StoreAction::Store);

						// Sets the corresponding render category. All game objects which do not match that category, will not be rendered for this camera
						// Note: MyGui is added to the final split combined workspace, so it does not make sense to exclude mygui objects from rendering
						unsigned int finalRenderMask = AppStateManager::getSingletonPtr()->getGameObjectController()->generateRenderCategoryId(this->cameraComponent->getExcludeRenderCategories());
						passScene->setVisibilityMask(finalRenderMask);

						passScene->mIncludeOverlays = false;
					}
				}
			}
			nodeDef->mapOutputChannel(0, "MinimapRT");
		}

		this->minimapWorkspaceName = "MinimapWorkspaceWithFoW";

		if (false == compositorManager->hasWorkspaceDefinition(this->minimapWorkspaceName))
		{
			Ogre::CompositorWorkspaceDef* workspaceDef = compositorManager->addWorkspaceDefinition(this->minimapWorkspaceName);
			workspaceDef->clearAllInterNodeConnections();

			workspaceDef->connectExternal(0, this->minimapNodeName, 0);
		}

		this->externalChannels.resize(1);
		this->externalChannels[0] = this->minimapTexture;

		this->workspace = compositorManager->addWorkspace(this->gameObjectPtr->getSceneManager(), this->externalChannels, this->cameraComponent->getCamera(), this->minimapWorkspaceName, true);
	}

	void MinimapComponent::clearFogOfWar(void)
	{
		// Threadsafe from the outside
		this->discoveryState.clear();
		this->discoveryState.resize((this->textureSize->getUInt()), std::vector<bool>(this->textureSize->getUInt(), false));

		unsigned int texSize = this->textureSize->getUInt();

		this->fogOfWarStagingTexture->startMapRegion();
		Ogre::TextureBox texBox = this->fogOfWarStagingTexture->mapRegion(this->fogOfWarTexture->getWidth(), this->fogOfWarTexture->getHeight(), 1u, 1u, this->fogOfWarTexture->getPixelFormat());

		const size_t bytesPerPixel = texBox.bytesPerPixel;

		for (uint32 y = 0; y < texSize; y++)
		{
			uint8* RESTRICT_ALIAS pixBoxData = reinterpret_cast<uint8 * RESTRICT_ALIAS>(texBox.at(0, y, 0));
			for (uint32 x = 0; x < texSize; x++)
			{
				const size_t dstIdx = x * bytesPerPixel;
				float rgba[4];
				// Create blank texture without details
				rgba[0] = 0.0f;
				rgba[1] = 0.0f;
				rgba[2] = 0.0f;
				rgba[3] = 255.0f;
				PixelFormatGpuUtils::packColour(rgba, this->fogOfWarTexture->getPixelFormat(), &pixBoxData[dstIdx]);
			}
		}

		this->fogOfWarStagingTexture->stopMapRegion();
		this->fogOfWarStagingTexture->upload(texBox, this->fogOfWarTexture, 0, 0, 0);
	}

	void MinimapComponent::updateFogOfWarTexture(const Ogre::Vector3& position, Ogre::Real radius)
	{
		// Threadsafe from the outside
		// Use code from EndFrameOnceFailureGameState.h for parallelization!
		unsigned int texSize = this->textureSize->getUInt();

		int radiusInPixels = static_cast<int>(this->visibilityRadius->getReal() * texSize / (this->maximumBounds.x - this->minimumBounds.x));

		bool visibilitySpotlight = this->useVisibilitySpotlight->getBool();

		// Determines spotlight direction
		Ogre::Vector3 spotlightDirection = Ogre::Vector3::ZERO;

		if (true == visibilitySpotlight)
		{
			spotlightDirection = this->targetGameObject->getOrientation() * this->targetGameObject->getDefaultDirection();
			spotlightDirection *= Ogre::Vector3(1.0f, 0.0f, 1.0f);
		}

		this->fogOfWarStagingTexture->startMapRegion();
		Ogre::TextureBox texBox = this->fogOfWarStagingTexture->mapRegion(this->fogOfWarTexture->getWidth(), this->fogOfWarTexture->getHeight(), 1u, 1u, this->fogOfWarTexture->getPixelFormat());

		// Assuming a 2D texture, the z-coordinate will be 0
		size_t z = 0;

		Ogre::Vector2 normalizedPosition = this->normalizePosition(position);
		Ogre::Vector2 textureCoordinates = this->mapToTextureCoordinates(normalizedPosition, texSize, texSize);

		const size_t bytesPerPixel = texBox.bytesPerPixel;

		float padding = 0.0f;
		if (false == this->minimapMask->getString().empty())
		{
			padding = 2.0f;
		}

		if (false == this->useDiscovery->getBool())
		{
			for (uint32 y = 0; y < texSize; y++)
			{
				uint8* RESTRICT_ALIAS pixBoxData = reinterpret_cast<uint8 * RESTRICT_ALIAS>(texBox.at(0, y, 0));
				for (uint32 x = 0; x < texSize; x++)
				{
					float dx = x + textureCoordinates.x;
					float dy = y + textureCoordinates.y;
					float distance = sqrt(dx * dx + dy * dy);
					
					float distanceRound = 0.0f;
					if (true == this->useRoundMinimap->getBool())
					{
						float rx = x - (static_cast<float>(texSize) * 0.5f);
						float ry = y - (static_cast<float>(texSize) * 0.5f);
						distanceRound = sqrt(rx * rx + ry * ry);
					}

					if (distance < radiusInPixels || distanceRound >= (static_cast<float>(texSize) * 0.5f) - padding)
					{
						const size_t dstIdx = x * bytesPerPixel;
						float rgba[4] = { 255.0f, 0.0f, 0.0f, 0.0f };
						PixelFormatGpuUtils::packColour(rgba, this->fogOfWarTexture->getPixelFormat(), &pixBoxData[dstIdx]);
					}
					else
					{
						const size_t dstIdx = x * bytesPerPixel;
						float rgba[4] = { 0.0f, 0.0f, 0.0f, 255.0f };
						PixelFormatGpuUtils::packColour(rgba, this->fogOfWarTexture->getPixelFormat(), &pixBoxData[dstIdx]);
					}
				}
			}
		}
		else
		{
			for (uint32 y = 0; y < this->fogOfWarTexture->getHeight(); y++)
			{
				uint8* RESTRICT_ALIAS pixBoxData = reinterpret_cast<uint8 * RESTRICT_ALIAS>(texBox.at(0, y, 0));
				for (uint32 x = 0; x < this->fogOfWarTexture->getWidth(); x++)
				{
					float dx = x + textureCoordinates.x;
					float dy = y + textureCoordinates.y;
					float distance = sqrt(dx * dx + dy * dy);

					float distanceRound = 0.0f;
					if (true == this->useRoundMinimap->getBool())
					{
						float rx = x - (static_cast<float>(texSize) * 0.5f);
						float ry = y - (static_cast<float>(texSize) * 0.5f);
						distanceRound = sqrt(rx * rx + ry * ry);
					}

					if ((distance < radiusInPixels) || (distanceRound >= (static_cast<float>(texSize) * 0.5f) - padding) || true == this->discoveryState[x][y])
					{
						if (true == visibilitySpotlight)
						{
							Ogre::Vector3 pointDirection(dx, 0, dy);
							pointDirection.normalise();
							Ogre::Real angle = Ogre::Math::ACos(spotlightDirection.dotProduct(pointDirection)).valueDegrees();
							if (angle <= 45.0f / 2)
							{
								this->discoveryState[x][y] = true;
								const size_t dstIdx = x * bytesPerPixel;
								float rgba[4] = { 255.0f, 0.0f, 0.0f, 0.0f };
								PixelFormatGpuUtils::packColour(rgba, this->fogOfWarTexture->getPixelFormat(), &pixBoxData[dstIdx]);
							}
						}
						else
						{
							this->discoveryState[x][y] = true;
							const size_t dstIdx = x * bytesPerPixel;
							float rgba[4] = { 255.0f, 0.0f, 0.0f, 0.0f };
							PixelFormatGpuUtils::packColour(rgba, this->fogOfWarTexture->getPixelFormat(), &pixBoxData[dstIdx]);
						}
					}
					else
					{
						const size_t dstIdx = x * bytesPerPixel;
						float rgba[4] = { 0.0f, 0.0f, 0.0f, 255.0f };
						PixelFormatGpuUtils::packColour(rgba, this->fogOfWarTexture->getPixelFormat(), &pixBoxData[dstIdx]);
					}
				}
			}
		}

		this->fogOfWarStagingTexture->stopMapRegion();
		this->fogOfWarStagingTexture->upload(texBox, this->fogOfWarTexture, 0, 0, 0);
	}

	Ogre::Vector2 MinimapComponent::normalizePosition(const Ogre::Vector3& position)
	{
		Ogre::Vector2 normalized = Ogre::Vector2::ZERO;
		normalized.x = (-position.x + this->minimumBounds.x) / (this->maximumBounds.x - this->minimumBounds.x);
		normalized.y = (-position.z + this->minimumBounds.z) / (this->maximumBounds.z - this->minimumBounds.z);
		return normalized;
	}

	Ogre::Vector2 MinimapComponent::mapToTextureCoordinates(const Ogre::Vector2& normalizedPosition, unsigned int textureWidth, unsigned int textureHeight)
	{
		Ogre::Vector2 textureCoordinates;
		textureCoordinates.x = normalizedPosition.x * textureWidth;
		textureCoordinates.y = normalizedPosition.y * textureHeight;
		return textureCoordinates;
	}

	void MinimapComponent::scrollMinimap(const Ogre::Vector3& position)
	{
		// Calculate the current camera height
		Ogre::Real cameraHeight = this->targetGameObject->getPosition().y + this->cameraHeight->getReal();
		Ogre::Real fovY = this->cameraComponent->getCamera()->getFOVy().valueRadians();
		Ogre::Real aspectRatio = this->cameraComponent->getCamera()->getAspectRatio();
		Ogre::Real viewHeight = 2.0f * cameraHeight * tan(fovY / 2.0f); // Calculate view height
		Ogre::Real viewWidth = viewHeight * aspectRatio; // Calculate view width

		const Ogre::Real padding = 2.0f;

		Ogre::Real leftBoundary = minimumBounds.x + (viewWidth / 2.0f) + padding;
		Ogre::Real rightBoundary = maximumBounds.x - (viewWidth / 2.0f) - padding;
		Ogre::Real topBoundary = minimumBounds.z + (viewHeight / 2.0f) + padding;
		Ogre::Real bottomBoundary = maximumBounds.z - (viewHeight / 2.0f) - padding;

		Ogre::Vector3 pos = this->targetGameObject->getPosition();
		Ogre::Quaternion orient = this->targetGameObject->getOrientation();

		bool isAtRightBorder = (pos.x >= rightBoundary);
		bool isAtBottomBorder = (pos.z >= bottomBoundary);
		bool isAtLeftBorder = (pos.x <= leftBoundary);
		bool isAtTopBorder = (pos.z <= topBoundary);

		// Adjust the camera position based on which border is being hit
		if (true == isAtRightBorder)
		{
			pos.x = rightBoundary; // Set to right boundary
		}
		if (true == isAtBottomBorder)
		{
			pos.z = bottomBoundary; // Set to bottom boundary
		}
		if (true == isAtLeftBorder)
		{
			pos.x = leftBoundary; // Set to left boundary
		}
		if (true == isAtTopBorder)
		{
			pos.z = topBoundary; // Set to top boundary
		}

		this->updateMinimapCamera(pos, orient);

		/*Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MinimapComponent] x: " + Ogre::StringConverter::toString(pos.x) + " y: " + Ogre::StringConverter::toString(pos.y)
						+ " leftBorder: " + Ogre::StringConverter::toString(isAtLeftBorder) + " topBorder: " + Ogre::StringConverter::toString(isAtTopBorder)
						+ " rightBorder: " + Ogre::StringConverter::toString(isAtRightBorder) + " bottomBorder: " + Ogre::StringConverter::toString(isAtBottomBorder));*/
	}

	void MinimapComponent::removeWorkspace(void)
	{
		if (nullptr != this->workspace)
		{
			Ogre::CompositorManager2* compositorManager = WorkspaceModule::getInstance()->getCompositorManager();

			compositorManager->removeWorkspace(this->workspace);

			// Note: Each time addWorkspaceDefinition is called, an "AutoGen Hash..." node is internally created by Ogre, this one must also be removed, else a crash occurs (bug in Ogre?)
			if (true == compositorManager->hasNodeDefinition(this->minimapNodeName))
			{
				compositorManager->removeNodeDefinition(this->minimapNodeName);
			}
			if (true == compositorManager->hasWorkspaceDefinition(this->minimapWorkspaceName))
			{
				compositorManager->removeWorkspaceDefinition(this->minimapWorkspaceName);
			}

			// auto texture = MyGUI::RenderManager::getInstancePtr()->getTexture(this->minimapTexture->getNameStr());
			// texture->destroy();

			// auto texture2 = MyGUI::RenderManager::getInstancePtr()->getTexture(this->fogOfWarTexture->getNameStr());
			// texture2->destroy();

			if (nullptr != this->maskWidget)
			{
				// Must be done, because mygui also holds a reference to the Ogre::TextureGpu texture.
				auto myGuiTexture = MyGUI::RenderManager::getInstancePtr()->getTexture(this->minimapMask->getString());
				if (nullptr != myGuiTexture)
				{
					static_cast<MyGUI::Ogre2RenderManager*>(MyGUI::RenderManager::getInstancePtr())->removeTexture(myGuiTexture);
				}
				this->maskWidget->setImageTexture("");
				MyGUI::Gui::getInstancePtr()->destroyWidget(this->maskWidget);
				this->maskWidget = nullptr;
			}
			if (nullptr != this->minimapWidget)
			{
				// Must be done, because mygui also holds a reference to the Ogre::TextureGpu texture.
				auto myGuiTexture = MyGUI::RenderManager::getInstancePtr()->getTexture(this->minimapTexture->getNameStr());
				if (nullptr != myGuiTexture)
				{
					static_cast<MyGUI::Ogre2RenderManager*>(MyGUI::RenderManager::getInstancePtr())->removeTexture(myGuiTexture);
				}
				this->minimapWidget->setImageTexture("");
				MyGUI::Gui::getInstancePtr()->destroyWidget(this->minimapWidget);
				this->minimapWidget = nullptr;
			}
			if (nullptr != this->fogOfWarWidget)
			{
				// Must be done, because mygui also holds a reference to the Ogre::TextureGpu texture.
				auto myGuiTexture = MyGUI::RenderManager::getInstancePtr()->getTexture(this->fogOfWarTexture->getNameStr());
				if (nullptr != myGuiTexture)
				{
					static_cast<MyGUI::Ogre2RenderManager*>(MyGUI::RenderManager::getInstancePtr())->removeTexture(myGuiTexture);
				}
				this->fogOfWarWidget->setImageTexture("");
				MyGUI::Gui::getInstancePtr()->destroyWidget(this->fogOfWarWidget);
				this->fogOfWarWidget = nullptr;
			}

			// Is deleted via destroy above 
			/*if (nullptr != this->minimapTexture)
			{
				this->textureManager->destroyTexture(this->minimapTexture);
				this->minimapTexture = nullptr;
			}
			if (nullptr != this->fogOfWarTexture)
			{
				this->textureManager->destroyTexture(this->fogOfWarTexture);
				this->fogOfWarTexture = nullptr;
			}*/

			if (nullptr != this->fogOfWarStagingTexture)
			{
				this->textureManager->removeStagingTexture(this->fogOfWarStagingTexture);
				this->fogOfWarStagingTexture = nullptr;
			}
		
			this->workspace = nullptr;
			this->externalChannels.clear();
		}
	}

	void MinimapComponent::adjustMinimapCamera(void)
	{
		// Calculate world dimensions
		Ogre::Real worldWidth = this->maximumBounds.x - this->minimumBounds.x;
		Ogre::Real worldDepth = this->maximumBounds.z - this->minimumBounds.z;
		Ogre::Real maxDimension = std::max(worldWidth, worldDepth);

		// If camera is orthographic, in order to see the whole scene, the orthographic window size must be adapted, to have minimum and maximum bounds!
		// Default is 10 10 meters, but e.g. if plane is 50 50, the ortho window also must get that size
		if (true == this->cameraComponent->getIsOrthographic())
		{
			this->cameraComponent->setOrthoWindowSize(Ogre::Vector2(worldWidth, worldDepth));
		}

		// Calculate camera height
		Ogre::Real fovY = this->cameraComponent->getCamera()->getFOVy().valueRadians();  // Vertical FOV in radians
		Ogre::Real aspectRatio = this->cameraComponent->getCamera()->getAspectRatio();  // Aspect ratio of the viewport
		Ogre::Real halfWidth = maxDimension / 2.0f;
		Ogre::Real halfHeight = halfWidth / aspectRatio;
		Ogre::Real cameraHeight = halfHeight / tan(fovY / 2.0f);

		// Position and orient the camera
		Ogre::Vector3 worldCenter = (this->minimumBounds + this->maximumBounds) / 2.0f;
		
		this->cameraComponent->setCameraPosition(Ogre::Vector3(worldCenter.x, cameraHeight, worldCenter.z));
		this->cameraComponent->setCameraDegreeOrientation(Ogre::Vector3(-90.0f, 90.0f, -90.0f));
		/*
		if (this->defaultDirection == Ogre::Vector3::NEGATIVE_UNIT_Z)
		{
			this->cameraComponent->setCameraDegreeOrientation(Ogre::Vector3(-90.0f, 90.0f, -90.0f));
		}
		else if (this->defaultDirection == Ogre::Vector3::UNIT_Z)
		{
			this->cameraComponent->setCameraDegreeOrientation(Ogre::Vector3(-90.0f, 90.0f, 90.0f));
		}
		else if (this->defaultDirection == Ogre::Vector3::UNIT_X)
		{
			this->cameraComponent->setCameraDegreeOrientation(Ogre::Vector3(-90.0f, 90.0f, 180.0f));
		}
		else if (this->defaultDirection == Ogre::Vector3::NEGATIVE_UNIT_X)
		{
			this->cameraComponent->setCameraDegreeOrientation(Ogre::Vector3(-90.0f, 90.0f, -180.0f));
		}*/
	}

	void MinimapComponent::updateMinimapCamera(const Ogre::Vector3& targetPosition, const Ogre::Quaternion& targetOrientation)
	{
		// Set the camera position above the scene node

		// Calculate the distance from the camera to the ground
		Ogre::Real distance = targetPosition.y + this->cameraHeight->getReal();
		this->cameraComponent->setCameraPosition(Ogre::Vector3(targetPosition.x, distance, targetPosition.z));

		// Orient the camera to look down at the scene node
		this->cameraComponent->setCameraDegreeOrientation(Ogre::Vector3(-90.0f, 90.0f, -90.0f));

		if (true == this->rotateMinimap->getBool())
		{
			Ogre::Real smoothedAngle = this->filter.update(targetOrientation.getYaw().valueDegrees());
			Ogre::Quaternion yAxisQuat(Ogre::Degree(smoothedAngle), Ogre::Vector3::UNIT_Y);

			// Create the local orientation quaternion
			Ogre::Degree yRad(90.0f);
			Ogre::Degree pRad(-90.0f);
			Ogre::Degree rRad(-90.0f);
			Ogre::Matrix3 localOrientationMat;
			localOrientationMat.FromEulerAnglesYXZ(Ogre::Radian(yRad.valueRadians()), Ogre::Radian(pRad.valueRadians()), Ogre::Radian(rRad.valueRadians()));
			Ogre::Quaternion localOrientationQuat;
			localOrientationQuat.FromRotationMatrix(localOrientationMat);

			// Combine the y-axis rotation with the local orientation
			Ogre::Quaternion finalOrientation = yAxisQuat * localOrientationQuat;
			this->cameraComponent->setCameraOrientation(finalOrientation);
		}

		if (true == this->cameraComponent->getIsOrthographic())
		{
			// Get the FOV and aspect ratio of the minimap camera
			Ogre::Real fovY = this->cameraComponent->getCamera()->getFOVy().valueRadians();
			Ogre::Real aspectRatio = this->cameraComponent->getCamera()->getAspectRatio();

			// Calculate the view height at the ground level
			Ogre::Real viewHeight = 2.0f * distance * tan(fovY / 2.0f);
			// Calculate the view width using the aspect ratio
			Ogre::Real viewWidth = viewHeight * aspectRatio;

			// Set the orthographic size of the minimap camera
			this->cameraComponent->setOrthoWindowSize(Ogre::Vector2(viewWidth, viewHeight));
		}
	}

	bool MinimapComponent::saveDiscoveryState(void)
	{
		Ogre::String existingMinimapDiscoveryFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + Core::getSingletonPtr()->getSceneName() + "_mData.bin";
		std::ofstream outFile(existingMinimapDiscoveryFilePathName, std::ios::binary);
		if (!outFile)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MinimapComponent] Failed to open file for writing: " + existingMinimapDiscoveryFilePathName + " for game object: " + this->gameObjectPtr->getName());
			return false;
		}

		// Save dimensions
		int rows = this->discoveryState.size();
		int cols = this->discoveryState.empty() ? 0 : this->discoveryState[0].size();
		outFile.write(reinterpret_cast<const char*>(&rows), sizeof(rows));
		outFile.write(reinterpret_cast<const char*>(&cols), sizeof(cols));

		for (const auto& row : this->discoveryState)
		{
			for (bool state : row)
			{
				outFile.write(reinterpret_cast<const char*>(&state), sizeof(state));
			}
		}

		outFile.close();
		return true;
	}

	bool MinimapComponent::loadDiscoveryState(void)
	{
		Ogre::String existingMinimapDiscoveryFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + Core::getSingletonPtr()->getSceneName() + "_mData.bin";
		std::ifstream inFile(existingMinimapDiscoveryFilePathName, std::ios::binary);
		if (!inFile)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MinimapComponent] Failed to open file for reading: " + existingMinimapDiscoveryFilePathName + " for game object: " + this->gameObjectPtr->getName());
			return false;
		}

		// Load dimensions
		int rows, cols;
		inFile.read(reinterpret_cast<char*>(&rows), sizeof(rows));
		inFile.read(reinterpret_cast<char*>(&cols), sizeof(cols));

		// Resize the discovery state array
		this->discoveryState.resize(rows, std::vector<bool>(cols, false));

		// Load data
		for (int i = 0; i < rows; ++i)
		{
			for (int j = 0; j < cols; ++j)
			{
				bool state;
				inFile.read(reinterpret_cast<char*>(&state), sizeof(state));
				this->discoveryState[i][j] = state;
			}
		}

		inFile.close();

		return true;
	}

	Ogre::TextureGpu* MinimapComponent::getMinimapTexture(void) const
	{
		return this->minimapTexture;
	}

	void MinimapComponent::deleteGameObjectDelegate(EventDataPtr eventData)
	{
		boost::shared_ptr<EventDataDeleteGameObject> castEventData = boost::static_pointer_cast<NOWA::EventDataDeleteGameObject>(eventData);
		unsigned long id = castEventData->getGameObjectId();
		if (nullptr != this->targetGameObject)
		{
			if (id == this->targetGameObject->getId())
			{
				this->targetGameObject = nullptr;
			}
		}
	}

	void MinimapComponent::handleUpdateBounds(NOWA::EventDataPtr eventData)
	{
		boost::shared_ptr<NOWA::EventDataBoundsUpdated> castEventData = boost::static_pointer_cast<EventDataBoundsUpdated>(eventData);
		this->minimumBounds = castEventData->getCalculatedBounds().first;
		this->maximumBounds = castEventData->getCalculatedBounds().second;

		this->minimumBounds.y = 0.0f;
		this->maximumBounds.y = 0.0f;
	}

	void MinimapComponent::handleGeometryChanged(EventDataPtr eventData)
	{
		if (nullptr == this->heightMapTexture)
		{
			return;
		}

		boost::shared_ptr<EventDataGeometryChanged> castEventData = boost::static_pointer_cast<EventDataGeometryChanged>(eventData);

		if (true == castEventData->getIsHeightMapChanged())
		{
#if 0
			this->updateHeightMapTexture(this->heightMapTexture, this->minimapGeometry->getVector4().z, this->minimapGeometry->getVector4().w);
#endif
		}
	}

	void MinimapComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (MinimapComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (MinimapComponent::AttrTargetId() == attribute->getName())
		{
			this->setTargetId(attribute->getULong());
		}
		else if (MinimapComponent::AttrTextureSize() == attribute->getName())
		{
			this->setTextureSize(attribute->getUInt());
		}
		else if (MinimapComponent::AttrMinimapGeometry() == attribute->getName())
		{
			this->setMinimapGeometry(attribute->getVector4());
		}
		else if (MinimapComponent::AttrTransparent() == attribute->getName())
		{
			this->setTransparent(attribute->getBool());
		}
		else if(MinimapComponent::AttrUseFogOfWar() == attribute->getName())
		{
			this->setUseFogOfWar(attribute->getBool());
		}
		else if (MinimapComponent::AttrUseDiscovery() == attribute->getName())
		{
			this->setUseDiscovery(attribute->getBool());
		}
		else if (MinimapComponent::AttrPersistDiscovery() == attribute->getName())
		{
			this->setPersistDiscovery(attribute->getBool());
		}
		else if (MinimapComponent::AttrVisibilityRadius() == attribute->getName())
		{
			this->setVisibilityRadius(attribute->getReal());
		}
		else if (MinimapComponent::AttrUseVisibilitySpotlight() == attribute->getName())
		{
			this->setUseVisibilitySpotlight(attribute->getBool());
		}
		else if (MinimapComponent::AttrWholeSceneVisible() == attribute->getName())
		{
			this->setWholeSceneVisible(attribute->getBool());
		}
		else if (MinimapComponent::AttrCameraHeight() == attribute->getName())
		{
			this->setCameraHeight(attribute->getReal());
		}
		else if (MinimapComponent::AttrMinimapMask() == attribute->getName())
		{
			this->setMinimapMask(attribute->getString());
		}
		else if (MinimapComponent::AttrUseRoundMinimap() == attribute->getName())
		{
			this->setUseRoundMinimap(attribute->getBool());
		}
		else if (MinimapComponent::AttrRotateMinimap() == attribute->getName())
		{
			this->setRotateMinimap(attribute->getBool());
		}
	}

	void MinimapComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TargetId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->targetId->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TextureSize"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->textureSize->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MinimapGeometry"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->minimapGeometry->getVector4())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Transparent"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->transparent->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UseFogOfWar"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useFogOfWar->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UseDiscovery"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useDiscovery->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "PersistDiscovery"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->persistDiscovery->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "VisibilityRadius"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->visibilityRadius->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UseVisibilitySpotlight"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useVisibilitySpotlight->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "WholeSceneVisible"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wholeSceneVisible->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CameraHeight"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->cameraHeight->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MinimapMask"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->minimapMask->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UseRoundMinimap"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useRoundMinimap->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RotateMinimap"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rotateMinimap->getBool())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String MinimapComponent::getClassName(void) const
	{
		return "MinimapComponent";
	}

	Ogre::String MinimapComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void MinimapComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
		if (true == activated)
		{
			this->setupMinimapWithFogOfWar();
		}
		else
		{
			if (nullptr != this->minimapWidget)
			{
				auto minimapWidget = this->minimapWidget;
				auto maskWidget = this->maskWidget;

				ENQUEUE_RENDER_COMMAND_MULTI("MinimapComponent::setActivated maskWidget", _2(minimapWidget, maskWidget),
				{
					if (minimapWidget)
					{
						minimapWidget->setVisible(false);
						if (maskWidget)
						{
							maskWidget->setVisible(false);
						}
					}
				});
			}
			if (nullptr != this->fogOfWarWidget)
			{
				auto fogOfWarWidget = this->fogOfWarWidget;

				ENQUEUE_RENDER_COMMAND_MULTI("MinimapComponent::setActivated fogOfWarWidget", _1(fogOfWarWidget),
				{
					if (fogOfWarWidget)
					{
						fogOfWarWidget->setVisible(false);
					}
				});
			}
		}
	}

	bool MinimapComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void MinimapComponent::setTargetId(unsigned long targetId)
	{
		this->targetId->setValue(targetId);
	}

	unsigned long MinimapComponent::getTargetId(void) const
	{
		return this->targetId->getULong();
	}

	GameObject* MinimapComponent::getTargetGameObject(void) const
	{
		return this->targetGameObject;
	}

	void MinimapComponent::setTextureSize(unsigned int textureSize)
	{
		this->textureSize->setValue(textureSize);
	}

	unsigned int MinimapComponent::getTextureSize(void) const
	{
		return this->textureSize->getUInt();
	}

	void MinimapComponent::setMinimapGeometry(const Ogre::Vector4& minimapGeometry)
	{
		this->minimapGeometry->setValue(minimapGeometry);
	}

	Ogre::Vector4 MinimapComponent::getMinimapGeometry(void) const
	{
		return this->minimapGeometry->getVector4();
	}

	void MinimapComponent::setTransparent(bool transparent)
	{
		this->transparent->setValue(transparent);
	}

	bool MinimapComponent::getTransparent(void) const
	{
		return this->transparent->getBool();
	}

	void MinimapComponent::setUseFogOfWar(bool useFogOfWar)
	{
		if (true == this->wholeSceneVisible->getBool())
		{
			this->useFogOfWar->setValue(useFogOfWar);
		}

		this->wholeSceneVisible->setVisible(false == this->useFogOfWar->getBool());
		this->useDiscovery->setVisible(true == this->useFogOfWar->getBool());
		this->persistDiscovery->setVisible(true == this->useFogOfWar->getBool());
		this->visibilityRadius->setVisible(true == this->useFogOfWar->getBool());
		this->useVisibilitySpotlight->setVisible(true == this->useFogOfWar->getBool());
		this->cameraHeight->setVisible(false == this->wholeSceneVisible->getBool());
		this->rotateMinimap->setVisible(false == this->wholeSceneVisible->getBool());
		
		if (false == useFogOfWar)
		{
			this->useDiscovery->setValue(false);
			this->persistDiscovery->setValue(false);
		}
	}

	bool MinimapComponent::getUseFogOfWar(void) const
	{
		return this->useFogOfWar->getBool();
	}

	void MinimapComponent::setUseDiscovery(bool useDiscovery)
	{
		if (true == this->useFogOfWar->getBool())
		{
			this->useDiscovery->setValue(useDiscovery);
		}
	}

	bool MinimapComponent::getUseDiscovery(void) const
	{
		return this->useDiscovery->getBool();
	}

	void MinimapComponent::setPersistDiscovery(bool persistDiscovery)
	{
		if (true == this->useFogOfWar->getBool() && this->useDiscovery->getBool())
		{
			this->persistDiscovery->setValue(persistDiscovery);
		}
	}

	bool MinimapComponent::getPersistDiscovery(void) const
	{
		return this->persistDiscovery->getBool();
	}

	void MinimapComponent::setVisibilityRadius(Ogre::Real visibilityRadius)
	{
		if (visibilityRadius < 0.0f)
		{
			visibilityRadius = 0.0f;
		}
		if (true == this->useFogOfWar->getBool())
		{
			this->visibilityRadius->setValue(visibilityRadius);
		}
	}

	Ogre::Real MinimapComponent::getVisibilityRadius(void) const
	{
		return this->visibilityRadius->getReal();
	}

	void MinimapComponent::setUseVisibilitySpotlight(bool useVisibilitySpotlight)
	{
		if (true == this->useFogOfWar->getBool())
		{
			this->useVisibilitySpotlight->setValue(useVisibilitySpotlight);
		}
	}

	bool MinimapComponent::getUseVisibilitySpotlight(void) const
	{
		return this->useVisibilitySpotlight->getBool();
	}

	void MinimapComponent::setWholeSceneVisible(bool wholeSceneVisible)
	{
		this->wholeSceneVisible->setValue(wholeSceneVisible);

		this->useFogOfWar->setVisible(true == this->wholeSceneVisible->getBool());
		this->useDiscovery->setVisible(true == this->useFogOfWar->getBool());
		this->persistDiscovery->setVisible(true == this->useFogOfWar->getBool());
		this->visibilityRadius->setVisible(true == this->useFogOfWar->getBool());
		this->useVisibilitySpotlight->setVisible(true == this->useFogOfWar->getBool());
		this->cameraHeight->setVisible(false == this->wholeSceneVisible->getBool());
		this->rotateMinimap->setVisible(false == this->wholeSceneVisible->getBool());

		if (false == wholeSceneVisible)
		{
			this->useFogOfWar->setValue(false);
			this->useDiscovery->setValue(false);
			this->persistDiscovery->setValue(false);
			this->visibilityRadius->setValue(false);
			this->useVisibilitySpotlight->setValue(false);
		}
	}

	bool MinimapComponent::getWholeSceneVisible(void) const
	{
		return this->wholeSceneVisible->getBool();
	}

	void MinimapComponent::setCameraHeight(Ogre::Real cameraHeight)
	{
		if (cameraHeight < 2.0f)
		{
			cameraHeight = 2.0f;
		}

		this->cameraHeight->setValue(cameraHeight);
	}

	Ogre::Real MinimapComponent::getCameraHeight(void) const
	{
		return this->cameraHeight->getReal();
	}

	void MinimapComponent::setMinimapMask(const Ogre::String& minimapMask)
	{
		this->minimapMask->setValue(minimapMask);
	}

	Ogre::String MinimapComponent::getMinimapMask(void) const
	{
		return minimapMask->getString();
	}

	void MinimapComponent::setUseRoundMinimap(bool useRoundMinimap)
	{
		this->useRoundMinimap->setValue(useRoundMinimap);
	}

	bool MinimapComponent::getUseRoundMinimap(void) const
	{
		return this->useRoundMinimap->getBool();
	}

	void MinimapComponent::setRotateMinimap(bool rotateMinimap)
	{
		if (false == this->wholeSceneVisible->getBool())
		{
			this->rotateMinimap->setValue(rotateMinimap);
		}
	}

	bool MinimapComponent::getRotateMinimap(void) const
	{
		return this->rotateMinimap->getBool();
	}

	void MinimapComponent::clearDiscoveryState(void)
	{
		this->discoveryState.clear();
		this->discoveryState.resize((this->textureSize->getUInt()), std::vector<bool>(this->textureSize->getUInt(), false));

		if (true == this->persistDiscovery->getBool())
		{
			this->saveDiscoveryState();
		}
	}

	// Lua registration part

	MinimapComponent* getMinimapComponentFromIndex(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<MinimapComponent>(gameObject->getComponentWithOccurrence<MinimapComponent>(occurrenceIndex)).get();
	}

	MinimapComponent* getMinimapComponent(GameObject* gameObject)
	{
		return makeStrongPtr<MinimapComponent>(gameObject->getComponent<MinimapComponent>()).get();
	}

	MinimapComponent* getMinimapComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<MinimapComponent>(gameObject->getComponentFromName<MinimapComponent>(name)).get();
	}

	void MinimapComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<MinimapComponent, GameObjectComponent>("MinimapComponent")
			.def("setActivated", &MinimapComponent::setActivated)
			.def("isActivated", &MinimapComponent::isActivated)
			.def("setVisibilityRadius", &MinimapComponent::setVisibilityRadius)
			.def("getVisibilityRadius", &MinimapComponent::getVisibilityRadius)
			.def("clearDiscoveryState", &MinimapComponent::clearDiscoveryState)
			.def("saveDiscoveryState", &MinimapComponent::saveDiscoveryState)
			.def("loadDiscoveryState", &MinimapComponent::loadDiscoveryState)
			.def("setCameraHeight", &MinimapComponent::setCameraHeight)
			.def("getCameraHeight", &MinimapComponent::getCameraHeight)
		];

		LuaScriptApi::getInstance()->addClassToCollection("MinimapComponent", "class inherits GameObjectComponent", MinimapComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("MinimapComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
		LuaScriptApi::getInstance()->addClassToCollection("MinimapComponent", "bool isActivated()", "Gets whether this component is activated.");
		LuaScriptApi::getInstance()->addClassToCollection("MinimapComponent", "void setVisibilityRadius(number visibilityRadius)", "If fog of war is used, sets the visibilty radius which discovers the fog of war.");
		LuaScriptApi::getInstance()->addClassToCollection("MinimapComponent", "number getVisibilityRadius()", "Gets the visibilty radius which discovers the fog of war.");
		LuaScriptApi::getInstance()->addClassToCollection("MinimapComponent", "void clearDiscoveryState()", "Clears the area which has been already discovered and saves the state.");
		LuaScriptApi::getInstance()->addClassToCollection("MinimapComponent", "void saveDiscoveryState()", "Saves the area which has been already discovered.");
		LuaScriptApi::getInstance()->addClassToCollection("MinimapComponent", "void loadDiscoveryState()", "Loads the area which has been already discovered.");
		LuaScriptApi::getInstance()->addClassToCollection("MinimapComponent", "void setCameraHeight(number cameraHeight)", "If whole scene visible is set to false, sets the camera height, which is added at the top of the y position of the target game object.");
		LuaScriptApi::getInstance()->addClassToCollection("MinimapComponent", "bool getCameraHeight()", "If whole scene visible is set to false, gets the camera height, which is added at the top of the y position of the target game object.");

		gameObjectClass.def("getMinimapComponentFromName", &getMinimapComponentFromName);
		gameObjectClass.def("getMinimapComponent", (MinimapComponent * (*)(GameObject*)) & getMinimapComponent);
		// If its desired to create several of this components for one game object
		gameObjectClass.def("getMinimapComponentFromIndex", (MinimapComponent * (*)(GameObject*, unsigned int)) & getMinimapComponentFromIndex);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "MinimapComponent getMinimapComponentFromIndex(unsigned int occurrenceIndex)", "Gets the component by the given occurence index, since a game object may this component maybe several times.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "MinimapComponent getMinimapComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "MinimapComponent getMinimapComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castMinimapComponent", &GameObjectController::cast<MinimapComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "MinimapComponent castMinimapComponent(MinimapComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool MinimapComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// Can only be added once
		auto minimapCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<MinimapComponent>());
		if (nullptr != minimapCompPtr)
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