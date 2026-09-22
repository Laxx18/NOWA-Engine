#include "NOWAPrecompiled.h"
#include "PhysicsMaterialComponent.h"
#include "LuaScriptComponent.h"
#include "PhysicsComponent.h"
#include "main/AppStateManager.h"
#include "modules/LuaScriptApi.h"
#include "utilities/XMLConverter.h"

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    PhysicsMaterialComponent::PhysicsMaterialComponent() :
        GameObjectComponent(),
        ogreNewt(AppStateManager::getSingletonPtr()->getOgreNewtModule()->getOgreNewt()),
        materialPair(nullptr),
        conveyorContactCallback(nullptr),
        genericContactCallback(nullptr),
        oneWayContactCallback(nullptr),
        wallSlideContactCallback(nullptr)
    {
        std::vector<Ogre::String> allCategories = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getAllCategoriesSoFar();
        this->category1 = new Variant(PhysicsMaterialComponent::AttrCategory1(), allCategories, this->attributes);
        this->category2 = new Variant(PhysicsMaterialComponent::AttrCategory2(), allCategories, this->attributes);

        this->friction = new Variant(PhysicsMaterialComponent::AttrFriction(), Ogre::Vector2(0.8f, 0.4f), this->attributes);
        this->softness = new Variant(PhysicsMaterialComponent::AttrSoftness(), Ogre::Real(0.01f), this->attributes);
        this->elasticity = new Variant(PhysicsMaterialComponent::AttrElasticity(), Ogre::Real(0.01f), this->attributes);
        this->surfaceThickness = new Variant(PhysicsMaterialComponent::AttrSurfaceThickness(), Ogre::Real(0.125f), this->attributes);
        this->collideable = new Variant(PhysicsMaterialComponent::AttrCollideable(), true, this->attributes);
        this->contactBehavior = new Variant(PhysicsMaterialComponent::AttrContactBehavior(), {"None", "ConveyorPlayer", "ConveyorObject", "OneWay", "WallSlide"}, this->attributes);
        this->contactSpeed = new Variant(PhysicsMaterialComponent::AttrContactSpeed(), Ogre::Real(10.0f), this->attributes);
        this->contactDirection = new Variant(PhysicsMaterialComponent::AttrContactDirection(), Ogre::Vector3::NEGATIVE_UNIT_Z, this->attributes);
        // One-way filtering: axis is in the CATEGORY1 body's own local space (rotated into world
        // space by its current orientation at contact time, same as ConveyorContactCallback already
        // does for its belt direction) - "Y" gives the classic jump-through-from-below platform,
        // "X" a one-way wall/corridor. AllowedDirection picks which way along that axis the moving
        // object may pass through; the opposite way always collides normally.
        this->oneWayAxis = new Variant(PhysicsMaterialComponent::AttrOneWayAxis(), std::vector<Ogre::String>{"Y", "X"}, this->attributes);
        this->oneWayAllowedDirection = new Variant(PhysicsMaterialComponent::AttrOneWayAllowedDirection(), std::vector<Ogre::String>{"Positive", "Negative"}, this->attributes);
        this->wallSlideAngle = new Variant(PhysicsMaterialComponent::AttrWallSlideAngle(), Ogre::Real(68.0f), this->attributes);
        this->overlapFunctionName = new Variant(PhysicsMaterialComponent::AttrOverlapFunctionName(), Ogre::String(""), this->attributes);
        this->contactFunctionName = new Variant(PhysicsMaterialComponent::AttrContactFunctionName(), Ogre::String(""), this->attributes);
        this->contactOnceFunctionName = new Variant(PhysicsMaterialComponent::AttrContactOnceFunctionName(), Ogre::String(""), this->attributes);
        this->contactScratchFunctionName = new Variant(PhysicsMaterialComponent::AttrContactScratchFunctionName(), Ogre::String(""), this->attributes);

        this->contactBehavior->setListSelectedValue("None");
        this->oneWayAxis->setListSelectedValue("Y");
        this->oneWayAllowedDirection->setListSelectedValue("Positive");
        this->oneWayAxis->setDescription("Only used when Contact Behavior is 'OneWay'. Local axis of category1's body (rotated by its "
                                         "current orientation) the one-way filter operates along. 'Y' is the classic platform you can jump through from below and land "
                                         "on from above; 'X' is a one-way wall/corridor.");
        this->oneWayAllowedDirection->setDescription("Only used when Contact Behavior is 'OneWay'. Which direction along One Way Axis the moving object may pass through without colliding - the opposite direction always collides normally.");
        this->wallSlideAngle->setDescription("Only used when Contact Behavior is 'WallSlide'. Minimum angle in degrees against the horizontal plane for a surface to count as a wall. Its contact normal is then flattened to horizontal, so pressing "
                                             "into a wall or a platform edge can no longer lift the character - it slides down instead of hanging in mid air. 90 is a perfectly vertical wall, 68 also covers typical platform edges, and walkable ramps "
                                             "stay below it and are left untouched.");
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
        if (nullptr != this->oneWayContactCallback)
        {
            delete this->oneWayContactCallback;
            this->oneWayContactCallback = nullptr;
        }
        if (nullptr != this->wallSlideContactCallback)
        {
            delete this->wallSlideContactCallback;
            this->wallSlideContactCallback = nullptr;
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
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OneWayAxis")
        {
            this->oneWayAxis->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OneWayAllowedDirection")
        {
            this->oneWayAllowedDirection->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WallSlideAngle")
        {
            this->wallSlideAngle->setValue(XMLConverter::getAttribReal(propertyElement, "data", 68.0f));
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
        if (nullptr != this->oneWayContactCallback)
        {
            delete this->oneWayContactCallback;
            this->oneWayContactCallback = nullptr;
        }
        if (nullptr != this->wallSlideContactCallback)
        {
            delete this->wallSlideContactCallback;
            this->wallSlideContactCallback = nullptr;
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
        auto targetCategoryNames = AppStateManager::getSingletonPtr()->getGameObjectController()->getAffectedCategories(categoryConnection);
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
                else if ("OneWay" == this->contactBehavior->getListSelectedValue())
                {
                    // The one-way body's own category id, same resolution ConveyorContactCallback
                    // already uses for conveyorCategoryId - it is category1's body whose
                    // orientation the configured axis gets rotated by, and whose "side" defines
                    // which direction is the allowed pass-through direction.
                    int oneWayCategoryId = AppStateManager::getSingletonPtr()->getGameObjectController()->getCategoryId(this->category1->getListSelectedValue());
                    Ogre::Vector3 axis = ("Y" == this->oneWayAxis->getListSelectedValue()) ? Ogre::Vector3::UNIT_Y : Ogre::Vector3::UNIT_X;
                    bool allowPositiveDirection = ("Positive" == this->oneWayAllowedDirection->getListSelectedValue());
                    if (nullptr != this->oneWayContactCallback)
                    {
                        delete this->oneWayContactCallback;
                        this->oneWayContactCallback = nullptr;
                    }
                    this->oneWayContactCallback = new OneWayContactCallback(axis, allowPositiveDirection, oneWayCategoryId);
                    materialPair->setContactCallback(this->oneWayContactCallback);
                }
                else if ("WallSlide" == this->contactBehavior->getListSelectedValue())
                {
                    // Explicitly selected: nothing but the wall friction removal.
                    this->wallSlideContactCallback = new WallSlideContactCallback(this->wallSlideAngle->getReal());
                    materialPair->setContactCallback(this->wallSlideContactCallback);
                }
                else if (nullptr != gameObjectPtr->getLuaScript() && (false == this->overlapFunctionName->getString().empty() || false == this->contactFunctionName->getString().empty() || false == this->contactOnceFunctionName->getString().empty()))
                {
                    int id = AppStateManager::getSingletonPtr()->getGameObjectController()->getCategoryId(this->category1->getListSelectedValue());
                    this->genericContactCallback = new GenericContactCallback(gameObjectPtr->getLuaScript(), id, this->overlapFunctionName->getString(), this->contactFunctionName->getString(), this->contactOnceFunctionName->getString(),
                        this->contactScratchFunctionName->getString());

                    materialPair->setContactCallback(this->genericContactCallback);
                }
                else if (this->wallSlideAngle->getReal() > 0.0f)
                {
                    // No other behavior claimed this pair, but the wall slide angle is set -
                    // so a callback is installed that does nothing except remove the tangent
                    // friction of wall-like contacts.
                    //
                    // Without this the friction removal would silently do nothing for the
                    // most common case of all: a plain "Player vs Platform" pair with contact
                    // behavior 'None'. That is exactly the pair that needs it, and leaving it
                    // without a callback is what made the character stick to ledges again
                    // after the dedicated 'WallSlide' behavior was folded into the others.
                    this->wallSlideContactCallback = new WallSlideContactCallback(this->wallSlideAngle->getReal());
                    materialPair->setContactCallback(this->wallSlideContactCallback);
                }
            }
            else
            {
                Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL,
                    "[PhysicsMaterialComponent] Could not set material pair between: " + this->category1->getListSelectedValue() + ", because there is no material category with the name: " + this->category2->getListSelectedValue());
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
        else if (PhysicsMaterialComponent::AttrOneWayAxis() == attribute->getName())
        {
            this->setOneWayAxis(attribute->getListSelectedValue());
        }
        else if (PhysicsMaterialComponent::AttrOneWayAllowedDirection() == attribute->getName())
        {
            this->setOneWayAllowedDirection(attribute->getListSelectedValue());
        }
        else if (PhysicsMaterialComponent::AttrWallSlideAngle() == attribute->getName())
        {
            this->setWallSlideAngle(attribute->getReal());
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
        propertyXML->append_attribute(doc.allocate_attribute("name", "OneWayAxis"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->oneWayAxis->getListSelectedValue())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "OneWayAllowedDirection"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->oneWayAllowedDirection->getListSelectedValue())));
        propertiesXML->append_node(propertyXML);

        // Written right after OneWayAllowedDirection, matching the order init() parses them
        // in - that parser walks the properties sequentially, so the two have to agree.
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "WallSlideAngle"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wallSlideAngle->getReal())));
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

    void PhysicsMaterialComponent::setOneWayAxis(const Ogre::String& oneWayAxis)
    {
        this->oneWayAxis->setListSelectedValue(oneWayAxis);
        if (nullptr != this->oneWayContactCallback)
        {
            Ogre::Vector3 axis = ("Y" == oneWayAxis) ? Ogre::Vector3::UNIT_Y : Ogre::Vector3::UNIT_X;
            this->oneWayContactCallback->setLocalAxis(axis);
        }
    }

    Ogre::String PhysicsMaterialComponent::getOneWayAxis(void) const
    {
        return this->oneWayAxis->getListSelectedValue();
    }

    void PhysicsMaterialComponent::setWallSlideAngle(Ogre::Real wallSlideAngle)
    {
        // 0 is the explicit "off" value and must pass through untouched. Everything above it
        // is clamped to a sane band: below 10 degrees even a floor would count as a wall and
        // lose its friction, and the character would slide off level ground.
        if (wallSlideAngle < 0.0f)
        {
            wallSlideAngle = 0.0f;
        }
        else if (wallSlideAngle > 0.0f && wallSlideAngle < 10.0f)
        {
            wallSlideAngle = 10.0f;
        }
        if (wallSlideAngle > 90.0f)
        {
            wallSlideAngle = 90.0f;
        }

        this->wallSlideAngle->setValue(wallSlideAngle);

        // Pushed into whichever callback is currently installed - the setting applies to
        // every contact behavior, not to one dedicated mode.
        if (nullptr != this->conveyorContactCallback)
        {
            this->conveyorContactCallback->setWallSlideAngle(wallSlideAngle);
        }
        if (nullptr != this->oneWayContactCallback)
        {
            this->oneWayContactCallback->setWallSlideAngle(wallSlideAngle);
        }
        if (nullptr != this->genericContactCallback)
        {
            this->genericContactCallback->setWallSlideAngle(wallSlideAngle);
        }
        if (nullptr != this->wallSlideContactCallback)
        {
            this->wallSlideContactCallback->setWallSlideAngle(wallSlideAngle);
        }
    }

    Ogre::Real PhysicsMaterialComponent::getWallSlideAngle(void) const
    {
        return this->wallSlideAngle->getReal();
    }

    void PhysicsMaterialComponent::setOneWayAllowedDirection(const Ogre::String& oneWayAllowedDirection)
    {
        this->oneWayAllowedDirection->setListSelectedValue(oneWayAllowedDirection);
        if (nullptr != this->oneWayContactCallback)
        {
            this->oneWayContactCallback->setAllowPositiveDirection("Positive" == oneWayAllowedDirection);
        }
    }

    Ogre::String PhysicsMaterialComponent::getOneWayAllowedDirection(void) const
    {
        return this->oneWayAllowedDirection->getListSelectedValue();
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

    ConveyorContactCallback::ConveyorContactCallback(Ogre::Real speed, const Ogre::Vector3& direction, int conveyorCategoryId, bool forPlayer) :
        OgreNewt::ContactCallback(),
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

    Ogre::Real wallSlideAngleToMaxUpComponent(Ogre::Real wallSlideAngle)
    {
        if (wallSlideAngle <= 0.0f)
        {
            // Off: returned above 1 so no normalised normal can ever pass the test.
            return 2.0f;
        }

        // The angle is measured against the HORIZONTAL plane, so a surface counts as a wall
        // when its normal deviates from up by at least that angle - i.e. when the up
        // component of the normal is at most cos(angle). A vertical wall has 0, a flat floor
        // has 1.
        return Ogre::Math::Cos(Ogre::Degree(wallSlideAngle));
    }

    void disableWallFriction(const OgreNewt::ContactJoint& contactJoint, Ogre::Real maxUpComponent)
    {
        if (maxUpComponent > 1.0f)
        {
            // Switched off, nothing to do - this is the common case, so it is tested first.
            return;
        }

        // TEMPORARY DIAGNOSTICS - remove once the ledge sticking is understood. Throttled to
        // twice a second so the physics step is not slowed down by the logging itself, which
        // would create exactly the stutter we are chasing elsewhere.
        static unsigned int diagCallCounter = 0;
        static unsigned int diagWallCounter = 0;
        static unsigned int diagFloorCounter = 0;
        diagCallCounter++;

        OgreNewt::ContactJoint& mutableJoint = const_cast<OgreNewt::ContactJoint&>(contactJoint);

        for (OgreNewt::Contact contact = mutableJoint.getFirstContact(); contact; contact = contact.getNext())
        {
            Ogre::Vector3 contactPosition;
            Ogre::Vector3 contactNormal;
            contact.getPositionAndNormal(contactPosition, contactNormal);

            if (contactNormal.squaredLength() < 0.0001f)
            {
                continue;
            }
            contactNormal.normalise();

            // How much the surface faces upwards. 1 is a flat floor, 0 a vertical wall.
            // Negative means the character is touching the underside of something, which is
            // left alone so bumping your head still stops you.
            const Ogre::Real upComponent = contactNormal.dotProduct(Ogre::Vector3::UNIT_Y);

            if (upComponent > maxUpComponent || upComponent < 0.0f)
            {
                diagFloorCounter++;
                continue;
            }

            diagWallCounter++;

            if (diagCallCounter % 60 == 0)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[WallFriction-DIAG] calls: " + Ogre::StringConverter::toString(diagCallCounter) + " wallContacts: " + Ogre::StringConverter::toString(diagWallCounter) +
                                                                                        " floorContacts: " + Ogre::StringConverter::toString(diagFloorCounter) + " normal: " + Ogre::StringConverter::toString(contactNormal) +
                                                                                        " upComponent: " + Ogre::StringConverter::toString(upComponent) + " maxUpComponent: " + Ogre::StringConverter::toString(maxUpComponent));
            }

            // ND4 gives every contact point two tangent friction directions spanning the
            // plane perpendicular to the normal. For a wall normal of (-1, 0, 0) that plane
            // is Y/Z, so one of them points straight UP and holds a character pressed
            // against the wall in mid air. Removing them leaves the normal reaction intact -
            // the wall still blocks - but gravity can pull him down along it.
            contact.setFriction0Enabled(false);
            contact.setFriction1Enabled(false);
        }
    }

#if 0

	void ConveyorContactCallback::contactsProcess(const OgreNewt::ContactJoint& contactJoint, Ogre::Real timeStep, int threadIndex)
	{
		// Applies to EVERY contact behavior: a steep contact loses its tangent friction so a
		// character cannot be held up by a wall. Does nothing while the angle is 0.
		disableWallFriction(contactJoint, this->maxUpComponent);

		if (0.0f == this->speed)
		{
			return;
		}

		OgreNewt::ContactJoint& mutableJoint = const_cast<OgreNewt::ContactJoint&>(contactJoint);
        const OgreNewt::Body* body0 = mutableJoint.getBody0();
        const OgreNewt::Body* body1 = mutableJoint.getBody1();
		// First, find which body represents the conveyor belt!
		const OgreNewt::Body* conveyor = nullptr;
		const OgreNewt::Body* object = nullptr;
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
				const_cast<OgreNewt::Body*>(object)->setVelocity(object->getVelocity() + (tempDirection * this->speed));
			}
			else
			{
				// okay, found the belt... let's adjust the collision based on this.
				for (OgreNewt::Contact contact = const_cast<OgreNewt::ContactJoint&>(contactJoint).getFirstContact(); contact; contact = contact.getNext())
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
					contact.getPositionAndNormal(contactPosition, contactNormal);
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
#else
    void ConveyorContactCallback::contactsProcess(const OgreNewt::ContactJoint& contactJoint, Ogre::Real timeStep, int threadIndex)
    {
        // Applies to EVERY contact behavior: a steep contact loses its tangent friction so a
        // character cannot be held up by a wall. Does nothing while the angle is 0.
        disableWallFriction(contactJoint, this->maxUpComponent);

        if (0.0f == this->speed)
        {
            return;
        }

        OgreNewt::ContactJoint& mutableJoint = const_cast<OgreNewt::ContactJoint&>(contactJoint);
        const OgreNewt::Body* body0 = mutableJoint.getBody0();
        const OgreNewt::Body* body1 = mutableJoint.getBody1();

        // Welcher Body ist das Förderband?
        const OgreNewt::Body* conveyor = nullptr;
        const OgreNewt::Body* object = nullptr;
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

        if (nullptr == conveyor || nullptr == object)
        {
            return;
        }

        // Belt-Richtung in Weltkoordinaten (dreht mit dem Förderband-Objekt)
        Ogre::Vector3 beltDir = conveyor->getOgreNode()->_getDerivedOrientation() * this->direction;
        beltDir.normalise();

        if (this->forPlayer)
        {
            // Spieler: nur den Belt-Anteil der aktuellen Velocity ersetzen,
            // nicht addieren — sonst unbegrenzte Beschleunigung pro Frame
            Ogre::Vector3 currentVel = object->getVelocity();
            Ogre::Real currentBeltComp = currentVel.dotProduct(beltDir);
            Ogre::Real delta = this->speed - currentBeltComp;

            // Sanfte Angleichung (0.3 = ca. 3 Frames zum Einpendeln)
            const_cast<OgreNewt::Body*>(object)->setVelocity(currentVel + beltDir * delta * 0.3f);
        }
        else
        {
            // Objekte: Kontaktpunkt-Beschleunigung über Newton-Solver
            Ogre::Vector3 objectPos;
            Ogre::Quaternion objectOri;
            object->getPositionOrientation(objectPos, objectOri);

            for (OgreNewt::Contact contact = mutableJoint.getFirstContact(); contact; contact = contact.getNext())
            {
                // Tangenten auf Belt-Richtung ausrichten
                contact.setRotateTangentDirections(beltDir);

                // Kontaktpunkt-Position holen
                Ogre::Vector3 contactPos;
                Ogre::Vector3 contactNormal;
                contact.getPositionAndNormal(contactPos, contactNormal);

                // Korrekte Kontaktpunkt-Geschwindigkeit:
                // v_kontakt = v_cm + ω x r
                // (früher: (v + r * omega) * direction — beides falsch)
                Ogre::Vector3 r = contactPos - objectPos;
                Ogre::Vector3 pointVel = object->getVelocity() + object->getOmega().crossProduct(r);

                // Differenz zwischen gewünschter Belt-Geschwindigkeit und
                // aktueller Objektgeschwindigkeit in Belt-Richtung
                Ogre::Real currentBeltSpeed = beltDir.dotProduct(pointVel);
                Ogre::Real resultAcceleration = (this->speed - currentBeltSpeed) * 10.0f;

                contact.setTangentAcceleration(resultAcceleration, 0);
            }
        }
    }

#endif

    void ConveyorContactCallback::setContactSpeed(Ogre::Real contactSpeed)
    {
        this->speed = contactSpeed;
    }

    void ConveyorContactCallback::setContactDirection(const Ogre::Vector3& contactDirection)
    {
        this->direction = contactDirection;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // OneWayContactCallback - one-directional collision along a configurable local axis.
    //
    // Filters a single contact PER FRAME based on the moving object's velocity at the contact
    // point, projected onto the platform's own local axis (rotated into world space by the
    // platform's current orientation - same pattern as ConveyorContactCallback's own belt
    // direction above). Moving in the allowed direction disables collision resolution for that
    // contact via Contact::setCollidable(false); moving the other way leaves it enabled, so the
    // object collides and can stand/land on it or be blocked by it.
    //
    // This never touches geometry or the collision shape - the filtering happens entirely
    // per-contact, per-frame, in contactsProcess(), exactly like the conveyor belt's own
    // tangent-acceleration adjustment happens per-contact rather than by changing the mesh.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////

    OneWayContactCallback::OneWayContactCallback(const Ogre::Vector3& localAxis, bool allowPositiveDirection, int oneWayCategoryId) :
        OgreNewt::ContactCallback(),
        localAxis(localAxis.normalisedCopy()),
        allowPositiveDirection(allowPositiveDirection),
        oneWayCategoryId(oneWayCategoryId)
    {
    }

    OneWayContactCallback::~OneWayContactCallback()
    {
    }

    int OneWayContactCallback::onAABBOverlap(OgreNewt::Body* body0, OgreNewt::Body* body1, int threadIndex)
    {
        // Same restriction ConveyorContactCallback uses above: only bother generating contacts
        // for pairs that actually involve the one-way body at all.
        if (body0->getType() == this->oneWayCategoryId || body1->getType() == this->oneWayCategoryId)
        {
            return 1;
        }
        return 0;
    }

    void OneWayContactCallback::contactsProcess(const OgreNewt::ContactJoint& contactJoint, Ogre::Real timeStep, int threadIndex)
    {
        // Applies to EVERY contact behavior: a steep contact loses its tangent friction so a
        // character cannot be held up by a wall. Does nothing while the angle is 0.
        disableWallFriction(contactJoint, this->maxUpComponent);

        OgreNewt::ContactJoint& mutableJoint = const_cast<OgreNewt::ContactJoint&>(contactJoint);
        const OgreNewt::Body* body0 = mutableJoint.getBody0();
        const OgreNewt::Body* body1 = mutableJoint.getBody1();

        // Which body is the one-way platform/wall, which is the moving object - identical
        // category-id resolution to ConveyorContactCallback's conveyor/object split above.
        const OgreNewt::Body* platform = nullptr;
        const OgreNewt::Body* object = nullptr;
        if (body0->getType() == this->oneWayCategoryId)
        {
            platform = body0;
            object = body1;
        }
        else if (body1->getType() == this->oneWayCategoryId)
        {
            platform = body1;
            object = body0;
        }

        if (nullptr == platform || nullptr == object)
        {
            return;
        }

        // The configured local axis rotated into world space by the PLATFORM's own current
        // orientation, so a rotated one-way platform still filters along its own "up" or
        // "right" rather than the world's - same pattern as the conveyor belt's own direction.
        Ogre::Vector3 worldAxis = platform->getOgreNode()->_getDerivedOrientation() * this->localAxis;
        worldAxis.normalise();

        Ogre::Vector3 objectPosition;
        Ogre::Quaternion objectOrientation;
        object->getPositionOrientation(objectPosition, objectOrientation);

        for (OgreNewt::Contact contact = mutableJoint.getFirstContact(); contact; contact = contact.getNext())
        {
            Ogre::Vector3 contactPosition;
            Ogre::Vector3 contactNormal;
            contact.getPositionAndNormal(contactPosition, contactNormal);

            // Velocity of the OBJECT at the actual contact point, not its center of mass - v =
            // v_cm + omega x r, same formula ConveyorContactCallback uses above, since a
            // spinning or off-center-contacting object's point velocity can differ from its
            // center velocity.
            Ogre::Vector3 r = contactPosition - objectPosition;
            Ogre::Vector3 pointVelocity = object->getVelocity() + object->getOmega().crossProduct(r);

            // Component of the object's contact-point velocity along the platform's configured
            // axis - positive means moving in the platform's local +axis direction (e.g. up, or
            // right).
            Ogre::Real axisSpeed = pointVelocity.dotProduct(worldAxis);

            // allowPositiveDirection true: passes through while moving in +axis (e.g. jumping up
            // through a platform's underside); moving in -axis (e.g. falling onto its top) still
            // collides, which is what makes the platform landable. Flipped when false.
            const bool movingInAllowedDirection = this->allowPositiveDirection ? (axisSpeed > 0.0f) : (axisSpeed < 0.0f);

            // Re-evaluated every frame for every contact, on purpose: a contact that is
            // currently passing through (collision disabled) must go back to colliding normally
            // the instant the object's velocity crosses back the other way (e.g. the player
            // stops rising and starts falling while still overlapping the platform) - there is
            // no state to reset between frames, just this per-contact decision.
            contact.setCollidable(false == movingInAllowedDirection);
        }
    }

    void OneWayContactCallback::setLocalAxis(const Ogre::Vector3& localAxis)
    {
        this->localAxis = localAxis.normalisedCopy();
    }

    void OneWayContactCallback::setAllowPositiveDirection(bool allowPositiveDirection)
    {
        this->allowPositiveDirection = allowPositiveDirection;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////

    GenericContactCallback::GenericContactCallback(LuaScript* luaScript, int firstObjectId, const Ogre::String& overlapFunctionName, const Ogre::String& contactFunctionName, const Ogre::String& contactOnceFunctionName,
        const Ogre::String& contactOnceScratchName) :
        OgreNewt::ContactCallback(),
        firstObjectId(firstObjectId),
        luaScript(luaScript),
        overlapFunctionName(overlapFunctionName),
        contactFunctionName(contactFunctionName),
        contactOnceFunctionName(contactOnceFunctionName),
        contactScratchFunctionName(contactOnceScratchName)
    {
    }

    GenericContactCallback::~GenericContactCallback()
    {
    }

    GameObject* GenericContactCallback::resolveGameObject(OgreNewt::Body* body)
    {
        if (nullptr == body)
        {
            return nullptr;
        }

        auto physicsComponent = OgreNewt::any_cast<PhysicsComponent*>(body->getUserData());
        if (nullptr == physicsComponent)
        {
            return nullptr;
        }
        return physicsComponent->getOwner().get();
    }

    void GenericContactCallback::orderBodies(OgreNewt::Body* bodyA, OgreNewt::Body* bodyB, OgreNewt::Body*& outBody0, OgreNewt::Body*& outBody1) const
    {
        outBody0 = bodyA;
        outBody1 = bodyB;

        // Swap the INPUTS rather than assigning crosswise. The original code read bodyA into
        // gameObject1 and bodyB into gameObject0, which was correct but near impossible to
        // verify at a glance.
        if (nullptr != bodyA && nullptr != bodyB && bodyB->getType() == this->firstObjectId && bodyA->getType() != this->firstObjectId)
        {
            outBody0 = bodyB;
            outBody1 = bodyA;
        }
    }

    int GenericContactCallback::onAABBOverlap(OgreNewt::Body* body0, OgreNewt::Body* body1, int threadIndex)
    {
        if (nullptr == this->luaScript || true == this->overlapFunctionName.empty())
        {
            return 1;
        }

        OgreNewt::Body* orderedBody0 = nullptr;
        OgreNewt::Body* orderedBody1 = nullptr;
        this->orderBodies(body0, body1, orderedBody0, orderedBody1);

        // Everything the logic command needs is captured BY VALUE, frozen at the moment the
        // overlap happened. Nothing is stored in the object, so parallel calls from several
        // ND4 worker threads cannot interfere with each other.
        GameObject* const capturedGameObject0 = GenericContactCallback::resolveGameObject(orderedBody0);
        GameObject* const capturedGameObject1 = GenericContactCallback::resolveGameObject(orderedBody1);
        const Ogre::String capturedFunctionName = this->overlapFunctionName;

        NOWA::AppStateManager::LogicCommand logicCommand = [this, capturedGameObject0, capturedGameObject1, capturedFunctionName]()
        {
            // Re-checked here, not only above: the command runs one or more frames later and
            // the script may have been torn down in between.
            if (nullptr == this->luaScript)
            {
                return;
            }

            this->luaScript->callTableFunction(capturedFunctionName, capturedGameObject0, capturedGameObject1);
        };
        NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));

        return 1;
    }

    void GenericContactCallback::contactsProcess(const OgreNewt::ContactJoint& contactJoint, Ogre::Real timeStep, int threadIndex)
    {
        // Applies to EVERY contact behavior: a steep contact loses its tangent friction so a
        // character cannot be held up by a wall. Does nothing while the angle is 0.
        disableWallFriction(contactJoint, this->maxUpComponent);

        if (nullptr == this->luaScript)
        {
            return;
        }

        // The bodies come straight from the contact joint we were handed, instead of from
        // members that onAABBOverlap() happened to write earlier. That is what removes the
        // shared state: with ND4 running one worker per core, those members were rewritten by
        // other threads while this function was still using them, so Lua could receive the
        // correct contact snapshot together with the game objects of a DIFFERENT collision.
        OgreNewt::ContactJoint& mutableContactJoint = const_cast<OgreNewt::ContactJoint&>(contactJoint);

        OgreNewt::Body* orderedBody0 = nullptr;
        OgreNewt::Body* orderedBody1 = nullptr;
        this->orderBodies(mutableContactJoint.getBody0(), mutableContactJoint.getBody1(), orderedBody0, orderedBody1);

        if (nullptr == orderedBody0 || nullptr == orderedBody1)
        {
            return;
        }

        GameObject* const capturedGameObject0 = GenericContactCallback::resolveGameObject(orderedBody0);
        GameObject* const capturedGameObject1 = GenericContactCallback::resolveGameObject(orderedBody1);

        Ogre::Real maxTangentSpeed = 0.0f;
        Ogre::Real maxNormalSpeed = 0.0f;

        OgreNewt::ContactSnapshot scratchSnapshot;
        OgreNewt::ContactSnapshot impactSnapshot;
        bool hasScratch = false;
        bool hasImpact = false;
        OgreNewt::Contact* contact = nullptr;

        OgreNewt::Contact firstContact = mutableContactJoint.getFirstContact();
        for (contact = &firstContact; false == contact->isEmpty(); contact = &contact->getNext())
        {
            if (false == this->contactFunctionName.empty())
            {
                // Snapshot NOW - contact ptr is dangling by logic thread execution time
                OgreNewt::ContactSnapshot snap = contact->createSnapshot();
                const Ogre::String capturedContactFunctionName = this->contactFunctionName;

                NOWA::AppStateManager::LogicCommand logicCommand = [this, snap, capturedGameObject0, capturedGameObject1, capturedContactFunctionName]()
                {
                    if (nullptr == this->luaScript)
                    {
                        return;
                    }
                    this->luaScript->callTableFunction(capturedContactFunctionName, capturedGameObject0, capturedGameObject1, snap);
                };
                NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
            }

            if (false == this->contactOnceFunctionName.empty())
            {
                // The impact test used to compare against a lastNormalSpeed MEMBER, i.e. the
                // speed of whatever contact was processed last - which under parallel contact
                // processing is a completely unrelated body pair. That made the result
                // meaningless regardless of any locking, so the shared value is gone and the
                // decision is made from this contact joint alone: the strongest normal speed
                // among its own contact points.
                const Ogre::Real currentNormalSpeed = contact->getNormalSpeed();
                if (currentNormalSpeed > maxNormalSpeed)
                {
                    maxNormalSpeed = currentNormalSpeed;
                    impactSnapshot = contact->createSnapshot();
                    hasImpact = true;
                }
            }

            if (false == this->contactScratchFunctionName.empty())
            {
                Ogre::Vector3 point;
                Ogre::Vector3 normal;
                contact->getPositionAndNormal(point, normal);

                // Both velocities are sampled at the SAME contact point, once per body. The
                // former code called getPositionAndNormal() twice into two variable pairs,
                // which necessarily produced identical values and left one normal unused.
                const Ogre::Vector3 pointVeloc0(orderedBody0->getVelocityAtPoint(point));
                const Ogre::Vector3 pointVeloc1(orderedBody1->getVelocityAtPoint(point));
                const Ogre::Vector3 veloc(pointVeloc1 - pointVeloc0);

                const Ogre::Real verticalSpeed = normal.dotProduct(veloc);

                const Ogre::Vector3 tangVeloc(veloc - (normal * verticalSpeed));
                const Ogre::Real tangentSpeed = tangVeloc.dotProduct(tangVeloc);
                if (tangentSpeed > maxTangentSpeed)
                {
                    maxTangentSpeed = tangentSpeed;
                    // Snapshot the best scratch contact NOW while still valid
                    scratchSnapshot = contact->createSnapshot();
                    hasScratch = true;
                }
            }
        }

        // Impact
        if (false == this->contactOnceFunctionName.empty() && true == hasImpact && maxNormalSpeed > 3.0f)
        {
            const Ogre::String capturedContactOnceFunctionName = this->contactOnceFunctionName;

            NOWA::AppStateManager::LogicCommand logicCommand = [this, impactSnapshot, capturedGameObject0, capturedGameObject1, capturedContactOnceFunctionName]()
            {
                if (nullptr == this->luaScript)
                {
                    return;
                }
                this->luaScript->callTableFunction(capturedContactOnceFunctionName, capturedGameObject0, capturedGameObject1, impactSnapshot);
            };
            NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
        }

        // Scratch
        if (false == this->contactScratchFunctionName.empty())
        {
            maxTangentSpeed = Ogre::Math::Sqrt(maxTangentSpeed);
            if (true == hasScratch && maxTangentSpeed > 10.0f)
            {
                const Ogre::String capturedContactScratchFunctionName = this->contactScratchFunctionName;

                NOWA::AppStateManager::LogicCommand logicCommand = [this, scratchSnapshot, capturedGameObject0, capturedGameObject1, capturedContactScratchFunctionName]()
                {
                    if (nullptr == this->luaScript)
                    {
                        return;
                    }
                    this->luaScript->callTableFunction(capturedContactScratchFunctionName, capturedGameObject0, capturedGameObject1, scratchSnapshot);
                };
                NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
            }
        }
    }

}; // namespace end