#include "NOWAPrecompiled.h"
#include "BillboardComponent.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "modules/DeployResourceModule.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	BillboardComponent::BillboardComponent()
		: GameObjectComponent(),
		billboardSet(nullptr),
		billboard(nullptr),
		activated(new Variant(BillboardComponent::AttrActivated(), true, this->attributes)),
		position(new Variant(BillboardComponent::AttrPosition(), Ogre::Vector3::ZERO, this->attributes)),
		dimensions(new Variant(BillboardComponent::AttrDimensions(), Ogre::Vector2(1.0f, 1.0f), this->attributes)),
		color(new Variant(BillboardComponent::AttrColor(), Ogre::Vector3::UNIT_SCALE, this->attributes)),
		renderDistance(new Variant(BillboardComponent::AttrRenderDistance(), 10000.0f, this->attributes))
	{
		this->color->addUserData(GameObject::AttrActionColorDialog());
		
		/*
		 BBO_TOP_LEFT,
        BBO_TOP_CENTER,
        BBO_TOP_RIGHT,
        BBO_CENTER_LEFT,
        BBO_CENTER,
        BBO_CENTER_RIGHT,
        BBO_BOTTOM_LEFT,
        BBO_BOTTOM_CENTER,
        BBO_BOTTOM_RIGHT
		*/

		std::vector<Ogre::String> origins(9);
		origins[0] = "Top Left";
		origins[1] = "Top Center";
		origins[2] = "Top Right";
		origins[3] = "Center Left";
		origins[4] = "Center";
		origins[5] = "Center Right";
		origins[6] = "Bottom Left";
		origins[7] = "Bottom Center";
		origins[8] = "Bottom Right";
		this->origin = new Variant(BillboardComponent::AttrOrigin(), origins, this->attributes);

		/*
		/// Rotate the billboard's vertices around their facing direction
        BBR_VERTEX,
        /// Rotate the billboard's texture coordinates
        BBR_TEXCOORD
		*/
		
		std::vector<Ogre::String> rotationTypes(2);
		rotationTypes[0] = "Vertex";
		rotationTypes[1] = "TexCoord";
		this->rotationType = new Variant(BillboardComponent::AttrRotationType(), rotationTypes, this->attributes);

		/*
		BBT_POINT,
        /// Billboards are oriented around a shared direction vector (used as Y axis) and only rotate around this to face the camera
        BBT_ORIENTED_COMMON,
        /// Billboards are oriented around their own direction vector (their own Y axis) and only rotate around this to face the camera
        BBT_ORIENTED_SELF,
        /// Billboards are perpendicular to a shared direction vector (used as Z axis, the facing direction) and X, Y axis are determined by a shared up-vertor
        BBT_PERPENDICULAR_COMMON,
        /// Billboards are perpendicular to their own direction vector (their own Z axis, the facing direction) and X, Y axis are determined by a shared up-vertor
        BBT_PERPENDICULAR_SELF
		*/

		std::vector<Ogre::String> types(5);
		types[0] = "Point";
		types[1] = "Oriented Common";
		types[2] = "Oriented Self";
		types[3] = "Perpendicular Common";
		types[4] = "Perpendicular Self";
		this->type = new Variant(BillboardComponent::AttrType(), types, this->attributes);
		
		std::vector<Ogre::String> datablockNames;

		// Go through all types of registered hlms unlit and check if the data block exists and set the data block
		Ogre::Hlms* hlms = Ogre::Root::getSingletonPtr()->getHlmsManager()->getHlms(Ogre::HLMS_UNLIT);
		if (nullptr != hlms)
		{
			for (auto& it = hlms->getDatablockMap().cbegin(); it != hlms->getDatablockMap().cend(); ++it)
			{
				datablockNames.emplace_back(it->second.name);
			}
		}
		std::sort(datablockNames.begin(), datablockNames.end());
		this->datablockName = new Variant(BillboardComponent::AttrDatablockName(), datablockNames, this->attributes);
		this->datablockName->setListSelectedValue("Star");

		// this->renderDistance->setDescription("The render distance until which the billboard will be rendered, 0 means infinity.");
	}

	BillboardComponent::~BillboardComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[BillboardComponent] Destructor billboard component for game object: " + this->gameObjectPtr->getName());
		if (nullptr != this->billboardSet)
		{
			this->billboardSet->removeBillboard(this->billboard);
			this->gameObjectPtr->getSceneNode()->detachObject(this->billboardSet);
			// In debug mode ogre is ill: there is an assert which wants to access a nullptr datablock just to check alpha :( and because of that, the application does crash
			this->gameObjectPtr->getSceneManager()->destroyBillboardSet(this->billboardSet);
			this->billboardSet = nullptr;
			this->billboard = nullptr;
		}
	}

	bool BillboardComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DatablockName")
		{
			this->datablockName->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Position")
		{
			this->position->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Origin")
		{
			this->origin->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Dimensions")
		{
			this->dimensions->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Color")
		{
			this->color->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RotationType")
		{
			this->rotationType->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Type")
		{
			this->type->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RenderDistance")
		{
			this->renderDistance->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr BillboardComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		BillboardCompPtr clonedCompPtr(boost::make_shared<BillboardComponent>());

		
		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setDatablockName(this->datablockName->getListSelectedValue());
		clonedCompPtr->setPosition(this->position->getVector3());
		clonedCompPtr->setOrigin(this->mapStringToOrigin(this->origin->getListSelectedValue()));
		clonedCompPtr->setDimensions(this->dimensions->getVector2());
		clonedCompPtr->setColor(this->color->getVector3());
		clonedCompPtr->setRotationType(this->mapStringToRotationType(this->rotationType->getListSelectedValue()));
		clonedCompPtr->setType(this->mapStringToType(this->type->getListSelectedValue()));
		clonedCompPtr->setRenderDistance(this->renderDistance->getReal());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool BillboardComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[BillboardComponent] Init billboard component for game object: " + this->gameObjectPtr->getName());

		this->createBillboard();
		return true;
	}

	void BillboardComponent::update(Ogre::Real dt, bool notSimulating)
	{
		
	}

	void BillboardComponent::createBillboard(void)
	{
		if (nullptr == this->billboard)
		{
			// https://forums.ogre3d.org/viewtopic.php?t=83421
			this->billboardSet = this->gameObjectPtr->getSceneManager()->createBillboardSet();
			
			this->billboardSet->setRenderQueueGroup(RENDER_QUEUE_PARTICLE_STUFF);
			this->billboardSet->setDatablock(Ogre::Root::getSingletonPtr()->getHlmsManager()->getDatablock(this->datablockName->getListSelectedValue()));
			// Must be set to false, else an ugly depthRange vertex shader error occurs, because an unlit material is used
			this->billboardSet->setCastShadows(false);
			this->billboardSet->setQueryFlags(0);

			auto data = DeployResourceModule::getInstance()->getPathAndResourceGroupFromDatablock(this->datablockName->getListSelectedValue(), Ogre::HlmsTypes::HLMS_UNLIT);

			DeployResourceModule::getInstance()->tagResource(this->datablockName->getString(), data.first, data.second);

			this->billboardSet->setBillboardOrigin(this->mapStringToOrigin(this->origin->getListSelectedValue()));
			this->billboardSet->setBillboardRotationType(this->mapStringToRotationType(this->rotationType->getListSelectedValue()));
			this->billboardSet->setBillboardType(this->mapStringToType(this->type->getListSelectedValue()));


			// this->billboard->setCastShadows(this->castShadows->getBool());
			// light->setDirection(this->gameObjectPtr->getSceneNode()->getOrientation() * Ogre::Vector3::NEGATIVE_UNIT_Z);
			this->gameObjectPtr->getSceneNode()->attachObject(this->billboardSet);
			
			this->billboard = billboardSet->createBillboard(this->position->getVector3());
			this->billboard->setDimensions(this->dimensions->getVector2().x, this->dimensions->getVector2().y);
			Ogre::Vector3 tempColor = this->color->getVector3();
			this->billboard->setColour(Ogre::ColourValue(tempColor.x, tempColor.y, tempColor.z, 1.0f));
			this->billboardSet->setRenderingDistance(this->renderDistance->getReal());

			this->billboardSet->setVisible(this->activated->getBool());
		}
	}

	void BillboardComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (BillboardComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (BillboardComponent::AttrDatablockName() == attribute->getName())
		{
			this->setDatablockName(attribute->getListSelectedValue());
		}
		else if (BillboardComponent::AttrPosition() == attribute->getName())
		{
			this->setPosition(attribute->getVector3());
		}
		else if (BillboardComponent::AttrOrigin() == attribute->getName())
		{
			this->setOrigin(this->mapStringToOrigin(attribute->getListSelectedValue()));
		}
		else if (BillboardComponent::AttrRotationType() == attribute->getName())
		{
			this->setRotationType(this->mapStringToRotationType(attribute->getListSelectedValue()));
		}
		else if (BillboardComponent::AttrType() == attribute->getName())
		{
			this->setType(this->mapStringToType(attribute->getListSelectedValue()));
		}
		else if (BillboardComponent::AttrDimensions() == attribute->getName())
		{
			this->setDimensions(attribute->getVector2());
		}
		else if (BillboardComponent::AttrColor() == attribute->getName())
		{
			this->setColor(attribute->getVector3());
		}
		else if (BillboardComponent::AttrRenderDistance() == attribute->getName())
		{
			this->setRenderDistance(attribute->getReal());
		}
		// else if (BillboardComponent::AttrCastShadows() == attribute->getName())
		// {
		// 	this->setCastShadows(attribute->getBool());
		// }
	}

	void BillboardComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DatablockName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->datablockName->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Position"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->position->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Origin"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->origin->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Dimensions"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->dimensions->getVector2())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Color"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->color->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RotationType"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rotationType->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Type"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->type->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RenderDistance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->renderDistance->getReal())));
		propertiesXML->append_node(propertyXML);
		
		/*propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CastShadows"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->castShadows->getBool())));
		propertiesXML->append_node(propertyXML);*/
	}

	void BillboardComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
		if (nullptr != this->billboardSet)
		{
			this->billboardSet->setVisible(activated);
		}
	}

	bool BillboardComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	Ogre::String BillboardComponent::getClassName(void) const
	{
		return "BillboardComponent";
	}

	Ogre::String BillboardComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	Ogre::String BillboardComponent::mapOriginToString(Ogre::v1::BillboardOrigin origin)
	{
		Ogre::String strOrigin = "Top Left";
		switch (origin)
		{
		case Ogre::v1::BBO_TOP_LEFT:
			strOrigin = "Top Left";
			break;
		case Ogre::v1::BBO_TOP_CENTER:
			strOrigin = "Top Center";
			break;
		case Ogre::v1::BBO_TOP_RIGHT:
			strOrigin = "Top Right";
			break;
		case Ogre::v1::BBO_CENTER_LEFT:
			strOrigin = "Center Left";
			break;
		case Ogre::v1::BBO_CENTER:
			strOrigin = "Center";
			break;
		case Ogre::v1::BBO_CENTER_RIGHT:
			strOrigin = "Center Right";
			break;
		case Ogre::v1::BBO_BOTTOM_LEFT:
			strOrigin = "Bottom Left";
			break;
		case Ogre::v1::BBO_BOTTOM_CENTER:
			strOrigin = "Bottom Center";
			break;
		case Ogre::v1::BBO_BOTTOM_RIGHT:
			strOrigin = "Bottom Right";
			break;
		}
		return strOrigin;
	}

	Ogre::v1::BillboardOrigin BillboardComponent::mapStringToOrigin(const Ogre::String& strOrigin)
	{
		Ogre::v1::BillboardOrigin origin = Ogre::v1::BBO_TOP_LEFT;
		if ("Top Left" == strOrigin)
			origin = Ogre::v1::BBO_TOP_LEFT;
		else if ("Top Center" == strOrigin)
			origin = Ogre::v1::BBO_TOP_CENTER;
		else if ("Top Right" == strOrigin)
			origin = Ogre::v1::BBO_TOP_RIGHT;
		else if ("Center Left" == strOrigin)
			origin = Ogre::v1::BBO_CENTER_LEFT;
		else if ("Center" == strOrigin)
			origin = Ogre::v1::BBO_CENTER;
		else if ("Center Right" == strOrigin)
			origin = Ogre::v1::BBO_CENTER_RIGHT;
		else if ("Bottom Left" == strOrigin)
			origin = Ogre::v1::BBO_BOTTOM_LEFT;
		else if ("Bottom Center" == strOrigin)
			origin = Ogre::v1::BBO_BOTTOM_CENTER;
		else if ("Bottom Right" == strOrigin)
			origin = Ogre::v1::BBO_BOTTOM_RIGHT;
		return origin;
	}

	Ogre::String BillboardComponent::mapRotationTypeToString(Ogre::v1::BillboardRotationType rotationType)
	{
		Ogre::String strRotationType = "Vertex";
		switch (rotationType)
		{
		case Ogre::v1::BBR_VERTEX:
			strRotationType = "Vertex";
			break;
		case Ogre::v1::BBR_TEXCOORD:
			strRotationType = "TexCoord";
			break;
		}
		return strRotationType;
	}

	Ogre::v1::BillboardRotationType BillboardComponent::mapStringToRotationType(const Ogre::String& strRotationType)
	{
		Ogre::v1::BillboardRotationType rotationType = Ogre::v1::BBR_VERTEX;
		if ("Vertex" == strRotationType)
			rotationType = Ogre::v1::BBR_VERTEX;
		else if ("TexCoord" == strRotationType)
			rotationType = Ogre::v1::BBR_TEXCOORD;
		return rotationType;
	}

	Ogre::String BillboardComponent::mapTypeToString(Ogre::v1::BillboardType type)
	{
		Ogre::String strType = "Point";
		switch (type)
		{
		case Ogre::v1::BBT_POINT:
			strType = "Point";
			break;
		case Ogre::v1::BBT_ORIENTED_COMMON:
			strType = "Oriented Common";
			break;
		case Ogre::v1::BBT_ORIENTED_SELF:
			strType = "Oriented Self";
			break;
		case Ogre::v1::BBT_PERPENDICULAR_COMMON:
			strType = "Perpendicular Common";
			break;
		case Ogre::v1::BBT_PERPENDICULAR_SELF:
			strType = "Perpendicular Self";
			break;
		}
		return strType;
	}

	Ogre::v1::BillboardType BillboardComponent::mapStringToType(const Ogre::String& strType)
	{
		Ogre::v1::BillboardType type = Ogre::v1::BBT_POINT;
		if ("Point" == strType)
			type = Ogre::v1::BBT_POINT;
		else if ("Oriented Common" == strType)
			type = Ogre::v1::BBT_ORIENTED_COMMON;
		else if ("Oriented Self" == strType)
			type = Ogre::v1::BBT_ORIENTED_SELF;
		else if ("Perpendicular Common" == strType)
			type = Ogre::v1::BBT_PERPENDICULAR_COMMON;
		else if ("Perpendicular Self" == strType)
			type = Ogre::v1::BBT_PERPENDICULAR_SELF;
		return type;
	}

	void BillboardComponent::setDatablockName(const Ogre::String& datablockName)
	{
		this->datablockName->setListSelectedValue(datablockName);
		if (nullptr != this->billboardSet)
		{
			this->billboardSet->setDatablock(Ogre::Root::getSingleton().getHlmsManager()->getDatablock(datablockName));
		}
	}

	Ogre::String BillboardComponent::getDatablockName(void) const
	{
		return this->datablockName->getListSelectedValue();
	}
	
	void BillboardComponent::setPosition(const Ogre::Vector3& position)
	{
		this->position->setValue(position);
		if (nullptr != this->billboard)
		{
			this->billboard->setPosition(this->position->getVector3());
		}
	}

	Ogre::Vector3 BillboardComponent::getPosition(void) const
	{
		return this->position->getVector3();
	}

	void BillboardComponent::setOrigin(Ogre::v1::BillboardOrigin origin)
	{
		this->origin->setListSelectedValue(this->mapOriginToString(origin));
		if (nullptr != this->billboardSet)
		{
			this->billboardSet->setBillboardOrigin(origin);
		}
	}

	Ogre::v1::BillboardOrigin BillboardComponent::getOrigin(void)
	{
		return this->mapStringToOrigin(this->origin->getListSelectedValue());
	}

	void BillboardComponent::setDimensions(const Ogre::Vector2& dimensions)
	{
		this->dimensions->setValue(dimensions);
		if (nullptr != this->billboard)
		{
			this->billboard->setDimensions(this->dimensions->getVector2().x, this->dimensions->getVector2().y);
		}
	}

	Ogre::Vector2 BillboardComponent::getDimensions(void) const
	{
		return this->dimensions->getVector2();
	}

	void BillboardComponent::setColor(const Ogre::Vector3& color)
	{
		this->color->setValue(color);
		if (nullptr != this->billboard)
		{
			this->billboard->setColour(Ogre::ColourValue(color.x, color.y, color.z, 1.0f));
		}
	}

	Ogre::Vector3 BillboardComponent::getColor(void) const
	{
		return this->color->getVector3();
	}

	void BillboardComponent::setRotationType(Ogre::v1::BillboardRotationType rotationType)
	{
		this->rotationType->setListSelectedValue(this->mapRotationTypeToString(rotationType));
		if (nullptr != this->billboardSet)
		{
			this->billboardSet->setBillboardRotationType(rotationType);
		}
	}

	Ogre::v1::BillboardRotationType BillboardComponent::getRotationType(void)
	{
		return this->mapStringToRotationType(this->rotationType->getListSelectedValue());
	}

	void BillboardComponent::setType(Ogre::v1::BillboardType type)
	{
		this->type->setListSelectedValue(this->mapTypeToString(type));
		if (nullptr != this->billboardSet)
		{
			this->billboardSet->setBillboardType(type);
		}
	}

	Ogre::v1::BillboardType BillboardComponent::getType(void)
	{
		return this->mapStringToType(this->type->getListSelectedValue());
	}

	void BillboardComponent::setRenderDistance(Ogre::Real renderDistance)
	{
		this->renderDistance->setValue(renderDistance);
		if (nullptr != this->billboardSet)
		{
			this->billboardSet->setRenderingDistance(renderDistance);
		}
	}

	Ogre::Real BillboardComponent::getRenderDistance(void) const
	{
		return this->renderDistance->getReal();
	}

	/*void BillboardComponent::setDirection(const Ogre::Vector3& direction)
	{
		this->direction->setValue(direction);
		if (nullptr != this->light)
		{
			this->light->setDirection(this->direction->getVector3());
		}
	}

	Ogre::Vector3 BillboardComponent::getDirection(void) const
	{
		return this->direction->getVector3();
	}

	void BillboardComponent::setCastShadows(bool castShadows)
	{
		this->castShadows->setValue(castShadows);
		if (nullptr != this->light)
		{
			this->light->setCastShadows(this->castShadows->getBool());
		}
	}

	bool BillboardComponent::getCastShadows(void) const
	{
		return this->castShadows->getBool();
	}*/

	Ogre::v1::BillboardSet* BillboardComponent::getBillboard(void) const
	{
		return this->billboardSet;
	}

}; // namespace end