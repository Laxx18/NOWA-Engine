-- A better random seed. This was taken from http://lua-users.org/wiki/MathLibraryTutorial

math.randomseed(tonumber(tostring(os.time()):reverse():sub(1,6)))

function dump(o)
	if type(o) == 'table' then
		local s = '{ '		for k,v in pairs(o) do
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
EnemyProfiles = {
    Rhino = {
        level = 1, energy = 10, strength = 20, experience = 10,
        attackImpactDelay = 0.15, attackDuration = 0.9, attackCooldown = 1.0,
        attackReach = 2.0, attackReachVertical = 1.5,
        playerKnockbackHorizontal = 6.0, playerKnockbackUp = 4.0, playerKnockbackTime = 0.3,
        deathKnockbackHorizontal = 40.0, deathKnockbackUp = 30.0, deathDeleteDelay = 2.0,
        locomotionAnimation = "ANIM_WALK_NORTH",
        energyBarOffsetY = 1.0
    },

    -- A bit stronger and faster than the rhino. Its attack is the rolling charge (Roll_InPlace, 1.33 s).
    Coyote = {
        level = 3, energy = 200, strength = 25, experience = 40,
        attackImpactDelay = 0.35, attackDuration = 1.33, attackCooldown = 1.0,
        attackReach = 1.6, attackReachVertical = 1.5,
        playerKnockbackHorizontal = 7.0, playerKnockbackUp = 4.5, playerKnockbackTime = 0.35,
        deathKnockbackHorizontal = 50.0, deathKnockbackUp = 35.0, deathDeleteDelay = 2.5,
        locomotionAnimation = "ANIM_RUN",
        energyBarOffsetY = 0.8
    }
};
