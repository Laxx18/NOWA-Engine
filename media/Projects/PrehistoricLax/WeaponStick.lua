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

-- Which way the player swings, +1 = right, -1 = left. Sent along with the PlayerAttackEvent.
--
-- The knockback direction used to be "enemy position minus stick position". That is wrong for a
-- swing: at the moment of the contact the stick is usually already INSIDE the enemy or even past
-- its center, so the vector pointed back towards the player about half of the time - the dead
-- rhino flew into the wrong direction. The player's facing is the direction of the blow.
attackDirectionX = 1;

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
    attackDirectionX = 1;
    alreadyHitThisSwing = {};

    mainGameObject = AppStateManager:getGameObjectController():getGameObjectFromId(MAIN_GAMEOBJECT_ID);
    if (mainGameObject ~= nil) then
        hitParticle = mainGameObject:getParticleFxComponentFromName("HitParticle");
    end

    hitSound = stick:getSimpleSoundComponentFromName("Hit");

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

    local directionX = eventData["attackDirectionX"];
    if (directionX ~= nil) then
        attackDirectionX = directionX;
    end

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
    
    if (hitSound ~= nil) then
        hitSound:setActivated(true);
    end

    otherGameObject = AppStateManager:getGameObjectController():castGameObject(otherGameObject);

    if (otherGameObject:getCategory() ~= "Enemy") then
        do return end;
    end

    local enemyId = otherGameObject:getId();
    if (alreadyHitThisSwing[enemyId] == true) then
        do return end;
    end

    local attributesComponent = otherGameObject:getAttributesComponent();
    if (attributesComponent == nil) then
        do return end;
    end

    local enemyEnergy = attributesComponent:getAttributeValueByName("Energy");
    if (enemyEnergy == nil) then
        do return end;
    end

    -- A corpse that is still lying around (it is deleted delayed) must not be killed a second time.
    if (enemyEnergy:getValueNumber() <= 0) then
        do return end;
    end

    alreadyHitThisSwing[enemyId] = true;

    -- Knockback direction: the direction of the blow, flattened onto the ground plane.
    local hitDirection = Vector3(attackDirectionX, 0, 0);

    -- Damage scales with the player's Strength attribute, like the old weapon contact did.
    local damage = baseDamage;
    if (mainGameObject ~= nil) then
        local strength = mainGameObject:getAttributesComponent():getAttributeValueByName("Strength");
        if (strength ~= nil) then
            damage = baseDamage * strength:getValueNumber();
        end
    end

    enemyEnergy:decrementValueNumber(damage);

    log("[PrehistoricLax Weapon] Hit " .. otherGameObject:getName() .. " for " .. toString(damage) .. " -> energy: " .. toString(enemyEnergy:getValueNumber()));

    if (hitParticle ~= nil) then
        hitParticle:setGlobalPosition(stick:getPosition());
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

        -- The enemy's own script does the death visuals (stun, ragdoll, knockback). Blending an
        -- animation here as well only fought against it.
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
        local animationComponent = otherGameObject:getAnimationComponentV2();
        if (animationComponent ~= nil) then
            animationComponent:getAnimationBlender():blend5(AnimationBlender.ANIM_TAKE_DAMAGE, AnimationBlender.BLEND_WHILE_ANIMATING, 0.1, false);
        end

        -- Enemy specific reaction (e.g. the coyote turns around). Every enemy script that wants to
        -- react listens to this event and checks the enemyId, exactly like with EnemyDeadEvent.
        if (EventType.EnemyHitEvent ~= nil) then
            local eventData = {};
            eventData["enemyId"] = enemyId;
            eventData["hitDirection"] = hitDirection;
            eventData["remainingEnergy"] = enemyEnergy:getValueNumber();
            AppStateManager:getScriptEventManager():queueEvent(EventType.EnemyHitEvent, eventData);
        end
    end
end
