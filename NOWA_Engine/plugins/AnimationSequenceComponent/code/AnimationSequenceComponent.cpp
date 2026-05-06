/*
Copyright (c) 2025 Lukas Kalinowski

GPL v3

Ported from Ogre::v1::Entity to Ogre::Item / AnimationBlenderV2.
Key changes vs. the v1 version:
  - Ogre::v1::Entity*             -> Ogre::Item*
  - Ogre::v1::AnimationStateSet / AnimationState
                                  -> Ogre::SkeletonInstance / SkeletonAnimation
  - AnimationBlender (v1)         -> AnimationBlenderV2
  - Ogre::v1::OldBone*            -> Ogre::Bone*
  - animationState->getLength()   -> skeletonAnim->getDuration()
  - animationState->setWeight(w)  -> skeletonAnim->mWeight = w
  - animationState->setTimePosition(t) -> skeletonAnim->setTime(t)
  - AnimationBlender::BlendingTransition -> AnimationBlenderV2::BlendingTransition
*/

#include "NOWAPrecompiled.h"
#include "AnimationSequenceComponent.h"
#include "gameobject/GameObjectFactory.h"
#include "main/AppStateManager.h"
#include "main/EventManager.h"
#include "modules/LuaScriptApi.h"
#include "utilities/XMLConverter.h"

// V2 skeleton headers (required for Ogre::Item animation)
#include "Animation/OgreSkeletonAnimation.h"
#include "Animation/OgreSkeletonInstance.h"

#include "OgreAbiUtils.h"

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    AnimationSequenceComponent::AnimationSequenceComponent() :
        GameObjectComponent(),
        name("AnimationSequenceComponent"),
        activated(new Variant(AnimationSequenceComponent::AttrActivated(), true, this->attributes)),
        animationRepeat(new Variant(AnimationSequenceComponent::AttrRepeat(), true, this->attributes)),
        showSkeleton(new Variant(AnimationSequenceComponent::AttrShowSkeleton(), false, this->attributes)),
        animationCount(new Variant(AnimationSequenceComponent::AttrAnimationCount(), static_cast<unsigned int>(0), this->attributes)),
        animationBlender(nullptr),
        skeleton(nullptr),
        skeletonVisualizer(nullptr),
        timePosition(0.0f),
        firstTimeRepeat(true),
        currentAnimationIndex(0)
    {
        // Since when animation count is changed, the whole properties must be refreshed, so that new field may come for animations
        this->animationCount->addUserData(GameObject::AttrActionNeedRefresh());
    }

    AnimationSequenceComponent::~AnimationSequenceComponent(void)
    {
    }

    void AnimationSequenceComponent::initialise()
    {
    }

    const Ogre::String& AnimationSequenceComponent::getName() const
    {
        return this->name;
    }

    void AnimationSequenceComponent::install(const Ogre::NameValuePairList* options)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<AnimationSequenceComponent>(AnimationSequenceComponent::getStaticClassId(), AnimationSequenceComponent::getStaticClassName());
    }

    void AnimationSequenceComponent::shutdown()
    {
    }

    void AnimationSequenceComponent::uninstall()
    {
    }

    void AnimationSequenceComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    bool AnimationSequenceComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
        {
            this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Repeat")
        {
            this->animationRepeat->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Count")
        {
            this->animationCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        // Only create new variant, if fresh loading. If snapshot is done, no new variant
        // must be created! Because the algorithm is working changed flag of each existing variant!
        if (this->animationNames.size() < this->animationCount->getUInt())
        {
            this->animationNames.resize(this->animationCount->getUInt());
            this->animationBlendTransitions.resize(this->animationCount->getUInt());
            this->animationDurations.resize(this->animationCount->getUInt());
            this->animationTimePositions.resize(this->animationCount->getUInt());
            this->animationSpeeds.resize(this->animationCount->getUInt());
        }

        for (size_t i = 0; i < this->animationNames.size(); i++)
        {
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnimationName" + Ogre::StringConverter::toString(i))
            {
                Ogre::String animationName = XMLConverter::getAttrib(propertyElement, "data");
                // List will be filled in postInit, in which the item is available, but set the selected animation name string now, even the list is yet empty
                if (nullptr == this->animationNames[i])
                {
                    this->animationNames[i] = new Variant(AnimationSequenceComponent::AttrAnimationName() + Ogre::StringConverter::toString(i), std::vector<Ogre::String>(), this->attributes);
                    this->animationNames[i]->setListSelectedValue(animationName);
                }
                else
                {
                    this->animationNames[i]->setListSelectedValue(animationName);
                }
                this->animationNames[i]->addUserData(GameObject::AttrActionNeedRefresh());
                propertyElement = propertyElement->next_sibling("property");
            }
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BlendTransition" + Ogre::StringConverter::toString(i))
            {
                if (nullptr == this->animationBlendTransitions[i])
                {
                    this->animationBlendTransitions[i] = new Variant(AnimationSequenceComponent::AttrBlendTransition() + Ogre::StringConverter::toString(i), {"BlendWhileAnimating", "BlendSwitch", "BlendThenAnimate"}, this->attributes);

                    this->animationBlendTransitions[i]->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
                }
                else
                {
                    this->animationBlendTransitions[i]->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
                }
                propertyElement = propertyElement->next_sibling("property");
            }
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Duration" + Ogre::StringConverter::toString(i))
            {
                if (nullptr == this->animationDurations[i])
                {
                    this->animationDurations[i] = new Variant(AnimationSequenceComponent::AttrDuration() + Ogre::StringConverter::toString(i), XMLConverter::getAttribReal(propertyElement, "data"), this->attributes);
                }
                else
                {
                    this->animationDurations[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
                }
                propertyElement = propertyElement->next_sibling("property");
            }
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TimePosition" + Ogre::StringConverter::toString(i))
            {
                if (nullptr == this->animationTimePositions[i])
                {
                    this->animationTimePositions[i] = new Variant(AnimationSequenceComponent::AttrTimePosition() + Ogre::StringConverter::toString(i), XMLConverter::getAttribReal(propertyElement, "data"), this->attributes);
                }
                else
                {
                    this->animationTimePositions[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
                }
                propertyElement = propertyElement->next_sibling("property");
            }
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Speed" + Ogre::StringConverter::toString(i))
            {
                if (nullptr == this->animationSpeeds[i])
                {
                    this->animationSpeeds[i] = new Variant(AnimationSequenceComponent::AttrSpeed() + Ogre::StringConverter::toString(i), XMLConverter::getAttribReal(propertyElement, "data", 1.0f), this->attributes);
                }
                else
                {
                    this->animationSpeeds[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
                }
                this->animationSpeeds[i]->addUserData(GameObject::AttrActionSeparator());
                propertyElement = propertyElement->next_sibling("property");
            }
        }

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ShowSkeleton")
        {
            this->showSkeleton->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
            propertyElement = propertyElement->next_sibling("property");
        }

        return true;
    }

    GameObjectCompPtr AnimationSequenceComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        AnimationSequenceCompPtr clonedCompPtr(boost::make_shared<AnimationSequenceComponent>());

        clonedCompPtr->setRepeat(this->animationRepeat->getBool());

        for (size_t i = 0; i < this->animationNames.size(); i++)
        {
            clonedCompPtr->setAnimationName(static_cast<unsigned int>(i), this->animationNames[i]->getListSelectedValue());
            clonedCompPtr->setBlendTransition(static_cast<unsigned int>(i), this->animationBlendTransitions[i]->getListSelectedValue());
            clonedCompPtr->setDuration(static_cast<unsigned int>(i), this->animationDurations[i]->getReal());
            clonedCompPtr->setTimePosition(static_cast<unsigned int>(i), this->animationTimePositions[i]->getReal());
            clonedCompPtr->setSpeed(static_cast<unsigned int>(i), this->animationSpeeds[i]->getReal());
        }

        clonedGameObjectPtr->addComponent(clonedCompPtr);
        clonedCompPtr->setOwner(clonedGameObjectPtr);
        // Activation after everything is set, because the game object is required
        clonedCompPtr->setActivated(this->activated->getBool());
        clonedCompPtr->setShowSkeleton(this->showSkeleton->getBool());

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
        return clonedCompPtr;
    }

    bool AnimationSequenceComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AnimationSequenceComponent] Init animation component for game object: " + this->gameObjectPtr->getName());

        // Component must be dynamic, because it will be moved
        this->gameObjectPtr->setDynamic(true);
        this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

        this->generateAnimationList();
        return true;
    }

    void AnimationSequenceComponent::generateAnimationList(void)
    {
        // Ported: use Ogre::Item + SkeletonInstance instead of v1::Entity + AnimationStateSet
        Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
        if (nullptr != item)
        {
            this->skeleton = item->getSkeletonInstance();
            if (nullptr == this->skeleton)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AnimationSequenceComponent] Cannot initialize animation blender, because the skeleton resource for item: " + item->getName() + " is missing!");
                return;
            }

            this->animationBlender = new NOWA::AnimationBlenderV2(item);

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AnimationSequenceComponent] List all animations for mesh '" + item->getMesh()->getName() + "':");

            // Iterate V2 skeleton animations (replaces AnimationStateIterator)
            for (auto& anim : this->skeleton->getAnimationsNonConst())
            {
                // Reset state as AnimationComponentV2 does
                anim.setEnabled(false);
                anim.mWeight = 0.0f;
                anim.setTime(0.0f);

                const Ogre::String animName = anim.getName().getFriendlyText();
                this->availableAnimations.emplace_back(animName);

                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AnimationSequenceComponent] Animation name: " + animName + " length: " + Ogre::StringConverter::toString(anim.getDuration()) + " seconds");
            }

            // Animation names must be set after loading, because just in postInit the item with the animations is available
            for (size_t i = 0; i < this->animationNames.size(); i++)
            {
                // Preserve the previously selected animation name
                Ogre::String selectedAnimationName = this->animationNames[i]->getListSelectedValue();
                this->animationNames[i]->setValue(this->availableAnimations);
                this->animationNames[i]->setListSelectedValue(selectedAnimationName);
            }
        }
    }

    bool AnimationSequenceComponent::connect(void)
    {
        GameObjectComponent::connect();

        if (true == this->activated->getBool())
        {
            this->activateAnimation();
        }
        return true;
    }

    bool AnimationSequenceComponent::disconnect(void)
    {
        GameObjectComponent::disconnect();

        this->resetAnimation();
        return true;
    }

    void AnimationSequenceComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AnimationSequenceComponent] Destructor animation component for game object: " + this->gameObjectPtr->getName());

        if (nullptr != this->skeletonVisualizer)
        {
            delete this->skeletonVisualizer;
            this->skeletonVisualizer = nullptr;
        }
        if (nullptr != this->animationBlender)
        {
            delete this->animationBlender;
            this->animationBlender = nullptr;
        }
        // skeleton is owned by the Item, do not delete it
        this->skeleton = nullptr;
    }

    Ogre::String AnimationSequenceComponent::getClassName(void) const
    {
        return "AnimationSequenceComponent";
    }

    Ogre::String AnimationSequenceComponent::getParentClassName(void) const
    {
        return "GameObjectComponent";
    }

    void AnimationSequenceComponent::update(Ogre::Real dt, bool notSimulating)
    {
        if (true == this->activated->getBool() && false == notSimulating)
        {
            if (nullptr != this->animationBlender && nullptr != this->animationBlender->getSource())
            {
                this->timePosition += dt;

                if (true == this->firstTimeRepeat)
                {
                    this->firstTimeRepeat = false;
                }

                // Start at the second animation (if existing), because the first is played when animation is activated
                if (this->timePosition >= this->animationDurations[this->currentAnimationIndex]->getReal())
                {
                    this->currentAnimationIndex++;
                    this->timePosition = 0.0f;

                    // If repeat, start the current animation index at the beginning
                    if (this->currentAnimationIndex > this->animationNames.size() - 1)
                    {
                        if (true == this->animationRepeat->getBool())
                        {
                            this->currentAnimationIndex = 0;
                        }
                        else
                        {
                            return;
                        }
                    }
                    this->animationBlender->blend(this->animationNames[this->currentAnimationIndex]->getListSelectedValue(), this->mapStringToBlendingTransition(this->animationBlendTransitions[this->currentAnimationIndex]->getListSelectedValue()),
                        0.2f, true);
                }

                // Ported: getDuration() replaces v1 getLength()
                Ogre::Real sourceDuration = this->animationBlender->getSource()->getDuration();
                if (sourceDuration > 0.0f)
                {
                    // See: comments in AnimationComponentV2
                    Ogre::Real deltaTime = dt * this->animationSpeeds[this->currentAnimationIndex]->getReal() /*/ sourceDuration*/;
                    this->animationBlender->addTime(deltaTime);

                    this->animationBlender->addTime(deltaTime);

                    if (true == this->bShowDebugData)
                    {
                        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AnimationSequenceComponent] Playing Animation: '" + this->animationNames[this->currentAnimationIndex]->getListSelectedValue() + "', animation index: '" +
                                                                                                Ogre::StringConverter::toString(this->currentAnimationIndex) + "', time position: '" + Ogre::StringConverter::toString(this->timePosition) +
                                                                                                "', animation duration: '" + Ogre::StringConverter::toString(this->animationDurations[this->currentAnimationIndex]->getReal()) + "', add time: '" +
                                                                                                Ogre::StringConverter::toString(deltaTime)
                                                                                                // Ported: mWeight (public field) replaces v1 getWeight()
                                                                                                + "', weight: '" + Ogre::StringConverter::toString(this->animationBlender->getSource()->mWeight) + "', duration: '" +
                                                                                                Ogre::StringConverter::toString(this->animationDurations[this->currentAnimationIndex]->getReal()));
                    }
                }
            }
        }
        if (true == this->showSkeleton->getBool() && nullptr != this->skeletonVisualizer)
        {
            this->skeletonVisualizer->update(dt);
        }
    }

    void AnimationSequenceComponent::resetAnimation(void)
    {
        this->timePosition = 0.0f;
        this->currentAnimationIndex = 0;
        this->firstTimeRepeat = true;

        if (nullptr != this->animationBlender && nullptr != this->animationBlender->getSource())
        {
            // Ported: mWeight (public field) replaces v1 setWeight()
            //         setTime() replaces v1 setTimePosition()
            this->animationBlender->getSource()->setEnabled(false);
            this->animationBlender->getSource()->mWeight = 0.0f;
            this->animationBlender->getSource()->setTime(0.0f);

            if (nullptr != this->animationBlender->getTarget())
            {
                this->animationBlender->getTarget()->setEnabled(false);
                this->animationBlender->getTarget()->mWeight = 0.0f;
                this->animationBlender->getTarget()->setTime(0.0f);
            }
        }
    }

    void AnimationSequenceComponent::actualizeValue(Variant* attribute)
    {
        if (nullptr != this->animationBlender && nullptr != this->animationBlender->getSource())
        {
            // Ported: setTime() replaces v1 setTimePosition()
            this->animationBlender->getSource()->setTime(0.0f);
        }

        GameObjectComponent::actualizeValue(attribute);

        if (AnimationSequenceComponent::AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
        else if (AnimationSequenceComponent::AttrRepeat() == attribute->getName())
        {
            this->setRepeat(attribute->getBool());
        }
        else if (AnimationSequenceComponent::AttrAnimationCount() == attribute->getName())
        {
            this->setAnimationCount(attribute->getUInt());
        }
        else if (AnimationSequenceComponent::AttrShowSkeleton() == attribute->getName())
        {
            this->setShowSkeleton(attribute->getBool());
        }
        else
        {
            for (unsigned int i = 0; i < static_cast<unsigned int>(this->animationNames.size()); i++)
            {
                if (AnimationSequenceComponent::AttrAnimationName() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setAnimationName(i, attribute->getListSelectedValue());
                }
            }
            for (unsigned int i = 0; i < static_cast<unsigned int>(this->animationBlendTransitions.size()); i++)
            {
                if (AnimationSequenceComponent::AttrBlendTransition() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setBlendTransition(i, attribute->getListSelectedValue());
                }
            }
            for (unsigned int i = 0; i < static_cast<unsigned int>(this->animationDurations.size()); i++)
            {
                if (AnimationSequenceComponent::AttrDuration() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setDuration(i, attribute->getReal());
                }
            }
            for (unsigned int i = 0; i < static_cast<unsigned int>(this->animationTimePositions.size()); i++)
            {
                if (AnimationSequenceComponent::AttrTimePosition() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setTimePosition(i, attribute->getReal());
                }
            }
            for (unsigned int i = 0; i < static_cast<unsigned int>(this->animationSpeeds.size()); i++)
            {
                if (AnimationSequenceComponent::AttrSpeed() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setSpeed(i, attribute->getReal());
                }
            }
        }
    }

    void AnimationSequenceComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Repeat"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animationRepeat->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Count"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animationCount->getUInt())));
        propertiesXML->append_node(propertyXML);

        for (size_t i = 0; i < this->animationNames.size(); i++)
        {
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "AnimationName" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animationNames[i]->getListSelectedValue())));
            propertiesXML->append_node(propertyXML);

            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "BlendTransition" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animationBlendTransitions[i]->getListSelectedValue())));
            propertiesXML->append_node(propertyXML);

            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "Duration" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animationDurations[i]->getReal())));
            propertiesXML->append_node(propertyXML);

            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "TimePosition" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animationTimePositions[i]->getReal())));
            propertiesXML->append_node(propertyXML);

            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "Speed" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animationSpeeds[i]->getReal())));
            propertiesXML->append_node(propertyXML);
        }

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "ShowSkeleton"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->showSkeleton->getBool())));
        propertiesXML->append_node(propertyXML);
    }

    void AnimationSequenceComponent::activateAnimation(void)
    {
        if (nullptr == this->gameObjectPtr)
        {
            return;
        }

        if (true == this->animationNames.empty())
        {
            return;
        }

        // Ported: use Ogre::Item instead of Ogre::v1::Entity
        Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
        if (nullptr != item)
        {
            // First deactivate, then restart from the first animation
            this->resetAnimation();
            this->animationBlender->init(this->animationNames[0]->getListSelectedValue(), false);
            this->animationBlender->blend(this->animationNames[0]->getListSelectedValue(), this->mapStringToBlendingTransition(this->animationBlendTransitions[0]->getListSelectedValue()), 0.2f, true);
        }
    }

    void AnimationSequenceComponent::setActivated(bool activated)
    {
        this->activated->setValue(activated);

        // First deactivate
        if (nullptr != this->animationBlender && nullptr != this->animationBlender->getSource())
        {
            this->resetAnimation();
        }

        if (true == this->bConnected && true == activated)
        {
            this->activateAnimation();
        }
    }

    bool AnimationSequenceComponent::isActivated(void) const
    {
        return this->activated->getBool();
    }

    void AnimationSequenceComponent::setRepeat(bool animationRepeat)
    {
        this->animationRepeat->setValue(animationRepeat);
        this->setActivated(this->activated->getBool());
    }

    bool AnimationSequenceComponent::getRepeat(void) const
    {
        return this->animationRepeat->getBool();
    }

    void AnimationSequenceComponent::setShowSkeleton(bool showSkeleton)
    {
        this->showSkeleton->setValue(showSkeleton);
        // Ported: Ogre::Item has no setDisplaySkeleton(); SkeletonVisualizer would
        // need to be adapted for V2 items separately (currently not supported).
        // Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
    }

    bool AnimationSequenceComponent::getShowSkeleton(void) const
    {
        return this->showSkeleton->getBool();
    }

    void AnimationSequenceComponent::setAnimationCount(unsigned int animationCount)
    {
        this->animationCount->setValue(animationCount);

        size_t oldSize = this->animationNames.size();

        if (animationCount > oldSize)
        {
            this->animationNames.resize(animationCount);
            this->animationBlendTransitions.resize(animationCount);
            this->animationDurations.resize(animationCount);
            this->animationTimePositions.resize(animationCount);
            this->animationSpeeds.resize(animationCount);

            for (size_t i = oldSize; i < animationCount; i++)
            {
                this->animationNames[i] = new Variant(AnimationSequenceComponent::AttrAnimationName() + Ogre::StringConverter::toString(i), this->availableAnimations, this->attributes);
                this->animationNames[i]->addUserData(GameObject::AttrActionNeedRefresh());
                this->animationBlendTransitions[i] = new Variant(AnimationSequenceComponent::AttrBlendTransition() + Ogre::StringConverter::toString(i), {"BlendWhileAnimating", "BlendSwitch", "BlendThenAnimate"}, this->attributes);
                this->animationDurations[i] = new Variant(AnimationSequenceComponent::AttrDuration() + Ogre::StringConverter::toString(i), 0.0f, this->attributes);
                this->setAnimationName(i, this->animationNames[i]->getListSelectedValue());

                this->animationTimePositions[i] = new Variant(AnimationSequenceComponent::AttrTimePosition() + Ogre::StringConverter::toString(i), 0.0f, this->attributes);
                this->animationSpeeds[i] = new Variant(AnimationSequenceComponent::AttrSpeed() + Ogre::StringConverter::toString(i), 1.0f, this->attributes);
                this->animationSpeeds[i]->addUserData(GameObject::AttrActionSeparator());
            }
        }
        else if (animationCount < oldSize)
        {
            this->eraseVariants(this->animationNames, animationCount);
            this->eraseVariants(this->animationBlendTransitions, animationCount);
            this->eraseVariants(this->animationDurations, animationCount);
            this->eraseVariants(this->animationTimePositions, animationCount);
            this->eraseVariants(this->animationSpeeds, animationCount);
        }
    }

    unsigned int AnimationSequenceComponent::getAnimationCount(void) const
    {
        return this->animationCount->getUInt();
    }

    void AnimationSequenceComponent::setAnimationName(unsigned int index, const Ogre::String& animationName)
    {
        if (index >= this->animationNames.size())
        {
            index = static_cast<unsigned int>(this->animationNames.size()) - 1;
        }

        this->animationNames[index]->setListSelectedValue(animationName);

        // Ported: use Ogre::Item + SkeletonInstance instead of v1::Entity + AnimationStateSet
        Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
        if (nullptr != item)
        {
            Ogre::SkeletonInstance* skeletonInst = item->getSkeletonInstance();
            if (nullptr != skeletonInst)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AnimationSequenceComponent] List all animations for mesh '" + item->getMesh()->getName() + "':");

                // Ported: hasAnimation() + getAnimation()->getDuration() replace
                //         hasAnimationState() + getAnimationState()->getLength()
                if (true == skeletonInst->hasAnimation(animationName))
                {
                    Ogre::Real duration = skeletonInst->getAnimation(animationName)->getDuration();
                    this->animationDurations[index]->setValue(duration);
                }
            }
        }
        this->setActivated(this->activated->getBool());
    }

    Ogre::String AnimationSequenceComponent::getAnimationName(unsigned int index) const
    {
        if (index >= this->animationNames.size())
        {
            return "";
        }
        return this->animationNames[index]->getListSelectedValue();
    }

    void AnimationSequenceComponent::setBlendTransition(unsigned int index, const Ogre::String& strBlendTransition)
    {
        if (index >= this->animationBlendTransitions.size())
        {
            index = static_cast<unsigned int>(this->animationBlendTransitions.size()) - 1;
        }

        this->animationBlendTransitions[index]->setListSelectedValue(strBlendTransition);
    }

    Ogre::String AnimationSequenceComponent::getBlendTransition(unsigned int index) const
    {
        if (index >= this->animationBlendTransitions.size())
        {
            return "";
        }
        return this->animationBlendTransitions[index]->getListSelectedValue();
    }

    void AnimationSequenceComponent::setDuration(unsigned int index, Ogre::Real duration)
    {
        if (index >= this->animationDurations.size())
        {
            index = static_cast<unsigned int>(this->animationDurations.size()) - 1;
        }

        this->animationDurations[index]->setValue(duration);
    }

    Ogre::Real AnimationSequenceComponent::getDuration(unsigned int index)
    {
        if (index >= this->animationDurations.size())
        {
            return -1.0f;
        }
        return this->animationDurations[index]->getReal();
    }

    void AnimationSequenceComponent::setTimePosition(unsigned int index, Ogre::Real timePosition)
    {
        if (index >= this->animationTimePositions.size())
        {
            index = static_cast<unsigned int>(this->animationTimePositions.size()) - 1;
        }

        this->animationTimePositions[index]->setValue(timePosition);
    }

    Ogre::Real AnimationSequenceComponent::getTimePosition(unsigned int index)
    {
        if (index >= this->animationTimePositions.size())
        {
            return -1.0f;
        }
        return this->animationTimePositions[index]->getReal();
    }

    void AnimationSequenceComponent::setSpeed(unsigned int index, Ogre::Real animationSpeed)
    {
        if (index >= this->animationSpeeds.size())
        {
            index = static_cast<unsigned int>(this->animationSpeeds.size()) - 1;
        }

        this->animationSpeeds[index]->setValue(animationSpeed);
    }

    Ogre::Real AnimationSequenceComponent::getSpeed(unsigned int index) const
    {
        if (index >= this->animationSpeeds.size())
        {
            return -1.0f;
        }
        return this->animationSpeeds[index]->getReal();
    }

    AnimationBlenderV2* AnimationSequenceComponent::getAnimationBlender(void) const
    {
        return this->animationBlender;
    }

    // Ported: returns Ogre::Bone* (V2) instead of Ogre::v1::OldBone*
    Ogre::Bone* AnimationSequenceComponent::getBone(const Ogre::String& boneName)
    {
        // Use the cached skeleton pointer; fall back to fetching from the item
        if (nullptr == this->skeleton)
        {
            Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
            if (nullptr != item)
            {
                this->skeleton = item->getSkeletonInstance();
            }
        }

        if (nullptr != this->skeleton)
        {
            if (true == this->skeleton->hasBone(boneName))
            {
                return this->skeleton->getBone(boneName);
            }
        }
        return nullptr;
    }

    // Ported: AnimationBlender::BlendingTransition -> AnimationBlenderV2::BlendingTransition
    Ogre::String AnimationSequenceComponent::mapBlendingTransitionToString(AnimationBlenderV2::BlendingTransition blendingTransition)
    {
        Ogre::String strBlendingTransition = "BlendWhileAnimating";
        switch (blendingTransition)
        {
        case AnimationBlenderV2::BlendSwitch:
            strBlendingTransition = "BlendSwitch";
            break;
        case AnimationBlenderV2::BlendThenAnimate:
            strBlendingTransition = "BlendThenAnimate";
            break;
        default:
            break;
        }
        return strBlendingTransition;
    }

    AnimationBlenderV2::BlendingTransition AnimationSequenceComponent::mapStringToBlendingTransition(const Ogre::String& strBlendingTransition)
    {
        AnimationBlenderV2::BlendingTransition blendingTransition = AnimationBlenderV2::BlendWhileAnimating;
        if ("BlendSwitch" == strBlendingTransition)
        {
            blendingTransition = AnimationBlenderV2::BlendSwitch;
        }
        else if ("BlendThenAnimate" == strBlendingTransition)
        {
            blendingTransition = AnimationBlenderV2::BlendThenAnimate;
        }
        return blendingTransition;
    }

    // Lua registration part

    AnimationSequenceComponent* getAnimationSequenceComponentFromIndex(GameObject* gameObject, unsigned int occurrenceIndex)
    {
        return makeStrongPtr<AnimationSequenceComponent>(gameObject->getComponentWithOccurrence<AnimationSequenceComponent>(occurrenceIndex)).get();
    }

    AnimationSequenceComponent* getAnimationSequenceComponent(GameObject* gameObject)
    {
        return makeStrongPtr<AnimationSequenceComponent>(gameObject->getComponent<AnimationSequenceComponent>()).get();
    }

    AnimationSequenceComponent* getAnimationSequenceComponentFromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<AnimationSequenceComponent>(gameObject->getComponentFromName<AnimationSequenceComponent>(name)).get();
    }

    void AnimationSequenceComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
    {
        module(lua)[class_<AnimationSequenceComponent, GameObjectComponent>("AnimationSequenceComponent")
                .def("getParentClassName", &AnimationSequenceComponent::getParentClassName)
                .def("setActivated", &AnimationSequenceComponent::setActivated)
                .def("isActivated", &AnimationSequenceComponent::isActivated)
                .def("getAnimationCount", &AnimationSequenceComponent::getAnimationCount)
                .def("getAnimationName", &AnimationSequenceComponent::getAnimationName)
                .def("setBlendTransition", &AnimationSequenceComponent::setBlendTransition)
                .def("getBlendTransition", &AnimationSequenceComponent::getBlendTransition)
                .def("setDuration", &AnimationSequenceComponent::setDuration)
                .def("getDuration", &AnimationSequenceComponent::getDuration)
                .def("setTimePosition", &AnimationSequenceComponent::setTimePosition)
                .def("getTimePosition", &AnimationSequenceComponent::getTimePosition)
                .def("setSpeed", &AnimationSequenceComponent::setSpeed)
                .def("getSpeed", &AnimationSequenceComponent::getSpeed)
                .def("setRepeat", &AnimationSequenceComponent::setRepeat)
                .def("getRepeat", &AnimationSequenceComponent::getRepeat)
                .def("getAnimationBlender", &AnimationSequenceComponent::getAnimationBlender)
                .def("getBone", &AnimationSequenceComponent::getBone)];

        LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "class inherits GameObjectComponent", AnimationSequenceComponent::getStaticInfoText());
        LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not (Start the animations).");
        LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "bool isActivated()", "Gets whether this component is activated.");
        LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "int getAnimationCount()", "Gets the number of used animations.");
        LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "void setBlendTransition(int index, String strBlendTransition)",
            "Sets the blending transition for the given animation by index. Possible values are: 'BlendWhileAnimating', 'BlendSwitch', 'BlendThenAnimate'");
        LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "String getBlendTransition(int index)",
            "Gets the blending transition for the given animation by index. Possible values are: 'BlendWhileAnimating', 'BlendSwitch', 'BlendThenAnimate'");
        LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "void setDuration(int index, float duration)", "Sets the duration in seconds for the given animation by index.");
        LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "float getDuration(int index)", "Gets the duration in seconds for the given animation by index.");
        LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "void setTimePosition(int index, float timePosition)", "Sets the time position in seconds for the given animation by index.");
        LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "float getTimePosition(int index)", "Gets the time position in seconds for the given animation by index.");
        LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "void setSpeed(int index, float speed)", "Sets the speed for the given animation by index.");
        LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "float getSpeed(int index)", "Gets the speed for the given animation by index.");
        LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "void setRepeat(bool repeat)", "Sets whether to repeat the whole sequence, after it has been played.");
        LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "bool getRepeat()", "Gets whether the whole sequence is repeated, after it has been played.");
        LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "AnimationBlenderV2 getAnimationBlender()", "Gets animation blender to manipulate animations directly.");
        LuaScriptApi::getInstance()->addClassToCollection("AnimationSequenceComponent", "Bone getBone(String boneName)", "Gets the bone by the given bone name for direct manipulation. Nil is delivered, if the bone name does not exist.");

        gameObjectClass.def("getAnimationSequenceComponentFromName", &getAnimationSequenceComponentFromName);
        gameObjectClass.def("getAnimationSequenceComponent", (AnimationSequenceComponent * (*)(GameObject*)) & getAnimationSequenceComponent);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "AnimationSequenceComponent getAnimationSequenceComponent()", "Gets the component. This can be used if the game object this component just once.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "AnimationSequenceComponent getAnimationSequenceComponentFromName(String name)", "Gets the component from name.");

        gameObjectControllerClass.def("castAnimationSequenceComponent", &GameObjectController::cast<AnimationSequenceComponent>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "AnimationSequenceComponent castAnimationSequenceComponent(AnimationSequenceComponent other)", "Casts an incoming type from function for lua auto completion.");
    }

    bool AnimationSequenceComponent::canStaticAddComponent(GameObject* gameObject)
    {
        // Can only be added once
        auto animationSequenceCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<AnimationSequenceComponent>());
        if (nullptr != animationSequenceCompPtr)
        {
            return false;
        }

        // Ported: use Ogre::Item + SkeletonInstance instead of v1::Entity + AnimationStateSet
        auto playerControllerCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<PlayerControllerComponent>());
        Ogre::Item* item = gameObject->getMovableObject<Ogre::Item>();
        if (nullptr != item && nullptr == playerControllerCompPtr)
        {
            Ogre::SkeletonInstance* skeletonInst = item->getSkeletonInstance();
            if (nullptr != skeletonInst)
            {
                if (false == skeletonInst->getAnimations().empty())
                {
                    return true;
                }
            }
        }
        return false;
    }

}; // namespace end