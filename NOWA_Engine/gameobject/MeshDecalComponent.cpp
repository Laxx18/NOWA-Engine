#include "NOWAPrecompiled.h"
#include "MeshDecalComponent.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "modules/DeployResourceModule.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	MeshDecalComponent::MeshDecalComponent()
		: GameObjectComponent(),
		meshDecal(nullptr),
		updateTimer(0.0f),
		oldPosition(Ogre::Vector3::ZERO),
		oldOrientation(Ogre::Quaternion::IDENTITY),
		activated(new Variant(MeshDecalComponent::AttrActivated(), true, this->attributes)),
		targetId(new Variant(MeshDecalComponent::AttrTargetId(), static_cast<unsigned long>(0), this->attributes, true)),
		textureName(new Variant(MeshDecalComponent::AttrTextureName(), "Decal4", this->attributes)),
		size(new Variant(MeshDecalComponent::AttrSize(), Ogre::Vector2(10.0f, 10.0f), this->attributes)),
		updateFrequency(new Variant(MeshDecalComponent::AttrUpdateFrequency(), 0.5f, this->attributes))
	{
		this->updateFrequency->setDescription("The frequency in seconds at which the decal will be updated. When set to 0, it will not be updated.");
	}

	MeshDecalComponent::~MeshDecalComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshDecalComponent] Destructor mesh decal component for game object: " + this->gameObjectPtr->getName());
		// this->destroyMeshDecal();
	}

	bool MeshDecalComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetId")
		{
			this->targetId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TextureName")
		{
			this->textureName->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Size")
		{
			this->size->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UpdateFrequency")
		{
			this->updateFrequency->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr MeshDecalComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		MeshDecalCompPtr clonedCompPtr(boost::make_shared<MeshDecalComponent>());

		
		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setTargetId(this->targetId->getULong());
		clonedCompPtr->setTextureName(this->textureName->getString());
		clonedCompPtr->setSize(this->size->getVector2());
		clonedCompPtr->setUpdateFrequency(this->updateFrequency->getReal());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool MeshDecalComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshDecalComponent] Init mesh decal component for game object: " + this->gameObjectPtr->getName());

		// this->createMeshDecal();
		return true;
	}
	
	bool MeshDecalComponent::connect(void)
	{
		this->oldPosition = this->gameObjectPtr->getPosition();
		this->oldOrientation = this->gameObjectPtr->getOrientation();
		// Must be resetted, because the decal must be rebuild at origin
		this->gameObjectPtr->setAttributePosition(Ogre::Vector3(0.0f, 0.1f, 0.0f));
		this->gameObjectPtr->setAttributeOrientation(Ogre::Quaternion::IDENTITY);
		return true;
	}

	bool MeshDecalComponent::disconnect(void)
	{
		this->gameObjectPtr->setAttributePosition(this->oldPosition);
		this->gameObjectPtr->setAttributeOrientation(this->oldOrientation);
		return true;
	}
	
	bool MeshDecalComponent::onCloned(void)
	{
		// Search for the prior id of the cloned game object and set the new id and set the new id, if not found set better 0, else the game objects may be corrupt!
		if (0 != this->targetId->getULong())
		{
			GameObjectPtr targetGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(this->targetId->getULong());
			if (nullptr != targetGameObjectPtr)
			{
				this->targetId->setValue(targetGameObjectPtr->getId());
			}
			// Since connect is called during cloning process, it does not make sense to process further here, but only when simulation started!
		}
		return true;
	}

	void MeshDecalComponent::update(Ogre::Real dt, bool notSimulating)
	{
		//if (true == this->activated->getBool() && false == notSimulating)
		//{
		//	if (this->updateFrequency->getReal() > 0.0f)
		//	{
		//		this->updateTimer += dt;
		//	}
		//	// check area only 2x a second
		//	if (this->updateTimer >= this->updateFrequency->getReal())
		//	{
		//		this->updateTimer = 0.0f;

		//		if (false == this->oldPosition.positionEquals(this->gameObjectPtr->getPosition()))
		//		{
		//			this->meshDecal->moveAndRotateMeshDecalOnMesh(this->gameObjectPtr->getPosition(), 1, this->categoryId);
		//			this->oldPosition = this->gameObjectPtr->getPosition();
		//		}
		//	}
		//}
	}
	
	void MeshDecalComponent::destroyMeshDecal(void)
	{
// Attention: Is that correct?
		if (nullptr != this->meshDecal)
		{
			delete this->meshDecal;
			this->meshDecal = nullptr;
		}
	}

	void MeshDecalComponent::createMeshDecal(const Ogre::Vector3& position)
	{
		// this->destroyMeshDecal();
		
		/// Get the DecalGenerator singleton and initialize it
		//MeshDecalGeneratorModule::getInstance()->initialize(this->gameObjectPtr->getSceneManager());
		//
		///// This is the mesh object that we will pass to the decal generator.
		///// Do not create any more than one OgreMesh per mesh, even if you have multiple instances of the same mesh in your scene.
		//OgreMesh mesh;
		//
		//GameObjectPtr targetGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(targetAgentId);
		//if (nullptr != targetGameObjectPtr)
		//{
		//	Ogre::v1::Entity* targetEntity = targetGameObjectPtr->getMovableObject<Ogre::v1::Entity*>();
		//	if (nullptr != targetEntity)
		//	{
		//		/// This method will extract all of the triangles from the mesh to be used later. Only should be called once.
		//		/// If you scale your mesh at all, pass it in here.
		//		mesh.initialize(targetEntity->getMesh(), targetGameObjectPtr->getScale());
		//	}
		//	else
		//	{
		//		return;
		//	}
		//}
		//else
		//{
		//	return;
		//}
		//
		//// this->meshDecal = new MeshDecal(this->gameObjectPtr->getSceneManager(), this->gameObjectPtr->getSceneNode());
		//// Ogre::ManualObject* manualObject = this->meshDecal->createMeshDecal(this->datablockName->getString(), this->numPartitions->getUInt(), this->numPartitions->getUInt());

		//// auto data = DeployResourceModule::getInstance()->getPathAndResourceGroupFromDatablock(this->textureName->getString(), Ogre::HlmsTypes::HLMS_UNLIT);

		//// DeployResourceModule::getInstance()->tagResource(this->datablockName->getString(), data.first, data.second);

		///// Set Decal parameters:
		//Ogre::Vector3 pos = getRaycastPoint(); /// Send a ray into the mesh
		//// std::string textureName = "MyTexture"; /// Make sure your texture has a depth_bias greater than 1 to avoid z-fighting

		///// We have everything ready to go. Now it's time to actually generate the decal:
		//if (nullptr == this->meshDecal)
		//{
		//	this->meshDecal = MeshDecalGeneratorModule::getInstance()->createDecal(&mesh, position, this->size->getVector2().x, this->size->getVector2().y, this->textureName->getString());
		//	/// Render the decal object. Always verify the returned object - it will be NULL if no decal could be created.
		//	if (this->meshDecal)
		//	{
		//		this->gameObjectPtr->getSceneNode()->attachObject(decal.object);
		//	
		//		// Manual object will be destroyed inside MeshDecal
		//		this->gameObjectPtr->setDoNotDestroyMovableObject(true);

		//		// manualObject->setVisible(this->gameObjectPtr->getAttribute(GameObject::AttrVisible())->getBool());
		//		// manualObject->setCastShadows(this->gameObjectPtr->getAttribute(GameObject::AttrCastShadows())->getBool());

		//		this->gameObjectPtr->init(this->meshDecal);
		//		// Register after the component has been created
		//		AppStateManager::getSingletonPtr()->getGameObjectController()->registerGameObject(gameObjectPtr);
		//	}
		//}
		//else
		//{
		//	this->meshDecal = MeshDecalGeneratorModule::getInstance()->createDecal(&mesh, pos, this->size->getVector2().x, this->size->getVector2().y, this->textureName->getString(), this->meshDecal);
		//}
	}

	void MeshDecalComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (MeshDecalComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (MeshDecalComponent::AttrTargetId() == attribute->getName())
		{
			this->setTargetId(attribute->getULong());
		}
		else if (MeshDecalComponent::AttrTextureName() == attribute->getName())
		{
			this->setTextureName(attribute->getString());
		}
		else if (MeshDecalComponent::AttrSize() == attribute->getName())
		{
			this->setSize(attribute->getVector2());
		}
		else if (MeshDecalComponent::AttrUpdateFrequency() == attribute->getName())
		{
			this->setUpdateFrequency(attribute->getReal());
		}
	}

	void MeshDecalComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->activated->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TargetId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->targetId->getULong())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Size"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->size->getVector2())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UpdateFrequency"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->updateFrequency->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String MeshDecalComponent::getClassName(void) const
	{
		return "MeshDecalComponent";
	}

	Ogre::String MeshDecalComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}
	
	void MeshDecalComponent::setActivated(bool activated)
	{
		/*this->updateTimer = 0.0f;
		this->activated->setValue(activated);
		if (nullptr != this->meshDecal)
			this->meshDecal->show(activated);*/
	}

	bool MeshDecalComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}
	
	void MeshDecalComponent::setTargetId(unsigned long targetId)
	{
		this->targetId->setValue(targetId);
	}

	unsigned long MeshDecalComponent::getTargetId(void) const
	{
		return this->targetId->getULong();
	}
	
	void MeshDecalComponent::setTextureName(const Ogre::String& textureName)
	{
		/*this->textureName->setValue(textureName);
		this->createMeshDecal();*/
	}
	
	Ogre::String MeshDecalComponent::getTextureName(void) const
	{
		return this->textureName->getString();
	}
	
	void MeshDecalComponent::setSize(const Ogre::Vector2& size)
	{
		this->size->setValue(size);
		// this->createMeshDecal();
	}
	
	Ogre::Vector2 MeshDecalComponent::getSize(void) const
	{
		return this->textureName->getVector2();
	}
	
	void MeshDecalComponent::setUpdateFrequency(Ogre::Real updateFrequency)
	{
		this->updateFrequency->setValue(updateFrequency);
	}

	Ogre::Real MeshDecalComponent::getUpdateFrequency(void) const
	{
		return this->updateFrequency->getReal();
	}
	
	Ogre::ManualObject* MeshDecalComponent::getMeshDecal(void) const
	{
		return this->meshDecal;
	}

	void MeshDecalComponent::setPosition(const Ogre::Vector3& position)
	{
		//if (nullptr != this->meshDecal)
		//{
		//	// this->gameObjectPtr->setPos
		//	this->meshDecal->moveMeshDecalOnMesh(position, 1, this->categoryId);
		//}
	}

	void MeshDecalComponent::setMousePosition(int x, int y)
	{
		/*if (nullptr != this->meshDecal.object)
		{
			this->meshDecal.object->moveMeshDecalOnMesh(x, y, 1, this->categoryId);
		}*/
	}

}; // namespace end