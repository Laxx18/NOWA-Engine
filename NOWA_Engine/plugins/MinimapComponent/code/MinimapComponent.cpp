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
// #include "OgreAsyncTextureTicket.h"




#include "OgreRectangle2D2.h"

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
		heightMapStagingTexture(nullptr),
		fogOfWarStagingTexture(nullptr),
		textureManager(nullptr),
		workspace(nullptr),
		minimapWidget(nullptr),
		heightMapWidget(nullptr),
		fogOfWarWidget(nullptr),
		minimumBounds(Ogre::Vector3::ZERO),
		maximumBounds(Ogre::Vector3::ZERO),
		cameraComponent(nullptr),
		terraComponent(nullptr),
		running(false),
		readyToUpdate(false),
		timeSinceLastUpdate(0.0f),
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
		hideId(new Variant(MinimapComponent::AttrHideId(), static_cast<unsigned long>(0), this->attributes)),
		useRoundMinimap(new Variant(MinimapComponent::AttrUseRoundMinimap(), false, this->attributes)),
		rotateMinimap(new Variant(MinimapComponent::AttrRotateMinimap(), false, this->attributes))
	{
		this->targetId->setDescription("Sets the target id for the game object, which shall be tracked.");
		this->textureSize->setDescription("Sets the minimap texture size in pixels. Note: The texture is quadratic. Also note: The higher the texture size, the less performant the application will run.");
		this->minimapGeometry->setDescription("Sets the geometry of the minimap relative to the window screen in the format Vector4(pos.x, pos.y, width, height).");
		this->transparent->setDescription("Sets whether the minimap shall be somewhat transparent.");
		this->useFogOfWar->setDescription("Sets whether fog of war is used.");
		this->useDiscovery->setDescription("If fog of war is used, sets whether places in which the target game object has visited remain visible.");
		this->persistDiscovery->setDescription("If fog of war is used and use discovery, sets whether places in which the target game object has visited remain visible and also stored and loaded.");
		this->visibilityRadius->setDescription("If fog of war is used, sets the visibilty radius which discovers the fog of war.");
		this->useVisibilitySpotlight->setDescription("If fog of war is used, sets a spotlight instead of rounded radius area. The spotlight is as big as the visibility radius.");
		this->wholeSceneVisible->setDescription("Sets whether the whole scene is visible. If set to true, the camera will be positioned in the middle of the whole scene and the height adapted, for complete scene rendering on the minimap. "
												" If set to false, only an area is visible and the minimap is scrolled according to the target game object position.");
		this->hideId->setDescription("Manages all game objects that will be visible on the minimap. If the hide id is set to 0, all game objects will be rendered. If set to e.g. 5, all game objects, which have its hide id set to 5 will not be rendered and are hidden on the minimap.");
		this->useRoundMinimap->setDescription("If whole scene visible is set to false, sets whether the minimap has a round geometry instead of a rectangle.");
		this->useRoundMinimap->setDescription("If whole scene visible is set to false, sets whether the minimap is rotated according to the target game object orientation.");
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
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<MinimapComponent>(MinimapComponent::getStaticClassId(), MinimapComponent::getStaticClassName());
	}
	
	void MinimapComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool MinimapComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &MinimapComponent::deleteGameObjectDelegate), EventDataDeleteGameObject::getStaticEventType());
		AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &MinimapComponent::handleUpdateBounds), EventDataBoundsUpdated::getStaticEventType());

		GameObjectComponent::init(propertyElement, filename);

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
			this->useFogOfWar->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
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
			this->wholeSceneVisible->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "HideId")
		{
			this->hideId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
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

		this->textureManager = Ogre::Root::getSingleton().getRenderSystem()->getTextureGpuManager();

		// Removes complexity, by setting default direction to -z, which is Ogre default.
		this->gameObjectPtr->setDefaultDirection(Ogre::Vector3::NEGATIVE_UNIT_Z);
		this->gameObjectPtr->getAttribute(GameObject::AttrDefaultDirection())->setVisible(false);

		return true;
	}

	bool MinimapComponent::connect(void)
	{
		const auto& targetGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->targetId->getULong());
		if (nullptr != targetGameObjectPtr)
		{
			this->targetGameObject = targetGameObjectPtr.get();
		}

		this->setActivated(this->activated->getBool());

		// Start the thread to fill the texture
		this->running = true;
		// this->fillThread = std::thread(&MinimapComponent::updateFogOfWarThreadFunc, this);
		
		return true;
	}

	bool MinimapComponent::disconnect(void)
	{
		this->targetGameObject = nullptr;
		this->cameraComponent = nullptr;
		this->terraComponent = nullptr;

		// Stop the running task
		this->running = false;

		// Notify the thread
		this->condVar.notify_all();

		// Wait for the thread to complete
		if (true == this->fillThread.joinable())
		{
			this->fillThread.join();
		}

		this->timeSinceLastUpdate = 0.0f;

		this->removeWorkspace();

		return true;
	}

	bool MinimapComponent::onCloned(void)
	{
		
		return true;
	}

	void MinimapComponent::onRemoveComponent(void)
	{
		AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &MinimapComponent::deleteGameObjectDelegate), EventDataDeleteGameObject::getStaticEventType());
		AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &MinimapComponent::handleUpdateBounds), EventDataBoundsUpdated::getStaticEventType());

		if (true == this->persistDiscovery->getBool())
		{
			this->saveDiscoveryState();
		}
	}
	
	void MinimapComponent::onOtherComponentRemoved(unsigned int index)
	{
		
	}
	
	void MinimapComponent::onOtherComponentAdded(unsigned int index)
	{
		
	}
	
	void MinimapComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
#if 1
			this->timeSinceLastUpdate += dt;

			if (this->timeSinceLastUpdate >= 0.25f)
			{
				if (nullptr != this->targetGameObject && true == this->useFogOfWar->getBool())
				{
					this->updateFogOfWarTexture(this->targetGameObject->getPosition(), this->visibilityRadius->getReal());
				}
				this->timeSinceLastUpdate = 0.0f;
			}
#else
			{
				// std::lock_guard<std::mutex> lock(this->mutex);
				this->readyToUpdate = true;
			}
			this->condVar.notify_all();
#endif
			/*
			this->workspace->_validateFinalTarget();
			this->workspace->_beginUpdate(true);
			this->workspace->_update();
			this->workspace->_endUpdate(true);
			*/
		}
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

		auto gameObjectsWithTerraComponent = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsFromComponent(TerraComponent::getStaticClassName());
		if (gameObjectsWithTerraComponent.size() > 0)
		{
			// For now support just for 1 terra
			const auto& terraCompPtr = NOWA::makeStrongPtr(gameObjectsWithTerraComponent[0]->getComponent<TerraComponent>());
			if (nullptr != terraCompPtr)
			{
				this->terraComponent = terraCompPtr.get();

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
			this->adjustMinimapCamera();
		}

		if (nullptr == this->minimapTexture)
		{
			this->minimapTexture = this->createMinimapTexture("MinimapRT", this->textureSize->getUInt(), this->textureSize->getUInt());
		}
#if 0
		if (nullptr == this->heightMapTexture && nullptr != this->terraComponent)
		{
			this->heightMapTexture = this->createHeightMapTexture("HeighMapTexture", 1024, 1024);
		}
#endif
		if (nullptr == this->fogOfWarTexture && true == this->useFogOfWar->getBool())
		{
			this->fogOfWarTexture = this->createFogOfWarTexture("FogOfWarTexture", this->textureSize->getUInt(), this->textureSize->getUInt());
		}

		if (true == this->useFogOfWar->getBool())
		{
			this->fogOfWarStagingTexture = this->textureManager->getStagingTexture(this->fogOfWarTexture->getWidth(), this->fogOfWarTexture->getHeight(), 1u, 1u, this->fogOfWarTexture->getPixelFormat());
		}

		if (true == this->useDiscovery->getBool())
		{
			this->discoveryState.resize((this->textureSize->getUInt()), std::vector<bool>(this->textureSize->getUInt(), false));
		}

		Ogre::CompositorManager2* compositorManager = WorkspaceModule::getInstance()->getCompositorManager();

		this->createMinimapNode();

		this->minimapWorkspaceName = "MinimapWorkspaceWithFoW";
		Ogre::CompositorWorkspaceDef* workspaceDef = compositorManager->addWorkspaceDefinition(this->minimapWorkspaceName);
		workspaceDef->clearAllInterNodeConnections();

		workspaceDef->connectExternal(0, this->minimapNodeName, 0);

		this->externalChannels.resize(1);
		this->externalChannels[0] = this->minimapTexture;

		this->workspace = compositorManager->addWorkspace(this->gameObjectPtr->getSceneManager(), this->externalChannels, this->cameraComponent->getCamera(), this->minimapWorkspaceName, true);

		// Create MyGUI widget for the minimap
		if (nullptr == this->minimapWidget)
		{
			Ogre::Vector4 geometry = this->minimapGeometry->getVector4();
			this->minimapWidget = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::ImageBox>("ImageBox", MyGUI::FloatCoord(geometry.x, geometry.y, geometry.z, geometry.w), MyGUI::Align::HCenter, "Overlapped");
			this->minimapWidget->setImageTexture(this->minimapTexture->getNameStr());
		}
		else
		{
			this->minimapWidget->setVisible(true);
		}

		// Create MyGUI widget for the heightmap if existing
		if (nullptr != this->heightMapTexture)
		{
			if (nullptr == this->heightMapWidget)
			{
				Ogre::Vector4 geometry = this->minimapGeometry->getVector4();
				this->heightMapWidget = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::ImageBox>("ImageBox", MyGUI::FloatCoord(geometry.x, geometry.y, geometry.z, geometry.w), MyGUI::Align::HCenter, "Overlapped");
				this->heightMapWidget->setImageTexture(this->heightMapTexture->getNameStr());
			}
			else
			{
				this->heightMapWidget->setVisible(true);
			}
		}

		
		if (true == this->useFogOfWar->getBool())
		{
			if (nullptr == this->fogOfWarWidget)
			{
				Ogre::Vector4 geometry = this->minimapGeometry->getVector4();
				this->fogOfWarWidget = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::ImageBox>("ImageBox", MyGUI::FloatCoord(geometry.x, geometry.y, geometry.z, geometry.w), MyGUI::Align::HCenter, "Overlapped");
				this->fogOfWarWidget->setImageTexture(this->fogOfWarTexture->getNameStr());
			}
			else
			{
				this->fogOfWarWidget->setVisible(true);
			}
		}
	}


	Ogre::TextureGpu* MinimapComponent::createMinimapTexture(const Ogre::String& name, unsigned int width, unsigned int height)
	{
		Ogre::TextureGpu* texture = nullptr;
		if (false == this->transparent->getBool())
		{
			texture = this->textureManager->createOrRetrieveTexture(name, Ogre::GpuPageOutStrategy::SaveToSystemRam, Ogre::TextureFlags::RenderToTexture, Ogre::TextureTypes::Type2D);
			texture->setResolution(width, height);
			texture->setPixelFormat(Ogre::PFG_RGBA8_UNORM_SRGB);
			texture->setNumMipmaps(1u);
			texture->_transitionTo(Ogre::GpuResidency::Resident, (Ogre::uint8*)0);
		}
		else
		{
			texture = this->textureManager->createOrRetrieveTexture(name, Ogre::GpuPageOutStrategy::Discard, Ogre::TextureFlags::RenderToTexture | Ogre::TextureFlags::AllowAutomipmaps, Ogre::TextureTypes::Type2D);
			texture->setResolution(width, height);
			texture->scheduleTransitionTo(Ogre::GpuResidency::OnStorage);

			texture->setNumMipmaps(Ogre::PixelFormatGpuUtils::getMaxMipmapCount(width, height));
			texture->setPixelFormat(Ogre::PFG_RGBA8_UNORM_SRGB);
			texture->_transitionTo(Ogre::GpuResidency::Resident, (Ogre::uint8*)0);
		}
		return texture;
	}
#if 0
	Ogre::TextureGpu* MinimapComponent::createHeightMapTexture(const Ogre::String& name, unsigned int width, unsigned int height)
	{
		Ogre::TextureGpu* texture = this->textureManager->createOrRetrieveTexture(name, Ogre::GpuPageOutStrategy::SaveToSystemRam, TextureFlags::RenderToTexture, Ogre::TextureTypes::Type2D);
		texture->setResolution(width, height);
		// texture->setPixelFormat(Ogre::PFG_RGBA8_UNORM_SRGB);
		texture->setPixelFormat(Ogre::PFG_R16_UNORM);
		texture->setNumMipmaps(1u);
		texture->_transitionTo(Ogre::GpuResidency::Resident, (Ogre::uint8*)0);

		texture = this->updateHeightMapTexture(texture, width, height);

		return texture;
	}
#endif
	Ogre::TextureGpu* MinimapComponent::createFogOfWarTexture(const Ogre::String& name, unsigned int width, unsigned int height)
	{
		Ogre::TextureGpu* texture = this->textureManager->createOrRetrieveTexture(name, Ogre::GpuPageOutStrategy::SaveToSystemRam, Ogre::TextureFlags::RenderToTexture, Ogre::TextureTypes::Type2D);
		texture->setResolution(width, height);
		texture->setPixelFormat(Ogre::PFG_RGBA8_UNORM_SRGB);
		texture->setNumMipmaps(1u);
		texture->_transitionTo(Ogre::GpuResidency::Resident, (Ogre::uint8*)0);
		return texture;
	}

	void MinimapComponent::createMinimapNode(void)
	{
		Ogre::CompositorManager2* compositorManager = WorkspaceModule::getInstance()->getCompositorManager();

		this->minimapNodeName = "MinimapNode";
		Ogre::CompositorNodeDef* nodeDef = compositorManager->addNodeDefinition(this->minimapNodeName);

		nodeDef->addTextureSourceName("MinimapRT", 0, Ogre::TextureDefinitionBase::TEXTURE_INPUT);

		nodeDef->setNumTargetPass(1);
		{
			Ogre::CompositorTargetDef* targetDef = nodeDef->addTargetPass("MinimapRT");
			targetDef->setNumPasses(2);
			{
				// Clear Pass
				{
					Ogre::CompositorPassSceneDef* passClear;
					passClear = static_cast<Ogre::CompositorPassSceneDef*>(targetDef->addPass(Ogre::PASS_CLEAR));

					passClear->mClearColour[0] = Ogre::ColourValue(0.2f, 0.4f, 0.6f);
					// passClear->setAllStoreActions(Ogre::StoreAction::Store);
					passClear->mStoreActionColour[0] = Ogre::StoreAction::Store;
					passClear->mStoreActionDepth = Ogre::StoreAction::Store;
					passClear->mStoreActionStencil = Ogre::StoreAction::Store;
				}

				{
					// Scene pass for minimap
					Ogre::CompositorPassSceneDef* passScene = static_cast<Ogre::CompositorPassSceneDef*>(
						targetDef->addPass(Ogre::PASS_SCENE));
					passScene->mCameraName = this->cameraComponent->getCamera()->getName();

					passScene->setAllLoadActions(Ogre::LoadAction::Clear);
					passScene->mClearColour[0] = Ogre::ColourValue::Black;

					passScene->mStoreActionColour[0] = Ogre::StoreAction::StoreOrResolve;
					passScene->mStoreActionDepth = Ogre::StoreAction::DontCare;
					passScene->mStoreActionStencil = Ogre::StoreAction::DontCare;

					// If set, nothing is visible somehow from terra
					// passScene->mVisibilityMask = this->hideId->getULong(); // Custom mask for minimap objects

					passScene->mIncludeOverlays = false;

				}
			}
		}
		nodeDef->mapOutputChannel(0, "MinimapRT");
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

	void MinimapComponent::updateFogOfWarTexture(const Ogre::Vector3& position, Ogre::Real radius)
	{
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

		/*if (this->fogOfWarTexture->getResidencyStatus() != Ogre::GpuResidency::Resident)
		{
			this->fogOfWarTexture->_transitionTo(Ogre::GpuResidency::Resident, (Ogre::uint8*)0);
			this->fogOfWarTexture->_setNextResidencyStatus(Ogre::GpuResidency::Resident);
		}*/

		this->fogOfWarStagingTexture->startMapRegion();

		Ogre::TextureBox texBox = this->fogOfWarStagingTexture->mapRegion(this->fogOfWarTexture->getWidth(), this->fogOfWarTexture->getHeight(), 1u, 1u, this->fogOfWarTexture->getPixelFormat());

		// Assuming a 2D texture, the z-coordinate will be 0
		size_t z = 0;

		Ogre::Vector2 normalizedPosition = normalizePosition(position);
		Ogre::Vector2 textureCoordinates = mapToTextureCoordinates(normalizedPosition, texSize, texSize);

		const size_t bytesPerPixel = texBox.bytesPerPixel;

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

					if (distance < radiusInPixels)
					{
						const size_t dstIdx = x * bytesPerPixel;
						float rgba[4];
						// Create blank texture without details
						rgba[0] = 255.0f;
						rgba[1] = 0.0f;
						rgba[2] = 0.0f;
						rgba[3] = 0.0f;
						PixelFormatGpuUtils::packColour(rgba, this->fogOfWarTexture->getPixelFormat(), &pixBoxData[dstIdx]);
					}
					else
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

					if (distance < radiusInPixels || true == this->discoveryState[x][y])
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
								float rgba[4];
								// Create blank texture without details
								rgba[0] = 255.0f;
								rgba[1] = 0.0f;
								rgba[2] = 0.0f;
								rgba[3] = 0.0f;
								PixelFormatGpuUtils::packColour(rgba, this->fogOfWarTexture->getPixelFormat(), &pixBoxData[dstIdx]);
							}
						}
						else
						{
							this->discoveryState[x][y] = true;
							const size_t dstIdx = x * bytesPerPixel;
							float rgba[4];
							// Create blank texture without details
							rgba[0] = 255.0f;
							rgba[1] = 0.0f;
							rgba[2] = 0.0f;
							rgba[3] = 0.0f;
							PixelFormatGpuUtils::packColour(rgba, this->fogOfWarTexture->getPixelFormat(), &pixBoxData[dstIdx]);
						}
					}
					else
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
			}
		}

		this->fogOfWarStagingTexture->stopMapRegion();
		this->fogOfWarStagingTexture->upload(texBox, this->fogOfWarTexture, 0, 0, 0);

		// this->fogOfWarTexture->notifyDataIsReady();
	}
	void MinimapComponent::updateFogOfWarThreadFunc(void)
	{
		if (nullptr != this->targetGameObject && true == this->useFogOfWar->getBool())
		{
			while (true == this->running)
			{
				std::unique_lock<std::mutex> lock(this->mutex);
				this->condVar.wait(lock, [this]() { return this->readyToUpdate || !this->running; });

				if (false == this->running)
				{
					break;
				}

				this->updateFogOfWarTexture(this->targetGameObject->getPosition(), this->visibilityRadius->getReal());

				std::this_thread::sleep_for(std::chrono::milliseconds(16)); // Sleep for 16ms to simulate 60fps update rate
				this->readyToUpdate = false;
			}
		}
	}

#if 0
	Ogre::TextureGpu* MinimapComponent::updateHeightMapTexture(Ogre::TextureGpu* texture, unsigned int width, unsigned int height)
	{
		const auto originalHeightMapTexture = this->terraComponent->getTerra()->getHeightMapTex();

		Ogre::AsyncTextureTicket* asyncTicket = textureManager->createAsyncTextureTicket(originalHeightMapTexture->getWidth(), originalHeightMapTexture->getHeight(), 1, originalHeightMapTexture->getTextureType(), originalHeightMapTexture->getPixelFormat());

		asyncTicket->download(originalHeightMapTexture, 0, true);

		this->heightMapStagingTexture = this->textureManager->getStagingTexture(width, height, 1u, 1u, texture->getPixelFormat());

		this->heightMapStagingTexture->startMapRegion();
		Ogre::TextureBox uploadTexBox = this->heightMapStagingTexture->mapRegion(width, height, 1u, 1u, texture->getPixelFormat());

		const TextureBox srcBox = asyncTicket->map(static_cast<uint32>(0));
		asyncTicket->unmap();

		memcpy(uploadTexBox.data, srcBox.data, this->heightMapStagingTexture->_getSizeBytes());
		this->heightMapStagingTexture->stopMapRegion();
			
		// Finalize the upload
		this->heightMapStagingTexture->upload(uploadTexBox, texture, 0, 0, 0);

		textureManager->destroyAsyncTextureTicket(asyncTicket);

		Ogre::TextureGpu* finalTexture = this->textureManager->createOrRetrieveTexture(name, Ogre::GpuPageOutStrategy::SaveToSystemRam, TextureFlags::RenderToTexture, Ogre::TextureTypes::Type2D);
		finalTexture->setResolution(width, height);
		finalTexture->setPixelFormat(Ogre::PFG_RGBA8_UNORM_SRGB);
		finalTexture->setNumMipmaps(1u);
		finalTexture->_transitionTo(Ogre::GpuResidency::Resident, (Ogre::uint8*)0);


		this->heightMapStagingTexture->startMapRegion();

		Ogre::TextureBox texBox = this->heightMapStagingTexture->mapRegion(1024, 1024, 1u, 1u, texture->getPixelFormat());
		const size_t bytesPerPixel = texBox.bytesPerPixel;

		for (uint32 y = 0; y < 1024; y++)
		{
			uint8* RESTRICT_ALIAS pixBoxData = reinterpret_cast<uint8 * RESTRICT_ALIAS>(texBox.at(0, y, 0));
			for (uint32 x = 0; x < 1024; x++)
			{
				const size_t srcIdx = x * bytesPerPixel;

				auto height = pixBoxData[srcIdx];

				//float ra[2];
				////// Create blank texture without details
				//ra[0] = height;
				//ra[1] = height;
				//PixelFormatGpuUtils::packColour(ra, texture->getPixelFormat(), &pixBoxData[dstIdx]);

				const size_t dstIdx = x * 4;
				float rgba[4];
				// Create blank texture without details
				rgba[0] = height; // R
				rgba[1] = height; // G
				rgba[2] = height; // B
				rgba[3] = height; // (normPixelValue < threshold) ? 0 : 255; // A
				PixelFormatGpuUtils::packColour(rgba, finalTexture->getPixelFormat(), &pixBoxData[dstIdx]);

			}
		}

		this->heightMapStagingTexture->stopMapRegion();
		this->heightMapStagingTexture->upload(texBox, finalTexture, 0, 0, 0);

		this->textureManager->destroyTexture(texture);

		return finalTexture;
	}
#endif
	//void MinimapComponent::updateHeightMapTexture(Ogre::TextureGpu* texture, unsigned int width, unsigned int height)
	//{
	//	const auto originalHeightMapTexture = this->terraComponent->getTerra()->getHeightMapTex();

	//	auto originalStagingTexture = this->textureManager->getStagingTexture(originalHeightMapTexture->getWidth(), originalHeightMapTexture->getHeight(), 1u, 1u, originalHeightMapTexture->getPixelFormat());

	//	auto pixelFormat = texture->getPixelFormat();

	//	// Download the original heightmap texture to CPU
	//	originalStagingTexture->startMapRegion();

	//	Ogre::TextureBox origTexBox = originalStagingTexture->mapRegion(originalHeightMapTexture->getWidth(), originalHeightMapTexture->getHeight(), 1u, 1u, originalHeightMapTexture->getPixelFormat());
	//	
	//	float fBpp = (float)(PixelFormatGpuUtils::getBytesPerPixel(originalHeightMapTexture->getPixelFormat()) << 3u);
	//	const float maxValue = powf(2.0f, fBpp) - 1.0f;
	//	float invMaxValue = 1.0f / maxValue;

	//	const size_t srcBytesPerPixel = origTexBox.bytesPerPixel;
	//	const size_t destBytesPerPixel = 4;

	//	int scaleFactor = originalHeightMapTexture->getWidth() / width;

	//	std::vector<uint8_t> resizedData(width * height * 4, 0);
	//	for (uint32 y = 0; y < height; y++)
	//	{
	//		uint16* RESTRICT_ALIAS data = reinterpret_cast<uint16 * RESTRICT_ALIAS>(origTexBox.at(0, y, 0));
	//		for (uint32 x = 0; x < width; x++)
	//		{
	//			// size_t srcX = x * scaleFactor * bytesPerPixel; // Scale down by 4
	//			// size_t srcY = y * scaleFactor * bytesPerPixel; // Scale down by 4

	//			// uint8* RESTRICT_ALIAS srcPixData = reinterpret_cast<uint8 * RESTRICT_ALIAS>(&data[srcY * 1024u + srcX]);
	//			// auto height = (*srcPixData * invMaxValue) * 255.0f;

	//			// dstIndex = (x * width + y);
	//			// const size_t dstIndex = x * scaleFactor * bytesPerPixel;

	//			// const size_t srcIdx = x * srcBytesPerPixel * scaleFactor;

	//			// const size_t srcIdx = ((y * scaleFactor * width) + (x * scaleFactor)) /** srcBytesPerPixel*/;

	//			// const size_t dstIdx = x /** destBytesPerPixel*/;

	//			uint32 srcX = x * scaleFactor; // Scale down by 4
	//			uint32 srcY = y * scaleFactor;
	//			const size_t srcIdx = srcY * (width * scaleFactor) + srcX;

	//			const size_t dstIdx = (y * width + x) * destBytesPerPixel;

	//			auto height = (data[srcIdx * srcBytesPerPixel] * invMaxValue) * 255.0f;
	//			resizedData[dstIdx + 0] = height; // R
	//			resizedData[dstIdx + 1] = height; // G
	//			resizedData[dstIdx + 2] = height; // B
	//			resizedData[dstIdx + 3] = height; // (normPixelValue < threshold) ? 0 : 255; // A
	//		}
	//	}

	//	originalStagingTexture->stopMapRegion();

	//	this->heightMapStagingTexture = this->textureManager->getStagingTexture(width, height, 1u, 1u, texture->getPixelFormat());
	//	
	//	this->heightMapStagingTexture->startMapRegion();
	//	
	//	Ogre::TextureBox uploadTexBox = this->heightMapStagingTexture->mapRegion(width, height, 1u, 1u, texture->getPixelFormat());
	//	
	//	memcpy(uploadTexBox.data, resizedData.data(), resizedData.size());
	//	this->heightMapStagingTexture->stopMapRegion();
	//	
	//	// Finalize the upload
	//	this->heightMapStagingTexture->upload(uploadTexBox, texture, 0, 0, 0);
	//}

	//void MinimapComponent::updateHeightMapTexture(Ogre::TextureGpu* texture, unsigned int width, unsigned int height)
	//{
	//	const auto originalHeightMapTexture = this->terraComponent->getTerra()->getHeightMapTex();

	//	auto originalStagingTexture = this->textureManager->getStagingTexture(originalHeightMapTexture->getWidth(), originalHeightMapTexture->getHeight(), 1u, 1u, originalHeightMapTexture->getPixelFormat());

	//	auto pixelFormat = texture->getPixelFormat();

	//	// Download the original heightmap texture to CPU
	//	originalStagingTexture->startMapRegion();

	//	Ogre::TextureBox origTexBox = originalStagingTexture->mapRegion(originalHeightMapTexture->getWidth(), originalHeightMapTexture->getHeight(), 1u, 1u, originalHeightMapTexture->getPixelFormat());
	//	// Download the original heightmap texture to CPU
	//	uint8* srcData = reinterpret_cast<uint8 * RESTRICT_ALIAS>(origTexBox.data);
	//	originalStagingTexture->stopMapRegion();

	//	// const size_t bytesPerPixel = origTexBox.bytesPerPixel;

	//	std::vector<uint8_t> resizedData(256u * 256u * 4 /*bytesPerPixel*/); // RGBA
	//	const uint8_t threshold = 10; // Define near black threshold

	//	for (uint32 y = 0; y < 256u; y++)
	//	{
	//		uint8* RESTRICT_ALIAS pixBoxData = reinterpret_cast<uint8 * RESTRICT_ALIAS>(origTexBox.at(0, y, 0));
	//		for (uint32 x = 0; x < 256u; x++)
	//		{
	//			uint32 srcX = x * 4; // Scale down by 4
	//			uint32 srcY = y * 4;
	//			uint8* RESTRICT_ALIAS srcPixData = reinterpret_cast<uint8 * RESTRICT_ALIAS>(&srcData[srcY * 1024u + srcX]);

	//			ColourValue rgba;
	//			rgba.r = *srcPixData / 255.0f;
	//			rgba.g = *srcPixData / 255.0f;
	//			rgba.b = *srcPixData / 255.0f; // Convert to float [0, 1]
	//			rgba.a = *srcPixData / 255.0f;
	//			// rgba.a = (*srcPixData < threshold) ? 0.0f : 1.0f; // Alpha

	//			size_t dstIdx = x * 4/*bytesPerPixel*/;
	//			PixelFormatGpuUtils::packColour(rgba, pixelFormat, &pixBoxData[dstIdx]);
	//			// PixelFormatGpuUtils::packColour(rgba, PFG_RGBA8_UNORM, &resizedData[dstIdx]); // Pack RGBA
	//		}
	//	}

	//	// Upload the resized data to the new texture
	//	this->heightMapStagingTexture = this->textureManager->getStagingTexture(width, height, 1u, 1u, texture->getPixelFormat());
	//	this->heightMapStagingTexture->startMapRegion();
	//	Ogre::TextureBox uploadTexBox = this->heightMapStagingTexture->mapRegion(width, height, 1u, 1u, texture->getPixelFormat());

	//	uint8* RESTRICT_ALIAS dstData = reinterpret_cast<uint8 * RESTRICT_ALIAS>(uploadTexBox.data);
	//	memcpy(dstData, resizedData.data(), resizedData.size());
	//	this->heightMapStagingTexture->stopMapRegion();

	//	// Finalize the upload
	//	this->heightMapStagingTexture->upload(uploadTexBox, texture, 0, 0, 0);
	//}

//	void MinimapComponent::updateHeightMapTexture(Ogre::TextureGpu* texture, unsigned int width, unsigned int height)
//	{
//		auto originalHeightMapTexture = this->terraComponent->getTerra()->getHeightMapTex();
//
//		auto originalStagingTexture = this->textureManager->getStagingTexture(originalHeightMapTexture->getWidth(), originalHeightMapTexture->getHeight(), 1u, 1u, originalHeightMapTexture->getPixelFormat());
//
//		auto pixelFormat = texture->getPixelFormat();
//
//		// Download the original heightmap texture to CPU
//		originalStagingTexture->startMapRegion();
//
//		Ogre::TextureBox texBox = originalStagingTexture->mapRegion(originalHeightMapTexture->getWidth(), originalHeightMapTexture->getHeight(), 1u, 1u, originalHeightMapTexture->getPixelFormat());
//
//		// Create a temporary buffer for resizing
//		// * 4 because of RGBA8 for this texture, but heighmap original is PFG_R8_UNORM
//		std::vector<uint8_t> resizedData(width * height * 4);
//
//		int scaleFactor = originalHeightMapTexture->getWidth() / width;
//		const uint8_t threshold = 10; // Define near black threshold
//
//		const size_t bytesPerPixel = texBox.bytesPerPixel;
//
//		float fBpp = (float)(PixelFormatGpuUtils::getBytesPerPixel(originalHeightMapTexture->getPixelFormat()) << 3u);
//		const float maxValue = powf(2.0f, fBpp) - 1.0f;
//		float invMaxValue = 1.0f / maxValue;
//
//#if 0
//		for (size_t y = 0; y < height; ++y)
//		{
//			for (size_t x = 0; x < width; ++x)
//			{
//				size_t srcX = x * scaleFactor; // Scale down by 4
//				size_t srcY = y * scaleFactor;
//
//				uint8_t pixelValue = static_cast<uint8_t*>(texBox.data)[srcY * originalHeightMapTexture->getWidth() + srcX];
//				auto normPixelValue = pixelValue;
//				// auto normPixelValue = (pixelValue * invMaxValue) * originalHeightMapTexture->getHeight();
//
//				size_t dstIndex = (y * width + x) * 4;
//				resizedData[dstIndex + 0] = normPixelValue; // R
//				resizedData[dstIndex + 1] = normPixelValue; // G
//				resizedData[dstIndex + 2] = normPixelValue; // B
//				resizedData[dstIndex + 3] = normPixelValue; // (normPixelValue < threshold) ? 0 : 255; // A
//			}
//		}
//#else
//		for (uint32 y = 0; y < height; y++)
//		{
//			uint8* RESTRICT_ALIAS pixBoxData = reinterpret_cast<uint8 * RESTRICT_ALIAS>(texBox.at(0, y, 0));
//			for (uint32 x = 0; x < width; x++)
//			{
//				const size_t srcIdx = x * scaleFactor;
//				uint16 pixelValue = pixBoxData[srcIdx];
//
//				const size_t dstIdx = x * bytesPerPixel;
//				float rgba[4];
//				// Create blank texture without details
//				rgba[0] = pixelValue; // R
//				rgba[1] = pixelValue; // G
//				rgba[2] = pixelValue; // B
//				rgba[3] = pixelValue; // (pixelValue < threshold) ? 0 : 255; // A
//
//				resizedData[dstIdx + 0] = pixelValue; // R
//				resizedData[dstIdx + 1] = pixelValue; // G
//				resizedData[dstIdx + 2] = pixelValue; // B
//				resizedData[dstIdx + 3] = pixelValue; // (pixelValue < threshold) ? 0 : 255; // A
//
//				// PixelFormatGpuUtils::packColour(rgba, pixelFormat, &pixBoxData[dstIdx]);
//			}
//		}
//#endif
//
//		originalStagingTexture->stopMapRegion();
//
//		// Upload the resized data to the new texture
//		this->heightMapStagingTexture = this->textureManager->getStagingTexture(width, height, 1u, 1u, texture->getPixelFormat());
//
//		this->heightMapStagingTexture->startMapRegion();
//
//		Ogre::TextureBox uploadTexBox = this->heightMapStagingTexture->mapRegion(width, height, 1u, 1u, texture->getPixelFormat());
//
//		memcpy(uploadTexBox.data, resizedData.data(), resizedData.size());
//		this->heightMapStagingTexture->stopMapRegion();
//
//		// Finalize the upload
//		this->heightMapStagingTexture->upload(uploadTexBox, texture, 0, 0, 0);
//	}

//	void MinimapComponent::updateHeightMapTexture(Ogre::TextureGpu* texture, unsigned int width, unsigned int height)
//	{
//		auto originalHeightMapTexture = this->terraComponent->getTerra()->getHeightMapTex();
//
//		auto originalStagingTexture = this->textureManager->getStagingTexture(originalHeightMapTexture->getWidth(), originalHeightMapTexture->getHeight(), 1u, 1u, originalHeightMapTexture->getPixelFormat());
//
//		// Download the original heightmap texture to CPU
//		originalStagingTexture->startMapRegion();
//
//		Ogre::TextureBox texBox = originalStagingTexture->mapRegion(originalHeightMapTexture->getWidth(), originalHeightMapTexture->getHeight(), 1u, 1u, originalHeightMapTexture->getPixelFormat());
//
//		// Create a temporary buffer for resizing
//		// * 4 because of RGBA8 for this texture, but heighmap original is PFG_R8_UNORM
//		std::vector<uint8_t> resizedData(width * height * 4);
//
//		int scaleFactor = originalHeightMapTexture->getWidth() / width;
//		const uint8_t threshold = 10; // Define near black threshold
//
//		const size_t bytesPerPixel = texBox.bytesPerPixel;
//
//#if 0
//		for (size_t y = 0; y < height; ++y)
//		{
//			for (size_t x = 0; x < width; ++x)
//			{
//				size_t srcX = x * scaleFactor; // Scale down by 4
//				size_t srcY = y * scaleFactor;
//
//				uint8_t pixelValue = static_cast<uint8_t*>(texBox.data)[srcY * originalHeightMapTexture->getWidth() + srcX];
//
//				size_t dstIndex = (y * width + x) * 4;
//				resizedData[dstIndex + 0] = pixelValue; // R
//				resizedData[dstIndex + 1] = pixelValue; // G
//				resizedData[dstIndex + 2] = pixelValue; // B
//				resizedData[dstIndex + 3] = (pixelValue < threshold) ? 0 : 255; // A
//				// resizedData[y * width + x] = static_cast<uint8_t*>(texBox.data)[srcY * originalHeightMapTexture->getWidth() + srcX];
//			}
//		}
//#else
//
//
//		for (uint32 y = 0; y < height; y++)
//		{
//			uint8* RESTRICT_ALIAS pixBoxData = reinterpret_cast<uint8 * RESTRICT_ALIAS>(texBox.at(0, y, 0));
//			for (uint32 x = 0; x < width; x++)
//			{
//				const size_t srcIdx = x * bytesPerPixel * scaleFactor;
//				uint16 pixelValue = pixBoxData[srcIdx];
//
//				const size_t dstIdx = x * bytesPerPixel;
//				float rgba[4];
//				// Create blank texture without details
//				rgba[dstIdx + 0] = pixelValue; // R
//				rgba[dstIdx + 1] = pixelValue; // G
//				rgba[dstIdx + 2] = pixelValue; // B
//				rgba[dstIdx + 3] = (pixelValue < threshold) ? 0 : 255; // A
//			
//				PixelFormatGpuUtils::packColour(rgba, this->fogOfWarTexture->getPixelFormat(), &pixBoxData[dstIdx]);
//			}
//		}
//#endif
//
//		originalStagingTexture->stopMapRegion();
//
//		// Upload the resized data to the new texture
//		this->heightMapStagingTexture = this->textureManager->getStagingTexture(width, height, 1u, 1u, texture->getPixelFormat());
//
//		this->heightMapStagingTexture->startMapRegion();
//
//		Ogre::TextureBox uploadTexBox = this->heightMapStagingTexture->mapRegion(width, height, 1u, 1u, texture->getPixelFormat());
//
//		memcpy(uploadTexBox.data, resizedData.data(), resizedData.size());
//		this->heightMapStagingTexture->stopMapRegion();
//
//		// Finalize the upload
//		this->heightMapStagingTexture->upload(uploadTexBox, texture, 0, 0, 0);
//	}

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

			if (nullptr != this->heightMapStagingTexture)
			{
				this->textureManager->removeStagingTexture(this->heightMapStagingTexture);
				this->heightMapStagingTexture = nullptr;
			}

			if (nullptr != this->fogOfWarStagingTexture)
			{
				this->textureManager->removeStagingTexture(this->fogOfWarStagingTexture);
				this->fogOfWarStagingTexture = nullptr;
			}

			if (nullptr != this->minimapTexture)
			{
				if (nullptr != this->minimapWidget)
				{
					this->minimapWidget->setVisible(false);
				}
			}
			if (nullptr != this->heightMapTexture)
			{
				this->heightMapWidget->setVisible(false);
			}
			if (nullptr != this->fogOfWarWidget)
			{
				this->fogOfWarWidget->setVisible(false);
			}

			// MyGUI::Gui::getInstancePtr()->destroyWidget(this->minimapWidget);
			// MyGUI::Gui::getInstancePtr()->destroyWidget(this->fogOfWarWidget);
			
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

	bool MinimapComponent::saveDiscoveryState(void)
	{
		Ogre::String existingMinimapDiscoveryFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getWorldName() + "_mData.bin";
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
		Ogre::String existingMinimapDiscoveryFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getWorldName() + "_mData.bin";
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

	void MinimapComponent::handleTerraChanged(EventDataPtr eventData)
	{
		if (nullptr == this->heightMapTexture)
		{
			return;
		}

		boost::shared_ptr<EventDataTerraChanged> castEventData = boost::static_pointer_cast<EventDataTerraChanged>(eventData);

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
		else if (MinimapComponent::AttrHideId() == attribute->getName())
		{
			this->setHideId(attribute->getULong());
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

	void MinimapComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "HideId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->hideId->getULong())));
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
				this->minimapWidget->setVisible(false);
			}
			if (nullptr != this->fogOfWarWidget)
			{
				this->fogOfWarWidget->setVisible(false);
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
		this->useFogOfWar->setValue(useFogOfWar);
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
	}

	bool MinimapComponent::getWholeSceneVisible(void) const
	{
		return this->wholeSceneVisible->getBool();
	}

	void MinimapComponent::setHideId(unsigned long hideId)
	{
		this->hideId->setValue(hideId);
	}

	unsigned long MinimapComponent::getHideId(void) const
	{
		return this->hideId->getULong();
	}

	void MinimapComponent::setUseRoundMinimap(bool useRoundMinimap)
	{
		if (false == this->wholeSceneVisible->getBool())
		{
			this->useRoundMinimap->setValue(useRoundMinimap);
		}
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

	MinimapComponent* getMinimapComponent(GameObject* gameObject, unsigned int occurrenceIndex)
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
		];

		LuaScriptApi::getInstance()->addClassToCollection("MinimapComponent", "class inherits GameObjectComponent", MinimapComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("MinimapComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
		LuaScriptApi::getInstance()->addClassToCollection("MinimapComponent", "bool isActivated()", "Gets whether this component is activated.");
		LuaScriptApi::getInstance()->addClassToCollection("MinimapComponent", "void setVisibilityRadius(number visibilityRadius)", "If fog of war is used, sets the visibilty radius which discovers the fog of war.");
		LuaScriptApi::getInstance()->addClassToCollection("MinimapComponent", "bool getVisibilityRadius()", "Gets the visibilty radius which discovers the fog of war.");
		LuaScriptApi::getInstance()->addClassToCollection("MinimapComponent", "void clearDiscoveryState()", "Clears the area which has been already discovered and saves the state.");
		LuaScriptApi::getInstance()->addClassToCollection("MinimapComponent", "void clearDiscoveryState()", "Saves the area which has been already discovered.");
		LuaScriptApi::getInstance()->addClassToCollection("MinimapComponent", "void clearDiscoveryState()", "Loads the area which has been already discovered.");

		gameObjectClass.def("getMinimapComponentFromName", &getMinimapComponentFromName);
		gameObjectClass.def("getMinimapComponent", (MinimapComponent * (*)(GameObject*)) & getMinimapComponent);
		// If its desired to create several of this components for one game object
		gameObjectClass.def("getMinimapComponent2", (MinimapComponent * (*)(GameObject*, unsigned int)) & getMinimapComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "MinimapComponent getMinimapComponent2(unsigned int occurrenceIndex)", "Gets the component by the given occurence index, since a game object may this component maybe several times.");
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