#include "NOWAPrecompiled.h"
#include "AnimationBlender.h"
#include "OgreTagPoint.h"
#include "main/AppStateManager.h"
#include "utilities/Mathhelper.h"

namespace NOWA
{
	AnimationBlender::AnimationBlender(Ogre::v1::Entity* entity)
		: entity(entity),
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
		this->getAllAvailableAnimationNames(false);

		Ogre::v1::OldSkeletonInstance* skeleton = entity->getSkeleton();
		if (nullptr != skeleton)
		{
			skeleton->setBlendMode(Ogre::v1::ANIMBLEND_CUMULATIVE /*Ogre::v1::ANIMBLEND_AVERAGE*/);
		}

		AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &AnimationBlender::gameObjectIsInRagDollStateDelegate), EventDataGameObjectIsInRagDollingState::getStaticEventType());
	}

	AnimationBlender::~AnimationBlender()
	{
		AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &AnimationBlender::gameObjectIsInRagDollStateDelegate), EventDataGameObjectIsInRagDollingState::getStaticEventType());
	}

	void AnimationBlender::init(AnimID animationId, bool loop)
	{
		auto it = this->mappedAnimations.find(animationId);
		if (it != this->mappedAnimations.cend())
		{
			this->internalInit(it->second, loop);
		}
	}
	
	void AnimationBlender::init(const Ogre::String& animationName, bool loop)
	{
		this->internalInit(animationName, loop);
	}

	std::vector<Ogre::String> AnimationBlender::getAllAvailableAnimationNames(bool skipLogging) const
	{
		std::vector<Ogre::String> animationNames;

		if (false == skipLogging)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[Animation Blender] List all animations for mesh '" + this->entity->getMesh()->getName() + "':");
		}

		Ogre::v1::AnimationStateSet* set = this->entity->getAllAnimationStates();
		if (nullptr == set)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Animation Blender] Cannot initialize animation blender, because the skeleton resource for entity: "
															+ entity->getName() + " is missing!");
			return animationNames;
		}

		Ogre::v1::AnimationStateIterator it = set->getAnimationStateIterator();
		// Manual pose loading: http://www.ogre3d.org/tikiwiki/tiki-index.php?page=Generic+Manual+Pose+Loading
		// reset all animations
		while (it.hasMoreElements())
		{
			Ogre::v1::AnimationState* anim = it.getNext();
			anim->setEnabled(false);
			anim->setWeight(0.0f);
			anim->setTimePosition(0.0f);
			animationNames.emplace_back(anim->getAnimationName());

			if (false == skipLogging)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[Animation Blender] Animation name: " + anim->getAnimationName()
																+ " length: " + Ogre::StringConverter::toString(anim->getLength()) + " seconds");
			}
		}

		return animationNames;
	}

	void NOWA::AnimationBlender::setAnimationBlenderObserver(IAnimationBlenderObserver* animationBlenderObserver)
	{
		this->animationBlenderObserver = animationBlenderObserver;
		this->skipReactOnAnimation = false;
	}
	
	void AnimationBlender::internalInit(const Ogre::String& animationName, bool loop)
	{
		if (false == this->canAnimate)
			return;

		Ogre::v1::AnimationStateSet* set = this->entity->getAllAnimationStates();
		if (nullptr == set)
		{
			// No animations avaiable, skip
			this->canAnimate = false;
			return;
		}
		Ogre::v1::AnimationStateIterator it = set->getAnimationStateIterator();
		// Manual pose loading: http://www.ogre3d.org/tikiwiki/tiki-index.php?page=Generic+Manual+Pose+Loading
		// reset all animations
		while (it.hasMoreElements())
		{
			Ogre::v1::AnimationState* anim = it.getNext();
			anim->setEnabled(false);
			anim->setWeight(0.0f);
			anim->setTimePosition(0.0f);
		}
		
		// set current animation
		if (true == this->entity->hasAnimationState(animationName))
		{
			this->source = this->entity->getAnimationState(animationName);
			this->source->setEnabled(false);
			this->source->setWeight(1.0f);
			this->timeleft = 0.0f;
			this->duration = 1.0f;
			this->target = nullptr;
			this->complete = false;
			this->loop = loop;
		}
	}
	
	void AnimationBlender::blend(AnimID animationId, BlendingTransition transition, Ogre::Real duration, bool loop)
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
	
	void AnimationBlender::blend(const Ogre::String& animationName, BlendingTransition transition, Ogre::Real duration, bool loop)
	{
		if (true == animationName.empty())
		{
			return;
		}

		this->internalBlend(animationName, transition, duration, loop);
	}

	void AnimationBlender::blend(AnimID animationId, BlendingTransition transition, bool loop)
	{
		auto it2 = this->mappedAnimations.find(animationId);
		if (this->mappedAnimations.end() == it2)
		{
			// if the animation cannot be found, just skip
			return;
		}

		this->internalBlend(it2->second, transition, 0.2f, loop);
	}

	void AnimationBlender::blend(const Ogre::String& animationName, BlendingTransition transition, bool loop)
	{
		if (true == animationName.empty())
		{
			return;
		}

		this->internalBlend(animationName, transition, 0.2f, loop);
	}

	void AnimationBlender::blend(AnimID animationId, BlendingTransition transition)
	{
		auto it2 = this->mappedAnimations.find(animationId);
		if (this->mappedAnimations.end() == it2)
		{
			// if the animation cannot be found, just skip
			return;
		}

		this->internalBlend(it2->second, transition, 0.2f, true);
	}

	void AnimationBlender::blend(const Ogre::String& animationName, BlendingTransition transition)
	{
		if (true == animationName.empty())
		{
			return;
		}

		this->internalBlend(animationName, transition, 0.2f, true);
	}

	void AnimationBlender::blendExclusive(AnimID animationId, BlendingTransition transition, Ogre::Real duration, bool loop)
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

	void AnimationBlender::blendExclusive(const Ogre::String& animationName, BlendingTransition transition, Ogre::Real duration, bool loop)
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

	void AnimationBlender::blendExclusive(AnimID animationId, BlendingTransition transition, bool loop)
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

	void AnimationBlender::blendExclusive(const Ogre::String& animationName, BlendingTransition transition, bool loop)
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

	void AnimationBlender::blendExclusive(AnimID animationId, BlendingTransition transition)
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

	void AnimationBlender::blendExclusive(const Ogre::String& animationName, BlendingTransition transition)
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
	
	void AnimationBlender::internalBlend(const Ogre::String& animationName, BlendingTransition transition, Ogre::Real duration, bool loop)
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

		if (transition == AnimationBlender::BlendSwitch)
		{
			if (this->source != nullptr)
			{
				this->source->setEnabled(false);
			}
			this->source = this->entity->getAnimationState(animationName);
			this->source->setEnabled(true);
			this->source->setWeight(1.0f);
			this->source->setTimePosition(0.0f);
			this->timeleft = 0.0f;
		}
		else
		{
			Ogre::v1::AnimationState* newTarget = entity->getAnimationState(animationName);

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
						this->target->setWeight(0.0f);
					}
					else
					{
						// old target becomes new source
						this->source->setEnabled(false);
						this->source->setWeight(0.0f);
						this->source = this->target;
					}
					this->target = newTarget;
					this->target->setEnabled(true);
					this->target->setWeight(1.0f - this->timeleft / this->duration);
					this->target->setTimePosition(0.0f);
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
				this->target->setWeight(0.0f);
				this->target->setTimePosition(0.0f);
			}
		}
	}

	void AnimationBlender::addTime(Ogre::Real time)
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
					this->source->setWeight(0.0f);
					this->source = this->target;
					this->source->setEnabled(true);
					this->source->setWeight(1.0f);
					this->target = nullptr;
				}
				else
				{
					// still blending, advance weights
					this->source->setWeight(this->timeleft / this->duration);
					this->target->setWeight(1.0f - this->timeleft / this->duration);
					weightChange = true;
					if (this->transition == AnimationBlender::BlendWhileAnimating)
					{
						this->target->addTime(time);
					}
				}
			}

			this->source->addTime(time);

			// Must be minus fraction, else it will never complete, OgreAnimationState bug? hasEnded only works for not looping and not accurate too
			if (this->source->getTimePosition() + 0.02f >= this->source->getLength())
			{
				this->complete = true;

				if (nullptr != this->previousSource)
				{
					this->source = this->previousSource;
					this->loop = this->previousLoop;
					this->transition = this->previousTransition;
					this->duration = this->previousDuration;
					this->previousSource = nullptr;

					this->internalInit(this->source->getAnimationName());

					this->blend(this->source->getAnimationName(), this->transition, this->duration, this->loop);
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
					this->source->setWeight(1.0f);
					if (false == this->source->getEnabled())
					{
						this->source->setEnabled(true);
					}
					/*if (nullptr != this->target)
					{
						this->target->setWeight(1.0f);
					}*/
				}
			}

			if (true == this->debugLog)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "Name: " + this->entity->getParentSceneNode()->getName() + " Source Animation : " + this->source->getAnimationName()
					+ " timePosition: " + Ogre::StringConverter::toString(this->source->getTimePosition())
					+ " length: " + Ogre::StringConverter::toString(this->source->getLength())
					+ " complete: " + Ogre::StringConverter::toString(this->complete) + " weight: " + Ogre::StringConverter::toString(this->source->getWeight()));
				if (target != nullptr)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "Name: " + this->entity->getParentSceneNode()->getName() + " Target Animation: " + this->target->getAnimationName()
						+ " timePosition: " + Ogre::StringConverter::toString(this->target->getTimePosition())
						+ " length: " + Ogre::StringConverter::toString(this->target->getLength()) + " weight: " + Ogre::StringConverter::toString(this->target->getWeight()));
				}
			}
			this->source->setLoop(this->loop);
		}
	}

	void AnimationBlender::blendAndContinue(AnimID animationId, Ogre::Real duration)
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

	void AnimationBlender::blendAndContinue(const Ogre::String& animationName, Ogre::Real duration)
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

	void AnimationBlender::blendAndContinue(AnimID animationId)
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
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "Target animation active: " + this->target->getAnimationName());
			}
			return;
		}
		if (true == this->isAnimationActive(animationId))
		{
			if (true == this->debugLog)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "Source animation active: " + this->source->getAnimationName());
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

	void AnimationBlender::blendAndContinue(const Ogre::String& animationName)
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

	Ogre::Real AnimationBlender::getProgress()
	{
		return this->timeleft / this->duration;
	}

	Ogre::v1::AnimationState* AnimationBlender::getSource()
	{
		return this->source;
	}

	Ogre::v1::AnimationState* AnimationBlender::getTarget()
	{
		return this->target;
	}

	bool AnimationBlender::isComplete(void) const
	{
		return this->complete;
	}

	void AnimationBlender::registerAnimation(AnimID animationId, const Ogre::String& animationName)
	{
		if (animationName != "None")
		{
			this->mappedAnimations.insert(std::make_pair(animationId, animationName));
		}
	}

	AnimationBlender::AnimID AnimationBlender::getAnimationIdFromString(const Ogre::String& animationName)
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

	void AnimationBlender::clearAnimations(void)
	{
		this->mappedAnimations.clear();
	}

	bool AnimationBlender::hasAnimation(const Ogre::String& animationName)
	{
		AnimID animId = this->getAnimationIdFromString(animationName);
		return this->hasAnimation(animId);
	}

	bool AnimationBlender::hasAnimation(AnimID animationId)
	{
		return this->mappedAnimations.find(animationId) != this->mappedAnimations.end();
	}

	bool AnimationBlender::isAnimationActive(AnimID animationId)
	{
		bool active = false;
		if (true == this->mappedAnimations.empty())
			return active;

		if (nullptr == this->source)
			return active;

		auto it = this->mappedAnimations.find(animationId);
		if (it != this->mappedAnimations.end())
		{
			active = this->source->getAnimationName() == it->second;
		}
		return active;
	}

	bool AnimationBlender::isAnyAnimationActive(void)
	{
		return true == this->source->getEnabled() && (nullptr != this->target && true == this->target->getEnabled());
	}

	bool AnimationBlender::isTargetAnimationActive(AnimID animationId)
	{
		bool active = false;
		if (true == this->mappedAnimations.empty())
			return active;

		if (nullptr == this->target)
			return active;

		auto it = this->mappedAnimations.find(animationId);
		if (it != this->mappedAnimations.end())
		{
			active = this->target->getAnimationName() == it->second;
		}
		return active;
	}

	void AnimationBlender::gameObjectIsInRagDollStateDelegate(EventDataPtr eventData)
	{
		boost::shared_ptr<EventDataGameObjectIsInRagDollingState> castEventData = boost::static_pointer_cast<NOWA::EventDataGameObjectIsInRagDollingState>(eventData);
		// if this game object has an physics rag doll component and its in ragdolling state, no animation must be processed, else the skeleton will throw apart!
		unsigned long id = castEventData->getGameObjectId();
		NOWA::GameObject* gameObject = Ogre::any_cast<NOWA::GameObject*>(this->entity->getUserObjectBindings().getUserAny());
		if (nullptr != gameObject)
		{
			if (gameObject->getId() == id)
			{
				this->canAnimate = !castEventData->getIsInRagDollingState();
			}
		}
	}
	
	Ogre::v1::AnimationState* AnimationBlender::getAnimationState(AnimID animationId)
	{
		auto it = this->mappedAnimations.find(animationId);
		if (it != this->mappedAnimations.cend())
			return this->internalGetAnimationState(it->second);
		
		return nullptr;
	}
	
	Ogre::v1::AnimationState* AnimationBlender::getAnimationState(const Ogre::String& animationName)
	{
		return this->internalGetAnimationState(animationName);
	}
	
	Ogre::v1::AnimationState* AnimationBlender::internalGetAnimationState(const Ogre::String& animationName)
	{
		Ogre::v1::AnimationState* animationState = nullptr;
// Attention: Is this check necessary?
		if (true == this->entity->hasAnimationState(animationName))
			animationState = this->entity->getAnimationState(animationName);
		
		return animationState;
	}
	
	void AnimationBlender::dumpAllAnimations(Ogre::Node* node, Ogre::String padding)
	{
		//create the scene tree
	
		Ogre::SceneNode::ObjectIterator objectIt = ((Ogre::SceneNode*) node)->getAttachedObjectIterator();
		auto nodeIt = node->getChildIterator();

		while (objectIt.hasMoreElements())
		{
			//go through all scenenodes in the scene
			Ogre::MovableObject* movableObject = objectIt.getNext();
			Ogre::v1::Entity* entity = dynamic_cast<Ogre::v1::Entity*>(movableObject);
			if (entity)
			{
				if (entity->hasSkeleton())
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, padding + "List all animations for mesh '" + entity->getMesh()->getName() + "':");
					Ogre::v1::AnimationStateSet* set = entity->getAllAnimationStates();
					Ogre::v1::AnimationStateIterator it = set->getAnimationStateIterator();
					// reset all animations
					while (it.hasMoreElements())
					{
						Ogre::v1::AnimationState* anim = it.getNext();
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[Animation Blender] Animation name: " + anim->getAnimationName()
							+ " length: " + Ogre::StringConverter::toString(anim->getLength()) + " seconds");
					}
				}
			}
		}
		while (nodeIt.hasMoreElements())
		{
			//go through all objects recursive that are attached to the scenenodes
			AnimationBlender::dumpAllAnimations(nodeIt.getNext(), "  " + padding);
		}
	}

	void AnimationBlender::setTimePosition(Ogre::Real timePosition)
	{
		if (nullptr != this->source)
		{
			this->source->setTimePosition(timePosition);
		}
	}

	Ogre::Real AnimationBlender::getTimePosition(void) const
	{
		if (nullptr != this->source)
		{
			Ogre::Real ts = this->source->getTimePosition();
			return this->source->getTimePosition();
		}
		return 0.0f;
	}

	Ogre::Real AnimationBlender::getLength(void) const
	{
		if (nullptr != this->source)
		{
			return this->source->getLength();
		}
		return 0.0f;
	}

	void AnimationBlender::setWeight(Ogre::Real weight)
	{
		if (nullptr != this->source)
		{
			this->source->setWeight(weight);
		}
	}

	Ogre::Real AnimationBlender::getWeight(void) const
	{
		if (nullptr != this->source)
		{
			return this->source->getWeight();
		}
		return 0.0f;
	}

	Ogre::v1::OldBone* AnimationBlender::getBone(const Ogre::String& boneName)
	{
		Ogre::v1::OldBone* bone = nullptr;
		if (nullptr != this->entity)
		{
			Ogre::v1::Skeleton* skeleton = this->entity->getSkeleton();
			if (nullptr != skeleton)
			{
				if (true == skeleton->hasBone(boneName))
				{
					bone = skeleton->getBone(boneName);
					return bone;
				}
			}
		}
		return bone;
	}

	void AnimationBlender::setBoneWeight(Ogre::v1::OldBone* bone, Ogre::Real weight)
	{
		// Note: If entity is in ragdolling state, animation would explode the ragdoll and since its no possible to avoid, that a developer calls
		// e.g. in a lua script some animation functions, it must be avoided internally
		if (false == this->canAnimate)
		{
			return;
		}

		if (nullptr == bone || nullptr == this->entity)
		{
			return;
		}
		bone->reset(); // _pBone is Ogre::Bone*

		auto boneHandle = bone->getHandle();
		Ogre::v1::Skeleton* skeleton = this->entity->getSkeleton();
		if (nullptr != skeleton)
		{
			auto animStateIt = this->entity->getAllAnimationStates()->getAnimationStateIterator();
			while (animStateIt.hasMoreElements())
			{
				Ogre::v1::AnimationState* animationState = animStateIt.getNext();

				// ignore disabled animations
				if (!animationState->getEnabled())
					continue;

				// get animation
				auto animation = skeleton->getAnimation(animationState->getAnimationName());
				// if (!animation->hasNodeTrack(boneHandle))
				// 	continue;

				auto nodeAnimTrack = animation->getNodeTrack(boneHandle);

				Ogre::Real time = animationState->getTimePosition();
				Ogre::Real tempWeight = animationState->getWeight();

				// apply animation to bone
				nodeAnimTrack->apply(time, weight);
			}
		}
	}

	void AnimationBlender::resetBones(void)
	{
		// Note: If entity is in ragdolling state, animation would explode the ragdoll and since its no possible to avoid, that a developer calls
		// e.g. in a lua script some animation functions, it must be avoided internally
		if (false == this->canAnimate)
		{
			return;
		}

		if (nullptr != this->entity)
		{
			Ogre::v1::Skeleton* skeleton = this->entity->getSkeleton();
			if (nullptr != skeleton)
			{
				skeleton->reset(true);
			}
		}
	}

	void AnimationBlender::setDebugLog(bool debugLog)
	{
		this->debugLog = debugLog;
	}

	void AnimationBlender::setSourceEnabled(bool bEnable)
	{
		if (nullptr != this->source)
		{
			this->source->setEnabled(bEnable);
		}
	}

	Ogre::Vector3 AnimationBlender::getLocalToWorldPosition(Ogre::v1::OldBone* bone)
	{
		//Ogre::Vector3 globalPos = bone->_getDerivedPosition();

		//// Scaling must be applied, else if a huge scaled down character is used, a bone could be at position 4 60 -19
		//globalPos *= this->entity->getParentSceneNode()->getScale();
		//globalPos = this->entity->getParentSceneNode()->_getDerivedOrientation() * globalPos;

		//// Set bone local position and add to world position of the character
		//globalPos += this->entity->getParentSceneNode()->_getDerivedPosition();

		//return globalPos;

		Ogre::Vector3 worldPosition = bone->_getDerivedPosition();

        //multiply with the parent derived transformation
        Ogre::Node* parentNode = this->entity->getParentNode();
        Ogre::SceneNode* sceneNode = this->entity->getParentSceneNode();
        while (parentNode != nullptr)
        {
            // Process the current i_Node
            if (parentNode != sceneNode)
            {
                // This is a tag point (a connection point between 2 entities). which means it has a parent i_Node to be processed
                worldPosition = ((Ogre::v1::TagPoint*)parentNode)->_getFullLocalTransform() * worldPosition;
                parentNode = ((Ogre::v1::TagPoint*)parentNode)->getParentEntity()->getParentNode();
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

	Ogre::Quaternion AnimationBlender::getLocalToWorldOrientation(Ogre::v1::OldBone* bone)
	{
		/*Ogre::Quaternion globalOrient = bone->_getDerivedOrientation();

		globalOrient = this->entity->getParentSceneNode()->_getDerivedOrientation() * globalOrient;
		
		return globalOrient;*/

		Ogre::Quaternion worldOrientation = bone->_getDerivedOrientation();

        // Multiply with the parent derived transformation
        Ogre::Node* parentNode = this->entity->getParentNode();
        Ogre::SceneNode* pSceneNode = this->entity->getParentSceneNode();
        while (parentNode != nullptr)
        {
            //process the current i_Node
            if (parentNode != pSceneNode)
            {
                // This is a tag point (a connection point between 2 entities). which means it has a parent i_Node to be processed
                worldOrientation = ((Ogre::v1::TagPoint*)parentNode)->_getDerivedOrientation() * worldOrientation;
                parentNode = ((Ogre::v1::TagPoint*)parentNode)->getParentEntity()->getParentNode();
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