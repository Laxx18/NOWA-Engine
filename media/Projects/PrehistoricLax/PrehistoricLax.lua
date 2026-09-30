module("PrehistoricLax", package.seeall);
-- Scene: Level1

require("init");

-- File scope locals are fine here: the state tables below live in the SAME chunk, so they
-- close over these as upvalues. What does NOT work is declaring them inside connect() - the
-- state functions cannot see those, which is the trap the old PrehistoricLax_0 script ran
-- into ("variables created in connect are out of scope in WalkState execute").
local prehistoricLax = nil;
local cameraComponent = nil;
local areaOfInterestComponent = nil;
local attributesComponent = nil;
local mainGameObject = nil;
local playerController = nil;
local animationBlender = nil;
local moneySound = nil;
local hurtSound = nil;
-- Player attributes (main game object, saved) and the status bar texts that show them.
local energy = nil;
local strength = nil;
local experience = nil;
local ascension = nil;
local level = nil;
local coins = nil;
local killedEnemies = nil;
local energyProgress = nil;
-- Damage trail of the energy bar: a second progress bar (skin ProgressValueBarTrail) right under
-- 'EnergyProgress' (skin ProgressValueBarOverlay). It holds the old value for a moment after a hit and
-- then shrinks, like the enemy energy bars.
local energyTrailProgress = nil;
local energyPercent = 0;
local energyTrailPercent = 0;
local energyTrailDelay = 0;
local strengthText = nil;
local levelText = nil;
local experienceText = nil;
local coinsText = nil;
local killedEnemiesText = nil;
local hitParticle = nil;
local playerDeadListenerId = nil;
local enemyDeadListenerId = nil;
local enemyNearPlayerListenerId = nil;

-- Mirrors NOWA::Direction from PlayerControllerComponents.h.
-- Attention: this order is NOT the one the old script used - RIGHT comes before LEFT here.
local DIR_NONE, DIR_RIGHT, DIR_LEFT, DIR_UP, DIR_DOWN = 0, 1, 2, 3, 4;

-- Which way the player is facing, kept up to date by reactOnDirectionChanged, so a state can
-- hold the facing or throw the ragdoll in the right direction without looking at the input.
local facingDirection = DIR_RIGHT;

-- Counted up on every swing and sent along with the PlayerAttackEvent. The cudgel script uses
-- it to make sure ONE swing can only ever cost an enemy energy ONCE, no matter how many
-- frames the kinematic contact keeps firing.
local attackId = 0;

-- True while the player lies in the ragdoll. The short invulnerability after a hit (i-frames) is
-- tracked separately in invulnerableTimer, so the two can never switch each other off.
local isInvulnerable = false;

---------------------------------------------------------------------------------------------------
-- Attack
--
-- The swing is NOT a state and not a child state any more. It is an OVERLAY animation that is
-- layered on top of whatever the walking state is playing, plus a timer that lives right here.
--
-- Why the overlay and not a state:
--
--   A state - child state or not - owns the animation. While it ran, the walking state was not
--   allowed to blend, so the player kept the punch pose while the physics carried on moving him:
--   he was sliding across the floor in a punch. Layering solves exactly that. The walking state
--   keeps doing its job - input, velocity, facing, the locomotion clip and addTime() - and the
--   punch is added on top of it. That is the "oben schlagen, unten laufen" case.
--
--   The overlay also does not go through AnimationBlenderV2::internalBlend(), which refuses every
--   new blend while a non looping clip is still running. That guard is what made the second press
--   do nothing for a second or two. An overlay has no such gate.
--
-- The timer is driven from PrehistoricLax["update"], which the LuaScriptComponent calls every frame.
---------------------------------------------------------------------------------------------------

-- Which bone chain the swing owns. That bone and ALL of its children are driven by the punch,
-- everything below it keeps walking.
--
--     Boy 1 Pelvis
--       Boy 1 Spine  ->  Spine1 -> Spine2 -> Neck/Head, clavicles, arms, hands, hoodie, backpack
--       Boy 1 L Thigh -> L Calf -> L Foot -> L Toe0
--       Boy 1 R Thigh -> R Calf -> R Foot -> R Toe0
--
-- The pelvis itself stays with the walk animation, which keeps the hip movement of the walk cycle
-- intact while the arms swing.
local ATTACK_OVERLAY_BONE = "Boy 1 Spine";

-- Prints the overlay bone hierarchy and one line per frame while the swing runs.
local ATTACK_DEBUG = false;

-- ATTACK_CHAIN_INFLUENCE: how much of the spine chain the punch owns (1.0 = walk has no say above
-- the pelvis). ATTACK_BODY_INFLUENCE: how far the punch reaches into pelvis and legs; 0.25 to 0.35
-- lets the whole body lean into the swing. Above 0.4 the root motion of the clip starts to pull the
-- character backwards.
local ATTACK_CHAIN_INFLUENCE = 1.0;
local ATTACK_BODY_INFLUENCE = 0.3;

-- Playback speed of the punch. NOT the player controller's animation speed, which follows the
-- walking speed and made a 0.87 second punch take two to three seconds.
local ATTACK_SPEED = 1.0;

-- Fade in / out of the overlay. Short, but not zero - a hard cut makes the arm jump.
local ATTACK_BLEND_IN = 0.08;

-- The part of the swing that actually hurts, as a fraction of the punch clip. Used for the trade
-- rule against enemy attacks.
local ATTACK_HIT_START = 0.35;
local ATTACK_HIT_END = 0.75;

-- Pure safety net. The swing ends on the clip, not on this.
local ATTACK_TIMEOUT = 2.0;

local isAttacking = false;
local attackTime = 0;

---------------------------------------------------------------------------------------------------
-- Enemy combat
--
--   1. Telegraph. An enemy starts ITS attack (ANIM_ATTACK_1) as soon as the player comes within
--      its attackReach (EnemyNearPlayerEvent from the enemy's script) or touches it. The damage
--      lands at the impact moment of that attack (attackImpactDelay), and only if the player is
--      still within reach. A player who jumps or steps away in time dodges.
--   2. Trade rule. If the player's own swing is in its active window and he faces the enemy, the
--      enemy's attack is not started resp. does not land (at impact).
--   3. Hit reaction. Energy loss, sound, particle and the KnockbackState, which throws the player
--      away from the enemy. Getting hit cancels the own swing.
--   4. I-frames. PLAYER_IFRAME_TIME seconds of invulnerability with blinking after a hit.
--   5. Pressure. The enemy attacks again after its cooldown while the player stays in reach
--      (its script keeps sending EnemyNearPlayerEvent).
--
-- All values per enemy type live in EnemyProfiles (init.lua), keyed by the enemy's tag name.
---------------------------------------------------------------------------------------------------
local invulnerableTimer = 0;
local blinkTimer = 0;
local playerBlinkVisible = true;

-- Set by hitPlayer, used by the KnockbackState.
local knockbackVelocityX = 0;
local knockbackVelocityUp = 0;
local knockbackTime = 0;

-- Per enemy attack state, keyed by the enemy's game object id. Entries are dropped on the enemy's
-- EnemyDeadEvent, long before its delayed delete - so every id in here refers to a live enemy.
local enemyCombat = {};

-- Enemies killed but not deleted yet, keyed by game object id (from EnemyDeadEvent).
local deadEnemies = {};

---------------------------------------------------------------------------------------------------
-- Helpers
---------------------------------------------------------------------------------------------------

function getEnergy()
    return energy:getValueNumber();
end

function getLevel()
    return level:getValueNumber();
end

-- Clamped to the max energy of the current level. The progress bar has a range of 100, so it
-- shows the percentage.
function setEnergy(value)
    local maxEnergy = getMaxEnergy(getLevel());
    if (value > maxEnergy) then
        value = maxEnergy;
    end
    energy:setValueNumber(value);

    energyPercent = math.floor(value * 100 / maxEnergy + 0.5);
    energyProgress:setValue(energyPercent);

    if (energyPercent < energyTrailPercent) then
        -- Damage: the trail stays for a moment, see updateEnergyTrail.
        energyTrailDelay = HUD_ENERGY_TRAIL_DELAY;
    else
        -- Heal: the trail simply follows.
        energyTrailPercent = energyPercent;
        energyTrailProgress:setValue(energyTrailPercent);
    end
end

function updateEnergyTrail(dt)
    if (energyTrailPercent <= energyPercent) then
        do return end;
    end

    if (energyTrailDelay > 0) then
        energyTrailDelay = energyTrailDelay - dt;
        do return end;
    end

    energyTrailPercent = energyTrailPercent - HUD_ENERGY_TRAIL_SPEED * dt;
    if (energyTrailPercent < energyPercent) then
        energyTrailPercent = energyPercent;
    end
    energyTrailProgress:setValue(math.floor(energyTrailPercent + 0.5));
end

-- Writes all attributes into the status bar.
function updateHud()
    strengthText:setCaption("Strength: " .. strength:getValueNumber());
    levelText:setCaption("Level: " .. getLevel());
    experienceText:setCaption("Experience: " .. experience:getValueNumber() .. " / " .. ascension:getValueNumber());
    coinsText:setCaption("Coins: " .. coins:getValueNumber());
    killedEnemiesText:setCaption("Killed Enemies: " .. killedEnemies:getValueNumber());
    setEnergy(getEnergy());
end

-- Adds experience and levels up as often as it is enough for. Every level up gives strength and max
-- energy (see init.lua) and refills the energy.
function addExperience(amount)
    local value = experience:getValueNumber() + amount;
    local hasLevelUp = false;

    while (value >= ascension:getValueNumber()) do
        value = value - ascension:getValueNumber();
        level:setValueNumber(getLevel() + 1);
        strength:setValueNumber(strength:getValueNumber() + STRENGTH_PER_LEVEL);
        ascension:setValueNumber(getRequiredExperience(getLevel()));
        hasLevelUp = true;
    end

    experience:setValueNumber(value);

    if (true == hasLevelUp) then
        setEnergy(getMaxEnergy(getLevel()));
        log("[PrehistoricLax] Level up -> level: " .. getLevel() .. " strength: " .. strength:getValueNumber() .. " max energy: " .. getMaxEnergy(getLevel()));
    end

    updateHud();
end

-- +1 when the player faces right, -1 when he faces left.
function getFacingSign()
    if (facingDirection == DIR_LEFT) then
        return -1;
    end
    return 1;
end

-- Central damage entry point for energy and death. The visible reaction is up to the caller.
function applyDamage(amount)
    if (true == isInvulnerable or invulnerableTimer > 0) then
        do return end;
    end

    local newEnergy = getEnergy() - amount;
    if (newEnergy < 0) then
        newEnergy = 0;
    end
    setEnergy(newEnergy);

    if (newEnergy <= 0) then
        -- requestState is queued and applied at the top of the next update, so it is safe to
        -- call from any closure.
        playerController:requestState("RagDollState");
    end
end

-- Whether an enemy attack may hurt the player right now.
function canPlayerBeHit()
    if (true == isInvulnerable or invulnerableTimer > 0) then
        return false;
    end
    -- No hits while ragdolling or walking through a portal.
    return playerController:isInState("WalkingStateJumpNRun");
end

function setPlayerVisible(visible)
    playerBlinkVisible = visible;
    prehistoricLax:setVisible(visible);
end

-- The complete hit reaction of the player.
function hitPlayer(sourcePosition, profile)
    if (false == canPlayerBeHit()) then
        do return end;
    end

    -- Getting hit cancels the own swing.
    stopAttack();

    applyDamage(profile.strength);

    local playerPosition = prehistoricLax:getPosition();

    hitParticle:setGlobalPosition(playerPosition);
    if (hitParticle:isPlaying() == false or hitParticle:isActivated() == false) then
        hitParticle:setActivated(true);
    end
    hurtSound:setActivated(true);

    log("[PrehistoricLax] Player hit for " .. toString(profile.strength) .. " -> energy: " .. toString(getEnergy()));

    -- The last hit: applyDamage already requested the RagDollState, which takes over from here.
    if (getEnergy() <= 0) then
        do return end;
    end

    invulnerableTimer = PLAYER_IFRAME_TIME;
    blinkTimer = PLAYER_IFRAME_BLINK_INTERVAL;

    -- Away from the enemy. Standing exactly on top of each other: backwards from the facing.
    local directionX = -getFacingSign();
    if (playerPosition.x > sourcePosition.x) then
        directionX = 1;
    elseif (playerPosition.x < sourcePosition.x) then
        directionX = -1;
    end

    knockbackVelocityX = directionX * profile.playerKnockbackHorizontal;
    knockbackVelocityUp = profile.playerKnockbackUp;
    knockbackTime = profile.playerKnockbackTime;
    playerController:requestState("KnockbackState");
end

function isPlayerInReach(enemyGameObject, profile)
    local delta = prehistoricLax:getPosition() - enemyGameObject:getPosition();
    return math.abs(delta.x) <= profile.attackReach and math.abs(delta.y) <= profile.attackReachVertical;
end

-- Trade rule: the player's swing is in its active window and he faces the enemy.
function isPlayerSwingBeatingEnemy(enemyGameObject)
    if (false == isAttacking) then
        return false;
    end

    local progress = animationBlender:getOverlayProgress();
    if (progress < ATTACK_HIT_START or progress > ATTACK_HIT_END) then
        return false;
    end

    local towardsEnemyX = enemyGameObject:getPosition().x - prehistoricLax:getPosition().x;
    return towardsEnemyX * getFacingSign() > 0;
end

-- The moment the enemy's attack hurts.
function resolveEnemyImpact(enemyGameObject, profile)
    if (false == isPlayerInReach(enemyGameObject, profile)) then
        -- Dodged.
        do return end;
    end

    if (true == isPlayerSwingBeatingEnemy(enemyGameObject)) then
        -- The player's swing won the exchange.
        do return end;
    end

    hitPlayer(enemyGameObject:getPosition(), profile);
end

function startEnemyAttack(enemyGameObject, profile)
    local enemyId = enemyGameObject:getId();
    local state = enemyCombat[enemyId];

    -- Still attacking or cooling down.
    if (state ~= nil and (state.cooldownTimer > 0 or state.recoverTimer > 0)) then
        do return end;
    end

    -- Attention: looping on purpose. A non looping clip blocks every other blend until it is
    -- complete, and the enemy's MovingBehavior slows the clip down while the enemy stands still
    -- (pressed against the player) - the enemy got stuck in its attack. The recover timer below
    -- ends the attack instead.
    enemyGameObject:getAnimationComponentV2():getAnimationBlender():blend5(AnimationBlender.ANIM_ATTACK_1, AnimationBlender.BLEND_WHILE_ANIMATING, 0.1, true);

    state = {};
    state.profile = profile;
    state.cooldownTimer = profile.attackCooldown;
    state.impactTimer = profile.attackImpactDelay;
    state.recoverTimer = profile.attackDuration;
    enemyCombat[enemyId] = state;

    if (profile.attackImpactDelay <= 0) then
        state.impactTimer = -1;
        resolveEnemyImpact(enemyGameObject, profile);
    end
end

-- Advances one enemy's attack. Returns false if the entry can be dropped.
function updateSingleEnemyCombat(enemyId, state, dt)
    local enemyGameObject = AppStateManager:getGameObjectController():getGameObjectFromId(enemyId);
    local profile = state.profile;

    if (state.cooldownTimer > 0) then
        state.cooldownTimer = state.cooldownTimer - dt;
    end

    if (state.impactTimer >= 0) then
        state.impactTimer = state.impactTimer - dt;
        if (state.impactTimer <= 0) then
            state.impactTimer = -1;
            resolveEnemyImpact(enemyGameObject, profile);
        end
    end

    if (state.recoverTimer > 0) then
        state.recoverTimer = state.recoverTimer - dt;
        if (state.recoverTimer <= 0) then
            state.recoverTimer = 0;
            -- Back to walking / running after the attack.
            enemyGameObject:getAnimationComponentV2():getAnimationBlender():blend5(AnimationBlender[profile.locomotionAnimation], AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);
        end
    end

    -- Done: the next EnemyNearPlayerEvent starts the next attack.
    return state.cooldownTimer > 0 or state.recoverTimer > 0 or state.impactTimer >= 0;
end

function updateEnemyCombat(dt)
    local finishedEnemyIds = {};

    for enemyId, state in pairs(enemyCombat) do
        if (false == updateSingleEnemyCombat(enemyId, state, dt)) then
            table.insert(finishedEnemyIds, enemyId);
        end
    end

    for i = 1, #finishedEnemyIds do
        enemyCombat[finishedEnemyIds[i]] = nil;
    end
end

-- I-frames and blink.
function updateHitReaction(dt)
    if (invulnerableTimer > 0) then
        invulnerableTimer = invulnerableTimer - dt;

        blinkTimer = blinkTimer - dt;
        if (blinkTimer <= 0) then
            blinkTimer = PLAYER_IFRAME_BLINK_INTERVAL;
            setPlayerVisible(false == playerBlinkVisible);
        end

        if (invulnerableTimer <= 0) then
            invulnerableTimer = 0;
            setPlayerVisible(true);
        end
    end

end

-- Tells the cudgel script whether a real swing is going on and in which direction.
function sendAttackEvent(isActive)
    local eventData = {};
    eventData["isActive"] = isActive;
    eventData["attackId"] = attackId;
    eventData["attackDirectionX"] = getFacingSign();
    AppStateManager:getScriptEventManager():queueEvent(EventType.PlayerAttackEvent, eventData);
end

function startAttack()
    -- A new swing gets a new id, see attackId.
    attackId = attackId + 1;
    attackTime = 0;

    animationBlender:setOverlayAnimationForBoneChain1(AnimationBlender.ANIM_ATTACK_1, ATTACK_OVERLAY_BONE, ATTACK_BLEND_IN, false);
    isAttacking = true;

    sendAttackEvent(true);
end

function stopAttack()
    if (false == isAttacking) then
        do return end;
    end

    isAttacking = false;
    attackTime = 0;

    -- A non looping overlay fades itself out when the clip is over, so this is only needed when
    -- the swing is cut short - by a hit, a ragdoll, a portal or the safety timeout.
    if (true == animationBlender:isOverlayAnimationActive()) then
        animationBlender:clearOverlayAnimation(ATTACK_BLEND_IN);
    end

    sendAttackEvent(false);
end

-- The swing timing. The walking state stays untouched and keeps animating the player while he hits.
function updateAttack(dt)
    if (false == isAttacking) then
        do return end;
    end

    -- Going down or stepping into a portal cancels the swing.
    if (false == playerController:isInState("WalkingStateJumpNRun")) then
        stopAttack();
        do return end;
    end

    attackTime = attackTime + dt;

    -- The swing is over only once the overlay is completely gone - clip finished AND faded out.
    -- Attention: isOverlayBlendingOut() must NOT be used here, it cuts off the fade back.
    if (false == animationBlender:isOverlayAnimationActive()) then
        stopAttack();
        do return end;
    end

    if (attackTime >= ATTACK_TIMEOUT) then
        log("[PrehistoricLax] Attack safety timeout - the animation blender was not ticked.");
        stopAttack();
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

    cameraComponent = cameraGameObject:getCameraComponent();
    -- Important: Camera switch
    cameraComponent:setActivated(true);

    AppStateManager:getGameObjectController():activatePlayerController(true, prehistoricLax:getId(), true);

    areaOfInterestComponent = prehistoricLax:getAreaOfInterestComponent();
    
    -- The player's attributes live on the main game object, next to score and level - the HUD
    -- reads them from there, and they are saved.
    attributesComponent = mainGameObject:getAttributesComponent();
    energy = attributesComponent:getAttributeValueByName("Energy");
    strength = attributesComponent:getAttributeValueByName("Strength");
    experience = attributesComponent:getAttributeValueByName("Experience");
    ascension = attributesComponent:getAttributeValueByName("Ascension");
    level = attributesComponent:getAttributeValueByName("Level");
    coins = attributesComponent:getAttributeValueByName("Coins");
    killedEnemies = attributesComponent:getAttributeValueByName("KilledEnemies");

    energyProgress = mainGameObject:getMyGUIProgressBarComponentFromName("EnergyProgress");
    energyTrailProgress = mainGameObject:getMyGUIProgressBarComponentFromName("EnergyTrailProgress");
    -- 0, so the first setEnergy (in updateHud below) takes the heal path and puts the trail in place.
    energyTrailPercent = 0;
    energyTrailDelay = 0;
    strengthText = mainGameObject:getMyGUITextComponentFromName("Strength");
    levelText = mainGameObject:getMyGUITextComponentFromName("Level");
    experienceText = mainGameObject:getMyGUITextComponentFromName("Experience");
    coinsText = mainGameObject:getMyGUITextComponentFromName("Coins");
    killedEnemiesText = mainGameObject:getMyGUITextComponentFromName("KilledEnemies");

    -- The needed experience always follows the formula in init.lua, also for a saved game made
    -- with other balancing values.
    ascension:setValueNumber(getRequiredExperience(getLevel()));
    updateHud();

    playerController = prehistoricLax:getPlayerControllerJumpNRunComponent();
    animationBlender = playerController:getAnimationBlender();
    hitParticle = prehistoricLax:getParticleFxComponentFromName("HitParticle");
    moneySound = prehistoricLax:getSimpleSoundComponentFromName("Money");
    hurtSound = prehistoricLax:getSimpleSoundComponentFromName("Hurt");

    isAttacking = false;
    attackTime = 0;
    isInvulnerable = false;
    invulnerableTimer = 0;
    blinkTimer = 0;
    playerBlinkVisible = true;
    enemyCombat = {};
    deadEnemies = {};

    -- The punch runs at its own speed and weighting.
    animationBlender:setOverlaySpeed(ATTACK_SPEED);
    animationBlender:setOverlayInfluence(ATTACK_CHAIN_INFLUENCE, ATTACK_BODY_INFLUENCE);

    if (true == ATTACK_DEBUG) then
        animationBlender:setDebugLog(true);
    end

    -----------------------------------------------------------------------------------------
    -- Extra animations
    --
    -- The component registers the 14 locomotion clips from its own attributes. Everything a
    -- custom state needs on top is registered here.
    -----------------------------------------------------------------------------------------
    animationBlender:registerAnimation(AnimationBlender.ANIM_IDLE_1, "Boy 1 Idle");
    animationBlender:registerAnimation(AnimationBlender.ANIM_IDLE_2, "Boy 1 Idle Turn Left");
    animationBlender:registerAnimation(AnimationBlender.ANIM_IDLE_3, "Boy 1 Idle Turn Right");
    animationBlender:registerAnimation(AnimationBlender.ANIM_WALK_NORTH, "Boy 1 Walk");
    animationBlender:registerAnimation(AnimationBlender.ANIM_WALK_SOUTH, "Boy 1 Walk Backwards");
    animationBlender:registerAnimation(AnimationBlender.ANIM_WALK_WEST, "Boy 1 Walk Turn Left");
    animationBlender:registerAnimation(AnimationBlender.ANIM_WALK_EAST, "Boy 1 Walk Turn Right");
    animationBlender:registerAnimation(AnimationBlender.ANIM_JUMP_START, "Boy 1 Jump Up1");
    animationBlender:registerAnimation(AnimationBlender.ANIM_JUMP_WALK, "Boy 1 Jump Up1");
    animationBlender:registerAnimation(AnimationBlender.ANIM_HIGH_JUMP_END, "Boy 1 Get Up");
    animationBlender:registerAnimation(AnimationBlender.ANIM_JUMP_END, "Boy 1 Land");
    animationBlender:registerAnimation(AnimationBlender.ANIM_FALL, "Boy 1 Damage");
    animationBlender:registerAnimation(AnimationBlender.ANIM_RUN, "Boy 1 Run");
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
    -- The attack is NOT a state, it is an overlay, see the block at the top.
    -----------------------------------------------------------------------------------------
    playerController:registerLuaState("RagDollState", RagDollState);
    playerController:registerLuaState("PortalState", PortalState);
    playerController:registerLuaState("KnockbackState", KnockbackState);

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
        -- No swinging while ragdolling or walking through a portal.
        if (false == playerController:isInState("WalkingStateJumpNRun")) then
            do return end;
        end

        -- nil simply means: nothing in front of the player.
        if (otherGameObject ~= nil) then
            otherGameObject = AppStateManager:getGameObjectController():castGameObject(otherGameObject);

            if (otherGameObject:getTagName() == "Portal") then
                playerController:setInteractionGameObject(otherGameObject);
                playerController:requestState("PortalState");
                do return end;
            end
        end

        -- One swing at a time, and it always finishes.
        if (true == isAttacking) then
            do return end;
        end

        startAttack();
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
            prehistoricLax:getParticleFxComponentFromIndex(0):setActivated(true);
        end
    end);

    playerController:reactOnLand(function(fallTime)
        -- Below half a second it is an ordinary hop.
        if (fallTime > 0.5) then
            prehistoricLax:getParticleFxComponentFromName("DustLand"):setActivated(true);
        end
    end);

    playerController:reactOnAccelerationChanged(function(tempSpeed, topSpeed)
        prehistoricLax:getParticleFxComponentFromName("DustStep"):setActivated(true);
    end);

    -----------------------------------------------------------------------------------------
    -- Pickups
    -----------------------------------------------------------------------------------------
    areaOfInterestComponent:reactOnEnter(function(otherGameObject)
        otherGameObject = AppStateManager:getGameObjectController():castGameObject(otherGameObject);
        if (otherGameObject:getCategory() == "Item") then
            if (otherGameObject:getTagName() == "Coin") then
                AppStateManager:getGameObjectController():deleteGameObject(otherGameObject:getId());
                coins:setValueNumber(coins:getValueNumber() + 1);
                moneySound:setActivated(true);
                updateHud();
            elseif (otherGameObject:getTagName() == "Energy") then
                AppStateManager:getGameObjectController():deleteGameObject(otherGameObject:getId());
                setEnergy(getEnergy() + ENERGY_PICKUP_AMOUNT);
            end
        end
        -- Enemies are handled by the enemy combat above.
    end);

    -- Sent by main.lua when the enemy's weapon took the last energy away.
    playerDeadListenerId = AppStateManager:getScriptEventManager():registerEventListener(EventType.PlayerDeadEvent, PrehistoricLax["onPlayerDead"]);
    -- A killed enemy must not attack anymore while its corpse is still lying around, and the kill
    -- gives experience.
    enemyDeadListenerId = AppStateManager:getScriptEventManager():registerEventListener(EventType.EnemyDeadEvent, PrehistoricLax["onEnemyDead"]);
    -- An enemy came within its attack reach.
    enemyNearPlayerListenerId = AppStateManager:getScriptEventManager():registerEventListener(EventType.EnemyNearPlayerEvent, PrehistoricLax["onEnemyNearPlayer"]);
end

PrehistoricLax["disconnect"] = function()
    AppStateManager:getScriptEventManager():removeEventListener(playerDeadListenerId);
    AppStateManager:getScriptEventManager():removeEventListener(enemyDeadListenerId);
    AppStateManager:getScriptEventManager():removeEventListener(enemyNearPlayerListenerId);
    playerDeadListenerId = nil;
    enemyDeadListenerId = nil;
    enemyNearPlayerListenerId = nil;

    PointerManager:showMouse(true);
    cameraComponent:setActivated(false);
    AppStateManager:getGameObjectController():undoAll();

    isAttacking = false;

    -- Make sure the player is not left invisible by the i-frame blink.
    invulnerableTimer = 0;
    setPlayerVisible(true);
    enemyCombat = {};
    deadEnemies = {};

    playerController = nil;
    animationBlender = nil;
end

---------------------------------------------------------------------------------------------------
-- Called by the LuaScriptComponent every frame.
---------------------------------------------------------------------------------------------------
PrehistoricLax["update"] = function(dt)
    updateEnergyTrail(dt);
    updateHitReaction(dt);
    updateEnemyCombat(dt);
    updateAttack(dt);
end

PrehistoricLax["onPlayerDead"] = function(eventData)
    playerController:requestState("RagDollState");
end

PrehistoricLax["onEnemyDead"] = function(eventData)
    deadEnemies[eventData["enemyId"]] = true;
    enemyCombat[eventData["enemyId"]] = nil;

    killedEnemies:setValueNumber(killedEnemies:getValueNumber() + 1);
    addExperience(getExperienceForKill(EnemyProfiles[eventData["enemyTagName"]], getLevel()));
end

-- Damage over time while touching something dangerous (spikes, lava). No knockback and no
-- i-frames of its own - but it respects the i-frames of an enemy hit.
PrehistoricLax["onPlayerDangerContact"] = function(gameObject0, gameObject1, contact)
    if (false == canPlayerBeHit()) then
        do return end;
    end

    applyDamage(0.2);

    if (animationBlender:isAnimationActive(AnimationBlender.ANIM_TAKE_DAMAGE) == false) then
        animationBlender:blend5(AnimationBlender.ANIM_TAKE_DAMAGE, AnimationBlender.BLEND_WHILE_ANIMATING, 0.1, false);
        hurtSound:setActivated(true);
    end
end

-- Starts the enemy's attack, if it may attack right now.
function tryStartEnemyAttack(enemyGameObject)
    -- A killed enemy does not attack anymore.
    if (true == deadEnemies[enemyGameObject:getId()]) then
        do return end;
    end

    if (false == canPlayerBeHit()) then
        do return end;
    end

    -- Trade rule: the cudgel hits first, the enemy does not get to attack.
    if (true == isPlayerSwingBeatingEnemy(enemyGameObject)) then
        do return end;
    end

    startEnemyAttack(enemyGameObject, EnemyProfiles[enemyGameObject:getTagName()]);
end

-- Sent by the enemy's script every frame while the player is within its attack reach. Without this the enemy
-- only attacked on body contact - and the cudgel reaches further, so a coyote that turns around on
-- every hit never touched the player and never attacked.
PrehistoricLax["onEnemyNearPlayer"] = function(eventData)
    tryStartEnemyAttack(AppStateManager:getGameObjectController():getGameObjectFromId(eventData["enemyId"]));
end

-- Called once when the player's body starts touching an enemy's body.
PrehistoricLax["onPlayerEnemyContactOnce"] = function(gameObject0, gameObject1, contact)
    local enemyGameObject = AppStateManager:getGameObjectController():castGameObject(gameObject0);
    if (enemyGameObject:getCategory() ~= "Enemy") then
        enemyGameObject = AppStateManager:getGameObjectController():castGameObject(gameObject1);
    end

    tryStartEnemyAttack(enemyGameObject);
end

---------------------------------------------------------------------------------------------------
-- States
--
-- Same shape the lua state machine has always used: enter(gameObject),
-- execute(gameObject, dt) and exit(gameObject), all optional. They run in the SAME state
-- machine as the C++ walking state, so exactly one of them is active at a time.
---------------------------------------------------------------------------------------------------

RagDollState = { };

local ragDollTime = 0;

RagDollState["enter"] = function(gameObject)
    ragDollTime = PLAYER_RAGDOLL_TIME;
    isInvulnerable = true;

    -- A swing that was still in the air when the last energy went is dropped here.
    stopAttack();

    -- The ragdoll owns the body now: no blinking.
    invulnerableTimer = 0;
    setPlayerVisible(true);

    -- No input while lying on the floor. Only the owner 'ragdoll' can release this lock again.
    playerController:lockMovement("ragdoll", true);
    playerController:getPhysicsRagDollComponent():setState("Ragdolling");

    -- ONE impulse backwards, then the physics alone decides: the body flies a bit, falls and stays.
    --
    -- Attention: not applyRequiredForceForVelocity / applyOmegaForce. Both commands are LATCHED by
    -- the physics and re-applied in every substep until the next command arrives - and in this state
    -- none arrives. The former per frame push of (4, 0, 0) was therefore held forever, with a vertical
    -- velocity of 0 on top: the dead player floated and slid out of the level, spinning.
    playerController:getPhysicsComponent():applyRequiredForceForJumpVelocity(Vector3(-getFacingSign() * PLAYER_DEATH_KNOCKBACK_HORIZONTAL, PLAYER_DEATH_KNOCKBACK_UP, 0));
end

RagDollState["execute"] = function(gameObject, dt)
    ragDollTime = ragDollTime - dt;

    if (ragDollTime <= 0) then
        if (getEnergy() <= 0) then
            setEnergy(getMaxEnergy(getLevel()));
        end

        --playerController:requestState("WalkingStateJumpNRun");
    end
end

RagDollState["exit"] = function(gameObject)
    playerController:getPhysicsRagDollComponent():setState("Inactive");

    playerController:lockMovement("ragdoll", false);
    isInvulnerable = false;
end

---------------------------------------------------------------------------------------------------
-- Thrown away by an enemy hit (values from hitPlayer).
--
-- A state of its own, because the walking state sends its own velocity every frame - even with
-- locked input - and overwrote the knockback at once. While this state runs, the walking state
-- does not, so nothing else drives the body.
---------------------------------------------------------------------------------------------------

KnockbackState = { };

local knockbackTimer = 0;

KnockbackState["enter"] = function(gameObject)
    knockbackTimer = knockbackTime;
    animationBlender:blend5(AnimationBlender.ANIM_TAKE_DAMAGE, AnimationBlender.BLEND_WHILE_ANIMATING, 0.05, false);
    playerController:getPhysicsComponent():applyRequiredForceForVelocity(Vector3(knockbackVelocityX, knockbackVelocityUp, 0));
end

KnockbackState["execute"] = function(gameObject, dt)
    animationBlender:addTime(dt * playerController:getAnimationSpeed() / animationBlender:getLength(), "PlayerControllerJumpNRunComponent");

    -- The velocity command is latched by the physics, so it must be refreshed every frame. The
    -- horizontal part is held, the vertical part is left to gravity.
    local physicsComponent = playerController:getPhysicsComponent();
    physicsComponent:applyRequiredForceForVelocity(Vector3(knockbackVelocityX, physicsComponent:getVelocity().y, 0));

    knockbackTimer = knockbackTimer - dt;
    if (knockbackTimer <= 0) then
        playerController:requestState("WalkingStateJumpNRun");
    end
end

---------------------------------------------------------------------------------------------------

PortalState = { };

PortalState["enter"] = function(gameObject)
    stopAttack();

    playerController:lockMovement("portal", true);

    local portal = playerController:getInteractionGameObject();
    local pathFollow = portal:getAiPathFollowComponent();

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

    animationBlender:addTime(dt * playerController:getAnimationSpeed() / animationBlender:getLength(), "PlayerControllerJumpNRunComponent");
end

PortalState["exit"] = function(gameObject)
    playerController:lockMovement("portal", false);
    playerController:setInteractionGameObject(nil);
end
