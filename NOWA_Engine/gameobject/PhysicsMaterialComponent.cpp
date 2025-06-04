#include "NOWAPrecompiled.h"
#include "PhysicsMaterialComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "LuaScriptComponent.h"
#include "PhysicsComponent.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	PhysicsMaterialComponent::PhysicsMaterialComponent()
		: GameObjectComponent(),
		ogreNewt(AppStateManager::getSingletonPtr()->getOgreNewtModule()->getOgreNewt()),
		materialPair(nullptr),
		conveyorContactCallback(nullptr),
		genericContactCallback(nullptr)
	{
		std::vector<Ogre::String> allCategories = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getAllCategoriesSoFar();
		this->category1 = new Variant(PhysicsMaterialComponent::AttrCategory1(), allCategories, this->attributes);
		this->category2 = new Variant(PhysicsMaterialComponent::AttrCategory2(), allCategories, this->attributes);

		this->friction = new Variant(PhysicsMaterialComponent::AttrFriction(), Ogre::Vector2(0.8f, 0.4f), this->attributes);
		this->softness = new Variant(PhysicsMaterialComponent::AttrSoftness(), Ogre::Real(0.01f), this->attributes);
		this->elasticity = new Variant(PhysicsMaterialComponent::AttrElasticity(), Ogre::Real(0.01f), this->attributes);
		this->surfaceThickness = new Variant(PhysicsMaterialComponent::AttrSurfaceThickness(), Ogre::Real(0.125f), this->attributes);
		this->collideable = new Variant(PhysicsMaterialComponent::AttrCollideable(), true, this->attributes);
		this->contactBehavior = new Variant(PhysicsMaterialComponent::AttrContactBehavior(), { "None", "ConveyorPlayer", "ConveyorObject" }, this->attributes);
		this->contactSpeed = new Variant(PhysicsMaterialComponent::AttrContactSpeed(), Ogre::Real(10.0f), this->attributes);
		this->contactDirection = new Variant(PhysicsMaterialComponent::AttrContactDirection(), Ogre::Vector3::NEGATIVE_UNIT_Z, this->attributes);
		this->overlapFunctionName = new Variant(PhysicsMaterialComponent::AttrOverlapFunctionName(), Ogre::String(""), this->attributes);
		this->contactFunctionName = new Variant(PhysicsMaterialComponent::AttrContactFunctionName(), Ogre::String(""), this->attributes);
		this->contactOnceFunctionName = new Variant(PhysicsMaterialComponent::AttrContactOnceFunctionName(), Ogre::String(""), this->attributes);
		this->contactScratchFunctionName = new Variant(PhysicsMaterialComponent::AttrContactScratchFunctionName(), Ogre::String(""), this->attributes);

		this->contactBehavior->setListSelectedValue("None");
		// this->contactSpeed->setVisible("None" != this->contactBehavior->getListSelectedValue());
		// this->contactDirection->setVisible("None" != this->contactBehavior->getListSelectedValue());

		/*
		Set the default coefficients of friction for the material interaction between two physics materials . staticFriction*
		and kineticFriction must be positive values. kineticFriction must be lower than staticFriction. It is recommended
		that staticFriction and kineticFriction be set to a value lower or equal to 1.0, however because some synthetic
		materials can have higher than one coefficient of friction Newton allows for the coefficient of friction to be as
		high as 2.0.
		*/
		friction->setDescription("First is static friction, which must be lower than dynamic friction. Range: [0, 2]");
		// How todo with a vector2?
		// friction->setConstraints(0.0f, 2.0f);

		/*
		Set the default coefficients of restitution (elasticity) for the material interaction between two physics materials .
		elasticCoef* must be a positive value. It is recommended that elasticCoef be set to a value lower or equal to 1.0
		Return .
		*/
		elasticity->setDescription("Sets restitution. Range: [0, x] Recommended: 1 for good elasticity.");
		elasticity->setConstraints(0.0f, 2.0f);

		/*
		Set the default softness coefficients for the material interaction between two physics materials . softnessCoef*
		must be a positive value. It is recommended that softnessCoef be set to value lower or equal to 1.0 A low value
		for softnessCoef will make the material soft. A typical value for softnessCoef is 0.15
		*/
		softness->setDescription("Softness of material: Range: [0, x] Recommended: 1. Typical: 0.15");
		softness->setConstraints(0.0f, 0.25f);

		/*
		Set an imaginary thickness between the collision geometry of two colliding bodies whose physics properties are
		defined by this material pair when two bodies collide the engine resolve contact inter penetration by applying
		a small restoring velocity at each contact point. By default this restoring velocity will stop when the two
		contacts are at zero inter penetration distance. However by setting a non zero thickness the restoring velocity
		will continue separating the contacts until the distance between the two point of the collision geometry is equal
		to the surface thickness.
		*/
		surfaceThickness->setDescription("Surfaces thickness can improve the behaviors of rolling objects on flat surfaces. Range: [0, 0.125]");
		surfaceThickness->setConstraints(0.0f, 0.125f);

		this->overlapFunctionName->addUserData(GameObject::AttrActionNeedRefresh());
		this->contactFunctionName->addUserData(GameObject::AttrActionNeedRefresh());
		this->contactOnceFunctionName->addUserData(GameObject::AttrActionNeedRefresh());
		this->contactScratchFunctionName->addUserData(GameObject::AttrActionNeedRefresh());
		this->overlapFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction());
		this->contactFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction());
		this->contactOnceFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction());
		this->contactScratchFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction());

		this->overlapFunctionName->setDescription("Sets the lua function name, when two bodies do overlap their bounding box. E.g. onOverlapPlayerEnemy(gameObject1, gameObject2)");
		this->contactFunctionName->setDescription("Sets the lua function name, when two bodies have collision hull contact. E.g. onContactPlayerEnemy(gameObject1, gameObject2, contactData)");
		this->contactOnceFunctionName->setDescription("Sets the lua function name, when two bodies have collision hull contact once. E.g. onContactOncePlayerEnemy(gameObject1, gameObject2, contactData)");
		this->contactScratchFunctionName->setDescription("Sets the lua function name, when two bodies are scratching. E.g. onContactScratchCaseFloor(gameObject1, gameObject2, contactData)");
	}

	PhysicsMaterialComponent::~PhysicsMaterialComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsMaterialComponent] Destructor physics material component for game object: " + this->gameObjectPtr->getName());

		if (nullptr != this->conveyorContactCallback)
		{
			delete this->conveyorContactCallback;
			this->conveyorContactCallback = nullptr;
		}
		if (nullptr != this->genericContactCallback)
		{
			delete this->genericContactCallback;
			this->genericContactCallback = nullptr;
		}
		if (nullptr != this->materialPair)
		{
			delete this->materialPair;
			this->materialPair = nullptr;
		}
	}

	bool PhysicsMaterialComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Category1")
		{
			this->category1->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Category2")
		{
			this->category2->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Friction")
		{
			this->friction->setValue(XMLConverter::getAttribVector2(propertyElement, "data", Ogre::Vector2(0.8f, 0.4f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Softness")
		{
			this->softness->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0.01f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Elasticity")
		{
			this->elasticity->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0.01f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SurfaceThickness")
		{
			this->surfaceThickness->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0.5f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Collideable")
		{
			this->collideable->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ContactBehavior")
		{
			// Do not use the setter function here, because the material connection would be created to early
			this->contactBehavior->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
			// this->contactSpeed->setVisible("None" != this->contactBehavior->getListSelectedValue());
			// this->contactDirection->setVisible("None" != this->contactBehavior->getListSelectedValue());
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ContactSpeed")
		{
			this->contactSpeed->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ContactDirection")
		{
			this->contactDirection->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OverlapFunctionName")
		{
			this->setOverlapFunctionName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ContactFunctionName")
		{
			this->setContactFunctionName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ContactOnceFunctionName")
		{
			this->setContactOnceFunctionName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ContactScratchFunctionName")
		{
			this->setContactScratchFunctionName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		return true;
	}

	GameObjectCompPtr PhysicsMaterialComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		// Physics material component will not be cloned, because else the material pairs would have x-times the same category combinations, which does not make any sense.
		// hence delivering here 0, so when this game object will be cloned it will have all components also cloned but the physics material component!

		return nullptr;
	}

	bool PhysicsMaterialComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsMaterialComponent] Init physics material component for game object: " + this->gameObjectPtr->getName());

		std::vector<Ogre::String> allCategories = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getAllCategoriesSoFar();
		this->category1->setValue(allCategories);
		this->category2->setValue(allCategories);

		return true;
	}

	bool PhysicsMaterialComponent::connect(void)
	{
		this->createMaterialPair();
		return true;
	}

	void PhysicsMaterialComponent::createMaterialPair(void)
	{
		if (nullptr != this->genericContactCallback)
		{
			delete this->genericContactCallback;
			this->genericContactCallback = nullptr;
		}
		if (nullptr != this->conveyorContactCallback)
		{
			delete this->conveyorContactCallback;
			this->conveyorContactCallback = nullptr;
		}
		if (nullptr != this->materialPair)
		{
			delete this->materialPair;
			this->materialPair = nullptr;
		}
		
		// Get the affected category names and substract the category of the physics component, since this physics component create material, also use this category, 
		// so its possible to react when object of the same category collide
		Ogre::String categoryConnection = this->category2->getListSelectedValue();
		if (this->category2->getListSelectedValue()[0] != '+')
		{
			categoryConnection = '+' + this->category2->getListSelectedValue();
		}
		auto& targetCategoryNames = AppStateManager::getSingletonPtr()->getGameObjectController()->getAffectedCategories(categoryConnection);
		for (int j = 0; j < static_cast<int>(targetCategoryNames.size()); j++)
		{
			if (true == targetCategoryNames[j].empty())
			{
				continue;
			}
			
			// create the material pair, with the material group id for this object, and the one specified in ther materialpair category
			const OgreNewt::MaterialID* thisMaterial = AppStateManager::getSingletonPtr()->getGameObjectController()->getMaterialIDFromCategory(this->category1->getListSelectedValue(), this->ogreNewt);
			if (nullptr == thisMaterial)
			{
				// No physics component found, that uses this material, skip
				continue;
			}

			const OgreNewt::MaterialID* otherMaterial = AppStateManager::getSingletonPtr()->getGameObjectController()->getMaterialIDFromCategory(targetCategoryNames[j], this->ogreNewt);
			if (otherMaterial)
			{
				this->materialPair = new OgreNewt::MaterialPair(this->ogreNewt, thisMaterial, otherMaterial);
				// Set the data
				materialPair->setDefaultFriction(this->friction->getVector2().x, this->friction->getVector2().y);
				materialPair->setDefaultSoftness(this->softness->getReal());
				materialPair->setDefaultElasticity(this->elasticity->getReal());
				materialPair->setDefaultSurfaceThickness(this->surfaceThickness->getReal());
				materialPair->setDefaultCollidable(this->collideable->getBool());

				// for each phyisics body only one contact behaviour is possible at that time, else create conveyorcallback list and push_back
				if ("ConveyorPlayer" == this->contactBehavior->getListSelectedValue())
				{
					int conveyorCategoryId = AppStateManager::getSingletonPtr()->getGameObjectController()->getCategoryId(this->category1->getListSelectedValue());
					if (nullptr != this->conveyorContactCallback)
					{
						delete this->conveyorContactCallback;
						this->conveyorContactCallback = nullptr;
					}
					this->conveyorContactCallback = new ConveyorContactCallback(this->contactSpeed->getReal(), this->contactDirection->getVector3(), conveyorCategoryId, true);
					materialPair->setContactCallback(this->conveyorContactCallback);
				}
				else if ("ConveyorObject" == this->contactBehavior->getListSelectedValue())
				{
					int conveyorCategoryId = AppStateManager::getSingletonPtr()->getGameObjectController()->getCategoryId(this->category1->getListSelectedValue());
					this->conveyorContactCallback = new ConveyorContactCallback(this->contactSpeed->getReal(), this->contactDirection->getVector3(), conveyorCategoryId, false);
					materialPair->setContactCallback(this->conveyorContactCallback);
				}
				else if (nullptr != gameObjectPtr->getLuaScript())
				{
					if (false == this->overlapFunctionName->getString().empty() || false == this->contactFunctionName->getString().empty()
						|| false == this->contactOnceFunctionName->getString().empty())
					{
						int id = AppStateManager::getSingletonPtr()->getGameObjectController()->getCategoryId(this->category1->getListSelectedValue());
						this->genericContactCallback = new GenericContactCallback(gameObjectPtr->getLuaScript(), id, this->overlapFunctionName->getString(), 
							this->contactFunctionName->getString(), this->contactOnceFunctionName->getString(), this->contactScratchFunctionName->getString());

						materialPair->setContactCallback(this->genericContactCallback);
					}
				}
			}
			else
			{
				Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[PhysicsMaterialComponent] Could not set material pair between: " + this->category1->getListSelectedValue()
					+ ", because there is no material category with the name: " + this->category2->getListSelectedValue());
			}
		}
	}

	void PhysicsMaterialComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (PhysicsMaterialComponent::AttrCategory1() == attribute->getName())
		{
			this->category1->setListSelectedValue(attribute->getListSelectedValue());
		}
		else if (PhysicsMaterialComponent::AttrCategory2() == attribute->getName())
		{
			this->category2->setListSelectedValue(attribute->getListSelectedValue());
		}
		else if (PhysicsMaterialComponent::AttrFriction() == attribute->getName())
		{
			this->friction->setValue(attribute->getVector2());
		}
		else if (PhysicsMaterialComponent::AttrSoftness() == attribute->getName())
		{
			this->softness->setValue(attribute->getReal());
		}
		else if (PhysicsMaterialComponent::AttrElasticity() == attribute->getName())
		{
			this->elasticity->setValue(attribute->getReal());
		}
		else if (PhysicsMaterialComponent::AttrSurfaceThickness() == attribute->getName())
		{
			this->surfaceThickness->setValue(attribute->getReal());
		}
		else if (PhysicsMaterialComponent::AttrCollideable() == attribute->getName())
		{
			this->collideable->setValue(attribute->getBool());
		}
		else if (PhysicsMaterialComponent::AttrContactBehavior() == attribute->getName())
		{
			this->setContactBehavior(attribute->getListSelectedValue());
		}
		else if (PhysicsMaterialComponent::AttrContactSpeed() == attribute->getName())
		{
			this->contactSpeed->setValue(attribute->getReal());
		}
		else if (PhysicsMaterialComponent::AttrContactDirection() == attribute->getName())
		{
			this->contactDirection->setValue(attribute->getVector3());
		}
		else if (PhysicsMaterialComponent::AttrOverlapFunctionName() == attribute->getName())
		{
			this->setOverlapFunctionName(attribute->getString());
		}
		else if (PhysicsMaterialComponent::AttrContactFunctionName() == attribute->getName())
		{
			this->setContactFunctionName(attribute->getString());
		}
		else if (PhysicsMaterialComponent::AttrContactOnceFunctionName() == attribute->getName())
		{
			this->setContactOnceFunctionName(attribute->getString());
		}
		else if (PhysicsMaterialComponent::AttrContactScratchFunctionName() == attribute->getName())
		{
			this->setContactScratchFunctionName(attribute->getString());
		}
	}

	void PhysicsMaterialComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Category1"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->category1->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Category2"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->category2->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Friction"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->friction->getVector2())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Softness"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->softness->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Elasticity"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->elasticity->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SurfaceThickness"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->surfaceThickness->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Collideable"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->collideable->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ContactBehavior"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->contactBehavior->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ContactSpeed"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->contactSpeed->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ContactDirection"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->contactDirection->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OverlapFunctionName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->overlapFunctionName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ContactFunctionName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->contactFunctionName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ContactOnceFunctionName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->contactOnceFunctionName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ContactScratchFunctionName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->contactScratchFunctionName->getString())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String PhysicsMaterialComponent::getClassName(void) const
	{
		return "PhysicsMaterialComponent";
	}

	Ogre::String PhysicsMaterialComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void PhysicsMaterialComponent::setCategory1(const Ogre::String& category1)
	{
		this->category1->setListSelectedValue(category1);
		this->createMaterialPair();
	}

	Ogre::String PhysicsMaterialComponent::getCategory1(void) const
	{
		return this->category1->getListSelectedValue();
	}

	void PhysicsMaterialComponent::setCategory2(const Ogre::String& category2)
	{
		this->category2->setListSelectedValue(category2);
		this->createMaterialPair();
	}

	Ogre::String PhysicsMaterialComponent::getCategory2(void) const
	{
		return this->category2->getListSelectedValue();
	}

	void PhysicsMaterialComponent::setFriction(const Ogre::Vector2& friction)
	{
		this->friction->setValue(friction);
		if (nullptr != materialPair)
		{
			materialPair->setDefaultFriction(this->friction->getVector2().x, this->friction->getVector2().y);
		}
	}

	Ogre::Vector2 PhysicsMaterialComponent::getFriction(void) const
	{
		return this->friction->getVector2();
	}

	void PhysicsMaterialComponent::setSoftness(Ogre::Real softness)
	{
		this->softness->setValue(softness);
		if (nullptr != materialPair)
		{
			materialPair->setDefaultSoftness(this->softness->getReal());
		}
	}

	Ogre::Real PhysicsMaterialComponent::getSoftness(void) const
	{
		return this->softness->getReal();
	}

	void PhysicsMaterialComponent::setElasticity(Ogre::Real elasticity)
	{
		this->elasticity->setValue(elasticity);
		if (nullptr != materialPair)
		{
			materialPair->setDefaultElasticity(this->elasticity->getReal());
		}
	}

	Ogre::Real PhysicsMaterialComponent::getElasticity(void) const
	{
		return this->elasticity->getReal();
	}

	void PhysicsMaterialComponent::setSurfaceThickness(Ogre::Real surfaceThickness)
	{
		return this->surfaceThickness->setValue(surfaceThickness);
		if (nullptr != materialPair)
		{
			materialPair->setDefaultSurfaceThickness(this->surfaceThickness->getReal());
		}
	}

	Ogre::Real PhysicsMaterialComponent::getSurfaceThickness(void) const
	{
		return this->surfaceThickness->getReal();
	}

	void PhysicsMaterialComponent::setCollideable(bool collideable)
	{
		this->collideable->setValue(collideable);
		if (nullptr != materialPair)
		{
			materialPair->setDefaultCollidable(this->collideable->getBool());
		}
	}

	bool PhysicsMaterialComponent::getCollideable(void) const
	{
		return this->collideable->getBool();
	}

	void PhysicsMaterialComponent::setContactBehavior(const Ogre::String& contactBehavior)
	{
		this->contactBehavior->setListSelectedValue(contactBehavior);
		// Control visibility of the properties for the behavior, if some behavior is active
		// this->contactSpeed->setVisible("None" != this->contactBehavior->getListSelectedValue());
		// this->contactDirection->setVisible("None" != this->contactBehavior->getListSelectedValue());
		this->createMaterialPair();
	}

	Ogre::String PhysicsMaterialComponent::getContactBehavior(void) const
	{
		return this->contactBehavior->getString();
	}

	void PhysicsMaterialComponent::setContactSpeed(Ogre::Real contactSpeed)
	{
		this->contactSpeed->setValue(contactSpeed);
		if (nullptr != this->conveyorContactCallback)
		{
			this->conveyorContactCallback->setContactSpeed(contactSpeed);
		}
	}

	Ogre::Real PhysicsMaterialComponent::getContactSpeed(void) const
	{
		return this->contactSpeed->getReal();
	}

	void PhysicsMaterialComponent::setContactDirection(const Ogre::Vector3 contactDirection)
	{
		this->contactDirection->setValue(contactDirection);
		if (nullptr != this->conveyorContactCallback)
		{
			this->conveyorContactCallback->setContactDirection(contactDirection);
		}
	}

	Ogre::Vector3 PhysicsMaterialComponent::getContactDirection(void) const
	{
		return this->contactDirection->getVector3();
	}

	void PhysicsMaterialComponent::setOverlapFunctionName(const Ogre::String& overlapFunctionName)
	{
		this->overlapFunctionName->setValue(overlapFunctionName);
		this->overlapFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), overlapFunctionName + "(gameObject0, gameObject1)");
	}

	void PhysicsMaterialComponent::setContactFunctionName(const Ogre::String& contactFunctionName)
	{
		this->contactFunctionName->setValue(contactFunctionName);
		this->contactFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), contactFunctionName + "(gameObject0, gameObject1, contact)");
	}

	void PhysicsMaterialComponent::setContactOnceFunctionName(const Ogre::String& contactOnceFunctionName)
	{
		this->contactOnceFunctionName->setValue(contactOnceFunctionName);
		this->contactOnceFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), contactOnceFunctionName + "(gameObject0, gameObject1, contact)");
	}

	void PhysicsMaterialComponent::setContactScratchFunctionName(const Ogre::String& contactScratchFunctionName)
	{
		this->contactScratchFunctionName->setValue(contactScratchFunctionName);
		this->contactScratchFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), contactScratchFunctionName + "(gameObject0, gameObject1, contact)");
	}

	bool PhysicsMaterialComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// Constraints: Can only be placed under a main game object
		if (gameObject->getId() == GameObjectController::MAIN_GAMEOBJECT_ID)
		{
			return true;
		}
		return false;
	}

	/*void PhysicsMaterialComponent::setScriptFile(const Ogre::String& scriptFile)
	{
		this->scriptFile->setValue(scriptFile);

		if (false == scriptFile.empty())
		{
			if (nullptr != this->luaScript && this->luaScript->isCompiled() && scriptFile != this->luaScript->getName())
			{
				LuaScriptApi::getInstance()->copyScript(this->luaScript->getName(), scriptFile, true);
				LuaScriptApi::getInstance()->destroyScript(this->luaScript);
			}

			LuaScriptApi::getInstance()->destroyScript(this->luaScript);
			this->luaScript = LuaScriptApi::getInstance()->createScript(scriptFile + "_" + this->gameObjectPtr->getUniqueName(), scriptFile);
			
			this->luaScript->setInterfaceFunctionsTemplate("\nfunction onAABBOverlap(gameObject0, gameObject1)\n\nend\n\n"
				"function onContact(gameObject0, gameObject1, contact)\n\nend\n\nfunction onContactOnce(gameObject0, gameObject1, contact)\n"
				"\t--example code :\n\t--local targetGameObject = nil;\n"
				"\t-- if gameObject0:getCategory() == \"Player\" then\n\t\t-- targetGameObject = gameObject0;\n\t-- else\n\t\t--targetGameObject = gameObject1;\n\t--end\n\n"
				"\t-- if targetGameObject ~= nil then\n\t\t-- local soundComponent = targetGameObject:getSimpleSoundComponent();\n\t\t--soundComponent:setActivated(true);\n"
				"\t\t--soundComponent:connect();\n\t\t--local walkSound = soundComponent:getSound();\n\t\t--walkSound:setPitch(0.5);\n"
				"\t\t--walkSound:setGain(contact : getNormalSpeed() * 0.1);\n\t--end\nend\n");
			this->luaScript->setScriptFile(scriptFile);
		}

		this->createMaterialPair();
	}

	Ogre::String PhysicsMaterialComponent::getScriptFile(void) const
	{
		return this->scriptFile->getString();
	}*/

	/////////////////////////////////////////////////////////////////////////////////

	ConveyorContactCallback::ConveyorContactCallback(Ogre::Real speed, const Ogre::Vector3& direction, int conveyorCategoryId, bool forPlayer)
		: OgreNewt::ContactCallback(),
		speed(speed),
		direction(direction.normalisedCopy()),
		conveyorCategoryId(conveyorCategoryId),
		forPlayer(forPlayer)
	{

	}

	ConveyorContactCallback::~ConveyorContactCallback()
	{

	}

	int ConveyorContactCallback::onAABBOverlap(OgreNewt::Body* body0, OgreNewt::Body* body1, int threadIndex)
	{
		if (body0->getType() == this->conveyorCategoryId || body1->getType() == this->conveyorCategoryId)
		{
			return 1;
		}
		return 0;
	}

	void ConveyorContactCallback::contactsProcess(OgreNewt::ContactJoint& contactJoint, Ogre::Real timeStep, int threadIndex)
	{
		if (0.0f == this->speed)
		{
			return;
		}

		OgreNewt::Body* body0 = contactJoint.getBody0();
		OgreNewt::Body* body1 = contactJoint.getBody1();
		// First, find which body represents the conveyor belt!
		OgreNewt::Body* conveyor = nullptr;
		OgreNewt::Body* object = nullptr;

		if (body0->getType() == this->conveyorCategoryId)
		{
			conveyor = body0;
			object = body1;
		}
		else if (body1->getType() == this->conveyorCategoryId)
		{
			conveyor = body1;
			object = body0;
		}

		if (conveyor)
		{
			Ogre::Vector3 tempDirection = conveyor->getOgreNode()->_getDerivedOrientation() * this->direction;
			tempDirection.normalise();
			if (this->forPlayer)
			{
				// for player a different approach since the player is an manually controller object which cannot be move as easy as e.g. an box
				object->setVelocity(object->getVelocity() + (tempDirection * this->speed));
			}
			else
			{
				// okay, found the belt... let's adjust the collision based on this.
				for (OgreNewt::Contact contact = contactJoint.getFirstContact(); contact; contact = contact.getNext())
				{
					/*
					int state* - new state. 0 makes the contact frictionless along the index tangent vector.
					int index - index to the tangent vector. 0 for primary tangent vector or 1 for the secondary tangent vector.
					*/
					// contact.setFrictionCoef(1.0f, 1.0f, 0);
					// contact.setFrictionState(1, 0);
					contact.setRotateTangentDirections(tempDirection);
					Ogre::Vector3 contactPosition;
					Ogre::Vector3 contactNormal;
					contact.getPositionAndNormal(contactPosition, contactNormal, object);
					// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "pos: " + Ogre::StringConverter::toString(contactPosition) + "norm: " + Ogre::StringConverter::toString(contactNormal));
					Ogre::Vector3 objectPosition;
					Ogre::Quaternion objectOrientation;
					object->getPositionOrientation(objectPosition, objectOrientation);
					Ogre::Vector3 objectContactPointVelocity = (object->getVelocity() + (contactPosition - objectPosition) * object->getOmega()) * this->direction;
					Ogre::Real resultAcceleration = this->speed - tempDirection.dotProduct(objectContactPointVelocity);
					resultAcceleration *= 10.0f; // looks nicer
					contact.setTangentAcceleration(resultAcceleration, 0);
					// contact.setNormalAcceleration(resultAcceleration);

				}
			}
		}
	}

	void ConveyorContactCallback::setContactSpeed(Ogre::Real contactSpeed)
	{
		this->speed = contactSpeed;
	}

	void ConveyorContactCallback::setContactDirection(const Ogre::Vector3& contactDirection)
	{
		this->direction = contactDirection;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GenericContactCallback::GenericContactCallback(LuaScript* luaScript, int firstObjectId, const Ogre::String& overlapFunctionName,
		const Ogre::String& contactFunctionName, const Ogre::String& contactOnceFunctionName, const Ogre::String& contactScratchFunctionName)
		: OgreNewt::ContactCallback(),
		firstObjectId(firstObjectId),
		overlapFunctionName(overlapFunctionName),
		contactFunctionName(contactFunctionName),
		contactOnceFunctionName(contactOnceFunctionName),
		contactScratchFunctionName(contactScratchFunctionName),
		gameObject0(nullptr),
		gameObject1(nullptr),
		body0(nullptr),
		body1(nullptr),
		luaScript(luaScript),
		lastNormalSpeed(1.0f) // must be defined!, this causes sudden crashes, only in release mode!!
	{
	}

	GenericContactCallback::~GenericContactCallback()
	{

	}

	int GenericContactCallback::onAABBOverlap(OgreNewt::Body* body0, OgreNewt::Body* body1, int threadIndex)
	{
		if (nullptr == luaScript)
		{
			return 1;
		}

		// check who is who
		if (body0->getType() == this->firstObjectId)
		{
			this->body0 = body0;
			this->body1 = body1;
			// If a body cannot be cast to a physics component, set the game object to nullptr
			// This is the case e.g. when using a RagDoll, which bones are just bodies
			// But still the collision may be correct
			auto physicsComponent0 = OgreNewt::any_cast<PhysicsComponent*>(body0->getUserData());
			if (physicsComponent0)
				this->gameObject0 = physicsComponent0->getOwner().get();
			else
				this->gameObject0 = nullptr;

			auto physicsComponent1 = OgreNewt::any_cast<PhysicsComponent*>(body1->getUserData());
			if (physicsComponent1)
				this->gameObject1 = physicsComponent1->getOwner().get();
			else
				this->gameObject1 = nullptr;
		}
		else if (body1->getType() == this->firstObjectId)
		{
			this->body0 = body1;
			this->body1 = body0;

			auto physicsComponent1 = OgreNewt::any_cast<PhysicsComponent*>(body0->getUserData());
			if (physicsComponent1)
				this->gameObject1 = physicsComponent1->getOwner().get();
			else
				this->gameObject1 = nullptr;

			auto physicsComponent0 = OgreNewt::any_cast<PhysicsComponent*>(body1->getUserData());
			if (physicsComponent0)
				this->gameObject0 = physicsComponent0->getOwner().get();
			else
				this->gameObject0 = nullptr;
		}

		if (false == this->overlapFunctionName.empty())
		{
			NOWA::AppStateManager::LogicCommand logicCommand = [this]()
			{
				this->luaScript->callTableFunction(this->overlapFunctionName, this->gameObject0, this->gameObject1);
			};
			NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
		}
		return 1;
	}

	void GenericContactCallback::contactsProcess(OgreNewt::ContactJoint& contactJoint, Ogre::Real timeStep, int threadIndex)
	{
		// Note: If several threads are used, a lock is required, else lua will crash when a script is called
		AppStateManager::getSingletonPtr()->getOgreNewtModule()->getOgreNewt()->criticalSectionLock(threadIndex);
		
		if (nullptr == this->luaScript)
		{
			AppStateManager::getSingletonPtr()->getOgreNewtModule()->getOgreNewt()->criticalSectionUnlock();
			return;
		}

		Ogre::Real maxTangentSpeed = 0.0f;

		OgreNewt::Contact* normalContact = nullptr;
		OgreNewt::Contact* tangentContact = nullptr;
		OgreNewt::Contact* contact = nullptr;

		for (contact = &contactJoint.getFirstContact(); false == contact->isEmpty(); contact = &contact->getNext())
		{
			if (false == this->contactFunctionName.empty())
			{
				NOWA::AppStateManager::LogicCommand logicCommand = [this, contact]()
				{
					this->luaScript->callTableFunction(this->contactFunctionName, this->gameObject0, this->gameObject1, contact);
				};
				NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
			}

			if (false == this->contactOnceFunctionName.empty())
			{
				// Get the normal speed difference between two contacts
				Ogre::Real normalSpeedDiff = (this->lastNormalSpeed - contact->getNormalSpeed()) / this->lastNormalSpeed;

				// If the normal speed is high enough, an impact occured, like a body contacts with another one once
				// if it remains on another body, the normal speed is not high enough, so onContactOnce will not be called
				if (contact->getNormalSpeed() > 3.0f && abs(normalSpeedDiff) > 3.0f)
				{
					// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "hit: " + Ogre::StringConverter::toString(normalSpeedDiff));
					NOWA::AppStateManager::LogicCommand logicCommand = [this, contact]()
					{
							this->luaScript->callTableFunction(this->contactOnceFunctionName, this->gameObject0, this->gameObject1, contact);
					};
					NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
					break;
				}

				this->lastNormalSpeed = contact->getNormalSpeed();
				if (this->lastNormalSpeed == 0.0f)
					this->lastNormalSpeed = 1.0f;
			}

			if (false == this->contactScratchFunctionName.empty())
			{
				Ogre::Vector3 point0;
				Ogre::Vector3 normal0;
				contact->getPositionAndNormal(point0, normal0, this->body0);

				Ogre::Vector3 point1;
				Ogre::Vector3 normal1;
				contact->getPositionAndNormal(point1, normal1, this->body1);

				const Ogre::Vector3 pointVeloc0(this->body0->getVelocityAtPoint(point0));
				const Ogre::Vector3 pointVeloc1(this->body1->getVelocityAtPoint(point1));
				const Ogre::Vector3 veloc(pointVeloc1 - pointVeloc0);

				const Ogre::Real verticalSpeed = normal0.dotProduct(veloc);

				Ogre::Vector3 tangVeloc(veloc - (normal0 * verticalSpeed));
				const Ogre::Real tangentSpeed = tangVeloc.dotProduct(tangVeloc);
				if (tangentSpeed > maxTangentSpeed)
				{
					maxTangentSpeed = tangentSpeed;
					tangentContact = contact;
				}
			}
		}

		// Scratch
		if (false == this->contactScratchFunctionName.empty())
		{
			maxTangentSpeed = Ogre::Math::Sqrt(maxTangentSpeed);
			if (nullptr != tangentContact && maxTangentSpeed > 10.0f)
			{
				// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "scratch: " + Ogre::StringConverter::toString(maxTangentSpeed));
				NOWA::AppStateManager::LogicCommand logicCommand = [this, tangentContact]()
				{
					this->luaScript->callTableFunction(this->contactScratchFunctionName, this->gameObject0, this->gameObject1, tangentContact);
				};
				NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
			}
		}

		AppStateManager::getSingletonPtr()->getOgreNewtModule()->getOgreNewt()->criticalSectionUnlock();
	}

}; // namespace end