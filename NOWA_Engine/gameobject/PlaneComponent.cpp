#include "NOWAPrecompiled.h"
#include "PlaneComponent.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "PhysicsArtifactComponent.h"
#include "DatablockPbsComponent.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	PlaneComponent::PlaneComponent()
		: GameObjectComponent(),
		clonedDatablock(nullptr),
		distance(new Variant(PlaneComponent::AttrDistance(), 0.0f, this->attributes)),
		width(new Variant(PlaneComponent::AttrWidth(), 50.0f, this->attributes)),
		height(new Variant(PlaneComponent::AttrHeight(), 50.0f, this->attributes)),
		xSegments(new Variant(PlaneComponent::AttrXSegments(), 2, this->attributes)),
		ySegments(new Variant(PlaneComponent::AttrYSegments(), 2, this->attributes)),
		numTexCoordSets(new Variant(PlaneComponent::AttrNumTexCoordSets(), 1, this->attributes)),
		uTile(new Variant(PlaneComponent::AttrUTile(), 20.0f, this->attributes)),
		vTile(new Variant(PlaneComponent::AttrVTile(), 20.0f, this->attributes)),
		normal(new Variant(PlaneComponent::AttrNormal(), Ogre::Vector3(0.0f, 1.0f, 0.0f), this->attributes)),
		up(new Variant(PlaneComponent::AttrUp(), Ogre::Vector3(0.0f, 0.0f, 1.0f), this->attributes))
	{
		
	}

	PlaneComponent::~PlaneComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlaneComponent] Destructor plane component for game object: " + this->gameObjectPtr->getName());
		if (false == this->dataBlockName.empty())
		{
			this->gameObjectPtr->getMovableObjectUnsafe<Ogre::Item>()->setDatablock(this->dataBlockName);
		}
		// If datablock has been cloned, set original data block name for item and destroy the cloned one, else crash will occur
		// (is used by renderable, or datablock cannot be cloned, because same name does exist, if switching between scenes).
		if (nullptr != this->clonedDatablock)
		{
			this->clonedDatablock->getCreator()->destroyDatablock(this->clonedDatablock->getName());
		}
	}

	bool PlaneComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Distance")
		{
			this->distance->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Width")
		{
			this->width->setValue(XMLConverter::getAttribReal(propertyElement, "data", 100.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Height")
		{
			this->height->setValue(XMLConverter::getAttribReal(propertyElement, "data", 100.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "XSegments")
		{
			this->xSegments->setValue(XMLConverter::getAttribInt(propertyElement, "data", 2));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "YSegments")
		{
			this->ySegments->setValue(XMLConverter::getAttribInt(propertyElement, "data", 2));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "NumTexCoordSets")
		{
			this->numTexCoordSets->setValue(XMLConverter::getAttribInt(propertyElement, "data", 1));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UTile")
		{
			this->uTile->setValue(XMLConverter::getAttribReal(propertyElement, "data", 100.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "VTile")
		{
			this->vTile->setValue(XMLConverter::getAttribReal(propertyElement, "data", 100.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Normal")
		{
			this->normal->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(0.0f, 1.0f, 0.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Up")
		{
			this->up->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(0.0f, 0.0f, 1.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr PlaneComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		PlaneCompPtr clonedCompPtr(boost::make_shared<PlaneComponent>());

		clonedCompPtr->setDatablockName(this->dataBlockName);

		clonedCompPtr->setDistance(this->distance->getReal());
		clonedCompPtr->setWidth(this->width->getReal());
		clonedCompPtr->setHeight(this->height->getReal());
		clonedCompPtr->setXSegments(this->xSegments->getInt());
		clonedCompPtr->setYSegments(this->ySegments->getInt());
		clonedCompPtr->setNumTexCoordSets(this->numTexCoordSets->getInt());
		clonedCompPtr->setUTile(this->uTile->getReal());
		clonedCompPtr->setVTile(this->vTile->getReal());
		clonedCompPtr->setNormal(this->normal->getVector3());
		clonedCompPtr->setUp(this->up->getVector3());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool PlaneComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlaneComponent] Init plane component for game object: " + this->gameObjectPtr->getName());

		this->gameObjectPtr->getAttribute(GameObject::AttrClampY())->setVisible(false);

		this->createPlane();
		return true;
	}

	void PlaneComponent::createPlane(void)
	{
		// Plane cannot be moved if static, so let it be dynamic and user will set it static afterwards when it is no more moved
		// this->gameObjectPtr->getSceneNode()->setStatic(true);

		Ogre::String meshName = this->gameObjectPtr->getName();
		Ogre::Plane plane(this->normal->getVector3(), this->distance->getReal());
		Ogre::ResourcePtr resourceV1 = Ogre::v1::MeshManager::getSingletonPtr()->getResourceByName(meshName);
		// Destroy a potential plane v1, because an error occurs (plane with name ... already exists)
		if (nullptr != resourceV1)
		{
			Ogre::v1::MeshManager::getSingletonPtr()->destroyResourcePool(meshName);
			Ogre::v1::MeshManager::getSingletonPtr()->remove(resourceV1->getHandle());
		}

		// Destroy a potential plane v2, because an error occurs (plane with name ... already exists)
		Ogre::ResourcePtr resourceV2 = Ogre::MeshManager::getSingletonPtr()->getResourceByName(meshName);
		if (nullptr != resourceV2)
		{
			Ogre::MeshManager::getSingletonPtr()->destroyResourcePool(meshName);
			Ogre::MeshManager::getSingletonPtr()->remove(resourceV2->getHandle());
		}

		Ogre::v1::MeshPtr planeMeshV1 = Ogre::v1::MeshManager::getSingletonPtr()->createPlane(meshName, "General", plane, this->width->getReal(), this->height->getReal(),
			this->xSegments->getInt(), this->ySegments->getInt(), true,
			this->numTexCoordSets->getInt(), this->uTile->getReal(), this->vTile->getReal(), this->up->getVector3(), Ogre::v1::HardwareBuffer::HBU_STATIC, Ogre::v1::HardwareBuffer::HBU_STATIC);

		Ogre::MeshPtr planeMesh = Ogre::MeshManager::getSingletonPtr()->createByImportingV1(meshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, planeMeshV1.get(), true, true, true);
		planeMeshV1->unload();
		planeMeshV1.setNull();

		// Check whether to cast shadows
		bool castShadows = true;
		bool visible = true;

		if (nullptr != this->gameObjectPtr->getMovableObject())
		{
			castShadows = this->gameObjectPtr->getMovableObject()->getCastShadows();
			visible = this->gameObjectPtr->getMovableObject()->getVisible();
		}

		// Later: Make scene node and entity static!
		Ogre::Item* item = this->gameObjectPtr->getSceneManager()->createItem(planeMesh, Ogre::SCENE_DYNAMIC);

		this->gameObjectPtr->setDynamic(true);
		item->setCastShadows(castShadows);
		this->gameObjectPtr->getSceneNode()->attachObject(item);

		// Set the here newly created entity for this game object
		this->gameObjectPtr->init(item);

		// Get the used data block name 0
		auto datablockAttribute = this->gameObjectPtr->getAttribute(GameObject::AttrDataBlock() + "0");
		if (nullptr != datablockAttribute)
		{
			Ogre::String dataBlock = datablockAttribute->getString();
			if ("Missing" != dataBlock)
				item->setDatablock(dataBlock);
		}

		item->setName(this->gameObjectPtr->getName() + "mesh");

		// When plane is re-created actualize the data block component so that the plane gets the data block
		auto& datablockPbsComponent = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<DatablockPbsComponent>());
		if (nullptr != datablockPbsComponent)
		{
			Ogre::HlmsPbsDatablock* datablock = datablockPbsComponent->getDataBlock();
			if (nullptr != datablock)
			{
				const Ogre::HlmsSamplerblock* tempSamplerBlock = datablock->getSamplerblock(Ogre::PBSM_ROUGHNESS);
				if (nullptr != tempSamplerBlock)
				{
					Ogre::HlmsSamplerblock samplerblock(*datablock->getSamplerblock(Ogre::PBSM_ROUGHNESS));
					samplerblock.mU = Ogre::TAM_WRAP;
					samplerblock.mV = Ogre::TAM_WRAP;
					samplerblock.mW = Ogre::TAM_WRAP;
					//Set the new samplerblock. The Hlms system will
					//automatically create the API block if necessary
					datablock->setSamplerblock(Ogre::PBSM_ROUGHNESS, samplerblock);
				}

				item->setDatablock(datablock);
			}
		}
		else
		{
			// Get the used data block name, if not set
			if (true == this->dataBlockName.empty())
			{
				this->dataBlockName = this->gameObjectPtr->getAttribute(GameObject::AttrDataBlock() + "0")->getString();
			}
			Ogre::HlmsPbsDatablock* sourceDataBlock = static_cast<Ogre::HlmsPbsDatablock*>(Ogre::Root::getSingletonPtr()->getHlmsManager()->getDatablock(this->dataBlockName));
			if (nullptr != sourceDataBlock)
			{
				Ogre::String clonedDatablockName = *sourceDataBlock->getNameStr() + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
				bool duplicateItem = false;

				do
				{
					try
					{
						this->clonedDatablock = sourceDataBlock->clone(clonedDatablockName);
						duplicateItem = false;
					}
					catch (const Ogre::Exception& exception)
					{
						duplicateItem = true;
						clonedDatablockName = *sourceDataBlock->getNameStr() + Ogre::StringConverter::toString(this->gameObjectPtr->getId()) + "_" + Ogre::StringConverter::toString(static_cast<int>(Ogre::Math::RangeRandom(0.0f, 100.0f)));
					}
				} while (true == duplicateItem);

				if (nullptr != this->clonedDatablock)
				{
					item->setDatablock(this->clonedDatablock);
				}
			}
			
			for (size_t i = 0; i < item->getNumSubItems(); i++)
			{
				auto sourceDataBlock = dynamic_cast<Ogre::HlmsPbsDatablock*>(item->getSubItem(i)->getDatablock());
				if (nullptr != sourceDataBlock)
				{
					// Deactivate fresnel by default, because it looks ugly
					if (sourceDataBlock->getWorkflow() != Ogre::HlmsPbsDatablock::SpecularAsFresnelWorkflow && sourceDataBlock->getWorkflow() != Ogre::HlmsPbsDatablock::MetallicWorkflow)
					{
						sourceDataBlock->setFresnel(Ogre::Vector3(0.01f, 0.01f, 0.01f), false);
					}
				}
			}
		}

		item->setVisible(visible);
		this->gameObjectPtr->getSceneNode()->setVisible(visible);

		// Check if a physics artifact component does exist for this game object and recreate the collision shape when the plane has changed
		auto& physicsArtifactComponent = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsArtifactComponent>());
		if (nullptr != physicsArtifactComponent)
		{
// Attention: what is with joints in post init, is it required to recreate also the joints, if the collision does change for the physics body??????????
			if (nullptr != physicsArtifactComponent->getBody())
			{
				physicsArtifactComponent->reCreateCollision();
			}
			else
			{
				physicsArtifactComponent->createStaticBody();
			}
		}

		// - 1 else other v2 items will not by visible
		this->gameObjectPtr->setRenderQueueIndex(NOWA::RENDER_QUEUE_V2_MESH - 1);

		// Register after the component has been created
		AppStateManager::getSingletonPtr()->getGameObjectController()->registerGameObject(gameObjectPtr);
	}

	void PlaneComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (PlaneComponent::AttrDistance() == attribute->getName())
		{
			this->setDistance(attribute->getReal());
		}
		else if (PlaneComponent::AttrWidth() == attribute->getName())
		{
			this->setWidth(attribute->getReal());
		}
		else if (PlaneComponent::AttrHeight() == attribute->getName())
		{
			this->setHeight(attribute->getReal());
		}
		else if (PlaneComponent::AttrXSegments() == attribute->getName())
		{
			this->setXSegments(attribute->getInt());
		}
		else if (PlaneComponent::AttrYSegments() == attribute->getName())
		{
			this->setYSegments(attribute->getInt());
		}
		else if (PlaneComponent::AttrNumTexCoordSets() == attribute->getName())
		{
			this->setNumTexCoordSets(attribute->getInt());
		}
		else if (PlaneComponent::AttrUTile() == attribute->getName())
		{
			this->setUTile(attribute->getReal());
		}
		else if (PlaneComponent::AttrVTile() == attribute->getName())
		{
			this->setVTile(attribute->getReal());
		}
		else if (PlaneComponent::AttrNormal() == attribute->getName())
		{
			this->setNormal(attribute->getVector3());
		}
		else if (PlaneComponent::AttrUp() == attribute->getName())
		{
			this->setUp(attribute->getVector3());
		}
	}

	void PlaneComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Distance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->distance->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Width"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->width->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Height"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->height->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "XSegments"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->xSegments->getInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "YSegments"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ySegments->getInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "NumTexCoordSets"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->numTexCoordSets->getInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UTile"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->uTile->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "VTile"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->vTile->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Normal"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->normal->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Up"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->up->getVector3())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String PlaneComponent::getClassName(void) const
	{
		return "PlaneComponent";
	}

	Ogre::String PlaneComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void PlaneComponent::setDistance(Ogre::Real distance)
	{
		this->distance->setValue(distance);
		this->createPlane();
	}

	Ogre::Real PlaneComponent::getDistance(void) const
	{
		return this->distance->getReal();
	}

	void PlaneComponent::setWidth(Ogre::Real width)
	{
		this->width->setValue(width);
		this->createPlane();
	}

	Ogre::Real PlaneComponent::getWidth(void) const
	{
		return this->width->getReal();
	}

	void PlaneComponent::setHeight(Ogre::Real height)
	{
		this->height->setValue(height);
		this->createPlane();
	}

	Ogre::Real PlaneComponent::getHeight(void) const
	{
		return this->height->getReal();
	}

	void PlaneComponent::setXSegments(int xSegments)
	{
		this->xSegments->setValue(xSegments);
		this->createPlane();
	}

	int PlaneComponent::getXSegments(void) const
	{
		return this->xSegments->getInt();
	}

	void PlaneComponent::setYSegments(int ySegments)
	{
		this->ySegments->setValue(ySegments);
		this->createPlane();
	}

	int PlaneComponent::getYSegments(void) const
	{
		return this->ySegments->getInt();
	}

	void PlaneComponent::setNumTexCoordSets(int numTexCoordSets)
	{
		this->numTexCoordSets->setValue(numTexCoordSets);
		this->createPlane();
	}

	int PlaneComponent::getNumTexCoordSets(void) const
	{
		return this->numTexCoordSets->getInt();
	}

	void PlaneComponent::setUTile(Ogre::Real uTile)
	{
		this->uTile->setValue(uTile);
		this->createPlane();
	}

	Ogre::Real PlaneComponent::getUTile(void) const
	{
		return this->uTile->getReal();
	}

	void PlaneComponent::setVTile(Ogre::Real vTile)
	{
		this->vTile->setValue(vTile);
		this->createPlane();
	}

	Ogre::Real PlaneComponent::getVTile(void) const
	{
		return this->vTile->getReal();
	}

	void PlaneComponent::setNormal(const Ogre::Vector3& normal)
	{
		this->normal->setValue(normal);
		this->createPlane();
	}

	Ogre::Vector3 PlaneComponent::getNormal(void) const
	{
		return this->normal->getVector3();
	}

	void PlaneComponent::setUp(const Ogre::Vector3& up)
	{
		this->up->setValue(up);
		this->createPlane();
	}

	Ogre::Vector3 PlaneComponent::getUp(void) const
	{
		return this->up->getVector3();
	}

	void PlaneComponent::setDatablockName(const Ogre::String& datablockName)
	{
		this->dataBlockName = datablockName;
	}

	Ogre::String PlaneComponent::getDatablockName(void) const
	{
		return this->dataBlockName;
	}

}; // namespace end