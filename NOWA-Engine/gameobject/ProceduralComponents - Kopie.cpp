#include "NOWAPrecompiled.h"
#include "ProceduralComponents.h"
#include "GameObjectController.h"
#include "PhysicsActiveComponent.h"

#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"

namespace NOWA
{
	using namespace rapidxml;

	ProceduralComponent::ProceduralComponent()
		: GameObjectComponent(),
		name(new Variant(ProceduralComponent::AttrName(), "Procedural1", this->attributes)),
		id(new Variant(ProceduralComponent::AttrId(), makeUniqueID(), this->attributes))
	{
		this->id->setReadOnly(true);
	}

	ProceduralComponent::~ProceduralComponent()
	{
		// Dangerous in destructor, as when exiting the simulation, the game object will be deleted and this function called, to seek for another procedural component, that has been deleted
		// Thus its done in onRemoveComponent
	}

	bool ProceduralComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		propertyElement = propertyElement->next_sibling("property");
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Name")
		{
			this->name->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Id")
		{
			// Really important: else the loaded value will not be written and the old one from constructor is present!
			this->id->setReadOnly(false);
			this->id->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			this->id->setReadOnly(true);
			propertyElement = propertyElement->next_sibling("property");
		}
		
		return true;
	}
	
	unsigned long ProceduralComponent::getId(void) const
	{
		return this->id->getULong();
	}

	bool ProceduralComponent::postInit(void)
	{
		// Add to the procedural component map, but as weak ptr, because game object controller should not hold the life cycle of this components, because the game objects already do, which are
		// also hold shared by the game object controller
		GameObjectController::getInstance()->addProceduralComponent(this->gameObjectPtr->getId(), boost::dynamic_pointer_cast<ProceduralComponent>(shared_from_this()));

		// Create mesh from procedural data, when a level is loaded
		
		return true;
	}

	bool ProceduralComponent::connect(void)
	{
		
		return true;
	}

	bool ProceduralComponent::disconnect(void)
	{
		// Do not destroy, it is destroyed and recreated in control of the user
		
		return true;
	}

	void ProceduralComponent::onRemoveComponent(void)
	{
		// Dangerous in destructor, as when exiting the simulation, the game object will be deleted and this function called, to seek for another procedural component, that has been deleted
		// Thus handle it, just when a component is removed
		bool stillProceduralComponentActive = false;
		for (size_t i = 0; i < this->gameObjectPtr->getComponents()->size(); i++)
		{
			ProceduralCompPtr proceduralCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<ProceduralComponent>());
			// Seek for this component and go to the previous one to process
			if (proceduralCompPtr != nullptr && proceduralCompPtr.get() != this)
			{
				stillProceduralComponentActive = true;
				break;
			}
		}

		// If there is no procedural component for this game object any more, remove the mesh
		if (false == stillProceduralComponentActive)
		{
			GameObjectController::getInstance()->removeProceduralComponent(this->gameObjectPtr->getId(), this->id->getULong());
		}
	}

	void ProceduralComponent::actualizeValue(Variant* attribute)
	{
		if (ProceduralComponent::AttrName() == attribute->getName())
		{
			this->setName(attribute->getString());
		}
	}

	void ProceduralComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString("Component" + this->getClassName())));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->getClassName())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Name"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->name->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Id"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->id->getULong())));
		propertiesXML->append_node(propertyXML);
	}

	void ProceduralComponent::setName(const Ogre::String& name)
	{
		this->name->setValue(name);
	}

	Ogre::String ProceduralComponent::getName(void) const
	{
		return this->name->getString();
	}

	Ogre::String ProceduralComponent::getClassName(void) const
	{
		return "ProceduralComponent";
	}

	Ogre::String ProceduralComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}
	
	Ogre::v1::MeshPtr ProceduralComponent::getMeshPtr(void) const
	{
		return this->meshPtr;
	}
	
	Procedural::TriangleBuffer& ProceduralComponent::getTriangleBuffer(void)
	{
		return this->triangleBuffer;
	}

	bool ProceduralComponent::make(bool forTriangleBuffer)
	{
		return false;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ProceduralPrimitiveComponent::ProceduralPrimitiveComponent()
		: ProceduralComponent(),
		makeButton(new Variant(ProceduralPrimitiveComponent::AttrMake(), false, this->attributes)),
		exportMeshButton(new Variant(ProceduralPrimitiveComponent::AttrExportMesh(), Ogre::String(""), this->attributes)),
		dataBlockName(new Variant(ProceduralPrimitiveComponent::AttrDataBlockName(), Ogre::String("GroundDirtPlane"), this->attributes)),
		shapeType(new Variant(ProceduralPrimitiveComponent::AttrShapeType(), { "Box", "Capsule", "Cone", "Cylinder", "IcoSphere", "Plane", "Prism", "RoundedBox", "Sphere", "Spring", "Torus", "Torus Knot", "Tube" }, this->attributes)),
		uvTile(new Variant(ProceduralPrimitiveComponent::AttrUVTile(), Ogre::Vector2(1.0f, 1.0f), this->attributes)),
		enableNormals(new Variant(ProceduralPrimitiveComponent::AttrEnableNormals(), true, this->attributes)),
		numTexCoords(new Variant(ProceduralPrimitiveComponent::AttrNumTexCoords(), 1, this->attributes)),
		uvOrigin(new Variant(ProceduralPrimitiveComponent::AttrUVOrigin(), Ogre::Vector2(0.0f, 0.0f), this->attributes)),
		switchUV(new Variant(ProceduralPrimitiveComponent::AttrSwitchUV(), false, this->attributes)),
		position(new Variant(ProceduralPrimitiveComponent::AttrPosition(), Ogre::Vector3::ZERO, this->attributes)),
		orientation(new Variant(ProceduralPrimitiveComponent::AttrOrientation(), Ogre::Vector3::ZERO, this->attributes)),
		scale(new Variant(ProceduralPrimitiveComponent::AttrScale(), Ogre::Vector3(1.0f, 1.0f, 1.0f), this->attributes)),
		
		sizeXYZ(new Variant(ProceduralPrimitiveComponent::AttrSizeXYZ(), Ogre::Vector3(1.0f, 1.0f, 1.0f), this->attributes)),
		numSegXYZ(new Variant(ProceduralPrimitiveComponent::AttrNumSegXYZ(), Ogre::Vector3(1.0f, 1.0f, 1.0f), this->attributes)), // Box, RoundedBox
		numRings(new Variant(ProceduralPrimitiveComponent::AttrUVOrigin(), static_cast<unsigned int>(8), this->attributes)), // Capsule, Sphere
		numSegments(new Variant(ProceduralPrimitiveComponent::AttrNumSegments(), static_cast<unsigned int>(16), this->attributes)), // Capsule, Cone, Cylinder, Sphere, Tube
		numSegHeight(new Variant(ProceduralPrimitiveComponent::AttrNumSegHeight(), static_cast<unsigned int>(1), this->attributes)), // Capsule, Cone, Cylinder, Prism, Tube
		radius(new Variant(ProceduralPrimitiveComponent::AttrRadius(), 1.0f, this->attributes)), // Capsule, Cone, Cylinder, IcoSphere, Prism, Sphere, Spring, Torus, TorusKnot
		height(new Variant(ProceduralPrimitiveComponent::AttrHeight(), 1.0f, this->attributes)), // Capsule, Cone, Cylinder, Prism, Spring, Tube
		capped(new Variant(ProceduralPrimitiveComponent::AttrCapped(), true, this->attributes)), // Cylinder, Prism
		numIterations(new Variant(ProceduralPrimitiveComponent::AttrNumIterations(), static_cast<unsigned int>(2), this->attributes)), // IcoSphere
		numSegXY(new Variant(ProceduralPrimitiveComponent::AttrNumSegXY(), Ogre::Vector2(1.0f, 1.0f), this->attributes)), // Plane
		normalXYZ(new Variant(ProceduralPrimitiveComponent::AttrNormalXYZ(), Ogre::Vector3(0.0f, 0.0f, 1.0f), this->attributes)), // Plane
		sizeXY(new Variant(ProceduralPrimitiveComponent::AttrSizeXY(), Ogre::Vector2(1.0f, 1.0f), this->attributes)), // Plane
		numSides(new Variant(ProceduralPrimitiveComponent::AttrNumSides(), static_cast<unsigned int>(3), this->attributes)), // Prism
		chamferSize(new Variant(ProceduralPrimitiveComponent::AttrChamferSize(), 0.1f, this->attributes)), // RoundedBox
		chamferNumSeg(new Variant(ProceduralPrimitiveComponent::AttrChamferNumSeg(), static_cast<unsigned int>(8), this->attributes)), // RoundedBox
		numSegPath(new Variant(ProceduralPrimitiveComponent::AttrNumSegPath(), static_cast<unsigned int>(5), this->attributes)), // Spring
		numRounds(new Variant(ProceduralPrimitiveComponent::AttrNumRounds(), static_cast<unsigned int>(8), this->attributes)), // Spring
		numSegSection(new Variant(ProceduralPrimitiveComponent::AttrNumSegSection(),  static_cast<unsigned int>(16), this->attributes)), // Torus, TorusKnot
		numSegCircle(new Variant(ProceduralPrimitiveComponent::AttrNumSegCircle(), static_cast<unsigned int>(16), this->attributes)), // Torus, TorusKnot
		sectionRadius(new Variant(ProceduralPrimitiveComponent::AttrSectionRadius(), 0.2f, this->attributes)), // Torus, TorusKnot
		torusKnotPQ(new Variant(ProceduralPrimitiveComponent::AttrUVOrigin(), Ogre::Vector2(2.0f, 3.0f), this->attributes)), // TorusKnot
		outerInnerRadius(new Variant(ProceduralPrimitiveComponent::AttrOuterInnerRadius(), Ogre::Vector2(2.0f, 1.0f), this->attributes)) // Tube
	{
		this->shapeType->setReadOnly(true);
		this->shapeType->setListSelectedValue("Box");
		this->shapeType->setUserData("needRefresh");

		this->makeButton->setUserData("button");
		this->exportMeshButton->setUserData("fileOpenDialog");

		this->sizeXYZ->setVisible(true); // Box, RoundedBox
		this->numSegXYZ->setVisible(true); // Box, RoundedBox
		this->numRings->setVisible(false); // Capsule, Sphere
		this->numSegments->setVisible(false); // Capsule, Cone, Cylinder, Sphere, Tube
		this->numSegHeight->setVisible(false); // Capsule, Cone, Cylinder, Prism, Tube
		this->radius->setVisible(false); // Capsule, Cone, Cylinder, IcoSphere, Prism, Sphere, Spring, Torus, TorusKnot
		this->height->setVisible(false); // Capsule, Cone, Cylinder, Prism, Spring, Tube
		this->capped->setVisible(false); // Cylinder, Prism
		this->numIterations->setVisible(false); // IcoSphere
		this->numSegXY->setVisible(false); // Plane
		this->normalXYZ->setVisible(false); // Plane
		this->sizeXY->setVisible(false); // Plane
		this->numSides->setVisible(false); // Prism
		this->chamferSize->setVisible(false); // RoundedBox
		this->chamferNumSeg->setVisible(false); // RoundedBox
		this->numSegPath->setVisible(false); // Spring
		this->numRounds->setVisible(false); // Spring
		this->numSegSection->setVisible(false); // Torus, TorusKnot
		this->numSegCircle->setVisible(false); // Torus, TorusKnot
		this->sectionRadius->setVisible(false); // Torus, TorusKnot
		this->torusKnotPQ->setVisible(false); // TorusKnot
		this->outerInnerRadius->setVisible(false); // Tube
	}

	ProceduralPrimitiveComponent::~ProceduralPrimitiveComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPrimitiveComponent] Destructor ai move component for game object: " + this->gameObjectPtr->getName());
	}

	bool ProceduralPrimitiveComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = ProceduralComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DataBlockName")
		{
			this->dataBlockName->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ShapeType")
		{
			this->shapeType->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "Box"));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UVTile")
		{
			this->uvTile->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "EnableNormals")
		{
			this->enableNormals->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "NumTexCoords")
		{
			this->numTexCoords->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UVOrigin")
		{
			this->uvOrigin->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SwitchUV")
		{
			this->switchUV->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Position")
		{
			this->position->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Orientation")
		{
			this->orientation->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Scale")
		{
			this->scale->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SizeXYZ")
		{
			this->sizeXYZ->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "NumSegXYZ")
		{
			this->numSegXYZ->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "NumRings")
		{
			this->numRings->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "NumSegments")
		{
			this->numSegments->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "NumSegHeight")
		{
			this->numSegHeight->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Radius")
		{
			this->radius->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Height")
		{
			this->height->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Capped")
		{
			this->capped->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "NumIterations")
		{
			this->numIterations->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "NumSegXY")
		{
			this->numSegXY->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "NormalXYZ")
		{
			this->normalXYZ->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SizeXY")
		{
			this->sizeXY->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "NumSides")
		{
			this->numSides->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ChamferSize")
		{
			this->chamferSize->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ChamferNumSeg")
		{
			this->chamferNumSeg->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "NumSegPath")
		{
			this->numSegPath->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "NumRounds")
		{
			this->numRounds->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "NumSegSection")
		{
			this->numSegSection->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "NumSegCircle")
		{
			this->numSegCircle->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SectionRadius")
		{
			this->sectionRadius->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TorusKnotPQ")
		{
			this->torusKnotPQ->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OuterInnerRadius")
		{
			this->outerInnerRadius->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		return success;
	}

	bool ProceduralPrimitiveComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPrimitiveComponent] Init ai move component for game object: " + this->gameObjectPtr->getName());

		return ProceduralComponent::postInit();
	}
	
	GameObjectCompPtr ProceduralPrimitiveComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		ProceduralPrimitiveCompPtr clonedCompPtr(boost::make_shared<ProceduralPrimitiveComponent>());

		// clonedCompPtr->setActivated(this->activated->getBool());
		
		// clonedCompPtr->setName(...)
		clonedCompPtr->setDataBlockName(this->dataBlockName->getString());
		clonedCompPtr->setShapeType(this->shapeType->getListSelectedValue());
		clonedCompPtr->setUVTile(this->uvTile->getVector2());
		clonedCompPtr->setEnableNormals(this->enableNormals->getBool());
		clonedCompPtr->setNumTexCoords(this->numTexCoords->getUInt());
		clonedCompPtr->setUVOrigin(this->uvOrigin->getVector2());
		clonedCompPtr->setSwitchUV(this->switchUV->getBool());
		clonedCompPtr->setPosition(this->position->getVector3());
		clonedCompPtr->setOrientation(MathHelper::getInstance()->degreesToQuat(this->orientation->getVector3()));
		clonedCompPtr->setScale(this->scale->getVector3());

		clonedCompPtr->setShapeType(this->shapeType->getListSelectedValue());
		clonedCompPtr->setSizeXYZ(this->sizeXYZ->getVector3());
		clonedCompPtr->setNumSegXYZ(this->numSegXYZ->getVector3());
		clonedCompPtr->setNumRings(this->numRings->getUInt());
		clonedCompPtr->setNumSegments(this->numSegments->getUInt());
		clonedCompPtr->setNumSegHeight(this->numSegHeight->getUInt());
		clonedCompPtr->setRadius(this->radius->getReal());
		clonedCompPtr->setHeight(this->height->getReal());
		clonedCompPtr->setCapped(this->capped->getBool());
		clonedCompPtr->setNumIterations(this->numIterations->getUInt());
		clonedCompPtr->setNumSegXY(this->numSegXY->getVector2());
		clonedCompPtr->setNormalXYZ(this->normalXYZ->getVector3());
		clonedCompPtr->setSizeXY(this->sizeXY->getVector2());
		clonedCompPtr->setNumSides(this->numSides->getUInt());
		clonedCompPtr->setChamferSize(this->chamferSize->getReal());
		clonedCompPtr->setChamferNumSeg(this->chamferNumSeg->getUInt());
		clonedCompPtr->setNumSegPath(this->numSegPath->getUInt());
		clonedCompPtr->setNumRounds(this->numRounds->getUInt());
		clonedCompPtr->setNumSegSection(this->numSegSection->getUInt());
		clonedCompPtr->setNumSegCircle(this->numSegCircle->getUInt());
		clonedCompPtr->setSectionRadius(this->sectionRadius->getReal());
		clonedCompPtr->setTorusKnotPQ(this->torusKnotPQ->getVector2());
		clonedCompPtr->setOuterInnerRadius(this->outerInnerRadius->getVector2());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		return clonedCompPtr;
	}

	bool ProceduralPrimitiveComponent::connect(void)
	{
		bool success = true;

		return success;
	}

	bool ProceduralPrimitiveComponent::disconnect(void)
	{
		return ProceduralComponent::disconnect();
	}

	void ProceduralPrimitiveComponent::update(Ogre::Real dt, bool notSimulating)
	{
		// Moving behavior is updated in game object controller, because it has a map for moving behavior and the game object
		// because, a game object may have x ai components, which are working on one moving behavior and getting calculated priortized for the game object
	}

	void ProceduralPrimitiveComponent::actualizeValue(Variant* attribute)
	{
		ProceduralComponent::actualizeValue(attribute);
		
		if (ProceduralPrimitiveComponent::AttrMake() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (ProceduralPrimitiveComponent::AttrDataBlockName() == attribute->getName())
		{
			this->setDataBlockName(attribute->getString());
		}
		else if (ProceduralPrimitiveComponent::AttrExportMesh() == attribute->getName())
		{
			this->exportMeshButton->setValue(attribute->getString());
		}
		else if (ProceduralPrimitiveComponent::AttrShapeType() == attribute->getName())
		{
			this->setShapeType(attribute->getListSelectedValue());
		}
		else if (ProceduralPrimitiveComponent::AttrUVTile() == attribute->getName())
		{
			this->setUVTile(attribute->getVector2());
		}
		else if (ProceduralPrimitiveComponent::AttrEnableNormals() == attribute->getName())
		{
			this->setEnableNormals(attribute->getBool());
		}
		else if (ProceduralPrimitiveComponent::AttrNumTexCoords() == attribute->getName())
		{
			this->setNumTexCoords(attribute->getUInt());
		}
		else if (ProceduralPrimitiveComponent::AttrUVOrigin() == attribute->getName())
		{
			this->setUVOrigin(attribute->getVector2());
		}
		else if (ProceduralPrimitiveComponent::AttrSwitchUV() == attribute->getName())
		{
			this->setSwitchUV(attribute->getBool());
		}
		else if (ProceduralPrimitiveComponent::AttrPosition() == attribute->getName())
		{
			this->setPosition(attribute->getVector3());
		}
		else if (ProceduralPrimitiveComponent::AttrOrientation() == attribute->getName())
		{
			this->setOrientation(MathHelper::getInstance()->degreesToQuat(attribute->getVector3()));
		}
		else if (ProceduralPrimitiveComponent::AttrScale() == attribute->getName())
		{
			this->setScale(attribute->getVector3());
		}
		else if (ProceduralPrimitiveComponent::AttrSizeXYZ() == attribute->getName())
		{
			this->setSizeXYZ(attribute->getVector3());
		}
		else if (ProceduralPrimitiveComponent::AttrNumSegXYZ() == attribute->getName())
		{
			this->setNumSegXYZ(attribute->getVector3());
		}
		else if (ProceduralPrimitiveComponent::AttrNumRings() == attribute->getName())
		{
			this->setNumRings(attribute->getUInt());
		}
		else if (ProceduralPrimitiveComponent::AttrNumSegments() == attribute->getName())
		{
			this->setNumSegments(attribute->getUInt());
		}
		else if (ProceduralPrimitiveComponent::AttrNumSegHeight() == attribute->getName())
		{
			this->setNumSegHeight(attribute->getUInt());
		}
		else if (ProceduralPrimitiveComponent::AttrRadius() == attribute->getName())
		{
			this->setRadius(attribute->getReal());
		}
		else if (ProceduralPrimitiveComponent::AttrHeight() == attribute->getName())
		{
			this->setHeight(attribute->getReal());
		}
		else if (ProceduralPrimitiveComponent::AttrCapped() == attribute->getName())
		{
			this->setCapped(attribute->getBool());
		}
		else if (ProceduralPrimitiveComponent::AttrNumIterations() == attribute->getName())
		{
			this->setNumIterations(attribute->getUInt());
		}
		else if (ProceduralPrimitiveComponent::AttrNumSegXY() == attribute->getName())
		{
			this->setNumSegXY(attribute->getVector2());
		}
		else if (ProceduralPrimitiveComponent::AttrNormalXYZ() == attribute->getName())
		{
			this->setNormalXYZ(attribute->getVector3());
		}
		else if (ProceduralPrimitiveComponent::AttrNumSides() == attribute->getName())
		{
			this->setNumSides(attribute->getUInt());
		}
		else if (ProceduralPrimitiveComponent::AttrChamferSize() == attribute->getName())
		{
			this->setChamferSize(attribute->getReal());
		}
		else if (ProceduralPrimitiveComponent::AttrChamferNumSeg() == attribute->getName())
		{
			this->setChamferNumSeg(attribute->getUInt());
		}
		else if (ProceduralPrimitiveComponent::AttrNumSegPath() == attribute->getName())
		{
			this->setNumSegPath(attribute->getUInt());
		}
		else if (ProceduralPrimitiveComponent::AttrNumRounds() == attribute->getName())
		{
			this->setNumRounds(attribute->getUInt());
		}
		else if (ProceduralPrimitiveComponent::AttrNumSegSection() == attribute->getName())
		{
			this->setNumSegSection(attribute->getUInt());
		}
		else if (ProceduralPrimitiveComponent::AttrNumSegCircle() == attribute->getName())
		{
			this->setNumSegCircle(attribute->getUInt());
		}
		else if (ProceduralPrimitiveComponent::AttrSectionRadius() == attribute->getName())
		{
			this->setSectionRadius(attribute->getReal());
		}
		else if (ProceduralPrimitiveComponent::AttrTorusKnotPQ() == attribute->getName())
		{
			this->setTorusKnotPQ(attribute->getVector2());
		}
		else if (ProceduralPrimitiveComponent::AttrOuterInnerRadius() == attribute->getName())
		{
			this->setOuterInnerRadius(attribute->getVector2());
		}
	}

	void ProceduralPrimitiveComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool

		ProceduralComponent::writeXML(propertiesXML, doc);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DataBlockName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->dataBlockName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ShapeType"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->shapeType->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UVTile"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->uvTile->getVector2())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "EnableNormals"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->enableNormals->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "NumTexCoords"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->numTexCoords->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UVOrigin"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->uvOrigin->getVector2())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SwitchUV"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->switchUV->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Position"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->position->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Orientation"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->orientation->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Scale"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->scale->getVector3())));
		propertiesXML->append_node(propertyXML);

		Ogre::String shapeType = this->shapeType->getString();

		if ("Box" == shapeType || "RoundedBox" == shapeType)
		{
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
			propertyXML->append_attribute(doc.allocate_attribute("name", "SizeXYZ"));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->sizeXYZ->getVector3())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
			propertyXML->append_attribute(doc.allocate_attribute("name", "NumSegXYZ"));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->numSegXYZ->getVector3())));
			propertiesXML->append_node(propertyXML);
		}

		if ("Capsule" == shapeType || "Sphere" == shapeType)
		{

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
			propertyXML->append_attribute(doc.allocate_attribute("name", "NumRings"));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->numRings->getUInt())));
			propertiesXML->append_node(propertyXML);
		}

		if ("Capsule" == shapeType || "Cone" == shapeType || "Cylinder" == shapeType || "Sphere" == shapeType || "Tube" == shapeType)
		{
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
			propertyXML->append_attribute(doc.allocate_attribute("name", "NumSegments"));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->numSegments->getUInt())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
			propertyXML->append_attribute(doc.allocate_attribute("name", "NumSegHeight"));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->numSegHeight->getUInt())));
			propertiesXML->append_node(propertyXML);
		}

		if ("Capsule" == shapeType || "Cone" == shapeType || "Cylinder" == shapeType || "IcoSphere" == shapeType || "Prism" == shapeType ||
			"Sphere" == shapeType || "Spring" == shapeType || "Torus" == shapeType || "TorusKnot" == shapeType)
		{
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
			propertyXML->append_attribute(doc.allocate_attribute("name", "Radius"));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->radius->getReal())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
			propertyXML->append_attribute(doc.allocate_attribute("name", "Height"));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->height->getReal())));
			propertiesXML->append_node(propertyXML);
		}

		if ("Cylinder" == shapeType || "Prism" == shapeType)
		{
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
			propertyXML->append_attribute(doc.allocate_attribute("name", "Capped"));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->capped->getBool())));
			propertiesXML->append_node(propertyXML);
		}

		if ("IcoSphere" == shapeType)
		{
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
			propertyXML->append_attribute(doc.allocate_attribute("name", "NumIterations"));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->numIterations->getUInt())));
			propertiesXML->append_node(propertyXML);
		}

		if ("Plane" == shapeType)
		{
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
			propertyXML->append_attribute(doc.allocate_attribute("name", "NumSegXY"));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->numSegXY->getVector2())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
			propertyXML->append_attribute(doc.allocate_attribute("name", "NormalXYZ"));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->normalXYZ->getVector3())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
			propertyXML->append_attribute(doc.allocate_attribute("name", "SizeXY"));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->sizeXY->getVector3())));
			propertiesXML->append_node(propertyXML);
		}

		if ("Prism" == shapeType)
		{
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
			propertyXML->append_attribute(doc.allocate_attribute("name", "NumSides"));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->numSides->getUInt())));
			propertiesXML->append_node(propertyXML);
		}

		if ("RoundedBox" == shapeType)
		{
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
			propertyXML->append_attribute(doc.allocate_attribute("name", "ChamferSize"));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->chamferSize->getReal())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
			propertyXML->append_attribute(doc.allocate_attribute("name", "ChamferNumSeg"));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->chamferNumSeg->getUInt())));
			propertiesXML->append_node(propertyXML);
		}

		if ("Spring" == shapeType)
		{
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
			propertyXML->append_attribute(doc.allocate_attribute("name", "NumSegPath"));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->numSegPath->getUInt())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
			propertyXML->append_attribute(doc.allocate_attribute("name", "NumRounds"));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->numRounds->getUInt())));
			propertiesXML->append_node(propertyXML);
		}

		if ("Torus" == shapeType || "TorusKnot" == shapeType)
		{
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
			propertyXML->append_attribute(doc.allocate_attribute("name", "NumSegSection"));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->numSegSection->getUInt())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
			propertyXML->append_attribute(doc.allocate_attribute("name", "NumSegCircle"));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->numSegCircle->getUInt())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
			propertyXML->append_attribute(doc.allocate_attribute("name", "SectionRadius"));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->sectionRadius->getReal())));
			propertiesXML->append_node(propertyXML);
		}
		
		if ("TorusKnot" == shapeType)
		{
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
			propertyXML->append_attribute(doc.allocate_attribute("name", "TorusKnotPQ"));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->torusKnotPQ->getVector2())));
			propertiesXML->append_node(propertyXML);
		}

		if ("Tube" == shapeType)
		{
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
			propertyXML->append_attribute(doc.allocate_attribute("name", "OuterInnerRadius"));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->outerInnerRadius->getVector2())));
			propertiesXML->append_node(propertyXML);
		}
	}

	Ogre::String ProceduralPrimitiveComponent::getClassName(void) const
	{
		return "ProceduralPrimitiveComponent";
	}

	Ogre::String ProceduralPrimitiveComponent::getParentClassName(void) const
	{
		return "ProceduralComponent";
	}

	void ProceduralPrimitiveComponent::setActivated(bool activated)
	{
		this->make(false);
	}

	bool ProceduralPrimitiveComponent::make(bool forTriangleBuffer)
	{
		Ogre::RealRect rect;
		rect.left = this->uvOrigin->getVector2().x;
		rect.top = this->uvOrigin->getVector2().y;
		rect.right = this->uvTile->getVector2().x;
		rect.bottom = this->uvTile->getVector2().y;

		if (nullptr != this->meshPtr)
		{
			// first delete entity and mesh ptr
			Ogre::v1::MeshManager::getSingletonPtr()->destroyResourcePool(this->gameObjectPtr->getName());
			Ogre::v1::MeshManager::getSingletonPtr()->remove(this->meshPtr->getHandle());
			// Attention: MeshPtr must be actualized in GOC!!!
		}

		Ogre::String shapeType = this->shapeType->getListSelectedValue();
		// Create mesh from procedural data, but only if it has not been created before, or a value has changed
		// The user can also control when to create, by tweaking some parameters and calling the make function
		if ("Box" == shapeType)
		{

			Procedural::BoxGenerator boxGenerator = Procedural::BoxGenerator();
			boxGenerator.setPosition(this->position->getVector3())
				.setOrientation(MathHelper::getInstance()->degreesToQuat(this->orientation->getVector3()))
				.setScale(this->scale->getVector3())
				.setSize(this->sizeXYZ->getVector3())
				.setNumSegX(static_cast<unsigned int>(this->numSegXYZ->getVector3().x))
				.setNumSegY(static_cast<unsigned int>(this->numSegXYZ->getVector3().y))
				.setNumSegZ(static_cast<unsigned int>(this->numSegXYZ->getVector3().z))

				.setEnableNormals(this->enableNormals->getBool())
				.setNumTexCoordSet(this->numTexCoords->getUInt())
				.setSwitchUV(this->switchUV->getBool())
				.setTextureRectangle(rect);
				

			if (false == forTriangleBuffer)
			{
				this->meshPtr = boxGenerator.realizeMesh(this->name->getString());
			}
			else
			{
				boxGenerator.addToTriangleBuffer(this->triangleBuffer);
			}
		}
		else if ("RoundedBox" == shapeType)
		{
			Procedural::RoundedBoxGenerator roundedBoxGenerator = Procedural::RoundedBoxGenerator();

			roundedBoxGenerator.setPosition(this->position->getVector3())
				.setOrientation(MathHelper::getInstance()->degreesToQuat(this->orientation->getVector3()))
				.setScale(this->scale->getVector3())
				.setEnableNormals(this->enableNormals->getBool())
				.setNumTexCoordSet(this->numTexCoords->getUInt())
				.setSwitchUV(this->switchUV->getBool())
				.setTextureRectangle(rect)

				.setSize(this->sizeXYZ->getVector3())
				.setNumSegX(static_cast<unsigned int>(this->numSegXYZ->getVector3().x))
				.setNumSegY(static_cast<unsigned int>(this->numSegXYZ->getVector3().y))
				.setNumSegZ(static_cast<unsigned int>(this->numSegXYZ->getVector3().z))
				.setChamferSize(this->chamferSize->getReal());
				// .chamferNumSeg is only internal and set to 8, make it public?

			if (false == forTriangleBuffer)
			{
				this->meshPtr = roundedBoxGenerator.realizeMesh(this->name->getString());
			}
			else
			{
				roundedBoxGenerator.addToTriangleBuffer(this->triangleBuffer);
			}
		}
		else if ("Cone" == shapeType)
		{
			this->radius->setVisible(true); // Capsule, Cone, Cylinder, IcoSphere, Prism, Sphere, Spring, Torus, TorusKnot
			this->height->setVisible(true); // Capsule, Cone, Cylinder, Prism, Spring, Tube
			this->radius->setVisible(true); // Capsule, Cone, Cylinder, IcoSphere, Prism, Sphere, Spring, Torus, TorusKnot
			this->height->setVisible(true); // Capsule, Cone, Cylinder, Prism, Spring, Tube
		}
		else if ("Cylinder" == shapeType)
		{
			this->numSegments->setVisible(true); // Capsule, Cone, Cylinder, Sphere, Tube
			this->numSegHeight->setVisible(true); // Capsule, Cone, Cylinder, Prism, Tube
			this->radius->setVisible(true); // Capsule, Cone, Cylinder, IcoSphere, Prism, Sphere, Spring, Torus, TorusKnot
			this->height->setVisible(true); // Capsule, Cone, Cylinder, Prism, Spring, Tube
			this->capped->setVisible(true); // Cylinder, Prism
		}
		else if ("IcoSphere" == shapeType)
		{
			this->radius->setVisible(true); // Capsule, Cone, Cylinder, IcoSphere, Prism, Sphere, Spring, Torus, TorusKnot
			this->numIterations->setVisible(true); // IcoSphere
		}
		else if ("Plane" == shapeType)
		{
			this->numSegXY->setVisible(true); // Plane
			this->normalXYZ->setVisible(true); // Plane
			this->sizeXY->setVisible(true); // Plane
		}
		else if ("Prism" == shapeType)
		{
			this->numSegHeight->setVisible(true); // Capsule, Cone, Cylinder, Prism, Tube
			this->radius->setVisible(true); // Capsule, Cone, Cylinder, IcoSphere, Prism, Sphere, Spring, Torus, TorusKnot
			this->height->setVisible(true); // Capsule, Cone, Cylinder, Prism, Spring, Tube
			this->capped->setVisible(true); // Cylinder, Prism
			this->numSides->setVisible(true); // Prism
		}
		else if ("RoundedBox" == shapeType)
		{
			this->sizeXYZ->setVisible(true); // Box, RoundedBox
			this->numSegXYZ->setVisible(true); // RoundedBox
			this->chamferSize->setVisible(true); // RoundedBox
			this->chamferNumSeg->setVisible(true); // RoundedBox
		}
		else if ("Sphere" == shapeType)
		{
			this->numSegXYZ->setVisible(true); // Capsule, Sphere
			this->numRings->setVisible(true); // Capsule, Sphere
			this->numSegments->setVisible(true); // Capsule, Cone, Cylinder, Sphere, Tube
			this->radius->setVisible(true); // Capsule, Cone, Cylinder, IcoSphere, Prism, Sphere, Spring, Torus, TorusKnot
		}
		else if ("Spring" == shapeType)
		{
			this->radius->setVisible(true); // Capsule, Cone, Cylinder, IcoSphere, Prism, Sphere, Spring, Torus, TorusKnot
			this->height->setVisible(true); // Capsule, Cone, Cylinder, Prism, Spring, Tube
			this->numSegPath->setVisible(true); // Spring
			this->numRounds->setVisible(true); // Spring
		}
		else if ("Torus" == shapeType)
		{
			this->radius->setVisible(true); // Capsule, Cone, Cylinder, IcoSphere, Prism, Sphere, Spring, Torus, TorusKnot
			this->numSegSection->setVisible(true); // Torus, TorusKnot
			this->numSegCircle->setVisible(true); // Torus, TorusKnot
			this->sectionRadius->setVisible(true); // Torus, TorusKnot
		}
		else if ("Torus Knot" == shapeType)
		{
			this->radius->setVisible(true); // Capsule, Cone, Cylinder, IcoSphere, Prism, Sphere, Spring, Torus, TorusKnot
			this->numSegSection->setVisible(true); // Torus, TorusKnot
			this->numSegCircle->setVisible(true); // Torus, TorusKnot
			this->sectionRadius->setVisible(true); // Torus, TorusKnot
			this->torusKnotPQ->setVisible(true); // TorusKnot
		}
		else if ("Tube" == shapeType)
		{
			this->numSegments->setVisible(true); // Capsule, Cone, Cylinder, Sphere, Tube
			this->numSegHeight->setVisible(true); // Capsule, Cone, Cylinder, Prism, Tube
			this->height->setVisible(true); // Capsule, Cone, Cylinder, Prism, Spring, Tube
			this->outerInnerRadius->setVisible(true); // Tube
		}

		if (nullptr == meshPtr)
			return true;

		NOWA::MathHelper::getInstance()->ensureHasTangents(meshPtr);

		bool castShadows = true;
		bool visible = true;

		// Check whether to cast shadows
		Ogre::MovableObject* movableObject = this->gameObjectPtr->getMovableObject();
		if (nullptr != movableObject)
		{
			castShadows = this->gameObjectPtr->getMovableObject()->getCastShadows();
			visible = this->gameObjectPtr->getMovableObject()->getVisible();

			// Destroy the prior created entity
			this->gameObjectPtr->getSceneNode()->detachObject(gameObjectPtr->getMovableObject());
			this->gameObjectPtr->getSceneManager()->destroyMovableObject(gameObjectPtr->getMovableObject());
		}

		Ogre::v1::Entity* entity = this->gameObjectPtr->getSceneManager()->createEntity(this->name->getString());

		// Get the used data block name 0
		entity->setDatablockOrMaterialName(this->dataBlockName->getString());
		entity->setName(this->name->getString() + "_Entity");

		entity->setCastShadows(castShadows);
		this->gameObjectPtr->getSceneNode()->attachObject(entity);

		Ogre::Aabb worldAABB = entity->getWorldAabb();
		worldAABB.transformAffine(entity->_getParentNodeFullTransform());

		// Recreate everything
		this->gameObjectPtr->init(entity);
		entity->setVisible(visible);
		this->gameObjectPtr->getSceneNode()->setVisible(visible);
		// Register after the component has been created
		GameObjectController::getInstance()->registerGameObject(gameObjectPtr);

		return true;
	}

	void ProceduralPrimitiveComponent::setDataBlockName(const Ogre::String& dataBlockName)
	{
		this->dataBlockName->setValue(dataBlockName);
	}

	Ogre::String ProceduralPrimitiveComponent::getDataBlockName(void) const
	{
		return this->dataBlockName->getString();
	}
	
	void ProceduralPrimitiveComponent::setUVTile(const Ogre::Vector2& uvTile)
	{
		this->uvTile->setValue(uvTile);
	}
	
	Ogre::Vector2 ProceduralPrimitiveComponent::getUVTile(void) const
	{
		return this->uvTile->getVector2();
	}
	
	void ProceduralPrimitiveComponent::setEnableNormals(bool enable)
	{
		this->enableNormals->setValue(enable);
	}
	
	bool ProceduralPrimitiveComponent::getEnableNormals(void) const
	{
		return this->enableNormals->getBool();
	}
	
	void ProceduralPrimitiveComponent::setNumTexCoords(unsigned int numTexCoords)
	{
		this->numTexCoords->setValue(numTexCoords);
	}
	
	unsigned int ProceduralPrimitiveComponent::getNumTexCoords(void) const
	{
		return this->numTexCoords->getUInt();
	}
	
	void ProceduralPrimitiveComponent::setUVOrigin(const Ogre::Vector2& uvOrigin)
	{
		this->uvOrigin->setValue(uvOrigin);
	}
	
	Ogre::Vector2 ProceduralPrimitiveComponent::getUVOrigin(void) const
	{
		return this->uvOrigin->getVector2();
	}
	
	void ProceduralPrimitiveComponent::setSwitchUV(bool switchUV)
	{
		this->switchUV->setValue(switchUV);
	}
	
	bool ProceduralPrimitiveComponent::getSwitchUV(void) const
	{
		return this->switchUV->getBool();
	}
	
	void ProceduralPrimitiveComponent::setPosition(const Ogre::Vector3& position)
	{
		this->position->setValue(position);
	}
	
	Ogre::Vector3 ProceduralPrimitiveComponent::getPosition(void) const
	{
		return this->position->getVector3();
	}
	
	void ProceduralPrimitiveComponent::setOrientation(const Ogre::Quaternion& orientation)
	{
		this->orientation->setValue(MathHelper::getInstance()->quatToDegrees(orientation));
	}
	
	Ogre::Quaternion ProceduralPrimitiveComponent::getOrientation(void) const
	{
		return MathHelper::getInstance()->degreesToQuat(this->orientation->getVector3());
	}
	
	void ProceduralPrimitiveComponent::setScale(const Ogre::Vector3& scale)
	{
		this->scale->setValue(scale);
	}
	
	Ogre::Vector3 ProceduralPrimitiveComponent::getUVScale(void) const
	{
		return this->scale->getVector3();
	}
	
	void ProceduralPrimitiveComponent::setShapeType(const Ogre::String& shapeType)
	{
		this->shapeType->setListSelectedValue(shapeType);
		
		// "Box", "Capsule", "Cone", "Cylinder", "IcoSphere", "Plane", "Prism", "RoundedBox", "Sphere", "Spring", "Torus", "Torus Knot", "Tube"
		
		// Check which properties to show/hide
		this->sizeXYZ->setVisible(true); // Box, RoundedBox
		this->numSegXYZ->setVisible(true); // Capsule, Sphere
		this->numRings->setVisible(false); // Capsule, Sphere
		this->numSegments->setVisible(false); // Capsule, Cone, Cylinder, Sphere, Tube
		this->numSegHeight->setVisible(false); // Capsule, Cone, Cylinder, Prism, Tube
		this->radius->setVisible(false); // Capsule, Cone, Cylinder, IcoSphere, Prism, Sphere, Spring, Torus, TorusKnot
		this->height->setVisible(false); // Capsule, Cone, Cylinder, Prism, Spring, Tube
		this->capped->setVisible(false); // Cylinder, Prism
		this->numIterations->setVisible(false); // IcoSphere
		this->numSegXY->setVisible(false); // Plane
		this->normalXYZ->setVisible(false); // Plane
		this->sizeXY->setVisible(false); // Plane
		this->numSides->setVisible(false); // Prism
		this->chamferSize->setVisible(false); // RoundedBox
		this->chamferNumSeg->setVisible(false); // RoundedBox
		this->numSegPath->setVisible(false); // Spring
		this->numRounds->setVisible(false); // Spring
		this->numSegSection->setVisible(false); // Torus, TorusKnot
		this->numSegCircle->setVisible(false); // Torus, TorusKnot
		this->sectionRadius->setVisible(false); // Torus, TorusKnot
		this->torusKnotPQ->setVisible(false); // TorusKnot
		this->outerInnerRadius->setVisible(false); // Tube
		
		if ("Box" == shapeType)
		{
			this->sizeXYZ->setVisible(true); // Box, RoundedBox
			this->numSegXYZ->setVisible(true); // Box, RoundedBox
		}
		else if ("RoundedBox" == shapeType)
		{
			this->sizeXYZ->setVisible(true); // Box, RoundedBox
			this->numSegXYZ->setVisible(true); // RoundedBox
			this->chamferSize->setVisible(true); // RoundedBox
			this->chamferNumSeg->setVisible(true); // RoundedBox
		}
		else if ("Cone" == shapeType)
		{
			this->radius->setVisible(true); // Capsule, Cone, Cylinder, IcoSphere, Prism, Sphere, Spring, Torus, TorusKnot
			this->height->setVisible(true); // Capsule, Cone, Cylinder, Prism, Spring, Tube
			this->radius->setVisible(true); // Capsule, Cone, Cylinder, IcoSphere, Prism, Sphere, Spring, Torus, TorusKnot
			this->height->setVisible(true); // Capsule, Cone, Cylinder, Prism, Spring, Tube
		}
		else if ("Cylinder" == shapeType)
		{
			this->numSegments->setVisible(true); // Capsule, Cone, Cylinder, Sphere, Tube
			this->numSegHeight->setVisible(true); // Capsule, Cone, Cylinder, Prism, Tube
			this->radius->setVisible(true); // Capsule, Cone, Cylinder, IcoSphere, Prism, Sphere, Spring, Torus, TorusKnot
			this->height->setVisible(true); // Capsule, Cone, Cylinder, Prism, Spring, Tube
			this->capped->setVisible(true); // Cylinder, Prism
		}
		else if ("IcoSphere" == shapeType)
		{
			this->radius->setVisible(true); // Capsule, Cone, Cylinder, IcoSphere, Prism, Sphere, Spring, Torus, TorusKnot
			this->numIterations->setVisible(true); // IcoSphere
		}
		else if ("Plane" == shapeType)
		{
			this->numSegXY->setVisible(true); // Plane
			this->normalXYZ->setVisible(true); // Plane
			this->sizeXY->setVisible(true); // Plane
		}
		else if ("Prism" == shapeType)
		{
			this->numSegHeight->setVisible(true); // Capsule, Cone, Cylinder, Prism, Tube
			this->radius->setVisible(true); // Capsule, Cone, Cylinder, IcoSphere, Prism, Sphere, Spring, Torus, TorusKnot
			this->height->setVisible(true); // Capsule, Cone, Cylinder, Prism, Spring, Tube
			this->capped->setVisible(true); // Cylinder, Prism
			this->numSides->setVisible(true); // Prism
		}
		else if ("RoundedBox" == shapeType)
		{
			this->sizeXYZ->setVisible(true); // Box, RoundedBox
			this->numSegXYZ->setVisible(true); // RoundedBox
			this->chamferSize->setVisible(true); // RoundedBox
			this->chamferNumSeg->setVisible(true); // RoundedBox
		}
		else if ("Sphere" == shapeType)
		{
			this->numSegXYZ->setVisible(true); // Capsule, Sphere
			this->numRings->setVisible(true); // Capsule, Sphere
			this->numSegments->setVisible(true); // Capsule, Cone, Cylinder, Sphere, Tube
			this->radius->setVisible(true); // Capsule, Cone, Cylinder, IcoSphere, Prism, Sphere, Spring, Torus, TorusKnot
		}
		else if ("Spring" == shapeType)
		{
			this->radius->setVisible(true); // Capsule, Cone, Cylinder, IcoSphere, Prism, Sphere, Spring, Torus, TorusKnot
			this->height->setVisible(true); // Capsule, Cone, Cylinder, Prism, Spring, Tube
			this->numSegPath->setVisible(true); // Spring
			this->numRounds->setVisible(true); // Spring
		}
		else if ("Torus" == shapeType)
		{
			this->radius->setVisible(true); // Capsule, Cone, Cylinder, IcoSphere, Prism, Sphere, Spring, Torus, TorusKnot
			this->numSegSection->setVisible(true); // Torus, TorusKnot
			this->numSegCircle->setVisible(true); // Torus, TorusKnot
			this->sectionRadius->setVisible(true); // Torus, TorusKnot
		}
		else if ("Torus Knot" == shapeType)
		{
			this->radius->setVisible(true); // Capsule, Cone, Cylinder, IcoSphere, Prism, Sphere, Spring, Torus, TorusKnot
			this->numSegSection->setVisible(true); // Torus, TorusKnot
			this->numSegCircle->setVisible(true); // Torus, TorusKnot
			this->sectionRadius->setVisible(true); // Torus, TorusKnot
			this->torusKnotPQ->setVisible(true); // TorusKnot
		}
		else if ("Tube" == shapeType)
		{
			this->numSegments->setVisible(true); // Capsule, Cone, Cylinder, Sphere, Tube
			this->numSegHeight->setVisible(true); // Capsule, Cone, Cylinder, Prism, Tube
			this->height->setVisible(true); // Capsule, Cone, Cylinder, Prism, Spring, Tube
			this->outerInnerRadius->setVisible(true); // Tube
		}
	}
	
	Ogre::String ProceduralPrimitiveComponent::getShapeType(void) const
	{
		return this->shapeType->getListSelectedValue();
	}
	
	void ProceduralPrimitiveComponent::setSizeXYZ(const Ogre::Vector3& size)
	{
		this->sizeXYZ->setValue(size);
	}
	
	Ogre::Vector3 ProceduralPrimitiveComponent::getSizeXYZ(void) const
	{
		return this->sizeXYZ->getVector3();
	}
	
	void ProceduralPrimitiveComponent::setNumSegXYZ(const Ogre::Vector3& segments)
	{
		this->numSegXYZ->setValue(segments);
	}
	
	Ogre::Vector3 ProceduralPrimitiveComponent::getNumSegXYZ(void) const
	{
		return this->numSegXYZ->getVector3();
	}
	
	void ProceduralPrimitiveComponent::setNumRings(unsigned int numRings)
	{
		this->numRings->setValue(numRings);
	}
	
	unsigned int ProceduralPrimitiveComponent::getNumRings(void) const
	{
		return this->numRings->getUInt();
	}
	
	void ProceduralPrimitiveComponent::setNumSegments(unsigned int numSegments)
	{
		this->numSegments->setValue(numSegments);
	}
	
	unsigned int ProceduralPrimitiveComponent::getNumSegments(void) const
	{
		return this->numSegments->getUInt();
	}
	
	void ProceduralPrimitiveComponent::setNumSegHeight(unsigned int numSegHeight)
	{
		this->numSegHeight->setValue(numSegHeight);
	}
	
	unsigned int ProceduralPrimitiveComponent::getNumSegHeight(void) const
	{
		return this->numSegHeight->getUInt();
	}
	
	void ProceduralPrimitiveComponent::setRadius(Ogre::Real radius)
	{
		this->radius->setValue(radius);
	}
	
	Ogre::Real ProceduralPrimitiveComponent::getRadius(void) const
	{
		return this->radius->getReal();
	}
	
	void ProceduralPrimitiveComponent::setHeight(Ogre::Real height)
	{
		this->height->setValue(height);
	}
	
	Ogre::Real ProceduralPrimitiveComponent::getHeight(void) const
	{
		return this->height->getReal();
	}
	
	void ProceduralPrimitiveComponent::setCapped(bool capped)
	{
		this->capped->setValue(capped);
	}
	
	bool ProceduralPrimitiveComponent::getCapped(void) const
	{
		return this->capped->getBool();
	}
	
	void ProceduralPrimitiveComponent::setNumIterations(unsigned int numIterations)
	{
		this->numIterations->setValue(numIterations);
	}
	
	unsigned int ProceduralPrimitiveComponent::getNumIterations(void) const
	{
		return this->numIterations->getUInt();
	}
	
	void ProceduralPrimitiveComponent::setNumSegXY(const Ogre::Vector2& numSeg)
	{
		this->numSegXY->setValue(numSeg);
	}
	
	Ogre::Vector2 ProceduralPrimitiveComponent::getNumSegXY(void) const
	{
		return this->numSegXY->getVector2();
	}
	
	void ProceduralPrimitiveComponent::setNormalXYZ(const Ogre::Vector3& normal)
	{
		this->normalXYZ->setValue(normal);
	}
	
	Ogre::Vector3 ProceduralPrimitiveComponent::getNormalXYZ(void) const
	{
		return this->normalXYZ->getVector3();
	}
	
	void ProceduralPrimitiveComponent::setSizeXY(const Ogre::Vector2& size)
	{
		this->sizeXY->setValue(size);
	}
	
	Ogre::Vector2 ProceduralPrimitiveComponent::getSizeXY(void) const
	{
		return this->sizeXY->getVector2();
	}
	
	void ProceduralPrimitiveComponent::setNumSides(unsigned int numSides)
	{
		this->numSides->setValue(numSides);
	}
	
	unsigned int ProceduralPrimitiveComponent::getNumSides(void) const
	{
		return this->numSides->getUInt();
	}
	
	void ProceduralPrimitiveComponent::setChamferSize(Ogre::Real chamferSize)
	{
		this->chamferSize->setValue(chamferSize);
	}
	
	Ogre::Real ProceduralPrimitiveComponent::getChamferSize(void) const
	{
		return this->chamferSize->getReal();
	}
	
	void ProceduralPrimitiveComponent::setChamferNumSeg(unsigned int chamferNumSeg)
	{
		this->chamferNumSeg->setValue(chamferNumSeg);
	}
	
	unsigned int ProceduralPrimitiveComponent::getChamferNumSeg(void) const
	{
		return this->chamferNumSeg->getUInt();
	}
	
	void ProceduralPrimitiveComponent::setNumSegPath(unsigned int numSegPath)
	{
		this->numSegPath->setValue(numSegPath);
	}
	
	unsigned int ProceduralPrimitiveComponent::getNumSegPath(void) const
	{
		return this->numSegPath->getUInt();
	}
	
	void ProceduralPrimitiveComponent::setNumRounds(unsigned int numRounds)
	{
		this->numRounds->setValue(numRounds);
	}
	
	unsigned int ProceduralPrimitiveComponent::getNumRounds(void) const
	{
		return this->numRounds->getUInt();
	}
	
	void ProceduralPrimitiveComponent::setNumSegSection(unsigned int numSegSection)
	{
		this->numSegSection->setValue(numSegSection);
	}
	
	unsigned int ProceduralPrimitiveComponent::getNumSegSection(void) const
	{
		return this->numSegSection->getUInt();
	}
	
	void ProceduralPrimitiveComponent::setNumSegCircle(unsigned int numSegCircle)
	{
		this->numSegCircle->setValue(numSegCircle);
	}
	
	unsigned int ProceduralPrimitiveComponent::getNumSegCircle(void) const
	{
		return this->numSegCircle->getUInt();
	}
	
	void ProceduralPrimitiveComponent::setSectionRadius(Ogre::Real sectionRadius)
	{
		this->sectionRadius->setValue(sectionRadius);
	}
	
	Ogre::Real ProceduralPrimitiveComponent::getSectionRadius(void) const
	{
		return this->sectionRadius->getReal();
	}
	
	void ProceduralPrimitiveComponent::setTorusKnotPQ(const Ogre::Vector2& qp)
	{
		this->torusKnotPQ->setValue(qp);
	}
	
	Ogre::Vector2 ProceduralPrimitiveComponent::getTorusKnotPQ(void) const
	{
		return this->torusKnotPQ->getVector2();
	}
	
	void ProceduralPrimitiveComponent::setOuterInnerRadius(const Ogre::Vector2& outerInnerRadius)
	{
		this->outerInnerRadius->setValue(outerInnerRadius);
	}
	
	Ogre::Vector2 ProceduralPrimitiveComponent::getOuterInnerRadius(void) const
	{
		return this->outerInnerRadius->getVector2();
	}

	void ProceduralPrimitiveComponent::exportMesh(void)
	{
		Ogre::String fileName = this->exportMeshButton->getString();
		// Export as mesh file and json file for datablock
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ProceduralBooleanComponent::ProceduralBooleanComponent()
		: ProceduralComponent(),
		makeButton(new Variant(ProceduralBooleanComponent::AttrMake(), false, this->attributes)),
		exportMeshButton(new Variant(ProceduralPrimitiveComponent::AttrExportMesh(), Ogre::String(""), this->attributes)),
		dataBlockName(new Variant(ProceduralBooleanComponent::AttrDataBlockName(), Ogre::String("GroundDirtPlane"), this->attributes)),
		targetId1(new Variant(ProceduralBooleanComponent::AttrTargetId1(), 0, this->attributes)),
		targetId2(new Variant(ProceduralBooleanComponent::AttrTargetId2(), 0, this->attributes)),
		boolOperation(new Variant(ProceduralBooleanComponent::AttrBooleanOperation(), { "Union", "Intersection", "Difference" }, this->attributes))
	{
		this->makeButton->setUserData("button");
		this->exportMeshButton->setUserData("fileOpenDialog");
	}

	ProceduralBooleanComponent::~ProceduralBooleanComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralBooleanComponent] Destructor ai move component for game object: " + this->gameObjectPtr->getName());
	}

	bool ProceduralBooleanComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = ProceduralComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetId1")
		{
			this->targetId1->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetId2")
		{
			this->targetId2->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BoolOperation")
		{
			this->boolOperation->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		return success;
	}

	bool ProceduralBooleanComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralBooleanComponent] Init ai move component for game object: " + this->gameObjectPtr->getName());

		return ProceduralComponent::postInit();
	}

	GameObjectCompPtr ProceduralBooleanComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		ProceduralBooleanCompPtr clonedCompPtr(boost::make_shared<ProceduralBooleanComponent>());

		// clonedCompPtr->setActivated(this->activated->getBool());

		clonedCompPtr->setDataBlockName(this->dataBlockName->getString());
		clonedCompPtr->setTargetId1(this->targetId1->getULong());
		clonedCompPtr->setTargetId2(this->targetId2->getULong());
		clonedCompPtr->setBoolOperation(this->boolOperation->getListSelectedValue());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		return clonedCompPtr;
	}

	bool ProceduralBooleanComponent::connect(void)
	{
		bool success = true;

		return success;
	}

	bool ProceduralBooleanComponent::disconnect(void)
	{
		return ProceduralComponent::disconnect();
	}

	void ProceduralBooleanComponent::update(Ogre::Real dt, bool notSimulating)
	{
		// Moving behavior is updated in game object controller, because it has a map for moving behavior and the game object
		// because, a game object may have x ai components, which are working on one moving behavior and getting calculated priortized for the game object
	}

	void ProceduralBooleanComponent::actualizeValue(Variant* attribute)
	{
		ProceduralComponent::actualizeValue(attribute);

		if (ProceduralPrimitiveComponent::AttrMake() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (ProceduralPrimitiveComponent::AttrDataBlockName() == attribute->getName())
		{
			this->setDataBlockName(attribute->getString());
		}
		else if (ProceduralPrimitiveComponent::AttrExportMesh() == attribute->getName())
		{
			this->exportMeshButton->setValue(attribute->getString());
		}
		else if (ProceduralBooleanComponent::AttrTargetId1() == attribute->getName())
		{
			this->setTargetId1(attribute->getULong());
		}
		else if (ProceduralBooleanComponent::AttrTargetId2() == attribute->getName())
		{
			this->setTargetId2(attribute->getULong());
		}
		else if (ProceduralBooleanComponent::AttrBooleanOperation() == attribute->getName())
		{
			this->setBoolOperation(attribute->getListSelectedValue());
		}
	}

	void ProceduralBooleanComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool

		ProceduralComponent::writeXML(propertiesXML, doc);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DataBlockName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->dataBlockName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TargetId1"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->targetId1->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TargetId2"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->targetId2->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BoolOperation"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->boolOperation->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
	}

	bool ProceduralBooleanComponent::onCloned(void)
	{
		// Search for the prior id of the cloned game object and set the new id and set the new id, if not found set better 0, else the game objects may be corrupt!
		GameObjectPtr targetGameObjectPtr1 = GameObjectController::getInstance()->getClonedGameObjectFromPriorId(this->targetId1->getULong());
		if (nullptr != targetGameObjectPtr1)
			this->setTargetId1(targetGameObjectPtr1->getId());
		else
			this->setTargetId1(0);

		GameObjectPtr targetGameObjectPtr2 = GameObjectController::getInstance()->getClonedGameObjectFromPriorId(this->targetId2->getULong());
		if (nullptr != targetGameObjectPtr2)
			this->setTargetId2(targetGameObjectPtr2->getId());
		else
			this->setTargetId2(0);
		return true;
	}


	Ogre::String ProceduralBooleanComponent::getClassName(void) const
	{
		return "ProceduralBooleanComponent";
	}

	Ogre::String ProceduralBooleanComponent::getParentClassName(void) const
	{
		return "ProceduralComponent";
	}

	void ProceduralBooleanComponent::setActivated(bool activated)
	{
		this->make(false);
	}

	bool ProceduralBooleanComponent::make(bool forTriangleBuffer)
	{
		if (nullptr != this->meshPtr)
		{
			// first delete entity and mesh ptr
			Ogre::v1::MeshManager::getSingletonPtr()->destroyResourcePool(this->gameObjectPtr->getName());
			Ogre::v1::MeshManager::getSingletonPtr()->remove(this->meshPtr->getHandle());
			// Attention: MeshPtr must be actualized in GOC!!!
		}

		auto& targetProceduralCompPtr1 = NOWA::makeStrongPtr(GameObjectController::getInstance()->getProceduralComponentById(this->gameObjectPtr->getId(), this->targetId1->getULong()));
		if (nullptr != targetProceduralCompPtr1)
		{
			auto& targetProceduralCompPtr2 = NOWA::makeStrongPtr(GameObjectController::getInstance()->getProceduralComponentById(this->gameObjectPtr->getId(), this->targetId2->getULong()));
			if (nullptr != targetProceduralCompPtr2)
			{
				// Create internal triangle buffer (true)
				if (true == targetProceduralCompPtr1->make(true))
				{
					if (true == targetProceduralCompPtr2->make(true))
					{
						Procedural::Boolean boolean = Procedural::Boolean();

						Ogre::String boolOperation = this->boolOperation->getListSelectedValue();
						if ("Union" == boolOperation)
						{
							boolean.setBooleanOperation(Procedural::Boolean::BT_UNION)
								.setMesh1(&targetProceduralCompPtr1->getTriangleBuffer()).setMesh2(&targetProceduralCompPtr2->getTriangleBuffer());
						}
						else if ("Intersect" == boolOperation)
						{
							boolean.setBooleanOperation(Procedural::Boolean::BT_INTERSECTION)
								.setMesh1(&targetProceduralCompPtr1->getTriangleBuffer()).setMesh2(&targetProceduralCompPtr2->getTriangleBuffer());
						}
						else if ("Difference" == boolOperation)
						{
							boolean.setBooleanOperation(Procedural::Boolean::BT_DIFFERENCE)
								.setMesh1(&targetProceduralCompPtr1->getTriangleBuffer()).setMesh2(&targetProceduralCompPtr2->getTriangleBuffer());
						}

						if (false == forTriangleBuffer)
						{
							this->meshPtr = boolean.realizeMesh(this->name->getString());
						}
						else
						{
							// If add to triangle buffer, this component is just the glue between other components
							boolean.addToTriangleBuffer(this->triangleBuffer);
						}
					}
				}
			}
		}

		if (nullptr == meshPtr)
			return true;

		NOWA::MathHelper::getInstance()->ensureHasTangents(meshPtr);

		bool castShadows = true;
		bool visible = true;

		// Check whether to cast shadows
		Ogre::MovableObject* movableObject = this->gameObjectPtr->getMovableObject();
		if (nullptr != movableObject)
		{
			castShadows = this->gameObjectPtr->getMovableObject()->getCastShadows();
			visible = this->gameObjectPtr->getMovableObject()->getVisible();

			// Destroy the prior created entity
			this->gameObjectPtr->getSceneNode()->detachObject(gameObjectPtr->getMovableObject());
			this->gameObjectPtr->getSceneManager()->destroyMovableObject(gameObjectPtr->getMovableObject());
		}

		Ogre::v1::Entity* entity = this->gameObjectPtr->getSceneManager()->createEntity(this->name->getString());

		// Get the used data block name 0
		entity->setDatablockOrMaterialName(this->dataBlockName->getString());
		entity->setName(this->name->getString() + "_Entity");

		entity->setCastShadows(castShadows);
		this->gameObjectPtr->getSceneNode()->attachObject(entity);

		// Recreate everything
		this->gameObjectPtr->init(entity);
		entity->setVisible(visible);
		this->gameObjectPtr->getSceneNode()->setVisible(visible);
		// Register after the component has been created
		GameObjectController::getInstance()->registerGameObject(gameObjectPtr);

		return true;
	}

	void ProceduralBooleanComponent::setDataBlockName(const Ogre::String& dataBlockName)
	{
		this->dataBlockName->setValue(dataBlockName);
	}

	Ogre::String ProceduralBooleanComponent::getDataBlockName(void) const
	{
		return this->dataBlockName->getString();
	}

	void ProceduralBooleanComponent::setTargetId1(unsigned long targetId1)
	{
		this->targetId1->setValue(targetId1);
	}

	unsigned long ProceduralBooleanComponent::getTargetId1(void) const
	{
		return this->targetId1->getULong();
	}

	void ProceduralBooleanComponent::setTargetId2(unsigned long targetId2)
	{
		this->targetId2->setValue(targetId2);
	}

	unsigned long ProceduralBooleanComponent::getTargetId2(void) const
	{
		return this->targetId2->getULong();
	}

	void ProceduralBooleanComponent::setBoolOperation(const Ogre::String& boolOperation)
	{
		this->boolOperation->setListSelectedValue(boolOperation);
	}

	Ogre::String ProceduralBooleanComponent::getBoolOperation(void) const
	{
		return this->boolOperation->getListSelectedValue();
	}

	void ProceduralBooleanComponent::exportMesh(void)
	{

	}

}; // namespace end