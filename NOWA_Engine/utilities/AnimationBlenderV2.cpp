#include "NOWAPrecompiled.h"
#include "AnimationBlenderV2.h"
#include "main/AppStateManager.h"

#include "Animation/OgreSkeletonAnimation.h"
#include "Animation/OgreSkeletonInstance.h"
#include "Animation/OgreTagPoint.h"

namespace NOWA
{
    AnimationBlenderV2::AnimationBlenderV2(Ogre::Item* item) :
        item(item),
        skeleton(nullptr),
        source(nullptr),
        target(nullptr),
        previousSource(nullptr),
        transition(BlendingTransition::BlendWhileAnimating),
        previousTransition(BlendingTransition::BlendWhileAnimating),
        timeleft(0.0f),
        duration(0.0f),
        previousDuration(0.0f),
        loop(false),
        previousLoop(false),
        complete(false),
        canAnimate(true),
        debugLog(false),
        currentSpeed(1.0f),
        overlaySource(nullptr),
        overlayTimeleft(0.0f),
        overlayDuration(0.0f),
        overlayBlendingOut(false),
        overlayLoop(false),
        overlayBlendOutTime(0.2f),
        overlaySpeed(1.0f),
        overlayChainInfluence(1.0f),
        overlayOutsideInfluence(0.0f)
    {
        this->uniqueId = NOWA::makeUniqueID();

        this->skeleton = this->item->getSkeletonInstance();
        if (nullptr == this->skeleton)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Animation BlenderV2] Cannot initialize animation blender, because the skeleton resource for item: " + item->getName() + " is missing!");
            return;
        }

        this->getAllAvailableAnimationNames(false);

        AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &AnimationBlenderV2::gameObjectIsInRagDollStateDelegate), EventDataGameObjectIsInRagDollingState::getStaticEventType());
    }

    AnimationBlenderV2::~AnimationBlenderV2()
    {
        // Remove the per-frame addTime closure BEFORE this object is freed.
        // The closure captures `this` — if it keeps running after destruction,
        // it reads freed memory and can corrupt Ogre's skeleton state.
        Ogre::String id = "AnimationBlenderV2::addTime" + Ogre::StringConverter::toString(this->uniqueId);
        NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

        this->item = nullptr;
        AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &AnimationBlenderV2::gameObjectIsInRagDollStateDelegate), EventDataGameObjectIsInRagDollingState::getStaticEventType());
    }

    void AnimationBlenderV2::init(AnimID animationId, bool loop)
    {
        auto it = this->mappedAnimations.find(animationId);
        if (it != this->mappedAnimations.cend())
        {
            this->internalInit(it->second, loop);
        }
    }

    void AnimationBlenderV2::init(const Ogre::String& animationName, bool loop)
    {
        this->internalInit(animationName, loop);
    }

    std::vector<Ogre::String> AnimationBlenderV2::getAllAvailableAnimationNames(bool skipLogging)
    {
        std::vector<Ogre::String> animationNames;

        if (false == skipLogging)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[Animation Blender] List all animations for mesh '" + this->item->getMesh()->getName() + "':");
        }

        if (nullptr == this->skeleton)
        {
            return animationNames;
        }

        for (auto& anim : this->skeleton->getAnimationsNonConst())
        {
            anim.setEnabled(false);
            anim.mWeight = 0.0f;
            anim.setTime(0.0f);
            animationNames.emplace_back(anim.getName().getFriendlyText());

            // Capture the authored frame rate before anything modifies it
            this->baseFrameRates.insert(std::make_pair(anim.getName().getFriendlyText(), anim.mFrameRate));

            /*if (false == skipLogging)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AnimationComponent] Animation name: " + anim.getName().getFriendlyText() + " length: " + Ogre::StringConverter::toString(anim.getDuration()) + " seconds");
            }*/
        }

        return animationNames;
    }

    void AnimationBlenderV2::internalInit(const Ogre::String& animationName, bool loop)
    {
        this->canAnimate = true;

        if (nullptr == this->skeleton)
        {
            // No animations available, permanently disable to avoid null-pointer spam.
            this->canAnimate = false;
            return;
        }

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, animationName, loop]()
        {
            // BUG FIX (NEW BUG B): If a blend was in progress when init() is called,
            // cancel it cleanly before iterating all animations. Without this guard, the
            // loop below calls setEnabled(false) on this->target while timeleft > 0.
            // addTime() would then try to advance a disabled target for the remaining
            // timeleft frames, producing a visual snap to source-only mid-transition.
            if (this->timeleft > 0.0f && nullptr != this->target)
            {
                this->target->setEnabled(false);
                this->target->mWeight = 0.0f;
                this->target = nullptr;
                this->timeleft = 0.0f;
            }

            // Disable every animation so no previously running animation bleeds through.
            // setEnabled() has an internal guard (if mEnabled != bEnable), so calling
            // this on already-disabled animations is harmless.
            for (auto& anim : this->skeleton->getAnimationsNonConst())
            {
                anim.setEnabled(false);
                anim.mWeight = 0.0f;
                anim.setTime(0.0f);
            }

            // Set the new animation as the active source.
            if (true == this->skeleton->hasAnimation(animationName))
            {
                this->source = this->skeleton->getAnimation(animationName);
                this->source->setEnabled(true);
                this->source->mWeight = 1.0f;
                this->source->setTime(0.0f); // BUG FIX (Bug 2a): always start from frame 0,
                                             // not from a stale position of a previous playthrough.
                this->timeleft = 0.0f;
                this->duration = this->source->getDuration();
                this->target = nullptr;
                this->complete = false;
                this->loop = loop;

                // BUG FIX (Bug 2b + NEW BUG C): Ogre's default mLoop is TRUE.
                // Without calling setLoop() here, a non-looping animation (loop=false)
                // would incorrectly wrap on the very first addTime() call, because
                // addFrame() reads mLoop before our deferred setLoop() in addTime()
                // had a chance to run.
                this->source->setLoop(loop);

                // Apply current speed to the freshly assigned source.
                auto it = this->baseFrameRates.find(animationName);
                if (it != this->baseFrameRates.end())
                {
                    this->source->mFrameRate = it->second * this->currentSpeed;
                }
            }
        };
        // TODO: Was enqueueAndWait, but enqueue does also work
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "AnimationBlenderV2::internalInit");
    }

    void AnimationBlenderV2::blend(AnimID animationId, BlendingTransition transition, Ogre::Real duration, bool loop)
    {
        if (false == this->canAnimate)
        {
            return;
        }

        auto it2 = this->mappedAnimations.find(animationId);
        if (this->mappedAnimations.end() == it2)
        {
            // if the animation cannot be found, just skip
            return;
        }

        this->internalBlend(it2->second, transition, duration, loop);
    }

    void AnimationBlenderV2::blend(const Ogre::String& animationName, BlendingTransition transition, Ogre::Real duration, bool loop)
    {
        if (true == animationName.empty())
        {
            return;
        }

        this->internalBlend(animationName, transition, duration, loop);
    }

    void AnimationBlenderV2::blend(AnimID animationId, BlendingTransition transition, bool loop)
    {
        auto it2 = this->mappedAnimations.find(animationId);
        if (this->mappedAnimations.end() == it2)
        {
            // if the animation cannot be found, just skip
            return;
        }

        this->internalBlend(it2->second, transition, 0.2f, loop);
    }

    void AnimationBlenderV2::blend(const Ogre::String& animationName, BlendingTransition transition, bool loop)
    {
        if (true == animationName.empty())
        {
            return;
        }

        this->internalBlend(animationName, transition, 0.2f, loop);
    }

    void AnimationBlenderV2::blend(AnimID animationId, BlendingTransition transition)
    {
        auto it2 = this->mappedAnimations.find(animationId);
        if (this->mappedAnimations.end() == it2)
        {
            // if the animation cannot be found, just skip
            return;
        }

        this->internalBlend(it2->second, transition, 0.2f, true);
    }

    void AnimationBlenderV2::blend(const Ogre::String& animationName, BlendingTransition transition)
    {
        if (true == animationName.empty())
        {
            return;
        }

        this->internalBlend(animationName, transition, 0.2f, true);
    }

    void AnimationBlenderV2::blendPhaseSynced(AnimID animationId, BlendingTransition transition, Ogre::Real duration, bool loop)
    {
        if (false == this->canAnimate)
        {
            return;
        }

        auto it2 = this->mappedAnimations.find(animationId);
        if (this->mappedAnimations.end() == it2)
        {
            // if the animation cannot be found, just skip
            return;
        }

        this->internalBlend(it2->second, transition, duration, loop, true);
    }

    void AnimationBlenderV2::blendPhaseSynced(const Ogre::String& animationName, BlendingTransition transition, Ogre::Real duration, bool loop)
    {
        if (true == animationName.empty())
        {
            return;
        }

        this->internalBlend(animationName, transition, duration, loop, true);
    }

    void AnimationBlenderV2::blendExclusive(AnimID animationId, BlendingTransition transition, Ogre::Real duration, bool loop)
    {
        auto it2 = this->mappedAnimations.find(animationId);
        if (this->mappedAnimations.end() == it2)
        {
            // if the animation cannot be found, just skip
            return;
        }

        if (true == this->isAnimationActive(animationId))
        {
            return;
        }

        if (true == this->isTargetAnimationActive(animationId))
        {
            return;
        }

        this->internalBlend(it2->second, transition, duration, loop);
    }

    void AnimationBlenderV2::blendExclusive(const Ogre::String& animationName, BlendingTransition transition, Ogre::Real duration, bool loop)
    {
        if (true == animationName.empty())
        {
            return;
        }

        if (true == this->isAnimationActive(this->getAnimationIdFromString(animationName)))
        {
            return;
        }

        if (true == this->isTargetAnimationActive(this->getAnimationIdFromString(animationName)))
        {
            return;
        }

        this->internalBlend(animationName, transition, duration, loop);
    }

    void AnimationBlenderV2::blendExclusive(AnimID animationId, BlendingTransition transition, bool loop)
    {
        auto it2 = this->mappedAnimations.find(animationId);
        if (this->mappedAnimations.end() == it2)
        {
            // if the animation cannot be found, just skip
            return;
        }

        if (true == this->isAnimationActive(animationId))
        {
            return;
        }

        if (true == this->isTargetAnimationActive(animationId))
        {
            return;
        }

        this->internalBlend(it2->second, transition, 0.2f, loop);
    }

    void AnimationBlenderV2::blendExclusive(const Ogre::String& animationName, BlendingTransition transition, bool loop)
    {
        if (true == animationName.empty())
        {
            return;
        }

        if (true == this->isAnimationActive(this->getAnimationIdFromString(animationName)))
        {
            return;
        }

        if (true == this->isTargetAnimationActive(this->getAnimationIdFromString(animationName)))
        {
            return;
        }

        this->internalBlend(animationName, transition, 0.2f, loop);
    }

    void AnimationBlenderV2::blendExclusive(AnimID animationId, BlendingTransition transition)
    {
        auto it2 = this->mappedAnimations.find(animationId);
        if (this->mappedAnimations.end() == it2)
        {
            // if the animation cannot be found, just skip
            return;
        }

        if (true == this->isAnimationActive(animationId))
        {
            return;
        }
        if (true == this->isTargetAnimationActive(animationId))
        {
            return;
        }

        this->internalBlend(it2->second, transition, 0.2f, true);
    }

    void AnimationBlenderV2::blendExclusive(const Ogre::String& animationName, BlendingTransition transition)
    {
        if (true == animationName.empty())
        {
            return;
        }

        if (true == this->isAnimationActive(this->getAnimationIdFromString(animationName)))
        {
            return;
        }

        if (true == this->isTargetAnimationActive(this->getAnimationIdFromString(animationName)))
        {
            return;
        }

        this->internalBlend(animationName, transition, 0.2f, true);
    }

    void AnimationBlenderV2::internalBlend(const Ogre::String& animationName, BlendingTransition transition, Ogre::Real duration, bool loop, bool phaseSync)
    {
        // Note: If entity is in ragdolling state, animation would explode the ragdoll and since its no possible to avoid, that a developer calls
        // e.g. in a lua script some animation functions, it must be avoided internally
        if (false == this->canAnimate)
        {
            return;
        }

        if (nullptr == this->source)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Animation Blender] Animation name: " + animationName + " cannot be blend, because 'init' has not been called!");
            return;
        }

        if (false == this->complete && nullptr != this->previousSource)
        {
            return;
        }

        // TODO Wait?
        NOWA::GraphicsModule::RenderCommand renderCommand = [this, animationName, transition, duration, loop, phaseSync]()
        {
            this->loop = loop;

            if (transition == AnimationBlenderV2::BlendSwitch)
            {
                // Kept in sync so later logic never reads a stale transition type from a
                // previous blend. timeleft is zeroed below, so nothing reads it right now,
                // but leaving it stale is a trap for the next change here.
                this->transition = transition;

                if (this->source != nullptr)
                {
                    this->source->setEnabled(false);
                }

                Ogre::SkeletonInstance* skeleton = this->item->getSkeletonInstance();
                this->source = skeleton->getAnimation(animationName);
                this->source->setEnabled(true);
                this->source->mWeight = 1.0f;
                this->source->setTime(0.0f);
                this->timeleft = 0.0f;

                // Apply speed to the new source
                auto itS = this->baseFrameRates.find(animationName);
                if (itS != this->baseFrameRates.end())
                {
                    this->source->mFrameRate = itS->second * this->currentSpeed;
                }
            }
            else
            {
                auto newTarget = this->skeleton->getAnimation(animationName);

                if (this->timeleft > 0.0f)
                {
                    if (newTarget == this->target)
                    {
                        // nothing to do
                    }
                    else if (newTarget == this->source)
                    {
                        // reversing - source and target swap roles, timeleft mirrors.
                        // this->duration is deliberately kept here: mirroring the remaining
                        // time only makes sense against the duration that blend is running
                        // on. Only the transition type is adopted.
                        this->source = this->target;
                        this->target = newTarget;
                        this->timeleft = this->duration - this->timeleft;
                        this->transition = transition;
                        // no speed change needed - both were already at currentSpeed
                    }
                    else
                    {
                        // genuinely new target mid-blend
                        //
                        // Bug: this branch never assigned the incoming 'duration' to
                        // this->duration / this->timeleft - that only happened in the fresh
                        // start branch below. So a blend requested mid-transition silently
                        // inherited the REMAINING time of the previous one: with 0.05 s left
                        // over, a requested 0.5 s cross fade finished in 0.05 s, i.e. as a
                        // hard cut. This matters much more now that callers deliberately
                        // blend early, while the previous transition is still running.
                        //
                        // The progress of the outgoing blend has to be read BEFORE the clock
                        // is replaced, otherwise the incoming animation would pop in at the
                        // wrong weight.
                        const Ogre::Real previousProgress = (this->duration > 0.0f) ? (1.0f - this->timeleft / this->duration) : 1.0f;

                        if (this->timeleft < this->duration * 0.5f)
                        {
                            this->target->setEnabled(false);
                            this->target->mWeight = 0.0f;
                        }
                        else
                        {
                            this->source->setEnabled(false);
                            this->source->mWeight = 0.0f;
                            this->source = this->target;
                        }
                        this->target = newTarget;
                        this->target->setEnabled(true);
                        this->target->mWeight = previousProgress;
                        this->target->setTime(0.0f);

                        // Restart the blend clock on the REQUESTED duration, and adopt the
                        // requested transition type - it too was silently kept from the
                        // previous blend.
                        this->transition = transition;
                        this->timeleft = this->duration = duration;

                        // Apply speed to the new target
                        auto itT = this->baseFrameRates.find(animationName);
                        if (itT != this->baseFrameRates.end())
                        {
                            this->target->mFrameRate = itT->second * this->currentSpeed;
                        }
                    }
                }
                else
                {
                    // Normal fresh blend start
                    this->transition = transition;
                    this->timeleft = this->duration = duration;
                    this->target = newTarget;
                    this->target->setEnabled(true);
                    this->target->mWeight = 0.0f;

                    // Apply speed to the new target
                    auto itT = this->baseFrameRates.find(animationName);
                    if (itT != this->baseFrameRates.end())
                    {
                        this->target->mFrameRate = itT->second * this->currentSpeed;
                    }

                    // Phase-sync for looping locomotion blends.
                    //
                    // Now gated behind phaseSync (set only by blendPhaseSynced()). It used to
                    // apply to EVERY BlendWhileAnimating blend between two looping clips, which
                    // is only correct for genuinely comparable timelines such as walk into run.
                    // For anything else the shared phase is meaningless: blending an idle that
                    // happens to sit at 60% into a pick up animation started the pick up 60% in,
                    // played the remainder, looped, and only then played the gesture properly -
                    // looking like the character started the action twice.
                    if (true == phaseSync && transition == BlendWhileAnimating && this->source->mLoop && loop && this->source->getNumFrames() > 0.0f && this->target->getNumFrames() > 0.0f)
                    {
                        Ogre::Real sourcePhase = this->source->getCurrentFrame() / this->source->getNumFrames();
                        this->target->setFrame(sourcePhase * this->target->getNumFrames());
                    }
                    else
                    {
                        this->target->setTime(0.0f);
                    }
                }
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "AnimationBlenderV2::internalBlend");
    }

    void AnimationBlenderV2::addTime(Ogre::Real time, const Ogre::String& ownerId)
    {
        // Note: If entity is in ragdolling state, animation would explode the ragdoll and since its no possible to avoid, that a developer calls
        // e.g. in a lua script some animation functions, it must be avoided internally
        if (false == this->canAnimate)
        {
            return;
        }

        // If someone else already advanced this blender this frame, skip.
        if (false == this->tryClaimAddTime(ownerId))
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                "[AnimationBlender]: Warning: Add time will not be executed for " + ownerId + ", because another class: " + this->addTimeOwner + " does claim animation blender already.");
            return;
        }

        if (this->source != nullptr)
        {
            auto closureFunction = [this](Ogre::Real renderDt)
            {
                // Guard: if source was disabled by resetAnimation (disconnect),
                // and timeleft somehow got reset but source is still non-null,
                // skip execution to avoid re-enabling a dead animation.
                if (nullptr == this->source || false == this->source->mEnabled)
                {
                    // Only skip if there's no active blend in progress either
                    if (this->timeleft <= 0.0f)
                    {
                        return;
                    }
                }

                bool weightChange = false;

                if (this->timeleft > 0.0f)
                {
                    this->timeleft -= renderDt;
                    if (this->timeleft <= 0.0f)
                    {
                        // Blend finished: promote target to source.
                        this->source->setEnabled(false);
                        this->source->mWeight = 0.0f;
                        this->source = this->target;
                        this->source->setEnabled(true);
                        this->source->mWeight = 1.0f; // BUG FIX (Bug 3): was 0.0f, then corrected
                                                      // in the else branch — fragile and wrong
                                                      // if the completion check fired the same frame.
                        this->target = nullptr;

                        // BUG FIX (BlendThenAnimate): for BlendThenAnimate the target was
                        // deliberately NOT advanced during the blend phase (see fix below),
                        // so it is still at frame 0 and just starts playing naturally. The
                        // old setFrame(0.0f) call here was masking a different bug where the
                        // target WAS being advanced and had to be rewound, causing a visible
                        // jump. Now BlendThenAnimate simply does nothing extra here.
                    }
                    else
                    {
                        // Still blending — advance weights with smoothstep for a natural feel.
                        Ogre::Real t = 1.0f - (this->timeleft / this->duration); // 0->1 as blend progresses
                        Ogre::Real smooth = t * t * (3.0f - 2.0f * t);           // smoothstep formula
                        this->source->mWeight = 1.0f - smooth;
                        this->target->mWeight = smooth;
                        weightChange = true;

                        // BUG FIX (BlendThenAnimate): BlendWhileAnimating advances the target
                        // during the blend so both clips play simultaneously while fading.
                        // BlendThenAnimate keeps the target frozen at frame 0 during the blend
                        // and only starts playing once it is promoted to source (above).
                        // The old code advanced the target for BOTH modes and then rewound it
                        // with setFrame(0.0f) on promotion, producing a visible jump.
                        if (this->transition == AnimationBlenderV2::BlendWhileAnimating)
                        {
                            this->target->addTime(renderDt);
                        }
                    }
                }

                // BUG FIX (Bug 6): setLoop() BEFORE addTime() / addFrame().
                // Ogre's addFrame() reads mLoop internally to decide wrap vs clamp.
                // Calling setLoop() after addTime() meant the current frame used the
                // stale Ogre-default mLoop=true even for non-looping animations.
                this->source->setLoop(this->loop);

                // BUG FIX (Bug 4 + NEW BUG A): Separate loop/non-loop completion paths.
                //
                // Frame-based completion threshold (NEW BUG A):
                // The old fixed 0.05s threshold at 25 fps = 1.25 frames skipped at the end.
                // For short clips this is severe — jump-pose (3 frames, 0.12s) lost 42%!
                // New threshold: half a frame, scales with mFrameRate.
                //   At 25fps: 0.5/25 = 0.02s. At 50fps: 0.5/50 = 0.01s. Always proportional.
                // Guard against division by zero: mFrameRate comes from mOriginalFrameRate
                // (Ogre default 25.0f) and is always > 0 at runtime.
                const Ogre::Real completionThreshold = 0.5f / this->source->mFrameRate;

                if (this->loop)
                {
                    // BUG FIX (Bug 4): Looping animations NEVER complete and NEVER fire
                    // notifyObservers(). Ogre's addFrame() wraps the time automatically
                    // because setLoop(true) was called above. The old code fired
                    // notifyObservers() on every single loop cycle and toggled the complete
                    // flag true/false each cycle, causing spurious animation-finished callbacks.
                    this->complete = false;
                    if (false == weightChange)
                    {
                        this->source->mWeight = 1.0f;
                    }
                }
                else
                {
                    if (this->source->getCurrentTime() >= this->source->getDuration() - completionThreshold)
                    {
                        this->complete = true;
                        if (nullptr != this->previousSource)
                        {
                            this->source->setEnabled(false);
                            this->source->mWeight = 0.0f;

                            this->source = this->previousSource;
                            this->loop = this->previousLoop;
                            this->transition = this->previousTransition;
                            this->duration = this->previousDuration;
                            this->previousSource = nullptr;
                            this->target = nullptr;
                            this->timeleft = 0.0f;
                            this->complete = false;

                            this->source->setEnabled(true);
                            this->source->mWeight = 1.0f;
                            this->source->setTime(0.0f);
                            this->source->setLoop(this->loop);

                            // Reapply current speed — previousSource may not have been touched since last speed change
                            auto it = this->baseFrameRates.find(this->source->getName().getFriendlyText());
                            if (it != this->baseFrameRates.end())
                            {
                                this->source->mFrameRate = it->second * this->currentSpeed;
                            }
                        }

                        // Notify once per completion. Never fires for loop=true (see above).
                        this->notifyObservers();
                    }
                    else
                    {
                        this->complete = false;
                        if (false == weightChange)
                        {
                            this->source->mWeight = 1.0f;
                        }
                    }
                }

                // Advance the source. For looping animations Ogre's addFrame() wraps
                // automatically because setLoop(true) was called before this point.
                this->source->addTime(renderDt);

                // BUG FIX (overlay never ran): the overlay driving code used to sit INSIDE the
                // non looping completion branch above, nested in 'if (nullptr != previousSource)'.
                // It therefore only ever ran on the single frame on which a NON LOOPING main clip
                // reached its end while a blendAndContinue was pending. For a walking character,
                // whose main clip loops, that branch is never entered at all, so the overlay was
                // enabled with weight 0, had its bones zeroed on every other animation by
                // setOverrideBoneWeightsOnAllAnimations(0.0f, true), and was then never advanced:
                // the masked bones simply froze in the bind pose and the overlay never cleared
                // itself again. An overlay is a layer of its own - it has to be driven on EVERY
                // frame, independently of what the main animation is doing.
                this->internalUpdateOverlay(renderDt);

                if (true == this->debugLog)
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "Source Animation: " + this->source->getName().getFriendlyText() + " timePosition: " + Ogre::StringConverter::toString(this->source->getCurrentTime()) +
                                                                                            " length: " + Ogre::StringConverter::toString(this->source->getDuration()) + " complete: " + Ogre::StringConverter::toString(this->complete) +
                                                                                            " weight: " + Ogre::StringConverter::toString(this->source->mWeight));
                    if (target != nullptr)
                    {
                        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "Target Animation: " + this->target->getName().getFriendlyText() + " timePosition: " + Ogre::StringConverter::toString(this->target->getCurrentTime()) +
                                                                                                " length: " + Ogre::StringConverter::toString(this->target->getDuration()) + " weight: " + Ogre::StringConverter::toString(this->target->mWeight));
                    }
                }
            };
            Ogre::String id = "AnimationBlenderV2::addTime" + Ogre::StringConverter::toString(this->uniqueId);
            NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
        }
    }

    void AnimationBlenderV2::blendAndContinue(AnimID animationId, Ogre::Real duration)
    {
        auto it2 = this->mappedAnimations.find(animationId);
        if (this->mappedAnimations.end() == it2)
        {
            // if the animation cannot be found, just skip
            return;
        }

        if (true == this->isTargetAnimationActive(animationId))
        {
            return;
        }

        // Remember the previous source, to continue with this one later
        this->complete = true;
        this->previousSource = this->source;
        this->previousLoop = this->loop;
        this->previousTransition = this->transition;
        this->previousDuration = this->duration;

        this->internalBlend(it2->second, BlendingTransition::BlendSwitch, duration, false);
        this->complete = false;
    }

    void AnimationBlenderV2::blendAndContinue(const Ogre::String& animationName, Ogre::Real duration)
    {
        if (true == animationName.empty())
        {
            return;
        }

        if (true == this->isTargetAnimationActive(this->getAnimationIdFromString(animationName)))
        {
            return;
        }

        // Remember the previous source, to continue with this one later
        this->complete = true;
        this->previousSource = this->source;
        this->previousLoop = this->loop;
        this->previousTransition = this->transition;
        this->previousDuration = this->duration;

        this->internalBlend(animationName, BlendingTransition::BlendSwitch, duration, false);
        this->complete = false;
    }

    void AnimationBlenderV2::blendAndContinue(AnimID animationId)
    {
        auto it2 = this->mappedAnimations.find(animationId);
        if (this->mappedAnimations.end() == it2)
        {
            // if the animation cannot be found, just skip
            return;
        }

        // Only play if not active
        if (true == this->isTargetAnimationActive(animationId))
        {
            if (true == this->debugLog)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "Target animation active: " + this->target->getName().getFriendlyText());
            }
            return;
        }
        if (true == this->isAnimationActive(animationId))
        {
            if (true == this->debugLog)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "Source animation active: " + this->source->getName().getFriendlyText());
            }
            return;
        }

        // Remember the previous source, to continue with this one later
        this->complete = true;
        this->previousSource = this->source;
        this->previousLoop = this->loop;
        this->previousTransition = this->transition;
        this->previousDuration = this->duration;

        this->internalBlend(it2->second, BlendingTransition::BlendWhileAnimating, 0.2f, false);
        this->complete = false;
    }

    void AnimationBlenderV2::blendAndContinue(const Ogre::String& animationName)
    {
        if (true == animationName.empty())
        {
            return;
        }

        // Only play if not active
        if (true == this->isTargetAnimationActive(this->getAnimationIdFromString(animationName)))
        {
            return;
        }
        if (true == this->isAnimationActive(this->getAnimationIdFromString(animationName)))
        {
            return;
        }

        // Remember the previous source, to continue with this one later
        this->complete = true;
        this->previousSource = this->source;
        this->previousLoop = this->loop;
        this->previousTransition = this->transition;
        this->previousDuration = this->duration;

        this->internalBlend(animationName, BlendingTransition::BlendSwitch, 0.2f, false);
        this->complete = false;
    }

    void AnimationBlenderV2::setOverlayAnimation(AnimID animationId, Ogre::Real blendInTime)
    {
        auto it = this->mappedAnimations.find(animationId);
        if (it == this->mappedAnimations.end())
        {
            return;
        }
        this->internalSetOverlayAnimation(it->second, "", blendInTime, false);
    }

    void AnimationBlenderV2::setOverlayAnimation(const Ogre::String& animationName, Ogre::Real blendInTime)
    {
        if (animationName.empty())
        {
            return;
        }
        this->internalSetOverlayAnimation(animationName, "", blendInTime, false);
    }

    void AnimationBlenderV2::setOverlayAnimation(AnimID animationId, Ogre::Real blendInTime, bool loop)
    {
        auto it = this->mappedAnimations.find(animationId);
        if (it == this->mappedAnimations.end())
        {
            return;
        }
        this->internalSetOverlayAnimation(it->second, "", blendInTime, loop);
    }

    void AnimationBlenderV2::setOverlayAnimation(const Ogre::String& animationName, Ogre::Real blendInTime, bool loop)
    {
        if (animationName.empty())
        {
            return;
        }
        this->internalSetOverlayAnimation(animationName, "", blendInTime, loop);
    }

    void AnimationBlenderV2::setOverlayAnimationForBoneChain(AnimID animationId, const Ogre::String& rootBoneName, Ogre::Real blendInTime, bool loop)
    {
        auto it = this->mappedAnimations.find(animationId);
        if (it == this->mappedAnimations.end())
        {
            return;
        }
        this->internalSetOverlayAnimation(it->second, rootBoneName, blendInTime, loop);
    }

    void AnimationBlenderV2::setOverlayAnimationForBoneChain(const Ogre::String& animationName, const Ogre::String& rootBoneName, Ogre::Real blendInTime, bool loop)
    {
        if (animationName.empty())
        {
            return;
        }
        this->internalSetOverlayAnimation(animationName, rootBoneName, blendInTime, loop);
    }

    void AnimationBlenderV2::internalCollectBoneChain(Ogre::Bone* bone, std::vector<Ogre::String>& outBoneNames)
    {
        if (nullptr == bone)
        {
            return;
        }

        outBoneNames.emplace_back(bone->getName());

        for (Ogre::Bone* child : bone->getChildren())
        {
            this->internalCollectBoneChain(child, outBoneNames);
        }
    }

    void AnimationBlenderV2::internalSetOverlayOwnBoneWeights(bool restore)
    {
        // The overlay clip itself: full weight on the chain it owns, zero everywhere else, so a
        // full body punch cannot drive the legs. On restore everything goes back to 1, otherwise
        // the clip would stay crippled the next time it is used as a normal animation.
        //
        // Attention: RENDER THREAD only.
        if (nullptr == this->overlaySource)
        {
            return;
        }

        if (true == this->overlayChainBoneIds.empty())
        {
            return;
        }

        for (const Ogre::IdString& boneId : this->overlayChainBoneIds)
        {
            this->overlaySource->setBoneWeight(boneId, restore ? 1.0f : this->overlayChainInfluence);
        }

        for (const Ogre::IdString& boneId : this->overlayOtherBoneIds)
        {
            this->overlaySource->setBoneWeight(boneId, restore ? 1.0f : this->overlayOutsideInfluence);
        }
    }

    void AnimationBlenderV2::internalDriveOverlayChainWeight(Ogre::Real overlayAuthority)
    {
        // Hands the chain over to the overlay and back again, CONTINUOUSLY.
        //
        // Attention: this has to follow the overlay's own weight every frame, it cannot be a
        // one off switch. Ogre-Next accumulates every enabled animation onto the same bone with
        // 'animation weight * bone weight'. If the others are muted the moment the overlay starts,
        // the chain is driven by nothing but an overlay that is still fading in from 0 - the upper
        // body sags towards the bind pose at the start of every swing. And if the others are handed
        // the chain back the moment the clip ends, the chain is driven TWICE for the length of the
        // fade out, by the locomotion clip at full weight and by the punch at almost full weight.
        // That is what made the idle look chopped up and threw the character backwards on every
        // punch. The two weights have to add up to one at all times.
        //
        // Only the animations that can actually be enabled are touched - source, target and the
        // previous source. Sweeping all 38 clips of a character every frame would cost far more
        // than it buys, and every clip that IS touched is remembered so it can be restored.
        //
        // Attention: RENDER THREAD only.
        if (true == this->overlayChainBoneIds.empty())
        {
            return;
        }

        // The overlay's own bone weight already carries the influence, so what is left for the
        // others is whatever the overlay does not take. Both always add up to one.
        Ogre::Real chainWeightForOthers = 1.0f - overlayAuthority * this->overlayChainInfluence;
        if (chainWeightForOthers < 0.0f)
        {
            chainWeightForOthers = 0.0f;
        }
        else if (chainWeightForOthers > 1.0f)
        {
            chainWeightForOthers = 1.0f;
        }

        Ogre::Real outsideWeightForOthers = 1.0f - overlayAuthority * this->overlayOutsideInfluence;
        if (outsideWeightForOthers < 0.0f)
        {
            outsideWeightForOthers = 0.0f;
        }
        else if (outsideWeightForOthers > 1.0f)
        {
            outsideWeightForOthers = 1.0f;
        }

        // Only worth walking the second, much longer list when the overlay actually reaches
        // outside its chain.
        const bool driveOutsideBones = (this->overlayOutsideInfluence > 0.0f);

        Ogre::SkeletonAnimation* candidates[3] = {this->source, this->target, this->previousSource};

        for (Ogre::SkeletonAnimation* animation : candidates)
        {
            if (nullptr == animation || animation == this->overlaySource)
            {
                continue;
            }

            for (const Ogre::IdString& boneId : this->overlayChainBoneIds)
            {
                animation->setBoneWeight(boneId, chainWeightForOthers);
            }

            if (true == driveOutsideBones)
            {
                for (const Ogre::IdString& boneId : this->overlayOtherBoneIds)
                {
                    animation->setBoneWeight(boneId, outsideWeightForOthers);
                }
            }

            if (std::find(this->overlayMutedAnimations.cbegin(), this->overlayMutedAnimations.cend(), animation) == this->overlayMutedAnimations.cend())
            {
                this->overlayMutedAnimations.push_back(animation);
            }
        }
    }

    void AnimationBlenderV2::internalRestoreOverlayChainWeight(void)
    {
        // Everything that was ever muted for this overlay gets its bones back. An animation that
        // was the source two seconds ago and has been blended away since would otherwise keep a
        // muted spine forever and come back crippled the next time it is blended in.
        //
        // Attention: RENDER THREAD only.
        for (Ogre::SkeletonAnimation* animation : this->overlayMutedAnimations)
        {
            if (nullptr == animation)
            {
                continue;
            }

            for (const Ogre::IdString& boneId : this->overlayChainBoneIds)
            {
                animation->setBoneWeight(boneId, 1.0f);
            }

            for (const Ogre::IdString& boneId : this->overlayOtherBoneIds)
            {
                animation->setBoneWeight(boneId, 1.0f);
            }
        }
        this->overlayMutedAnimations.clear();
    }

    void AnimationBlenderV2::internalSetOverlayAnimation(const Ogre::String& animationName, const Ogre::String& maskRootBoneName, Ogre::Real blendInTime, bool loop)
    {
        if (false == this->canAnimate)
        {
            return;
        }

        if (nullptr == this->skeleton)
        {
            return;
        }

        if (false == this->skeleton->hasAnimation(animationName))
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AnimationBlenderV2] setOverlayAnimation: animation '" + animationName + "' not found.");
            return;
        }

        if (false == maskRootBoneName.empty() && false == this->skeleton->hasBone(maskRootBoneName))
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AnimationBlenderV2] setOverlayAnimation: bone '" + maskRootBoneName + "' not found, falling back to the animated bones of the clip itself.");
        }

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, animationName, maskRootBoneName, blendInTime, loop]()
        {
            // Clean up any existing overlay first. The bones it had taken away have to go back
            // BEFORE the mask of the new overlay is computed, otherwise a chain that the old
            // overlay muted and the new one does not touch would stay muted forever.
            if (nullptr != this->overlaySource)
            {
                this->internalRestoreOverlayChainWeight();
                this->internalSetOverlayOwnBoneWeights(true);
                this->overlaySource->setEnabled(false);
                this->overlaySource->mWeight = 0.0f;
                this->overlaySource = nullptr;
            }
            this->overlayMaskBoneNames.clear();
            this->overlayAllBoneNames.clear();
            this->overlayChainBoneIds.clear();
            this->overlayOtherBoneIds.clear();

            this->overlaySource = this->skeleton->getAnimation(animationName);
            this->overlaySource->setEnabled(true);
            this->overlaySource->mWeight = 0.0f; // will be driven by addTime
            this->overlaySource->setTime(0.0f);
            this->overlaySource->setLoop(loop);

            // BUG FIX (swing took two to three times as long as the clip): the overlay used to
            // inherit currentSpeed, the LOCOMOTION speed multiplier. The player controller drives
            // that from the walking speed to keep the feet in sync with the movement, so it sits
            // well below 1.0 most of the time - at 0.35 a 0.87 second punch took two and a half
            // seconds. An upper body action has nothing to do with how fast the legs move, so the
            // overlay gets a speed of its own, 1.0 by default.
            auto it = this->baseFrameRates.find(animationName);
            if (it != this->baseFrameRates.end())
            {
                this->overlaySource->mFrameRate = it->second * this->overlaySpeed;
            }

            if (false == maskRootBoneName.empty() && true == this->skeleton->hasBone(maskRootBoneName))
            {
                Ogre::Bone* chainRoot = this->skeleton->getBone(maskRootBoneName);

                // The chain the overlay owns.
                this->internalCollectBoneChain(chainRoot, this->overlayMaskBoneNames);

                // And the whole skeleton, reached by climbing to the topmost ancestor of that
                // chain and walking down from there. Every bone outside the chain has to be muted
                // on the overlay, otherwise the punch drives the legs too.
                Ogre::Bone* skeletonRoot = chainRoot;
                while (nullptr != skeletonRoot->getParent())
                {
                    skeletonRoot = skeletonRoot->getParent();
                }
                this->internalCollectBoneChain(skeletonRoot, this->overlayAllBoneNames);

                // The ids are hashed ONCE here. Doing it per frame would mean hashing every bone
                // name of the chain on every one of the 38 clips, every frame.
                for (const Ogre::String& boneName : this->overlayAllBoneNames)
                {
                    const bool isInChain = (std::find(this->overlayMaskBoneNames.cbegin(), this->overlayMaskBoneNames.cend(), boneName) != this->overlayMaskBoneNames.cend());

                    if (true == isInChain)
                    {
                        this->overlayChainBoneIds.emplace_back(Ogre::IdString(boneName));
                    }
                    else
                    {
                        this->overlayOtherBoneIds.emplace_back(Ogre::IdString(boneName));
                    }
                }
            }

            if (true == this->overlayChainBoneIds.empty())
            {
                // No usable chain: fall back to "whatever bones this clip animates".
                //
                // Attention: that is only the upper body if the overlay clip really only keys the
                // upper body. A full body mocap punch keys the legs and the root as well, and then
                // this takes the ENTIRE skeleton away from the locomotion clip - which looks
                // exactly like the old full body attack.
                this->overlaySource->setOverrideBoneWeightsOnAllAnimations(0.0f, true);
            }
            else
            {
                this->internalSetOverlayOwnBoneWeights(false);
                // Starts at 0: the overlay has no authority yet, it is still at weight 0. The
                // handover follows its weight, frame by frame, in internalUpdateOverlay().
                this->internalDriveOverlayChainWeight(0.0f);
            }

            if (true == this->debugLog)
            {
                Ogre::String message = "[AnimationBlenderV2] Overlay '" + animationName + "' length: " + Ogre::StringConverter::toString(this->overlaySource->getDuration()) +
                                       "s frameRate: " + Ogre::StringConverter::toString(this->overlaySource->mFrameRate) + " overlaySpeed: " + Ogre::StringConverter::toString(this->overlaySpeed) + " loop: " + Ogre::StringConverter::toString(loop);

                if (true == this->overlayMaskBoneNames.empty())
                {
                    message += " mask: NONE (the clip's own bones are used - a full body clip therefore takes the WHOLE skeleton)";
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, message);
                }
                else
                {
                    message += " mask: " + Ogre::StringConverter::toString(static_cast<int>(this->overlayMaskBoneNames.size())) + " of " + Ogre::StringConverter::toString(static_cast<int>(this->overlayAllBoneNames.size())) + " bones, starting at '" +
                               maskRootBoneName + "'";
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, message);

                    // The whole hierarchy, indented, with a marker on every bone the overlay owns.
                    // This is how a wrong or too high a chain root is spotted: if a leg bone comes
                    // out marked, the player punches with his legs.
                    Ogre::Bone* skeletonRoot = this->skeleton->getBone(maskRootBoneName);
                    while (nullptr != skeletonRoot->getParent())
                    {
                        skeletonRoot = skeletonRoot->getParent();
                    }
                    this->internalLogBoneChain(skeletonRoot, "  ");
                }
            }

            this->overlayLoop = loop;
            this->overlayDuration = (blendInTime > 0.0f) ? blendInTime : 0.001f;
            this->overlayTimeleft = this->overlayDuration;
            this->overlayBlendingOut = false;

            // A non looping overlay fades itself out again symmetrically when the clip is over,
            // instead of popping back to the locomotion pose in a single frame.
            this->overlayBlendOutTime = (blendInTime > 0.0f) ? blendInTime : 0.001f;
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "AnimationBlenderV2::setOverlayAnimation");
    }

    void AnimationBlenderV2::internalUpdateOverlay(Ogre::Real renderDt)
    {
        // Attention: called from inside the addTime closure, so this already runs on the render
        // thread - no render command here, that would deadlock.
        if (nullptr == this->overlaySource)
        {
            return;
        }

        // Same frame based threshold as the main source, so a short clip is not cut off.
        const Ogre::Real overlayThreshold = 0.5f / this->overlaySource->mFrameRate;

        if (false == this->overlayBlendingOut)
        {
            // Blend in phase: weight 0 -> 1
            if (this->overlayTimeleft > 0.0f)
            {
                this->overlayTimeleft -= renderDt;
                this->overlaySource->mWeight = (this->overlayTimeleft <= 0.0f) ? 1.0f : 1.0f - (this->overlayTimeleft / this->overlayDuration);
            }
            else
            {
                this->overlaySource->mWeight = 1.0f;
            }

            this->internalDriveOverlayChainWeight(this->overlaySource->mWeight);

            this->overlaySource->addTime(renderDt);

            if (true == this->debugLog)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                    "[AnimationBlenderV2] Overlay: " + this->overlaySource->getName().getFriendlyText() + " timePosition: " + Ogre::StringConverter::toString(this->overlaySource->getCurrentTime()) +
                        " length: " + Ogre::StringConverter::toString(this->overlaySource->getDuration()) + " weight: " + Ogre::StringConverter::toString(this->overlaySource->mWeight) + " main: " +
                        (nullptr != this->source ? this->source->getName().getFriendlyText() : Ogre::String("none")) + " mainWeight: " + (nullptr != this->source ? Ogre::StringConverter::toString(this->source->mWeight) : Ogre::String("0")));
            }

            // A non looping overlay ends itself. A looping one only ever ends on
            // clearOverlayAnimation().
            if (false == this->overlayLoop && this->overlaySource->getCurrentTime() >= this->overlaySource->getDuration() - overlayThreshold)
            {
                // No bone weights are touched here. The handover happens gradually in the fade out
                // branch below, in lock step with the overlay's own weight.
                this->overlayBlendingOut = true;
                this->overlayDuration = this->overlayBlendOutTime;
                this->overlayTimeleft = this->overlayBlendOutTime;
            }
        }
        else
        {
            // Blend out phase: weight 1 -> 0
            this->overlayTimeleft -= renderDt;

            if (this->overlayTimeleft <= 0.0f)
            {
                this->internalRestoreOverlayChainWeight();
                this->internalSetOverlayOwnBoneWeights(true);

                if (true == this->overlayChainBoneIds.empty())
                {
                    this->overlaySource->setOverrideBoneWeightsOnAllAnimations(1.0f, true);
                }

                this->overlaySource->setEnabled(false);
                this->overlaySource->mWeight = 0.0f;
                this->overlaySource = nullptr;
                this->overlayMaskBoneNames.clear();
                this->overlayAllBoneNames.clear();
                this->overlayChainBoneIds.clear();
                this->overlayOtherBoneIds.clear();
                this->overlayBlendingOut = false;
                this->overlayTimeleft = 0.0f;
            }
            else
            {
                this->overlaySource->mWeight = this->overlayTimeleft / this->overlayDuration;
                this->internalDriveOverlayChainWeight(this->overlaySource->mWeight);
                this->overlaySource->addTime(renderDt);
            }
        }
    }

    void AnimationBlenderV2::clearOverlayAnimation(Ogre::Real blendOutTime)
    {
        NOWA::GraphicsModule::RenderCommand renderCommand = [this, blendOutTime]()
        {
            if (nullptr == this->overlaySource)
            {
                return;
            }

            if (true == this->overlayBlendingOut)
            {
                // Already on its way out, do not restart the fade.
                return;
            }

            // No bone weights are touched here either - the handover follows the fade out in
            // internalUpdateOverlay(), so the chain is never driven by two clips at once.
            this->overlayBlendingOut = true;
            this->overlayDuration = (blendOutTime > 0.0f) ? blendOutTime : 0.001f;
            this->overlayTimeleft = this->overlayDuration;
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "AnimationBlenderV2::clearOverlayAnimation");
    }

    Ogre::Real AnimationBlenderV2::getOverlayTimePosition(void) const
    {
        if (nullptr != this->overlaySource)
        {
            return this->overlaySource->getCurrentTime();
        }
        return 0.0f;
    }

    Ogre::Real AnimationBlenderV2::getOverlayLength(void) const
    {
        if (nullptr != this->overlaySource)
        {
            return this->overlaySource->getDuration();
        }
        return 0.0f;
    }

    Ogre::Real AnimationBlenderV2::getOverlayProgress(void) const
    {
        // 0 at the first frame of the overlay clip, 1 at its last one. This is what a script
        // times a hit window or a follow up window on - getTimePosition() cannot be used for
        // that, it reports the MAIN animation, which is the walk cycle while the overlay runs.
        if (nullptr == this->overlaySource)
        {
            return 0.0f;
        }

        const Ogre::Real length = this->overlaySource->getDuration();
        if (length <= 0.0f)
        {
            return 0.0f;
        }

        Ogre::Real progress = this->overlaySource->getCurrentTime() / length;
        if (progress < 0.0f)
        {
            progress = 0.0f;
        }
        else if (progress > 1.0f)
        {
            progress = 1.0f;
        }
        return progress;
    }

    void AnimationBlenderV2::setOverlayTimePosition(Ogre::Real timePosition)
    {
        if (nullptr == this->overlaySource)
        {
            return;
        }

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, timePosition]()
        {
            if (nullptr != this->overlaySource)
            {
                this->overlaySource->setTime(timePosition);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "AnimationBlenderV2::setOverlayTimePosition");
    }

    bool AnimationBlenderV2::isOverlayBlendingOut(void) const
    {
        return nullptr != this->overlaySource && true == this->overlayBlendingOut;
    }

    void AnimationBlenderV2::setOverlayInfluence(Ogre::Real chainInfluence, Ogre::Real outsideChainInfluence)
    {
        this->overlayChainInfluence = (chainInfluence < 0.0f) ? 0.0f : ((chainInfluence > 1.0f) ? 1.0f : chainInfluence);
        this->overlayOutsideInfluence = (outsideChainInfluence < 0.0f) ? 0.0f : ((outsideChainInfluence > 1.0f) ? 1.0f : outsideChainInfluence);

        // Takes effect immediately on a running overlay, so the values can be tried out live.
        NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
        {
            if (nullptr != this->overlaySource && false == this->overlayChainBoneIds.empty())
            {
                this->internalSetOverlayOwnBoneWeights(false);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "AnimationBlenderV2::setOverlayInfluence");
    }

    Ogre::Real AnimationBlenderV2::getOverlayChainInfluence(void) const
    {
        return this->overlayChainInfluence;
    }

    Ogre::Real AnimationBlenderV2::getOverlayOutsideInfluence(void) const
    {
        return this->overlayOutsideInfluence;
    }

    void AnimationBlenderV2::setOverlaySpeed(Ogre::Real speed)
    {
        this->overlaySpeed = (speed > 0.0f) ? speed : 1.0f;

        NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
        {
            if (nullptr != this->overlaySource)
            {
                auto it = this->baseFrameRates.find(this->overlaySource->getName().getFriendlyText());
                if (it != this->baseFrameRates.end())
                {
                    this->overlaySource->mFrameRate = it->second * this->overlaySpeed;
                }
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "AnimationBlenderV2::setOverlaySpeed");
    }

    Ogre::Real AnimationBlenderV2::getOverlaySpeed(void) const
    {
        return this->overlaySpeed;
    }

    void AnimationBlenderV2::internalLogBoneChain(Ogre::Bone* bone, const Ogre::String& padding)
    {
        if (nullptr == bone)
        {
            return;
        }

        const bool isInChain = (std::find(this->overlayMaskBoneNames.cbegin(), this->overlayMaskBoneNames.cend(), bone->getName()) != this->overlayMaskBoneNames.cend());

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AnimationBlenderV2] " + padding + bone->getName() + (isInChain ? "   <- OVERLAY" : ""));

        for (Ogre::Bone* child : bone->getChildren())
        {
            this->internalLogBoneChain(child, padding + "  ");
        }
    }

    bool AnimationBlenderV2::isOverlayAnimationActive(void) const
    {
        return nullptr != this->overlaySource;
    }

    void AnimationBlenderV2::driveBlendSpace(Ogre::Real parameter, const AnimationBlenderV2::BlendSpaceEntryList& entryList)
    {
        const auto& entries = entryList.getEntries();
        if (entries.size() < 2 || false == this->canAnimate)
        {
            return;
        }

        // Clamp parameter to the defined range
        Ogre::Real clampedParam = std::max(entries.front().parameter, std::min(entries.back().parameter, parameter));

        // Find the two bracketing entries
        size_t upperIdx = 1;
        while (upperIdx < entries.size() - 1 && entries[upperIdx].parameter < clampedParam)
        {
            ++upperIdx;
        }

        const BlendSpaceEntry& lower = entries[upperIdx - 1];
        const BlendSpaceEntry& upper = entries[upperIdx];

        // Compute normalized blend factor between the two clips
        Ogre::Real range = upper.parameter - lower.parameter;
        Ogre::Real t = (range > 0.0f) ? (clampedParam - lower.parameter) / range : 0.0f;

        // Apply smoothstep for natural feel
        t = t * t * (3.0f - 2.0f * t);

        // Get animation names
        auto itLower = this->mappedAnimations.find(lower.animationId);
        auto itUpper = this->mappedAnimations.find(upper.animationId);
        if (itLower == this->mappedAnimations.end() || itUpper == this->mappedAnimations.end())
        {
            return;
        }

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, itLowerSecond = itLower->second, itUpperSecond = itUpper->second, t]()
        {
            // Disable everything first
            for (auto& anim : this->skeleton->getAnimationsNonConst())
            {
                if (anim.getName().getFriendlyText() != itLowerSecond && anim.getName().getFriendlyText() != itUpperSecond)
                {
                    anim.setEnabled(false);
                    anim.mWeight = 0.0f;
                }
            }

            auto* lowerAnim = this->skeleton->getAnimation(itLowerSecond);
            auto* upperAnim = this->skeleton->getAnimation(itUpperSecond);

            lowerAnim->setEnabled(true);
            lowerAnim->mWeight = 1.0f - t;

            upperAnim->setEnabled(true);
            upperAnim->mWeight = t;

            // Keep source pointer on the dominant clip for getLength() etc.
            this->source = (t < 0.5f) ? lowerAnim : upperAnim;
            this->target = nullptr;
            this->timeleft = 0.0f;
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "AnimationBlenderV2::driveBlendSpace");
    }

    Ogre::Real AnimationBlenderV2::getProgress()
    {
        if (this->duration <= 0.0f)
        {
            return 0.0f;
        }
        return this->timeleft / this->duration;
    }

    Ogre::SkeletonAnimation* AnimationBlenderV2::getSource()
    {
        return this->source;
    }

    Ogre::SkeletonAnimation* AnimationBlenderV2::getTarget()
    {
        return this->target;
    }

    bool AnimationBlenderV2::isComplete(void) const
    {
        return this->complete;
    }

    void AnimationBlenderV2::registerAnimation(AnimID animationId, const Ogre::String& animationName)
    {
        this->mappedAnimations.insert(std::make_pair(animationId, animationName));
    }

    AnimationBlenderV2::AnimID AnimationBlenderV2::getAnimationIdFromString(const Ogre::String& animationName)
    {
        for (auto& it = this->mappedAnimations.cbegin(); it != this->mappedAnimations.cend(); ++it)
        {
            if (it->second == animationName)
            {
                return it->first;
            }
        }
        return AnimID::ANIM_NONE;
    }

    void AnimationBlenderV2::clearAnimations(void)
    {
        this->mappedAnimations.clear();
    }

    bool AnimationBlenderV2::hasAnimation(const Ogre::String& animationName)
    {
        AnimID animId = this->getAnimationIdFromString(animationName);
        return this->hasAnimation(animId);
    }

    bool AnimationBlenderV2::hasAnimation(AnimID animationId)
    {
        return this->mappedAnimations.find(animationId) != this->mappedAnimations.end();
    }

    bool AnimationBlenderV2::isAnimationActive(AnimID animationId)
    {
        if (nullptr == this->skeleton)
        {
            return false;
        }

        auto it = this->mappedAnimations.find(animationId);
        if (it == this->mappedAnimations.end())
        {
            return false;
        }

        // Check Ogre's actual active list, not just our bookkeeping
        const Ogre::IdString nameId(it->second);
        for (const Ogre::SkeletonAnimation* anim : this->skeleton->getActiveAnimations())
        {
            if (anim->getName() == nameId)
            {
                return true;
            }
        }
        return false;
    }

    bool AnimationBlenderV2::isAnyAnimationActive(void)
    {
        if (nullptr == this->source)
        {
            return false;
        }
        if (this->source->getEnabled())
        {
            return true;
        }
        if (nullptr != this->target && this->target->getEnabled())
        {
            return true;
        }
        return false;
    }

    bool AnimationBlenderV2::isTargetAnimationActive(AnimID animationId)
    {
        bool active = false;
        if (true == this->mappedAnimations.empty())
        {
            return active;
        }

        if (nullptr == this->target)
        {
            return active;
        }

        auto it = this->mappedAnimations.find(animationId);
        if (it != this->mappedAnimations.end())
        {
            active = this->target->getName().getFriendlyText() == it->second;
        }
        return active;
    }

    void AnimationBlenderV2::gameObjectIsInRagDollStateDelegate(EventDataPtr eventData)
    {
        boost::shared_ptr<EventDataGameObjectIsInRagDollingState> castEventData = boost::static_pointer_cast<NOWA::EventDataGameObjectIsInRagDollingState>(eventData);
        Ogre::Item* item = this->item;

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, castEventData, item]()
        {
            if (nullptr != item)
            {
                // if this game object has an physics rag doll component and its in ragdolling state, no animation must be processed, else the skeleton will throw apart!
                unsigned long id = castEventData->getGameObjectId();
                NOWA::GameObject* gameObject = Ogre::any_cast<NOWA::GameObject*>(item->getUserObjectBindings().getUserAny());
                if (nullptr != gameObject)
                {
                    if (gameObject->getId() == id)
                    {
                        this->canAnimate = !castEventData->getIsInRagDollingState();
                    }
                }
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "AnimationBlenderV2::gameObjectIsInRagDollStateDelegate");
    }

    Ogre::SkeletonAnimation* AnimationBlenderV2::getAnimationState(AnimID animationId)
    {
        auto it = this->mappedAnimations.find(animationId);
        if (it != this->mappedAnimations.cend())
        {
            return this->internalGetAnimationState(it->second);
        }

        return nullptr;
    }

    Ogre::SkeletonAnimation* AnimationBlenderV2::getAnimationState(const Ogre::String& animationName)
    {
        return this->internalGetAnimationState(animationName);
    }

    Ogre::SkeletonAnimation* AnimationBlenderV2::internalGetAnimationState(const Ogre::String& animationName)
    {
        Ogre::SkeletonAnimation* animationState = nullptr;
        // Attention: Is this check necessary?
        if (nullptr != this->skeleton && true == this->skeleton->hasAnimation(animationName))
        {
            animationState = this->skeleton->getAnimation(animationName);
        }

        return animationState;
    }

    void AnimationBlenderV2::dumpAllAnimations(Ogre::Node* node, Ogre::String padding)
    {
        // create the scene tree

        Ogre::SceneNode::ObjectIterator objectIt = ((Ogre::SceneNode*)node)->getAttachedObjectIterator();
        auto nodeIt = node->getChildIterator();

        while (objectIt.hasMoreElements())
        {
            // go through all scenenodes in the scene
            Ogre::MovableObject* movableObject = objectIt.getNext();
            Ogre::Item* item = dynamic_cast<Ogre::Item*>(movableObject);
            if (nullptr != item)
            {
                if (true == item->hasSkeleton())
                {
                    /*Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, padding + "List all animations for mesh '" + item->getMesh()->getName() + "':");
                    Ogre::SkeletonInstance* skeleton = item->getSkeletonInstance();
                    if (nullptr != skeleton)
                    {
                        for (const auto& anim : skeleton->getAnimations())
                        {
                            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                                "[AnimationBlenderV2] Animation name: " + anim.getName().getFriendlyText() + " length: " + Ogre::StringConverter::toString(anim.getDuration()) + " seconds");
                        }
                    }*/
                }
            }
        }
        while (nodeIt.hasMoreElements())
        {
            // go through all objects recursive that are attached to the scenenodes
            AnimationBlenderV2::dumpAllAnimations(nodeIt.getNext(), "  " + padding);
        }
    }

    void AnimationBlenderV2::setTimePosition(Ogre::Real timePosition)
    {
        if (nullptr != this->source)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this, timePosition]()
            {
                this->source->setTime(timePosition);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "AnimationBlenderV2::setTimePosition");
        }
    }

    Ogre::Real AnimationBlenderV2::getTimePosition(void) const
    {
        // BUG FIX (Bug 8): removed dead-store local variable 'ts' that called
        // getCurrentTime() twice without using the first result.
        if (nullptr != this->source)
        {
            return this->source->getCurrentTime();
        }
        return 0.0f;
    }

    Ogre::Real AnimationBlenderV2::getLength(void) const
    {
        if (nullptr != this->source)
        {
            return this->source->getDuration();
        }
        return 0.0f;
    }

    void AnimationBlenderV2::setWeight(Ogre::Real weight)
    {
        if (nullptr != this->source)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this, weight]()
            {
                this->source->mWeight = weight;
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "AnimationBlenderV2::setWeight");
        }
    }

    Ogre::Real AnimationBlenderV2::getWeight(void) const
    {
        if (nullptr != this->source)
        {
            return this->source->mWeight;
        }
        return 0.0f;
    }

    Ogre::Bone* AnimationBlenderV2::getBone(const Ogre::String& boneName)
    {
        Ogre::Bone* bone = nullptr;
        if (nullptr != this->skeleton)
        {
            if (true == this->skeleton->hasBone(boneName))
            {
                bone = this->skeleton->getBone(boneName);
                return bone;
            }
        }
        return bone;
    }

    void AnimationBlenderV2::resetBones(void)
    {
        // Note: If entity is in ragdolling state, animation would explode the ragdoll and since its no possible to avoid, that a developer calls
        // e.g. in a lua script some animation functions, it must be avoided internally
        if (false == this->canAnimate)
        {
            return;
        }

        if (nullptr != this->skeleton)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
            {
                this->skeleton->resetToPose();
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "AnimationBlenderV2::resetBones");
        }
    }

    void AnimationBlenderV2::setDebugLog(bool debugLog)
    {
        this->debugLog = debugLog;
    }

    void AnimationBlenderV2::setSourceEnabled(bool bEnable)
    {
        if (nullptr != this->source)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this, bEnable]()
            {
                this->source->setEnabled(bEnable);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "AnimationBlenderV2::setSourceEnabled");
        }
    }

    void AnimationBlenderV2::setAnimationSpeed(Ogre::Real speed)
    {
        this->currentSpeed = speed; // store on main thread so internalBlend can read it

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, speed]()
        {
            if (nullptr != this->source)
            {
                auto it = this->baseFrameRates.find(this->source->getName().getFriendlyText());
                if (it != this->baseFrameRates.end())
                {
                    this->source->mFrameRate = it->second * speed;
                }
            }
            if (nullptr != this->target)
            {
                auto it = this->baseFrameRates.find(this->target->getName().getFriendlyText());
                if (it != this->baseFrameRates.end())
                {
                    this->target->mFrameRate = it->second * speed;
                }
            }
            // Attention: the overlay is deliberately NOT touched here. This speed is the
            // locomotion speed the player controller drives from the walking speed, and pushing
            // it onto an upper body action made a punch play at the speed of the legs. The
            // overlay has setOverlaySpeed() for that.
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "AnimationBlenderV2::setAnimationSpeed");
    }

    Ogre::Real AnimationBlenderV2::getAnimationSpeed(void) const
    {
        return this->currentSpeed;
    }

    void AnimationBlenderV2::addAnimationBlenderObserver(IAnimationBlenderObserver* observer)
    {
        if (nullptr != observer)
        {
            // Avoid adding the same observer more than once
            if (std::find(this->animationBlenderObservers.begin(), this->animationBlenderObservers.end(), observer) == this->animationBlenderObservers.end())
            {
                this->animationBlenderObservers.push_back(observer);
            }
        }
    }

    void AnimationBlenderV2::removeAnimationBlenderObserver(IAnimationBlenderObserver* observer)
    {
        auto it = std::remove(this->animationBlenderObservers.begin(), this->animationBlenderObservers.end(), observer);
        animationBlenderObservers.erase(it, this->animationBlenderObservers.end());
    }

    void AnimationBlenderV2::queueAnimationFinishedCallback(std::function<void()> callback)
    {
        this->deferredCallbacks.push_back(callback);
    }

    void AnimationBlenderV2::processDeferredCallbacks(void)
    {
        for (auto& callback : this->deferredCallbacks)
        {
            callback(); // Execute each callback
        }
        this->deferredCallbacks.clear(); // Clear the queue once processed
    }

    // Process all deferred callbacks after notifying observers
    /*
    Deferred Callbacks :

    The deferredCallbacks vector holds all the callbacks to be executed later, instead of executing them immediately when the animation finishes.This avoids issues with nested callbacks and ensures they are executed one after the other.

    queueAnimationFinishedCallback :

    This function adds a callback to the deferredCallbacks queue.It is called inside notifyObservers to add each observer’s callback.

    processDeferredCallbacks :

    This function processes each callback in the queue after the animation is complete, ensuring that callbacks are executed in order without interfering with each other.

    In Lua :

    The Lua script defines multiple reactOnAnimationFinished calls, which are now queued and executed sequentially, avoiding issues with nesting and callback interference.

    timing is a critical aspect here, especially with chained animations where the second animation must only start after the first one finishes. With the deferred callback approach, the timing should be correct, but there are a few important things
    to ensure:

    Sequential Callback Execution: Since the callbacks are queued up in the deferredCallbacks vector, they will execute in the order they were added. In your Lua code, each reactOnAnimationFinished callback will be executed only after the
    corresponding animation completes.

    Processing Deferred Callbacks After notifyObservers: The key point is that the notifyObservers function calls processDeferredCallbacks after notifying all observers. This ensures that once the animation finishes and all observers are notified,
    the deferred callbacks (which could include other animation transitions) are processed in sequence. This guarantees that the inner animation won’t be triggered until the previous one finishes.

    Key Points to Ensure Timing:
    Animation Completion and Callback: Make sure that the first animation has fully completed before the second one is triggered. This relies on how reactOnAnimationFinished works in your system. If it fires at the right moment (i.e., when the
    animation is actually finished and ready for the next action), everything will stay in sync.

    Chaining Callbacks: By chaining the reactOnAnimationFinished callbacks, you are already setting up a sequence. The inner animation callback will only be called after the outer animation completes.

    Lua Code Example: The Lua code you've written is structured to ensure that each animation starts only after the previous one finishes:

    lua
    Kopieren
    Bearbeiten
    elseif (inputDeviceModule:isActionDown(NOWA_A_ACTION)) then
        if (waypointReached and canPull) then
            canPull = false;
            pullAction = true;
            log("--->Action!");

            pathFollowComponent:getMovingBehavior():setBehavior(BehaviorType.NONE);

            -- Start first animation
            animationBlender:blendAndContinue1(AnimID.ANIM_PICKUP_1);

            -- Queue the first callback
            playerControllerComponent:reactOnAnimationFinished(function()
                log("First animation finished (Pickup)");
                currentBucket:getJointHingeActuatorComponent():setActivated(true);

                -- Start second animation
                animationBlender:blendAndContinue1(AnimID.ANIM_ACTION_1);

                -- Queue the second callback
                playerControllerComponent:reactOnAnimationFinished(function()
                    log("Second animation finished (Action)");
                    canPull = true;
                    log("--->hier");

                    -- Start idle animation
                    animationBlender:blend5(AnimID.ANIM_IDLE_1, BlendingTransition.BLEND_WHILE_ANIMATING, 0.1, false);
                end);
            end);
        end
    end
    */
    void AnimationBlenderV2::notifyObservers(void)
    {
        std::vector<IAnimationBlenderObserver*> observersToRemove;

        for (auto it = this->animationBlenderObservers.begin(); it != this->animationBlenderObservers.end(); ++it)
        {
            IAnimationBlenderObserver* observer = *it;

            // Enqueue the observer's callback to be executed later
            this->queueAnimationFinishedCallback(
                [observer]()
                {
                    observer->onAnimationFinished();
                });

            // Handle one-time observers
            if (observer->shouldReactOneTime())
            {
                observersToRemove.push_back(observer); // Mark for removal later
            }
        }

        // Process any deferred callbacks
        this->processDeferredCallbacks();

        // Remove one-time observers after the loop
        for (IAnimationBlenderObserver* observer : observersToRemove)
        {
            this->animationBlenderObservers.erase(std::remove(this->animationBlenderObservers.begin(), this->animationBlenderObservers.end(), observer), this->animationBlenderObservers.end());

            delete observer;
        }
    }

    void AnimationBlenderV2::deleteAllObservers(void)
    {
        for (IAnimationBlenderObserver* observer : this->animationBlenderObservers)
        {
            delete observer;
        }
        this->animationBlenderObservers.clear();
    }

    void AnimationBlenderV2::resetBlendState(void)
    {
        // Clears all in-progress blend state.  Called on the render thread from
        // resetAnimation() to prevent the stale addTime closure from completing
        // a leftover blend after disconnect and re-enabling dead animations.
        if (nullptr != this->source)
        {
            this->source->setEnabled(false);
            this->source->mWeight = 0.0f;
            this->source->setTime(0.0f);
        }
        if (nullptr != this->target)
        {
            this->target->setEnabled(false);
            this->target->mWeight = 0.0f;
            this->target->setTime(0.0f);
            this->target = nullptr;
        }
        if (nullptr != this->previousSource)
        {
            this->previousSource->setEnabled(false);
            this->previousSource->mWeight = 0.0f;
            this->previousSource = nullptr;
        }
        // The overlay is a layer of its own and survived every reset so far: on disconnect it
        // stayed enabled with the locomotion clip's bones still muted, so the next connect came
        // up with a frozen upper body.
        if (nullptr != this->overlaySource)
        {
            this->internalRestoreOverlayChainWeight();
            this->internalSetOverlayOwnBoneWeights(true);

            if (true == this->overlayChainBoneIds.empty())
            {
                this->overlaySource->setOverrideBoneWeightsOnAllAnimations(1.0f, true);
            }

            this->overlaySource->setEnabled(false);
            this->overlaySource->mWeight = 0.0f;
            this->overlaySource = nullptr;
        }
        this->overlayMutedAnimations.clear();
        this->overlayMaskBoneNames.clear();
        this->overlayAllBoneNames.clear();
        this->overlayChainBoneIds.clear();
        this->overlayOtherBoneIds.clear();
        this->overlayBlendingOut = false;
        this->overlayTimeleft = 0.0f;

        this->timeleft = 0.0f;
        this->complete = false;
        // source is intentionally kept (as a disabled pointer) so internalBlend's
        // "if (nullptr == this->source)" guard works correctly on next connect.

        // Step 1: post the closure removal to the closureQueue (lock-free, safe from
        // main thread).  processClosureCommands() will remove it in the same render
        // frame that the resetBlendState command fires — so the stale closure cannot
        // run after the state has been cleared.
        Ogre::String closureId = "AnimationBlenderV2::addTime" + Ogre::StringConverter::toString(this->uniqueId);
        NOWA::GraphicsModule::getInstance()->removeTrackedClosure(closureId);
    }

    Ogre::Vector3 AnimationBlenderV2::getLocalToWorldPosition(Ogre::Bone* bone)
    {
        // Ogre::Vector3 globalPos = bone->_getDerivedPosition();

        //// Scaling must be applied, else if a huge scaled down character is used, a bone could be at position 4 60 -19
        // globalPos *= this->entity->getParentSceneNode()->getScale();
        // globalPos = this->entity->getParentSceneNode()->_getDerivedOrientation() * globalPos;

        //// Set bone local position and add to world position of the character
        // globalPos += this->entity->getParentSceneNode()->_getDerivedPosition();

        // return globalPos;

        Ogre::Vector3 worldPosition = bone->getPosition();

        // multiply with the parent derived transformation
        Ogre::Node* parentNode = this->item->getParentNode();
        Ogre::SceneNode* sceneNode = this->item->getParentSceneNode();
        while (parentNode != nullptr)
        {
            // Process the current i_Node
            if (parentNode != sceneNode)
            {
                // This is a tag point (a connection point between 2 entities). which means it has a parent i_Node to be processed
                worldPosition = ((Ogre::TagPoint*)parentNode)->convertLocalToWorldPositionUpdated(worldPosition);
                parentNode = ((Ogre::TagPoint*)parentNode)->getParent();
            }
            else
            {
                // This is the scene i_Node meaning this is the last i_Node to process
                worldPosition = parentNode->_getFullTransform() * worldPosition;
                break;
            }
        }
        return worldPosition;
    }

    Ogre::Quaternion AnimationBlenderV2::getLocalToWorldOrientation(Ogre::Bone* bone)
    {
        /*Ogre::Quaternion globalOrient = bone->_getDerivedOrientation();

        globalOrient = this->entity->getParentSceneNode()->_getDerivedOrientation() * globalOrient;

        return globalOrient;*/

        Ogre::Quaternion worldOrientation = bone->getOrientation();

        // Multiply with the parent derived transformation
        Ogre::Node* parentNode = this->item->getParentNode();
        Ogre::SceneNode* pSceneNode = this->item->getParentSceneNode();
        while (parentNode != nullptr)
        {
            // process the current i_Node
            if (parentNode != pSceneNode)
            {
                // This is a tag point (a connection point between 2 entities). which means it has a parent i_Node to be processed
                worldOrientation = ((Ogre::TagPoint*)parentNode)->convertLocalToWorldOrientationUpdated(worldOrientation);
                parentNode = ((Ogre::TagPoint*)parentNode)->getParent();
            }
            else
            {
                // This is the scene i_Node meaning this is the last i_Node to process
                worldOrientation = parentNode->_getDerivedOrientation() * worldOrientation;
                break;
            }
        }
        return worldOrientation;
    }

    void AnimationBlenderV2::beginFrame(void)
    {
        this->addTimeOwner.clear();
    }

    bool AnimationBlenderV2::tryClaimAddTime(const Ogre::String& ownerId)
    {
        if (this->addTimeOwner.empty())
        {
            this->addTimeOwner = ownerId;
            return true;
        }
        return this->addTimeOwner == ownerId;
    }

}; // namespace end