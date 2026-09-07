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
        void internalSetOverlayAnimation(const Ogre::String& animationName, Ogre::Real blendInTime);

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

        Ogre::String addTimeOwner; // empty = unclaimed this frame
    };

}; // namespace end

#endif