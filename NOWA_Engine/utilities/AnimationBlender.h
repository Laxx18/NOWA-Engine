#ifndef ANIMATIONBLENDER_H
#define ANIMATIONBLENDER_H

#include "defines.h"
#include "IAnimationBlender.h"

namespace NOWA
{
	class GameObject;

	/**
	* @class AnimationBlender
	* @brief The animation blender utilities class helps fade between two animations in a smooth way
	*/
	class EXPORTED AnimationBlender : public IAnimationBlender
	{
	public:

		/**
		* @brief		Creates the animation blender for the given entity.
		* @param[in]	entity		The entity to use the animation blender on.
		* @Note			All animations for this entity will be written to log in order to see which animations the entity has.
		*/
		AnimationBlender(Ogre::v1::Entity* entity);

		virtual ~AnimationBlender();

		virtual void init(AnimID animationId, bool loop = true) override;
		
		virtual void init(const Ogre::String& animationName, bool loop = true) override;

		virtual std::vector<Ogre::String> getAllAvailableAnimationNames(bool skipLogging) const override;

		virtual void blend(AnimID animationId, BlendingTransition transition, Ogre::Real duration, bool loop) override;
		
		virtual void blend(const Ogre::String& animationName, BlendingTransition transition, Ogre::Real duration, bool loop) override;

		virtual void blend(AnimID animationId, BlendingTransition transition, bool loop) override;

		virtual void blend(const Ogre::String& animationName, BlendingTransition transition, bool loop) override;

		virtual void blend(AnimID animationId, BlendingTransition transition) override;

		virtual void blend(const Ogre::String& animationName, BlendingTransition transition) override;

		virtual void blendExclusive(AnimID animationId, BlendingTransition transition, Ogre::Real duration, bool loop) override;

		virtual void blendExclusive(const Ogre::String& animationName, BlendingTransition transition, Ogre::Real duration, bool loop) override;

		virtual void blendExclusive(AnimID animationId, BlendingTransition transition, bool loop) override;

		virtual void blendExclusive(const Ogre::String& animationName, BlendingTransition transition, bool loop) override;

		virtual void blendExclusive(AnimID animationId, BlendingTransition transition) override;

		virtual void blendExclusive(const Ogre::String& animationName, BlendingTransition transition) override;

		virtual void blendAndContinue(AnimID animationId, Ogre::Real duration) override;

		virtual void blendAndContinue(const Ogre::String& animationName, Ogre::Real duration) override;

		virtual void blendAndContinue(AnimID animationId) override;

		virtual void blendAndContinue(const Ogre::String& animationName) override;

		virtual void addTime(Ogre::Real time) override;

		virtual Ogre::Real getProgress(void) override;

		virtual bool isComplete(void) const override;

		virtual void registerAnimation(AnimID animationId, const Ogre::String& animationName) override;

		virtual AnimID getAnimationIdFromString(const Ogre::String& animationName) override;

		virtual void clearAnimations(void) override;

		virtual bool hasAnimation(const Ogre::String& animationName) override;

		virtual bool hasAnimation(AnimID animationId) override;

		virtual bool isAnimationActive(AnimID animationId) override;

		virtual bool isAnyAnimationActive(void) override;

		virtual void setTimePosition(Ogre::Real timePosition) override;

		virtual Ogre::Real getTimePosition(void) const override;

		virtual Ogre::Real getLength(void) const override;

		virtual void setWeight(Ogre::Real weight) override;

		virtual Ogre::Real getWeight(void) const override;

		virtual void resetBones(void) override;

		virtual void setDebugLog(bool debugLog) override;

		virtual void setSourceEnabled(bool bEnable) override;

		virtual void addAnimationBlenderObserver(IAnimationBlenderObserver* observer) override;

		virtual void removeAnimationBlenderObserver(IAnimationBlenderObserver* observer) override;

		virtual void notifyObservers(void) override;

		virtual void deleteAllObservers(void) override;

		Ogre::v1::OldBone* getBone(const Ogre::String& boneName);

		void setBoneWeight(Ogre::v1::OldBone* bone, Ogre::Real weight);

		Ogre::v1::AnimationState* getAnimationState(AnimID animationId);

		Ogre::v1::AnimationState* getAnimationState(const Ogre::String& animationName);

		Ogre::v1::AnimationState* getSource(void);

		Ogre::v1::AnimationState* getTarget(void);

		Ogre::Vector3 getLocalToWorldPosition(Ogre::v1::OldBone* bone);

		Ogre::Quaternion getLocalToWorldOrientation(Ogre::v1::OldBone* bone);
	public:
		static void dumpAllAnimations(Ogre::Node* node, Ogre::String padding);
	protected:
		virtual void queueAnimationFinishedCallback(std::function<void()> callback) override;

		virtual void processDeferredCallbacks(void) override;
	private:
		void internalInit(const Ogre::String& animationName, bool loop = true);
		void internalBlend(const Ogre::String& animationName, BlendingTransition transition, Ogre::Real duration, bool loop = true);
		Ogre::v1::AnimationState* internalGetAnimationState(const Ogre::String& animationName);
		bool isTargetAnimationActive(AnimID animationId);

		void gameObjectIsInRagDollStateDelegate(EventDataPtr eventData);
	private:
		Ogre::v1::Entity* entity;
		Ogre::v1::AnimationState* target;

		Ogre::v1::AnimationState* source;
		Ogre::v1::AnimationState* previousSource;

		BlendingTransition transition;
		BlendingTransition previousTransition;
		
		Ogre::Real duration;
		Ogre::Real previousDuration;
		bool loop;
		bool previousLoop;
		
		Ogre::Real timeleft;
		bool complete;

		bool debugLog;

		std::map<AnimID, Ogre::String> mappedAnimations;

		std::vector<IAnimationBlenderObserver*> animationBlenderObservers;
		// Deferred callback queue
		std::vector<std::function<void()>> deferredCallbacks;
		bool canAnimate;

		unsigned long uniqueId;
	};

}; // namespace end

#endif
