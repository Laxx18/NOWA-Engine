-- A better random seed. This was taken from http://lua-users.org/wiki/MathLibraryTutorial

math.randomseed(tonumber(tostring(os.time()):reverse():sub(1,6)))

function dump(o)
    if type(o) == 'table' then
        local s = '{ '        for k,v in pairs(o) do
            if type(k) ~= 'number' then k = '"'..k..'"' end
            s = s .. '['..k..'] = ' .. dump(v) .. ','
        end
        return s .. '} '
    else
        return tostring(o)
    end
end

-- Register your events here:
AppStateManager:getScriptEventManager():registerEvent("PlayerDeadEvent");
-- Sent by WeaponStick.lua for the killing hit (enemyId, enemyTagName, hitDirection).
AppStateManager:getScriptEventManager():registerEvent("EnemyDeadEvent");
AppStateManager:getScriptEventManager():registerEvent("PlayerAttackEvent");
-- Sent by WeaponStick.lua for every hit that does NOT kill the enemy (enemyId, hitDirection, remainingEnergy).
AppStateManager:getScriptEventManager():registerEvent("EnemyHitEvent");
-- Sent by an enemy's script every frame while the player is within its attack reach (enemyId).
AppStateManager:getScriptEventManager():registerEvent("EnemyNearPlayerEvent");
-- Sent by the boss area trigger when the player has entered the arena. The boss script starts its
-- fight on it, until then the boss only hovers and ignores the player.
AppStateManager:getScriptEventManager():registerEvent("BossFightStartEvent");
-- Sent by an enemy that makes itself defenceless on purpose (enemyId, isVulnerable). A boss does
-- that after every attack, so the player gets a window to strike: while it is set, the player's
-- script does not let that enemy start a contact attack, no matter how close the two bodies are.
AppStateManager:getScriptEventManager():registerEvent("EnemyVulnerableEvent");


CameraFx =
{
    -- ── Player hit ───────────────────────────────────────────────────────────────────
    -- impact(strength) scales shake, punch zoom and hitstop off one 0..1 number. The base is what
    -- even the lightest nip is worth, the scale is what the share of the energy bar adds on top.
    -- Base plus scale stays below 1 on purpose: a single ordinary hit should not spend the whole
    -- range, otherwise there is nothing left for a boss to feel stronger with.
    playerHitBase = 0.18,
    playerHitScale = 0.5,
 
    -- ── Landing ──────────────────────────────────────────────────────────────────────
    -- Shake at a full strength landing. The script scales this with the fall time, counted from
    -- the same 0.5 second threshold the dust particle uses.
    landShake = 0.3,
    -- Fall time ABOVE the 0.5 second threshold at which the landing shake reaches full strength.
    landFullFallTime = 1.0,
 
    -- ── Enemy kill ───────────────────────────────────────────────────────────────────
    -- Deliberately below a player hit. A kill is the player's own doing, so it wants confirmation,
    -- not punishment - shake that reads as damage on a win teaches the wrong thing.
    enemyKillImpact = 0.3,
 
    -- ── Level up ─────────────────────────────────────────────────────────────────────
    -- POSITIVE punch, so the camera breathes outwards and comes back. Slower than a hit punch,
    -- because this one is meant to be noticed rather than felt.
    levelUpPunch = 0.12,
    levelUpPunchTime = 0.45,
 
    -- ── Danger contact, spikes and lava ──────────────────────────────────────────────
    -- Small, and only every dangerShakeInterval seconds. The contact handler runs every frame, and
    -- shaking every frame pins the trauma at full strength: the decay never gets a turn and the
    -- result is a flat rumble that carries no information at all.
    dangerShake = 0.12,
    dangerShakeInterval = 0.35,
 
    -- ── Death ────────────────────────────────────────────────────────────────────────
    -- The hardest jolt in the game, plus a slow push in on the body. The zoom is what separates a
    -- death from an ordinary hit: the hit is a jolt and over, the death keeps closing in.
    deathShake = 0.8,
    deathHitstop = 0.14,
    deathZoom = 0.75,
    deathZoomTime = 1.2,
 
    -- ── Portal ───────────────────────────────────────────────────────────────────────
    -- Pulls back while the portal path plays. The player has no control during it, so a wider
    -- framing costs him nothing and shows where he is being taken.
    portalZoom = 1.25,
    portalZoomTime = 0.5
};

-- Levers and other mechanics
LEVER_ANIMATION = "ANIM_PICKUP_1";
LEVER_PULL_TIME = 1.2;

---------------------------------------------------------------------------------------------------
-- Pterodactyl, the first end boss. Everything the fight is tuned with lives here, Pterodactyl.lua
-- only reads it.
--
-- The fight is one readable loop: patrol -> attack -> hold still -> patrol. The holding still is
-- the whole point. The boss flies out of reach most of the time, so the ONLY moment the player can
-- land a hit is the window after an attack, and the player has to jump for it.
---------------------------------------------------------------------------------------------------
PTERODACTYL =
{
    -- ── Arena ────────────────────────────────────────────────────────────────────────
    -- The box the boss may fly in. Every flight target is clamped into it, so the boss can never
    -- end up inside the level geometry or outside of the camera. z is the 2.5D depth plane.
    minX = 14.5,
    maxX = 38.5,
    minY = 2.5,
    maxY = 18.0,
    z = -7.0,

    -- ── Speeds ───────────────────────────────────────────────────────────────────────
    -- Attention: both must stay below the MaxSpeed of the physics component, the moving behavior
    -- clamps the steering velocity against it.
    cruiseSpeed = 6.0,
    diveSpeed = 13.0,

    -- ── Start of the fight ───────────────────────────────────────────────────────────
    -- One roar before the first attack, so the player knows what just started.
    roarTime = 1.3,

    -- ── Patrol ───────────────────────────────────────────────────────────────────────
    -- A sweep across the whole arena, always starting away from the player, so the boss crosses the
    -- screen once before it attacks. patrolAmplitude lifts the middle waypoint, which turns the
    -- straight line into an arc.
    patrolY = 13.0,
    patrolAmplitude = 3.0,

    -- ── Dive attack ──────────────────────────────────────────────────────────────────
    -- Climbs over the player first and comes down from there, so the swoop reads as a swoop. The
    -- damage itself is the ordinary contact attack every enemy has, see the profile below.
    diveApproachHeight = 5.0,
    diveHeightOverPlayer = 0.8,

    -- ── Egg attack ───────────────────────────────────────────────────────────────────
    -- Hovers above the player and lets the SpawnComponent drop eggs for this long. With a spawn
    -- interval of 500 ms that is five eggs - enough to force the player out of the spot.
    eggHeight = 11.0,
    eggDropTime = 2.5,

    -- ── The window the player is supposed to use ─────────────────────────────────────
    -- After EVERY attack the boss drops to jump height next to the player and holds still. Beside
    -- him and not on top of him, otherwise the two bodies push each other around.
    vulnerableHeightOverPlayer = 2.2,
    vulnerableOffsetX = 2.2,
    vulnerableTime = 2.5,

    -- ── Reaction to a landed hit ─────────────────────────────────────────────────────
    -- Thrown back and stunned briefly, then it carries on with whatever it was doing. The stun time
    -- is NOT subtracted from the window above: the window is paused while the boss is thrown back,
    -- so a hit is always rewarded with a chance at the next one.
    hitKnockbackHorizontal = 9.0,
    hitKnockbackUp = 3.5,
    hitStunTime = 0.7,

    goalRadius = 1.0
};

---------------------------------------------------------------------------------------------------
-- Global game balancing. Everything static and global lives here, so it is encrypted together with
-- this file and can not be tampered with.
---------------------------------------------------------------------------------------------------

-- Player
PLAYER_NAME = "PrehistoricLax";       -- game object name of the player, the enemies look for it
PLAYER_IFRAME_TIME = 0.8;             -- invulnerability after an enemy hit, in seconds
PLAYER_IFRAME_BLINK_INTERVAL = 0.08;  -- blink rate of the player during the invulnerability
ENERGY_PICKUP_AMOUNT = 25;            -- energy an 'Energy' item restores
PLAYER_DEATH_KNOCKBACK_HORIZONTAL = 4.0; -- the dead player is thrown backwards with this velocity once ...
PLAYER_DEATH_KNOCKBACK_UP = 3.0;         -- ... and this upward velocity, then lies still
PLAYER_RAGDOLL_TIME = 5.0;            -- seconds in the ragdoll until the energy is refilled

-- Status bar: damage trail of the energy bar (EnergyTrailProgress under EnergyProgress).
HUD_ENERGY_TRAIL_DELAY = 0.3;         -- seconds the trail holds the old value after a hit
HUD_ENERGY_TRAIL_SPEED = 60;          -- then it shrinks with this many percent per second

-- Player progression. The attributes live on the main game object: Experience counts the progress
-- within the current level, Ascension is the experience needed for the next level.
--
-- Needed experience: ASCENSION_BASE * level ^ ASCENSION_EXPONENT (level 1: 20, 2: 61, 3: 116,
-- 5: 263, 10: 796). Polynomial instead of exponential (Diablo): with few enemies an exponential
-- curve becomes a wall after a handful of levels.
--
-- Replaying a stage is allowed, but outleveled enemies give less experience (like Diablo 2): up to
-- EXPERIENCE_PENALTY_GRACE levels above the enemy there is no penalty, every further level costs
-- EXPERIENCE_PENALTY_PER_LEVEL, but never below EXPERIENCE_MIN_FACTOR. Grinding the first stage
-- therefore stalls after a few levels, the next stage has to be played.
PLAYER_BASE_ENERGY = 100;             -- max energy at level 1
ENERGY_PER_LEVEL = 10;                -- max energy gained per level up
STRENGTH_PER_LEVEL = 2;               -- strength gained per level up (level 1 starts with the attribute's value)
ASCENSION_BASE = 20;
ASCENSION_EXPONENT = 1.6;
EXPERIENCE_PENALTY_GRACE = 1;
EXPERIENCE_PENALTY_PER_LEVEL = 0.2;
EXPERIENCE_MIN_FACTOR = 0.1;

-- Experience needed to get from 'level' to the next one.
function getRequiredExperience(level)
    return math.floor(ASCENSION_BASE * level ^ ASCENSION_EXPONENT + 0.5);
end

-- Max energy of the player at 'level'.
function getMaxEnergy(level)
    return PLAYER_BASE_ENERGY + (level - 1) * ENERGY_PER_LEVEL;
end

-- Experience the player at 'playerLevel' gets for killing an enemy with 'profile'. At least 1.
function getExperienceForKill(profile, playerLevel)
    local factor = 1 - EXPERIENCE_PENALTY_PER_LEVEL * (playerLevel - profile.level - EXPERIENCE_PENALTY_GRACE);
    if (factor > 1) then
        factor = 1;
    elseif (factor < EXPERIENCE_MIN_FACTOR) then
        factor = EXPERIENCE_MIN_FACTOR;
    end

    local experience = math.floor(profile.experience * factor + 0.5);
    if (experience < 1) then
        experience = 1;
    end
    return experience;
end

-- Weapons
CUDGEL_DAMAGE_FACTOR = 1.0;           -- cudgel damage per hit = player's Strength * this factor

-- Enemies
ENEMY_ENERGY_BAR_TIME = 1.0;          -- how long an enemy's energy bar stays visible after a hit

-- Look of the enemy energy bars (ValueBarComponent), the same for every enemy. The height above the
-- enemy is 'energyBarOffsetY' in its profile.
ENEMY_ENERGY_BAR_WIDTH = 1.0;
ENEMY_ENERGY_BAR_HEIGHT = 0.1;
ENEMY_ENERGY_BAR_BORDER = 0.025;
ENEMY_ENERGY_BAR_FILL_COLOR = Vector3(0.85, 0.10, 0.06);
ENEMY_ENERGY_BAR_FRAME_COLOR = Vector3(0.05, 0.04, 0.03);

---------------------------------------------------------------------------------------------------
-- Save game and player progress
--
-- Two storages are involved and they have different life times:
--
--   The AttributesComponent on the main game object belongs to the SCENE. It holds the start values from
--   the editor and is the working copy the HUD reads from. A scene change rebuilds it, so without the
--   helpers below everything would be back to the editor values after every level.
--
--   The global values of the GameProgressModule live in the AppStateManager. They survive every scene
--   change, and saveProgress writes them into the save game.
--
-- So the global values are the truth, the component is the working copy: connect pulls, disconnect pushes.
-- Between two levels NO file is involved, only at a save point.
---------------------------------------------------------------------------------------------------

SAVE_GAME_NAME = "save1";
START_SCENE_NAME = "Level1";

-- Player attributes that survive a scene change and end up in the save game. They must exist with these
-- names in the AttributesComponent of the main game object.
PLAYER_ATTRIBUTE_NAMES = { "Energy", "Strength", "Experience", "Ascension", "Level", "Coins", "KilledEnemies" };

-- Abilities that are unlocked during the game, see isWorldFlagSet.
ABILITY_CAN_SLIDE = "Ability_CanSlide";

-- Called in connect. A global value that is already there wins - that is either the running game or a save
-- game that has just been loaded. Is there none, this is the first scene of a new run and the editor values
-- of the component become the globals.
function pullPlayerAttributesFromProgress(attributesComponent)
    local gameProgressModule = AppStateManager:getGameProgressModule();

    for i = 1, #PLAYER_ATTRIBUTE_NAMES do
        local attributeName = PLAYER_ATTRIBUTE_NAMES[i];
        local attributeValue = attributesComponent:getAttributeValueByName(attributeName);
        local globalValue = gameProgressModule:getGlobalValue(attributeName);

        if (globalValue ~= nil) then
            attributeValue:setValueNumber(globalValue:getValueNumber());
        else
            gameProgressModule:setGlobalNumberValue(attributeName, attributeValue:getValueNumber());
        end
    end
end

-- Called in disconnect, so the next scene can pull the current values, and before saving.
function pushPlayerAttributesToProgress(attributesComponent)
    local gameProgressModule = AppStateManager:getGameProgressModule();

    for i = 1, #PLAYER_ATTRIBUTE_NAMES do
        local attributeName = PLAYER_ATTRIBUTE_NAMES[i];
        gameProgressModule:setGlobalNumberValue(attributeName, attributesComponent:getAttributeValueByName(attributeName):getValueNumber());
    end
end

-- World state: collected special items, defeated bosses, triggered mechanisms, unlocked abilities. All of
-- them are global values and therefore part of the save game.
--
-- Attention: the key has to stay the same across sessions. The game object NAME is unique within its scene,
-- its id is not - that one is handed out while loading.
-- Builds a key that is unique across the whole game and stays the same across sessions: the scene
-- name plus the game object name. The object NAME is unique within its scene, its id is not - that
-- one is handed out while loading.
function makeWorldFlagKey(gameObject)
    return AppStateManager:getGameProgressModule():getCurrentSceneName() .. "_" .. gameObject:getName();
end

function isWorldFlagSet(key)
    local globalValue = AppStateManager:getGameProgressModule():getGlobalValue(key);
    if (globalValue == nil) then
        return false;
    end
    return globalValue:getValueNumber() > 0;
end

function setWorldFlag(key)
    -- A number and not a bool on purpose: setGlobalNumberValue is the one setter that is bound for both.
    AppStateManager:getGameProgressModule():setGlobalNumberValue(key, 1);
end

-- Levers. One flag per lever, so a pulled lever stays pulled for the whole run and is cleared again
-- by startNewRun() after a death. The 'Lever_' prefix only namespaces the key, so it stays readable
-- in the save game: "Lever_Level5_Lever1_0".
function makeLeverFlagKey(leverGameObject)
    return "Lever_" .. makeWorldFlagKey(leverGameObject);
end

function isLeverPulled(leverGameObject)
    return isWorldFlagSet(makeLeverFlagKey(leverGameObject));
end

function setLeverPulled(leverGameObject)
    setWorldFlag(makeLeverFlagKey(leverGameObject));
end

-- Called by the save point. The scene snapshot is written as well, so loading restores the exact world with
-- one single call and no scene name has to be remembered anywhere.
function saveGame(attributesComponent)
    pushPlayerAttributesToProgress(attributesComponent);
    AppStateManager:getGameProgressModule():saveProgress(SAVE_GAME_NAME, true, true);
end

function hasSaveGame()
    return AppStateManager:getGameProgressModule():hasSaveGame(SAVE_GAME_NAME);
end

function loadGame()
    AppStateManager:getGameProgressModule():loadProgress(SAVE_GAME_NAME, true, false);
end

-- Death without a save point. Nothing of the failed run may survive: attributes, collected items, defeated
-- bosses, unlocked abilities.
function startNewRun()
    AppStateManager:getGameProgressModule():clearGlobalValues();
    AppStateManager:getGameProgressModule():changeScene(START_SCENE_NAME);
end

-- Damage reaction of an enemy. Called from the enemy's onEnemyHit, returns the time the animation runs.
--
-- Attention: the clip is blended LOOPING, exactly like the attack and for the same reason. A non looping clip
-- blocks every other blend until it has played to the end, and the enemy's MovingBehavior slows the clip down
-- while the enemy stands still - the enemy would get stuck in its damage pose. The returned timer ends it.
function startEnemyDamageAnimation(animationBlender, profile)
    animationBlender:blend5(AnimationBlender.ANIM_TAKE_DAMAGE, AnimationBlender.BLEND_WHILE_ANIMATING, 0.1, true);
    return profile.damageDuration;
end

-- Back to idle once the damage animation has run its time. Called from the enemy's update.
function endEnemyDamageAnimation(animationBlender)
    animationBlender:blend5(AnimationBlender.ANIM_IDLE_1, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);
end

-- Sets up an enemy's energy bar at connect: look, full energy, hidden until the first hit.
function setupEnemyEnergyBar(energyBar, profile)
    energyBar:setWidth(ENEMY_ENERGY_BAR_WIDTH);
    energyBar:setHeight(ENEMY_ENERGY_BAR_HEIGHT);
    energyBar:setBorderSize(ENEMY_ENERGY_BAR_BORDER);
    energyBar:setInnerColor(ENEMY_ENERGY_BAR_FILL_COLOR);
    energyBar:setOuterColor(ENEMY_ENERGY_BAR_FRAME_COLOR);
    energyBar:setOffsetPosition(Vector3(0, profile.energyBarOffsetY, 0));
    energyBar:setMaxValue(profile.energy);
    energyBar:setCurrentValue(profile.energy);
    energyBar:setActivated(false);
end

-- Enemies, keyed by the enemy's TagName. Static data only - the runtime energy of each enemy is
-- tracked by WeaponStick.lua.
--
--   level                      enemy level, used for the experience penalty
--   energy                     energy at spawn
--   strength                   energy the PLAYER loses per landed attack
--   experience                 experience the player gets for the kill (before the penalty)
--   attackImpactDelay          seconds from the attack start to the moment it hurts (0 = on contact)
--   attackDuration             length of the enemy's ANIM_ATTACK_1 clip, then back to locomotion
--   damageDuration             length of the enemy's ANIM_TAKE_DAMAGE clip, then back to idle
--   attackCooldown             minimum time between two attacks, measured from the attack start
--   attackReach                horizontal distance at which the attack still lands
--   attackReachVertical        vertical distance at which the attack still lands
--   playerKnockbackHorizontal  sideways velocity the player is thrown with
--   playerKnockbackUp          upward velocity the player is thrown with
--   playerKnockbackTime        how long the player is thrown away (KnockbackState)
--   deathKnockbackHorizontal   sideways velocity of the ragdoll when killed
--   deathKnockbackUp           upward velocity of the ragdoll when killed
--   deathDeleteDelay           seconds until the corpse is deleted
--   locomotionAnimation        AnimationBlender constant the enemy goes back to after an attack
--   energyBarOffsetY           height of the energy bar above the enemy's origin
--   isProjectile               optional. A thrown thing, not a creature: it has no attack animation
--                              and no cooldown, it simply costs the player 'strength' on contact.
EnemyProfiles = 
{
    Rhino = 
    {
        level = 1, energy = 10, strength = 20, experience = 10,
        attackImpactDelay = 0.25, attackDuration = 0.9, attackCooldown = 1.0, damageDuration = 0.6,
        attackReach = 1.6, attackReachVertical = 1,
        playerKnockbackHorizontal = 6.0, playerKnockbackUp = 4.0, playerKnockbackTime = 0.3,
        deathKnockbackHorizontal = 40.0, deathKnockbackUp = 30.0, deathDeleteDelay = 2.0,
        locomotionAnimation = "ANIM_WALK_NORTH",
        energyBarOffsetY = 2.0
    },
    
    -- Same as rhino, but more energy.
    Elephant = 
    {
        level = 1, energy = 30, strength = 10, experience = 15,
        attackImpactDelay = 0.45, attackDuration = 0.9, attackCooldown = 0.8, damageDuration = 0.6,
        attackReach = 1.2, attackReachVertical = 1,
        playerKnockbackHorizontal = 8.0, playerKnockbackUp = 6.0, playerKnockbackTime = 0.3,
        deathKnockbackHorizontal = 50.0, deathKnockbackUp = 40.0, deathDeleteDelay = 2.0,
        locomotionAnimation = "ANIM_RUN",
        energyBarOffsetY = 2.0
    },

    -- A bit stronger and faster than the rhino. Its attack is the rolling charge (Roll_InPlace, 1.33 s).
    Coyote = 
    {
        level = 3, energy = 50, strength = 25, experience = 20,
        attackImpactDelay = 0.35, attackDuration = 1.33, attackCooldown = 1.0, damageDuration = 0.6,
        attackReach = 1.6, attackReachVertical = 1,
        playerKnockbackHorizontal = 7.0, playerKnockbackUp = 4.5, playerKnockbackTime = 0.35,
        deathKnockbackHorizontal = 50.0, deathKnockbackUp = 35.0, deathDeleteDelay = 2.5,
        locomotionAnimation = "ANIM_RUN",
        energyBarOffsetY = 2
    },
    -- Slow but totally strong and much energy
    Beaver = 
    {
        level = 3, energy = 200, strength = 80, experience = 40,
        attackImpactDelay = 0.35, attackDuration = 1.33, attackCooldown = 1.0, damageDuration = 0.8,
        attackReach = 1, attackReachVertical = 1,
        playerKnockbackHorizontal = 7.0, playerKnockbackUp = 4.5, playerKnockbackTime = 0.35,
        deathKnockbackHorizontal = 50.0, deathKnockbackUp = 35.0, deathDeleteDelay = 2.5,
        locomotionAnimation = "ANIM_IDLE_1",
        energyBarOffsetY = 2
    },

    -- First end boss. Much energy and a lot of experience, but not a damage monster: the challenge
    -- is reaching it at all, not surviving it. Its values are tuned around the window it opens after
    -- every attack (see PTERODACTYL above) - roughly seven landed hits.
    --
    -- attackDuration is the length of Air_Attack_Claw (1.33 s), damageDuration the one of
    -- Air_Damage_Light (1.33 s). attackReachVertical is generous because the boss attacks from the
    -- air and the player is below it.
    Pterodactyl =
    {
        level = 5, energy = 180, strength = 25, experience = 300,
        attackImpactDelay = 0.3, attackDuration = 1.33, attackCooldown = 1.5, damageDuration = 1.33,
        attackReach = 1.8, attackReachVertical = 1.8,
        playerKnockbackHorizontal = 9.0, playerKnockbackUp = 5.0, playerKnockbackTime = 0.35,
        deathKnockbackHorizontal = 25.0, deathKnockbackUp = 10.0, deathDeleteDelay = 4.0,
        locomotionAnimation = "ANIM_WALK_NORTH",
        energyBarOffsetY = 2.0
    },

    -- The egg the pterodactyl drops. A projectile, see isProjectile: it hurts on contact and that is
    -- all it does.
    --
    -- Attention: the energy is deliberately out of reach. An egg is category 'Enemy', so the cudgel
    -- can hit it - it should spark and make a noise, but it must never die, otherwise an
    -- EnemyDeadEvent would count it as a kill and hand out experience for it.
    Egg =
    {
        isProjectile = true,
        level = 5, energy = 100000, strength = 20, experience = 1,
        attackImpactDelay = 0, attackDuration = 0, attackCooldown = 0, damageDuration = 0,
        attackReach = 0.5, attackReachVertical = 0.5,
        playerKnockbackHorizontal = 4.0, playerKnockbackUp = 2.5, playerKnockbackTime = 0.2,
        deathKnockbackHorizontal = 0, deathKnockbackUp = 0, deathDeleteDelay = 0,
        locomotionAnimation = "ANIM_IDLE_1",
        energyBarOffsetY = 0
    }
};