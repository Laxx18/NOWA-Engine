#ifndef ANIMATIONBLENDER_V2_H
#define ANIMATIONBLENDER_V2_H

#include "defines.h"
#include "main/Events.h"

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
    class EXPORTED AnimationBlenderV2
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
            ANIM_GETUP,
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
            ANIM_PULL,
            ANIM_PUSH,
            ANIM_KNOCK_DOWN,
            ANIM_STAND_UP,
            ANIM_TALK_1,
            ANIM_TALK_2,
            ANIM_POINT,
            ANIM_LAUGH,
            ANIM_LAND_1,
            ANIM_LAND_2,
            ANIM_SHOOT,
            ANIM_START_CLIMB,
            ANIM_TAKE_DAMAGE,
            ANIM_SHRUG,
            ANIM_SALTO,
            ANIM_CRY,
            ANIM_CHEER,
            ANIM_CAST_SPELL_1,
            ANIM_CAST_SPELL_2,
            ANIM_CAST_SPELL_3,
            ANIM_NONE
        };

        struct BlendSpaceEntry
        {
            AnimID animationId;
            Ogre::Real parameter; // e.g. speed value this clip represents
        };

        /**
         * @class BlendSpaceEntryList
         * @brief Lua-constructible list of blend space entries. Build it once in
         *        connect(), keep it as a variable, and pass it to driveBlendSpace()
         *        every frame from execute().
         */
        class BlendSpaceEntryList
        {
        public:
            void add(AnimID animationId, Ogre::Real parameter)
            {
                BlendSpaceEntry entry;
                entry.animationId = animationId;
                entry.parameter = parameter;
                this->entries.push_back(entry);
            }

            void clear()
            {
                this->entries.clear();
            }

            size_t size() const
            {
                return this->entries.size();
            }

            const std::vector<BlendSpaceEntry>& getEntries() const
            {
                return this->entries;
            }

        private:
            std::vector<BlendSpaceEntry> entries;
        };

    public:
        /**
         * @brief		Creates the animation blender for the given item.
         * @param[in]	item						The item to use the animation blender on.
         * @Note			All animations for this entity will be written to log in order to see which animations the item has.
         */
        AnimationBlenderV2(Ogre::Item* item);

        ~AnimationBlenderV2();

        void init(AnimID animationId, bool loop = true);

        void init(const Ogre::String& animationName, bool loop = true);

        std::vector<Ogre::String> getAllAvailableAnimationNames(bool skipLogging = true);

        void blend(AnimID animationId, BlendingTransition transition, Ogre::Real duration, bool loop);

        void blend(const Ogre::String& animationName, BlendingTransition transition, Ogre::Real duration, bool loop);

        void blend(AnimID animationId, BlendingTransition transition, bool loop);

        void blend(const Ogre::String& animationName, BlendingTransition transition, bool loop);

        void blend(AnimID animationId, BlendingTransition transition);

        void blend(const Ogre::String& animationName, BlendingTransition transition);

        void blendPhaseSynced(AnimID animationId, BlendingTransition transition, Ogre::Real duration, bool loop);

        void blendPhaseSynced(const Ogre::String& animationName, BlendingTransition transition, Ogre::Real duration, bool loop);

        void blendExclusive(AnimID animationId, BlendingTransition transition, Ogre::Real duration, bool loop);

        void blendExclusive(const Ogre::String& animationName, BlendingTransition transition, Ogre::Real duration, bool loop);

        void blendExclusive(AnimID animationId, BlendingTransition transition, bool loop);

        void blendExclusive(const Ogre::String& animationName, BlendingTransition transition, bool loop);

        void blendExclusive(AnimID animationId, BlendingTransition transition);

        void blendExclusive(const Ogre::String& animationName, BlendingTransition transition);

        void blendAndContinue(AnimID animationId, Ogre::Real duration);

        void blendAndContinue(const Ogre::String& animationName, Ogre::Real duration);

        void blendAndContinue(AnimID animationId);

        void blendAndContinue(const Ogre::String& animationName);

        void setOverlayAnimation(AnimID animationId, Ogre::Real blendInTime = 0.2f);

        void setOverlayAnimation(const Ogre::String& animationName, Ogre::Real blendInTime = 0.2f);

        void clearOverlayAnimation(Ogre::Real blendOutTime = 0.2f);

        bool isOverlayAnimationActive(void) const;

        /**
         * @brief	Starts an overlay animation on top of the current one and optionally loops it.
         * @param	animationId		The clip to overlay
         * @param	blendInTime		How long the overlay fades in, in seconds. The same value is
         *							used to fade it out again when a non looping clip is over.
         * @param	loop			true keeps the overlay running until clearOverlayAnimation() is
         *							called, false ends it when the clip reaches its end.
         * @Note	The bones the overlay takes over are the bones the CLIP ITSELF animates. That is
         *			only the upper body if the clip really only keys the upper body - a full body
         *			mocap punch keys the legs and the root too and then takes the whole skeleton away
         *			from the locomotion clip. Use setOverlayAnimationForBoneChain() for those.
         */
        void setOverlayAnimation(AnimID animationId, Ogre::Real blendInTime, bool loop);

        void setOverlayAnimation(const Ogre::String& animationName, Ogre::Real blendInTime, bool loop);

        /**
         * @brief	Starts an overlay animation that only drives one bone chain, so the rest of the
         *			skeleton keeps playing the main animation. This is the "upper body attack, lower
         *			body locomotion" case.
         * @param	animationId		The clip to overlay
         * @param	rootBoneName	The first bone of the chain the overlay owns, for example the
         *							lowest spine bone. That bone and ALL of its children are driven
         *							by the overlay, everything else keeps playing the main animation.
         *							An empty or unknown name falls back to the bones of the clip.
         * @param	blendInTime		How long the overlay fades in, in seconds
         * @param	loop			Whether the overlay loops
         */
        void setOverlayAnimationForBoneChain(AnimID animationId, const Ogre::String& rootBoneName, Ogre::Real blendInTime, bool loop);

        void setOverlayAnimationForBoneChain(const Ogre::String& animationName, const Ogre::String& rootBoneName, Ogre::Real blendInTime, bool loop);

        /**
         * @brief	Gets the time position of the OVERLAY clip in seconds.
         * @Note	getTimePosition() cannot be used for this, it reports the main animation, which
         *			is the walk cycle for as long as the overlay runs.
         */
        Ogre::Real getOverlayTimePosition(void) const;

        /**
         * @brief	Gets the length of the overlay clip in seconds, or 0 when there is none.
         */
        Ogre::Real getOverlayLength(void) const;

        /**
         * @brief	Gets how far the overlay clip has come, 0 at its first frame and 1 at its last.
         *			This is what a script times a hit window or a follow up window on.
         */
        Ogre::Real getOverlayProgress(void) const;

        /**
         * @brief	Sets the time position of the overlay clip, for example to restart a swing.
         */
        void setOverlayTimePosition(Ogre::Real timePosition);

        /**
         * @brief	Gets whether the overlay has reached its end and is fading out. isOverlayAnimationActive()
         *			is still true during that fade, so this is the one to test for "the swing is over".
         */
        bool isOverlayBlendingOut(void) const;

        /**
         * @brief	Sets the playback speed of the OVERLAY only. 1.0 is the authored speed.
         * @Note	setAnimationSpeed() deliberately does not touch the overlay. That one is the
         *			locomotion speed the player controller drives from the walking speed, and it
         *			used to be pushed onto the overlay as well - a 0.87 second punch then took two
         *			and a half seconds, because the legs were moving slowly.
         */
        /**
         * @brief	Sets how strongly the overlay takes over, separately for the chain it owns and
         *			for the rest of the skeleton.
         * @param	chainInfluence			0 to 1, default 1. How much of the chain the overlay
         *									takes. 1 means the locomotion clip has no say there at
         *									all, which is what a punch wants.
         * @param	outsideChainInfluence	0 to 1, default 0. How much the overlay reaches into the
         *									REST of the skeleton - the pelvis and the legs. 0 keeps
         *									the locomotion untouched below the chain; a small value
         *									like 0.3 lets the whole body lean into the action while
         *									the legs keep walking, which makes a punch read as much
         *									heavier. Attention: the pelvis is where a mocap clip
         *									carries its root motion, so high values pull the
         *									character around - 0.4 is about the sensible ceiling.
         * @Note	Whatever the overlay takes, the main animation gets the rest, at every moment of
         *			the fade. Ogre-Next accumulates both onto the same bone, so the two weights have
         *			to add up to one or the pose is either doubled or missing.
         */
        void setOverlayInfluence(Ogre::Real chainInfluence, Ogre::Real outsideChainInfluence);

        Ogre::Real getOverlayChainInfluence(void) const;

        Ogre::Real getOverlayOutsideInfluence(void) const;

        void setOverlaySpeed(Ogre::Real speed);

        Ogre::Real getOverlaySpeed(void) const;

        void driveBlendSpace(Ogre::Real parameter, const AnimationBlenderV2::BlendSpaceEntryList& entryList);

        void addTime(Ogre::Real time, const Ogre::String& ownerId);

        Ogre::Real getProgress(void);

        bool isComplete(void) const;

        void registerAnimation(AnimID animationId, const Ogre::String& animationName);

        AnimID getAnimationIdFromString(const Ogre::String& animationName);

        void clearAnimations(void);

        bool hasAnimation(const Ogre::String& animationName);

        bool hasAnimation(AnimID animationId);

        bool isAnimationActive(AnimID animationId);

        bool isAnyAnimationActive(void);

        void setTimePosition(Ogre::Real timePosition);

        Ogre::Real getTimePosition(void) const;

        Ogre::Real getLength(void) const;

        void setWeight(Ogre::Real weight);

        Ogre::Real getWeight(void) const;

        void resetBones(void);

        void setDebugLog(bool debugLog);

        void setSourceEnabled(bool bEnable);

        void setAnimationSpeed(Ogre::Real speed);

        Ogre::Real getAnimationSpeed(void) const;

        void addAnimationBlenderObserver(IAnimationBlenderObserver* observer);

        void removeAnimationBlenderObserver(IAnimationBlenderObserver* observer);

        void notifyObservers(void);

        void deleteAllObservers(void);

        void resetBlendState(void);

        void beginFrame(void);

        Ogre::SkeletonAnimation* getAnimationState(AnimID animationId);

        Ogre::SkeletonAnimation* getAnimationState(const Ogre::String& animationName);

        Ogre::SkeletonAnimation* getSource(void);

        Ogre::SkeletonAnimation* getTarget(void);

        Ogre::Bone* getBone(const Ogre::String& boneName);

        Ogre::Vector3 getLocalToWorldPosition(Ogre::Bone* bone);

        Ogre::Quaternion getLocalToWorldOrientation(Ogre::Bone* bone);

    public:
        static void dumpAllAnimations(Ogre::Node* node, Ogre::String padding);

    protected:
        void queueAnimationFinishedCallback(std::function<void()> callback);

        void processDeferredCallbacks(void);

    private:
        void internalInit(const Ogre::String& animationName, bool loop = true);
        // phaseSync defaults to false: every existing call site keeps the safe behaviour of
        // starting the incoming animation at its beginning. Only blendPhaseSynced() opts in.
        void internalBlend(const Ogre::String& animationName, BlendingTransition transition, Ogre::Real duration, bool loop = true, bool phaseSync = false);

        Ogre::SkeletonAnimation* internalGetAnimationState(const Ogre::String& animationName);

        bool isTargetAnimationActive(AnimID animationId);
        void internalSetOverlayAnimation(const Ogre::String& animationName, const Ogre::String& maskRootBoneName, Ogre::Real blendInTime, bool loop);

        void internalUpdateOverlay(Ogre::Real renderDt);

        // The overlay clip itself: full weight on the chain it owns, zero everywhere else.
        void internalSetOverlayOwnBoneWeights(bool restore);

        // Hands the chain over to the overlay and back again, following the overlay's own weight
        // every frame, so the chain is never driven by two clips at once and never by none.
        void internalDriveOverlayChainWeight(Ogre::Real overlayAuthority);

        void internalRestoreOverlayChainWeight(void);

        void internalCollectBoneChain(Ogre::Bone* bone, std::vector<Ogre::String>& outBoneNames);

        // Only used by the debug log: prints the hierarchy indented and marks every bone the
        // overlay currently owns.
        void internalLogBoneChain(Ogre::Bone* bone, const Ogre::String& padding);

        bool tryClaimAddTime(const Ogre::String& ownerId); // returns true if caller won

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
        std::atomic<bool> complete;
        std::atomic<bool> canAnimate;

        bool debugLog;

        std::map<AnimID, Ogre::String> mappedAnimations;
        std::map<Ogre::String, Ogre::Real> baseFrameRates;
        Ogre::Real currentSpeed;

        std::vector<IAnimationBlenderObserver*> animationBlenderObservers;
        // Deferred callback queue
        std::vector<std::function<void()>> deferredCallbacks;

        unsigned long uniqueId;

        Ogre::SkeletonAnimation* overlaySource;
        Ogre::Real overlayTimeleft;
        Ogre::Real overlayDuration;
        bool overlayBlendingOut;
        bool overlayLoop;
        Ogre::Real overlayBlendOutTime;
        Ogre::Real overlaySpeed;
        Ogre::Real overlayChainInfluence;
        Ogre::Real overlayOutsideInfluence;
        // The bones the overlay owns, and the whole skeleton. Both are needed: muting the other
        // animations on the chain alone still leaves the overlay at its default weight of 1.0 on
        // every bone OUTSIDE the chain, so the punch would drive the legs as well.
        // Names only for the debug log. The ids are what the per frame code uses - hashing the
        // bone names again on every frame would be pure waste.
        std::vector<Ogre::String> overlayMaskBoneNames;
        std::vector<Ogre::String> overlayAllBoneNames;
        std::vector<Ogre::IdString> overlayChainBoneIds;
        std::vector<Ogre::IdString> overlayOtherBoneIds;
        // Every animation whose chain weight was lowered for this overlay, so all of them can be
        // restored - not just the one that happens to be the source when the overlay ends.
        std::vector<Ogre::SkeletonAnimation*> overlayMutedAnimations;

        Ogre::String addTimeOwner; // empty = unclaimed this frame
    };

}; // namespace end

#endif