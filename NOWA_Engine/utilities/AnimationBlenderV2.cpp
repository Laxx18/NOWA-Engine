#include "NOWAPrecompiled.h"
#include "AnimationBlenderV2.h"
#include "main/AppStateManager.h"

#include "Animation/OgreSkeletonAnimation.h"
#include "Animation/OgreSkeletonInstance.h"
#include "Animation/OgreTagPoint.h"

namespace NOWA
{
	AnimationBlenderV2::AnimationBlenderV2(Ogre::Item* item)
		: item(item),
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
        overlayBlendingOut(false)
	{
		this->uniqueId = NOWA::makeUniqueID();

		this->skeleton = this->item->getSkeletonInstance();
		if (nullptr == this->skeleton)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Animation BlenderV2] Cannot initialize animation blender, because the skeleton resource for item: "
															+ item->getName() + " is missing!");
			return;
		}

		this->getAllAvailableAnimationNames(false);

		AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &AnimationBlenderV2::gameObjectIsInRagDollStateDelegate), EventDataGameObjectIsInRagDollingState::getStaticEventType());
	}

	AnimationBlenderV2::~AnimationBlenderV2()
	{
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

			if (false == skipLogging)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AnimationComponent] Animation name: " + anim.getName().getFriendlyText()
																+ " length: " + Ogre::StringConverter::toString(anim.getDuration()) + " seconds");
			}
		}

		return animationNames;
	}
	
	void AnimationBlenderV2::internalInit(const Ogre::String& animationName, bool loop)
    {
        if (false == this->canAnimate)
        {
            return;
        }

        if (nullptr == this->skeleton)
        {
            // No animations available — permanently disable to avoid null-pointer spam.
            this->canAnimate = false;
            return;
        }

        // Disable every animation on the skeleton before activating the new one,
        // so no previously running animation bleeds into the new source.
        for (auto& anim : this->skeleton->getAnimationsNonConst())
        {
            anim.setEnabled(false);
            anim.mWeight = 0.0f;
            anim.setTime(0.0f);
        }

        if (true == this->skeleton->hasAnimation(animationName))
        {
            this->source = this->skeleton->getAnimation(animationName);
            this->source->setEnabled(true);
            this->source->mWeight = 1.0f;
            this->timeleft = 0.0f;
            this->duration = this->source->getDuration();
            this->target = nullptr;
            this->complete = false;
            this->loop = loop;

            // Propagate the loop flag to Ogre immediately so that the very first
            // addTime() call wraps (or stops) correctly without waiting one extra tick.
            this->source->setLoop(loop);

            // Apply the current playback speed to the freshly assigned source.
            auto it = this->baseFrameRates.find(animationName);
            if (it != this->baseFrameRates.end())
            {
                this->source->mFrameRate = it->second * this->currentSpeed;
            }
        }
    }
	
	void AnimationBlenderV2::blend(AnimID animationId, BlendingTransition transition, Ogre::Real duration, bool loop)
	{
		if (false == this->canAnimate)
			return;

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
	
	void AnimationBlenderV2::internalBlend(const Ogre::String& animationName, BlendingTransition transition, Ogre::Real duration, bool loop)
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
        NOWA::GraphicsModule::RenderCommand renderCommand = [this, animationName, transition, duration, loop]()
        {
            this->loop = loop;

            if (transition == AnimationBlenderV2::BlendSwitch)
            {
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
                        // reversing — source and target swap roles, timeleft mirrors
                        this->source = this->target;
                        this->target = newTarget;
                        this->timeleft = this->duration - this->timeleft;
                        // no speed change needed — both were already at currentSpeed
                    }
                    else
                    {
                        // genuinely new target mid-blend
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
                        this->target->mWeight = 1.0f - this->timeleft / this->duration;
                        this->target->setTime(0.0f);

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

                    // Phase-sync for looping locomotion blends
                    if (this->source->mLoop && loop && this->source->getNumFrames() > 0.0f && this->target->getNumFrames() > 0.0f)
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
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "AnimationBlenderV2::internalBlend");
	}

	//   1. source->setLoop() is now called BEFORE source->addTime() so Ogre's
    //      internal wrap is applied on the same frame the end is reached.
    //   2. Looping and non-looping paths are fully separated:
    //        - loop = true  → complete stays false, notifyObservers() never fires.
    //        - loop = false → old threshold logic kept, notifyObservers() fires once.
    //   3. The completionThreshold constant is named so it is easy to adjust.
    // -----------------------------------------------------------------------------
    void AnimationBlenderV2::addTime(Ogre::Real time)
    {
        // If the entity is in ragdolling state, skip animation updates entirely —
        // driving animations during ragdoll would corrupt the physics simulation.
        if (false == this->canAnimate)
        {
            return;
        }

        if (nullptr == this->source)
        {
            return;
        }

        auto closureFunction = [this, time](Ogre::Real renderDt)
        {
            bool weightChange = false;

            // ---- Cross-fade / blend phase ----------------------------------------
            if (this->timeleft > 0.0f)
            {
                this->timeleft -= time;

                if (this->timeleft <= 0.0f)
                {
                    // Blend finished: promote target to source.
                    this->source->setEnabled(false);
                    this->source->mWeight = 0.0f;
                    this->source = this->target;
                    this->source->setEnabled(true);
                    this->source->mWeight = 1.0f;
                    this->target = nullptr;

                    // For BlendThenAnimate the animation starts playing from frame 0
                    // once the blend phase is complete.
                    if (this->transition == BlendingTransition::BlendThenAnimate)
                    {
                        this->source->setFrame(0.0f);
                    }
                }
                else
                {
                    // Still in blend phase — update weights with a smoothstep curve
                    // so the transition eases in and out instead of feeling linear.
                    Ogre::Real t = 1.0f - (this->timeleft / this->duration);
                    Ogre::Real smooth = t * t * (3.0f - 2.0f * t);
                    this->source->mWeight = 1.0f - smooth;
                    this->target->mWeight = smooth;
                    weightChange = true;

                    if (this->transition == AnimationBlenderV2::BlendWhileAnimating || this->transition == AnimationBlenderV2::BlendThenAnimate)
                    {
                        this->target->addTime(time);
                    }
                }
            }

            // ---- FIX: set loop flag BEFORE addTime --------------------------------
            // Ogre reads mLoop inside addTime to decide whether to wrap the time or
            // clamp it.  Setting it after addTime meant the wrap decision on the very
            // last frame was made with the stale flag from the previous animation.
            this->source->setLoop(this->loop);

            // ---- Completion / loop handling ---------------------------------------

            if (this->loop)
            {
                // FIX: looping animations must NEVER set complete = true and must
                // NEVER call notifyObservers().  Ogre wraps time internally via
                // addTime() as long as setLoop(true) has been called (see above).
                // The old code fired notifyObservers() on every single loop cycle,
                // causing spurious animation-finished callbacks and visually the
                // complete flag toggling true/false each cycle made the weight
                // management inconsistent.
                this->complete = false;

                if (false == weightChange)
                {
                    this->source->mWeight = 1.0f;
                }
            }
            else
            {
                // Non-looping path: use a small threshold instead of comparing
                // against the exact duration value because floating-point accumulation
                // can overshoot getDuration() by tiny amounts, and Ogre's hasEnded()
                // is documented as unreliable for non-looping skeleton animations.
                const Ogre::Real completionThreshold = 0.05f;

                if (this->source->getCurrentTime() >= this->source->getDuration() - completionThreshold)
                {
                    this->complete = true;

                    // blendAndContinue support: if a previous source was saved before
                    // the one-shot animation started, restore it now that the one-shot
                    // has finished playing.
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

                        // Re-apply the playback speed — the previous source may not
                        // have been touched since the last setAnimationSpeed() call.
                        auto it = this->baseFrameRates.find(this->source->getName().getFriendlyText());
                        if (it != this->baseFrameRates.end())
                        {
                            this->source->mFrameRate = it->second * this->currentSpeed;
                        }

                        // ---- Overlay animation driving ---------------------------
                        if (nullptr != this->overlaySource)
                        {
                            if (false == this->overlayBlendingOut)
                            {
                                // Blend-in phase: ramp weight from 0 to 1.
                                if (this->overlayTimeleft > 0.0f)
                                {
                                    this->overlayTimeleft -= time;
                                    this->overlaySource->mWeight = (this->overlayTimeleft <= 0.0f) ? 1.0f : 1.0f - (this->overlayTimeleft / this->overlayDuration);
                                }

                                this->overlaySource->addTime(time);

                                // Non-looping overlay finished on its own — auto-remove.
                                if (false == this->overlaySource->mLoop && this->overlaySource->getCurrentTime() >= this->overlaySource->getDuration() - completionThreshold)
                                {
                                    this->overlaySource->setOverrideBoneWeightsOnAllAnimations(1.0f, true);
                                    this->overlaySource->setEnabled(false);
                                    this->overlaySource->mWeight = 0.0f;
                                    this->overlaySource = nullptr;
                                }
                            }
                            else
                            {
                                // Blend-out phase: ramp weight from 1 to 0.
                                if (this->overlayTimeleft > 0.0f)
                                {
                                    this->overlayTimeleft -= time;

                                    if (this->overlayTimeleft <= 0.0f)
                                    {
                                        // Fully blended out — shut down overlay.
                                        this->overlaySource->setEnabled(false);
                                        this->overlaySource->mWeight = 0.0f;
                                        this->overlaySource = nullptr;
                                    }
                                    else
                                    {
                                        this->overlaySource->mWeight = this->overlayTimeleft / this->overlayDuration;
                                        this->overlaySource->addTime(time);
                                    }
                                }
                            }
                        }
                    }

                    // Notify all registered observers that the non-looping animation
                    // has ended.  Only fired once per completion, not every frame.
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

            // ---- Advance source animation time ------------------------------------
            // addTime() wraps automatically for looping animations because setLoop()
            // was already called above before this point.
            this->source->addTime(time);

            // ---- Debug logging ----------------------------------------------------
            if (true == this->debugLog)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "Source Animation: " + this->source->getName().getFriendlyText() + " timePosition: " + Ogre::StringConverter::toString(this->source->getCurrentTime()) +
                                                                                        " length: " + Ogre::StringConverter::toString(this->source->getDuration()) + " complete: " + Ogre::StringConverter::toString(this->complete) +
                                                                                        " weight: " + Ogre::StringConverter::toString(this->source->mWeight));

                if (nullptr != this->target)
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "Target Animation: " + this->target->getName().getFriendlyText() + " timePosition: " + Ogre::StringConverter::toString(this->target->getCurrentTime()) +
                                                                                            " length: " + Ogre::StringConverter::toString(this->target->getDuration()) + " weight: " + Ogre::StringConverter::toString(this->target->mWeight));
                }
            }
        };

        Ogre::String id = "AnimationBlenderV2::addTime" + Ogre::StringConverter::toString(this->uniqueId);
        NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction);
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
        this->internalSetOverlayAnimation(it->second, blendInTime);
    }

    void AnimationBlenderV2::setOverlayAnimation(const Ogre::String& animationName, Ogre::Real blendInTime)
    {
        if (animationName.empty())
        {
            return;
        }
        this->internalSetOverlayAnimation(animationName, blendInTime);
    }

	void AnimationBlenderV2::internalSetOverlayAnimation(const Ogre::String& animationName, Ogre::Real blendInTime)
    {
        if (false == this->canAnimate)
        {
            return;
        }

        if (false == this->skeleton->hasAnimation(animationName))
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AnimationBlenderV2] setOverlayAnimation: animation '" + animationName + "' not found.");
            return;
        }

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, animationName, blendInTime]()
        {
            // Clean up any existing overlay first
            if (nullptr != this->overlaySource)
            {
                // Restore all animations' bone weights before replacing
                this->overlaySource->setOverrideBoneWeightsOnAllAnimations(1.0f, true);
                this->overlaySource->setEnabled(false);
                this->overlaySource->mWeight = 0.0f;
                this->overlaySource = nullptr;
            }

            this->overlaySource = this->skeleton->getAnimation(animationName);
            this->overlaySource->setEnabled(true);
            this->overlaySource->mWeight = 0.0f; // will be driven by addTime
            this->overlaySource->setTime(0.0f);
            this->overlaySource->setLoop(false);

            // Apply current playback speed
            auto it = this->baseFrameRates.find(animationName);
            if (it != this->baseFrameRates.end())
            {
                this->overlaySource->mFrameRate = it->second * this->currentSpeed;
            }

            // Key call: read which bones THIS animation uses and zero those exact
            // bones on ALL other animations (active and inactive).
            // Using the AllAnimations variant so any animation blended in later
            // while the overlay is still active also gets the correct bone weights.
            this->overlaySource->setOverrideBoneWeightsOnAllAnimations(0.0f, true);

            this->overlayDuration = (blendInTime > 0.0f) ? blendInTime : 0.001f;
            this->overlayTimeleft = this->overlayDuration;
            this->overlayBlendingOut = false;
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "AnimationBlenderV2::setOverlayAnimation");
    }

    void AnimationBlenderV2::clearOverlayAnimation(Ogre::Real blendOutTime)
    {
        NOWA::GraphicsModule::RenderCommand renderCommand = [this, blendOutTime]()
        {
            if (nullptr == this->overlaySource)
            {
                return;
            }

            // Restore all animations' bone weights so the main animation
            // fully controls those bones again during blend-out
            this->overlaySource->setOverrideBoneWeightsOnAllAnimations(1.0f, true);

            this->overlayBlendingOut = true;
            this->overlayDuration = (blendOutTime > 0.0f) ? blendOutTime : 0.001f;
            this->overlayTimeleft = this->overlayDuration;
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "AnimationBlenderV2::clearOverlayAnimation");
    }

    bool AnimationBlenderV2::isOverlayAnimationActive(void) const
    {
        return nullptr != this->overlaySource;
    }

    void AnimationBlenderV2::driveBlendSpace(Ogre::Real parameter, const IAnimationBlender::BlendSpaceEntryList& entryList)
    {
        const auto entries = entryList.getEntries();
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
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "AnimationBlenderV2::driveBlendSpace");
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
		for (auto it = this->mappedAnimations.cbegin(); it != this->mappedAnimations.cend(); ++it)
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
			return active;

		if (nullptr == this->target)
			return active;

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
			return this->internalGetAnimationState(it->second);
		
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
			animationState = this->skeleton->getAnimation(animationName);
		
		return animationState;
	}
	
	void AnimationBlenderV2::dumpAllAnimations(Ogre::Node* node, Ogre::String padding)
	{
		//create the scene tree
	
		Ogre::SceneNode::ObjectIterator objectIt = ((Ogre::SceneNode*) node)->getAttachedObjectIterator();
		auto nodeIt = node->getChildIterator();

		while (objectIt.hasMoreElements())
		{
			//go through all scenenodes in the scene
			Ogre::MovableObject* movableObject = objectIt.getNext();
			Ogre::Item* item = dynamic_cast<Ogre::Item*>(movableObject);
			if (nullptr != item)
			{
				if (true == item->hasSkeleton())
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, padding + "List all animations for mesh '" + item->getMesh()->getName() + "':");
					Ogre::SkeletonInstance* skeleton = item->getSkeletonInstance();
					if (nullptr != skeleton)
					{
                        for (const auto& anim : skeleton->getAnimations())
						{
							Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AnimationComponent] Animation name: " + anim.getName().getFriendlyText()
																			+ " length: " + Ogre::StringConverter::toString(anim.getDuration()) + " seconds");
						}
					}
				}
			}
		}
		while (nodeIt.hasMoreElements())
		{
			//go through all objects recursive that are attached to the scenenodes
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
            NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "AnimationBlenderV2::setTimePosition");
		}
	}

	Ogre::Real AnimationBlenderV2::getTimePosition(void) const
	{
		if (nullptr != this->source)
		{
			Ogre::Real ts = this->source->getCurrentTime();
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
            NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "AnimationBlenderV2::setWeight");
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
            NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "AnimationBlenderV2::resetBones");
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
            NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "AnimationBlenderV2::setSourceEnabled");
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
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "AnimationBlenderV2::setAnimationSpeed");
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
			callback();  // Execute each callback
		}
		this->deferredCallbacks.clear();  // Clear the queue once processed
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

	timing is a critical aspect here, especially with chained animations where the second animation must only start after the first one finishes. With the deferred callback approach, the timing should be correct, but there are a few important things to ensure:

	Sequential Callback Execution: Since the callbacks are queued up in the deferredCallbacks vector, they will execute in the order they were added. In your Lua code, each reactOnAnimationFinished callback will be executed only after the corresponding animation completes.

	Processing Deferred Callbacks After notifyObservers: The key point is that the notifyObservers function calls processDeferredCallbacks after notifying all observers. This ensures that once the animation finishes and all observers are notified, the deferred callbacks (which could include other animation transitions) are processed in sequence. This guarantees that the inner animation won’t be triggered until the previous one finishes.

	Key Points to Ensure Timing:
	Animation Completion and Callback: Make sure that the first animation has fully completed before the second one is triggered. This relies on how reactOnAnimationFinished works in your system. If it fires at the right moment (i.e., when the animation is actually finished and ready for the next action), everything will stay in sync.

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
			this->queueAnimationFinishedCallback([observer]()
			{
				observer->onAnimationFinished();
			});

			// Handle one-time observers
			if (observer->shouldReactOneTime())
			{
				observersToRemove.push_back(observer);  // Mark for removal later
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

	Ogre::Vector3 AnimationBlenderV2::getLocalToWorldPosition(Ogre::Bone* bone)
	{
		//Ogre::Vector3 globalPos = bone->_getDerivedPosition();

		//// Scaling must be applied, else if a huge scaled down character is used, a bone could be at position 4 60 -19
		//globalPos *= this->entity->getParentSceneNode()->getScale();
		//globalPos = this->entity->getParentSceneNode()->_getDerivedOrientation() * globalPos;

		//// Set bone local position and add to world position of the character
		//globalPos += this->entity->getParentSceneNode()->_getDerivedPosition();

		//return globalPos;

		Ogre::Vector3 worldPosition = bone->getPosition();

        //multiply with the parent derived transformation
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
            //process the current i_Node
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

}; // namespace end