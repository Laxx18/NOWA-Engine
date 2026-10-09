#include "NOWAPrecompiled.h"
#include "BlinkComponent.h"
#include "gameobject/GameObjectFactory.h"
#include "gameobject/PhysicsComponent.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "modules/LuaScriptApi.h"
#include "utilities/XMLConverter.h"

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    BlinkComponent::BlinkComponent()
        : GameObjectComponent(),
        name("BlinkComponent"),
        blinkMode(new Variant(BlinkComponent::AttrBlinkMode(), Ogre::String("Single"), this->attributes)),
        gameObjectCount(new Variant(BlinkComponent::AttrGameObjectCount(), static_cast<unsigned int>(0), this->attributes))
    {
        std::vector<Ogre::String> blinkModes = { "Single", "Sequential", "Alternating", "AllAtOnce" };
        this->blinkMode->setValue(blinkModes);
        this->blinkMode->setListSelectedValue("Single");
        this->blinkMode->setDescription(
            "Blink preset mode. "
                                        "'Single': one object cycles on its own. "
                                        "'Sequential': objects fade out one after the other (use StartDelay to stagger). "
                                        "'Alternating': even-index objects and odd-index objects alternate. "
                                        "'AllAtOnce': all objects blink in perfect sync.");


        this->gameObjectCount->addUserData(GameObject::AttrActionNeedRefresh());
        this->gameObjectCount->setDescription(
            "Number of target GameObjects to manage. "
            "Increasing adds new per-entry rows (Id, StartDelay, SolidTime, FadeOutTime, GoneTime, FadeInTime). "
                                              "Decreasing removes them.");
    }

    BlinkComponent::~BlinkComponent()
    {
    }

    // -----------------------------------------------------------------------
    // Ogre::Plugin
    // -----------------------------------------------------------------------

    void BlinkComponent::install(const Ogre::NameValuePairList* options)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<BlinkComponent>(BlinkComponent::getStaticClassId(), BlinkComponent::getStaticClassName());
    }

    void BlinkComponent::initialise()
    {
    }

    void BlinkComponent::shutdown()
    {
    }

    void BlinkComponent::uninstall()
    {
    }

    void BlinkComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    const Ogre::String& BlinkComponent::getName() const
    {
        return this->name;
    }

    // -----------------------------------------------------------------------
    // clone
    // -----------------------------------------------------------------------

    GameObjectCompPtr BlinkComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        return nullptr;
    }

    // -----------------------------------------------------------------------
    // init (XML load)
    // -----------------------------------------------------------------------

    bool BlinkComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrBlinkMode())
        {
            this->blinkMode->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "Single"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrGameObjectCount())
        {
            this->gameObjectCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data", 0));
            propertyElement = propertyElement->next_sibling("property");
        }

        unsigned int count = this->gameObjectCount->getUInt();

        // Resize all arrays
        this->gameObjectIds.resize(count);
        this->startDelays.resize(count);
        this->solidTimes.resize(count);
        this->fadeOutTimes.resize(count);
        this->goneTimes.resize(count);
        this->fadeInTimes.resize(count);

        for (unsigned int i = 0; i < count; i++)
        {
            Ogre::String idx = Ogre::StringConverter::toString(i);

            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrGameObjectId() + idx)
            {
                if (nullptr == this->gameObjectIds[i])
                {
                    this->gameObjectIds[i] = new Variant(AttrGameObjectId() + idx, XMLConverter::getAttrib(propertyElement, "data", "0"), this->attributes);
                }
                else
                {
                    this->gameObjectIds[i]->setValue(XMLConverter::getAttrib(propertyElement, "data", "0"));
                }
                propertyElement = propertyElement->next_sibling("property");
            }
            else
            {
                this->gameObjectIds[i] = new Variant(AttrGameObjectId() + idx, Ogre::String("0"), this->attributes);
            }
            this->gameObjectIds[i]->setDescription("Id of target GameObject " + idx + ". Must have an Item as its movable object.");
            this->gameObjectIds[i]->addUserData(GameObject::AttrActionSeparator());

            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrStartDelay() + idx)
            {
                if (nullptr == this->startDelays[i])
                {
                    this->startDelays[i] = new Variant(AttrStartDelay() + idx, XMLConverter::getAttribReal(propertyElement, "data", 0.0f), this->attributes);
                }
                else
                {
                    this->startDelays[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.0f));
                }
                propertyElement = propertyElement->next_sibling("property");
            }
            else
            {
                this->startDelays[i] = new Variant(AttrStartDelay() + idx, 0.0f, this->attributes);
            }
            this->startDelays[i]->setConstraints(0.0f, 60.0f);
            this->startDelays[i]->setDescription("Initial delay in seconds before the first cycle begins for entry " + idx + ". Use to stagger platforms.");

            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrSolidTime() + idx)
            {
                if (nullptr == this->solidTimes[i])
                {
                    this->solidTimes[i] = new Variant(AttrSolidTime() + idx, XMLConverter::getAttribReal(propertyElement, "data", 2.0f), this->attributes);
                }
                else
                {
                    this->solidTimes[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data", 2.0f));
                }
                propertyElement = propertyElement->next_sibling("property");
            }
            else
            {
                this->solidTimes[i] = new Variant(AttrSolidTime() + idx, 2.0f, this->attributes);
            }
            this->solidTimes[i]->setConstraints(0.0f, 60.0f);
            this->solidTimes[i]->setDescription("How long (seconds) entry " + idx + " stays fully visible and solid before fading.");

            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrFadeOutTime() + idx)
            {
                if (nullptr == this->fadeOutTimes[i])
                {
                    this->fadeOutTimes[i] = new Variant(AttrFadeOutTime() + idx, XMLConverter::getAttribReal(propertyElement, "data", 0.5f), this->attributes);
                }
                else
                {
                    this->fadeOutTimes[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.5f));
                }
                propertyElement = propertyElement->next_sibling("property");
            }
            else
            {
                this->fadeOutTimes[i] = new Variant(AttrFadeOutTime() + idx, 0.5f, this->attributes);
            }
            this->fadeOutTimes[i]->setConstraints(0.0f, 10.0f);
            this->fadeOutTimes[i]->setDescription("Duration (seconds) of the fade-out phase for entry " + idx + ". Physics stays on during this phase as a warning.");

            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrGoneTime() + idx)
            {
                if (nullptr == this->goneTimes[i])
                {
                    this->goneTimes[i] = new Variant(AttrGoneTime() + idx, XMLConverter::getAttribReal(propertyElement, "data", 1.0f), this->attributes);
                }
                else
                {
                    this->goneTimes[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
                }
                propertyElement = propertyElement->next_sibling("property");
            }
            else
            {
                this->goneTimes[i] = new Variant(AttrGoneTime() + idx, 1.0f, this->attributes);
            }
            this->goneTimes[i]->setConstraints(0.0f, 60.0f);
            this->goneTimes[i]->setDescription("How long (seconds) entry " + idx + " stays invisible (physics off) before fading back in.");

            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrFadeInTime() + idx)
            {
                if (nullptr == this->fadeInTimes[i])
                {
                    this->fadeInTimes[i] = new Variant(AttrFadeInTime() + idx, XMLConverter::getAttribReal(propertyElement, "data", 0.5f), this->attributes);
                }
                else
                {
                    this->fadeInTimes[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.5f));
                }
                propertyElement = propertyElement->next_sibling("property");
            }
            else
            {
                this->fadeInTimes[i] = new Variant(AttrFadeInTime() + idx, 0.5f, this->attributes);
            }
            this->fadeInTimes[i]->setConstraints(0.0f, 10.0f);
            this->fadeInTimes[i]->setDescription("Duration (seconds) of the fade-in phase for entry " + idx + ". Physics is re-enabled at start of this phase.");
        }

        return true;
    }

    // -----------------------------------------------------------------------
    // postInit
    // -----------------------------------------------------------------------

    bool BlinkComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[BlinkComponent] Init component for game object: " + this->gameObjectPtr->getName());
        return true;
    }

    // -----------------------------------------------------------------------
    // onRemoveComponent
    // -----------------------------------------------------------------------

    void BlinkComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();

        Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
        NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

        this->restoreAllTargets();
    }

    // -----------------------------------------------------------------------
    // connect / disconnect
    // -----------------------------------------------------------------------

    bool BlinkComponent::connect(void)
    {
        GameObjectComponent::connect();
        this->resolveTargets();

        unsigned int count = static_cast<unsigned int>(this->targets.size());
        const Ogre::String mode = this->blinkMode->getListSelectedValue();

        for (unsigned int i = 0; i < count; i++)
        {
            TargetEntry& entry = this->targets[i];
            entry.phase = Phase::Delay;
            entry.timer = 0.0f;

            // Alternating: odd-index entries start half a solid cycle ahead so they
            // are out of phase with even-index entries from the very first frame.
            if (mode == "Alternating" && i % 2 != 0)
            {
                entry.timer = -(this->solidTimes[i]->getReal() * 0.5f);
            }

            if (nullptr != entry.physics)
            {
                entry.physics->setActivated(true);
            }
        }

        // Set all targets fully opaque via tracked closure on connect
        this->applyTransparencyImmediate(1.0f);

        return true;
    }

    bool BlinkComponent::disconnect(void)
    {
        GameObjectComponent::disconnect();

        Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
        NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

        this->restoreAllTargets();
        this->targets.clear();
        return true;
    }

    void BlinkComponent::update(Ogre::Real dt, bool notSimulating)
    {
        if (true == notSimulating)
        {
            return;
        }

        unsigned int count = static_cast<unsigned int>(this->targets.size());
        if (0 == count)
        {
            return;
        }

        const Ogre::String mode = this->blinkMode->getListSelectedValue();

        // Advance all timers and phase state on the logic thread
        for (unsigned int i = 0; i < count; i++)
        {
            TargetEntry& entry = this->targets[i];
            entry.timer += dt;

            switch (entry.phase)
            {
            case Phase::Delay:
            {
                if (entry.timer >= this->startDelays[i]->getReal())
                {
                    this->enterPhase(i, Phase::Solid);
                }
                break;
            }
            case Phase::Solid:
            {
                if (entry.timer >= this->solidTimes[i]->getReal())
                {
                    this->enterPhase(i, Phase::FadeOut);
                }
                break;
            }
            case Phase::FadeOut:
            {
                Ogre::Real duration = this->fadeOutTimes[i]->getReal();
                    entry.currentAlpha = 1.0f - ((duration > 0.0f) ? Ogre::Math::Clamp(entry.timer / duration, 0.0f, 1.0f) : 1.0f);

                if (entry.timer >= duration)
                {
                    this->enterPhase(i, Phase::Gone);
                }
                break;
            }
            case Phase::Gone:
            {
                if (entry.timer >= this->goneTimes[i]->getReal())
                {
                    this->enterPhase(i, Phase::FadeIn);
                }
                break;
            }
            case Phase::FadeIn:
            {
                Ogre::Real duration = this->fadeInTimes[i]->getReal();
                    entry.currentAlpha = (duration > 0.0f) ? Ogre::Math::Clamp(entry.timer / duration, 0.0f, 1.0f) : 1.0f;

                if (entry.timer >= duration)
                {
                    this->enterPhase(i, Phase::Solid);

                    // Sequential: kick off the next entry's Delay phase
                    if (mode == "Sequential")
                    {
                        unsigned int next = (i + 1) % count;
                        if (next != i)
                        {
                            this->targets[next].phase = Phase::Delay;
                            this->targets[next].timer = 0.0f;
                        }
                    }
                }
                break;
            }
            }
        }

        // Build a snapshot of all render-relevant data for the closure —
        // no 'this', no pointers into targets, everything by value.
        struct RenderEntry
        {
            Ogre::Item*                                                     item;
            Ogre::Real                                                      alpha;
            std::vector<std::pair<Ogre::HlmsDatablock*, unsigned int>>*    clonedDatablocks;
            unsigned long                                                   targetId;
        };

        std::vector<RenderEntry> renderEntries;
        renderEntries.reserve(count);

        for (unsigned int i = 0; i < count; i++)
        {
            TargetEntry& entry = this->targets[i];
            if (nullptr == entry.item)
        {
                continue;
            }
            RenderEntry re;
            re.item             = entry.item;
            re.alpha            = entry.currentAlpha;
            re.clonedDatablocks = &entry.clonedDatablocks;
            re.targetId         = entry.gameObjectPtr->getId();
            renderEntries.push_back(re);
        }

        auto closureFunction = [renderEntries](Ogre::Real renderDt) mutable
    {
            for (auto& re : renderEntries)
        {
                const Ogre::Real clampedAlpha = Ogre::Math::Clamp(re.alpha, 0.0f, 1.0f);

                if (clampedAlpha >= 1.0f)
        {
                    // Restore originals and destroy clones
                    if (false == re.clonedDatablocks->empty())
            {
                        for (auto& [originalDatablock, subIndex] : *re.clonedDatablocks)
                {
                            if (subIndex >= re.item->getNumSubItems())
                    {
                        continue;
                    }

                            auto* currentDatablock = re.item->getSubItem(subIndex)->getDatablock();
                            re.item->getSubItem(subIndex)->setDatablock(originalDatablock);

                    if (nullptr != currentDatablock && currentDatablock != originalDatablock)
                    {
                                if (currentDatablock->getLinkedRenderables().empty())
                        {
                            currentDatablock->getCreator()->destroyDatablock(currentDatablock->getName());
                        }
                    }
                }
                        re.clonedDatablocks->clear();
                    }
            }
            else
            {
                    // Clone datablocks on first non-opaque call
                    if (true == re.clonedDatablocks->empty())
                {
                        for (unsigned int s = 0; s < re.item->getNumSubItems(); s++)
                    {
                            auto* originalDatablock = re.item->getSubItem(s)->getDatablock();
                        if (nullptr == originalDatablock)
                        {
                            continue;
                        }

                        Ogre::String originalName = originalDatablock->getName().getFriendlyText();
                            Ogre::HlmsDatablock* cloned = AppStateManager::getSingletonPtr()->getGameObjectController()->cloneDatablockUnique(
                                originalDatablock, originalName, re.targetId, static_cast<int>(s));

                        if (nullptr == cloned)
                        {
                            continue;
                        }

                        auto* clonedPbs = dynamic_cast<Ogre::HlmsPbsDatablock*>(cloned);
                        if (nullptr == clonedPbs)
                        {
                            cloned->getCreator()->destroyDatablock(cloned->getName());
                            continue;
                        }

                            re.item->getSubItem(s)->setDatablock(clonedPbs);
                            re.clonedDatablocks->emplace_back(originalDatablock, s);
                    }
                }

                    // Apply transparency on cloned datablocks
                    for (auto& [originalDatablock, subIndex] : *re.clonedDatablocks)
                {
                        if (subIndex >= re.item->getNumSubItems())
                    {
                        continue;
                    }

                        auto* clonedPbs = dynamic_cast<Ogre::HlmsPbsDatablock*>(re.item->getSubItem(subIndex)->getDatablock());
                    if (nullptr == clonedPbs)
                    {
                        continue;
                    }

                    if (clampedAlpha <= 0.0f)
                    {
                        clonedPbs->setTransparency(0.0f, Ogre::HlmsPbsDatablock::Transparent, true);
                    }
                    else
                    {
                        clonedPbs->setTransparency(clampedAlpha, Ogre::HlmsPbsDatablock::Transparent, true);
                    }
                }
            }
            }
        };

        Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
        NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
    }

    void BlinkComponent::enterPhase(unsigned int index, Phase newPhase)
    {
        if (index >= this->targets.size())
        {
            return;
        }
        TargetEntry& entry = this->targets[index];
        entry.phase = newPhase;
        entry.timer = 0.0f;
        switch (newPhase)
        {
            case Phase::Solid:
            {
                entry.currentAlpha = 1.0f;
                if (nullptr != entry.physics)
                {
                    entry.physics->setActivated(true);
                }
                break;
            }
            case Phase::FadeOut:
            {
                // Physics stays ON during fade-out — the fading is the warning to the player,
                // the platform is still solid during this phase.
                entry.currentAlpha = 1.0f;
                break;
            }
            case Phase::Gone:
            {
                entry.currentAlpha = 0.0f;
                if (nullptr != entry.physics)
                {
                    entry.physics->setActivated(false);
                }
                break;
            }
            case Phase::FadeIn:
            {
                // Re-enable physics at the start of fade-in so the platform is already
                // solid when it becomes visible again.
                entry.currentAlpha = 0.0f;
                if (nullptr != entry.physics)
                {
                    entry.physics->setActivated(true);
                }
                break;
            }
            case Phase::Delay:
            default:
                break;
        }
    }

    void BlinkComponent::applyTransparencyImmediate(Ogre::Real alpha)
    {
        for (unsigned int i = 0; i < static_cast<unsigned int>(this->targets.size()); i++)
        {
            this->targets[i].currentAlpha = alpha;
        }
    }

    void BlinkComponent::resolveTargets(void)
    {
        this->targets.clear();

        unsigned int count = this->gameObjectCount->getUInt();
        this->targets.resize(count);

        for (unsigned int i = 0; i < count; i++)
        {
            TargetEntry& entry = this->targets[i];

            unsigned long id = Ogre::StringConverter::parseUnsignedLong(this->gameObjectIds[i]->getString());
            if (0 == id)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                    "[BlinkComponent] resolveTargets: entry " + Ogre::StringConverter::toString(i) + " has id 0 — skipped.");
                continue;
            }

            auto goPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(id);
            if (nullptr == goPtr)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                    "[BlinkComponent] resolveTargets: GameObject with id " + Ogre::StringConverter::toString(id) + " not found.");
                continue;
            }

            entry.gameObjectPtr = goPtr;
            entry.item = dynamic_cast<Ogre::Item*>(goPtr->getMovableObject());
            entry.currentAlpha = 1.0f;

            if (nullptr == entry.item)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                    "[BlinkComponent] resolveTargets: GameObject '" + goPtr->getName() + "' has no Item movable object.");
            }

            // Physics is optional — not every blink target needs to be collidable.
            auto physicsCompPtr = NOWA::makeStrongPtr(goPtr->getComponent<PhysicsComponent>());
            if (nullptr != physicsCompPtr)
            {
                entry.physics = physicsCompPtr.get();
            }
        }
    }

    void BlinkComponent::restoreAllTargets(void)
    {
        unsigned int count = static_cast<unsigned int>(this->targets.size());
        if (0 == count)
        {
            return;
        }

        // Build restore data for the render thread
        struct RestoreEntry
        {
            Ogre::Item*                                                     item;
            std::vector<std::pair<Ogre::HlmsDatablock*, unsigned int>>*    clonedDatablocks;
        };

        std::vector<RestoreEntry> restoreEntries;
        restoreEntries.reserve(count);

        for (unsigned int i = 0; i < count; i++)
    {
            TargetEntry& entry = this->targets[i];

            if (nullptr != entry.physics)
        {
                entry.physics->setActivated(true);
            }

            if (nullptr == entry.item || true == entry.clonedDatablocks.empty())
            {
                continue;
            }

            RestoreEntry re;
            re.item             = entry.item;
            re.clonedDatablocks = &entry.clonedDatablocks;
            restoreEntries.push_back(re);
        }

        if (false == restoreEntries.empty())
        {
            NOWA::GraphicsModule::RenderCommand cmd = [restoreEntries]() mutable
            {
                for (auto& re : restoreEntries)
                {
                    for (auto& [originalDatablock, subIndex] : *re.clonedDatablocks)
                    {
                        if (subIndex >= re.item->getNumSubItems())
                        {
                            continue;
    }

                        auto* currentDatablock = re.item->getSubItem(subIndex)->getDatablock();
                        re.item->getSubItem(subIndex)->setDatablock(originalDatablock);

                        if (nullptr != currentDatablock && currentDatablock != originalDatablock)
                        {
                            if (currentDatablock->getLinkedRenderables().empty())
                            {
                                currentDatablock->getCreator()->destroyDatablock(currentDatablock->getName());
                            }
                        }
                    }
                    re.clonedDatablocks->clear();
                }
            };
            // enqueueAndWait is correct here — we are in disconnect/onRemoveComponent, not in the game loop.
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "BlinkComponent::restoreAllTargets");
        }
    }

    void BlinkComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (AttrBlinkMode() == attribute->getName())
        {
            this->setBlinkMode(attribute->getListSelectedValue());
        }
        else if (AttrGameObjectCount() == attribute->getName())
        {
            this->setGameObjectCount(attribute->getUInt());
        }
        else
        {
            for (unsigned int i = 0; i < this->gameObjectCount->getUInt(); i++)
            {
                Ogre::String idx = Ogre::StringConverter::toString(i);

                if (AttrGameObjectId() + idx == attribute->getName())
                {
                    this->setGameObjectId(i, attribute->getString());
                }
                else if (AttrStartDelay() + idx == attribute->getName())
                {
                    this->setStartDelay(i, attribute->getReal());
                }
                else if (AttrSolidTime() + idx == attribute->getName())
                {
                    this->setSolidTime(i, attribute->getReal());
                }
                else if (AttrFadeOutTime() + idx == attribute->getName())
                {
                    this->setFadeOutTime(i, attribute->getReal());
                }
                else if (AttrGoneTime() + idx == attribute->getName())
                {
                    this->setGoneTime(i, attribute->getReal());
                }
                else if (AttrFadeInTime() + idx == attribute->getName())
                {
                    this->setFadeInTime(i, attribute->getReal());
                }
            }
        }
    }

    // -----------------------------------------------------------------------
    // writeXML
    // -----------------------------------------------------------------------

    void BlinkComponent::writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc)
    {
        GameObjectComponent::writeXML(propertiesXML, doc);

        rapidxml::xml_node<>* propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrBlinkMode())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->blinkMode->getListSelectedValue())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrGameObjectCount())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->gameObjectCount->getUInt())));
        propertiesXML->append_node(propertyXML);

        for (unsigned int i = 0; i < this->gameObjectCount->getUInt(); i++)
        {
            Ogre::String idx = Ogre::StringConverter::toString(i);

            propertyXML = doc.allocate_node(rapidxml::node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrGameObjectId() + idx)));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->gameObjectIds[i]->getString())));
            propertiesXML->append_node(propertyXML);

            propertyXML = doc.allocate_node(rapidxml::node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrStartDelay() + idx)));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->startDelays[i]->getReal())));
            propertiesXML->append_node(propertyXML);

            propertyXML = doc.allocate_node(rapidxml::node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrSolidTime() + idx)));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->solidTimes[i]->getReal())));
            propertiesXML->append_node(propertyXML);

            propertyXML = doc.allocate_node(rapidxml::node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrFadeOutTime() + idx)));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->fadeOutTimes[i]->getReal())));
            propertiesXML->append_node(propertyXML);

            propertyXML = doc.allocate_node(rapidxml::node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrGoneTime() + idx)));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->goneTimes[i]->getReal())));
            propertiesXML->append_node(propertyXML);

            propertyXML = doc.allocate_node(rapidxml::node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrFadeInTime() + idx)));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->fadeInTimes[i]->getReal())));
            propertiesXML->append_node(propertyXML);
        }
    }

    // -----------------------------------------------------------------------
    // Setters / Getters — global
    // -----------------------------------------------------------------------

    void BlinkComponent::setBlinkMode(const Ogre::String& mode)
    {
        this->blinkMode->setListSelectedValue(mode);
    }

    Ogre::String BlinkComponent::getBlinkMode(void) const
    {
        return this->blinkMode->getListSelectedValue();
    }

    void BlinkComponent::setGameObjectCount(unsigned int count)
    {
        size_t oldSize = this->gameObjectIds.size();
        this->gameObjectCount->setValue(count);

        if (count > oldSize)
        {
            this->gameObjectIds.resize(count);
            this->startDelays.resize(count);
            this->solidTimes.resize(count);
            this->fadeOutTimes.resize(count);
            this->goneTimes.resize(count);
            this->fadeInTimes.resize(count);

            for (size_t i = oldSize; i < count; i++)
            {
                Ogre::String idx = Ogre::StringConverter::toString(i);

                this->gameObjectIds[i] = new Variant(AttrGameObjectId() + idx, Ogre::String("0"), this->attributes);
                this->gameObjectIds[i]->setDescription("Id of target GameObject " + idx + ".");
                this->gameObjectIds[i]->addUserData(GameObject::AttrActionSeparator());

                this->startDelays[i] = new Variant(AttrStartDelay() + idx, 0.0f, this->attributes);
                this->startDelays[i]->setConstraints(0.0f, 60.0f);
                this->startDelays[i]->setDescription("Initial delay in seconds before the first cycle for entry " + idx + ".");

                this->solidTimes[i] = new Variant(AttrSolidTime() + idx, 2.0f, this->attributes);
                this->solidTimes[i]->setConstraints(0.0f, 60.0f);
                this->solidTimes[i]->setDescription("Solid (visible + collidable) duration in seconds for entry " + idx + ".");

                this->fadeOutTimes[i] = new Variant(AttrFadeOutTime() + idx, 0.5f, this->attributes);
                this->fadeOutTimes[i]->setConstraints(0.0f, 10.0f);
                this->fadeOutTimes[i]->setDescription("Fade-out duration in seconds for entry " + idx + ". Physics stays on during this phase.");

                this->goneTimes[i] = new Variant(AttrGoneTime() + idx, 1.0f, this->attributes);
                this->goneTimes[i]->setConstraints(0.0f, 60.0f);
                this->goneTimes[i]->setDescription("Gone (invisible + no collision) duration in seconds for entry " + idx + ".");

                this->fadeInTimes[i] = new Variant(AttrFadeInTime() + idx, 0.5f, this->attributes);
                this->fadeInTimes[i]->setConstraints(0.0f, 10.0f);
                this->fadeInTimes[i]->setDescription("Fade-in duration in seconds for entry " + idx + ". Physics re-enabled at start.");
            }
        }
        else if (count < oldSize)
        {
            this->eraseVariants(this->gameObjectIds, count);
            this->eraseVariants(this->startDelays, count);
            this->eraseVariants(this->solidTimes, count);
            this->eraseVariants(this->fadeOutTimes, count);
            this->eraseVariants(this->goneTimes, count);
            this->eraseVariants(this->fadeInTimes, count);
        }
    }

    unsigned int BlinkComponent::getGameObjectCount(void) const
    {
        return this->gameObjectCount->getUInt();
    }

    // -----------------------------------------------------------------------
    // Setters / Getters — per entry
    // -----------------------------------------------------------------------

    void BlinkComponent::setGameObjectId(unsigned int index, const Ogre::String& id)
    {
        if (index >= this->gameObjectIds.size())
        {
            return;
        }
        this->gameObjectIds[index]->setValue(id);
    }

    Ogre::String BlinkComponent::getGameObjectId(unsigned int index) const
    {
        if (index >= this->gameObjectIds.size())
        {
            return "0";
        }
        return this->gameObjectIds[index]->getString();
    }

    void BlinkComponent::setStartDelay(unsigned int index, Ogre::Real delay)
    {
        if (index >= this->startDelays.size())
        {
            return;
        }
        this->startDelays[index]->setValue(delay);
    }

    Ogre::Real BlinkComponent::getStartDelay(unsigned int index) const
    {
        if (index >= this->startDelays.size())
        {
            return 0.0f;
        }
        return this->startDelays[index]->getReal();
    }

    void BlinkComponent::setSolidTime(unsigned int index, Ogre::Real t)
    {
        if (index >= this->solidTimes.size())
        {
            return;
        }
        this->solidTimes[index]->setValue(t);
    }

    Ogre::Real BlinkComponent::getSolidTime(unsigned int index) const
    {
        if (index >= this->solidTimes.size())
        {
            return 2.0f;
        }
        return this->solidTimes[index]->getReal();
    }

    void BlinkComponent::setFadeOutTime(unsigned int index, Ogre::Real t)
    {
        if (index >= this->fadeOutTimes.size())
        {
            return;
        }
        this->fadeOutTimes[index]->setValue(t);
    }

    Ogre::Real BlinkComponent::getFadeOutTime(unsigned int index) const
    {
        if (index >= this->fadeOutTimes.size())
        {
            return 0.5f;
        }
        return this->fadeOutTimes[index]->getReal();
    }

    void BlinkComponent::setGoneTime(unsigned int index, Ogre::Real t)
    {
        if (index >= this->goneTimes.size())
        {
            return;
        }
        this->goneTimes[index]->setValue(t);
    }

    Ogre::Real BlinkComponent::getGoneTime(unsigned int index) const
    {
        if (index >= this->goneTimes.size())
        {
            return 1.0f;
        }
        return this->goneTimes[index]->getReal();
    }

    void BlinkComponent::setFadeInTime(unsigned int index, Ogre::Real t)
    {
        if (index >= this->fadeInTimes.size())
        {
            return;
        }
        this->fadeInTimes[index]->setValue(t);
    }

    Ogre::Real BlinkComponent::getFadeInTime(unsigned int index) const
    {
        if (index >= this->fadeInTimes.size())
        {
            return 0.5f;
        }
        return this->fadeInTimes[index]->getReal();
    }

    // -----------------------------------------------------------------------
    // Free Lua getter functions (outside NOWA namespace)
    // -----------------------------------------------------------------------

    NOWA::BlinkComponent* getBlinkComponent(NOWA::GameObject* gameObject)
    {
        return NOWA::makeStrongPtr<NOWA::BlinkComponent>(gameObject->getComponent<NOWA::BlinkComponent>()).get();
    }

    NOWA::BlinkComponent* getBlinkComponentFromIndex(NOWA::GameObject* gameObject, unsigned int occurrenceIndex)
    {
        return NOWA::makeStrongPtr<NOWA::BlinkComponent>(gameObject->getComponentWithOccurrence<NOWA::BlinkComponent>(occurrenceIndex)).get();
    }

    NOWA::BlinkComponent* getBlinkComponentFromName(NOWA::GameObject* gameObject, const Ogre::String& name)
    {
        return NOWA::makeStrongPtr<NOWA::BlinkComponent>(gameObject->getComponentFromName<NOWA::BlinkComponent>(name)).get();
    }

    void BlinkComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
    {
        using namespace luabind;

        module(lua)[luabind::class_<BlinkComponent, GameObjectComponent>("BlinkComponent")
                .def("setBlinkMode", &BlinkComponent::setBlinkMode)
                .def("getBlinkMode", &BlinkComponent::getBlinkMode)
                .def("setGameObjectCount", &BlinkComponent::setGameObjectCount)
                .def("getGameObjectCount", &BlinkComponent::getGameObjectCount)
                .def("setGameObjectId", &BlinkComponent::setGameObjectId)
                .def("getGameObjectId", &BlinkComponent::getGameObjectId)
                .def("setStartDelay", &BlinkComponent::setStartDelay)
                .def("getStartDelay", &BlinkComponent::getStartDelay)
                .def("setSolidTime", &BlinkComponent::setSolidTime)
                .def("getSolidTime", &BlinkComponent::getSolidTime)
                .def("setFadeOutTime", &BlinkComponent::setFadeOutTime)
                .def("getFadeOutTime", &BlinkComponent::getFadeOutTime)
                .def("setGoneTime", &BlinkComponent::setGoneTime)
                .def("getGoneTime", &BlinkComponent::getGoneTime)
                .def("setFadeInTime", &BlinkComponent::setFadeInTime)
                .def("getFadeInTime", &BlinkComponent::getFadeInTime)];

        LuaScriptApi::getInstance()->addClassToCollection("BlinkComponent", "class inherits GameObjectComponent", BlinkComponent::getStaticInfoText());
        LuaScriptApi::getInstance()->addClassToCollection("BlinkComponent", "void setBlinkMode(string mode)", "Sets the blink preset mode: 'Single', 'Sequential', 'Alternating', 'AllAtOnce'.");
        LuaScriptApi::getInstance()->addClassToCollection("BlinkComponent", "string getBlinkMode()", "Gets the current blink preset mode.");
        LuaScriptApi::getInstance()->addClassToCollection("BlinkComponent", "void setGameObjectCount(unsigned int count)", "Sets how many target GameObjects are managed. Adds or removes per-entry Variant rows.");
        LuaScriptApi::getInstance()->addClassToCollection("BlinkComponent", "unsigned int getGameObjectCount()", "Gets the number of managed target GameObjects.");
        LuaScriptApi::getInstance()->addClassToCollection("BlinkComponent", "void setGameObjectId(unsigned int index, string id)", "Sets the GameObject id for the given entry index.");
        LuaScriptApi::getInstance()->addClassToCollection("BlinkComponent", "string getGameObjectId(unsigned int index)", "Gets the GameObject id for the given entry index.");
        LuaScriptApi::getInstance()->addClassToCollection("BlinkComponent", "void setStartDelay(unsigned int index, float delay)", "Sets the initial delay in seconds before the first cycle for entry index.");
        LuaScriptApi::getInstance()->addClassToCollection("BlinkComponent", "float getStartDelay(unsigned int index)", "Gets the initial delay in seconds for entry index.");
        LuaScriptApi::getInstance()->addClassToCollection("BlinkComponent", "void setSolidTime(unsigned int index, float t)", "Sets how long (seconds) entry index stays fully visible and solid before fading out.");
        LuaScriptApi::getInstance()->addClassToCollection("BlinkComponent", "float getSolidTime(unsigned int index)", "Gets the solid duration in seconds for entry index.");
        LuaScriptApi::getInstance()->addClassToCollection("BlinkComponent", "void setFadeOutTime(unsigned int index, float t)", "Sets the fade-out duration in seconds for entry index. Physics stays on during fade-out.");
        LuaScriptApi::getInstance()->addClassToCollection("BlinkComponent", "float getFadeOutTime(unsigned int index)", "Gets the fade-out duration in seconds for entry index.");
        LuaScriptApi::getInstance()->addClassToCollection("BlinkComponent", "void setGoneTime(unsigned int index, float t)", "Sets how long (seconds) entry index stays invisible (physics off) before fading back in.");
        LuaScriptApi::getInstance()->addClassToCollection("BlinkComponent", "float getGoneTime(unsigned int index)", "Gets the gone duration in seconds for entry index.");
        LuaScriptApi::getInstance()->addClassToCollection("BlinkComponent", "void setFadeInTime(unsigned int index, float t)", "Sets the fade-in duration in seconds for entry index. Physics is re-enabled at the start of fade-in.");
        LuaScriptApi::getInstance()->addClassToCollection("BlinkComponent", "float getFadeInTime(unsigned int index)", "Gets the fade-in duration in seconds for entry index.");

        gameObjectClass.def("getBlinkComponent", (BlinkComponent * (*)(GameObject*)) & getBlinkComponent);
        gameObjectClass.def("getBlinkComponentFromIndex", (BlinkComponent * (*)(GameObject*, unsigned int)) & getBlinkComponentFromIndex);
        gameObjectClass.def("getBlinkComponentFromName", &getBlinkComponentFromName);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "BlinkComponent getBlinkComponent()", "Gets the BlinkComponent. Use when the game object has exactly one.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "BlinkComponent getBlinkComponentFromIndex(unsigned int occurrenceIndex)", "Gets the BlinkComponent by occurrence index.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "BlinkComponent getBlinkComponentFromName(string name)", "Gets the BlinkComponent by its component name.");

        gameObjectControllerClass.def("castBlinkComponent", &GameObjectController::cast<BlinkComponent>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "BlinkComponent castBlinkComponent(BlinkComponent other)", "Casts an incoming type from a function for Lua auto completion.");
    }

    bool BlinkComponent::canStaticAddComponent(GameObject* gameObject)
    {
        auto existingComp = NOWA::makeStrongPtr(gameObject->getComponent<BlinkComponent>());
        return nullptr == existingComp;
    }

} // namespace NOWA
