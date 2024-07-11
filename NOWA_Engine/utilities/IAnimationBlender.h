#ifndef I_ANIMATIONBLENDER_H
#define I_ANIMATIONBLENDER_H

#include "defines.h"
#include "main/Events.h"

namespace NOWA
{

	/**
	* @class IAnimationBlender
	* @brief The animation blender utilities interface helps fade between two animations in a smooth way.
	*/
	class EXPORTED IAnimationBlender
	{
	public:

		/**
		* @class IAnimationBlenderObserver
		* @brief This interface can be implemented to react when an animation, that is started via blendAndContinue is finished.
		*/
		class EXPORTED IAnimationBlenderObserver
		{
		public:
			/**
			* @brief		Called when animation is finished
			*/
			virtual void onAnimationFinished(void) = 0;

			/**
			 * @brief		Gets whether the reaction should be done just once.
			 * @return		if true, this observer will be called only once.
			 */
			virtual bool shouldReactOneTime(void) const = 0;
		};

		enum BlendingTransition
		{
			BlendSwitch,         // End current animation and start a new one
			BlendWhileAnimating, // Fade from current animation to a new one
			BlendThenAnimate     // Fade the current animation to the first frame of the new one, after that execute the new animation
		};

		// all the animations our character has, and a null ID
		// some of these affect separate body parts and will be blended together
		enum AnimID
		{
			ANIM_IDLE_1,
			ANIM_IDLE_2,
			ANIM_IDLE_3,
			ANIM_IDLE_4,
			ANIM_IDLE_5,
			ANIM_WALK_NORTH,
			ANIM_WALK_SOUTH,
			ANIM_WALK_WEST,
			ANIM_WALK_EAST,
			ANIM_RUN,
			ANIM_CLIMB,
			ANIM_SNEAK,
			ANIM_HANDS_CLOSED,
			ANIM_HANDS_RELAXED,
			ANIM_DRAW_WEAPON,
			ANIM_SLICE_VERTICAL,
			ANIM_SLICE_HORIZONTAL,
			ANIM_JUMP_START,
			ANIM_JUMP_LOOP,
			ANIM_HIGH_JUMP_END,
			ANIM_JUMP_END,
			ANIM_JUMP_WALK,
			ANIM_FALL,
			ANIM_EAT_1,
			ANIM_EAT_2,
			ANIM_PICKUP_1,
			ANIM_PICKUP_2,
			ANIM_ATTACK_1,
			ANIM_ATTACK_2,
			ANIM_ATTACK_3,
			ANIM_ATTACK_4,
			ANIM_SWIM,
			ANIM_THROW_1,
			ANIM_THROW_2,
			ANIM_DEAD_1,
			ANIM_DEAD_2,
			ANIM_DEAD_3,
			ANIM_SPEAK_1,
			ANIM_SPEAK_2,
			ANIM_SLEEP,
			ANIM_DANCE,
			ANIM_DUCK,
			ANIM_CROUCH,
			ANIM_HALT,
			ANIM_ROAR,
			ANIM_SIGH,
			ANIM_GREETINGS,
			ANIM_NO_IDEA,
			ANIM_ACTION_1,
			ANIM_ACTION_2,
			ANIM_ACTION_3,
			ANIM_ACTION_4,
			ANIM_NONE
		};


		/**
		* @brief		Inits the animation blender, also sets and start the first current animation.
		* @param[in]	animationId		The animation id to start with
		* @param[in]	loop			Whether to loop the new animation or not.
		*/
		virtual void init(AnimID animationId, bool loop = true) = 0;

		/**
		* @brief		Inits the animation blender, also sets and start the first current animation.
		* @param[in]	animationName	The animation name to start with
		* @param[in]	loop			Whether to loop the new animation or not.
		*/
		virtual void init(const Ogre::String& animationName, bool loop = true) = 0;

		/**
		* @brief		Gets all available animation names.
		* @param[in]	skipLogging		Whether to skip the internal logging.
		* @return		A string list with all animation names. If none found, empty list will be delivered.
		*/
		virtual std::vector<Ogre::String> getAllAvailableAnimationNames(bool skipLogging = true) const = 0;

		/**
		 * @brief		Sets the animation blender observer to react when an animation, that is started via blendAndContinue is finished.
		 * @param[in]	animationBlenderObserver		The animation blender observer to set
		 */
		virtual void setAnimationBlenderObserver(IAnimationBlenderObserver* animationBlenderObserver) = 0;

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
		virtual void blend(AnimID animationId, BlendingTransition transition, Ogre::Real duration, bool loop) = 0;

		virtual void blend(const Ogre::String& animationName, BlendingTransition transition, Ogre::Real duration, bool loop) = 0;

		virtual void blend(AnimID animationId, BlendingTransition transition, bool loop) = 0;

		virtual void blend(const Ogre::String& animationName, BlendingTransition transition, bool loop) = 0;

		virtual void blend(AnimID animationId, BlendingTransition transition) = 0;

		virtual void blend(const Ogre::String& animationName, BlendingTransition transition) = 0;

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
		virtual void blendExclusive(AnimID animationId, BlendingTransition transition, Ogre::Real duration, bool loop) = 0;

		virtual void blendExclusive(const Ogre::String& animationName, BlendingTransition transition, Ogre::Real duration, bool loop) = 0;

		virtual void blendExclusive(AnimID animationId, BlendingTransition transition, bool loop) = 0;

		virtual void blendExclusive(const Ogre::String& animationName, BlendingTransition transition, bool loop) = 0;

		virtual void blendExclusive(AnimID animationId, BlendingTransition transition) = 0;

		virtual void blendExclusive(const Ogre::String& animationName, BlendingTransition transition) = 0;

		/**
		* @brief		Blends from the current animation to the new given one and when finished continues with the previous one.
		* @param[in]	animationId		The new animation to start
		* @param[in]	duration		The duration of the new animation. Its possible to also stop an animation earlier
		*/
		virtual void blendAndContinue(AnimID animationId, Ogre::Real duration) = 0;

		virtual void blendAndContinue(const Ogre::String& animationName, Ogre::Real duration) = 0;

		virtual void blendAndContinue(AnimID animationId) = 0;

		virtual void blendAndContinue(const Ogre::String& animationName) = 0;

		/**
		* @brief		Adds time to the animation blender to keep the animation process running.
		* @param[in]	time			The time amount.
		* @Note			This function should be called in each update function. Typical usage is as follows:
		*				animationBlender->addTime(dt * animationSpeed / animationBlender->getSource()->getLength())
		*/
		virtual void addTime(Ogre::Real time) = 0;

		/**
		* @brief		Gets the progress in seconds of the current animation.
		* @return		Progress		The progress of the current animation.
		*/
		virtual Ogre::Real getProgress(void) = 0;

		/**
		* @brief		Gets whether the current animation progress is completed.
		* @return		complete		True if the current animation progress is completed, else false.
		*/
		virtual bool isComplete(void) const = 0;

		/**
		* @brief		Registers and maps an animations string to an internal animation id for better and more generic animation handling
		* @param[in]	animationId			The animation id to map
		* @param[in]	animationName		The animation name that is mapped for to the animation id
		*/
		virtual void registerAnimation(AnimID animationId, const Ogre::String& animationName) = 0;

		/**
		 * @brief		Gets the animation id from string
		 * @param[in]	animationName		The animation name that is mapped fto get the animation id
		 * @return		animationId			The animation id, if not found for the given string NONE will be delivered
		 */
		virtual AnimID getAnimationIdFromString(const Ogre::String& animationName) = 0;

		/**
		 * @brief		Clears the mapped animations
		 */
		virtual void clearAnimations(void) = 0;

		virtual bool hasAnimation(const Ogre::String& animationName) = 0;

		virtual bool hasAnimation(AnimID animationId) = 0;

		virtual bool isAnimationActive(AnimID animationId) = 0;

		virtual bool isAnyAnimationActive(void) = 0;

		virtual void setTimePosition(Ogre::Real timePosition) = 0;

		virtual Ogre::Real getTimePosition(void) const = 0;

		virtual Ogre::Real getLength(void) const = 0;

		virtual void setWeight(Ogre::Real weight) = 0;

		virtual Ogre::Real getWeight(void) const = 0;

		virtual void resetBones(void) = 0;

		virtual void setDebugLog(bool debugLog) = 0;

		virtual void setSourceEnabled(bool bEnable) = 0;
	};

}; // namespace end

#endif
