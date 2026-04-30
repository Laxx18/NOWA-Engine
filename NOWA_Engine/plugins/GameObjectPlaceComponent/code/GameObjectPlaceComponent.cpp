/*
Copyright (c) 2025 Lukas Kalinowski
GPL v3
*/

#include "NOWAPrecompiled.h"
#include "GameObjectPlaceComponent.h"
#include "OgreAbiUtils.h"
#include "gameobject/GameObjectFactory.h"
#include "main/AppStateManager.h"
#include "main/InputDeviceCore.h"
#include "modules/LuaScriptApi.h"
#include "utilities/MathHelper.h"
#include "utilities/XMLConverter.h"

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    GameObjectPlaceComponent::GameObjectPlaceComponent() :
        GameObjectComponent(),
        name("GameObjectPlaceComponent"),
        activated(new Variant(GameObjectPlaceComponent::AttrActivated(), true, this->attributes)),
        placeObjectCount(new Variant(GameObjectPlaceComponent::AttrPlaceObjectCount(), 0, this->attributes)),
        activeGameObjectId(0),
        isPlacing(false),
        currentHitPoint(Ogre::Vector3::ZERO),
        raySceneQuery(nullptr)
    {
        // Changing count refreshes the properties panel so new ID fields appear/disappear
        this->placeObjectCount->addUserData(GameObject::AttrActionNeedRefresh());
    }

    GameObjectPlaceComponent::~GameObjectPlaceComponent()
    {
    }

    void GameObjectPlaceComponent::install(const Ogre::NameValuePairList* options)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<GameObjectPlaceComponent>(GameObjectPlaceComponent::getStaticClassId(), GameObjectPlaceComponent::getStaticClassName());
    }

    void GameObjectPlaceComponent::initialise()
    {
    }

    void GameObjectPlaceComponent::shutdown()
    {
    }

    void GameObjectPlaceComponent::uninstall()
    {
    }

    void GameObjectPlaceComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    const Ogre::String& GameObjectPlaceComponent::getName() const
    {
        return this->name;
    }

    bool GameObjectPlaceComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
        {
            this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PlaceObjectCount")
        {
            this->placeObjectCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        if (this->gameObjectIds.size() < this->placeObjectCount->getUInt())
        {
            this->gameObjectIds.resize(this->placeObjectCount->getUInt());
        }

        for (size_t i = 0; i < this->placeObjectCount->getUInt(); i++)
        {
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrGameObjectId() + Ogre::StringConverter::toString(i))
            {
                if (nullptr == this->gameObjectIds[i])
                {
                    this->gameObjectIds[i] = new Variant(AttrGameObjectId() + Ogre::StringConverter::toString(i), XMLConverter::getAttrib(propertyElement, "data"), this->attributes);
                }
                else
                {
                    this->gameObjectIds[i]->setValue(XMLConverter::getAttrib(propertyElement, "data"));
                }
                propertyElement = propertyElement->next_sibling("property");
            }
            else
            {
                this->gameObjectIds[i] = new Variant(AttrGameObjectId() + Ogre::StringConverter::toString(i), Ogre::String("0"), this->attributes);
            }
            // Show a file-open-style action to pick from existing game object ids
            this->gameObjectIds[i]->addUserData(GameObject::AttrActionSeparator());
        }

        return true;
    }

    bool GameObjectPlaceComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[GameObjectPlaceComponent] Init component for game object: " + this->gameObjectPtr->getName());

        // Create a ray scene query for mouse-to-world hit testing
        this->raySceneQuery = this->gameObjectPtr->getSceneManager()->createRayQuery(Ogre::Ray());
        this->raySceneQuery->setSortByDistance(true);

        return true;
    }

    bool GameObjectPlaceComponent::connect(void)
    {
        GameObjectComponent::connect();

        // Delay listener registration by one frame to avoid iterator invalidation
        // during the connect phase when other listeners may still be iterating
        NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.25f));
        auto ptrFunction = [this]()
        {
            InputDeviceCore::getSingletonPtr()->addKeyListener(this, GameObjectPlaceComponent::getStaticClassName());
            InputDeviceCore::getSingletonPtr()->addMouseListener(this, GameObjectPlaceComponent::getStaticClassName());
        };
        NOWA::ProcessPtr closureProcess(new NOWA::ClosureProcess(ptrFunction));
        delayProcess->attachChild(closureProcess);
        NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);

        return true;
    }

    bool GameObjectPlaceComponent::disconnect(void)
    {
        GameObjectComponent::disconnect();

        // If we were placing when simulation stopped, clean up
        if (this->isPlacing)
        {
            this->endPlacement(true);
        }

        InputDeviceCore::getSingletonPtr()->removeKeyListener(GameObjectPlaceComponent::getStaticClassName());
        InputDeviceCore::getSingletonPtr()->removeMouseListener(GameObjectPlaceComponent::getStaticClassName());

        return true;
    }

    bool GameObjectPlaceComponent::onCloned(void)
    {
        return true;
    }

    void GameObjectPlaceComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();

        if (this->isPlacing)
        {
            this->endPlacement(true);
        }

        InputDeviceCore::getSingletonPtr()->removeKeyListener(GameObjectPlaceComponent::getStaticClassName());
        InputDeviceCore::getSingletonPtr()->removeMouseListener(GameObjectPlaceComponent::getStaticClassName());

        if (nullptr != this->raySceneQuery)
        {
            this->gameObjectPtr->getSceneManager()->destroyQuery(this->raySceneQuery);
            this->raySceneQuery = nullptr;
        }
    }

    void GameObjectPlaceComponent::onOtherComponentRemoved(unsigned int index)
    {
    }
    void GameObjectPlaceComponent::onOtherComponentAdded(unsigned int index)
    {
    }

    GameObjectCompPtr GameObjectPlaceComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        // This component should not be cloned — it is a singleton manager
        return nullptr;
    }

    void GameObjectPlaceComponent::update(Ogre::Real dt, bool notSimulating)
    {
        // Nothing needed in update — all logic is event-driven via mouse/key listeners
    }

    void GameObjectPlaceComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
        else if (AttrPlaceObjectCount() == attribute->getName())
        {
            this->setPlaceObjectCount(attribute->getUInt());
        }
        else
        {
            for (unsigned int i = 0; i < this->placeObjectCount->getUInt(); i++)
            {
                if (AttrGameObjectId() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setGameObjectId(i, attribute->getString());
                }
            }
        }
    }

    void GameObjectPlaceComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        GameObjectComponent::writeXML(propertiesXML, doc);

        xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "PlaceObjectCount"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->placeObjectCount->getUInt())));
        propertiesXML->append_node(propertyXML);

        for (size_t i = 0; i < this->placeObjectCount->getUInt(); i++)
        {
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrGameObjectId() + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->gameObjectIds[i]->getString())));
            propertiesXML->append_node(propertyXML);
        }
    }

    Ogre::String GameObjectPlaceComponent::getClassName(void) const
    {
        return "GameObjectPlaceComponent";
    }
    Ogre::String GameObjectPlaceComponent::getParentClassName(void) const
    {
        return "GameObjectComponent";
    }

    void GameObjectPlaceComponent::setActivated(bool activated)
    {
        this->activated->setValue(activated);
    }

    bool GameObjectPlaceComponent::isActivated(void) const
    {
        return this->activated->getBool();
    }

    void GameObjectPlaceComponent::setPlaceObjectCount(unsigned int count)
    {
        size_t oldSize = this->gameObjectIds.size();
        this->placeObjectCount->setValue(count);

        if (count > oldSize)
        {
            this->gameObjectIds.resize(count);
            for (size_t i = oldSize; i < count; i++)
            {
                this->gameObjectIds[i] = new Variant(AttrGameObjectId() + Ogre::StringConverter::toString(i), Ogre::String("0"), this->attributes);
                this->gameObjectIds[i]->addUserData(GameObject::AttrActionSeparator());
            }
        }
        else if (count < oldSize)
        {
            this->eraseVariants(this->gameObjectIds, count);
        }
    }

    unsigned int GameObjectPlaceComponent::getPlaceObjectCount(void) const
    {
        return this->placeObjectCount->getUInt();
    }

    void GameObjectPlaceComponent::setGameObjectId(unsigned int index, const Ogre::String& id)
    {
        if (index >= this->gameObjectIds.size())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameObjectPlaceComponent] setGameObjectId: index " + Ogre::StringConverter::toString(index) + " out of range.");
            return;
        }
        this->gameObjectIds[index]->setValue(id);
    }

    Ogre::String GameObjectPlaceComponent::getGameObjectId(unsigned int index) const
    {
        if (index >= this->gameObjectIds.size())
        {
            return "0";
        }
        return this->gameObjectIds[index]->getString();
    }

    void GameObjectPlaceComponent::activatePlacement(const Ogre::String& gameObjectId)
    {
        if (false == this->activated->getBool() || false == this->bConnected)
        {
            return;
        }

        // Cancel any existing placement first
        if (this->isPlacing)
        {
            this->endPlacement(true);
        }

        unsigned long id = Ogre::StringConverter::parseUnsignedLong(gameObjectId);
        if (0 == id)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameObjectPlaceComponent] activatePlacement: invalid id '" + gameObjectId + "'.");
            return;
        }

        // Verify the id is in our configured list
        bool found = false;
        for (size_t i = 0; i < this->gameObjectIds.size(); i++)
        {
            if (Ogre::StringConverter::parseUnsignedLong(this->gameObjectIds[i]->getString()) == id)
            {
                found = true;
                break;
            }
        }
        if (false == found)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameObjectPlaceComponent] activatePlacement: id " + gameObjectId + " is not in the configured GameObjectId list. Add it in the editor.");
            return;
        }

        auto shadowGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(id);
        if (nullptr == shadowGameObjectPtr)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameObjectPlaceComponent] activatePlacement: game object with id " + gameObjectId + " not found.");
            return;
        }

        this->activeGameObjectId = id;
        this->isPlacing = true;

        // Make shadow visible so it acts as the placement preview
        shadowGameObjectPtr->setVisible(true);

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[GameObjectPlaceComponent] Placement started for game object: " + shadowGameObjectPtr->getName());
    }

    void GameObjectPlaceComponent::cancelPlacement(void)
    {
        if (this->isPlacing)
        {
            this->endPlacement(true);
        }
    }

    void GameObjectPlaceComponent::endPlacement(bool cancelled)
    {
        if (false == this->isPlacing)
        {
            return;
        }

        // Hide the shadow object again
        auto shadowGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->activeGameObjectId);
        if (nullptr != shadowGameObjectPtr)
        {
            shadowGameObjectPtr->setVisible(false);
        }

        this->isPlacing = false;
        this->activeGameObjectId = 0;
        this->currentHitPoint = Ogre::Vector3::ZERO;

        if (cancelled && this->cancelledClosureFunction.is_valid())
        {
            NOWA::AppStateManager::LogicCommand logicCommand = [this]()
            {
                try
                {
                    luabind::call_function<void>(this->cancelledClosureFunction);
                }
                catch (luabind::error& error)
                {
                    luabind::object errorMsg(luabind::from_stack(error.state(), -1));
                    std::stringstream msg;
                    msg << errorMsg;
                    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[GameObjectPlaceComponent] Caught error in 'reactOnPlacementCancelled': " + Ogre::String(error.what()) + " details: " + msg.str());
                }
            };
            NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
        }
    }

    Ogre::Vector3 GameObjectPlaceComponent::getMouseWorldPosition(const OIS::MouseEvent& evt)
    {
        if (nullptr == this->raySceneQuery)
        {
            return Ogre::Vector3::ZERO;
        }

        Ogre::Real x = 0.0f;
        Ogre::Real y = 0.0f;
        MathHelper::getInstance()->mouseToViewPort(evt.state.X.abs, evt.state.Y.abs, x, y, Core::getSingletonPtr()->getOgreRenderWindow());

        Ogre::Camera* camera = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
        if (nullptr == camera)
        {
            return Ogre::Vector3::ZERO;
        }

        Ogre::Ray mouseRay;
        ENQUEUE_GET_CAMERA_TO_VIEWPORT_RAY(camera, x, y, mouseRay);

        Ogre::Vector3 hitPoint = Ogre::Vector3::ZERO;
        Ogre::MovableObject* hitObject = nullptr;
        Ogre::Real closestDistance = 0.0f;
        Ogre::Vector3 normal = Ogre::Vector3::UNIT_Y;

        // Exclude the shadow object itself from the ray test so we hit the ground, not the preview
        std::vector<Ogre::MovableObject*> excludeObjects;
        if (this->activeGameObjectId != 0)
        {
            auto shadowGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->activeGameObjectId);
            if (nullptr != shadowGameObjectPtr && nullptr != shadowGameObjectPtr->getMovableObject())
            {
                excludeObjects.emplace_back(shadowGameObjectPtr->getMovableObject());
            }
        }

        MathHelper::getInstance()->getRaycastFromPoint(this->raySceneQuery, camera, hitPoint, (size_t&)hitObject, closestDistance, normal, &excludeObjects);

        return hitPoint;
    }

    // -----------------------------------------------------------------------
    // Mouse / Key handlers
    // -----------------------------------------------------------------------

    bool GameObjectPlaceComponent::mouseMoved(const OIS::MouseEvent& evt)
    {
        if (false == this->bConnected || false == this->isPlacing || false == this->activated->getBool())
        {
            return true;
        }

        // Skip if mouse is over a MyGUI widget
        if (nullptr != MyGUI::InputManager::getInstance().getMouseFocusWidget())
        {
            return true;
        }

        Ogre::Vector3 hitPoint = this->getMouseWorldPosition(evt);
        if (hitPoint == Ogre::Vector3::ZERO)
        {
            return true;
        }

        this->currentHitPoint = hitPoint;

        // Move the shadow game object to the hit point so it acts as a live preview
        auto shadowGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->activeGameObjectId);
        if (nullptr != shadowGameObjectPtr)
        {
            // Bottom-center the mesh on the hit point using the same helper EditorManager uses
            Ogre::SceneNode* node = shadowGameObjectPtr->getSceneNode();
            if (nullptr != node)
            {
                Ogre::Vector3 offset = Ogre::Vector3::ZERO;
                if (nullptr != shadowGameObjectPtr->getMovableObject())
                {
                    offset = MathHelper::getInstance()->getBottomCenterOfMesh(node, shadowGameObjectPtr->getMovableObject());
                }
                NOWA::GraphicsModule::getInstance()->updateNodeTransform(node, hitPoint + offset, node->getOrientation());
            }
        }

        return true;
    }

    bool GameObjectPlaceComponent::mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
    {
        if (false == this->bConnected || false == this->isPlacing || false == this->activated->getBool())
        {
            return true;
        }

        // Skip if mouse is over a MyGUI widget
        if (nullptr != MyGUI::InputManager::getInstance().getMouseFocusWidget())
        {
            return true;
        }

        if (id == OIS::MB_Left)
        {
            // Commit: clone the shadow object at the current hit position
            if (this->currentHitPoint == Ogre::Vector3::ZERO)
            {
                return true;
            }

            unsigned long originalId = this->activeGameObjectId;
            Ogre::Vector3 placePosition = this->currentHitPoint;

            // Hide shadow and reset state before clone so the clone itself is not hidden
            this->endPlacement(false);

            // Clone the original shadow game object at the world hit position
            auto clonedGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->clone(originalId,
                nullptr, // parent node: none, use scene root
                0,       // new id: auto-generated
                placePosition, Ogre::Quaternion::IDENTITY, Ogre::Vector3(1.0f, 1.0f, 1.0f),
                true // clone datablock
            );

            if (nullptr == clonedGameObjectPtr)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameObjectPlaceComponent] Failed to clone game object with id: " + Ogre::StringConverter::toString(originalId));
                return true;
            }

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[GameObjectPlaceComponent] Placed game object: " + clonedGameObjectPtr->getName() + " at " + Ogre::StringConverter::toString(placePosition));

            // Fire Lua callback with the new game object's id
            if (this->placedClosureFunction.is_valid())
            {
                Ogre::String newId = Ogre::StringConverter::toString(clonedGameObjectPtr->getId());

                NOWA::AppStateManager::LogicCommand logicCommand = [this, newId]()
                {
                    try
                    {
                        luabind::call_function<void>(this->placedClosureFunction, newId);
                    }
                    catch (luabind::error& error)
                    {
                        luabind::object errorMsg(luabind::from_stack(error.state(), -1));
                        std::stringstream msg;
                        msg << errorMsg;
                        Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[GameObjectPlaceComponent] Caught error in 'reactOnGameObjectPlaced': " + Ogre::String(error.what()) + " details: " + msg.str());
                    }
                };
                NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
            }
        }
        else if (id == OIS::MB_Right)
        {
            // Cancel placement
            this->endPlacement(true);
        }

        return true;
    }

    bool GameObjectPlaceComponent::mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
    {
        return true;
    }

    bool GameObjectPlaceComponent::keyPressed(const OIS::KeyEvent& keyEventRef)
    {
        if (false == this->bConnected || false == this->isPlacing)
        {
            return true;
        }

        if (keyEventRef.key == OIS::KC_ESCAPE)
        {
            this->endPlacement(true);
        }

        return true;
    }

    bool GameObjectPlaceComponent::keyReleased(const OIS::KeyEvent& keyEventRef)
    {
        return true;
    }

    void GameObjectPlaceComponent::reactOnGameObjectPlaced(luabind::object closureFunction)
    {
        this->placedClosureFunction = closureFunction;
    }

    void GameObjectPlaceComponent::reactOnPlacementCancelled(luabind::object closureFunction)
    {
        this->cancelledClosureFunction = closureFunction;
    }

    // -----------------------------------------------------------------------
    // Lua registration
    // -----------------------------------------------------------------------

    GameObjectPlaceComponent* getGameObjectPlaceComponent(GameObject* gameObject)
    {
        return makeStrongPtr<GameObjectPlaceComponent>(gameObject->getComponent<GameObjectPlaceComponent>()).get();
    }

    GameObjectPlaceComponent* getGameObjectPlaceComponentFromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<GameObjectPlaceComponent>(gameObject->getComponentFromName<GameObjectPlaceComponent>(name)).get();
    }

    void GameObjectPlaceComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
    {
        module(lua)
        [
            class_<GameObjectPlaceComponent, GameObjectComponent>("GameObjectPlaceComponent")
            .def("setActivated", &GameObjectPlaceComponent::setActivated)
            .def("isActivated", &GameObjectPlaceComponent::isActivated)
            .def("setPlaceObjectCount", &GameObjectPlaceComponent::setPlaceObjectCount)
            .def("getPlaceObjectCount", &GameObjectPlaceComponent::getPlaceObjectCount)
            .def("setGameObjectId", &GameObjectPlaceComponent::setGameObjectId)
            .def("getGameObjectId", &GameObjectPlaceComponent::getGameObjectId)
            .def("activatePlacement", &GameObjectPlaceComponent::activatePlacement)
            .def("cancelPlacement", &GameObjectPlaceComponent::cancelPlacement)
            .def("reactOnGameObjectPlaced", &GameObjectPlaceComponent::reactOnGameObjectPlaced)
            .def("reactOnPlacementCancelled", &GameObjectPlaceComponent::reactOnPlacementCancelled)
        ];

        LuaScriptApi::getInstance()->addClassToCollection("GameObjectPlaceComponent", "class inherits GameObjectComponent", getStaticInfoText());
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectPlaceComponent", "void setActivated(bool activated)", "Activates or deactivates the component.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectPlaceComponent", "bool isActivated()", "Gets whether the component is activated.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectPlaceComponent", "void setPlaceObjectCount(int count)", "Sets how many shadow game object slots are configured.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectPlaceComponent", "int getPlaceObjectCount()", "Gets the shadow game object slot count.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectPlaceComponent", "void setGameObjectId(int index, string id)", "Sets the shadow game object id for the given slot.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectPlaceComponent", "string getGameObjectId(int index)", "Gets the shadow game object id for the given slot.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectPlaceComponent", "void activatePlacement(string gameObjectId)", "Starts placement mode for the given shadow game object id. Call from Lua on inventory drop event.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectPlaceComponent", "void cancelPlacement()", "Cancels the current placement mode programmatically.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectPlaceComponent", "void reactOnGameObjectPlaced(func closureFunction(string newId))",
            "Registers a Lua callback fired after a game object is successfully placed. Receives the new game object id.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectPlaceComponent", "void reactOnPlacementCancelled(func closureFunction())", "Registers a Lua callback fired when placement is cancelled.");

        gameObjectClass.def("getGameObjectPlaceComponent", &getGameObjectPlaceComponent);
        gameObjectClass.def("getGameObjectPlaceComponentFromName", &getGameObjectPlaceComponentFromName);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "GameObjectPlaceComponent getGameObjectPlaceComponent()", "Gets the GameObjectPlaceComponent.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "GameObjectPlaceComponent getGameObjectPlaceComponentFromName(string name)", "Gets the GameObjectPlaceComponent by name.");

        gameObjectControllerClass.def("castGameObjectPlaceComponent", &GameObjectController::cast<GameObjectPlaceComponent>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "GameObjectPlaceComponent castGameObjectPlaceComponent(GameObjectPlaceComponent other)", "Casts for Lua auto completion.");
    }

    bool GameObjectPlaceComponent::canStaticAddComponent(GameObject* gameObject)
    {
        // Only one instance allowed, only on the main game object
        auto existingComp = NOWA::makeStrongPtr(gameObject->getComponent<GameObjectPlaceComponent>());
        return (gameObject->getId() == GameObjectController::MAIN_GAMEOBJECT_ID && nullptr == existingComp);
    }

} // namespace end