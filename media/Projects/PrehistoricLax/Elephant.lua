module("Elephant", package.seeall);
-- Scene: Level2

require("init");

local elephant = nil;
local player = nil;
-- Event handlers compare against this id, never against elephant:getId(): an event can still reach this
-- script after the game object was deleted, and then 'elephant' points to freed memory.
local elephantId = nil;
local profile = nil;
local enemyHitListenerId = nil;
local enemyDeadListenerId = nil;

-- Shown for ENEMY_ENERGY_BAR_TIME seconds after every hit.
local energyBar = nil;
local energyBarTimer = 0;

Elephant = {}

local function showEnergyBar(value)
    energyBar:setCurrentValue(value);
    energyBar:setActivated(true);
    energyBarTimer = ENEMY_ENERGY_BAR_TIME;
end

Elephant["connect"] = function(gameObject)
    elephant = AppStateManager:getGameObjectController():castGameObject(gameObject);
    elephantId = elephant:getId();
    profile = EnemyProfiles[elephant:getTagName()];
    player = AppStateManager:getGameObjectController():getGameObjectFromName(PLAYER_NAME);

    energyBar = elephant:getValueBarComponent();
    setupEnemyEnergyBar(energyBar, profile);
    energyBarTimer = 0;

    local animationBlender = elephant:getAnimationComponentV2():getAnimationBlender();
    animationBlender:registerAnimation(AnimationBlender.ANIM_IDLE_1, "Elephant_Idle");
    animationBlender:registerAnimation(AnimationBlender.ANIM_IDLE_2, "Elephant_Idle_2");
    animationBlender:registerAnimation(AnimationBlender.ANIM_WALK_NORTH, "Elephant_Walk_In_Place");
    animationBlender:registerAnimation(AnimationBlender.ANIM_JUMP_START, "Elephant_Jump_Up");
    animationBlender:registerAnimation(AnimationBlender.ANIM_JUMP_WALK, "Elephant_Walk_In_Place");
    animationBlender:registerAnimation(AnimationBlender.ANIM_HIGH_JUMP_END, "Elephant_Land");
    animationBlender:registerAnimation(AnimationBlender.ANIM_JUMP_END, "Elephant_Land");
    animationBlender:registerAnimation(AnimationBlender.ANIM_FALL, "Elephant_Fall");
    animationBlender:registerAnimation(AnimationBlender.ANIM_RUN, "Elephant_Run_In_Place");
    animationBlender:registerAnimation(AnimationBlender.ANIM_ATTACK_1, "Elephant_Kick");
    animationBlender:registerAnimation(AnimationBlender.ANIM_ATTACK_2, "Elephant_Roll_In_Place");
    animationBlender:registerAnimation(AnimationBlender.ANIM_TAKE_DAMAGE, "Elephant_Damage");

    enemyHitListenerId = AppStateManager:getScriptEventManager():registerEventListener(EventType.EnemyHitEvent, Elephant["onEnemyHit"]);
    enemyDeadListenerId = AppStateManager:getScriptEventManager():registerEventListener(EventType.EnemyDeadEvent, Elephant["onEnemyDead"]);
end

Elephant["disconnect"] = function()
    -- Called at simulation stop AND when the elephant is deleted while the simulation runs (killed).
    AppStateManager:getScriptEventManager():removeEventListener(enemyHitListenerId);
    AppStateManager:getScriptEventManager():removeEventListener(enemyDeadListenerId);
    enemyHitListenerId = nil;
    enemyDeadListenerId = nil;

    elephant = nil;
    player = nil;
    energyBar = nil;
    elephantId = nil;
    profile = nil;
end

Elephant["onEnemyHit"] = function(eventData)
    -- The event fires for ANY hit enemy - only react if it's this one.
    if (eventData["enemyId"] ~= elephantId) then
        do return end;
    end
	
	if (animationBlender:isAnimationActive(AnimationBlender.ANIM_TAKE_DAMAGE) == false) then
        animationBlender:blend5(AnimationBlender.ANIM_TAKE_DAMAGE, AnimationBlender.BLEND_WHILE_ANIMATING, 0.1, false);
    end

    showEnergyBar(eventData["remainingEnergy"]);
end

Elephant["onEnemyDead"] = function(eventData)
    if (eventData["enemyId"] ~= elephantId) then
        do return end;
    end

    -- Only if the bar is up anyway (the enemy was hit shortly before). Killed with one blow, the
    -- bar is not shown at all.
    if (energyBarTimer > 0) then
        showEnergyBar(0);
    end

    -- WeaponStick already switched the AI off (BehaviorType.NONE), so nothing drives the body
    -- anymore and the ragdoll can take over in this very frame - no waiting.
    local directionX = 1;
    if (eventData["hitDirection"].x < 0) then
        directionX = -1;
    end
	
	elephant:getSimpleSoundComponentFromName("Death"):setActivated(true);

    local ragDollComponent = elephant:getPhysicsRagDollComponentV2();
    ragDollComponent:setState("Ragdolling");
    -- ONE impulse, not the latched applyRequiredForceForVelocity, which would hold the body at a
    -- constant velocity.
    ragDollComponent:applyRequiredForceForJumpVelocity(Vector3(directionX * profile.deathKnockbackHorizontal, profile.deathKnockbackUp, 0));

    AppStateManager:getGameObjectController():deleteDelayedGameObject(elephantId, profile.deathDeleteDelay);
end

---------------------------------------------------------------------------------------------------
-- Called by the LuaScriptComponent every frame.
---------------------------------------------------------------------------------------------------
Elephant["update"] = function(dt)
    if (energyBarTimer > 0) then
        energyBarTimer = energyBarTimer - dt;
        if (energyBarTimer <= 0) then
            energyBar:setActivated(false);
        end
    end

    -- Tells the player's script every frame while the player is within the attack reach. The
    -- player's script decides whether the attack may start (cooldown, i-frames, trade rule).
    local delta = player:getPosition() - elephant:getPosition();
    if (math.abs(delta.x) <= profile.attackReach and math.abs(delta.y) <= profile.attackReachVertical) then
        local eventData = {};
        eventData["enemyId"] = elephantId;
        AppStateManager:getScriptEventManager():queueEvent(EventType.EnemyNearPlayerEvent, eventData);
    end
end
