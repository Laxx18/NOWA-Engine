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
#include "gameobject/PlayerControllerComponents.h"
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
        sequenceFinished(false),
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
            this->animationLengths.resize(this->animationCount->getUInt());
            this->animationBlendDurations.resize(this->animationCount->getUInt());
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
                    this->animationDurations[i]->setDescription("How long this segment is PLAYED, in seconds. See 'Animation Length' for how long the animation actually is.");
                    this->animationLengths[i] = new Variant(AnimationSequenceComponent::AttrAnimationLength() + Ogre::StringConverter::toString(i), 0.0f, this->attributes);
                    this->animationLengths[i]->setReadOnly(true);
                    this->animationLengths[i]->setDescription("Real length of the selected animation in seconds, taken from the skeleton. Informational only.");
                    this->animationBlendDurations[i] = new Variant(AnimationSequenceComponent::AttrBlendDuration() + Ogre::StringConverter::toString(i), 0.2f, this->attributes);
                    this->animationBlendDurations[i]->setDescription("Cross fade length into the NEXT animation, in seconds. The blend starts this much before the segment ends.");
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
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BlendDuration" + Ogre::StringConverter::toString(i))
            {
                if (nullptr == this->animationBlendDurations[i])
                {
                    this->animationBlendDurations[i] = new Variant(AnimationSequenceComponent::AttrBlendDuration() + Ogre::StringConverter::toString(i), XMLConverter::getAttribReal(propertyElement, "data", 0.2f), this->attributes);
                }
                else
                {
                    this->animationBlendDurations[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.2f));
                }
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

            // FIX: this used to read this->animationNames[0] and eagerly call
            // animationBlender->init(selectedAnim, repeat) here, in postInit() -
            // long before the user has configured the sequence (animation count +
            // per-index animation names) via the editor or Lua. At this point
            // this->animationNames can legitimately still be EMPTY, so
            // this->animationNames[0] was undefined behavior on an empty vector -
            // almost certainly the real crash, which then surfaced downstream
            // during the skeleton iteration right below (heap corruption from an
            // earlier out-of-bounds access often crashes somewhere unrelated).
            //
            // AnimationSequenceComponent's whole point is that the sequence gets
            // configured AFTER this runs, so there is nothing valid to init()
            // with yet. The blender only needs to exist here (so
            // getAnimationBlender() returns a valid, non-null pointer - e.g. for
            // direct Lua access) - real initialization with an actual animation
            // name already happens correctly, and only once the sequence is
            // genuinely configured, in activateAnimation() at connect()-time
            // (which already guards against an empty animationNames list).
            this->animationBlender = new NOWA::AnimationBlenderV2(item);

            // Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AnimationSequenceComponent] List all animations for mesh '" + item->getMesh()->getName() + "':");

            // Iterate V2 skeleton animations (replaces AnimationStateIterator)
            for (auto& anim : this->skeleton->getAnimationsNonConst())
            {
                // Reset state as AnimationComponentV2 does
                anim.setEnabled(false);
                anim.mWeight = 0.0f;
                anim.setTime(0.0f);

                const Ogre::String animName = anim.getName().getFriendlyText();
                this->availableAnimations.emplace_back(animName);

                // Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AnimationSequenceComponent] Animation name: " + animName + " length: " + Ogre::StringConverter::toString(anim.getDuration()) + " seconds");
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
        // Bug: this only checked the persisted 'activated' Variant, not this->bConnected (the
        // actual connect()/disconnect() lifecycle flag) - same as setActivated() already does
        // correctly a few lines below ("if (true == this->bConnected && true == activated)").
        // disconnect() calls resetAnimation() but never touches 'activated' itself, so once
        // disconnected, update() kept driving the animationBlender exactly as if still playing -
        // with several sequences running, that's several game objects that never actually stop
        // animating after disconnect.
        // A blend started from OUTSIDE this component (a Lua script calling
        // blender->blend(...) right after setActivated(false)) still needs someone to
        // advance it - addTime() is what counts timeleft down. Without this the blend
        // was armed but frozen, so the cross fade never actually ran.
        const bool blendStillRunning = (nullptr != this->animationBlender && nullptr != this->animationBlender->getTarget());

        if ((true == this->activated->getBool() || true == blendStillRunning) && true == this->bConnected && false == notSimulating)
        {
            if (nullptr != this->animationBlender && nullptr != this->animationBlender->getSource())
            {
                this->animationBlender->beginFrame();

                if (false == this->activated->getBool())
                {
                    // Deactivated: do NOT run the sequence logic below (it would advance
                    // segments and index the vectors). Just keep the blender ticking until
                    // the external cross fade has finished.
                    Ogre::Real outgoingDuration = this->animationBlender->getSource()->getDuration();
                    if (outgoingDuration > 0.0f)
                    {
                        this->animationBlender->addTime(dt / outgoingDuration, this->getClassName());
                    }

                    if (true == this->showSkeleton->getBool() && nullptr != this->skeletonVisualizer)
                    {
                        this->skeletonVisualizer->update(dt);
                    }
                    return;
                }

                // Bug: this used to be "this->timePosition += dt;" - ignoring
                // animationSpeeds[currentAnimationIndex] entirely. timePosition is the clock
                // that decides when the current segment has finished and it's time to blend
                // into the next animation in the sequence (compared against
                // animationDurations[...] below); the ACTUAL playback a few lines down
                // (addTime()) already scales dt by speed. With the two out of sync, at
                // speed > 1 the animation visually finishes (and freezes on its last frame)
                // long before timePosition catches up to switch segments; at speed < 1
                // timePosition reaches the segment's nominal duration - and switches to the
                // next animation - while the current one is still mid-playback, causing a
                // visible jump-cut. Scale timePosition by the same speed so both clocks track
                // the same real, sped-up-or-slowed-down progress.
                // Bug: once a NON repeating sequence had played its last segment, the old
                // code incremented currentAnimationIndex to size() and returned - but only
                // for that one frame. From the next frame on, this very line indexed
                // animationSpeeds[size()], i.e. one past the end of the vector, and so did
                // every other access below it. Undefined behaviour every single frame.
                if (true == this->sequenceFinished)
                {
                    return;
                }

                this->timePosition += dt * this->animationSpeeds[this->currentAnimationIndex]->getReal();

                if (true == this->firstTimeRepeat)
                {
                    this->firstTimeRepeat = false;
                }

                // A cross fade needs the OUTGOING animation to still be running while it
                // fades out. Starting the blend at the very end of the segment meant the
                // source had already reached its last frame and was frozen there, so there
                // was nothing left to fade from and the switch looked like a hard cut -
                // most obvious at the wrap around, where the sequence jumps back to the
                // start. Trigger the blend blendDuration seconds EARLY instead.
                const Ogre::Real segmentDuration = this->animationDurations[this->currentAnimationIndex]->getReal();
                const Ogre::Real segmentSpeed = this->animationSpeeds[this->currentAnimationIndex]->getReal();
                Ogre::Real blendDuration = this->animationBlendDurations[this->currentAnimationIndex]->getReal();

                if (blendDuration < 0.0f)
                {
                    blendDuration = 0.0f;
                }

                // Bug: blendDuration is a REAL time value in seconds, but timePosition
                // counts in SCALED units (dt * speed), so subtracting one from the other
                // mixed two different clocks. Example from a real scene: duration 1.03333
                // at speed 0.3 gave switchTime 0.83333 scaled = 2.78 s real, while the
                // segment itself lasts 1.03333 / 0.3 = 3.44 s real - so the blend started
                // 0.66 s early instead of 0.2 s, more than three times too soon. Scaling
                // the blend into the segment clock makes the real lead time exactly
                // blendDuration again, whatever the speed is.
                Ogre::Real blendDurationScaled = blendDuration * segmentSpeed;

                // Never let the fade eat more than half the segment, otherwise a long blend
                // on a short animation would trigger the switch immediately, every frame.
                if (blendDurationScaled > segmentDuration * 0.5f)
                {
                    blendDurationScaled = segmentDuration * 0.5f;
                }

                const Ogre::Real switchTime = segmentDuration - blendDurationScaled;

                // Start at the second animation (if existing), because the first is played when animation is activated
                if (this->timePosition >= switchTime)
                {
                    this->currentAnimationIndex++;

                    // Bug: this used to be "timePosition = 0.0f", discarding however far
                    // the last frame overshot switchTime. That remainder is lost on every
                    // single switch, so the segment clock drifts against the blender's
                    // actual playback position - and because the error ACCUMULATES over
                    // laps, the second run through the sequence behaves differently from
                    // the first. Carrying the overshoot into the new segment keeps the two
                    // clocks locked together indefinitely.
                    this->timePosition -= switchTime;
                    if (this->timePosition < 0.0f)
                    {
                        this->timePosition = 0.0f;
                    }

                    // If repeat, start the current animation index at the beginning
                    if (this->currentAnimationIndex > this->animationNames.size() - 1)
                    {
                        if (true == this->animationRepeat->getBool())
                        {
                            this->currentAnimationIndex = 0;
                        }
                        else
                        {
                            // Clamp back to the last valid index and latch, so nothing below
                            // and no later frame can index out of range.
                            this->currentAnimationIndex = this->animationNames.size() - 1;
                            this->sequenceFinished = true;
                            return;
                        }
                    }
                    // loop = FALSE, deliberately. It used to be hardcoded true, which is
                    // wrong twice over: a sequence segment is meant to play exactly once
                    // (repeating the whole sequence is what the 'Repeat' attribute does),
                    // and more importantly AnimationBlenderV2::internalBlend() only applies
                    // its phase-sync when BOTH the source loops and this flag is true:
                    //
                    //     sourcePhase = source->getCurrentFrame() / source->getNumFrames();
                    //     target->setFrame(sourcePhase * target->getNumFrames());
                    //
                    // That is meant for locomotion blends (walk into run, keeping the feet
                    // in step). Applied to a sequence it starts the incoming animation at
                    // the OUTGOING one's phase - so blending into a pick up animation while
                    // idle sits at 60% started the pick up 60% in, played the remainder,
                    // looped back and only then played it properly. Exactly the "starts
                    // briefly, stops, then does the real pick up" symptom.
                    this->animationBlender->blend(this->animationNames[this->currentAnimationIndex]->getListSelectedValue(), this->mapStringToBlendingTransition(this->animationBlendTransitions[this->currentAnimationIndex]->getListSelectedValue()),
                        blendDuration, false);

                    // Each segment in the sequence can have its own configured speed - the
                    // new segment just blended into may run at a different speed than the
                    // one before it, so push it now. See the comment in activateAnimation()
                    // for why this (not the addTime() parameter) is the real speed control.
                    this->animationBlender->setAnimationSpeed(this->animationSpeeds[this->currentAnimationIndex]->getReal());
                }

                // Ported: getDuration() replaces v1 getLength()
                Ogre::Real sourceDuration = this->animationBlender->getSource()->getDuration();
                if (sourceDuration > 0.0f)
                {
                    // Note: the value passed here is NOT what drives playback speed - inside
                    // AnimationBlenderV2::addTime()'s render-thread closure, the "time"
                    // parameter is unused; actual advancement uses renderDt, and actual speed
                    // comes from AnimationBlenderV2::setAnimationSpeed() (called above on
                    // segment switch, and in setSpeed()). This call's only real job left is to
                    // drive the addTime()->tryClaimAddTime()/beginFrame() per-frame bookkeeping
                    // (blend weights, completion detection, observer notification) each tick.
                    Ogre::Real deltaTime = dt * this->animationSpeeds[this->currentAnimationIndex]->getReal() / sourceDuration;
                    this->animationBlender->addTime(deltaTime, this->getClassName());

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

    void AnimationSequenceComponent::resetSequenceClock(void)
    {
        this->timePosition = 0.0f;
        this->currentAnimationIndex = 0;
        this->firstTimeRepeat = true;
        this->sequenceFinished = false;
    }

    void AnimationSequenceComponent::resetAnimation(void)
    {
        this->resetSequenceClock();

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
            for (unsigned int i = 0; i < static_cast<unsigned int>(this->animationBlendDurations.size()); i++)
            {
                if (AnimationSequenceComponent::AttrBlendDuration() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setBlendDuration(i, attribute->getReal());
                }
            }
            // 'Animation Length' is read only and intentionally has no branch here.
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

            // Written right after Speed<i>, matching the order init() parses them in -
            // that parser walks the properties sequentially, so the two must agree.
            // 'Animation Length' is deliberately NOT serialised: it is derived from the
            // mesh and is refreshed from the skeleton whenever the name is applied.
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "BlendDuration" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animationBlendDurations[i]->getReal())));
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
            const Ogre::Real initialBlendDuration = (false == this->animationBlendDurations.empty() && nullptr != this->animationBlendDurations[0]) ? this->animationBlendDurations[0]->getReal() : 0.2f;
            // Same reasoning as in update(): a sequence segment plays once, and passing
            // true here would additionally arm internalBlend()'s locomotion phase-sync.
            this->animationBlender->blend(this->animationNames[0]->getListSelectedValue(), this->mapStringToBlendingTransition(this->animationBlendTransitions[0]->getListSelectedValue()), initialBlendDuration, false);

            // AnimationBlenderV2::addTime()'s "time" parameter is dead code inside its
            // render-thread closure (it drives playback purely off renderDt, ignoring the
            // value passed in) - the actually working speed control is
            // AnimationBlenderV2::setAnimationSpeed(), which scales mFrameRate against a
            // cached base rate. Apply THIS segment's configured speed now, since
            // currentSpeed may still be left over from a previous run/segment otherwise.
            if (false == this->animationSpeeds.empty())
            {
                this->animationBlender->setAnimationSpeed(this->animationSpeeds[0]->getReal());
            }
        }
    }

    void AnimationSequenceComponent::setActivated(bool activated)
    {
        this->activated->setValue(activated);

        if (true == activated)
        {
            // Full reset only when (re)starting: the sequence takes the pose over anyway.
            if (nullptr != this->animationBlender && nullptr != this->animationBlender->getSource())
            {
                this->resetAnimation();
            }

            if (true == this->bConnected)
            {
                this->activateAnimation();
            }
        }
        else
        {
            // Bug: deactivating used to call resetAnimation(), which does
            //     getSource()->setEnabled(false); getSource()->mWeight = 0.0f;
            // i.e. it HARD KILLS the animation that is currently on screen. Anything
            // blending afterwards - typically a script doing
            // setActivated(false) followed by blender->blend(idle, BlendWhileAnimating)
            // - then had no live source left to fade out of, so the character snapped
            // into the new pose no matter which transition type was requested.
            //
            // Only the sequence bookkeeping is reset here. The current animation stays
            // enabled and simply stops being advanced, which makes it a perfectly good
            // source for a following cross fade. disconnect() still calls
            // resetAnimation() for the real, hard stop.
            this->resetSequenceClock();
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

        unsigned int oldSize = this->animationNames.size();

        if (animationCount > oldSize)
        {
            this->animationNames.resize(animationCount);
            this->animationBlendTransitions.resize(animationCount);
            this->animationDurations.resize(animationCount);
            this->animationLengths.resize(animationCount);
            this->animationBlendDurations.resize(animationCount);
            this->animationTimePositions.resize(animationCount);
            this->animationSpeeds.resize(animationCount);

            for (unsigned int i = oldSize; i < animationCount; i++)
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
            this->eraseVariants(this->animationLengths, animationCount);
            this->eraseVariants(this->animationBlendDurations, animationCount);
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

                    // Always mirrored into the read only length: it describes the mesh,
                    // not a user setting, so it must stay truthful at all times.
                    if (index < this->animationLengths.size() && nullptr != this->animationLengths[index])
                    {
                        this->animationLengths[index]->setValue(duration);
                    }

                    // Duration is only SEEDED, never overwritten. It used to be clobbered
                    // unconditionally, so a duration the user had deliberately configured
                    // (to play only part of an animation) was silently reset every time the
                    // component reloaded or the name was re-applied.
                    if (this->animationDurations[index]->getReal() <= 0.0f)
                    {
                        this->animationDurations[index]->setValue(duration);
                    }
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

        // Bug: this only ever stored the value for later - it never actually reached
        // AnimationBlenderV2 (the addTime() parameter it used to feed is dead code there,
        // see activateAnimation()/update() for the real mechanism). If this is the segment
        // CURRENTLY playing, apply it right away instead of waiting for the next segment
        // switch to pick it up in update().
        if (index == this->currentAnimationIndex && nullptr != this->animationBlender)
        {
            this->animationBlender->setAnimationSpeed(animationSpeed);
        }
    }

    Ogre::Real AnimationSequenceComponent::getSpeed(unsigned int index) const
    {
        if (index >= this->animationSpeeds.size())
        {
            return -1.0f;
        }
        return this->animationSpeeds[index]->getReal();
    }

    Ogre::Real AnimationSequenceComponent::getAnimationLength(unsigned int index) const
    {
        if (index >= this->animationLengths.size() || nullptr == this->animationLengths[index])
        {
            return 0.0f;
        }
        return this->animationLengths[index]->getReal();
    }

    void AnimationSequenceComponent::setBlendDuration(unsigned int index, Ogre::Real blendDuration)
    {
        // Guard with >= instead of clamping to size() - 1: on an empty vector that
        // expression underflows to a huge unsigned value and indexes out of range. The
        // same pattern is still used in several setters above and is worth cleaning up.
        if (index >= this->animationBlendDurations.size() || nullptr == this->animationBlendDurations[index])
        {
            return;
        }
        if (blendDuration < 0.0f)
        {
            blendDuration = 0.0f;
        }
        this->animationBlendDurations[index]->setValue(blendDuration);
    }

    Ogre::Real AnimationSequenceComponent::getBlendDuration(unsigned int index) const
    {
        if (index >= this->animationBlendDurations.size() || nullptr == this->animationBlendDurations[index])
        {
            return 0.0f;
        }
        return this->animationBlendDurations[index]->getReal();
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

    void AnimationSequenceComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
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