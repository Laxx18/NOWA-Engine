#include "NOWAPrecompiled.h"
#include "PhysicsBuoyancyComponent.h"
#include "utilities/XMLConverter.h"
#include "main/AppStateManager.h"
#include "PhysicsActiveComponent.h"

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    // ============================================================
    // PhysicsBuoyancyTriggerCallback (Lua/event layer)
    // ============================================================
    PhysicsBuoyancyComponent::PhysicsBuoyancyTriggerCallback::PhysicsBuoyancyTriggerCallback(
        GameObject* owner,
        Ogre::Real waterToSolidVolumeRatio,
        Ogre::Real viscosity,
        LuaScript* luaScript,
        luabind::object& enterClosureFunction,
        luabind::object& insideClosureFunction,
        luabind::object& leaveClosureFunction)
        : OgreNewt::BuoyancyForceTriggerCallback(waterToSolidVolumeRatio, viscosity)
        , owner(owner)
        , luaScript(luaScript)
        , onInsideFunctionAvailable(true)
        , categoryId(0)
        , enterClosureFunction(enterClosureFunction)
        , insideClosureFunction(insideClosureFunction)
        , leaveClosureFunction(leaveClosureFunction)
    {
        if (false == this->insideClosureFunction.is_valid())
        {
            this->onInsideFunctionAvailable = false;
        }
    }

    PhysicsBuoyancyComponent::PhysicsBuoyancyTriggerCallback::~PhysicsBuoyancyTriggerCallback()
    {
    }

    void PhysicsBuoyancyComponent::PhysicsBuoyancyTriggerCallback::OnEnter(const OgreNewt::Body* visitor)
    {
        // Base: set per-body density in ND4 material
        OgreNewt::BuoyancyForceTriggerCallback::OnEnter(visitor);

        // Note: visitor is the body ENTERING the volume (not the body that owns this trigger).
        PhysicsComponent* visitorPhysicsComponent =
            OgreNewt::any_cast<PhysicsComponent*>(visitor->getUserData());
        if (nullptr != visitorPhysicsComponent)
        {
            GameObjectPtr visitorGameObjectPtr = visitorPhysicsComponent->getOwner();

            // Only allow for physics active components
            auto physicsActiveComponent =
                NOWA::makeStrongPtr(visitorGameObjectPtr->getComponent<PhysicsActiveComponent>());
            if (nullptr == physicsActiveComponent)
                return;

            // Check for correct category
            unsigned int type = visitorGameObjectPtr->getCategoryId();
            unsigned int finalType = type & this->categoryId;
            if (type == finalType)
            {
                if (nullptr != luaScript)
                {
                    if (this->enterClosureFunction.is_valid())
                    {
                        NOWA::AppStateManager::LogicCommand logicCommand =
                            [this, visitorGameObjectPtr]()
                            {
                                try
                                {
                                    luabind::call_function<void>(this->enterClosureFunction,
                                        visitorGameObjectPtr.get());
                                }
                                catch (luabind::error& error)
                                {
                                    luabind::object errorMsg(luabind::from_stack(error.state(), -1));
                                    std::stringstream msg;
                                    msg << errorMsg;

                                    Ogre::LogManager::getSingleton().logMessage(
                                        Ogre::LML_CRITICAL,
                                        "[PhysicsBuoyancyComponent::PhysicsBuoyancyTriggerCallback] "
                                        "Caught error in 'reactOnEnter' Error: " +
                                        Ogre::String(error.what()) + " details: " + msg.str());
                                }
                            };
                        NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
                    }
                }
            }
        }
    }

    void PhysicsBuoyancyComponent::PhysicsBuoyancyTriggerCallback::OnInside(const OgreNewt::Body* visitor)
    {
        // Base does nothing (no physics) – we only use this for Lua callbacks.
        OgreNewt::BuoyancyForceTriggerCallback::OnInside(visitor);

        if (false == this->onInsideFunctionAvailable)
            return;

        PhysicsComponent* visitorPhysicsComponent =
            OgreNewt::any_cast<PhysicsComponent*>(visitor->getUserData());
        if (nullptr != visitorPhysicsComponent)
        {
            GameObjectPtr visitorGameObjectPtr = visitorPhysicsComponent->getOwner();

            auto physicsActiveComponent =
                NOWA::makeStrongPtr(visitorGameObjectPtr->getComponent<PhysicsActiveComponent>());
            if (nullptr == physicsActiveComponent)
                return;

            unsigned int type = visitorGameObjectPtr->getCategoryId();
            unsigned int finalType = type & this->categoryId;
            if (type == finalType)
            {
                if (nullptr != luaScript)
                {
                    if (this->insideClosureFunction.is_valid())
                    {
                        NOWA::AppStateManager::LogicCommand logicCommand =
                            [this, visitorGameObjectPtr]()
                            {
                                try
                                {
                                    luabind::call_function<void>(this->insideClosureFunction,
                                        visitorGameObjectPtr.get());
                                }
                                catch (luabind::error& error)
                                {
                                    luabind::object errorMsg(luabind::from_stack(error.state(), -1));
                                    std::stringstream msg;
                                    msg << errorMsg;

                                    Ogre::LogManager::getSingleton().logMessage(
                                        Ogre::LML_CRITICAL,
                                        "[PhysicsBuoyancyComponent::PhysicsBuoyancyTriggerCallback] "
                                        "Caught error in 'reactOnInside' Error: " +
                                        Ogre::String(error.what()) + " details: " + msg.str());
                                }
                            };
                        NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
                    }
                }
            }
        }
    }

    void PhysicsBuoyancyComponent::PhysicsBuoyancyTriggerCallback::OnExit(const OgreNewt::Body* visitor)
    {
        // Base does nothing (no physics).
        OgreNewt::BuoyancyForceTriggerCallback::OnExit(visitor);

        PhysicsComponent* visitorPhysicsComponent =
            OgreNewt::any_cast<PhysicsComponent*>(visitor->getUserData());
        if (nullptr != visitorPhysicsComponent)
        {
            GameObjectPtr visitorGameObjectPtr = visitorPhysicsComponent->getOwner();

            auto physicsActiveComponent =
                NOWA::makeStrongPtr(visitorGameObjectPtr->getComponent<PhysicsActiveComponent>());
            if (nullptr == physicsActiveComponent)
                return;

            unsigned int type = visitorGameObjectPtr->getCategoryId();
            unsigned int finalType = type & this->categoryId;
            if (type == finalType)
            {
                if (nullptr != luaScript)
                {
                    if (this->leaveClosureFunction.is_valid())
                    {
                        NOWA::AppStateManager::LogicCommand logicCommand =
                            [this, visitorGameObjectPtr]()
                            {
                                try
                                {
                                    luabind::call_function<void>(this->leaveClosureFunction,
                                        visitorGameObjectPtr.get());
                                }
                                catch (luabind::error& error)
                                {
                                    luabind::object errorMsg(luabind::from_stack(error.state(), -1));
                                    std::stringstream msg;
                                    msg << errorMsg;

                                    Ogre::LogManager::getSingleton().logMessage(
                                        Ogre::LML_CRITICAL,
                                        "[PhysicsBuoyancyComponent::PhysicsBuoyancyTriggerCallback] "
                                        "Caught error in 'reactOnLeave' Error: " +
                                        Ogre::String(error.what()) + " details: " + msg.str());
                                }
                            };
                        NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
                    }
                }
            }
        }
    }

    void PhysicsBuoyancyComponent::PhysicsBuoyancyTriggerCallback::setLuaScript(LuaScript* luaScript)
    {
        this->luaScript = luaScript;
        if (false == this->insideClosureFunction.is_valid())
        {
            this->onInsideFunctionAvailable = false;
        }
    }

    void PhysicsBuoyancyComponent::PhysicsBuoyancyTriggerCallback::setCategoryId(unsigned int categoryId)
    {
        this->categoryId = categoryId;
    }

    void PhysicsBuoyancyComponent::PhysicsBuoyancyTriggerCallback::setTriggerFunctions(
        luabind::object& enterClosureFunction,
        luabind::object& insideClosureFunction,
        luabind::object& leaveClosureFunction)
    {
        this->enterClosureFunction = enterClosureFunction;
        this->insideClosureFunction = insideClosureFunction;
        this->leaveClosureFunction = leaveClosureFunction;

        if (false == this->insideClosureFunction.is_valid())
        {
            this->onInsideFunctionAvailable = false;
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    PhysicsBuoyancyComponent::PhysicsBuoyancyComponent()
        : PhysicsComponent(),
        waterToSolidVolumeRatio(new Variant(PhysicsBuoyancyComponent::AttrWaterToSolidVolumeRatio(), 0.9f, this->attributes)),
        viscosity(new Variant(PhysicsBuoyancyComponent::AttrViscosity(), 0.995f, this->attributes)),
        buoyancyGravity(new Variant(PhysicsBuoyancyComponent::AttrBuoyancyGravity(), Ogre::Vector3(0.0f, -19.8f, 0.0f), this->attributes)),
        offsetHeight(new Variant(PhysicsBuoyancyComponent::AttrOffsetHeight(), 6, this->attributes)),
        categories(new Variant(PhysicsBuoyancyComponent::AttrCategories(), Ogre::String("All"), this->attributes)),
        waveAmplitude(new Variant(PhysicsBuoyancyComponent::AttrWaveAmplitude(), 0.0f, this->attributes)),
        waveFrequency(new Variant(PhysicsBuoyancyComponent::AttrWaveFrequency(), 0.3f, this->attributes)),
        physicsBuoyancyTriggerCallback(nullptr),
        newleyCreated(true),
        categoriesId(GameObjectController::ALL_CATEGORIES_ID)
    {
        std::vector<Ogre::String> collisionTypes(1);
        collisionTypes[0] = "ConvexHull";
        this->collisionType->setValue(collisionTypes);
        this->collisionType->setVisible(false);
        this->mass->setVisible(false);
    }

    PhysicsBuoyancyComponent::~PhysicsBuoyancyComponent()
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsBuoyancyComponent] Destructor physics buoyancy component for game object: " + this->gameObjectPtr->getName());

        this->physicsBuoyancyTriggerCallback = nullptr;
    }

    bool PhysicsBuoyancyComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CollisionType")
        {
            this->collisionType->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", ""));
            // For artifact component only tree collision hull is possible
            // this->collisionType->setReadOnly(true);
            // Do not double the propagation, in gameobjectfactory there is also a propagation
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WaterToSolidVolumeRatio")
        {
            this->waterToSolidVolumeRatio->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Viscosity")
        {
            this->viscosity->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BuoyancyGravity")
        {
            this->buoyancyGravity->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OffsetHeight")
        {
            this->offsetHeight->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Categories")
        {
            this->categories->setValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WaveAmplitude")
        {
            this->waveAmplitude->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WaveFrequency")
        {
            this->waveFrequency->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        this->newleyCreated = false;
        return true;
    }

    GameObjectCompPtr PhysicsBuoyancyComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        PhysicsBuoyancyCompPtr clonedCompPtr(boost::make_shared<PhysicsBuoyancyComponent>());

        clonedCompPtr->setCollisionType(this->collisionType->getListSelectedValue());

        clonedCompPtr->setWaterToSolidVolumeRatio(this->waterToSolidVolumeRatio->getReal());
        clonedCompPtr->setViscosity(this->viscosity->getReal());
        clonedCompPtr->setBuoyancyGravity(this->buoyancyGravity->getVector3());
        clonedCompPtr->setOffsetHeight(this->offsetHeight->getReal());
        clonedCompPtr->setCategories(this->categories->getString());
        clonedCompPtr->setWaveAmplitude(this->waveAmplitude->getReal());
        clonedCompPtr->setWaveFrequency(this->waveFrequency->getReal());
        // Bug in newton, setting afterwards collidable to true, will not work, hence do not clone this property
        // clonedCompPtr->setCollidable(this->collidable->getBool());

        clonedGameObjectPtr->addComponent(clonedCompPtr);
        clonedCompPtr->setOwner(clonedGameObjectPtr);

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
        return clonedCompPtr;
    }

    bool PhysicsBuoyancyComponent::postInit(void)
    {
        bool success = PhysicsComponent::postInit();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsBuoyancyComponent] Init physics buoyancy component for game object: " + this->gameObjectPtr->getName());

        if (!this->createStaticBody())
        {
            return false;
        }

        this->physicsBody->setType(gameObjectPtr->getCategoryId());

        const auto materialId = AppStateManager::getSingletonPtr()->getGameObjectController()->getMaterialID(this->gameObjectPtr.get(), this->ogreNewt);
        this->physicsBody->setMaterialGroupID(materialId);

        if (true == this->newleyCreated)
        {
            this->offsetHeight->setValue(this->gameObjectPtr->getPosition().y + this->gameObjectPtr->getSize().y);
            this->newleyCreated = false;
        }

        return success;
    }

    bool PhysicsBuoyancyComponent::connect(void)
    {
        // Depending on order, in postInit maybe there is no valid lua script yet, so set it here again
        if (nullptr != this->physicsBody)
        {
            auto triggerCallback = static_cast<OgreNewt::BuoyancyBody*>(this->physicsBody)->getTriggerCallback();
            auto* cb = static_cast<PhysicsBuoyancyComponent::PhysicsBuoyancyTriggerCallback*>(triggerCallback);

            cb->setTriggerFunctions(this->enterClosureFunction, this->insideClosureFunction, this->leaveClosureFunction);
            cb->setLuaScript(this->gameObjectPtr->getLuaScript());
            cb->setCategoryId(this->categoriesId);
        }

        return true;
    }

    Ogre::String PhysicsBuoyancyComponent::getClassName(void) const
    {
        return "PhysicsBuoyancyComponent";
    }

    Ogre::String PhysicsBuoyancyComponent::getParentClassName(void) const
    {
        return "PhysicsComponent";
    }

    void PhysicsBuoyancyComponent::showDebugData(void)
    {
        GameObjectComponent::showDebugData();
        if (nullptr != this->physicsBody)
        {
            ENQUEUE_RENDER_COMMAND_WAIT("PhysicsBuoyancyComponent::showDebugData",
                {
                    this->physicsBody->showDebugCollision(false, this->bShowDebugData);
                });
        }
    }

    bool PhysicsBuoyancyComponent::createStaticBody(void)
    {
        this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
        this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
        this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();

        Ogre::String meshName;
        OgreNewt::CollisionPtr collision;

        ENQUEUE_RENDER_COMMAND_MULTI_WAIT("PhysicsBuoyancyComponent::createStaticBody", _2(&meshName, &collision),
            {
                Ogre::v1::Entity * entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
                if (nullptr != entity)
                {
                    meshName = entity->getMesh()->getName();
                    if (Ogre::StringUtil::match(meshName, "Plane*", true))
                    {
                        Ogre::Vector3 size = entity->getMesh()->getBounds().getSize() * this->initialScale;
                        size.y = 0.001f;
                        collision = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::Box(this->ogreNewt, size, this->gameObjectPtr->getCategoryId(), Ogre::Quaternion::IDENTITY, Ogre::Vector3::ZERO));
                    }
                    else
                    {
                        collision = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::ConvexHull(this->ogreNewt, entity, this->gameObjectPtr->getCategoryId()));
                    }
                }
                else
                {
                    Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
                    if (nullptr != item)
                    {
                        meshName = item->getMesh()->getName();
                        if (Ogre::StringUtil::match(meshName, "Plane*", true))
                        {
                            Ogre::Vector3 size = item->getMesh()->getAabb().getSize() * this->initialScale;
                            size.y = 0.001f;
                            collision = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::Box(this->ogreNewt, size, this->gameObjectPtr->getCategoryId(), Ogre::Quaternion::IDENTITY, Ogre::Vector3::ZERO));
                        }
                        else
                        {
                            collision = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::ConvexHull(this->ogreNewt, item, this->gameObjectPtr->getCategoryId()));
                        }
                    }
                    else
                    {
                        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsTriggerComponent] Error cannot create static body, because the " "game object has no entity/item with mesh for game object: " + this->gameObjectPtr->getName());
                        return false;
                    }
                }
            });

        if (nullptr == collision)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsTriggerComponent] Could not create collision file for game object: " + this->gameObjectPtr->getName() + " and mesh: " + meshName + ". Maybe the mesh is corrupt.");
            throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[PhysicsTriggerComponent] Could not create collision file for game object: " + this->gameObjectPtr->getName() + " and mesh: " + meshName + ". Maybe the mesh is corrupt.\n", "NOWA");
        }

        if (nullptr == this->physicsBuoyancyTriggerCallback)
        {
            this->physicsBuoyancyTriggerCallback = new PhysicsBuoyancyComponent::PhysicsBuoyancyTriggerCallback(
                this->gameObjectPtr.get(),
                this->waterToSolidVolumeRatio->getReal(),
                this->viscosity->getReal(),
                this->gameObjectPtr->getLuaScript(),
                this->enterClosureFunction,
                this->insideClosureFunction,
                this->leaveClosureFunction);
        }

        if (nullptr == this->physicsBody)
        {
            this->physicsBody = new OgreNewt::BuoyancyBody(this->ogreNewt, this->gameObjectPtr->getSceneManager(), collision, this->physicsBuoyancyTriggerCallback);
        }

        auto* body = static_cast<OgreNewt::BuoyancyBody*>(this->physicsBody);

        if (this->offsetHeight->getReal() == 0.0f)
        {
            // Use automatic plane detection via raycast
            body->setUseRaycastPlane(true);
        }
        else
        {
            body->setUseRaycastPlane(false);
            body->setFluidPlane(Ogre::Plane(Ogre::Vector3(0.0f, 1.0f, 0.0f), this->offsetHeight->getReal()));
        }

        body->setGravity(this->buoyancyGravity->getVector3());
        body->setWaterToSolidVolumeRatio(this->waterToSolidVolumeRatio->getReal());
        body->setViscosity(this->viscosity->getReal());
        body->setWaveAmplitude(this->waveAmplitude->getReal());
        body->setWaveFrequency(this->waveFrequency->getReal());

        // Set mass to 0 = infinity = static
        this->physicsBody->setMassMatrix(0.0f, Ogre::Vector3::ZERO);
        this->physicsBody->setUserData(OgreNewt::Any(dynamic_cast<PhysicsComponent*>(this)));
        this->physicsBody->attachNode(this->gameObjectPtr->getSceneNode());

        this->setPosition(this->initialPosition);
        this->setOrientation(this->initialOrientation);

        return true;
    }

    void PhysicsBuoyancyComponent::reCreateCollision(bool overwrite)
    {
        if (nullptr != this->physicsBody)
        {
            this->destroyCollision();
        }
        this->createStaticBody();
    }

    void PhysicsBuoyancyComponent::update(Ogre::Real dt, bool notSimulating)
    {
        if (false == notSimulating)
        {
            // If needed in future: static_cast<OgreNewt::BuoyancyBody*>(this->physicsBody)->update(dt);
        }
    }

    void PhysicsBuoyancyComponent::actualizeValue(Variant* attribute)
    {
        PhysicsComponent::actualizeValue(attribute);

        if (PhysicsBuoyancyComponent::AttrWaterToSolidVolumeRatio() == attribute->getName())
        {
            this->setWaterToSolidVolumeRatio(attribute->getReal());
        }
        else if (PhysicsBuoyancyComponent::AttrViscosity() == attribute->getName())
        {
            this->setViscosity(attribute->getReal());
        }
        else if (PhysicsBuoyancyComponent::AttrBuoyancyGravity() == attribute->getName())
        {
            this->setBuoyancyGravity(attribute->getVector3());
        }
        else if (PhysicsBuoyancyComponent::AttrOffsetHeight() == attribute->getName())
        {
            this->setOffsetHeight(attribute->getReal());
        }
        else if (PhysicsBuoyancyComponent::AttrCategories() == attribute->getName())
        {
            this->setCategories(attribute->getString());
        }
        else if (PhysicsBuoyancyComponent::AttrWaveAmplitude() == attribute->getName())
        {
            this->setWaveAmplitude(attribute->getReal());
        }
        else if (PhysicsBuoyancyComponent::AttrWaveFrequency() == attribute->getName())
        {
            this->setWaveFrequency(attribute->getReal());
        }
    }

    void PhysicsBuoyancyComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        GameObjectComponent::writeXML(propertiesXML, doc);

        xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "CollisionType"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->collisionType->getListSelectedValue())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "WaterToSolidVolumeRatio"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->waterToSolidVolumeRatio->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Viscosity"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->viscosity->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "BuoyancyGravity"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->buoyancyGravity->getVector3())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "OffsetHeight"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->offsetHeight->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Categories"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->categories->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "WaveAmplitude"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->waveAmplitude->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "WaveFrequency"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->waveFrequency->getReal())));
        propertiesXML->append_node(propertyXML);
    }

    void PhysicsBuoyancyComponent::setWaterToSolidVolumeRatio(Ogre::Real waterToSolidVolumeRatio)
    {
        this->waterToSolidVolumeRatio->setValue(waterToSolidVolumeRatio);

        if (nullptr != this->physicsBuoyancyTriggerCallback)
            this->physicsBuoyancyTriggerCallback->setWaterToSolidVolumeRatio(waterToSolidVolumeRatio);

        if (this->physicsBody)
        {
            if (auto* body = dynamic_cast<OgreNewt::BuoyancyBody*>(this->physicsBody))
                body->setWaterToSolidVolumeRatio(waterToSolidVolumeRatio);
        }
    }

    Ogre::Real PhysicsBuoyancyComponent::getWaterToSolidVolumeRatio(void) const
    {
        return this->waterToSolidVolumeRatio->getReal();
    }

    void PhysicsBuoyancyComponent::setViscosity(Ogre::Real viscosity)
    {
        this->viscosity->setValue(viscosity);

        if (nullptr != this->physicsBuoyancyTriggerCallback)
            this->physicsBuoyancyTriggerCallback->setViscosity(viscosity);

        if (this->physicsBody)
        {
            if (auto* body = dynamic_cast<OgreNewt::BuoyancyBody*>(this->physicsBody))
                body->setViscosity(viscosity);
        }
    }

    Ogre::Real PhysicsBuoyancyComponent::getViscosity(void) const
    {
        return this->viscosity->getReal();
    }

    void PhysicsBuoyancyComponent::setBuoyancyGravity(const Ogre::Vector3& buoyancyGravity)
    {
        this->buoyancyGravity->setValue(buoyancyGravity);

        if (this->physicsBody)
        {
            if (auto* body = dynamic_cast<OgreNewt::BuoyancyBody*>(this->physicsBody))
                body->setGravity(buoyancyGravity);
        }
    }

    const Ogre::Vector3 PhysicsBuoyancyComponent::getBuoyancyGravity(void) const
    {
        return this->buoyancyGravity->getVector3();
    }

    void PhysicsBuoyancyComponent::setOffsetHeight(Ogre::Real offsetHeight)
    {
        this->offsetHeight->setValue(offsetHeight);

        if (this->physicsBody)
        {
            if (auto* body = dynamic_cast<OgreNewt::BuoyancyBody*>(this->physicsBody))
            {
                if (offsetHeight == 0.0f)
                {
                    // Switch to auto-plane (raycast) mode
                    body->setUseRaycastPlane(true);
                }
                else
                {
                    body->setUseRaycastPlane(false);
                    body->setFluidPlane(Ogre::Plane(Ogre::Vector3(0.0f, 1.0f, 0.0f), offsetHeight));
                }
            }
        }
    }

    Ogre::Real PhysicsBuoyancyComponent::getOffsetHeight(void) const
    {
        return this->offsetHeight->getReal();
    }

    void PhysicsBuoyancyComponent::setCategories(const Ogre::String& categories)
    {
        this->categories->setValue(categories);
        this->categoriesId =
            AppStateManager::getSingletonPtr()->getGameObjectController()->generateCategoryId(
                categories);
    }

    Ogre::String PhysicsBuoyancyComponent::getCategories(void) const
    {
        return this->categories->getString();
    }

    void PhysicsBuoyancyComponent::setWaveAmplitude(Ogre::Real waveAmplitude)
    {
        this->waveAmplitude->setValue(waveAmplitude);

        if (this->physicsBody)
        {
            if (auto* body = dynamic_cast<OgreNewt::BuoyancyBody*>(this->physicsBody))
                body->setWaveAmplitude(waveAmplitude);
        }
    }

    Ogre::Real PhysicsBuoyancyComponent::getWaveAmplitude(void) const
    {
        return this->waveAmplitude->getReal();
    }

    void PhysicsBuoyancyComponent::setWaveFrequency(Ogre::Real waveFrequency)
    {
        this->waveFrequency->setValue(waveFrequency);

        if (this->physicsBody)
        {
            if (auto* body = dynamic_cast<OgreNewt::BuoyancyBody*>(this->physicsBody))
                body->setWaveFrequency(waveFrequency);
        }
    }

    Ogre::Real PhysicsBuoyancyComponent::getWaveFrequency(void) const
    {
        return this->waveFrequency->getReal();
    }

    void PhysicsBuoyancyComponent::reactOnEnter(luabind::object closureFunction)
    {
        this->enterClosureFunction = closureFunction;

        if (nullptr != this->physicsBody)
        {
            if (nullptr != this->physicsBuoyancyTriggerCallback)
            {
                this->physicsBuoyancyTriggerCallback->setTriggerFunctions(this->enterClosureFunction, this->insideClosureFunction, this->leaveClosureFunction);
            }
        }
    }

    void PhysicsBuoyancyComponent::reactOnInside(luabind::object closureFunction)
    {
        this->insideClosureFunction = closureFunction;

        if (nullptr != this->physicsBody)
        {
            if (nullptr != this->physicsBuoyancyTriggerCallback)
            {
                this->physicsBuoyancyTriggerCallback->setTriggerFunctions(this->enterClosureFunction, this->insideClosureFunction, this->leaveClosureFunction);
            }
        }
    }

    void PhysicsBuoyancyComponent::reactOnLeave(luabind::object closureFunction)
    {
        this->leaveClosureFunction = closureFunction;

        if (nullptr != this->physicsBody)
        {
            if (nullptr != this->physicsBuoyancyTriggerCallback)
            {
                this->physicsBuoyancyTriggerCallback->setTriggerFunctions(this->enterClosureFunction, this->insideClosureFunction, this->leaveClosureFunction);
            }
        }
    }

}; // namespace end
