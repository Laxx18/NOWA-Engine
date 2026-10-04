module("Beaver", package.seeall);
-- Scene: Level2

require("init");

local beaver = nil;
local player = nil;
-- Event handlers compare against this id, never against Beaver:getId(): an event can still reach this
-- script after the game object was deleted, and then 'Beaver' points to freed memory.
local beaverId = nil;
local profile = nil;
local enemyHitListenerId = nil;
local enemyDeadListenerId = nil;

-- Shown for ENEMY_ENERGY_BAR_TIME seconds after every hit.
local energyBar = nil;
local energyBarTimer = 0;

Beaver = {}

local function showEnergyBar(value)
    energyBar:setCurrentValue(value);
    energyBar:setActivated(true);
    energyBarTimer = ENEMY_ENERGY_BAR_TIME;
end

Beaver["connect"] = function(gameObject)
    beaver = AppStateManager:getGameObjectController():castGameObject(gameObject);
    beaverId = beaver:getId();
    profile = EnemyProfiles[beaver:getTagName()];
    player = AppStateManager:getGameObjectController():getGameObjectFromName(PLAYER_NAME);

    energyBar = beaver:getValueBarComponent();
    setupEnemyEnergyBar(energyBar, profile);
    energyBarTimer = 0;

    local animationBlender = beaver:getAnimationComponentV2():getAnimationBlender();
    animationBlender:registerAnimation(AnimationBlender.ANIM_IDLE_1, "Beaver Idle");
    animationBlender:registerAnimation(AnimationBlender.ANIM_IDLE_2, "Beaver Idle 2");
    animationBlender:registerAnimation(AnimationBlender.ANIM_WALK_NORTH, "Beaver Walk In Place");
    animationBlender:registerAnimation(AnimationBlender.ANIM_FALL, "Beaver Fall");
    animationBlender:registerAnimation(AnimationBlender.ANIM_ATTACK_1, "Beaver Roll In Place");
    animationBlender:registerAnimation(AnimationBlender.ANIM_ATTACK_2, "Beaver Kick");
    animationBlender:registerAnimation(AnimationBlender.ANIM_TAKE_DAMAGE, "Beaver Damage");

    enemyHitListenerId = AppStateManager:getScriptEventManager():registerEventListener(EventType.EnemyHitEvent, Beaver["onEnemyHit"]);
    enemyDeadListenerId = AppStateManager:getScriptEventManager():registerEventListener(EventType.EnemyDeadEvent, Beaver["onEnemyDead"]);
end

Beaver["disconnect"] = function()
    -- Called at simulation stop AND when the beaver is deleted while the simulation runs (killed).
    AppStateManager:getScriptEventManager():removeEventListener(enemyHitListenerId);
    AppStateManager:getScriptEventManager():removeEventListener(enemyDeadListenerId);
    enemyHitListenerId = nil;
    enemyDeadListenerId = nil;

    beaver = nil;
    player = nil;
    energyBar = nil;
    beaverId = nil;
    profile = nil;
end

Beaver["onEnemyHit"] = function(eventData)
    -- The event fires for ANY hit enemy - only react if it's this one.
    if (eventData["enemyId"] ~= beaverId) then
        do return end;
    end

    showEnergyBar(eventData["remainingEnergy"]);
end

Beaver["onEnemyDead"] = function(eventData)
    if (eventData["enemyId"] ~= beaverId) then
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
    
    beaver:getSimpleSoundComponentFromName("Death"):setActivated(true);

    local ragDollComponent = beaver:getPhysicsRagDollComponentV2();
    ragDollComponent:setState("Ragdolling");
    -- ONE impulse, not the latched applyRequiredForceForVelocity, which would hold the body at a
    -- constant velocity.
    ragDollComponent:applyRequiredForceForJumpVelocity(Vector3(directionX * profile.deathKnockbackHorizontal, profile.deathKnockbackUp, 0));
    local sliderComponent = beaver:getJointActiveSliderComponent();
    if (sliderComponent) then
        sliderComponent:releaseJoint(true);
    end

    AppStateManager:getGameObjectController():deleteDelayedGameObject(beaverId, profile.deathDeleteDelay);
end

---------------------------------------------------------------------------------------------------
-- Called by the LuaScriptComponent every frame.
---------------------------------------------------------------------------------------------------
Beaver["update"] = function(dt)
    if (energyBarTimer > 0) then
        energyBarTimer = energyBarTimer - dt;
        if (energyBarTimer <= 0) then
            energyBar:setActivated(false);
        end
    end

    -- Tells the player's script every frame while the player is within the attack reach. The
    -- player's script decides whether the attack may start (cooldown, i-frames, trade rule).
    local delta = player:getPosition() - beaver:getPosition();
    if (math.abs(delta.x) <= profile.attackReach and math.abs(delta.y) <= profile.attackReachVertical) then
        local eventData = {};
        eventData["enemyId"] = beaverId;
        AppStateManager:getScriptEventManager():queueEvent(EventType.EnemyNearPlayerEvent, eventData);
    end
end
