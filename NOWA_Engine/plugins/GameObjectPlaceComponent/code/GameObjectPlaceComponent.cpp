/*
Copyright (c) 2025 Lukas Kalinowski
GPL v3
*/

#include "NOWAPrecompiled.h"
#include "GameObjectPlaceComponent.h"
#include "OgreAbiUtils.h"
#include "gameobject/GameObjectFactory.h"
#include "gameobject/PhysicsComponent.h"
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
        categories(new Variant(GameObjectPlaceComponent::AttrCategories(), Ogre::String("All"), this->attributes)),
        showPreview(new Variant(GameObjectPlaceComponent::AttrShowPreview(), true, this->attributes)),
        rotateEnabled(new Variant(GameObjectPlaceComponent::AttrRotateEnabled(), false, this->attributes)),
        placeObjectCount(new Variant(GameObjectPlaceComponent::AttrPlaceObjectCount(), 0, this->attributes)),
        activeGameObjectId(0),
        isPlacing(false),
        currentHitPoint(Ogre::Vector3::ZERO),
        raySceneQuery(nullptr),
        categoryId(0),
        shadowPhysicsComponent(nullptr),
        oldWasDynamic(false),
        currentRotationDegrees(0.0f)
    {
        this->placeObjectCount->addUserData(GameObject::AttrActionNeedRefresh());
        this->categories->setDescription("Categories to place objects on. Use 'All' for everything, "
                                         "or combine with '+' to include and '-' to exclude. E.g. 'All-Building-Agent' places "
                                         "on everything except Building and Agent categories.");
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
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Categories")
        {
            this->categories->setValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ShowPreview")
        {
            this->showPreview->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RotateEnabled")
        {
            this->rotateEnabled->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
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

        this->raySceneQuery = this->gameObjectPtr->getSceneManager()->createRayQuery(Ogre::Ray(), NOWA::GameObjectController::ALL_CATEGORIES_ID);
        this->raySceneQuery->setSortByDistance(true);

        return true;
    }

    bool GameObjectPlaceComponent::connect(void)
    {
        GameObjectComponent::connect();

        // Apply categories to the ray query now that all game objects are registered
        this->setCategories(this->categories->getString());

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

        if (this->isPlacing)
        {
            this->endPlacement(true);
        }

        // endPlacement already clears shadowPhysicsComponent,
        // but guard here in case isPlacing was false
        this->shadowPhysicsComponent = nullptr;

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

        this->shadowPhysicsComponent = nullptr;

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
        // If the physics component of the currently active shadow object was removed,
        // clear our cached pointer so mouseMoved falls back to node-based movement
        if (nullptr != this->shadowPhysicsComponent)
        {
            auto shadowGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->activeGameObjectId);
            if (nullptr != shadowGameObjectPtr)
            {
                auto currentPhysics = NOWA::makeStrongPtr(shadowGameObjectPtr->getComponent<PhysicsComponent>()).get();
                if (nullptr == currentPhysics)
                {
                    this->shadowPhysicsComponent = nullptr;
                }
            }
        }
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
        else if (AttrCategories() == attribute->getName())
        {
            this->setCategories(attribute->getString());
        }
        else if (AttrShowPreview() == attribute->getName())
        {
            this->setShowPreview(attribute->getBool());
        }
        else if (AttrRotateEnabled() == attribute->getName())
        {
            this->setRotateEnabled(attribute->getBool());
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

        // Categories before PlaceObjectCount — must match init() read order
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Categories"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->categories->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "ShowPreview"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->showPreview->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "RotateEnabled"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rotateEnabled->getBool())));
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

    void GameObjectPlaceComponent::setCategories(const Ogre::String& categories)
    {
        this->categories->setValue(categories);
        this->categoryId = AppStateManager::getSingletonPtr()->getGameObjectController()->generateCategoryId(categories);

        if (nullptr != this->raySceneQuery)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
            {
                this->raySceneQuery->setQueryMask(this->categoryId);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "GameObjectPlaceComponent::setCategories");
        }
    }

    Ogre::String GameObjectPlaceComponent::getCategories(void) const
    {
        return this->categories->getString();
    }

    void GameObjectPlaceComponent::setShowPreview(bool showPreview)
    {
        this->showPreview->setValue(showPreview);
    }

    bool GameObjectPlaceComponent::getShowPreview(void) const
    {
        return this->showPreview->getBool();
    }

    void GameObjectPlaceComponent::setRotateEnabled(bool rotateEnabled)
    {
        this->rotateEnabled->setValue(rotateEnabled);
    }

    bool GameObjectPlaceComponent::getRotateEnabled(void) const
    {
        return this->rotateEnabled->getBool();
    }

    void GameObjectPlaceComponent::applyPreviewTransparency(GameObjectPtr shadowGameObjectPtr)
    {
        if (false == this->showPreview->getBool())
        {
            return;
        }

        Ogre::Item* item = dynamic_cast<Ogre::Item*>(shadowGameObjectPtr->getMovableObject());
        if (nullptr == item)
        {
            return;
        }

        unsigned long goId = shadowGameObjectPtr->getId();

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, item, goId]()
        {
            this->clonedDatablocks.clear();

            for (unsigned int i = 0; i < item->getNumSubItems(); i++)
            {
                auto* originalDatablock = item->getSubItem(i)->getDatablock();
                if (nullptr == originalDatablock)
                {
                    continue;
                }

                // Clone with a unique name so it's completely independent
                Ogre::String originalName = originalDatablock->getName().getFriendlyText();
                Ogre::HlmsDatablock* cloned = AppStateManager::getSingletonPtr()->getGameObjectController()->cloneDatablockUnique(originalDatablock, originalName, goId, static_cast<int>(i));

                if (nullptr == cloned)
                {
                    continue;
                }

                auto* clonedPbs = dynamic_cast<Ogre::HlmsPbsDatablock*>(cloned);
                if (nullptr == clonedPbs)
                {
                    // Cast failed — destroy immediately to avoid leak
                    cloned->getCreator()->destroyDatablock(cloned->getName());
                    continue;
                }

                // Apply transparency only on the clone — original is untouched
                clonedPbs->setTransparency(0.5f, Ogre::HlmsPbsDatablock::Transparent, false);
                item->getSubItem(i)->setDatablock(clonedPbs);

                this->clonedDatablocks.emplace_back(originalDatablock, static_cast<unsigned int>(i));
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "GameObjectPlaceComponent::applyPreviewTransparency");
    }

    void GameObjectPlaceComponent::resetPreviewTransparency(GameObjectPtr shadowGameObjectPtr)
    {
        if (false == this->showPreview->getBool())
        {
            return;
        }

        Ogre::Item* item = dynamic_cast<Ogre::Item*>(shadowGameObjectPtr->getMovableObject());
        if (nullptr == item)
        {
            return;
        }

        if (this->clonedDatablocks.empty())
        {
            return;
        }

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, item]()
        {
            for (auto& [originalDatablock, subIndex] : this->clonedDatablocks)
            {
                if (subIndex >= item->getNumSubItems())
                {
                    continue;
                }

                // Get the cloned datablock currently assigned to this subitem
                auto* currentDatablock = item->getSubItem(subIndex)->getDatablock();

                // Restore original
                item->getSubItem(subIndex)->setDatablock(originalDatablock);

                // Destroy the clone — but never destroy the original
                if (nullptr != currentDatablock && currentDatablock != originalDatablock)
                {
                    auto* linkedRenderables = &currentDatablock->getLinkedRenderables();
                    if (nullptr != linkedRenderables && linkedRenderables->empty())
                    {
                        currentDatablock->getCreator()->destroyDatablock(currentDatablock->getName());
                    }
                }
            }
            this->clonedDatablocks.clear();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "GameObjectPlaceComponent::resetPreviewTransparency");
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
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameObjectPlaceComponent] activatePlacement: id " + gameObjectId + " is not in the configured GameObjectId list.");
            return;
        }

        auto shadowGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(id);
        if (nullptr == shadowGameObjectPtr)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameObjectPlaceComponent] activatePlacement: game object with id " + gameObjectId + " not found.");
            return;
        }

        this->oldWasDynamic = shadowGameObjectPtr->isDynamic();
        shadowGameObjectPtr->setDynamic(true);

        this->activeGameObjectId = id;
        this->isPlacing = true;
        this->currentRotationDegrees = 0.0f;

        // Resolve physics component for this shadow object — must be done here
        // because different slots may have different shadow objects with/without physics
        this->resolveShadowPhysicsComponent();

        shadowGameObjectPtr->setVisible(true);
        this->applyPreviewTransparency(shadowGameObjectPtr);

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, 
            "[GameObjectPlaceComponent] Placement started for game object: " + shadowGameObjectPtr->getName() + (nullptr != this->shadowPhysicsComponent ? " (physics-driven)" : " (node-driven)"));
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

        auto shadowGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->activeGameObjectId);
        if (nullptr != shadowGameObjectPtr)
        {
            this->resetPreviewTransparency(shadowGameObjectPtr);
            shadowGameObjectPtr->setVisible(false);
            shadowGameObjectPtr->setDynamic(this->oldWasDynamic);
        }

        this->isPlacing = false;
        this->activeGameObjectId = 0;
        this->currentHitPoint = Ogre::Vector3::ZERO;
        this->shadowPhysicsComponent = nullptr;
        this->currentRotationDegrees = 0.0f;

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

        this->raySceneQuery->setRay(mouseRay);

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

    void GameObjectPlaceComponent::resolveShadowPhysicsComponent(void)
    {
        this->shadowPhysicsComponent = nullptr;

        if (0 == this->activeGameObjectId)
        {
            return;
        }

        auto shadowGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->activeGameObjectId);
        if (nullptr == shadowGameObjectPtr)
        {
            return;
        }

        // makeStrongPtr returns a shared_ptr; .get() gives raw pointer.
        // Lifetime is safe: the shadow object exists for the full placement session.
        this->shadowPhysicsComponent = NOWA::makeStrongPtr(shadowGameObjectPtr->getComponent<PhysicsComponent>()).get();
    }

    // -----------------------------------------------------------------------
    // Mouse / Key handlers
    // -----------------------------------------------------------------------
    bool GameObjectPlaceComponent::mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
    {
        if (false == this->bConnected || false == this->isPlacing || false == this->activated->getBool())
        {
            return true;
        }

        // Check for MyGUI focus FIRST, before handling
        if (nullptr != NOWA::GraphicsModule::getInstance()->getMyGUIFocusWidget())
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
            auto clonedGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->clone(originalId, nullptr, 0, placePosition,
                Ogre::Quaternion(Ogre::Degree(this->currentRotationDegrees), Ogre::Vector3::UNIT_Y), Ogre::Vector3(1.0f, 1.0f, 1.0f), true);

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

    bool GameObjectPlaceComponent::mouseMoved(const OIS::MouseEvent& evt)
    {
        if (false == this->bConnected || false == this->isPlacing || false == this->activated->getBool())
        {
            return true;
        }

        if (nullptr != NOWA::GraphicsModule::getInstance()->getMyGUIFocusWidget())
        {
            return true;
        }

        // Handle mousewheel rotation — only when rotate is enabled
        if (true == this->rotateEnabled->getBool() && evt.state.Z.rel != 0)
        {
            // 15 degrees per wheel tick — feels responsive without being too coarse
            this->currentRotationDegrees += (evt.state.Z.rel > 0) ? 15.0f : -15.0f;
            // Normalise to [0, 360)
            this->currentRotationDegrees = std::fmod(this->currentRotationDegrees, 360.0f);
            if (this->currentRotationDegrees < 0.0f)
            {
                this->currentRotationDegrees += 360.0f;
            }
        }

        // If only wheel scroll and no mouse movement, still apply rotation
        Ogre::Vector3 hitPoint = this->getMouseWorldPosition(evt);
        if (hitPoint == Ogre::Vector3::ZERO)
        {
            // No ground hit — still update rotation on the last known position
            hitPoint = this->currentHitPoint;
            if (hitPoint == Ogre::Vector3::ZERO)
            {
                return true;
            }
        }
        else
        {
            this->currentHitPoint = hitPoint;
        }

        auto shadowGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->activeGameObjectId);
        if (nullptr == shadowGameObjectPtr)
        {
            return true;
        }

        shadowGameObjectPtr->setVisible(true);
        Ogre::SceneNode* node = shadowGameObjectPtr->getSceneNode();
        if (nullptr == node)
        {
            return true;
        }

        Ogre::Vector3 offset = Ogre::Vector3::ZERO;
        if (nullptr != shadowGameObjectPtr->getMovableObject())
        {
            offset = MathHelper::getInstance()->getPlacementYOffset(node, shadowGameObjectPtr->getMovableObject());
        }

        Ogre::Vector3 targetPosition = hitPoint /*+ offset*/;

        // Build orientation — always apply accumulated rotation
        Ogre::Quaternion targetOrientation(Ogre::Degree(this->currentRotationDegrees), Ogre::Vector3::UNIT_Y);

        if (nullptr != this->shadowPhysicsComponent)
        {
            this->shadowPhysicsComponent->setPosition(targetPosition);
            this->shadowPhysicsComponent->setOrientation(targetOrientation);
        }
        else
        {
            // fireAndForget=true: rotation is discrete, not interpolated
            NOWA::GraphicsModule::getInstance()->updateNodeTransform(node, targetPosition, targetOrientation, Ogre::Vector3::UNIT_SCALE, false, true);
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
            .def("setCategories", &GameObjectPlaceComponent::setCategories)
            .def("getCategories", &GameObjectPlaceComponent::getCategories)
            .def("setShowPreview", &GameObjectPlaceComponent::setShowPreview)
            .def("getShowPreview", &GameObjectPlaceComponent::getShowPreview)
            .def("setRotateEnabled", &GameObjectPlaceComponent::setRotateEnabled)
            .def("getRotateEnabled", &GameObjectPlaceComponent::getRotateEnabled)
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
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectPlaceComponent", "void setCategories(string categories)",
            "Sets the surface categories the object can be placed on. Use 'All' for everything, "
            "combine with '+' to include and '-' to exclude. E.g. 'All-Building-Agent' allows placement "
            "on all surfaces except those in the Building and Agent categories.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectPlaceComponent", "string getCategories()", "Gets the current placement surface categories filter.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectPlaceComponent", "void setShowPreview(bool showPreview)", "Sets whether the shadow object is shown semi-transparent at the mouse cursor during placement.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectPlaceComponent", "bool getShowPreview()", "Gets whether the placement preview transparency is enabled.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectPlaceComponent", "void setRotateEnabled(bool rotateEnabled)", "If true, the mousewheel rotates the shadow object on its Y axis before placement.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectPlaceComponent", "bool getRotateEnabled()", "Gets whether mousewheel Y-axis rotation is enabled during placement.");
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