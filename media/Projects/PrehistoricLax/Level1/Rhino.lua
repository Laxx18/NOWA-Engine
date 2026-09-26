module("Rhino", package.seeall);
-- Scene: Level1

require("init");

local rhino = nil;
local animationBlender = nil;

Rhino = {}

Rhino["connect"] = function(gameObject)
    rhino = AppStateManager:getGameObjectController():castGameObject(gameObject);
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

    -- WeaponStick.lua already fires this exact event (with enemyId) the moment
    -- this rhino's Energy attribute reaches 0 - no new event type needed.
    if (EventType.EnemyDeadEvent ~= nil) then
        AppStateManager:getScriptEventManager():registerEventListener(EventType.EnemyDeadEvent, Rhino["onEnemyDead"]);
    end
end

Rhino["disconnect"] = function()
    rhino = nil;
    animationBlender = nil;
end

Rhino["onEnemyDead"] = function(eventData)
    if (rhino == nil) then
        do return end;
    end

    -- The event fires for ANY killed enemy - only react if it's this one.
    if (eventData["enemyId"] ~= rhino:getId()) then
        do return end;
    end

    local ragDollComponent = rhino:getPhysicsRagDollComponent();
    if (ragDollComponent ~= nil) then
        ragDollComponent:setState("Ragdolling");

        -- Knock the ragdoll away from where the hit came from, same mechanism the
        -- player controller itself uses (applyRequiredForceForVelocity), just on the
        -- ragdoll's own physics component instead of a player controller.
        local hitDirection = eventData["hitDirection"];
        if (hitDirection ~= nil) then
            local knockbackStrength = 8;
            local upKick = 4;
            local pushDirection = Vector3(hitDirection.x * knockbackStrength, upKick, hitDirection.z * knockbackStrength);
            ragDollComponent:getPhysicsComponent():applyRequiredForceForVelocity(pushDirection);
        end
    end

    rhino:getLuaScriptComponent():callDelayedMethod(function()
        AppStateManager:getGameObjectController():deleteGameObject(rhino:getId());
    end, 2);
end