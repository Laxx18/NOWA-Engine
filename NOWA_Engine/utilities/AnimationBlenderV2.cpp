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
		debugLog(false),
		animationBlenderObserver(nullptr),
		skipReactOnAnimation(false),
		canAnimate(true)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AnimationBlenderV2] List all animations for mesh '" + this->item->getMesh()->getName() + "':");

		std::vector<Ogre::String> animationNames;

		this->skeleton = this->item->getSkeletonInstance();
		if (nullptr != this->skeleton)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Animation Blender] Cannot initialize animation blender, because the skeleton resource for item: "
															+ item->getName() + " is missing!");
			return;

			for (auto& anim : this->skeleton->getAnimationsNonConst())
			{
				anim.setEnabled(false);
				anim.mWeight = 0.0f;
				anim.setTime(0.0f);
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AnimationComponent] Animation name: " + anim.getName().getFriendlyText()
																+ " length: " + Ogre::StringConverter::toString(anim.getDuration()) + " seconds");
			}
		}

		AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &AnimationBlenderV2::gameObjectIsInRagDollStateDelegate), EventDataGameObjectIsInRagDollingState::getStaticEventType());
	}

	AnimationBlenderV2::~AnimationBlenderV2()
	{
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

	void NOWA::AnimationBlenderV2::setAnimationBlenderObserver(IAnimationBlenderObserver* animationBlenderObserver)
	{
		this->animationBlenderObserver = animationBlenderObserver;
		this->skipReactOnAnimation = false;
	}
	
	void AnimationBlenderV2::internalInit(const Ogre::String& animationName, bool loop)
	{
		if (false == this->canAnimate)
			return;

		if (nullptr == this->skeleton)
		{
			// No animations avaiable, skip
			this->canAnimate = false;
			return;
		}
		for (auto& anim : this->skeleton->getAnimationsNonConst())
		{
			anim.setEnabled(false);
			anim.mWeight = 0.0f;
			anim.setTime(0.0f);
		}
		
		// set current animation
		if (true == this->skeleton->hasAnimation(animationName))
		{
			this->source = this->skeleton->getAnimation(animationName);
			this->source->setEnabled(true);
			this->source->mWeight = 1.0f;
			this->timeleft = 0.0f;
			this->duration = 1.0f;
			this->target = nullptr;
			this->complete = false;
			this->loop = loop;
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
		}
		else
		{
			auto newTarget = this->skeleton->getAnimation(animationName);

			if (this->timeleft > 0.0f)
			{
				// oops, weren't finished yet
				if (newTarget == this->target)
				{
					// nothing to do! (ignoring duration here)
				}
				else if (newTarget == this->source)
				{
					// going back to the source state, so let's switch
					this->source = this->target;
					this->target = newTarget;
					this->timeleft = this->duration - this->timeleft; // i'm ignoring the new duration here
				}
				else
				{
					// ok, newTarget is really new, so either we simply replace the target with this one, or
					// we make the target the new source 
// Attention:
					if (this->timeleft < this->duration * 0.5f)
					// if (this->timeleft <= this->duration)
					{
						// simply replace the target with this one
						this->target->setEnabled(false);
						this->target->mWeight = 0.0f;
					}
					else
					{
						// old target becomes new source
						this->source->setEnabled(false);
						this->source->mWeight = 0.0f;
						this->source = this->target;
					}
					this->target = newTarget;
					this->target->setEnabled(true);
					this->target->mWeight = 1.0f - this->timeleft / this->duration;
					this->target->setTime(0.0f);
				}
			}
			else
			{
				// assert( target == 0, "target should be 0 when not blending" )
				// this->source->setEnabled(true);
				// this->source->setWeight(1.0f);
				this->transition = transition;
				this->timeleft = this->duration = duration;
				this->target = newTarget;
				this->target->setEnabled(true);
				this->target->mWeight = 0.0f;
				this->target->setTime(0.0f);
			}
		}
	}

	void AnimationBlenderV2::addTime(Ogre::Real time)
	{
		// Note: If entity is in ragdolling state, animation would explode the ragdoll and since its no possible to avoid, that a developer calls
		// e.g. in a lua script some animation functions, it must be avoided internally
		if (false == this->canAnimate)
		{
			return;
		}

		if (this->source != nullptr)
		{
			bool weightChange = false;
			if (this->timeleft > 0.0f)
			{
				this->timeleft -= time;
				if (this->timeleft <= 0.0f)
				{
					// finish blending
					this->source->setEnabled(false);
					this->source->mWeight = 0.0f;
					this->source = this->target;
					this->source->setEnabled(true);
					this->source->mWeight = 0.0f;
					this->target = nullptr;
				}
				else
				{
					// still blending, advance weights
					this->source->mWeight = this->timeleft / this->duration;
					this->target->mWeight = 1.0f - this->timeleft / this->duration;
					weightChange = true;
					if (this->transition == AnimationBlenderV2::BlendWhileAnimating)
					{
						this->target->addTime(time);
					}
				}
			}
			
			// Must be minus fraction, else it will never complete, OgreAnimationState bug? hasEnded only works for not looping and not accurate too
			if (this->source->getCurrentTime() >= this->source->getDuration() - 0.05f)
			{
				this->complete = true;
				if (nullptr != this->previousSource)
				{
					this->source = this->previousSource;
					this->loop = this->previousLoop;
					this->transition = this->previousTransition;
					this->duration = this->previousDuration;
					this->previousSource = nullptr;

					this->internalInit(this->source->getName().getFriendlyText());

					this->blend(this->source->getName().getFriendlyText(), this->transition, this->duration, this->loop);
				}

				// Check if there is an animation blender observer, and call when animation is finished
				if (nullptr != this->animationBlenderObserver && false == skipReactOnAnimation)
				{
					this->animationBlenderObserver->onAnimationFinished();

					if (true == this->animationBlenderObserver->shouldReactOneTime())
					{
						this->skipReactOnAnimation = true;
					}
				}
			}
			else
			{
				this->complete = false;
				if (false == weightChange)
				{
					this->source->mWeight = 1.0f;
					/*if (nullptr != this->target)
					{
						this->target->setWeight(1.0f);
					}*/
				}
			}
			this->source->addTime(time);

			if (true == this->debugLog)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "Source Animation: " + this->source->getName().getFriendlyText()
					+ " timePosition: " + Ogre::StringConverter::toString(this->source->getCurrentTime())
					+ " length: " + Ogre::StringConverter::toString(this->source->getDuration())
					+ " complete: " + Ogre::StringConverter::toString(this->complete) + " weight: " + Ogre::StringConverter::toString(this->source->mWeight));
				if (target != nullptr)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "Target Animation: " + this->target->getName().getFriendlyText()
						+ " timePosition: " + Ogre::StringConverter::toString(this->target->getCurrentTime())
						+ " length: " + Ogre::StringConverter::toString(this->target->getDuration()) + " weight: " + Ogre::StringConverter::toString(this->target->mWeight));
				}
			}
			this->source->setLoop(this->loop);
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

		this->internalBlend(it2->second, BlendingTransition::BlendSwitch, 0.2f, false);
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

	Ogre::Real AnimationBlenderV2::getProgress()
	{
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
		bool active = false;
		if (true == this->mappedAnimations.empty())
			return active;

		if (nullptr == this->source)
			return active;

		auto it = this->mappedAnimations.find(animationId);
		if (it != this->mappedAnimations.end())
		{
			active = this->source->getName().getFriendlyText() == it->second;
		}
		return active;
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
		// if this game object has an physics rag doll component and its in ragdolling state, no animation must be processed, else the skeleton will throw apart!
		unsigned long id = castEventData->getGameObjectId();
		NOWA::GameObject* gameObject = Ogre::any_cast<NOWA::GameObject*>(this->item->getUserObjectBindings().getUserAny());
		if (nullptr != gameObject)
		{
			if (gameObject->getId() == id)
			{
				this->canAnimate = !castEventData->getIsInRagDollingState();
			}
		}
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
			this->source->setTime(timePosition);
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
			this->source->mWeight = weight;
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
			skeleton->resetToPose();
		}
	}

	void AnimationBlenderV2::setDebugLog(bool debugLog)
	{
		this->debugLog = debugLog;
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