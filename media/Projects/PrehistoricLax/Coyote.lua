module("Coyote", package.seeall);
-- Scene: Level1

require("init");

local coyote = nil;
local player = nil;
-- Event handlers compare against this id, never against coyote:getId(): an event can still reach
-- this script after the game object was deleted, and then 'coyote' points to freed memory.
local coyoteId = nil;
local profile = nil;
local enemyHitListenerId = nil;
local enemyDeadListenerId = nil;

-- Shown for ENEMY_ENERGY_BAR_TIME seconds after every hit.
local energyBar = nil;
local energyBarTimer = 0;

Coyote = {}

local function showEnergyBar(value)
    energyBar:setCurrentValue(value);
    energyBar:setActivated(true);
    energyBarTimer = ENEMY_ENERGY_BAR_TIME;
end

Coyote["connect"] = function(gameObject)
    coyote = AppStateManager:getGameObjectController():castGameObject(gameObject);
    coyoteId = coyote:getId();
    profile = EnemyProfiles[coyote:getTagName()];
    player = AppStateManager:getGameObjectController():getGameObjectFromName(PLAYER_NAME);

    energyBar = coyote:getValueBarComponent();
    setupEnemyEnergyBar(energyBar, profile);
    energyBarTimer = 0;

    -- Attention: the locomotion clips MUST be the *_InPlace variants. 'Walk', 'Run' and 'Roll' carry
    -- root motion: the mesh walks away from its scene node and snaps back at the end of every loop.
    local animationBlender = coyote:getAnimationComponentV2():getAnimationBlender();
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
    -- No bite or kick clip - the rolling charge is the attack.
    animationBlender:registerAnimation(AnimationBlender.ANIM_ATTACK_1, "Roll_InPlace");
    animationBlender:registerAnimation(AnimationBlender.ANIM_ATTACK_2, "Roll_InPlace");
    -- No dedicated damage clip either. 'Fall' is one second long, short enough for a hit reaction.
    animationBlender:registerAnimation(AnimationBlender.ANIM_TAKE_DAMAGE, "Fall");
    animationBlender:registerAnimation(AnimationBlender.ANIM_ACTION_1, "Failure");
    animationBlender:registerAnimation(AnimationBlender.ANIM_NO_IDEA, "Talk");

    enemyHitListenerId = AppStateManager:getScriptEventManager():registerEventListener(EventType.EnemyHitEvent, Coyote["onEnemyHit"]);
    enemyDeadListenerId = AppStateManager:getScriptEventManager():registerEventListener(EventType.EnemyDeadEvent, Coyote["onEnemyDead"]);
end

Coyote["disconnect"] = function()
    -- Called at simulation stop AND when the coyote is deleted while the simulation runs (killed).
    AppStateManager:getScriptEventManager():removeEventListener(enemyHitListenerId);
    AppStateManager:getScriptEventManager():removeEventListener(enemyDeadListenerId);
    enemyHitListenerId = nil;
    enemyDeadListenerId = nil;

    coyote = nil;
    player = nil;
    energyBar = nil;
    coyoteId = nil;
    profile = nil;
end

-- A non lethal hit: the coyote turns around and runs to the waypoint it is coming from.
Coyote["onEnemyHit"] = function(eventData)
    if (eventData["enemyId"] ~= coyoteId) then
        do return end;
    end

    showEnergyBar(eventData["remainingEnergy"]);
    coyote:getAiPathFollowComponent():turnAround();
    coyote:getAnimationComponentV2():getAnimationBlender():blend5(AnimationBlender.ANIM_RUN, AnimationBlender.BLEND_WHILE_ANIMATING, 0.1, true);
end

Coyote["onEnemyDead"] = function(eventData)
    if (eventData["enemyId"] ~= coyoteId) then
        do return end;
    end

    -- Only if the bar is up anyway (the enemy was hit shortly before). Killed with one blow, the
    -- bar is not shown at all.
    if (energyBarTimer > 0) then
        showEnergyBar(0);
    end

    -- WeaponStick already switched the AI off (BehaviorType.NONE), so the ragdoll can take over in
    -- this very frame.
    local directionX = 1;
    if (eventData["hitDirection"].x < 0) then
        directionX = -1;
    end
	
	coyote:getSimpleSoundComponentFromName("Death"):setActivated(true);

    local ragDollComponent = coyote:getPhysicsRagDollComponentV2();
    ragDollComponent:setState("Ragdolling");
    ragDollComponent:applyRequiredForceForJumpVelocity(Vector3(directionX * profile.deathKnockbackHorizontal, profile.deathKnockbackUp, 0));

    AppStateManager:getGameObjectController():deleteDelayedGameObject(coyoteId, profile.deathDeleteDelay);
end

---------------------------------------------------------------------------------------------------
-- Called by the LuaScriptComponent every frame.
---------------------------------------------------------------------------------------------------
Coyote["update"] = function(dt)
    if (energyBarTimer > 0) then
        energyBarTimer = energyBarTimer - dt;
        if (energyBarTimer <= 0) then
            energyBar:setActivated(false);
        end
    end

    -- Tells the player's script every frame while the player is within the attack reach. The
    -- player's script decides whether the attack may start (cooldown, i-frames, trade rule).
    local delta = player:getPosition() - coyote:getPosition();
    if (math.abs(delta.x) <= profile.attackReach and math.abs(delta.y) <= profile.attackReachVertical) then
        local eventData = {};
        eventData["enemyId"] = coyoteId;
        AppStateManager:getScriptEventManager():queueEvent(EventType.EnemyNearPlayerEvent, eventData);
    end
end
