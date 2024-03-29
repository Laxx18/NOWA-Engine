#ifndef ANIMATIONBLENDER_V2_H
#define ANIMATIONBLENDER_V2_H

#include "defines.h"
#include "IAnimationBlender.h"

namespace Ogre
{
	class SkeletonAnimation;
	class Bone;
}

namespace NOWA
{
	class GameObject;

	/**
	* @class AnimationBlenderV2
	* @brief The animation blender v2 utilities class helps fade between two animations in a smooth way for Ogre::Item objects.
	*/
	class EXPORTED AnimationBlenderV2 : public IAnimationBlender
	{
	public:

		/**
		* @brief		Creates the animation blender for the given item
		* @param[in]	item						The item to use the animation blender on.
		* @Note			All animations for this entity will be written to log in order to see which animations the item has.
		*/
		AnimationBlenderV2(Ogre::Item* item);

		~AnimationBlenderV2();

		/**
		* @brief		Inits the animation blender, also sets and start the first current animation.
		* @param[in]	animationId		The animation name to start with
		* @param[in]	loop			Whether to loop the new animation or not.
		*/
		void init(AnimID animationId, bool loop = true) override;
		
		void init(const Ogre::String& animationName, bool loop = true) override;

		/**
		 * @brief		Sets the animation blender observer to react when an animation, that is started via blendAndContinue is finished. 
		 * @param[in]	animationBlenderObserver		The animation blender observer to set
		 */
		void setAnimationBlenderObserver(IAnimationBlenderObserver* animationBlenderObserver) override;

		/**
		* @brief		Blends from the current animation to the new given one.
		* @param[in]	animationId		The new animation to start
		* @param[in]	transition		The transition from the current animation to the new one. There are 3 transition modes:
		*								1) BlendSwitch: End current animation and start a new one
		*								2) BlendWhileAnimating: Fade from current animation to a new one (most useful transition)
		*								3) BlendThenAnimate: Fade the current animation to the first frame of the new one, after that execute the new animation
		* @param[in]	duration		The duration of the new animation. Its possible to also stop an animation earlier
		* @param[in]	loop			Whether to loop the new animation or not.
		*/
		void blend(AnimID animationId, BlendingTransition transition, Ogre::Real duration, bool loop) override;
		
		void blend(const Ogre::String& animationName, BlendingTransition transition, Ogre::Real duration, bool loop) override;

		void blend(AnimID animationId, BlendingTransition transition, bool loop) override;

		void blend(const Ogre::String& animationName, BlendingTransition transition, bool loop) override;

		void blend(AnimID animationId, BlendingTransition transition) override;

		void blend(const Ogre::String& animationName, BlendingTransition transition) override;

		/**
		* @brief		Blends from the current animation to the new given one exclusively. That means, blendExclusive can be used in an update method, because it only blends if the animation is not active.
		* @param[in]	animationId		The new animation to start
		* @param[in]	transition		The transition from the current animation to the new one. There are 3 transition modes:
		*								1) BlendSwitch: End current animation and start a new one
		*								2) BlendWhileAnimating: Fade from current animation to a new one (most useful transition)
		*								3) BlendThenAnimate: Fade the current animation to the first frame of the new one, after that execute the new animation
		* @param[in]	duration		The duration of the new animation. Its possible to also stop an animation earlier
		* @param[in]	loop			Whether to loop the new animation or not.
		*/
		void blendExclusive(AnimID animationId, BlendingTransition transition, Ogre::Real duration, bool loop) override;

		void blendExclusive(const Ogre::String& animationName, BlendingTransition transition, Ogre::Real duration, bool loop) override;

		void blendExclusive(AnimID animationId, BlendingTransition transition, bool loop) override;

		void blendExclusive(const Ogre::String& animationName, BlendingTransition transition, bool loop) override;

		void blendExclusive(AnimID animationId, BlendingTransition transition) override;

		void blendExclusive(const Ogre::String& animationName, BlendingTransition transition) override;

		/**
		* @brief		Blends from the current animation to the new given one and when finished continues with the previous one.
		* @param[in]	animationId		The new animation to start
		* @param[in]	duration		The duration of the new animation. Its possible to also stop an animation earlier
		*/
		void blendAndContinue(AnimID animationId, Ogre::Real duration) override;

		void blendAndContinue(const Ogre::String& animationName, Ogre::Real duration) override;

		void blendAndContinue(AnimID animationId) override;

		void blendAndContinue(const Ogre::String& animationName) override;

		/**
		* @brief		Adds time to the animation blender to keep the animation process running.
		* @param[in]	time			The time amount.
		* @Note			This function should be called in each update function. Typical usage is as follows:
		*				animationBlender->addTime(dt * animationSpeed / animationBlender->getSource()->getLength())
		*/
		void addTime(Ogre::Real time) override;

		/**
		* @brief		Gets the progress in seconds of the current animation.
		* @return		Progress		The progress of the current animation.
		*/
		Ogre::Real getProgress(void) override;

		/**
		* @brief		Gets the current animation state
		* @return		source			The source animation state
		*/
		Ogre::SkeletonAnimation* getSource(void);

		/**
		* @brief		Gets the target animation state
		* @return		source			The target animation state
		*/
		Ogre::SkeletonAnimation* getTarget(void);

		/**
		* @brief		Gets whether the current animation progress is completed.
		* @return		complete		True if the current animation progress is completed, else false.
		*/
		bool isComplete(void) const override;

		 /**
		 * @brief		Registers and maps an animations string to an internal animation id for better and more generic animation handling
		 * @param[in]	animationId			The animation id to map
		 * @param[in]	animationName		The animation name that is mapped for to the animation id
		 */
		void registerAnimation(AnimID animationId, const Ogre::String& animationName) override;

		/**
		 * @brief		Gets the animation id from string
		 * @param[in]	animationName		The animation name that is mapped fto get the animation id
		 * @return		animationId			The animation id, if not found for the given string NONE will be delivered
		 */
		AnimID getAnimationIdFromString(const Ogre::String& animationName) override;

		/**
		 * @brief		Clears the mapped animations
		 */
		void clearAnimations(void) override;

		bool hasAnimation(const Ogre::String& animationName) override;

		bool hasAnimation(AnimID animationId) override;

		bool isAnimationActive(AnimID animationId) override;
		
		Ogre::SkeletonAnimation* getAnimationState(AnimID animationId);
		
		Ogre::SkeletonAnimation* getAnimationState(const Ogre::String& animationName);

		void setTimePosition(Ogre::Real timePosition) override;

		Ogre::Real getTimePosition(void) const override;

		Ogre::Real getLength(void) const override;

		void setWeight(Ogre::Real weight) override;

		Ogre::Real getWeight(void) const override;

		Ogre::Bone* getBone(const Ogre::String& boneName);

		void resetBones(void) override;

		void setDebugLog(bool debugLog) override;

		void setSourceEnabled(bool bEnable) override;

		Ogre::Vector3 getLocalToWorldPosition(Ogre::Bone* bone);

		Ogre::Quaternion getLocalToWorldOrientation(Ogre::Bone* bone);
	public:
		static void dumpAllAnimations(Ogre::Node* node, Ogre::String padding);
	private:
		void internalInit(const Ogre::String& animationName, bool loop = true);
		void internalBlend(const Ogre::String& animationName, BlendingTransition transition, Ogre::Real duration, bool loop = true);
		Ogre::SkeletonAnimation* internalGetAnimationState(const Ogre::String& animationName);
		bool isTargetAnimationActive(AnimID animationId);

		void gameObjectIsInRagDollStateDelegate(EventDataPtr eventData);
	private:
		Ogre::Item* item;
		Ogre::SkeletonInstance* skeleton;
		Ogre::SkeletonAnimation* target;

		Ogre::SkeletonAnimation* source;
		Ogre::SkeletonAnimation* previousSource;

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

		IAnimationBlenderObserver* animationBlenderObserver;
		bool skipReactOnAnimation;
		bool canAnimate;
	};

}; // namespace end

#endif
