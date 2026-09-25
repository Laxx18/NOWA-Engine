module("PrehistoricLax", package.seeall);
-- Scene: Level1

require("init");

local prehistoricLax = nil;
local cameraComponent = nil;
local areaOfInterestComponent = nil;
local attributesComponent = nil;
local mainGameObject = nil;
local playerController = nil;
local animationBlender = nil;

-- Mirrors NOWA::Direction from PlayerControllerComponents.h.
-- Attention: this order is NOT the one the old script used - RIGHT comes before LEFT here.
local DIR_NONE, DIR_RIGHT, DIR_LEFT, DIR_UP, DIR_DOWN = 0, 1, 2, 3, 4;

-- State names. Kept in one place, because they are used as plain strings on both sides.
local STATE_WALK = "WalkingStateJumpNRun";
local STATE_RAGDOLL = "RagDollState";
local STATE_ATTACK = "AttackState";
local STATE_PORTAL = "PortalState";

-- Which way the player is facing, kept up to date by reactOnDirectionChanged, so a state can
-- hold the facing or throw the ragdoll in the right direction without looking at the input.
local facingDirection = DIR_RIGHT;

-- Counted up on every swing and sent along with the PlayerAttackEvent. The cudgel script uses
-- it to make sure ONE swing can only ever cost an enemy energy ONCE, no matter how many
-- frames the kinematic contact keeps firing.
local attackId = 0;

local isInvulnerable = false;

local energy = nil;
local hitParticle = nil;
local baseDamage = 10;

-- Owner id handed to AnimationBlenderV2::addTime.
--
-- Attention: this MUST be the player controller's class name and not an id of its own. The
-- blender lets the first caller of a frame claim it and drops everyone else with a critical
-- log line; the claim is only released by AnimationComponentV2::update() calling beginFrame().
-- Using the same id makes tryClaimAddTime() succeed through its second branch
-- (addTimeOwner == ownerId) no matter what ran first.
local ANIM_OWNER = "PlayerControllerJumpNRunComponent";

---------------------------------------------------------------------------------------------------
-- Helpers
---------------------------------------------------------------------------------------------------

function getEnergy()
    if (energy == nil) then
        return 100;
    end
    return energy:getValueNumber();
end

function setEnergy(value)
    if (energy == nil) then
        do return end;
    end
    energy:setValueNumber(value);
end

-- Central damage entry point, so an enemy, a trap or a hard landing all go through one place.
function applyDamage(amount)
    if (isInvulnerable == true) then
        do return end;
    end

    local newEnergy = getEnergy() - amount;
    if (newEnergy < 0) then
        newEnergy = 0;
    end
    setEnergy(newEnergy);

    if (newEnergy <= 0) then
        -- requestState is queued and applied at the top of the next update, so it is safe to
        -- call from any closure - even from one that fires while the walking state is still
        -- in the middle of its own update.
        playerController:requestState(STATE_RAGDOLL);
    else
        animationBlender:blend5(AnimationBlender.ANIM_TAKE_DAMAGE, AnimationBlender.BLEND_WHILE_ANIMATING, 0.1, false);
    end
end

-- Hits an enemy once. Kept here rather than in the state, so a trap or a thrown rock can
-- use the very same path later.
function damageEnemy(enemyGameObject)
    local enemyAttributes = enemyGameObject:getAttributesComponent();
    if (enemyAttributes == nil) then
        do return end;
    end

    local enemyEnergy = enemyAttributes:getAttributeValueByName("Energy");
    if (enemyEnergy == nil) then
        do return end;
    end

    local damage = baseDamage;
    if (mainGameObject ~= nil) then
        local strength = mainGameObject:getAttributesComponent():getAttributeValueByName("Strength");
        if (strength ~= nil) then
            damage = baseDamage * strength:getValueNumber();
        end
    end

    enemyEnergy:decrementValueNumber(damage);

    log("[PrehistoricLax] Hit " .. enemyGameObject:getName() .. " for " .. toString(damage) .. " -> energy: " .. toString(enemyEnergy:getValueNumber()));

    if (hitParticle ~= nil) then
        hitParticle:setGlobalPosition(enemyGameObject:getPosition());
        if (hitParticle:isPlaying() == false or hitParticle:isActivated() == false) then
            hitParticle:setActivated(true);
        end
    end

    if (enemyEnergy:getValueNumber() <= 0) then
        enemyEnergy:setValueNumber(0);

        if (mainGameObject ~= nil) then
            local killedEnemies = mainGameObject:getAttributesComponent():getAttributeValueByName("KilledEnemies");
            if (killedEnemies ~= nil) then
                killedEnemies:incrementValueNumber(1);
            end
        end

        if (EventType.EnemyDeadEvent ~= nil) then
            local eventData = {};
            eventData["enemyId"] = enemyGameObject:getId();
            AppStateManager:getScriptEventManager():queueEvent(EventType.EnemyDeadEvent, eventData);
        end
    end
end

---------------------------------------------------------------------------------------------------

PrehistoricLax = {}

PrehistoricLax["connect"] = function(gameObject)
    PointerManager:showMouse(false);
    prehistoricLax = AppStateManager:getGameObjectController():castGameObject(gameObject);
    -- Sets the camera id for the camera behavior
    local cameraGameObject = AppStateManager:getGameObjectController():getGameObjectFromName("GameCamera");
    prehistoricLax:getCameraBehaviorComponent():setCameraGameObjectId(cameraGameObject:getId());
    
    mainGameObject = AppStateManager:getGameObjectController():getGameObjectFromId(MAIN_GAMEOBJECT_ID);

    cameraComponent = AppStateManager:getGameObjectController():getGameObjectFromName("GameCamera"):getCameraComponent();
    -- Important: Camera switch
    cameraComponent:setActivated(true);

    AppStateManager:getGameObjectController():activatePlayerController(true, prehistoricLax:getId(), true);

    areaOfInterestComponent = prehistoricLax:getAreaOfInterestComponent();
    attributesComponent = mainGameObject:getAttributesComponent();

    
    -- The player's energy lives on the main game object, next to score and level, exactly
    -- like in main.lua - the HUD reads it from there.
    energy = mainGameObject:getAttributesComponent():getAttributeValueByName("Energy");
    hitParticle = mainGameObject:getParticleFxComponentFromName("HitParticle");

    playerController = prehistoricLax:getPlayerControllerJumpNRunComponent();
    animationBlender = playerController:getAnimationBlender();

    -----------------------------------------------------------------------------------------
    -- Extra animations
    --
    -- The component registers the 14 locomotion clips from its own attributes. Everything a
    -- custom state needs on top is registered here.
    -----------------------------------------------------------------------------------------
    animationBlender:registerAnimation(AnimationBlender.ANIM_ATTACK_1, "Boy 1 Punch");
    animationBlender:registerAnimation(AnimationBlender.ANIM_ATTACK_2, "Boy 1 Heavy Kick");
    animationBlender:registerAnimation(AnimationBlender.ANIM_ATTACK_3, "Boy 1 Light Kick");
    animationBlender:registerAnimation(AnimationBlender.ANIM_TAKE_DAMAGE, "Boy 1 Damage");
    animationBlender:registerAnimation(AnimationBlender.ANIM_PICKUP_1, "Boy 1 Idle Pick Up Item");
    animationBlender:registerAnimation(AnimationBlender.ANIM_ACTION_1, "Boy 1 Pass Out");
    animationBlender:registerAnimation(AnimationBlender.ANIM_NO_IDEA, "Boy 1 Look Side");

    -----------------------------------------------------------------------------------------
    -- State setup
    --
    -- The walking state is C++ and already registered by the component. Everything below is
    -- authored here in lua and only made ADDRESSABLE - registerLuaState does not switch.
    -----------------------------------------------------------------------------------------
    playerController:registerLuaState(STATE_RAGDOLL, RagDollState);
    playerController:registerLuaState(STATE_ATTACK, AttackState);
    playerController:registerLuaState(STATE_PORTAL, PortalState);

    playerController:reactOnStateChanged(function(oldStateName, newStateName)
        log("[PrehistoricLax] State: " .. oldStateName .. " -> " .. newStateName);
    end);

    -----------------------------------------------------------------------------------------
    -- Action key
    --
    -- C++ only reports WHAT is in front of the player on the rising edge of the key. What
    -- that object is - a lever, a portal, nothing at all - is decided here.
    -----------------------------------------------------------------------------------------
    playerController:setActionKey(NOWA_A_ATTACK_1);

    playerController:reactOnActionPressed(function(otherGameObject)
        -- Never interrupt a state that is busy with something else, and never start a second
        -- swing while one is still running.
        if (playerController:isInState(STATE_WALK) == false) then
            do return end;
        end
        if (playerController:getCurrentChildStateName() ~= "") then
            do return end;
        end

        if (otherGameObject ~= nil) then
            otherGameObject = AppStateManager:getGameObjectController():castGameObject(otherGameObject);

            if (otherGameObject:getTagName() == "Portal") then
                playerController:setInteractionGameObject(otherGameObject);
                playerController:requestState(STATE_PORTAL);
                do return end;
            end
        end

        -- Everything else is a swing of the stick. requestCHILDState, not requestState:
        -- the attack then runs IN PARALLEL to the walking state, so the player keeps moving,
        -- turning and jumping while he swings instead of stopping dead.
        playerController:requestChildState(STATE_ATTACK);
    end);

    -----------------------------------------------------------------------------------------
    -- Movement reactions
    -----------------------------------------------------------------------------------------
    playerController:reactOnDirectionChanged(function(oldDirection, newDirection)
        facingDirection = newDirection;
    end);

    playerController:reactOnJump(function(jumpCount)
        -- jumpCount is 1 for the jump off the ground, 2 and up for every air jump.
        if (jumpCount >= 2) then
            local smokeParticle = prehistoricLax:getParticleFxComponentFromIndex(0);
            if (smokeParticle ~= nil) then
                smokeParticle:setActivated(true);
            end
        end
    end);

    playerController:reactOnLand(function(fallTime)
        -- The longer the fall, the harder the landing. Below half a second it is an ordinary
        -- hop and must not cost anything.
        if (fallTime > 1.5) then
            applyDamage(math.floor(fallTime * 10));
        elseif (fallTime > 0.5) then
            local smokeParticle = prehistoricLax:getParticleFxComponentFromIndex(0);
            if (smokeParticle ~= nil) then
                smokeParticle:setActivated(true);
            end
        end
    end);

    playerController:reactOnWallContact(function(otherGameObject, wallNormal)
        -- Always called, no matter what 'Use Wall Separation Mode' is set to. With the flag
        -- on, C++ has already dropped the movement input pointing into the wall, so this is
        -- purely for effects - or for a ledge grab once the flag is switched off.
        if (otherGameObject == nil) then
            do return end;
        end

        otherGameObject = AppStateManager:getGameObjectController():castGameObject(otherGameObject);
        if (otherGameObject:getTagName() == "Spikes") then
            applyDamage(20);
        end
    end);

    -----------------------------------------------------------------------------------------
    -- Pickups
    -----------------------------------------------------------------------------------------
    areaOfInterestComponent:reactOnEnter(function(otherGameObject)
        otherGameObject = AppStateManager:getGameObjectController():castGameObject(otherGameObject);
        if (otherGameObject:getCategory() == "Item") then
            if (otherGameObject:getTagName() == "Coin") then
                AppStateManager:getGameObjectController():deleteGameObject(otherGameObject:getId());
                attributesComponent:addAttributeNumber("Coins", 1);
            elseif (otherGameObject:getTagName() == "Energy") then
                AppStateManager:getGameObjectController():deleteGameObject(otherGameObject:getId());
                setEnergy(getEnergy() + 25);
            end
        elseif (otherGameObject:getCategory() == "Enemy") then
            applyDamage(25);
        elseif (otherGameObject:getCategory() == "Quester") then

        end
    end);

    -- Sent by main.lua when the enemy's weapon took the last energy away.
    if (EventType.PlayerDeadEvent ~= nil) then
        AppStateManager:getScriptEventManager():registerEventListener(EventType.PlayerDeadEvent, PrehistoricLax["onPlayerDead"]);
    end
end

PrehistoricLax["disconnect"] = function()
    PointerManager:showMouse(true);
    cameraComponent:setActivated(false);
    AppStateManager:getGameObjectController():undoAll();

    playerController = nil;
    animationBlender = nil;
end

PrehistoricLax["onPlayerDead"] = function(eventData)
    playerController:requestState(STATE_RAGDOLL);
end

---------------------------------------------------------------------------------------------------
-- States
--
-- Same shape the lua state machine has always used: enter(gameObject),
-- execute(gameObject, dt) and exit(gameObject), all optional. They now run in the SAME state
-- machine as the C++ walking state, so exactly one of them is active at a time.
---------------------------------------------------------------------------------------------------

RagDollState = { };

ragDollTime = 0;

RagDollState["enter"] = function(gameObject)
    ragDollTime = 5;
    isInvulnerable = true;

    -- No input while lying on the floor. The owner name matters: only 'ragdoll' can release
    -- this lock again, so a pickup animation running at the same time cannot unlock it.
    playerController:lockMovement("ragdoll", true);

    local ragDollComponent = playerController:getPhysicsRagDollComponent();
    if (ragDollComponent ~= nil) then
        ragDollComponent:setBoneConfigFile("PrehistoricLax2.rag");
        ragDollComponent:setState("Ragdolling");
    end
end

RagDollState["execute"] = function(gameObject, dt)
    -- Throw the body away from the direction the player was facing, but only for the first
    -- second - after that the ragdoll is left to the physics.
    if (ragDollTime >= 4) then
        local pushDirection = Vector3(-4, 0, 0);
        if (facingDirection == DIR_LEFT) then
            pushDirection = Vector3(4, 0, 0);
        end

        playerController:getPhysicsComponent():applyRequiredForceForVelocity(pushDirection);
        playerController:getPhysicsComponent():applyOmegaForce(Vector3(0, 0, -10));
    end

    ragDollTime = ragDollTime - dt;

    if (ragDollTime <= 0) then
        if (getEnergy() <= 0) then
            setEnergy(100);
        end

        playerController:requestState(STATE_WALK);
    end
end

RagDollState["exit"] = function(gameObject)
    local ragDollComponent = playerController:getPhysicsRagDollComponent();
    if (ragDollComponent ~= nil) then
        ragDollComponent:setBoneConfigFile("PrehistoricLaxPartial.rag");
        ragDollComponent:setState("PartialRagdolling");
    end

    playerController:lockMovement("ragdoll", false);
    isInvulnerable = false;
end

---------------------------------------------------------------------------------------------------

AttackState = { };

-- Hard upper bound, so a missing or broken clip cannot leave the swing running forever.
attackTimeout = 0;
-- Minimum time the swing runs before it is allowed to end. Without it a single frame in which
-- something else claims the blender would already end the attack.
attackMinimumTime = 0;
hasHitThisSwing = false;

-- Attention: this runs as a CHILD state, in parallel to WalkingStateJumpNRun.
--
-- That means the walking state is still doing its job: input, velocity, facing, the movement
-- animation and addTime(). This state must therefore NOT touch any of that, or the two fight
-- each other every frame. All it does is start the swing animation, watch for a hit and end
-- itself.
--
-- The attack clip survives because the walking state's own blend gate only overrides an
-- animation that is complete or an idle/jump one - ANIM_ATTACK_1 is neither while it plays.

AttackState["enter"] = function(gameObject)
    attackTimeout = 2;
    attackMinimumTime = 0.15;
    hasHitThisSwing = false;

    -- A new swing gets a new id. The cudgel remembers per enemy which id it already hit with,
    -- so a kinematic contact firing on every frame of the swing still costs energy only once.
    attackId = attackId + 1;

    animationBlender:blend5(AnimationBlender.ANIM_ATTACK_1, AnimationBlender.BLEND_WHILE_ANIMATING, 0.1, false);

    if (EventType.PlayerAttackEvent ~= nil) then
        local eventData = {};
        eventData["isActive"] = true;
        eventData["attackId"] = attackId;
        AppStateManager:getScriptEventManager():queueEvent(EventType.PlayerAttackEvent, eventData);
    end
end

AttackState["execute"] = function(gameObject, dt)
    -- No velocity, no facing, no addTime here - the walking state owns all of that.

    -----------------------------------------------------------------------------------------
    -- Hit detection
    --
    -- The front rays are cast by PlayerControllerComponent::update() every frame anyway, so
    -- reading them here costs nothing. Once the cudgel's kinematic contact works, this block
    -- moves into the cudgel's own script and only the timing stays here.
    --
    -- The window starts at 40% of the clip, so the hit lands when the cudgel is coming down
    -- rather than on the wind-up, and hasHitThisSwing makes one swing count once.
    -----------------------------------------------------------------------------------------
    if (hasHitThisSwing == false and animationBlender:getTimePosition() >= animationBlender:getLength() * 0.4) then
        local target = playerController:getHitGameObjectFront();
        if (target ~= nil) then
            target = AppStateManager:getGameObjectController():castGameObject(target);
            if (target:getCategory() == "Enemy") then
                hasHitThisSwing = true;
                damageEnemy(target);
            end
        end
    end

    attackTimeout = attackTimeout - dt;
    attackMinimumTime = attackMinimumTime - dt;

    -- Ends when the swing clip is no longer the active one.
    --
    -- Attention: isComplete() cannot be used for this. AnimationBlenderV2::addTime() sets the
    -- complete flag when a non looping clip reaches its end, but if there is a previous source
    -- - and there always is, the walking state had idle or walk running - it reverts to that
    -- clip and sets complete back to FALSE in the very same call. From the outside the flag is
    -- therefore never seen as true, the swing only ever ended on the two second timeout, and
    -- every key press during those two seconds was swallowed by the guard in
    -- reactOnActionPressed.
    --
    -- That auto revert is exactly what makes isAnimationActive() the right test: the moment
    -- the swing is over, the blender is back on the walking animation. The minimum time covers
    -- the first frames, where the blend has not made the attack the active clip yet.
    if (attackMinimumTime <= 0 and (animationBlender:isAnimationActive(AnimationBlender.ANIM_ATTACK_1) == false or attackTimeout <= 0)) then
        playerController:requestEndChildState();
    end
end

AttackState["exit"] = function(gameObject)
    if (EventType.PlayerAttackEvent ~= nil) then
        local eventData = {};
        eventData["isActive"] = false;
        eventData["attackId"] = attackId;
        AppStateManager:getScriptEventManager():queueEvent(EventType.PlayerAttackEvent, eventData);
    end
end

---------------------------------------------------------------------------------------------------

PortalState = { };

PortalState["enter"] = function(gameObject)
    playerController:lockMovement("portal", true);

    local portal = playerController:getInteractionGameObject();
    if (portal == nil) then
        playerController:lockMovement("portal", false);
        playerController:requestState(STATE_WALK);
        do return end;
    end

    local pathFollow = portal:getAiPathFollowComponent();
    if (pathFollow == nil) then
        playerController:lockMovement("portal", false);
        playerController:requestState(STATE_WALK);
        do return end;
    end

    local referenceId = portal:getReferenceId();
    AppStateManager:getGameObjectController():activateGameObjectComponentsFromReferenceId(referenceId, true);

    pathFollow:setActivated(true);
    pathFollow:reactOnPathGoalReached(function()
        pathFollow:setActivated(false);
        AppStateManager:getGameObjectController():activateGameObjectComponentsFromReferenceId(referenceId, false);

        -- Back to wherever the player came from, instead of hardcoding the walk state.
        playerController:requestPreviousState();
    end);
end

PortalState["execute"] = function(gameObject, dt)
    if (animationBlender:isAnimationActive(AnimationBlender.ANIM_WALK_NORTH) == false) then
        animationBlender:blend5(AnimationBlender.ANIM_WALK_NORTH, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);
    end

    animationBlender:addTime(dt * playerController:getAnimationSpeed() / animationBlender:getLength(), ANIM_OWNER);
end

PortalState["exit"] = function(gameObject)
    playerController:lockMovement("portal", false);
    playerController:setInteractionGameObject(nil);
end