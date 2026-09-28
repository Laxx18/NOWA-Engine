module("Rhino", package.seeall);
-- Scene: Level1

require("init");

local rhino = nil;
-- The id is kept separately on purpose. Event handlers compare against THIS, never against
-- rhino:getId(): an event can still reach this script after the game object was deleted, and then
-- 'rhino' points to freed memory - calling any function on it crashes the engine.
local rhinoId = nil;
local animationBlender = nil;

-- Handle returned by registerEventListener. removeEventListener takes exactly this id.
local enemyDeadListenerId = nil;

-- Death sequence
--
-- The rhino does NOT go into the ragdoll in the very same frame the killing blow lands. Two
-- reasons, both of them the "he always flies off in the wrong direction" bug:
--
-- 1. AiComponent::setActivated(false) does not stop the MovingBehavior right away. It removes the
--    behavior through a DelayProcess of 0.25 seconds, and until then the MovingBehavior keeps
--    sending applyRequiredForceForVelocity() with the rhino's WALKING velocity every frame. That
--    command is latched inside PhysicsActiveComponent::moveCallback and re-applied on every physics
--    substep, so the corpse kept walking in its old path direction - usually straight towards the
--    player, i.e. exactly the opposite of a knockback.
--
-- 2. The former push itself used applyRequiredForceForVelocity(), which is latched as well. The
--    body was therefore not pushed ONCE, it was held at a constant velocity of (x, 2, 0) for as long
--    as the latch lived.
--
-- Now: the path follow is switched off at once, the rhino shows its damage animation for a short
-- stun, and only after DEATH_PUSH_DELAY (longer than the 0.25 seconds of the delayed removal) the
-- latched values are cleared with resetForce(), the ragdoll is switched on and a ONE SHOT impulse
-- (applyRequiredForceForJumpVelocity) throws the body away from the hit.
local DEATH_PUSH_DELAY = 0.3;
local DEATH_KNOCKBACK_HORIZONTAL = 4.0;
local DEATH_KNOCKBACK_UP = 3.0;
local DEATH_DELETE_DELAY = 2.0;

local isDying = false;
local dyingTime = 0;
local deathPushDone = false;
local deathDirectionX = 1;

-- Logged only once per session, so a missing binding does not flood the log.
local missingApiLogged = false;

Rhino = {}

Rhino["connect"] = function(gameObject)
    rhino = AppStateManager:getGameObjectController():castGameObject(gameObject);
    rhinoId = rhino:getId();
    animationBlender = rhino:getAnimationComponentV2():getAnimationBlender();

    animationBlender:registerAnimation(AnimationBlender.ANIM_IDLE_1, "Rhino_Idle");
    animationBlender:registerAnimation(AnimationBlender.ANIM_IDLE_2, "Rhino_Idle_2");
    animationBlender:registerAnimation(AnimationBlender.ANIM_WALK_NORTH, "Rhino_Walk_In_Place");
    animationBlender:registerAnimation(AnimationBlender.ANIM_JUMP_START, "Rhino_Jump_Up");
    animationBlender:registerAnimation(AnimationBlender.ANIM_JUMP_WALK, "Rhino_Walk_In_Place");
    animationBlender:registerAnimation(AnimationBlender.ANIM_HIGH_JUMP_END, "Rhino_Land");
    animationBlender:registerAnimation(AnimationBlender.ANIM_JUMP_END, "Rhino_Land");
    animationBlender:registerAnimation(AnimationBlender.ANIM_FALL, "Rhino_Fall");
    animationBlender:registerAnimation(AnimationBlender.ANIM_RUN, "Rhino_Run_In_Place");
    animationBlender:registerAnimation(AnimationBlender.ANIM_ATTACK_1, "Rhino_Kick");
    animationBlender:registerAnimation(AnimationBlender.ANIM_ATTACK_2, "Rhino_Roll_In_Place");
    animationBlender:registerAnimation(AnimationBlender.ANIM_TAKE_DAMAGE, "Rhino_Damage");

    isDying = false;
    dyingTime = 0;
    deathPushDone = false;
    deathDirectionX = 1;

    -- WeaponStick.lua fires this event (with enemyId and hitDirection) the moment this rhino's
    -- Energy attribute reaches 0.
    if (EventType.EnemyDeadEvent ~= nil) then
        enemyDeadListenerId = AppStateManager:getScriptEventManager():registerEventListener(EventType.EnemyDeadEvent, Rhino["onEnemyDead"]);
    end
end

Rhino["disconnect"] = function()
    -- Called at simulation stop AND when the rhino is deleted while the simulation runs (killed).
    -- The listener must go, otherwise the next EnemyDeadEvent still calls into this script.
    if (enemyDeadListenerId ~= nil) then
        AppStateManager:getScriptEventManager():removeEventListener(enemyDeadListenerId);
        enemyDeadListenerId = nil;
    end

    rhino = nil;
    rhinoId = nil;
    animationBlender = nil;
    isDying = false;
    dyingTime = 0;
    deathPushDone = false;
end

Rhino["onEnemyDead"] = function(eventData)
    if (rhino == nil) then
        do return end;
    end

    -- The event fires for ANY killed enemy - only react if it's this one.
    if (eventData["enemyId"] ~= rhinoId) then
        do return end;
    end

    -- The weapon script and the player script can both report the same kill. Only the first one
    -- counts, a second ragdoll switch and push on a corpse would throw it around again.
    if (true == isDying) then
        do return end;
    end

    isDying = true;
    dyingTime = 0;
    deathPushDone = false;

    -- Only the horizontal part matters in this 2.5D level: away from the player.
    deathDirectionX = 1;
    local hitDirection = eventData["hitDirection"];
    if (hitDirection ~= nil and hitDirection.x < 0) then
        deathDirectionX = -1;
    end

    -- Stop the path follow right away. See the block at the top: the MovingBehavior is only removed
    -- 0.25 seconds later, which is why the push itself waits in update().
    local pathFollowComponent = rhino:getAiPathFollowComponent();
    if (pathFollowComponent ~= nil) then
        pathFollowComponent:setActivated(false);
    end

    -- Short stun before the body goes limp. It also reads much better than an instant ragdoll.
    if (animationBlender ~= nil) then
        animationBlender:blend5(AnimationBlender.ANIM_TAKE_DAMAGE, AnimationBlender.BLEND_WHILE_ANIMATING, 0.05, false);
    end

    AppStateManager:getGameObjectController():deleteDelayedGameObject(rhinoId, DEATH_PUSH_DELAY + DEATH_DELETE_DELAY);
end

---------------------------------------------------------------------------------------------------
-- Called by the LuaScriptComponent every frame. Only used for the death sequence.
---------------------------------------------------------------------------------------------------
Rhino["update"] = function(dt)
    if (rhino == nil or false == isDying or true == deathPushDone) then
        do return end;
    end

    dyingTime = dyingTime + dt;
    if (dyingTime < DEATH_PUSH_DELAY) then
        do return end;
    end

    deathPushDone = true;

    local ragDollComponent = rhino:getPhysicsRagDollComponentV2();
    if (ragDollComponent == nil) then
        do return end;
    end

    -- Clears the velocity AND the latched walking velocity of the path follow. Without it the
    -- latch keeps pulling the body in its old walking direction, whatever the push says.
    local resetOk = pcall(function() ragDollComponent:resetForce(); end);

    ragDollComponent:setState("Ragdolling");

    -- One shot impulse instead of the latched applyRequiredForceForVelocity().
    local pushVelocity = Vector3(deathDirectionX * DEATH_KNOCKBACK_HORIZONTAL, DEATH_KNOCKBACK_UP, 0);
    local pushOk = pcall(function() ragDollComponent:applyRequiredForceForJumpVelocity(pushVelocity); end);

    if ((false == resetOk or false == pushOk) and false == missingApiLogged) then
        missingApiLogged = true;
        log("[Rhino] resetForce or applyRequiredForceForJumpVelocity is not bound for lua in this build - the death knockback is skipped. resetForce: "
            .. toString(resetOk) .. " applyRequiredForceForJumpVelocity: " .. toString(pushOk));
    end
end
