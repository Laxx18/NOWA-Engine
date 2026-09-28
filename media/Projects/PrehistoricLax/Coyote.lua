module("Coyote", package.seeall);
-- Scene: Level1

require("init");

local coyote = nil;
local coyoteId = nil;
local animationBlender = nil;

-- Handles returned by registerEventListener. removeEventListener takes exactly these ids.
local enemyHitListenerId = nil;
local enemyDeadListenerId = nil;

-- Turn around when hit
--
-- A non lethal hit makes the coyote turn around and run back to the waypoint behind it.
--
-- Attention: AiPathFollowComponent:setInvertDirection() can NOT do this. It only stores the
-- attribute (the running path takes it over on the next connect() only), and Path::setInvertDirection()
-- jumps to an END of the waypoint list instead of turning around. AiPathFollowComponent:turnAround()
-- (KI::Path::turnAround) flips the direction on the LIVE path, so the waypoint the coyote is coming
-- from becomes its next target. Until that C++ addition is built, the call is caught and logged once.
--
-- The cooldown keeps a fast combo from flipping the coyote back and forth on every single hit.
local TURN_AROUND_COOLDOWN = 0.6;
local turnAroundTimer = 0;
local turnAroundLogged = false;

-- Death sequence, same approach as the rhino (see Rhino.lua for why the push is delayed and why it
-- must be a one shot impulse). Slightly harder knockback, the coyote is lighter.
local DEATH_PUSH_DELAY = 0.3;
local DEATH_KNOCKBACK_HORIZONTAL = 5.0;
local DEATH_KNOCKBACK_UP = 3.5;
local DEATH_DELETE_DELAY = 2.5;

local isDying = false;
local dyingTime = 0;
local deathPushDone = false;
local deathDirectionX = 1;
local missingApiLogged = false;

Coyote = {}

Coyote["connect"] = function(gameObject)
    coyote = AppStateManager:getGameObjectController():castGameObject(gameObject);
    coyoteId = coyote:getId();
    animationBlender = coyote:getAnimationComponentV2():getAnimationBlender();

    -- Attention: the locomotion clips MUST be the *_InPlace variants. 'Walk', 'Run' and 'Roll' carry
    -- root motion: the mesh walks away from its scene node and snaps back at the end of every loop -
    -- exactly the "runs away from its physics hull" effect the rhino had.
    animationBlender:registerAnimation(AnimationBlender.ANIM_IDLE_1, "Idle");
    animationBlender:registerAnimation(AnimationBlender.ANIM_IDLE_2, "Idle_2");
    animationBlender:registerAnimation(AnimationBlender.ANIM_IDLE_3, "Sleep");
    animationBlender:registerAnimation(AnimationBlender.ANIM_WALK_NORTH, "Walk_InPlace");
    animationBlender:registerAnimation(AnimationBlender.ANIM_RUN, "Run_InPlace");
    animationBlender:registerAnimation(AnimationBlender.ANIM_JUMP_START, "Jump_Up");
    animationBlender:registerAnimation(AnimationBlender.ANIM_JUMP_WALK, "Run_InPlace");
    animationBlender:registerAnimation(AnimationBlender.ANIM_HIGH_JUMP_END, "Land");
    animationBlender:registerAnimation(AnimationBlender.ANIM_JUMP_END, "Land");
    animationBlender:registerAnimation(AnimationBlender.ANIM_FALL, "Fall");
    -- The coyote has no bite or kick clip - the rolling charge is its attack.
    animationBlender:registerAnimation(AnimationBlender.ANIM_ATTACK_1, "Roll_InPlace");
    animationBlender:registerAnimation(AnimationBlender.ANIM_ATTACK_2, "Roll_InPlace");
    -- No dedicated damage clip either. 'Fall' is one second long, short enough for a hit reaction.
    animationBlender:registerAnimation(AnimationBlender.ANIM_TAKE_DAMAGE, "Fall");
    -- Defeat, only used when the coyote has no ragdoll component, see update().
    animationBlender:registerAnimation(AnimationBlender.ANIM_ACTION_1, "Failure");
    animationBlender:registerAnimation(AnimationBlender.ANIM_NO_IDEA, "Talk");

    turnAroundTimer = 0;
    isDying = false;
    dyingTime = 0;
    deathPushDone = false;
    deathDirectionX = 1;

    -- Non lethal hits, sent by WeaponStick.lua. Registered in init.lua.
    if (EventType.EnemyHitEvent ~= nil) then
        enemyHitListenerId = AppStateManager:getScriptEventManager():registerEventListener(EventType.EnemyHitEvent, Coyote["onEnemyHit"]);
    end

    if (EventType.EnemyDeadEvent ~= nil) then
        enemyDeadListenerId = AppStateManager:getScriptEventManager():registerEventListener(EventType.EnemyDeadEvent, Coyote["onEnemyDead"]);
    end
end

Coyote["disconnect"] = function()
    -- Called at simulation stop AND when the coyote is deleted while the simulation runs (killed).
    -- The listeners must go, otherwise the next hit or kill of ANY enemy still calls into this script.
    if (enemyHitListenerId ~= nil) then
        AppStateManager:getScriptEventManager():removeEventListener(enemyHitListenerId);
        enemyHitListenerId = nil;
    end
    if (enemyDeadListenerId ~= nil) then
        AppStateManager:getScriptEventManager():removeEventListener(enemyDeadListenerId);
        enemyDeadListenerId = nil;
    end

    coyote = nil;
    coyoteId = nil;
    animationBlender = nil;
    isDying = false;
    dyingTime = 0;
    deathPushDone = false;
end

---------------------------------------------------------------------------------------------------
-- EnemyHitEvent: sent by WeaponStick.lua for every hit that does NOT kill the enemy.
---------------------------------------------------------------------------------------------------
Coyote["onEnemyHit"] = function(eventData)
    if (coyote == nil or true == isDying) then
        do return end;
    end

    -- The event fires for ANY hit enemy - only react if it's this one.
    if (eventData["enemyId"] ~= coyoteId) then
        do return end;
    end

    if (turnAroundTimer > 0) then
        do return end;
    end

    local pathFollowComponent = coyote:getAiPathFollowComponent();
    if (pathFollowComponent == nil) then
        do return end;
    end

    local turned = false;
    local callOk = pcall(function() turned = pathFollowComponent:turnAround(); end);

    if (false == callOk) then
        if (false == turnAroundLogged) then
            turnAroundLogged = true;
            log("[Coyote] AiPathFollowComponent:turnAround() is not bound for lua in this build yet - the coyote does not turn around on a hit.");
        end
        do return end;
    end

    if (true == turned) then
        turnAroundTimer = TURN_AROUND_COOLDOWN;
    end
end

Coyote["onEnemyDead"] = function(eventData)
    if (coyote == nil) then
        do return end;
    end

    -- The event fires for ANY killed enemy - only react if it's this one.
    if (eventData["enemyId"] ~= coyoteId) then
        do return end;
    end

    -- Weapon and player script may both report the same kill.
    if (true == isDying) then
        do return end;
    end

    isDying = true;
    dyingTime = 0;
    deathPushDone = false;

    deathDirectionX = 1;
    local hitDirection = eventData["hitDirection"];
    if (hitDirection ~= nil and hitDirection.x < 0) then
        deathDirectionX = -1;
    end

    local pathFollowComponent = coyote:getAiPathFollowComponent();
    if (pathFollowComponent ~= nil) then
        pathFollowComponent:setActivated(false);
    end

    if (animationBlender ~= nil) then
        animationBlender:blend5(AnimationBlender.ANIM_TAKE_DAMAGE, AnimationBlender.BLEND_WHILE_ANIMATING, 0.05, false);
    end

    AppStateManager:getGameObjectController():deleteDelayedGameObject(coyoteId, DEATH_PUSH_DELAY + DEATH_DELETE_DELAY);
end

---------------------------------------------------------------------------------------------------
-- Called by the LuaScriptComponent every frame.
---------------------------------------------------------------------------------------------------
Coyote["update"] = function(dt)
    if (coyote == nil) then
        do return end;
    end

    if (turnAroundTimer > 0) then
        turnAroundTimer = turnAroundTimer - dt;
    end

    if (false == isDying or true == deathPushDone) then
        do return end;
    end

    dyingTime = dyingTime + dt;
    if (dyingTime < DEATH_PUSH_DELAY) then
        do return end;
    end

    deathPushDone = true;

    -- With a ragdoll component the coyote dies like the rhino. Without one, the defeat clip plays
    -- and the plain physics body gets the knockback.
    local ragDollComponent = coyote:getPhysicsRagDollComponentV2();
    local physicsComponent = ragDollComponent;
    if (physicsComponent == nil) then
        physicsComponent = coyote:getPhysicsActiveComponent();
    end

    if (physicsComponent == nil) then
        do return end;
    end

    -- Clears the latched walking velocity of the path follow first. Same order as the rhino:
    -- reset, switch the state, then one impulse.
    local resetOk = pcall(function() physicsComponent:resetForce(); end);

    if (ragDollComponent ~= nil) then
        ragDollComponent:setState("Ragdolling");
    elseif (animationBlender ~= nil) then
        animationBlender:blend5(AnimationBlender.ANIM_ACTION_1, AnimationBlender.BLEND_WHILE_ANIMATING, 0.1, false);
    end

    local pushVelocity = Vector3(deathDirectionX * DEATH_KNOCKBACK_HORIZONTAL, DEATH_KNOCKBACK_UP, 0);
    local pushOk = pcall(function() physicsComponent:applyRequiredForceForJumpVelocity(pushVelocity); end);

    if ((false == resetOk or false == pushOk) and false == missingApiLogged) then
        missingApiLogged = true;
        log("[Coyote] resetForce or applyRequiredForceForJumpVelocity is not bound for lua in this build - the death knockback is skipped. resetForce: "
            .. toString(resetOk) .. " applyRequiredForceForJumpVelocity: " .. toString(pushOk));
    end
end
