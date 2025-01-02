#include "NOWAPrecompiled.h"
#include "TerraComponent.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "camera/CameraManager.h"
#include "modules/WorkspaceModule.h"
#include "main/ProcessManager.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "LightDirectionalComponent.h"
#include "PhysicsTerrainComponent.h"
#include "DatablockTerraComponent.h"
#include "CameraComponent.h"
#include "WorkspaceComponents.h"

namespace
{
	// Function to check if a number is divisible by a power of 2
	template <typename T>
	bool isPowerOf2(T number)
	{
		return number > 0 && (number & (number - 1)) == 0;
	}

	// Function to adjust the number to the next power of 2
	template <typename T>
	T adjustToNextPowerOf2(T number)
	{
		if (isPowerOf2(number))
		{
			return number;  // The number is already a power of 2
		}
		else
		{
			// Calculate the next power of 2 greater than the number
			T nextPowerOf2 = 1;
			while (nextPowerOf2 < number)
			{
				nextPowerOf2 <<= 1;  // Left shift by 1 (multiply by 2)
			}
			return nextPowerOf2;
		}
	}
}

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	TerraComponent::TerraComponent()
		: GameObjectComponent(),
		terra(nullptr),
		hlmsPbsTerraShadows(nullptr),
		sunLight(nullptr),
		terraWorkspaceListener(nullptr),
		usedCamera(nullptr),
		postInitDone(false),
		terraLoadedFromFile(false),
		center(new Variant(TerraComponent::AttrCenter(), Ogre::Vector3(0.0f, 25.0f, 0.0f), this->attributes)),
		pixelSize(new Variant(TerraComponent::AttrPixelSize(), Ogre::Vector2(1024.0f, 1024.0f), this->attributes)),
		dimensions(new Variant(TerraComponent::AttrDimensions(), Ogre::Vector3(100.0f, 100.0f, 100.0f), this->attributes)),
		lightId(new Variant(TerraComponent::AttrLightId(), static_cast<unsigned long>(0), this->attributes, true)),
		cameraId(new Variant(TerraComponent::AttrCameraId(), static_cast<unsigned long>(0), this->attributes, true)),
		basePixelDimension(new Variant(TerraComponent::AttrBasePixelDimension(), static_cast<unsigned int>(64), this->attributes, true)),
		strength(new Variant(TerraComponent::AttrStrength(), 50, this->attributes)),
		brushSize(new Variant(TerraComponent::AttrBrushSize(), static_cast<int>(64), this->attributes)),
		brushIntensity(new Variant(TerraComponent::AttrBrushIntensity(), static_cast<int>(255), this->attributes)),
		imageLayer(new Variant(TerraComponent::AttrImageLayer(), { "0", "1", "2", "3" }, this->attributes)),
		brush(new Variant(TerraComponent::AttrBrush(), this->attributes))
	{
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &TerraComponent::handleSwitchCamera), EventDataSwitchCamera::getStaticEventType());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &TerraComponent::handleEventDataGameObjectMadeGlobal), EventDataGameObjectMadeGlobal::getStaticEventType());

		this->lightId->setDescription("The directional light game object id, in order to calculate light position for shading and shadows. Note: Initially no shadows are shown, if no light id is set!");
		this->cameraId->setDescription("The optional camera game object id which can be set. E.g. useful if the MinimapComponent is involved, to set the minimap camera, so that terra is painted correctly on minimap. Can be left of, default is the main active camera.");

		Ogre::StringVectorPtr brushNames = Ogre::ResourceGroupManager::getSingleton().findResourceNames("Brushes", "*.png");
		std::vector<Ogre::String> compatibleBrushNames(brushNames.getPointer()->size() + 1);
		unsigned int i = 0;
		for (auto& it = brushNames.getPointer()->cbegin(); it != brushNames.getPointer()->cend(); it++)
		{
			compatibleBrushNames[i++] = *it;
		}

		this->pixelSize->setDescription("Sets the pixel size of the height map and blend map. This size should be increased if huge terra (e.g. 4096 200 4096) is used.");

		this->strength->setConstraints(-500, 500);
		this->strength->addUserData(GameObject::AttrActionNoUndo());

		this->brush->setValue(compatibleBrushNames);
		if (compatibleBrushNames.size() > 1)
		{
			this->brush->setListSelectedValue(compatibleBrushNames[1]);
		}
		this->brush->addUserData(GameObject::AttrActionImage());
		this->brush->addUserData(GameObject::AttrActionNoUndo());

		this->brushSize->setConstraints(1.0f, 5000.0f);
		this->brushSize->addUserData(GameObject::AttrActionNoUndo());

		this->brushIntensity->setConstraints(1.0f, 255.0f);
		this->brushIntensity->addUserData(GameObject::AttrActionNoUndo());

		this->imageLayer->addUserData(GameObject::AttrActionNoUndo());

		this->basePixelDimension->setDescription("Lower values makes LOD very aggressive. Higher values less aggressive. Must be power of 2.");
	}

	TerraComponent::~TerraComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[TerraComponent] Destructor terra component for game object: " + this->gameObjectPtr->getName());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &TerraComponent::handleEventDataGameObjectMadeGlobal), EventDataGameObjectMadeGlobal::getStaticEventType());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &TerraComponent::handleSwitchCamera), EventDataSwitchCamera::getStaticEventType());
		
		this->destroyTerra();
	}

	bool TerraComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Center")
		{
			this->center->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PixelSize")
		{
			this->pixelSize->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Dimensions")
		{
			this->dimensions->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "LightId")
		{
			this->lightId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CameraId")
		{
			this->cameraId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BasePixelDimension")
		{
			this->basePixelDimension->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		this->terraLoadedFromFile = true;
		
		return true;
	}

	void TerraComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		WorkspaceBaseComponent* workspaceBaseComponent = WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent();
		if (nullptr != workspaceBaseComponent && false == AppStateManager::getSingletonPtr()->getIsShutdown())
		{
			workspaceBaseComponent->setUseTerra(false);
		}

		this->gameObjectPtr->getAttribute(GameObject::AttrUseReflection())->setVisible(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrMeshName())->setVisible(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrScale())->setVisible(true);

		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(true);
	}

	GameObjectCompPtr TerraComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		/*TerraCompPtr clonedCompPtr(boost::make_shared<TerraComponent>());

		
		clonedCompPtr->setHeightMap(this->heightMap->getString());
		clonedCompPtr->setDimensions(this->dimensions->getVector3());
		clonedCompPtr->setLightId(this->lightId->getULong());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;*/

		return nullptr;
	}

	bool TerraComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[TerraComponent] Init terra component for game object: " + this->gameObjectPtr->getName());
		// Terra is destroyed here, so do not destroy it in the game object ptr, because its destroyed there via scenemanager which will not work, because its created differently
		this->gameObjectPtr->setDoNotDestroyMovableObject(true);

		this->gameObjectPtr->setUseReflection(false);
		this->gameObjectPtr->getAttribute(GameObject::AttrUseReflection())->setVisible(false);
		this->gameObjectPtr->getAttribute(GameObject::AttrMeshName())->setVisible(false);
		this->gameObjectPtr->getAttribute(GameObject::AttrScale())->setVisible(false);

		this->gameObjectPtr->getAttribute(GameObject::AttrCastShadows())->setVisible(false);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
		this->gameObjectPtr->getAttribute(GameObject::AttrClampY())->setVisible(false);

		// Get the sun light (directional light for sun power setting)
		GameObjectPtr lightGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromName("SunLight");
		if (nullptr != lightGameObjectPtr)
		{
			this->setLightId(lightGameObjectPtr->getId());
		}

		this->postInitDone = true;

		this->createTerra();

		return true;
	}

	bool TerraComponent::connect(void)
	{
		return true;
	}

	bool TerraComponent::disconnect(void)
	{
		return true;
	}

	bool TerraComponent::onCloned(void)
	{
		// Search for the prior id of the cloned game object and set the new id and set the new id, if not found set better 0, else the game objects may be corrupt!
		/*GameObjectPtr lightGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(this->lightId->getULong());
		if (nullptr != lightGameObjectPtr)
			this->setLightId(lightGameObjectPtr->getId());
		else
			this->setLightId(0);*/
		return true;
	}

	void TerraComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (nullptr != this->terra)
		{
			//Force update the shadow map every frame to avoid the feeling we're "cheating" the
			//user in this sample with higher framerates than what he may encounter in many of
			//his possible uses.
			const float lightEpsilon = 0.0f;
			if (nullptr != this->sunLight)
			{
				this->terra->update(this->sunLight->getDerivedDirectionUpdated(), lightEpsilon);
			}
			else
			{
				this->terra->update(Ogre::Vector3::ZERO, lightEpsilon);
			}
		}
	}
	
	void TerraComponent::destroyTerra(void)
	{
		//Unset the PBS listener and destroy it
		if (nullptr != this->terra)
		{
			Core::getSingletonPtr()->getBaseListenerContainer()->removeConcreteListener(this->hlmsPbsTerraShadows);
			this->hlmsPbsTerraShadows = nullptr;

			WorkspaceBaseComponent* workspaceBaseComponent = WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent();
			if (nullptr != workspaceBaseComponent && false == AppStateManager::getSingletonPtr()->getIsShutdown())
			{
				 workspaceBaseComponent->setUseTerra(false);
				 workspaceBaseComponent->getWorkspace()->removeListener(this->terraWorkspaceListener);
				 delete this->terraWorkspaceListener;
				 this->terraWorkspaceListener = nullptr;
			}

			this->gameObjectPtr->sceneNode->detachObject(this->terra);
			delete this->terra;
			this->terra = nullptr;
			if (nullptr != this->gameObjectPtr->boundingBoxDraw)
			{
				this->gameObjectPtr->sceneManager->destroyWireAabb(this->gameObjectPtr->boundingBoxDraw);
				this->gameObjectPtr->boundingBoxDraw = nullptr;
			}
		}
	}

	void TerraComponent::handleSwitchCamera(NOWA::EventDataPtr eventData)
	{
		// When camera changed event must be triggered, to set the new camera for the terra
		WorkspaceBaseComponent* workspaceBaseComponent = WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent();
		if (nullptr != workspaceBaseComponent)
		{
			this->createTerra();
		}
	}

	void TerraComponent::createTerra(void)
	{
		if (nullptr == this->terra && nullptr != AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera() && true == this->postInitDone)
		{
			if (this->cameraId->getULong() != 0)
			{
				GameObjectPtr cameraGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->cameraId->getULong());
				if (nullptr != cameraGameObjectPtr)
				{
					auto& cameraCompPtr = NOWA::makeStrongPtr(cameraGameObjectPtr->getComponent<CameraComponent>());
					if (nullptr != cameraCompPtr)
					{
						this->usedCamera = cameraCompPtr->getCamera();
					}
				}
			}
			else
			{
				this->usedCamera = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
			}

			this->terra = new Ogre::Terra(Ogre::Id::generateNewId<Ogre::MovableObject>(), &this->gameObjectPtr->getSceneManager()->_getEntityMemoryManager(Ogre::SCENE_STATIC),
				this->gameObjectPtr->getSceneManager(), NOWA::RENDER_QUEUE_TERRA, WorkspaceModule::getInstance()->getCompositorManager(), this->usedCamera, false);
			// Attention: Shadows must not be casted for terra, else ugly crash shader cache is created
			this->terra->setCastShadows(false);
			this->terra->setName(this->gameObjectPtr->getName());
			this->terra->setPrefix(Core::getSingletonPtr()->getSceneName());

			// There can be up to 4 blend maps! one for texture state in datablock!


			Ogre::String existingDetailMapFilePathName;
			Ogre::String existingHeightMapFilePathName;
			if (false == this->gameObjectPtr->getGlobal())
			{
				existingDetailMapFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + Core::getSingletonPtr()->getSceneName() + "_detailMap.png";
				existingHeightMapFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + Core::getSingletonPtr()->getSceneName() + "_heightMap.png";
			}
			else
			{
				existingDetailMapFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "_detailMap.png";
				existingHeightMapFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "_heightMap.png";
			}

			std::fstream detailMapFile(existingDetailMapFilePathName);
			if (false == detailMapFile.good())
			{
				existingDetailMapFilePathName = "";
			}

			
			std::fstream heightMapFile(existingHeightMapFilePathName);
			if (true == heightMapFile.good())
			{
				heightMapFile.close();
				// true memory sharing when having more than one terra!
				this->terra->load(existingHeightMapFilePathName, existingDetailMapFilePathName, this->center->getVector3(), this->dimensions->getVector3(), false, false);
			}
			else
			{
				this->terra->load(this->center->getVector3().y, static_cast<uint16_t>(this->pixelSize->getVector2().x), static_cast<uint16_t>(this->pixelSize->getVector2().y), this->center->getVector3(), this->dimensions->getVector3(), false, false);
			}

#if 0
			// If terra has been created from the scratch and center.y e.g. is 50 meters, place the camera approprirate to have a visible ground
			if (false == this->terraLoadedFromFile)
			{
				if (this->usedCamera->getPositionForViewUpdate().y < this->center->getVector3().y)
				{
					this->usedCamera->setPosition(this->usedCamera->getPositionForViewUpdate().x, this->center->getVector3().y, this->usedCamera->getPositionForViewUpdate().z);
				}
				this->terraLoadedFromFile = true;
			}
#endif

			Ogre::Vector3 center = terra->getTerrainOrigin() + (Ogre::Vector3(terra->getXZDimensions().x, this->dimensions->getVector3().y, terra->getXZDimensions().y) / 2.0f);
			// startX = -184
			Ogre::Real startX = terra->getTerrainOrigin().x;
			// endX = 184 + 64 * 2 = 312
			Ogre::Real endX = terra->getTerrainOrigin().x * -1 + (center.x * 2.0f);

			Ogre::Real startY = terra->getTerrainOrigin().y;
			// endX = 184 + 64 * 2 = 312
			Ogre::Real endY = terra->getTerrainOrigin().y * -1 + (center.y * 2.0f);

			Ogre::Real startZ = terra->getTerrainOrigin().z;
			Ogre::Real endZ = terra->getTerrainOrigin().z * -1 + center.z * 2.0f;

			Ogre::Vector3 min = Ogre::Vector3(startX - this->center->getVector3().x, this->center->getVector3().y - this->dimensions->getVector3().y, startZ - this->center->getVector3().z);
			Ogre::Vector3 max = Ogre::Vector3(endX - this->center->getVector3().x, this->dimensions->getVector3().y - this->center->getVector3().y, endZ - this->center->getVector3().z);

			auto aabb = Ogre::Aabb::newFromExtents(min, max);
			terra->setLocalAabb(aabb);

			if (nullptr != this->terra)
			{
				// Note: Default terra datablock must first always be used from json. On that base the data block may be adapted in datablock terra component.
				Ogre::HlmsDatablock* datablock = WorkspaceModule::getInstance()->getHlmsManager()->getDatablock("TerraDefaultMaterial");
				if (nullptr != datablock)
				{
					//        Ogre::HlmsDatablock *datablock = hlmsManager->getHlms( Ogre::HLMS_USER3 )->getDefaultDatablock();
					//        Ogre::HlmsMacroblock macroblock;
					//        macroblock.mPolygonMode = Ogre::PM_WIREFRAME;
					//datablock->setMacroblock( macroblock );

					this->terra->setDatablock(datablock);
				}
				else
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[TerraComponent] Error cannot create terra because there is no data block 'TerraDefaultMaterial' for game object: " + this->gameObjectPtr->getName());
					throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[TerraComponent] Error cannot create terra because there is no data block 'TerraDefaultMaterial' for game object: " + this->gameObjectPtr->getName() + ".\n", "NOWA");
				}
			}

			// Terra cannot be moved after being created, so set static
			this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
			this->terra->setStatic(true);
			this->gameObjectPtr->setDynamic(false);
			this->gameObjectPtr->getSceneNode()->attachObject(this->terra);

			{
				this->hlmsPbsTerraShadows = new Ogre::HlmsPbsTerraShadows();
				this->hlmsPbsTerraShadows->setTerra(this->terra);
				// Set the PBS listener so regular objects also receive terrain shadows
				Core::getSingletonPtr()->getBaseListenerContainer()->addConcreteListener(this->hlmsPbsTerraShadows);
				// Ogre::Hlms *hlmsPbs = Ogre::Root::getSingletonPtr()->getHlmsManager()->getHlms( Ogre::HLMS_PBS );
				// hlmsPbs->setListener(this->hlmsPbsTerraShadows);
			}

			this->terra->setVisible(this->gameObjectPtr->getAttribute(GameObject::AttrVisible())->getBool());
			this->gameObjectPtr->init(this->terra);
			// Register after the component has been created
			AppStateManager::getSingletonPtr()->getGameObjectController()->registerGameObject(gameObjectPtr);

			this->setLightId(this->lightId->getULong());

			WorkspaceBaseComponent* workspaceBaseComponent = WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent();
			if (nullptr != workspaceBaseComponent)
			{
				if (false == workspaceBaseComponent->getUseTerra())
				{
					workspaceBaseComponent->terra = this->terra;
					workspaceBaseComponent->setUseTerra(true);
				}

				if (nullptr == this->terraWorkspaceListener)
				{
					Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingletonPtr()->getHlmsManager();
					Ogre::Hlms* hlms = hlmsManager->getHlms(Ogre::HLMS_USER3);
					OGRE_ASSERT_HIGH(dynamic_cast<HlmsTerra*>(hlms));
					this->terraWorkspaceListener = new Ogre::TerraWorkspaceListener((Ogre::HlmsTerra*)hlms);
				}
				workspaceBaseComponent->getWorkspace()->addListener(this->terraWorkspaceListener);
			}
		}

		if (nullptr != this->terra)
		{
			this->terra->setBrushName(this->brush->getListSelectedValue());
		}

		boost::shared_ptr<NOWA::EventDataResourceCreated> eventDataResourceCreated(new NOWA::EventDataResourceCreated());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataResourceCreated);
	}

	void TerraComponent::modifyTerrainStart(const Ogre::Vector3& position, float strength)
	{
		if (nullptr != this->terra)
		{
			if (true == this->brush->getListSelectedValue().empty())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[TerraComponent] Could not modify terrain, because no brush name has been selected for game object: " + this->gameObjectPtr->getName());
				return;
			}

			this->terra->modifyTerrainStart(position, strength);
		}
	}

	void TerraComponent::smoothTerrainStart(const Ogre::Vector3& position, float strength)
	{
		if (nullptr != this->terra)
		{
			if (true == this->brush->getListSelectedValue().empty())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[TerraComponent] Could not smooth terrain, because no brush name has been selected for game object: " + this->gameObjectPtr->getName());
				return;
			}

			this->terra->smoothTerrainStart(position, strength);
		}
	}

	void TerraComponent::paintTerrainStart(const Ogre::Vector3& position, float intensity, int imageLayer)
	{
		if (nullptr != this->terra)
		{
			if (true == this->brush->getListSelectedValue().empty())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[TerraComponent] Could not paint terrain, because no brush name has been selected for game object: " + this->gameObjectPtr->getName());
				return;
			}

			this->terra->paintTerrainStart(position, intensity, imageLayer);
		}
	}

	void TerraComponent::modifyTerrain(const Ogre::Vector3& position, float strength)
	{
		if (nullptr != this->terra)
		{
			if (true == this->brush->getListSelectedValue().empty())
			{
				return;
			}

			this->terra->modifyTerrain(position, strength);
		}
	}

	void TerraComponent::smoothTerrain(const Ogre::Vector3& position, float strength)
	{
		if (nullptr != this->terra)
		{
			if (true == this->brush->getListSelectedValue().empty())
			{
				return;
			}

			this->terra->smoothTerrain(position, strength);
		}
	}

	void TerraComponent::paintTerrain(const Ogre::Vector3& position, float intensity, int imageLayer)
	{
		if (nullptr != this->terra)
		{
			if (true == this->brush->getListSelectedValue().empty())
			{
				return;
			}

			this->terra->paintTerrain(position, intensity, imageLayer);
		}
	}

	void TerraComponent::modifyTerrainEnd(void)
	{
		if (nullptr != this->terra)
		{
			if (true == this->brush->getListSelectedValue().empty())
			{
				return;
			}

			boost::shared_ptr<EventDataTerraChanged> eventDataTerraChanged(new EventDataTerraChanged(this->gameObjectPtr->getId(), true, false));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataTerraChanged);

			// Prevents undo redo in game, because of fps dropdown (heavy operation). For editor mode its ok
			if (false == Core::getSingletonPtr()->getIsGame())
			{
				boost::shared_ptr<EventDataTerraModifyEnd> eventDataTerraModifyEnd(new EventDataTerraModifyEnd(this->terra->modifyTerrainFinished().first, this->terra->modifyTerrainFinished().second, this->gameObjectPtr->getId()));
				NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataTerraModifyEnd);
			}

			auto& component = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsTerrainComponent>());
			if (nullptr != component)
			{
				component->reCreateCollision();
			}
		}
	}

	void TerraComponent::smoothTerrainEnd(void)
	{
		if (true == this->brush->getListSelectedValue().empty())
		{
			return;
		}

		if (nullptr != this->terra)
		{
			boost::shared_ptr<EventDataTerraChanged> eventDataTerraChanged(new EventDataTerraChanged(this->gameObjectPtr->getId(), true, false));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataTerraChanged);

			// Prevents undo redo in game, because of fps dropdown (heavy operation). For editor mode its ok
			if (false == Core::getSingletonPtr()->getIsGame())
			{
				boost::shared_ptr<EventDataTerraModifyEnd> eventDataTerraModifyEnd(new EventDataTerraModifyEnd(this->terra->modifyTerrainFinished().first, this->terra->modifyTerrainFinished().second, this->gameObjectPtr->getId()));
				NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataTerraModifyEnd);
			}
		}
	}

	void TerraComponent::paintTerrainEnd(void)
	{
		std::pair<std::vector<Ogre::uint8>, std::vector<Ogre::uint8>> detailBlendData;

		if (true == this->brush->getListSelectedValue().empty())
		{
			return;
		}

		if (nullptr != this->terra)
		{
			// Sends event, that terra has been modified
			boost::shared_ptr<EventDataTerraChanged> eventDataTerraChanged(new EventDataTerraChanged(this->gameObjectPtr->getId(), false, true));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataTerraChanged);

			// Prevents undo redo in game, because of fps dropdown (heavy operation). For editor mode its ok
			if (false == Core::getSingletonPtr()->getIsGame())
			{
				boost::shared_ptr<EventDataTerraPaintEnd> eventDataTerraPaintEnd(new EventDataTerraPaintEnd(this->terra->paintTerrainFinished().first, this->terra->paintTerrainFinished().second, this->gameObjectPtr->getId()));
				NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataTerraPaintEnd);
			}
		}
	}

	void TerraComponent::modifyTerrainLoop(const Ogre::Vector3& position, float strength, unsigned int loopCount)
	{
		if (true == this->brush->getListSelectedValue().empty())
		{
			return;
		}

		this->modifyTerrainStart(position, strength);

		for (unsigned int i = 0; i < loopCount; i++)
		{
			this->modifyTerrain(position, strength);
		}

		this->modifyTerrainEnd();
	}

	void TerraComponent::smoothTerrainLoop(const Ogre::Vector3& position, float strength, unsigned int loopCount)
	{
		if (true == this->brush->getListSelectedValue().empty())
		{
			return;
		}

		this->smoothTerrainStart(position, strength);

		for (unsigned int i = 0; i < loopCount; i++)
		{
			this->smoothTerrain(position, strength);
		}

		this->smoothTerrainEnd();
	}

	void TerraComponent::paintTerrainLoop(const Ogre::Vector3& position, float intensity, int imageLayer, unsigned int loopCount)
	{
		if (true == this->brush->getListSelectedValue().empty())
		{
			return;
		}

		this->paintTerrainStart(position, intensity, imageLayer);

		for (unsigned int i = 0; i < loopCount; i++)
		{
			this->paintTerrain(position, intensity, imageLayer);
		}

		this->paintTerrainEnd();
	}

	std::pair<std::vector<Ogre::uint16>, std::vector<Ogre::uint16>> TerraComponent::modifyTerrainFinished(void)
	{
		std::pair<std::vector<Ogre::uint16>, std::vector<Ogre::uint16>> heightData;

		if (true == this->brush->getListSelectedValue().empty())
		{
			return std::move(heightData);
		}

		if (nullptr != this->terra)
		{
			// Sends event, that terra has been modified
			boost::shared_ptr<EventDataTerraChanged> eventDataTerraChanged(new EventDataTerraChanged(this->gameObjectPtr->getId(), true, false));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataTerraChanged);

			heightData = this->terra->modifyTerrainFinished();
		}

		return std::move(heightData);
	}

	std::pair<std::vector<Ogre::uint16>, std::vector<Ogre::uint16>> TerraComponent::smoothTerrainFinished(void)
	{
		std::pair<std::vector<Ogre::uint16>, std::vector<Ogre::uint16>> heightData;

		if (true == this->brush->getListSelectedValue().empty())
		{
			return std::move(heightData);
		}

		if (nullptr != this->terra)
		{
			heightData = this->terra->smoothTerrainFinished();

			// Sends event, that terra has been modified
			boost::shared_ptr<EventDataTerraChanged> eventDataTerraChanged(new EventDataTerraChanged(this->gameObjectPtr->getId(), true, false));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataTerraChanged);
		}
		
		return std::move(heightData);
	}

	std::pair<std::vector<Ogre::uint8>, std::vector<Ogre::uint8>> TerraComponent::paintTerrainFinished(void)
	{
		std::pair<std::vector<Ogre::uint8>, std::vector<Ogre::uint8>> detailBlendData;

		if (true == this->brush->getListSelectedValue().empty())
		{
			return std::move(detailBlendData);
		}

		if (nullptr != this->terra)
		{
			// Sends event, that terra has been modified
			boost::shared_ptr<EventDataTerraChanged> eventDataTerraChanged(new EventDataTerraChanged(this->gameObjectPtr->getId(), false, true));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataTerraChanged);

			detailBlendData = this->terra->paintTerrainFinished();
			return std::move(detailBlendData);
		}
		return std::move(detailBlendData);
	}

	void TerraComponent::setHeightData(const std::vector<Ogre::uint16>& heightData)
	{
		if (nullptr != this->terra)
		{
			this->terra->setHeightData(heightData);

			// Sends event, that terra has been modified
			boost::shared_ptr<EventDataTerraChanged> eventDataTerraChanged(new EventDataTerraChanged(this->gameObjectPtr->getId(), true, false));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataTerraChanged);
		}
	}

	void TerraComponent::setBlendWeightData(const std::vector<Ogre::uint8>& blendWeightData)
	{
		if (nullptr != this->terra)
		{
			// Sends event, that terra has been modified
			boost::shared_ptr<EventDataTerraChanged> eventDataTerraChanged(new EventDataTerraChanged(this->gameObjectPtr->getId(), false, true));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataTerraChanged);

			this->terra->setBlendWeightData(blendWeightData);
		}
	}

	std::tuple<bool, Ogre::Vector3, Ogre::Vector3, Ogre::Real> TerraComponent::checkRayIntersect(const Ogre::Ray& ray)
	{
		if (nullptr != this->terra)
		{
			return this->terra->checkRayIntersect(ray);
		}

		return std::tuple<bool, Ogre::Vector3, Ogre::Vector3, Ogre::Real>(false, Ogre::Vector3::ZERO, Ogre::Vector3::ZERO, 0.0f);
	}

	void TerraComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		// Is not called, because this is no GO!
		if (GameObject::AttrPosition() == attribute->getName())
		{
			auto& component = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsTerrainComponent>());
			if (nullptr != component)
			{
				component->setPosition(attribute->getVector3());
			}
			else
			{
				this->gameObjectPtr->getSceneNode()->setPosition(attribute->getVector3());
			}
			this->destroyTerra();
			this->createTerra();
		}
		if (TerraComponent::AttrCenter() == attribute->getName())
		{
			this->center->setValue(attribute->getVector3());
			this->destroyTerra();
			this->createTerra();
		}
		else if (TerraComponent::AttrPixelSize() == attribute->getName())
		{
			this->setPixelSize(attribute->getVector2());
		}
		else if (TerraComponent::AttrDimensions() == attribute->getName())
		{
			this->setDimensions(attribute->getVector3());
		}
		else if (TerraComponent::AttrLightId() == attribute->getName())
		{
			this->setLightId(attribute->getULong());
		}
		else if (TerraComponent::AttrCameraId() == attribute->getName())
		{
			this->setCameraId(attribute->getULong());
		}
		else if (TerraComponent::AttrBasePixelDimension() == attribute->getName())
		{
			this->setBasePixelDimension(attribute->getUInt());
		}
		else if (TerraComponent::AttrStrength() == attribute->getName())
		{
			this->setStrength(attribute->getInt());
		}
		else if (TerraComponent::AttrBrush() == attribute->getName())
		{
			this->setBrushName(attribute->getListSelectedValue());
		}
		else if (TerraComponent::AttrBrushSize() == attribute->getName())
		{
			this->setBrushSize(static_cast<short>(attribute->getInt()));
		}
		else if (TerraComponent::AttrBrushIntensity() == attribute->getName())
		{
			this->setBrushIntensity(attribute->getInt());
		}
		else if (TerraComponent::AttrImageLayer() == attribute->getName())
		{
			this->setImageLayer(attribute->getListSelectedValue());
		}
	}

	void TerraComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "Center"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->center->getVector3())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "PixelSize"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->pixelSize->getVector2())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Dimensions"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->dimensions->getVector3())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "LightId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->lightId->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CameraId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->cameraId->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BasePixelDimension"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->basePixelDimension->getUInt())));
		propertiesXML->append_node(propertyXML);
	
		if (false == this->gameObjectPtr->getGlobal())
		{
			this->terra->saveTextures(Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName(), Core::getSingletonPtr()->getSceneName());
		}
		else
		{
			this->terra->saveTextures(Core::getSingletonPtr()->getCurrentProjectPath(), Core::getSingletonPtr()->getSceneName());
		}
	}

	void TerraComponent::handleEventDataGameObjectMadeGlobal(NOWA::EventDataPtr eventData)
	{
		if (nullptr == this->terra || true == this->terra->getHeightMapTextureName().empty())
		{
			return;
		}

		boost::shared_ptr<EventDataGameObjectMadeGlobal> castEventData = boost::static_pointer_cast<EventDataGameObjectMadeGlobal>(eventData);

		if (this->gameObjectPtr->getId() != castEventData->getGameObjectId())
		{
			return;
		}

		bool isGlobal = castEventData->getIsGlobal();
		if (false == isGlobal)
		{
			Ogre::String sourceTerraTextureFilePathName1 = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + this->terra->getHeightMapTextureName();
			Ogre::String destinationTerraTextureFilePathName1 = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + this->terra->getHeightMapTextureName();
			CopyFile(sourceTerraTextureFilePathName1.c_str(), destinationTerraTextureFilePathName1.c_str(), false);
			
			Ogre::String sourceTerraTextureFilePathName2 = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + this->terra->getBlendWeightTextureName();
			Ogre::String destinationTerraTextureFilePathName2 = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + this->terra->getBlendWeightTextureName();
			CopyFile(sourceTerraTextureFilePathName2.c_str(), destinationTerraTextureFilePathName2.c_str(), false);

			this->destroyTerra();

			try
			{
				DeleteFile(sourceTerraTextureFilePathName1.c_str());
				DeleteFile(sourceTerraTextureFilePathName2.c_str());
			}
			catch (...)
			{

			}
			
			this->createTerra();
		}
		else
		{
			Ogre::String sourceTerraTextureFilePathName1 = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + this->terra->getHeightMapTextureName();
			Ogre::String destinationTerraTextureFilePathName1 = Core::getSingletonPtr()->getCurrentProjectPath()  + "/" + this->terra->getHeightMapTextureName();
			CopyFile(sourceTerraTextureFilePathName1.c_str(), destinationTerraTextureFilePathName1.c_str(), false);
			
			Ogre::String sourceTerraTextureFilePathName2 = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + this->terra->getBlendWeightTextureName();
			Ogre::String destinationTerraTextureFilePathName2 = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + this->terra->getBlendWeightTextureName();
			CopyFile(sourceTerraTextureFilePathName2.c_str(), destinationTerraTextureFilePathName2.c_str(), false);

			this->destroyTerra();

			try
			{
				DeleteFile(sourceTerraTextureFilePathName1.c_str());
				DeleteFile(sourceTerraTextureFilePathName2.c_str());
			}
			catch (...)
			{

			}
			
			this->createTerra();
		}
	}

	void TerraComponent::setPixelSize(const Ogre::Vector2& pixelSize)
	{
		this->pixelSize->setValue(pixelSize);
		this->destroyTerra();
		this->createTerra();
	}

	Ogre::Vector2 TerraComponent::getPixelSize(void) const
	{
		return this->pixelSize->getVector2();
	}

	Ogre::String TerraComponent::getClassName(void) const
	{
		return "TerraComponent";
	}

	Ogre::String TerraComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	/*void TerraComponent::setHeightMap(const Ogre::String& heightMap)
	{
		this->heightMap->setValue(heightMap);
		this->destroyTerra();
		this->createTerra();
	}
	
	Ogre::String TerraComponent::getHeightMap(void) const
	{
		return this->heightMap->getString();
	}*/
	
	void TerraComponent::setDimensions(const Ogre::Vector3& dimensions)
	{
		this->dimensions->setValue(dimensions);
		this->destroyTerra();
		this->createTerra();
	}
		
	Ogre::Vector3 TerraComponent::getDimensions(void) const
	{
		return this->dimensions->getVector3();
	}
	
	void TerraComponent::setLightId(unsigned long lightId)
	{
		this->lightId->setValue(lightId);

		if (0 == lightId)
			return;

		GameObjectPtr lightGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->lightId->getULong());

		if (nullptr == lightGameObjectPtr)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[TerraComponent] Could not get game object for light from id: " + Ogre::StringConverter::toString(this->lightId->getULong())
				+ " for game object: " + this->gameObjectPtr->getName());
		}

		auto& lightDirectionalCompPtr = NOWA::makeStrongPtr(lightGameObjectPtr->getComponent<LightDirectionalComponent>());
		if (nullptr != lightDirectionalCompPtr)
		{
			this->sunLight = lightDirectionalCompPtr->getOgreLight();
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[TerraComponent] Could not get light component from id: " + Ogre::StringConverter::toString(this->lightId->getULong())
				+ " for game object: " + this->gameObjectPtr->getName());
		}
	}

	unsigned int TerraComponent::getLightId(void) const
	{
		return this->lightId->getULong();
	}

	void TerraComponent::setCameraId(unsigned long cameraId)
	{
		this->cameraId->setValue(cameraId);

		this->destroyTerra();
		this->createTerra();
	}

	unsigned int TerraComponent::geCameraId(void) const
	{
		return this->cameraId->getULong();
	}

	void TerraComponent::setBasePixelDimension(unsigned int basePixelDimension)
	{
		basePixelDimension = adjustToNextPowerOf2<unsigned int>(basePixelDimension);

		this->basePixelDimension->setValue(basePixelDimension);

		if (nullptr != this->terra)
		{
			this->terra->setBasePixelDimension(basePixelDimension);
		}
	}

	unsigned int TerraComponent::getBasePixelDimension(void) const
	{
		return this->basePixelDimension->getUInt();
	}

	void TerraComponent::setStrength(int strength)
	{
		this->strength->setValue(strength);
	}

	int TerraComponent::getStrength(void) const
	{
		return this->strength->getReal();
	}

	void TerraComponent::setBrushName(const Ogre::String& brushName)
	{
		this->brush->setListSelectedValue(brushName);

		if (nullptr != this->terra)
		{
			this->terra->setBrushName(brushName);
		}
	}

	Ogre::String TerraComponent::getBrushName(void) const
	{
		return this->brush->getListSelectedValue();
	}

	std::vector<Ogre::String> TerraComponent::getAllBrushNames(void) const
	{
		return this->brush->getList();
	}

	void TerraComponent::setBrushSize(short brushSize)
	{
		this->brushSize->setValue(brushSize);
		if (nullptr != this->terra)
		{
			this->terra->setBrushSize(brushSize);
			this->terra->setBrushName(this->brush->getListSelectedValue());
		}
	}

	short TerraComponent::getBrushSize(void) const
	{
		return static_cast<short>(this->brushSize->getInt());
	}

	void TerraComponent::setBrushIntensity(int brushIntensity)
	{
		this->brushIntensity->setValue(brushIntensity);
	}

	int TerraComponent::getBrushIntensity(void) const
	{
		return this->brushIntensity->getInt();
	}

	void TerraComponent::setImageLayer(const Ogre::String& imageLayer)
	{
		this->imageLayer->setListSelectedValue(imageLayer);
	}

	Ogre::String TerraComponent::getImageLayer(void) const
	{
		return this->imageLayer->getListSelectedValue();
	}

	unsigned int TerraComponent::getImageLayerId(void) const
	{
		return Ogre::StringConverter::parseUnsignedInt(this->imageLayer->getListSelectedValue());
	}

	void TerraComponent::setImageLayerId(unsigned int imageLayerId)
	{
		this->imageLayer->setListSelectedValue(Ogre::StringConverter::toString(imageLayerId));
	}

	std::vector<Ogre::String> TerraComponent::getAllImageLayer(void) const
	{
		return this->imageLayer->getList();
	}
	
	Ogre::Terra* TerraComponent::getTerra(void) const
	{
		return this->terra;
	}

}; // namespace end