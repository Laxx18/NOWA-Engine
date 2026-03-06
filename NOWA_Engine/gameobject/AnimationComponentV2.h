/*
Copyright (c) 2023 Lukas Kalinowski

GPL v3
*/

#ifndef ANIMATIONCOMPONENTV2_H
#define ANIMATIONCOMPONENTV2_H

#include "gameobject/GameObjectComponent.h"
#include "utilities/AnimationBlenderV2.h"
#include "main/Events.h"

namespace Ogre
{
	class SkeletonAnimation;
	class Bone;
}

/*

-- ============================================================
-- Setup (in connect)
-- ============================================================

animationBlender:registerAnimation(AnimID.ANIM_IDLE_1,    "Idle")
animationBlender:registerAnimation(AnimID.ANIM_WALK_NORTH, "Walk")
animationBlender:registerAnimation(AnimID.ANIM_RUN,        "Run")
animationBlender:registerAnimation(AnimID.ANIM_ATTACK_1,   "Melee_attack")
animationBlender:registerAnimation(AnimID.ANIM_ATTACK_2,   "Melee_attack2")
animationBlender:registerAnimation(AnimID.ANIM_SHOOT,      "Shoot")
animationBlender:registerAnimation(AnimID.ANIM_DEAD_1,     "Dead")

animationBlender:init1(AnimID.ANIM_IDLE_1, true)

-- ============================================================
-- setAnimationSpeed / getAnimationSpeed
-- ============================================================

-- Double speed globally (e.g. slow-mo reversal, haste buff)
animationBlender:setAnimationSpeed(2.0)
log("Current speed: " .. animationBlender:getAnimationSpeed()) -- 2.0

-- Now just pass raw dt - speed is baked into mFrameRate internally
-- animationBlender:addTime(dt)

-- Restore normal speed
animationBlender:setAnimationSpeed(1.0)

-- ============================================================
-- setOverlayAnimation — non-looping one-shot (auto-clears)
-- ============================================================

-- Fire a melee attack overlay on the upper body while walk loop
-- keeps playing on the legs. blendInTime = 0.15 seconds.
-- Since Melee_attack is non-looping it will auto-clear when done.
animationBlender:setOverlayAnimation1(AnimID.ANIM_ATTACK_1, 0.15)

-- String variant:
-- animationBlender:setOverlayAnimation2("Melee_attack", 0.15)

-- Guard against spamming a new overlay before the current one finishes:
if animationBlender:isOverlayAnimationActive() == false then
    animationBlender:setOverlayAnimation1(AnimID.ANIM_ATTACK_2, 0.15)
end

-- ============================================================
-- setOverlayAnimation — looping overlay (must be cleared manually)
-- ============================================================

-- Start a looping aim/shoot overlay (upper body only)
animationBlender:setOverlayAnimation1(AnimID.ANIM_SHOOT, 0.2)

-- Later, when the player stops aiming, fade it out over 0.2s
animationBlender:clearOverlayAnimation(0.2)

-- ============================================================
-- Chaining overlay with blendAndContinue
-- ============================================================

-- Play a pickup animation, then return to whatever was playing before,
-- while an attack overlay can still fire independently on upper body
animationBlender:blendAndContinue1(AnimID.ANIM_PICKUP_1)

playerController:reactOnAnimationFinished(function()
    log("Pickup finished, resuming previous animation")
    -- overlay is independent — it may still be active here
end, true)

-- ============================================================
-- Typical execute loop pattern
-- ============================================================

-- execute = function(gameObject, dt)

    -- Speed changes respond immediately to gameplay state
    if (isSlowMotion) then
        animationBlender:setAnimationSpeed(0.3)
    elseif (isHasted) then
        animationBlender:setAnimationSpeed(1.8)
    else
        animationBlender:setAnimationSpeed(1.0)
    end

    -- Overlay attack on upper body, locomotion continues on legs
    if (InputDeviceModule:isActionDown(NOWA_A_ATTACK_1) and
        animationBlender:isOverlayAnimationActive() == false) then
        animationBlender:setOverlayAnimation1(AnimID.ANIM_ATTACK_1, 0.1)
    end

    -- Advance animation with raw dt — speed is handled by setAnimationSpeed
    animationBlender:addTime(dt)

-- end

Scenario 2:

-- ============================================================
-- connect() — build the blend space once, store as variable
-- ============================================================

locomotionBlendSpace = BlendSpaceEntryList.new()
locomotionBlendSpace:add(AnimID.ANIM_IDLE_1,     0.0)   -- standing still
locomotionBlendSpace:add(AnimID.ANIM_WALK_NORTH,  3.0)   -- walking
locomotionBlendSpace:add(AnimID.ANIM_RUN,         8.0)   -- running

animationBlender:init1(AnimID.ANIM_IDLE_1, true)

-- ============================================================
-- execute(gameObject, dt) — drive it every frame
-- ============================================================

local velocity = playerController:getPhysicsComponent():getVelocity()
-- Use only horizontal speed for the blend parameter
local horizontalSpeed = math.sqrt(velocity.x * velocity.x + velocity.z * velocity.z)

animationBlender:driveBlendSpace(horizontalSpeed, locomotionBlendSpace)

-- Overlay attacks on upper body independently — blend space keeps running on legs
if (InputDeviceModule:isActionDown(NOWA_A_ATTACK_1) and
    animationBlender:isOverlayAnimationActive() == false) then
    animationBlender:setOverlayAnimation1(AnimID.ANIM_ATTACK_1, 0.1)
end

-- Always advance with raw dt
animationBlender:addTime(dt)
*/

namespace NOWA
{
	/**
	  * @brief		Play one animation with Ogre Item (v2). Requirements: Item must have a skeleton with animations.
	  */
	class EXPORTED AnimationComponentV2 : public GameObjectComponent
	{
	public:
		typedef boost::shared_ptr<AnimationComponentV2> AnimationComponentV2Ptr;
	public:
		class EXPORTED AnimationBlenderObserver : public AnimationBlenderV2::IAnimationBlenderObserver
		{
		public:
			AnimationBlenderObserver(luabind::object closureFunction, bool oneTime);

			virtual ~AnimationBlenderObserver();

			virtual void onAnimationFinished(void) override;

			virtual bool shouldReactOneTime(void) const override;

			void setNewFunctionName(luabind::object closureFunction, bool oneTime);

		private:
			luabind::object closureFunction;
			bool oneTime;
		};
	public:

		AnimationComponentV2();

		virtual ~AnimationComponentV2();

		/**
		* @see		GameObjectComponent::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

		/**
		* @see		GameObjectComponent::postInit
		*/
		virtual bool postInit(void) override;

		/**
		* @see		GameObjectComponent::connect
		*/
		virtual bool connect(void) override;

		/**
		* @see		GameObjectComponent::disconnect
		*/
		virtual bool disconnect(void) override;

		/**
		* @see		GameObjectComponent::onCloned
		*/
		virtual bool onCloned(void) override;

		/**
		* @see		GameObjectComponent::onRemoveComponent
		*/
		virtual void onRemoveComponent(void);
		
		/**
		 * @see		GameObjectComponent::onOtherComponentRemoved
		 */
		virtual void onOtherComponentRemoved(unsigned int index) override;

		/**
		 * @see		GameObjectComponent::onOtherComponentAdded
		 */
		virtual void onOtherComponentAdded(unsigned int index) override;

		/**
		* @see		GameObjectComponent::getClassName
		*/
		virtual Ogre::String getClassName(void) const override;

		/**
		* @see		GameObjectComponent::getParentClassName
		*/
		virtual Ogre::String getParentClassName(void) const override;

		/**
		* @see		GameObjectComponent::clone
		*/
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		/**
		* @see		GameObjectComponent::update
		*/
		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		* @see		GameObjectComponent::setActivated
		*/
		virtual void setActivated(bool activated) override;

		/**
		* @see		GameObjectComponent::isActivated
		*/
		virtual bool isActivated(void) const override;

		void setAnimationName(const Ogre::String& animationName);

		const Ogre::String getAnimationName(void) const;

		void setSpeed(Ogre::Real animationSpeed);

		Ogre::Real getSpeed(void) const;

		void setRepeat(bool animationRepeat);

		bool getRepeat(void) const;

		void activateAnimation(void);

		bool isComplete(void) const;

		AnimationBlenderV2* getAnimationBlender(void) const;

		Ogre::Bone* getBone(const Ogre::String& boneName);

		Ogre::Vector3 getLocalToWorldPosition(Ogre::Bone* bone);

		Ogre::Quaternion getLocalToWorldOrientation(Ogre::Bone* bone);

		// These methods are just for lua and are not stored for an editor

		void setTimePosition(Ogre::Real timePosition);

		Ogre::Real getTimePosition(void) const;

		Ogre::Real getLength(void) const;

		void setWeight(Ogre::Real weight);

		Ogre::Real getWeight(void) const;

		void reactOnAnimationFinished(luabind::object closureFunction, bool oneTime);

	public:
		/**
		* @see		GameObjectComponent::getStaticClassId
		*/
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("AnimationComponentV2");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "AnimationComponentV2";
		}
	
		/**
		* @see		GameObjectComponent::canStaticAddComponent
		*/
		static bool canStaticAddComponent(GameObject* gameObject);

		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: Play one animation with Ogre Item (v2). Requirements: Item must have a skeleton with animations.";
		}
		
		/**
		 * @see	GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrName(void) { return "Animation Name"; }
		static const Ogre::String AttrSpeed(void) { return "Speed"; }
		static const Ogre::String AttrRepeat(void) { return "Repeat"; }
	private:
		void resetAnimation(void);
		void createAnimationBlender(void);
	protected:
		Ogre::SkeletonInstance* skeleton;
		Variant* activated;
		Variant* animationName;
		Variant* animationSpeed;
		Variant* animationRepeat;
		AnimationBlenderV2* animationBlender;
	};

}; // namespace end

#endif

