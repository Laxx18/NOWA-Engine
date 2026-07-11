#include "NOWAPrecompiled.h"
#include "AreaOfInterestComponent.h"
#include "gameobject/GameObjectFactory.h"
#include "gameobject/LuaScriptComponent.h"
#include "main/AppStateManager.h"
#include "main/EventManager.h"
#include "modules/LuaScriptApi.h"
#include "plugins/ActivationComponent/code/ActivationComponent.cpp"
#include "utilities/XMLConverter.h"

#include "OgreAbiUtils.h"

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    AreaOfInterestComponent::AreaOfInterestComponent() :
        GameObjectComponent(),
        name("AreaOfInterestComponent"),
        sphereSceneQuery(nullptr),
        triggerSphereQueryObserver(nullptr),
        triggerUpdateTimer(0.0f),
        luaScriptComponent(nullptr),
        activated(new Variant(AreaOfInterestComponent::AttrActivated(), true, this->attributes)),
        areaCount(new Variant(AreaOfInterestComponent::AttrAreaCount(), 1, this->attributes)),
        updateThreshold(new Variant(AreaOfInterestComponent::AttrUpdateThreshold(), 0.5f, this->attributes)),
        shortTimeActivation(new Variant(AreaOfInterestComponent::AttrShortTimeActivation(), false, this->attributes)),
        triggerPermanentely(new Variant(AreaOfInterestComponent::AttrTriggerPermanentely(), true, this->attributes))
    {
        this->shortTimeActivation->setDescription("If set to true, this component will be deactivated shortly after it has been activated.");
        this->updateThreshold->setDescription("Sets how often the areas are checked for game objects. Default is 0.5 seconds.");
        this->triggerPermanentely->setDescription("Sets whether to trigger callbacks permanently or just once per entry/leave loop.");

        this->areaCount->addUserData(GameObject::AttrActionNeedRefresh());

        this->setAreaCount(1);
    }

    AreaOfInterestComponent::~AreaOfInterestComponent(void)
    {
    }

    void AreaOfInterestComponent::initialise()
    {
    }

    const Ogre::String& AreaOfInterestComponent::getName() const
    {
        return this->name;
    }

    void AreaOfInterestComponent::install(const Ogre::NameValuePairList* options)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<AreaOfInterestComponent>(AreaOfInterestComponent::getStaticClassId(), AreaOfInterestComponent::getStaticClassName());
    }

    void AreaOfInterestComponent::shutdown()
    {
    }

    void AreaOfInterestComponent::uninstall()
    {
    }

    void AreaOfInterestComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    bool AreaOfInterestComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
        {
            this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ShortTimeActivation")
        {
            this->shortTimeActivation->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
            propertyElement = propertyElement->next_sibling("property");
        }

        // BACKWARDS COMPATIBILITY: Intercept original single 'Radius' property
        Ogre::Real legacyRadius = 10.0f;
        bool hasLegacyRadius = false;
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Radius")
        {
            legacyRadius = XMLConverter::getAttribReal(propertyElement, "data");
            hasLegacyRadius = true;
            propertyElement = propertyElement->next_sibling("property");
        }

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UpdateThreshold")
        {
            this->updateThreshold->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        // BACKWARDS COMPATIBILITY: Intercept original single 'Categories' property
        Ogre::String legacyCategories = "All";
        bool hasLegacyCategories = false;
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Categories")
        {
            legacyCategories = XMLConverter::getAttrib(propertyElement, "data");
            hasLegacyCategories = true;
            propertyElement = propertyElement->next_sibling("property");
        }

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TriggerPermanentely")
        {
            this->triggerPermanentely->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        // Determine targeted Layout count (Default to 1 area zone for standard legacy fallback compatibility)
        unsigned int count = 1;
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AreaCount")
        {
            count = XMLConverter::getAttribUnsignedInt(propertyElement, "data", 1);
            this->areaCount->setValue(count);
            propertyElement = propertyElement->next_sibling("property");
        }
        else
        {
            this->areaCount->setValue(1);
        }

        // Ensure dynamic variant vectors align perfectly with parsed layout tracking configurations
        if (this->radii.size() < count)
        {
            this->radii.resize(count);
            this->categoriesVariants.resize(count);
            this->categoriesId.resize(count, GameObjectController::ALL_CATEGORIES_ID);
            this->tagNamesVariants.resize(count);
        }

        // BACKWARDS COMPATIBILITY FIX: Commit single-area legacy configurations in-place on slot 0
        if (hasLegacyRadius || hasLegacyCategories)
        {
            if (this->radii[0])
            {
                this->radii[0]->setValue(legacyRadius);
            }
            if (this->categoriesVariants[0])
            {
                this->setCategories(0, legacyCategories);
            }
            if (this->tagNamesVariants[0])
            {
                this->tagNamesVariants[0]->setValue(Ogre::String("")); // Default to empty string
            }
        }

        // Read array collections sequentially (Safely completely skipped on original legacy assets)
        for (unsigned int i = 0; i < count; i++)
        {
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Radius" + Ogre::StringConverter::toString(i))
            {
                if (nullptr == this->radii[i])
                {
                    this->radii[i] = new Variant(AreaOfInterestComponent::AttrRadius() + Ogre::StringConverter::toString(i), 10.0f, this->attributes);
                }
                this->radii[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
                propertyElement = propertyElement->next_sibling("property");
            }

            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Categories" + Ogre::StringConverter::toString(i))
            {
                if (nullptr == this->categoriesVariants[i])
                {
                    this->categoriesVariants[i] = new Variant(AreaOfInterestComponent::AttrCategories() + Ogre::StringConverter::toString(i), Ogre::String("All"), this->attributes);
                }
                // Must call setCategories() not setValue() directly -- setCategories()
                // calls generateCategoryId() to populate categoriesId[i]. Direct setValue() only
                // updates the Variant string but leaves categoriesId[i] as ALL_CATEGORIES_ID,
                // causing all category filters to be ignored.
                this->setCategories(i, XMLConverter::getAttrib(propertyElement, "data"));
                propertyElement = propertyElement->next_sibling("property");
            }

            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TagName" + Ogre::StringConverter::toString(i))
            {
                if (nullptr == this->tagNamesVariants[i])
                {
                    this->tagNamesVariants[i] = new Variant(AreaOfInterestComponent::AttrTagNames() + Ogre::StringConverter::toString(i), Ogre::String(""), this->attributes);
                }
                this->tagNamesVariants[i]->setValue(XMLConverter::getAttrib(propertyElement, "data"));
                this->tagNamesVariants[i]->addUserData(GameObject::AttrActionSeparator());
                propertyElement = propertyElement->next_sibling("property");
            }
        }

        return true;
    }

    GameObjectCompPtr AreaOfInterestComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        AreaOfInterestCompPtr clonedCompPtr(boost::make_shared<AreaOfInterestComponent>());

        clonedCompPtr->setShortTimeActivation(this->shortTimeActivation->getBool());
        clonedCompPtr->setUpdateThreshold(this->updateThreshold->getReal());
        clonedCompPtr->setTriggerPermanentely(this->triggerPermanentely->getBool());
        clonedCompPtr->setAreaCount(this->getAreaCount());

        for (unsigned int i = 0; i < this->getAreaCount(); i++)
        {
            clonedCompPtr->setRadius(i, this->getRadius(i));
            clonedCompPtr->setCategories(i, this->getCategories(i));
            clonedCompPtr->setTagName(i, this->getTagName(i));
        }

        clonedGameObjectPtr->addComponent(clonedCompPtr);
        clonedCompPtr->setOwner(clonedGameObjectPtr);
        clonedCompPtr->setActivated(this->activated->getBool());

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
        return clonedCompPtr;
    }

    bool AreaOfInterestComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AreaOfInterestComponent] Init single-query optimized multi-area component for game object: " + this->gameObjectPtr->getName());

        AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &AreaOfInterestComponent::handleGameObjectDeleted), EventDataDeleteGameObject::getStaticEventType());

        // Instantiated with a dummy radius. Configured dynamically on update ticks based on active rules.
        this->sphereSceneQuery = this->gameObjectPtr->getSceneManager()->createSphereQuery(Ogre::Sphere(this->gameObjectPtr->getPosition(), 1.0f));

        return true;
    }

    bool AreaOfInterestComponent::connect(void)
    {
        this->setActivated(this->activated->getBool());

        for (unsigned int i = 0; i < this->getAreaCount(); i++)
        {
            this->setCategories(i, this->categoriesVariants[i]->getString());
        }

        return true;
    }

    bool AreaOfInterestComponent::disconnect(void)
    {
        this->triggerUpdateTimer = 0.0f;

        Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
        NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

        return true;
    }

    bool AreaOfInterestComponent::onCloned(void)
    {
        return true;
    }

    void AreaOfInterestComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();
        if (nullptr != this->sphereSceneQuery)
        {
            this->gameObjectPtr->getSceneManager()->destroyQuery(this->sphereSceneQuery);
            this->sphereSceneQuery = nullptr;
        }

        if (nullptr != this->triggerSphereQueryObserver)
        {
            delete this->triggerSphereQueryObserver;
            this->triggerSphereQueryObserver = nullptr;
        }
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &AreaOfInterestComponent::handleGameObjectDeleted), EventDataDeleteGameObject::getStaticEventType());
    }

    void AreaOfInterestComponent::update(Ogre::Real dt, bool notSimulating)
    {
        if (false == notSimulating && true == this->activated->getBool())
        {
            auto closureFunction = [this, dt](Ogre::Real renderDt)
            {
                this->checkAreaForActiveObjects(dt);
            };
            Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
            NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
        }
    }

    void AreaOfInterestComponent::handleGameObjectDeleted(EventDataPtr eventData)
    {
        if (AppStateManager::getSingletonPtr()->getGameObjectController()->getIsDestroying())
        {
            return;
        }

        boost::shared_ptr<EventDataDeleteGameObject> castEventData = boost::static_pointer_cast<NOWA::EventDataDeleteGameObject>(eventData);
        unsigned long id = castEventData->getGameObjectId();
        for (auto it = this->triggeredGameObjects.cbegin(); it != this->triggeredGameObjects.cend();)
        {
            if (it->second.first->getId() == id)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AreaOfInterestComponent] Removing gameobject: " + it->second.first->getName() + " from life time queue");
                it = this->triggeredGameObjects.erase(it);
                break;
            }
            else
            {
                ++it;
            }
        }
    }

    void AreaOfInterestComponent::attachTriggerObserver(AreaOfInterestComponent::ITriggerSphereQueryObserver* triggerSphereQueryObserver)
    {
        this->triggerSphereQueryObserver = triggerSphereQueryObserver;
    }

    void AreaOfInterestComponent::checkAreaForActiveObjects(Ogre::Real dt)
    {
        if (this->updateThreshold->getReal() > 0.0f)
        {
            this->triggerUpdateTimer += dt;
        }

        if (this->triggerUpdateTimer >= this->updateThreshold->getReal())
        {
            this->triggerUpdateTimer = 0.0f;
            unsigned int count = this->getAreaCount();
            if (count == 0 || !this->sphereSceneQuery)
            {
                return;
            }

            // Step 1: Compute absolute max radius and the globally combined broad-phase bitmask
            Ogre::Real maxRadius = 0.0f;
            unsigned int combinedMask = 0;
            for (unsigned int i = 0; i < count; i++)
            {
                Ogre::Real r = this->getRadius(i);
                if (r > maxRadius)
                {
                    maxRadius = r;
                }
                combinedMask |= this->categoriesId[i];
            }

            // Configure the scene query bounds using the largest possible outer boundary.
            // Add 10% hysteresis so objects near the edge do not oscillate in/out of the
            // query result due to render-thread position interpolation jitter.
            // The exact per-layer radius check (distance <= getRadius(i)) still enforces
            // the configured trigger boundary precisely.
            Ogre::Sphere updateSphere(this->gameObjectPtr->getPosition(), maxRadius * 1.1f);
            this->sphereSceneQuery->setSphere(updateSphere);
            if (combinedMask > 0)
            {
                this->sphereSceneQuery->setQueryMask(combinedMask);
            }

            // Track unique object ID per layer area layer index to correctly separate triggers
            // Key: std::pair<unsigned long (GameObjectId), unsigned int (LayerIndex)>
            std::set<std::pair<unsigned long, unsigned int>> currentObjectsInLayers;
            Ogre::Vector3 myPos = this->gameObjectPtr->getPosition();

            // Step 2: Broad Phase Execution via Ogre
            Ogre::SceneQueryResultMovableList& result = this->sphereSceneQuery->execute().movables;
            for (auto it = result.cbegin(); it != result.cend(); ++it)
            {
                Ogre::MovableObject* movableObject = *it;
                GameObject* gameObject = nullptr;
                if ("Camera" != movableObject->getMovableType())
                {
                    const Ogre::Any& userAny = movableObject->getUserObjectBindings().getUserAny();
                    if (!userAny.isEmpty())
                    {
                        try
                        {
                            gameObject = Ogre::any_cast<GameObject*>(userAny);
                        }
                        catch (Ogre::Exception&)
                        {
                        }
                    }
                }

                if (nullptr != gameObject && gameObject != this->gameObjectPtr.get())
                {
                    Ogre::Real distance = myPos.distance(gameObject->getPosition());
                    unsigned int objQueryFlags = movableObject->getQueryFlags();
                    Ogre::String objTagName = gameObject->getTagName();

                    // Step 3: Evaluate each layout layer 100% independently
                    for (unsigned int i = 0; i < count; i++)
                    {
                        // Rule A: Does it match this specific layer's category bitmask?
                        if ((objQueryFlags & this->categoriesId[i]) != 0)
                        {
                            // Rule B: Does it fall within this specific layer's physical radius restriction?
                            if (distance <= this->getRadius(i))
                            {
                                // Rule C: Optional TagName checks (treat as completely optional if empty, "Any", or "All")
                                Ogre::String configuredTag = this->getTagName(i);
                                bool tagMatches = false;

                                if (configuredTag.empty() || configuredTag == "Any" || configuredTag == "All")
                                {
                                    tagMatches = true;
                                }
                                else if (!objTagName.empty())
                                {
                                    // Support multi-tag filtering (checks if objTagName is found within "relict1, book")
                                    if (configuredTag.find(objTagName) != Ogre::String::npos)
                                    {
                                        tagMatches = true;
                                    }
                                }

                                if (tagMatches)
                                {
                                    // Use a unique tracking hash per object + layer index combined!
                                    uint64_t compoundKey = (static_cast<uint64_t>(gameObject->getId()) << 8) | (static_cast<uint64_t>(i) & 0xFFu);
                                    currentObjectsInLayers.insert(std::make_pair(gameObject->getId(), i));

                                    auto otherIt = this->triggeredGameObjects.find(compoundKey);
                                    if (otherIt == this->triggeredGameObjects.end())
                                    {
                                        // Fire enter event for this specific layer
                                        this->callEnterFunction(gameObject);
                                        this->triggeredGameObjects.emplace(compoundKey, std::make_pair(gameObject, true));
                                    }
                                    else if (!otherIt->second.second)
                                    {
                                        // Re-entered this layer
                                        this->callEnterFunction(gameObject);
                                        otherIt->second.second = true;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Step 4: Handle independent Leave Events per layer
            for (auto it = this->triggeredGameObjects.begin(); it != this->triggeredGameObjects.end();)
            {
                uint64_t compoundKey = it->first;
                GameObject* gameObject = it->second.first;

                // Extract the layer index from the compound key.
                // Must use uint64_t -- unsigned long is 32-bit on Windows and truncates
                // the key, causing searchPair never to match and leave firing every frame.
                unsigned int layerIdx = static_cast<unsigned int>(compoundKey & 0xFFu);

                auto searchPair = std::make_pair(gameObject->getId(), layerIdx);

                if (currentObjectsInLayers.find(searchPair) == currentObjectsInLayers.end())
                {
                    if (it->second.second)
                    {
                        this->callLeaveFunction(gameObject);
                        it->second.second = false;
                    }

                    if (!this->triggerPermanentely->getBool())
                    {
                        it = this->triggeredGameObjects.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }
                else
                {
                    ++it;
                }
            }

            if (this->shortTimeActivation->getBool())
            {
                this->setActivated(false);
            }
        }
    }

    void AreaOfInterestComponent::logLuaError(const Ogre::String& context, const luabind::error& error)
    {
        luabind::object errorMsg(luabind::from_stack(error.state(), -1));
        std::stringstream msg;
        msg << errorMsg;

        Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in '" + context + "' Error: " + Ogre::String(error.what()) + " details: " + msg.str());
    }

    void AreaOfInterestComponent::callEnterFunction(GameObject* gameObject)
    {
        if (this->triggerSphereQueryObserver)
        {
            this->triggerSphereQueryObserver->onEnter(gameObject);
        }

        if (this->gameObjectPtr->getLuaScript() && this->enterClosureFunction.is_valid())
        {
            NOWA::AppStateManager::LogicCommand logicCommand = [this, gameObject]()
            {
                try
                {
                    luabind::call_function<void>(this->enterClosureFunction, gameObject);
                }
                catch (luabind::error& error)
                {
                    this->logLuaError("reactOnEnter", error);
                }
            };
            NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
        }
    }

    void AreaOfInterestComponent::callLeaveFunction(GameObject* gameObject)
    {
        if (this->triggerSphereQueryObserver)
        {
            this->triggerSphereQueryObserver->onLeave(gameObject);
        }

        if (this->gameObjectPtr->getLuaScript() && this->leaveClosureFunction.is_valid())
        {
            NOWA::AppStateManager::LogicCommand logicCommand = [this, gameObject]()
            {
                try
                {
                    luabind::call_function<void>(this->leaveClosureFunction, gameObject);
                }
                catch (luabind::error& error)
                {
                    this->logLuaError("reactOnLeave", error);
                }
            };
            NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
        }
    }

    void AreaOfInterestComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (AreaOfInterestComponent::AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
        else if (AreaOfInterestComponent::AttrShortTimeActivation() == attribute->getName())
        {
            this->setShortTimeActivation(attribute->getBool());
        }
        else if (AreaOfInterestComponent::AttrUpdateThreshold() == attribute->getName())
        {
            this->setUpdateThreshold(attribute->getReal());
        }
        else if (AreaOfInterestComponent::AttrTriggerPermanentely() == attribute->getName())
        {
            this->setTriggerPermanentely(attribute->getBool());
        }
        else if (AreaOfInterestComponent::AttrAreaCount() == attribute->getName())
        {
            this->setAreaCount(attribute->getUInt());
        }
        else
        {
            for (unsigned int i = 0; i < static_cast<unsigned int>(this->radii.size()); i++)
            {
                if (this->radii[i] && AreaOfInterestComponent::AttrRadius() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setRadius(i, attribute->getReal());
                    break;
                }
                if (this->categoriesVariants[i] && AreaOfInterestComponent::AttrCategories() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setCategories(i, attribute->getString());
                    break;
                }
                else if (AreaOfInterestComponent::AttrTagNames() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setTagName(i, attribute->getString());
                    break;
                }
            }
        }
    }

    void AreaOfInterestComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        GameObjectComponent::writeXML(propertiesXML, doc);

        xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "ShortTimeActivation"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->shortTimeActivation->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "UpdateThreshold"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->updateThreshold->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "TriggerPermanentely"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->triggerPermanentely->getBool())));
        propertiesXML->append_node(propertyXML);

        // Modern Layout serialization tracking AreaCount
        unsigned int count = this->radii.size();
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2")); // 2 = int/uint
        propertyXML->append_attribute(doc.allocate_attribute("name", "AreaCount"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, count)));
        propertiesXML->append_node(propertyXML);

        // Loop and save indexed items
        for (unsigned int i = 0; i < count; i++)
        {
            if (this->radii[i])
            {
                propertyXML = doc.allocate_node(node_element, "property");
                propertyXML->append_attribute(doc.allocate_attribute("type", "6")); // 6 = real
                propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string((AreaOfInterestComponent::AttrRadius() + Ogre::StringConverter::toString(i)).c_str())));
                propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->radii[i]->getReal())));
                propertiesXML->append_node(propertyXML);
            }

            if (this->categoriesVariants[i])
            {
                propertyXML = doc.allocate_node(node_element, "property");
                propertyXML->append_attribute(doc.allocate_attribute("type", "7")); // 7 = string
                propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string((AreaOfInterestComponent::AttrCategories() + Ogre::StringConverter::toString(i)).c_str())));
                propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->categoriesVariants[i]->getString())));
                propertiesXML->append_node(propertyXML);
            }

            if (this->tagNamesVariants[i])
            {
                propertyXML = doc.allocate_node(node_element, "property");
                propertyXML->append_attribute(doc.allocate_attribute("type", "7")); // 7 = string
                propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string((AreaOfInterestComponent::AttrTagNames() + Ogre::StringConverter::toString(i)).c_str())));
                propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->tagNamesVariants[i]->getString())));
                propertiesXML->append_node(propertyXML);
            }
        }
    }

    Ogre::String AreaOfInterestComponent::getClassName(void) const
    {
        return "AreaOfInterestComponent";
    }

    Ogre::String AreaOfInterestComponent::getParentClassName(void) const
    {
        return "GameObjectComponent";
    }

    void AreaOfInterestComponent::setActivated(bool activated)
    {
        if (AppStateManager::getSingletonPtr()->getGameObjectController()->getIsDestroying())
        {
            this->triggerUpdateTimer = 0.0f;
            this->activated->setValue(activated);

            if (false == activated)
            {
                for (auto it = this->triggeredGameObjects.cbegin(); it != this->triggeredGameObjects.cend();)
                {
                    if (nullptr != this->triggerSphereQueryObserver)
                    {
                        this->triggerSphereQueryObserver->onLeave(it->second.first);
                    }
                    it = this->triggeredGameObjects.erase(it);
                }
            }
            return;
        }

        NOWA::AppStateManager::LogicCommand logicCommand = [this, activated]()
        {
            this->triggerUpdateTimer = 0.0f;

            auto luaScriptCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<LuaScriptComponent>());
            if (nullptr != luaScriptCompPtr)
            {
                this->luaScriptComponent = luaScriptCompPtr.get();
            }

            if (true == activated)
            {
                if (nullptr != luaScriptComponent && false == luaScriptComponent->isActivated())
                {
                    boost::shared_ptr<EventDataPrintLuaError> eventDataPrintLuaError(new EventDataPrintLuaError(this->gameObjectPtr->getLuaScript()->getScriptName(), this->gameObjectPtr->getLuaScript()->getScriptFilePathName(), 0,
                        "Cannot activate component, because the 'LuaScriptComponent' is not activated for game object: " + this->gameObjectPtr->getName()));
                    AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataPrintLuaError);
                    return;
                }
            }

            this->activated->setValue(activated);

            if (false == activated)
            {
                for (auto it = this->triggeredGameObjects.cbegin(); it != this->triggeredGameObjects.cend();)
                {
                    GameObject* gameObject = it->second.first;

                    auto activationComponent = NOWA::makeStrongPtr(gameObject->getComponent<ActivationComponent>());
                    if (nullptr != activationComponent)
                    {
                        activationComponent->setActivated(false);
                    }

                    if (nullptr != this->triggerSphereQueryObserver)
                    {
                        this->triggerSphereQueryObserver->onLeave(gameObject);
                    }

                    if (nullptr != this->gameObjectPtr->getLuaScript())
                    {
                        if (this->leaveClosureFunction.is_valid())
                        {
                            try
                            {
                                luabind::call_function<void>(this->leaveClosureFunction, gameObject);
                            }
                            catch (luabind::error& error)
                            {
                                this->logLuaError("reactOnLeaveFallback", error);
                            }
                        }
                    }
                    it = this->triggeredGameObjects.erase(it);
                }
            }
        };
        NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
    }

    void AreaOfInterestComponent::setAreaCount(unsigned int count)
    {
        this->areaCount->setValue(count);
        unsigned int oldSize = this->radii.size();

        if (count > oldSize)
        {
            this->radii.resize(count);
            this->categoriesVariants.resize(count);
            this->categoriesId.resize(count, GameObjectController::ALL_CATEGORIES_ID);
            this->tagNamesVariants.resize(count); // <-- Resize tag names array

            for (unsigned int i = oldSize; i < count; i++)
            {
                this->radii[i] = new Variant(AreaOfInterestComponent::AttrRadius() + Ogre::StringConverter::toString(i), 10.0f, this->attributes);
                this->radii[i]->addUserData(GameObject::AttrActionNeedRefresh());

                this->categoriesVariants[i] = new Variant(AreaOfInterestComponent::AttrCategories() + Ogre::StringConverter::toString(i), Ogre::String("All"), this->attributes);

                // Allocate empty default TagName per area
                this->tagNamesVariants[i] = new Variant(AreaOfInterestComponent::AttrTagNames() + Ogre::StringConverter::toString(i), Ogre::String(""), this->attributes);
                this->tagNamesVariants[i]->addUserData(GameObject::AttrActionSeparator());
            }
        }
        else if (count < oldSize)
        {
            this->eraseVariants(this->radii, count);
            this->eraseVariants(this->categoriesVariants, count);
            this->eraseVariants(this->tagNamesVariants, count);
            this->categoriesId.resize(count);
        }
    }

    unsigned int AreaOfInterestComponent::getAreaCount(void) const
    {
        return this->areaCount->getUInt();
    }

    void AreaOfInterestComponent::setRadius(unsigned int index, Ogre::Real radius)
    {
        if (index >= this->radii.size())
        {
            return;
        }
        if (radius < 0.1f)
        {
            radius = 0.1f;
        }

        this->radii[index]->setValue(radius);
    }

    Ogre::Real AreaOfInterestComponent::getRadius(unsigned int index) const
    {
        if (index >= this->radii.size() || !this->radii[index])
        {
            return 10.0f;
        }
        return this->radii[index]->getReal();
    }

    bool AreaOfInterestComponent::isActivated(void) const
    {
        return this->activated->getBool();
    }

    void AreaOfInterestComponent::setCategories(unsigned int index, const Ogre::String& categories)
    {
        if (index >= this->categoriesVariants.size())
        {
            return;
        }

        this->categoriesVariants[index]->setValue(categories);
        this->categoriesId[index] = AppStateManager::getSingletonPtr()->getGameObjectController()->generateCategoryId(categories);
    }

    Ogre::String AreaOfInterestComponent::getCategories(unsigned int index) const
    {
        if (index >= this->categoriesVariants.size() || !this->categoriesVariants[index])
        {
            return "All";
        }
        return this->categoriesVariants[index]->getString();
    }

    void AreaOfInterestComponent::setTagName(unsigned int index, const Ogre::String& tagName)
    {
        if (index < this->tagNamesVariants.size() && this->tagNamesVariants[index] != nullptr)
        {
            this->tagNamesVariants[index]->setValue(tagName);
        }
    }

    Ogre::String AreaOfInterestComponent::getTagName(unsigned int index) const
    {
        if (index < this->tagNamesVariants.size() && this->tagNamesVariants[index] != nullptr)
        {
            return this->tagNamesVariants[index]->getString();
        }
        return "";
    }

    void AreaOfInterestComponent::setUpdateThreshold(Ogre::Real updateThreshold)
    {
        if (updateThreshold < 0.0f)
        {
            updateThreshold = 0.0f;
        }
        this->updateThreshold->setValue(updateThreshold);
    }

    Ogre::Real AreaOfInterestComponent::getUpdateThreshold(void) const
    {
        return this->updateThreshold->getReal();
    }

    void AreaOfInterestComponent::reactOnEnter(luabind::object closureFunction)
    {
        this->enterClosureFunction = closureFunction;
    }

    void AreaOfInterestComponent::reactOnLeave(luabind::object closureFunction)
    {
        this->leaveClosureFunction = closureFunction;
    }

    void AreaOfInterestComponent::setShortTimeActivation(bool shortTimeActivation)
    {
        this->shortTimeActivation->setValue(shortTimeActivation);
    }

    bool AreaOfInterestComponent::getShortTimeActivation(void) const
    {
        return this->shortTimeActivation->getBool();
    }

    void AreaOfInterestComponent::setTriggerPermanentely(bool triggerPermanentely)
    {
        this->triggerPermanentely->setValue(triggerPermanentely);
    }

    bool AreaOfInterestComponent::getTriggerPermanentely(void) const
    {
        return this->triggerPermanentely->getBool();
    }

    // Lua registration part

    AreaOfInterestComponent* getAreaOfInterestComponentFromIndex(GameObject* gameObject, unsigned int occurrenceIndex)
    {
        return makeStrongPtr<AreaOfInterestComponent>(gameObject->getComponentWithOccurrence<AreaOfInterestComponent>(occurrenceIndex)).get();
    }

    AreaOfInterestComponent* getAreaOfInterestComponent(GameObject* gameObject)
    {
        return makeStrongPtr<AreaOfInterestComponent>(gameObject->getComponent<AreaOfInterestComponent>()).get();
    }

    AreaOfInterestComponent* getAreaOfInterestComponentFromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<AreaOfInterestComponent>(gameObject->getComponentFromName<AreaOfInterestComponent>(name)).get();
    }

    void AreaOfInterestComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectController)
    {
        module(lua)[class_<AreaOfInterestComponent, GameObjectComponent>("AreaOfInterestComponent")
                .def("setActivated", &AreaOfInterestComponent::setActivated)
                .def("isActivated", &AreaOfInterestComponent::isActivated)
                .def("setAreaCount", &AreaOfInterestComponent::setAreaCount)
                .def("getAreaCount", &AreaOfInterestComponent::getAreaCount)
                .def("setRadius", &AreaOfInterestComponent::setRadius)
                .def("getRadius", &AreaOfInterestComponent::getRadius)
                .def("setCategories", &AreaOfInterestComponent::setCategories)
                .def("getCategories", &AreaOfInterestComponent::getCategories)
                .def("setTagName", &AreaOfInterestComponent::setTagName)
                .def("getTagName", &AreaOfInterestComponent::getTagName)
                .def("reactOnEnter", &AreaOfInterestComponent::reactOnEnter)
                .def("reactOnLeave", &AreaOfInterestComponent::reactOnLeave)];

        LuaScriptApi::getInstance()->addClassToCollection("AreaOfInterestComponent", "class inherits GameObjectComponent", AreaOfInterestComponent::getStaticInfoText());
        LuaScriptApi::getInstance()->addClassToCollection("AreaOfInterestComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
        LuaScriptApi::getInstance()->addClassToCollection("AreaOfInterestComponent", "bool isActivated()", "Gets whether this component is activated.");
        LuaScriptApi::getInstance()->addClassToCollection("AreaOfInterestComponent", "void setAreaCount(unsigned int count)", "Sets the total amount of sub-areas.");
        LuaScriptApi::getInstance()->addClassToCollection("AreaOfInterestComponent", "unsigned int getAreaCount()", "Gets the total amount of registered fields.");
        LuaScriptApi::getInstance()->addClassToCollection("AreaOfInterestComponent", "void setRadius(unsigned int index, float radius)", "Sets the area check radius by layout layer index.");
        LuaScriptApi::getInstance()->addClassToCollection("AreaOfInterestComponent", "float getRadius(unsigned int index)", "Gets the target area check radius by layer index.");
        LuaScriptApi::getInstance()->addClassToCollection("AreaOfInterestComponent", "void setCategories(unsigned int index, String categories)", "Sets filter classification keywords by layout layer index.");
        LuaScriptApi::getInstance()->addClassToCollection("AreaOfInterestComponent", "String getCategories(unsigned int index)", "Gets defined criteria string bounds by index.");
        LuaScriptApi::getInstance()->addClassToCollection("AreaOfInterestComponent", "void setTagName(unsigned int index, String tagName)", "Sets the specific string subcategory filter for an indexed check area.");
        LuaScriptApi::getInstance()->addClassToCollection("AreaOfInterestComponent", "String getTagName(unsigned int index)", "Gets the specific string subcategory filter assigned on an indexed check area.");
        LuaScriptApi::getInstance()->addClassToCollection("AreaOfInterestComponent", "void reactOnEnter(func closure, otherGameObject)", "Sets callback hooks when tracking requirements match.");
        LuaScriptApi::getInstance()->addClassToCollection("AreaOfInterestComponent", "void reactOnLeave(func closure, otherGameObject)", "Sets callback hooks when tracking states collapse via boundary exit checks.");

        gameObjectClass.def("getAreaOfInterestComponent", (AreaOfInterestComponent * (*)(GameObject*)) & getAreaOfInterestComponent);
        gameObjectClass.def("getAreaOfInterestComponentFromIndex", (AreaOfInterestComponent * (*)(GameObject*, unsigned int)) & getAreaOfInterestComponentFromIndex);
        gameObjectClass.def("getAreaOfInterestComponentFromName", &getAreaOfInterestComponentFromName);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "AreaOfInterestComponent getAreaOfInterestComponentFromIndex(unsigned int occurrenceIndex)",
            "Gets the area of interest component by the given occurence index, since a game object may have besides other components several area of interest components.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "AreaOfInterestComponent getAreaOfInterestComponent()", "Gets the area of interest component. This can be used if the game object just has one area of interest component.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "AreaOfInterestComponent getAreaOfInterestComponentFromName(String name)", "Gets the area of interest component.");

        gameObjectController.def("castAreaOfInterestComponent", &GameObjectController::cast<AreaOfInterestComponent>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "AreaOfInterestComponent castAreaOfInterestComponent(AreaOfInterestComponent other)", "Casts an incoming type from function for lua auto completion.");
    }

    bool AreaOfInterestComponent::canStaticAddComponent(GameObject* gameObject)
    {
        return true;
    }

}; // namespace end