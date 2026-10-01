module("WeaponStick", package.seeall);

require("init");

local stick = nil;
-- Only for the global player data (attributes).
local mainGameObject = nil;
local hitParticle = nil;
local hitSound = nil;
local playerAttackListenerId = nil;

-- Set by the PlayerAttackEvent the player's attack sends. Without it the cudgel would cost an enemy
-- energy just by brushing past him while walking.
local isPlayerAttacking = false;
local currentAttackId = 0;

-- Which way the player swings, +1 = right, -1 = left. The direction of the blow is the knockback
-- direction of a killing hit ("enemy minus stick position" pointed back at the player about half of
-- the time, because the stick is usually already inside the enemy at the moment of the contact).
local attackDirectionX = 1;

-- Which enemies were already hit with the CURRENT swing, keyed by game object id. The kinematic
-- contact fires once per physics frame for as long as the bodies overlap.
local alreadyHitThisSwing = {};

-- Runtime energy of every enemy hit so far, keyed by game object id. EnemyProfiles in init.lua is
-- static; an enemy starts with its profile energy on the first hit.
local enemyEnergies = {};

WeaponStick = {};

WeaponStick["connect"] = function(gameObject)
    stick = AppStateManager:getGameObjectController():castGameObject(gameObject);
    mainGameObject = AppStateManager:getGameObjectController():getGameObjectFromId(MAIN_GAMEOBJECT_ID);
    -- Every weapon brings its own hit effect (a sword later on has a different one).
    hitParticle = stick:getParticleFxComponentFromName("HitParticle");
    hitSound = stick:getSimpleSoundComponent();

    isPlayerAttacking = false;
    currentAttackId = 0;
    attackDirectionX = 1;
    alreadyHitThisSwing = {};
    enemyEnergies = {};

    playerAttackListenerId = AppStateManager:getScriptEventManager():registerEventListener(EventType.PlayerAttackEvent, WeaponStick["onPlayerAttacking"]);
end

WeaponStick["disconnect"] = function()
    AppStateManager:getScriptEventManager():removeEventListener(playerAttackListenerId);
    playerAttackListenerId = nil;

    isPlayerAttacking = false;
    alreadyHitThisSwing = {};
    enemyEnergies = {};
    stick = nil;
    mainGameObject = nil;
    hitParticle = nil;
    hitSound = nil;
end

WeaponStick["onPlayerAttacking"] = function(eventData)
    isPlayerAttacking = eventData["isActive"];
    attackDirectionX = eventData["attackDirectionX"];

    if (eventData["attackId"] ~= currentAttackId) then
        -- A new swing started: everybody may be hit again.
        currentAttackId = eventData["attackId"];
        alreadyHitThisSwing = {};
    end
end

WeaponStick["onKinematicContact"] = function(otherGameObject)
    -- Only a real swing does damage.
    if (false == isPlayerAttacking) then
        do return end;
    end

    otherGameObject = AppStateManager:getGameObjectController():castGameObject(otherGameObject);
    if (otherGameObject:getCategory() == "HeavyBrick") then
       --todo: Later another weapon is required to set then on the heavybrick the mass to 10 and destroy it
       -- For now and testing, set it here to
       otherGameObject:getPhysicsActiveComponent():setMass(10);
       hitSound:setActivated(true);
       do return end;
    end
    
    if (otherGameObject:getCategory() ~= "Enemy") then
        do return end;
    end

    local enemyId = otherGameObject:getId();
    if (true == alreadyHitThisSwing[enemyId]) then
        do return end;
    end

    local profile = EnemyProfiles[otherGameObject:getTagName()];

    local enemyEnergy = enemyEnergies[enemyId];
    if (enemyEnergy == nil) then
        enemyEnergy = profile.energy;
    end

    -- A corpse that is still lying around (it is deleted delayed) must not be killed a second time.
    if (enemyEnergy <= 0) then
        do return end;
    end

    alreadyHitThisSwing[enemyId] = true;

    local strength = mainGameObject:getAttributesComponent():getAttributeValueByName("Strength"):getValueNumber();
    local damage = math.floor(strength * CUDGEL_DAMAGE_FACTOR + 0.5);

    enemyEnergy = enemyEnergy - damage;
    if (enemyEnergy < 0) then
        enemyEnergy = 0;
    end
    enemyEnergies[enemyId] = enemyEnergy;

    log("[PrehistoricLax Weapon] Hit " .. otherGameObject:getName() .. " for " .. toString(damage) .. " -> energy: " .. toString(enemyEnergy));

    hitParticle:setGlobalPosition(stick:getPosition());
    if (hitParticle:isPlaying() == false or hitParticle:isActivated() == false) then
        hitParticle:setActivated(true);
    end
    hitSound:setActivated(true);

    local hitDirection = Vector3(attackDirectionX, 0, 0);

    if (enemyEnergy <= 0) then
        -- Stop the AI right now, before the ragdoll takes over.
        --
        -- Attention: BehaviorType.NONE, not STOP. MovingBehavior::update() only returns early for
        -- NONE. With STOP it keeps running its force branch and latches a horizontal velocity of 0
        -- every frame - that brakes the death knockback. setBehavior(NONE) also clears the latched
        -- walking velocity of the path follow at once.
        --
        -- The path follow component is deliberately NOT deactivated: AiComponent::setActivated(false)
        -- removes the moving behavior 0.25 seconds later, and that removal can reset the agent's
        -- velocity - in the middle of the flight. The corpse is deleted soon anyway.
        local pathFollowComponent = otherGameObject:getAiPathFollowComponent();
        if (pathFollowComponent ~= nil) then
            pathFollowComponent:getMovingBehavior():setBehavior(BehaviorType.NONE);
        end

        -- The enemy's own script does the death visuals (ragdoll, knockback, delayed delete), the
        -- player's script the experience and the kill counter.
        local eventData = {};
        eventData["enemyId"] = enemyId;
        eventData["enemyTagName"] = otherGameObject:getTagName();
        eventData["hitDirection"] = hitDirection;
        AppStateManager:getScriptEventManager():queueEvent(EventType.EnemyDeadEvent, eventData);
    else
        -- The enemy's own script reacts (e.g. the coyote turns around and runs away).
        local eventData = {};
        eventData["enemyId"] = enemyId;
        eventData["hitDirection"] = hitDirection;
        eventData["remainingEnergy"] = enemyEnergy;
        AppStateManager:getScriptEventManager():queueEvent(EventType.EnemyHitEvent, eventData);
    end
end
