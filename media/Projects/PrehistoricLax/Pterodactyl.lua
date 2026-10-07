module("Pterodactyl", package.seeall);
-- Scene: Level1
--
-- First end boss. The fight is one fixed, readable loop:
--
--   ApproachState    hovers at its start position and ignores the player, until the boss area
--                    trigger sends the BossFightStartEvent.
--   RoarState        one roar, so the player knows what just started.
--   PatrolState      a sweep across the whole arena, starting away from the player. At the end of
--                    the sweep the next attack begins - alternating, never random: a boss the
--                    player can read is a boss the player can beat.
--   DiveState        climbs over the player and swoops down onto him. The damage is the ordinary
--                    contact attack every enemy has (EnemyNearPlayerEvent -> PrehistoricLax.lua).
--   EggState         hovers above the player and drops eggs from its SpawnComponent.
--   VulnerableState  after EVERY attack: drops to jump height beside the player and holds still.
--                    That is the window the player is meant to use, and while it is open the boss
--                    does not attack at all (EnemyVulnerableEvent).
--
-- A landed hit always throws the boss back and stuns it briefly, afterwards it carries on with
-- whatever it was doing.
--
-- All timing and all positions come from PTERODACTYL in init.lua.

require("init");

local pterodactyl = nil;
-- Event handlers compare against this id, never against pterodactyl:getId(): an event can still
-- reach this script after the game object was deleted, and then 'pterodactyl' points to freed memory.
local pterodactylId = nil;
local player = nil;
local profile = nil;
local config = nil;

local physicsComponent = nil;
local animationBlender = nil;
local spawnComponent = nil;
local aiLuaComponent = nil;
local movingBehavior = nil;

-- Shown for ENEMY_ENERGY_BAR_TIME seconds after every hit.
local energyBar = nil;
local energyBarTimer = 0;

local bossFightStartListenerId = nil;
local enemyHitListenerId = nil;
local enemyDeadListenerId = nil;

-- From the start of the fight until the boss is dead. Before and after it, the boss does nothing.
local isFightRunning = false;

-- While the boss hovers defencelessly after an attack.
local isVulnerable = false;

-- Set by the path goal observer and evaluated one frame later in update, see connect.
local hasReachedGoal = false;
-- What the state that is currently flying wants to do once its path is done. Set by flyTo.
local goalReachedHandler = nil;

-- The behavior the current state drives the boss with. A landed hit switches to NONE for the
-- knockback and update puts this one back when the stun is over.
local stateBehavior = nil;

-- Only one of these runs at a time, each one belongs to exactly one state.
local roarTimer = 0;
local eggDropTimer = 0;
local vulnerableTimer = 0;
local stunTimer = 0;

-- Decides whether the next attack is the dive or the eggs.
local attackCounter = 0;

Pterodactyl = {}

---------------------------------------------------------------------------------------------------
-- Helpers
---------------------------------------------------------------------------------------------------

local function showEnergyBar(value)
    energyBar:setCurrentValue(value);
    energyBar:setActivated(true);
    energyBarTimer = ENEMY_ENERGY_BAR_TIME;
end

local function clampValue(value, minimum, maximum)
    if (value < minimum) then
        return minimum;
    end
    if (value > maximum) then
        return maximum;
    end
    return value;
end

-- Every flight target goes through here, so none of them can ever leave the arena.
local function arenaPosition(x, y)
    return Vector3(clampValue(x, config.minX, config.maxX), clampValue(y, config.minY, config.maxY), config.z);
end

local function blendAnimation(animationId, blendTime)
    -- Looping on purpose, exactly like the attack clip of the ordinary enemies: a non looping clip
    -- blocks every other blend until it has played to the end, and the MovingBehavior slows the
    -- clip down while the boss stands still - it would get stuck in its pose. The timers end the
    -- phases, not the clips.
    animationBlender:blend5(animationId, AnimationBlender.BLEND_WHILE_ANIMATING, blendTime, true);
end

-- Starts a flight along 'waypoints'. 'handler' is called from update one frame after the last
-- waypoint was reached.
--
-- Attention: waypoints must NEVER be added from inside the path goal observer. MovingBehavior's
-- followPath2D calls the observer and clears the path immediately afterwards, so everything added
-- in there would be wiped. That is why the observer only raises a flag and update does the work.
local function flyTo(waypoints, speed, handler)
    physicsComponent:setSpeed(speed);

    for i = 1, #waypoints do
        movingBehavior:getPath():addWayPoint(waypoints[i]);
    end

    goalReachedHandler = handler;
    stateBehavior = BehaviorType.FOLLOW_PATH_2D;
    movingBehavior:setBehavior(stateBehavior);
end

-- Holds the boss on the spot in the air. STOP and not NONE: STOP keeps latching a velocity of zero
-- every frame, so the boss really stands still - with NONE nothing holds it and it drifts on.
local function hover()
    goalReachedHandler = nil;
    stateBehavior = BehaviorType.STOP;
    movingBehavior:setBehavior(stateBehavior);
end

local function setVulnerable(vulnerable)
    isVulnerable = vulnerable;

    local eventData = {};
    eventData["enemyId"] = pterodactylId;
    eventData["isVulnerable"] = vulnerable;
    AppStateManager:getScriptEventManager():queueEvent(EventType.EnemyVulnerableEvent, eventData);
end

---------------------------------------------------------------------------------------------------
-- Life cycle
---------------------------------------------------------------------------------------------------

Pterodactyl["connect"] = function(gameObject)
    pterodactyl = AppStateManager:getGameObjectController():castGameObject(gameObject);
    pterodactylId = pterodactyl:getId();
    profile = EnemyProfiles[pterodactyl:getTagName()];
    config = PTERODACTYL;
    player = AppStateManager:getGameObjectController():getGameObjectFromName(PLAYER_NAME);

    physicsComponent = pterodactyl:getPhysicsRagDollComponentV2();

    spawnComponent = pterodactyl:getSpawnComponent();
    spawnComponent:setActivated(false);

    energyBar = pterodactyl:getValueBarComponent();
    setupEnemyEnergyBar(energyBar, profile);
    energyBarTimer = 0;

    -- Only "in place" clips for everything the MovingBehavior drives. A clip with root motion would
    -- move the boss a second time and fight the steering.
    animationBlender = pterodactyl:getAnimationComponentV2():getAnimationBlender();
    animationBlender:registerAnimation(AnimationBlender.ANIM_IDLE_1, "Hovering");
    animationBlender:registerAnimation(AnimationBlender.ANIM_IDLE_2, "Look_Side");
    animationBlender:registerAnimation(AnimationBlender.ANIM_IDLE_3, "Idle");
    animationBlender:registerAnimation(AnimationBlender.ANIM_WALK_NORTH, "Flap_InPlace");
    animationBlender:registerAnimation(AnimationBlender.ANIM_WALK_SOUTH, "Fly_Up_InPlace");
    animationBlender:registerAnimation(AnimationBlender.ANIM_RUN, "Glide_InPlace");
    animationBlender:registerAnimation(AnimationBlender.ANIM_FALL, "Glide_InPlace");
    animationBlender:registerAnimation(AnimationBlender.ANIM_JUMP_START, "Take_Off");
    animationBlender:registerAnimation(AnimationBlender.ANIM_JUMP_END, "Land");
    animationBlender:registerAnimation(AnimationBlender.ANIM_HIGH_JUMP_END, "Land");
    animationBlender:registerAnimation(AnimationBlender.ANIM_ATTACK_1, "Air_Attack_Claw");
    animationBlender:registerAnimation(AnimationBlender.ANIM_ATTACK_2, "Air_Attack_Bite");
    animationBlender:registerAnimation(AnimationBlender.ANIM_ATTACK_3, "Air_Roar");
    animationBlender:registerAnimation(AnimationBlender.ANIM_TAKE_DAMAGE, "Air_Damage_Light");

    aiLuaComponent = pterodactyl:getAiLuaComponent();
    movingBehavior = aiLuaComponent:getMovingBehavior();
    movingBehavior:setGoalRadius(config.goalRadius);
    -- The boss flies in open air, nothing can wedge it - and the slow approach to a waypoint would
    -- look exactly like being stuck. 0 switches the check off.
    movingBehavior:setStuckCheckTime(0);
    -- Turns the boss into its flight direction, so it faces left when it flies left. The upright
    -- constraint of the physics component (ConstraintDirection 0 1 0) keeps it from pitching over.
    movingBehavior:setAutoOrientation(true);

    -- ONE observer for all states. The state that is currently flying puts its continuation into
    -- goalReachedHandler, see flyTo.
    aiLuaComponent:reactOnPathGoalReached(function()
        hasReachedGoal = true;
    end);

    isFightRunning = false;
    isVulnerable = false;
    hasReachedGoal = false;
    goalReachedHandler = nil;
    roarTimer = 0;
    eggDropTimer = 0;
    vulnerableTimer = 0;
    stunTimer = 0;
    attackCounter = 0;

    bossFightStartListenerId = AppStateManager:getScriptEventManager():registerEventListener(EventType.BossFightStartEvent, Pterodactyl["onBossFightStart"]);
    enemyHitListenerId = AppStateManager:getScriptEventManager():registerEventListener(EventType.EnemyHitEvent, Pterodactyl["onEnemyHit"]);
    enemyDeadListenerId = AppStateManager:getScriptEventManager():registerEventListener(EventType.EnemyDeadEvent, Pterodactyl["onEnemyDead"]);
end

Pterodactyl["disconnect"] = function()
    -- Called at simulation stop AND when the boss is deleted while the simulation runs (killed).
    AppStateManager:getScriptEventManager():removeEventListener(bossFightStartListenerId);
    AppStateManager:getScriptEventManager():removeEventListener(enemyHitListenerId);
    AppStateManager:getScriptEventManager():removeEventListener(enemyDeadListenerId);
    bossFightStartListenerId = nil;
    enemyHitListenerId = nil;
    enemyDeadListenerId = nil;

    isFightRunning = false;
    isVulnerable = false;
    goalReachedHandler = nil;
    stateBehavior = nil;

    pterodactyl = nil;
    pterodactylId = nil;
    player = nil;
    profile = nil;
    config = nil;
    physicsComponent = nil;
    animationBlender = nil;
    spawnComponent = nil;
    aiLuaComponent = nil;
    movingBehavior = nil;
    energyBar = nil;
end

---------------------------------------------------------------------------------------------------
-- Events
---------------------------------------------------------------------------------------------------

-- Sent by the boss area trigger as soon as the player has run into the arena and the gates came down.
Pterodactyl["onBossFightStart"] = function(eventData)
    if (true == isFightRunning) then
        do return end;
    end

    log("[Pterodactyl] Boss fight started");
    isFightRunning = true;
    aiLuaComponent:changeState(RoarState);
end

Pterodactyl["onEnemyHit"] = function(eventData)
    -- The event fires for ANY hit enemy - only react if it's this one.
    if (eventData["enemyId"] ~= pterodactylId) then
        do return end;
    end

    showEnergyBar(eventData["remainingEnergy"]);
    blendAnimation(AnimationBlender.ANIM_TAKE_DAMAGE, 0.1);

    -- Thrown back. BehaviorType.NONE and not STOP: STOP would latch a velocity of zero every frame
    -- and brake the impulse away at once. update puts the state's behavior back when the stun is
    -- over, so the boss simply continues the flight it was on.
    movingBehavior:setBehavior(BehaviorType.NONE);
    physicsComponent:applyRequiredForceForJumpVelocity(Vector3(eventData["hitDirection"].x * config.hitKnockbackHorizontal, config.hitKnockbackUp, 0));
    stunTimer = config.hitStunTime;
end

Pterodactyl["onEnemyDead"] = function(eventData)
    if (eventData["enemyId"] ~= pterodactylId) then
        do return end;
    end

    isFightRunning = false;
    setVulnerable(false);
    spawnComponent:setActivated(false);

    -- WeaponStick only switches an AiPathFollowComponent off, the boss is driven by an
    -- AiLuaComponent - without this the moving behavior would keep pushing the ragdoll around.
    movingBehavior:setBehavior(BehaviorType.NONE);

    -- Only if the bar is up anyway (the enemy was hit shortly before). Killed with one blow, the
    -- bar is not shown at all.
    if (energyBarTimer > 0) then
        showEnergyBar(0);
    end

    local directionX = 1;
    if (eventData["hitDirection"].x < 0) then
        directionX = -1;
    end

    pterodactyl:getSimpleSoundComponentFromName("Death"):setActivated(true);

    local ragDollComponent = pterodactyl:getPhysicsRagDollComponentV2();
    -- The boss flies with a gravity of zero, so without this the corpse would hang in the air.
    ragDollComponent:setGravity(Vector3(0, -16.9, 0));
    ragDollComponent:setState("Ragdolling");
    -- ONE impulse, not the latched applyRequiredForceForVelocity, which would hold the body at a
    -- constant velocity.
    ragDollComponent:applyRequiredForceForJumpVelocity(Vector3(directionX * profile.deathKnockbackHorizontal, profile.deathKnockbackUp, 0));

    AppStateManager:getGameObjectController():deleteDelayedGameObject(pterodactylId, profile.deathDeleteDelay);
end

---------------------------------------------------------------------------------------------------
-- Called by the LuaScriptComponent every frame. ALL timing of the fight runs here, the states only
-- set the flight up and start a timer.
---------------------------------------------------------------------------------------------------
Pterodactyl["update"] = function(dt)
    if (energyBarTimer > 0) then
        energyBarTimer = energyBarTimer - dt;
        if (energyBarTimer <= 0) then
            energyBar:setActivated(false);
        end
    end

    if (false == isFightRunning) then
        do return end;
    end

    -- Knocked back after a landed hit. Nothing else happens while the boss is flying backwards,
    -- which also pauses the vulnerable window: a hit is always rewarded with a chance at the next.
    if (stunTimer > 0) then
        stunTimer = stunTimer - dt;
        if (stunTimer <= 0) then
            stunTimer = 0;
            movingBehavior:setBehavior(stateBehavior);
            blendAnimation(AnimationBlender.ANIM_IDLE_1, 0.2);
        end
        do return end;
    end

    -- One frame after the last waypoint was reached, see the observer in connect. The handler is
    -- taken out BEFORE it is called, because it usually starts the next flight and sets a new one.
    if (true == hasReachedGoal) then
        hasReachedGoal = false;

        local handler = goalReachedHandler;
        goalReachedHandler = nil;
        if (handler ~= nil) then
            handler();
        end
    end

    if (roarTimer > 0) then
        roarTimer = roarTimer - dt;
        if (roarTimer <= 0) then
            roarTimer = 0;
            aiLuaComponent:changeState(PatrolState);
        end
    end

    if (eggDropTimer > 0) then
        eggDropTimer = eggDropTimer - dt;
        if (eggDropTimer <= 0) then
            eggDropTimer = 0;
            spawnComponent:setActivated(false);
            aiLuaComponent:changeState(VulnerableState);
        end
    end

    if (vulnerableTimer > 0) then
        vulnerableTimer = vulnerableTimer - dt;
        if (vulnerableTimer <= 0) then
            vulnerableTimer = 0;
            aiLuaComponent:changeState(PatrolState);
        end
    end

    -- Tells the player's script every frame while the player is within the attack reach. Not while
    -- the boss is defenceless - that window exists so the player can walk up and strike.
    if (false == isVulnerable) then
        local delta = player:getPosition() - pterodactyl:getPosition();
        if (math.abs(delta.x) <= profile.attackReach and math.abs(delta.y) <= profile.attackReachVertical) then
            local eventData = {};
            eventData["enemyId"] = pterodactylId;
            AppStateManager:getScriptEventManager():queueEvent(EventType.EnemyNearPlayerEvent, eventData);
        end
    end
end

---------------------------------------------------------------------------------------------------
-- States of the AiLuaComponent. The start state in the editor is ApproachState.
---------------------------------------------------------------------------------------------------

ApproachState = {}

ApproachState["enter"] = function(gameObject)
    log("[Pterodactyl] Enter ApproachState: " .. gameObject:getName());
    hover();
    blendAnimation(AnimationBlender.ANIM_IDLE_1, 0.3);
end

---------------------------------------------------------------------------------------------------

RoarState = {}

RoarState["enter"] = function(gameObject)
    log("[Pterodactyl] Enter RoarState");
    hover();
    blendAnimation(AnimationBlender.ANIM_ATTACK_3, 0.2);
    roarTimer = config.roarTime;
end

RoarState["exit"] = function(gameObject)
    roarTimer = 0;
end

---------------------------------------------------------------------------------------------------

PatrolState = {}

PatrolState["enter"] = function(gameObject)
    log("[Pterodactyl] Enter PatrolState");
    blendAnimation(AnimationBlender.ANIM_WALK_NORTH, 0.3);

    local centerX = (config.minX + config.maxX) * 0.5;

    -- Always starts on the side the player is NOT on, so the boss crosses the whole screen once
    -- before it attacks. The raised middle waypoint turns the straight line into an arc.
    local firstX = config.minX;
    local lastX = config.maxX;
    if (player:getPosition().x < centerX) then
        firstX = config.maxX;
        lastX = config.minX;
    end

    local waypoints = {};
    waypoints[1] = arenaPosition(firstX, config.patrolY);
    waypoints[2] = arenaPosition(centerX, config.patrolY + config.patrolAmplitude);
    waypoints[3] = arenaPosition(lastX, config.patrolY);

    flyTo(waypoints, config.cruiseSpeed, function()
        -- Alternating and not random: a pattern the player can learn is what makes a boss fair.
        attackCounter = attackCounter + 1;
        if (attackCounter % 2 == 1) then
            aiLuaComponent:changeState(DiveState);
        else
            aiLuaComponent:changeState(EggState);
        end
    end);
end

---------------------------------------------------------------------------------------------------

DiveState = {}

DiveState["enter"] = function(gameObject)
    log("[Pterodactyl] Enter DiveState");
    blendAnimation(AnimationBlender.ANIM_RUN, 0.2);

    local playerPosition = player:getPosition();

    -- First up and over the player, then straight down onto him. Two waypoints and not one, so the
    -- swoop really looks like a swoop and the player gets a moment to see it coming.
    local waypoints = {};
    waypoints[1] = arenaPosition(playerPosition.x, playerPosition.y + config.diveApproachHeight);
    waypoints[2] = arenaPosition(playerPosition.x, playerPosition.y + config.diveHeightOverPlayer);

    flyTo(waypoints, config.diveSpeed, function()
        aiLuaComponent:changeState(VulnerableState);
    end);
end

DiveState["exit"] = function(gameObject)
    physicsComponent:setSpeed(config.cruiseSpeed);
end

---------------------------------------------------------------------------------------------------

EggState = {}

EggState["enter"] = function(gameObject)
    log("[Pterodactyl] Enter EggState");
    blendAnimation(AnimationBlender.ANIM_WALK_NORTH, 0.2);

    local waypoints = {};
    waypoints[1] = arenaPosition(player:getPosition().x, config.eggHeight);

    flyTo(waypoints, config.cruiseSpeed, function()
        -- Arrived above the player: roar once and let the SpawnComponent do the rest. The boss keeps
        -- hovering while it drops, so the eggs come down as a cluster the player has to leave.
        hover();
        blendAnimation(AnimationBlender.ANIM_ATTACK_3, 0.2);
        spawnComponent:setActivated(true);
        eggDropTimer = config.eggDropTime;
    end);
end

EggState["exit"] = function(gameObject)
    spawnComponent:setActivated(false);
    eggDropTimer = 0;
end

---------------------------------------------------------------------------------------------------

VulnerableState = {}

VulnerableState["enter"] = function(gameObject)
    log("[Pterodactyl] Enter VulnerableState");
    blendAnimation(AnimationBlender.ANIM_IDLE_1, 0.3);

    local playerPosition = player:getPosition();

    -- Beside the player and not on top of him, otherwise the two bodies push each other around.
    -- It keeps the side it is already on, so it does not fly through the player to get there.
    local offsetX = config.vulnerableOffsetX;
    if (pterodactyl:getPosition().x < playerPosition.x) then
        offsetX = -offsetX;
    end

    local waypoints = {};
    waypoints[1] = arenaPosition(playerPosition.x + offsetX, playerPosition.y + config.vulnerableHeightOverPlayer);

    flyTo(waypoints, config.cruiseSpeed, function()
        hover();
        setVulnerable(true);
        vulnerableTimer = config.vulnerableTime;
    end);
end

VulnerableState["exit"] = function(gameObject)
    setVulnerable(false);
    vulnerableTimer = 0;
end