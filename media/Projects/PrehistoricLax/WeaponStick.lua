module("WeaponStick", package.seeall);

require("init");

local stick = nil;
local mainGameObject = nil;
local hitParticle = nil;
local hitSound = nil;

-- Set by the PlayerAttackEvent the player's AttackState sends. Without it the cudgel would
-- cost an enemy energy just by brushing past him while walking.
isPlayerAttacking = false;
currentAttackId = 0;

-- Which enemies were already hit with the CURRENT swing, keyed by game object id.
-- The kinematic contact fires once per physics frame for as long as the two bodies overlap,
-- so without this one swing would drain an enemy in a fraction of a second.
alreadyHitThisSwing = {};

baseDamage = 10;

WeaponStick = {};

WeaponStick["connect"] = function(gameObject)
    stick = AppStateManager:getGameObjectController():castGameObject(gameObject);

    isPlayerAttacking = false;
    currentAttackId = 0;
    alreadyHitThisSwing = {};

    mainGameObject = AppStateManager:getGameObjectController():getGameObjectFromId(MAIN_GAMEOBJECT_ID);
    if (mainGameObject ~= nil) then
        hitParticle = mainGameObject:getParticleFxComponentFromName("HitParticle");
    end

    hitSound = stick:getSimpleSoundComponent();

    if (EventType.PlayerAttackEvent ~= nil) then
        AppStateManager:getScriptEventManager():registerEventListener(EventType.PlayerAttackEvent, WeaponStick["onPlayerAttacking"]);
    end
end

WeaponStick["disconnect"] = function()
    isPlayerAttacking = false;
    alreadyHitThisSwing = {};
    stick = nil;
    hitParticle = nil;
    hitSound = nil;
end

WeaponStick["onPlayerAttacking"] = function(eventData)
    isPlayerAttacking = eventData["isActive"];

    local newAttackId = eventData["attackId"];
    if (newAttackId ~= nil and newAttackId ~= currentAttackId) then
        -- A new swing started: everybody may be hit again.
        currentAttackId = newAttackId;
        alreadyHitThisSwing = {};
    end
end

WeaponStick["onKinematicContact"] = function(otherGameObject)
    if (otherGameObject == nil) then
        do return end;
    end

    -- Only a real swing does damage. Walking into an enemy with the cudgel dangling from the
    -- hand must not cost him anything.
    if (isPlayerAttacking == false) then
        do return end;
    end

    otherGameObject = AppStateManager:getGameObjectController():castGameObject(otherGameObject);

    if (otherGameObject:getCategory() ~= "Enemy") then
        do return end;
    end

    log("->hit: " .. otherGameObject:getName());

    local enemyId = otherGameObject:getId();
    if (alreadyHitThisSwing[enemyId] == true) then
        do return end;
    end
    alreadyHitThisSwing[enemyId] = true;

    log("->alreadyHitThisSwing");

    local enemyAttributes = otherGameObject:getAttributesComponent();
    if (enemyAttributes == nil) then
        do return end;
    end

    log("->enemyAttributes");

    local enemyEnergy = enemyAttributes:getAttributeValueByName("Energy");
    if (enemyEnergy == nil) then
        do return end;
    end

    log("->enemyEnergy");

    -- Knockback direction: away from the stick, flattened onto the ground plane so a
    -- hit doesn't launch the enemy straight up/down depending on stick height.
    local hitDirection = otherGameObject:getPosition() - stick:getPosition();
    hitDirection.y = 0;
    if (hitDirection:squaredLength() > 0.0001) then
        hitDirection = hitDirection:normalisedCopy();
    else
        hitDirection = Vector3(1, 0, 0);
    end

    -- Damage scales with the player's Strength attribute, like the old weapon contact did.
    local damage = baseDamage;
    if (mainGameObject ~= nil) then
        local strength = mainGameObject:getAttributesComponent():getAttributeValueByName("Strength");
        if (strength ~= nil) then
            damage = baseDamage * strength:getValueNumber();
        end
    end

    enemyEnergy:decrementValueNumber(damage);

    log("[Cudgel] Hit " .. otherGameObject:getName() .. " for " .. toString(damage) .. " -> energy: " .. toString(enemyEnergy:getValueNumber()));

    if (hitParticle ~= nil) then
        hitParticle:setGlobalPosition(stick:getPosition());
        if (hitParticle:isPlaying() == false or hitParticle:isActivated() == false) then
            hitParticle:setActivated(true);
        end
    end

    if (hitSound ~= nil) then
        hitSound:setActivated(true);
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
            eventData["enemyId"] = enemyId;
            -- Needed so the enemy's own script can knock itself back in the right
            -- direction - it has no other way to know where the hit came from.
            eventData["hitDirection"] = hitDirection;
            AppStateManager:getScriptEventManager():queueEvent(EventType.EnemyDeadEvent, eventData);
        end
    else
        -- Not dead yet: play the hit-reaction animation.
        local animationBlender = otherGameObject:getAnimationComponentV2():getAnimationBlender();
        if (animationBlender ~= nil) then
            animationBlender:blend5(AnimationBlender.ANIM_TAKE_DAMAGE, AnimationBlender.BLEND_WHILE_ANIMATING, 0.1, false);
        end
    end
end