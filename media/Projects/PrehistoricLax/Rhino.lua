module("Rhino", package.seeall);
-- Scene: Level1

require("init");

local rhino = nil;
local player = nil;
-- Event handlers compare against this id, never against rhino:getId(): an event can still reach this
-- script after the game object was deleted, and then 'rhino' points to freed memory.
local rhinoId = nil;
local profile = nil;
local enemyHitListenerId = nil;
local enemyDeadListenerId = nil;

-- Shown for ENEMY_ENERGY_BAR_TIME seconds after every hit.
local energyBar = nil;
local energyBarTimer = 0;

Rhino = {}

local function showEnergyBar(value)
    energyBar:setCurrentValue(value);
    energyBar:setActivated(true);
    energyBarTimer = ENEMY_ENERGY_BAR_TIME;
end

Rhino["connect"] = function(gameObject)
    rhino = AppStateManager:getGameObjectController():castGameObject(gameObject);
    rhinoId = rhino:getId();
    profile = EnemyProfiles[rhino:getTagName()];
    player = AppStateManager:getGameObjectController():getGameObjectFromName(PLAYER_NAME);

    energyBar = rhino:getValueBarComponent();
    setupEnemyEnergyBar(energyBar, profile);
    energyBarTimer = 0;

    local animationBlender = rhino:getAnimationComponentV2():getAnimationBlender();
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

    enemyHitListenerId = AppStateManager:getScriptEventManager():registerEventListener(EventType.EnemyHitEvent, Rhino["onEnemyHit"]);
    enemyDeadListenerId = AppStateManager:getScriptEventManager():registerEventListener(EventType.EnemyDeadEvent, Rhino["onEnemyDead"]);
end

Rhino["disconnect"] = function()
    -- Called at simulation stop AND when the rhino is deleted while the simulation runs (killed).
    AppStateManager:getScriptEventManager():removeEventListener(enemyHitListenerId);
    AppStateManager:getScriptEventManager():removeEventListener(enemyDeadListenerId);
    enemyHitListenerId = nil;
    enemyDeadListenerId = nil;

    rhino = nil;
    player = nil;
    energyBar = nil;
    rhinoId = nil;
    profile = nil;
end

Rhino["onEnemyHit"] = function(eventData)
    -- The event fires for ANY hit enemy - only react if it's this one.
    if (eventData["enemyId"] ~= rhinoId) then
        do return end;
    end

    showEnergyBar(eventData["remainingEnergy"]);
end

Rhino["onEnemyDead"] = function(eventData)
    if (eventData["enemyId"] ~= rhinoId) then
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
	
	rhino:getSimpleSoundComponentFromName("Death"):setActivated(true);

    local ragDollComponent = rhino:getPhysicsRagDollComponentV2();
    ragDollComponent:setState("Ragdolling");
    -- ONE impulse, not the latched applyRequiredForceForVelocity, which would hold the body at a
    -- constant velocity.
    ragDollComponent:applyRequiredForceForJumpVelocity(Vector3(directionX * profile.deathKnockbackHorizontal, profile.deathKnockbackUp, 0));

    AppStateManager:getGameObjectController():deleteDelayedGameObject(rhinoId, profile.deathDeleteDelay);
end

---------------------------------------------------------------------------------------------------
-- Called by the LuaScriptComponent every frame.
---------------------------------------------------------------------------------------------------
Rhino["update"] = function(dt)
    if (energyBarTimer > 0) then
        energyBarTimer = energyBarTimer - dt;
        if (energyBarTimer <= 0) then
            energyBar:setActivated(false);
        end
    end

    -- Tells the player's script every frame while the player is within the attack reach. The
    -- player's script decides whether the attack may start (cooldown, i-frames, trade rule).
    local delta = player:getPosition() - rhino:getPosition();
    if (math.abs(delta.x) <= profile.attackReach and math.abs(delta.y) <= profile.attackReachVertical) then
        local eventData = {};
        eventData["enemyId"] = rhinoId;
        AppStateManager:getScriptEventManager():queueEvent(EventType.EnemyNearPlayerEvent, eventData);
    end
end
