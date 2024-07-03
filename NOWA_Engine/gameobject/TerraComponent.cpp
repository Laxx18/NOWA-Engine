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
		postInitDone(false),
		center(new Variant(TerraComponent::AttrCenter(), Ogre::Vector3(0.0f, 50.0f, 0.0f), this->attributes)),
		// heightMap(new Variant(TerraComponent::AttrHeightMap(), "Heightmap.png", this->attributes)),
		dimensions(new Variant(TerraComponent::AttrDimensions(), Ogre::Vector3(100.0f, 100.0f, 100.0f), this->attributes)),
		lightId(new Variant(TerraComponent::AttrLightId(), static_cast<unsigned long>(0), this->attributes, true)),
		strength(new Variant(TerraComponent::AttrStrength(), 10, this->attributes)),
		brushSize(new Variant(TerraComponent::AttrBrushSize(), static_cast<int>(64), this->attributes)),
		brushIntensity(new Variant(TerraComponent::AttrBrushIntensity(), static_cast<int>(255), this->attributes)),
		imageLayer(new Variant(TerraComponent::AttrImageLayer(), { "0", "1", "2", "3" }, this->attributes)),
		brush(new Variant(TerraComponent::AttrBrush(), this->attributes))
	{
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &TerraComponent::handleSwitchCamera), EventDataSwitchCamera::getStaticEventType());

		this->lightId->setDescription("The directional light game object id, in order to calculate light position for shading and shadows. Note: Initially no shadows are shown, if no light id is set!");

		Ogre::StringVectorPtr brushNames = Ogre::ResourceGroupManager::getSingleton().findResourceNames("Brushes", "*.png");
		std::vector<Ogre::String> compatibleBrushNames(brushNames.getPointer()->size() + 1);
		unsigned int i = 0;
		for (auto& it = brushNames.getPointer()->cbegin(); it != brushNames.getPointer()->cend(); it++)
		{
			compatibleBrushNames[i++] = *it;
		}

		this->strength->setConstraints(-100, 100);
		this->strength->addUserData(GameObject::AttrActionNoUndo());

		this->brush->setValue(compatibleBrushNames);
		this->brush->addUserData(GameObject::AttrActionImage());
		this->brush->addUserData(GameObject::AttrActionNoUndo());

		this->brushSize->setConstraints(1.0f, 128.0f);
		this->brushSize->addUserData(GameObject::AttrActionNoUndo());

		this->brushIntensity->setConstraints(1.0f, 255.0f);
		this->brushIntensity->addUserData(GameObject::AttrActionNoUndo());

		this->imageLayer->addUserData(GameObject::AttrActionNoUndo());
	}

	TerraComponent::~TerraComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[TerraComponent] Destructor terra component for game object: " + this->gameObjectPtr->getName());
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
		/*if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "HeightMap")
		{
			this->heightMap->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}*/
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
			const float lightEpsilon = 1.0f;
			if (nullptr != this->sunLight)
				this->terra->update(this->sunLight->getDerivedDirectionUpdated(), lightEpsilon);
			else
				this->terra->update(Ogre::Vector3::ZERO, lightEpsilon);
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
			this->terra = new Ogre::Terra(Ogre::Id::generateNewId<Ogre::MovableObject>(), &this->gameObjectPtr->getSceneManager()->_getEntityMemoryManager(Ogre::SCENE_STATIC),
				this->gameObjectPtr->getSceneManager(), NOWA::RENDER_QUEUE_TERRA, WorkspaceModule::getInstance()->getCompositorManager(), AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera(), false);
			// Attention: Shadows must not be casted for terra, else ugly crash shader cache is created
			this->terra->setCastShadows(false);
			this->terra->setName(this->gameObjectPtr->getName());
			this->terra->setPrefix(Core::getSingletonPtr()->getWorldName());

			// There can be up to 4 blend maps! one for texture state in datablock!
			Ogre::String existingDetailMapFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getWorldName() + "_detailMap.png";
			std::fstream detailMapFile(existingDetailMapFilePathName);
			if (false == detailMapFile.good())
			{
				existingDetailMapFilePathName = "";
			}

			Ogre::String existingHeightMapFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getWorldName() + "_heightMap.png";
			std::fstream heightMapFile(existingHeightMapFilePathName);
			if (true == heightMapFile.good())
			{
				heightMapFile.close();
				// true memory sharing when having more than one terra!
				this->terra->load(existingHeightMapFilePathName, existingDetailMapFilePathName, this->center->getVector3(), this->dimensions->getVector3(), true, false);
			}
			else
			{
				this->terra->load(this->center->getVector3().y, 1024u, 1024u, this->center->getVector3(), this->dimensions->getVector3(), true, false);
			}

			// this->terra->load(0.0f, 1024u, 1024u, Ogre::Vector3(this->gameObjectPtr->getPosition().x, this->gameObjectPtr->getPosition().y + this->dimensions->getVector3().y / 2.0f, this->gameObjectPtr->getPosition().z), this->dimensions->getVector3());

			// this->terra->levelTerrain(5.0f);
			//mTerra->load( "Heightmap.png", Ogre::Vector3( 64.0f, 0, 64.0f ), Ogre::Vector3( 1024.0f, 5.0f, 1024.0f ) );
			//mTerra->load( "Heightmap.png", Ogre::Vector3( 64.0f, 0, 64.0f ), Ogre::Vector3( 4096.0f * 4, 15.0f * 64.0f*4, 4096.0f * 4 ) );
			// this->terra->load("Heightmap.png", Ogre::Vector3(64.0f, 496.0f * 0.5f, 64.0f), Ogre::Vector3(496.0f, 496.0f, 496.0f));
			//mTerra->load( "Heightmap.png", Ogre::Vector3( 64.0f, 4096.0f * 0.5f, 64.0f ), Ogre::Vector3( 14096.0f, 14096.0f, 14096.0f ) );

// Later get corresponding camera and workspacecomponent and set data from terra?
#if 0
			Ogre::String samplerTextureName;
			// Get Main Camera to get its sky box name
			GameObjectPtr mainCameraGameObject = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromName("MainCamera");
			if (nullptr != mainCameraGameObject)
			{
				auto& cameraCompPtr = NOWA::makeStrongPtr(mainCameraGameObject->getComponent<CameraComponent>());
				if (nullptr != cameraCompPtr)
				{
					// Set the terra workspace name for the camera
					cameraCompPtr->setWorkspaceName(WorkspaceModule::getInstance()->workspaceNameTerra);
					samplerTextureName = cameraCompPtr->getSkyBoxName();
				}
			}


			// Recreate new workspace for terra specifigs
// Attention: There is no viewport yet!
			WorkspaceModule::getInstance()->createPrimaryWorkspaceModule(AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera(), WorkspaceModule::getInstance()->workspaceNameTerra, this->gameObjectPtr->getSceneManager(),
				samplerTextureName, Ogre::ColourValue(0.2f, 0.4f, 0.6f), Ogre::Vector4(0.0f, 0.0f, 1.0f, 1.0f), this->terra);

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

			auto aabb = Ogre::Aabb::newFromExtents(Ogre::Vector3(startX - this->center->getVector3().x, -(this->dimensions->getVector3().y * 0.5f) + (this->center->getVector3().y), startZ - this->center->getVector3().z),
												   Ogre::Vector3(endX - this->center->getVector3().x,    (this->dimensions->getVector3().y * 0.5f) + (this->center->getVector3().y), endZ - this->center->getVector3().z));
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

					if(nullptr == this->terraWorkspaceListener)
					{
						Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingletonPtr()->getHlmsManager();
						Ogre::Hlms* hlms = hlmsManager->getHlms(Ogre::HLMS_USER3);
						OGRE_ASSERT_HIGH(dynamic_cast<HlmsTerra*>(hlms));
						this->terraWorkspaceListener = new Ogre::TerraWorkspaceListener((Ogre::HlmsTerra*)hlms);
					}
					workspaceBaseComponent->getWorkspace()->addListener(this->terraWorkspaceListener);
				}
			}
		}
	}

	void TerraComponent::modifyTerrainStart(const Ogre::Vector3& position, float strength)
	{
		if (nullptr != this->terra)
		{
			return this->terra->modifyTerrainStart(position, strength);
		}
	}

	void TerraComponent::smoothTerrainStart(const Ogre::Vector3& position, float strength)
	{
		if (nullptr != this->terra)
		{
			return this->terra->smoothTerrainStart(position, strength);
		}
	}

	void TerraComponent::paintTerrainStart(const Ogre::Vector3& position, float intensity, int blendLayer)
	{
		if (nullptr != this->terra)
		{
			return this->terra->paintTerrainStart(position, intensity, blendLayer);
		}
	}

	void TerraComponent::modifyTerrain(const Ogre::Vector3& position, float strength)
	{
		if (nullptr != this->terra)
		{
			this->terra->modifyTerrain(position, strength);
		}
	}

	void TerraComponent::smoothTerrain(const Ogre::Vector3& position, float strength)
	{
		if (nullptr != this->terra)
		{
			this->terra->smoothTerrain(position, strength);
		}
	}

	void TerraComponent::paintTerrain(const Ogre::Vector3& position, float intensity, int blendLayer)
	{
		if (nullptr != this->terra)
		{
			this->terra->paintTerrain(position, intensity, blendLayer);
		}
	}

	std::pair<std::vector<Ogre::uint16>, std::vector<Ogre::uint16>> TerraComponent::modifyTerrainFinished(void)
	{
		std::pair<std::vector<Ogre::uint16>, std::vector<Ogre::uint16>> heightData;

		if (nullptr != this->terra)
		{
			heightData = this->terra->modifyTerrainFinished();
			auto& component = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsTerrainComponent>());
			if (nullptr != component)
			{
				component->reCreateCollision();
			}
		}
		
		return std::move(heightData);
	}

	std::pair<std::vector<Ogre::uint16>, std::vector<Ogre::uint16>> TerraComponent::smoothTerrainFinished(void)
	{
		std::pair<std::vector<Ogre::uint16>, std::vector<Ogre::uint16>> heightData;

		if (nullptr != this->terra)
		{
			heightData = this->terra->smoothTerrainFinished();
			auto& component = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsTerrainComponent>());
			if (nullptr != component)
			{
				component->reCreateCollision();
			}
		}
		
		return std::move(heightData);
	}

	std::pair<std::vector<Ogre::uint8>, std::vector<Ogre::uint8>> TerraComponent::paintTerrainFinished(void)
	{
		std::pair<std::vector<Ogre::uint8>, std::vector<Ogre::uint8>> detailBlendData;

		if (nullptr != this->terra)
		{
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

			auto& component = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsTerrainComponent>());
			if (nullptr != component)
			{
				component->reCreateCollision();
			}
		}
	}

	void TerraComponent::setBlendWeightData(const std::vector<Ogre::uint8>& blendWeightData)
	{
		if (nullptr != this->terra)
		{
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
		/*else if (TerraComponent::AttrHeightMap() == attribute->getName())
		{
			this->setHeightMap(attribute->getString());
		}*/
		else if (TerraComponent::AttrDimensions() == attribute->getName())
		{
			this->setDimensions(attribute->getVector3());
		}
		else if (TerraComponent::AttrLightId() == attribute->getName())
		{
			this->setLightId(attribute->getULong());
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
		
		/*propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "HeightMap"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->heightMap->getString())));
		propertiesXML->append_node(propertyXML);*/

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

	
		this->terra->saveTextures(Core::getSingletonPtr()->getCurrentProjectPath(), Core::getSingletonPtr()->getWorldName());
		
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
	
	Ogre::Terra* TerraComponent::getTerra(void) const
	{
		return this->terra;
	}

}; // namespace end