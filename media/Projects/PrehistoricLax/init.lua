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
    minX = 15,
    maxX = 38,
    minY = 2.5,
    maxY = 8.0,
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
    diveArcRadius = 7.0,   -- horizontal distance from the player at which the dive starts
    diveArcPoints = 12,    -- waypoints of the arc, more = rounder

    -- ── Egg attack ───────────────────────────────────────────────────────────────────
    -- Hovers above the player and lets the SpawnComponent drop eggs for this long. With a spawn
    -- interval of 1000 ms that is five eggs - enough to force the player out of the spot.
    eggHeight = 12,
    eggDropTime = 10,
    -- Egg attack: half width of the sweep around the player, and its speed. Twice the half
    -- width must fit into the arena (maxX - minX).
    eggSweepHalfWidth = 9.0,
    eggSweepSpeed = 6.0,

    -- ── The window the player is supposed to use ─────────────────────────────────────
    -- After EVERY attack the boss drops to jump height next to the player and holds still. Beside
    -- him and not on top of him, otherwise the two bodies push each other around.
    vulnerableHeightOverPlayer = 3.0,
    vulnerableOffsetX = 1.2,
    vulnerableTime = 2.5,

    -- ── Reaction to a landed hit ─────────────────────────────────────────────────────
    -- Thrown back and stunned briefly, then it carries on with whatever it was doing. The stun time
    -- is NOT subtracted from the window above: the window is paused while the boss is thrown back,
    -- so a hit is always rewarded with a chance at the next one.
    hitKnockbackHorizontal = 9.0,
    hitKnockbackUp = 3.5,
    hitStunTime = 0.7,

    -- ── Facing ───────────────────────────────────────────────────────────────────────
    -- facingRunUp: before the boss arrives somewhere (dive start, window) it first flies a short
    -- horizontal leg towards the player, so the auto orientation has time to turn it his way.
    -- rotationSpeed is the turn rate of the MovingBehavior, see setRotationSpeed.
    facingRunUp = 4.0,
    rotationSpeed = 10.0,

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
-- So the global values are the truth, the component is the working copy: MainGameObject.lua pulls in
-- connect and pushes in disconnect. Between two levels NO file is involved, only at a save point.
--
-- The save game holds ONLY the global values (attributes, world flags, the index of the used save
-- point), no scene snapshot. The world is rebuilt from the authored scene on every load, with the world
-- flags (taken items, defeated bosses, pulled levers) applied on top, see applyWorldState.
--
-- Everything another script has to call must live HERE: the functions of a script are only visible
-- inside that script, init.lua is the one place all of them share.
---------------------------------------------------------------------------------------------------

-- State that scripts have to share at runtime. A table, because a script can not write to a global of
-- another one, but all of them can change the fields of the same table.
Session =
{
    -- true from the moment the dead player starts the reload until the next scene is connected. While it
    -- is set, the dead values must not be pushed to the global values, see MainGameObject.lua.
    isRespawning = false
};

SAVE_GAME_NAME = "save1";
START_SCENE_NAME = "Level1";

-- Every save point of the game, in any order. The save game stores only the INDEX of the one that was
-- used, because a number can be read back everywhere.
--   name:  the NAME of the save point game object in the editor. It is at the same time the target location
--          name the player is brought to, exactly like TargetLocationName of an ExitComponent, so a game
--          object with this name must exist in that scene.
--   scene: the scene the save point is in.
SAVEPOINTS =
{
    { name = "Savepoint_Level5", scene = "Level5" },
    { name = "Savepoint_Level3", scene = "Level3" }
};

-- true = additionally the whole scene as a snapshot. Only for one huge world where a lot changes, it
-- makes the save game big and freezes the layout of the level at the time of saving.
SAVE_SCENE_SNAPSHOT = false;

-- Three save games per player in a ring: the fourth save removes the oldest one. Every save game has the
-- player name plus date and time in its name, and a screenshot of exactly the same name:
-- "Player_2026-10-09_16-33-31" and "Player_2026-10-09_16-33-31.png".
SAVE_SLOT_COUNT = 3;

-- ATTENTION: the ending saveProgress gives the save game file. It is needed to delete the oldest one.
SAVE_FILE_ENDING = ".sav";
SCREENSHOT_ENDING = ".png";

-- Which save games exist is written into a small text file in the same folder, one name per line, the
-- oldest first. A directory can not be listed from Lua.
SAVE_INDEX_ENDING = ".txt";

-- Set as soon as the save game has been looked at in this run: either loaded or a new run. It keeps the
-- connect of every following scene from loading again.
GAME_LOADED_FLAG = "GameLoaded";

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

-- Switch for TESTING. The world state (pulled levers, taken items, defeated bosses) is applied and the
-- save game is loaded at the start only in the real game (Core:isGame()), never while testing in the
-- editor - there the world is always exactly as authored: levers can be pulled again, the boss is
-- there every time. Set WORLD_STATE_ACTIVE to false to get the same in the real game.
WORLD_STATE_ACTIVE = true;

function isWorldStateActive()
    return true == WORLD_STATE_ACTIVE and true == Core:isGame();
end

-- Levers. One flag per lever, so a pulled lever stays pulled for the whole run and is cleared again
-- by startNewRun() after a death. The 'Lever_' prefix only namespaces the key, so it stays readable
-- in the save game: "Lever_Level5_Lever1_0".
function makeLeverFlagKey(leverGameObject)
    return "Lever_" .. makeWorldFlagKey(leverGameObject);
end

function isLeverPulled(leverGameObject)
    return true == isWorldStateActive() and true == isWorldFlagSet(makeLeverFlagKey(leverGameObject));
end

function setLeverPulled(leverGameObject)
    setWorldFlag(makeLeverFlagKey(leverGameObject));
end

-- Bosses are unique in the whole game, so there is no scene in the key: "Boss_Pterodactyl".
function isBossDefeated(bossName)
    return true == isWorldStateActive() and true == isWorldFlagSet("Boss_" .. bossName);
end

function setBossDefeated(bossName)
    setWorldFlag("Boss_" .. bossName);
end

-- Pickups that come back every time the level is loaded (TagName). Everything else the player takes is
-- gone for good.
RESPAWNING_PICKUP_TAGS = { Coin = true, Energy = true };

-- Items that are in the scene from the start, but only exist after a boss was defeated.
-- Key: game object NAME of the item, value: the boss.
ITEM_UNLOCKS = { ShoesSpeed = "Pterodactyl" };

-- Abilities an item grants when it is taken. Key: game object NAME of the item.
ABILITY_SPEED_SHOES = "Ability_SpeedShoes";
SPEED_SHOES_FACTOR = 1.3;
ITEM_ABILITIES = { ShoesSpeed = ABILITY_SPEED_SHOES };

function makeItemFlagKey(itemGameObject)
    return "Item_" .. makeWorldFlagKey(itemGameObject);
end

function isItemTaken(itemGameObject)
    return true == isWorldStateActive() and true == isWorldFlagSet(makeItemFlagKey(itemGameObject));
end

-- Remembers a taken item and switches on the ability it grants.
function setItemTaken(itemGameObject)
    local ability = ITEM_ABILITIES[itemGameObject:getName()];
    if (nil ~= ability) then
        setWorldFlag(ability);
    end

    if (true ~= RESPAWNING_PICKUP_TAGS[itemGameObject:getTagName()]) then
        setWorldFlag(makeItemFlagKey(itemGameObject));
    end
end

function isItemLocked(itemGameObject)
    local bossName = ITEM_UNLOCKS[itemGameObject:getName()];
    return nil ~= bossName and false == isBossDefeated(bossName);
end

-- Everything the player has unlocked in the course of the game, read from the world flags. Called in
-- connect, and again right after an item that grants an ability has been taken.
function applyAbilities(prehistoricLax)
    playerController = prehistoricLax:getPlayerControllerJumpNRunComponent();
    playerController:setCanSlide(isWorldFlagSet(ABILITY_CAN_SLIDE));
    applySpeedShoes(prehistoricLax);
end

-- The base speeds are remembered once in the global values, so a loaded snapshot with already
-- boosted values can never boost twice. startNewRun clears them again.
function applySpeedShoes(prehistoricLax)
    local physics = prehistoricLax:getPhysicsActiveComponent();
	playerController = prehistoricLax:getPlayerControllerJumpNRunComponent();
    playerController:setUseAcceleration(true);
    local gameProgressModule = AppStateManager:getGameProgressModule();

    if (nil == gameProgressModule:getGlobalValue("BaseSpeed")) then
       gameProgressModule:setGlobalNumberValue("BaseSpeed", physics:getSpeed());
       gameProgressModule:setGlobalNumberValue("BaseMaxSpeed", physics:getMaxSpeed());
    end

    local factor = 1.0;
    if (true == isWorldFlagSet(ABILITY_SPEED_SHOES)) then
        factor = SPEED_SHOES_FACTOR;
    end

    physics:setSpeed(gameProgressModule:getGlobalValue("BaseSpeed"):getValueNumber() * factor);
    physics:setMaxSpeed(gameProgressModule:getGlobalValue("BaseMaxSpeed"):getValueNumber() * factor);
end

-- Called in connect of MainGameObject.lua: removes what was taken
function applyWorldState()
    local objects = AppStateManager:getGameObjectController():getGameObjectsFromCategory("PointOfInterest");

    -- Attention: the table from C++ is filled starting at index 0, so '#' and a numeric loop are both
    -- wrong here. pairs() does not care about the base.
    for key, object in pairs(objects) do
        local item = AppStateManager:getGameObjectController():castGameObject(object);

        if (true == isItemTaken(item)) then
            AppStateManager:getGameObjectController():deleteGameObject(item:getId());
        -- Attention: Is not used, point of interest items like speed shoes are visible always, but there is a gate, which does not open, until a boss is defeated
        --elseif (true == isItemLocked(item)) then
        --    item:setVisible(false);
        end
    end
end

-- Called by a boss when it dies: the items that waited for it appear.
-- Attention: Is not used, point of interest items like speed shoes are visible always, but there is a gate, which does not open, until a boss is defeated
function revealUnlockedItems()
    local objects = AppStateManager:getGameObjectController():getGameObjectsFromCategory("PointOfInterest");

    for key, object in pairs(objects) do
        local item = AppStateManager:getGameObjectController():castGameObject(object);

        if (nil ~= ITEM_UNLOCKS[item:getName()] and false == isItemLocked(item)) then
            item:setVisible(true);
        end
    end
end

-- Re-applies every lever that has already been pulled in this run. After a scene change the gate is
-- back at its authored position, but the lever counts as pulled - so it is opened again here, without
-- animation and without locking the player.
function applyPulledLevers()
    local mechanics = AppStateManager:getGameObjectController():getGameObjectsFromCategory("Mechanics");

    for key, mechanicGameObject in pairs(mechanics) do
        -- The table holds the raw pointer. Without the cast lua only sees part of the class, which is
        -- why getTagName() worked and getName() came back nil.
        local leverGameObject = AppStateManager:getGameObjectController():castGameObject(mechanicGameObject);

        if (leverGameObject:getTagName() == "Lever" and true == isLeverPulled(leverGameObject)) then
            leverGameObject:getJointHingeActuatorComponent():setActivated(true);
            AppStateManager:getGameObjectController():activateGameObjectComponentsFromReferenceId(leverGameObject:getReferenceId(), true);
        end
    end
end

function findSavepointIndex(savepointName)
    for i = 1, #SAVEPOINTS do
        if (SAVEPOINTS[i].name == savepointName) then
            return i;
        end
    end
    return nil;
end

-- The full path of a file of the save game system, in the folder of the save games. Needs
-- Core:getSaveFilePathName bound to Lua.
function getSavePath(name, fileEnding)
    return Core:getSaveFilePathName(name, fileEnding);
end

---------------------------------------------------------------------------------------------------
-- Players and the list of their save games
--
-- The menu lets the player type in a name. Everything that belongs to that name - the save games, the
-- screenshots, later the highscore - carries it in its file name, so deleting a player deletes all of it.
---------------------------------------------------------------------------------------------------

DEFAULT_PLAYER_NAME = "Player";
PLAYER_NAME_MAX_LENGTH = 16;

-- The list of all players, one name per line, in the folder of the save games.
PLAYERS_FILE_NAME = "players";

-- The name of the player lives in the global values: they survive every scene change of the game and are part
-- of the save game, so a loaded game knows whose it is. Only for the game, the menu has its own module.
function getPlayerName()
    local value = AppStateManager:getGameProgressModule():getGlobalValue("PlayerName");
    if (nil == value) then
        return DEFAULT_PLAYER_NAME;
    end
    return value:getValueString();
end

function setPlayerName(playerName)
    AppStateManager:getGameProgressModule():setGlobalStringValue("PlayerName", playerName);
end

-- The name becomes part of file names, and the underscore separates it from the date in the name of a save
-- game. So only letters, digits and '-' stay, spaces are removed. Returns an empty string if nothing is left.
function cleanPlayerName(text)
    local name = tostring(text or "");
    name = string.gsub(name, "%s+", "");
    name = string.gsub(name, "[^%w%-]", "");
    return string.sub(name, 1, PLAYER_NAME_MAX_LENGTH);
end

local function readLines(path)
    local lines = {};

    local file = io.open(path, "r");
    if (nil == file) then
        return lines;
    end

    for line in file:lines() do
        line = string.gsub(line, "\r", "");
        if (line ~= "") then
            lines[#lines + 1] = line;
        end
    end
    file:close();

    return lines;
end

local function writeLines(path, lines)
    local file = io.open(path, "w");
    if (nil == file) then
        log("[init] Could not write '" .. path .. "'.");
        return;
    end

    for i = 1, #lines do
        file:write(lines[i] .. "\n");
    end
    file:close();
end

function makeSaveName()
    return getPlayerName() .. "_" .. os.date("%Y-%m-%d_%H-%M-%S");
end

local function getSaveIndexPath(playerName)
    return getSavePath(playerName .. "_saves", SAVE_INDEX_ENDING);
end

-- The names of the save games of a player, the oldest first. Without a name: the current player.
function readSaveIndex(playerName)
    return readLines(getSaveIndexPath(playerName or getPlayerName()));
end

function writeSaveIndex(names, playerName)
    writeLines(getSaveIndexPath(playerName or getPlayerName()), names);
end

-- The newest save game that really exists. Someone may have deleted one by hand.
function getLatestSaveName(playerName)
    local names = readSaveIndex(playerName);

    for i = #names, 1, -1 do
        if (true == AppStateManager:getGameProgressModule():hasSaveGame(names[i])) then
            return names[i];
        end
    end
    return nil;
end

function hasSaveGame(playerName)
    return nil ~= getLatestSaveName(playerName);
end

function readPlayers()
    return readLines(getSavePath(PLAYERS_FILE_NAME, SAVE_INDEX_ENDING));
end

function writePlayers(names)
    writeLines(getSavePath(PLAYERS_FILE_NAME, SAVE_INDEX_ENDING), names);
end

-- The stored spelling of a player, or nil. Case does not matter: the file system does not care either, so
-- "max" and "Max" would be the same files.
function findPlayer(playerName)
    local wanted = string.lower(playerName);
    local players = readPlayers();

    for i = 1, #players do
        if (string.lower(players[i]) == wanted) then
            return players[i];
        end
    end
    return nil;
end

-- Adds a new player. Returns the name as it is stored, which is the old spelling if he exists already.
function addPlayer(playerName)
    local existing = findPlayer(playerName);
    if (nil ~= existing) then
        return existing;
    end

    local players = readPlayers();
    players[#players + 1] = playerName;
    writePlayers(players);

    return playerName;
end

-- Deletes the player with ALL of his save games and screenshots. A highscore entry is to be removed here too,
-- as soon as there is one.
function deletePlayer(playerName)
    local saves = readSaveIndex(playerName);
    for i = 1, #saves do
        os.remove(getSavePath(saves[i], SAVE_FILE_ENDING));
        os.remove(getSavePath(saves[i], SCREENSHOT_ENDING));
    end
    os.remove(getSaveIndexPath(playerName));

    local wanted = string.lower(playerName);
    local remaining = {};
    local players = readPlayers();
    for i = 1, #players do
        if (string.lower(players[i]) ~= wanted) then
            remaining[#remaining + 1] = players[i];
        end
    end
    writePlayers(remaining);
end

-- What the menu hands over to the game, in the current save game name of the Core:
--   "Max"                         continue: the newest save game of Max, or a new run if he has none
--   "Max_NEW"                     new game of Max, whatever he has saved
--   "Max_2026-10-09_16-33-31"     exactly this save game of Max
-- Returns the player name, the save game name (or nil) and whether it is a new run.
NEW_RUN_SUFFIX = "_NEW";

function splitStartName(startName)
    if (string.sub(startName, -#NEW_RUN_SUFFIX) == NEW_RUN_SUFFIX) then
        return string.sub(startName, 1, #startName - #NEW_RUN_SUFFIX), nil, true;
    end

    local playerName = string.match(startName, "^(.+)_%d%d%d%d%-%d%d%-%d%d_%d%d%-%d%d%-%d%d$");
    if (nil ~= playerName) then
        return playerName, startName, false;
    end

    return startName, nil, false;
end

-- Called by the save point.
function saveGame(attributesComponent, savepointName)
    pushPlayerAttributesToProgress(attributesComponent);

    local savepointIndex = findSavepointIndex(savepointName);
    if (nil == savepointIndex) then
        log("[init] Save point '" .. toString(savepointName) .. "' is not in SAVEPOINTS - the save game will not know where to come back.");
    else
        AppStateManager:getGameProgressModule():setGlobalNumberValue("SavepointIndex", savepointIndex);
    end

    local saveName = makeSaveName();
    AppStateManager:getGameProgressModule():saveProgress(saveName, true, SAVE_SCENE_SNAPSHOT);

    -- Screenshot and ring in a pcall: if the paths are not available in Lua yet, the save game itself
    -- must still be there.
    local success, errorMessage = pcall(function()
        Core:createScreenshot(getSavePath(saveName, SCREENSHOT_ENDING));

        local names = readSaveIndex();
        names[#names + 1] = saveName;

        while (#names > SAVE_SLOT_COUNT) do
            local oldest = table.remove(names, 1);
            os.remove(getSavePath(oldest, SAVE_FILE_ENDING));
            os.remove(getSavePath(oldest, SCREENSHOT_ENDING));
end

        writeSaveIndex(names);
    end);

    if (false == success) then
        log("[init] Screenshot or save ring failed: " .. toString(errorMessage));
    end
end

-- Back to the last save point: the values come from the file, the scene from the SAVEPOINTS table.
function loadGame(requestedSaveName)
    local gameProgressModule = AppStateManager:getGameProgressModule();
    local saveName = requestedSaveName or getLatestSaveName(getPlayerName());

    if (nil == saveName) then
        startNewRun();
        do return end;
    end

    -- The values of the file replace the globals. The scene that is left must not push its own values over
    -- them in disconnect.
    Session.isRespawning = true;

    if (true == SAVE_SCENE_SNAPSHOT) then
        -- The snapshot brings the scene and the player position with it.
        gameProgressModule:loadProgress(saveName, true, false);
        setWorldFlag(GAME_LOADED_FLAG);
        do return end;
    end

    gameProgressModule:loadProgress(saveName, false, false);
    setWorldFlag(GAME_LOADED_FLAG);

    local savepointIndex = gameProgressModule:getGlobalValue("SavepointIndex");
    local savepoint = nil;
    if (nil ~= savepointIndex) then
        savepoint = SAVEPOINTS[savepointIndex:getValueNumber()];
    end

    if (nil == savepoint) then
        log("[init] The save game has no valid save point - starting over.");
        startNewRun();
        do return end;
    end

    gameProgressModule:setRequestedTargetLocationName(savepoint.name);
    gameProgressModule:changeScene(savepoint.scene);
end

-- Called in connect of MainGameObject.lua. The menu has put into the Core what is to be started, see
-- splitStartName. Loads the save game, once per run. Returns true if the load was started: the scene is about
-- to be replaced and nothing else may happen in that connect.
function loadGameOnStart()
    if (false == isWorldStateActive()) then
        return false;
    end

    if (true == isWorldFlagSet(GAME_LOADED_FLAG)) then
        return false;
    end

    -- Set right away: with or without a save game, the connect of every following scene must not try again.
    setWorldFlag(GAME_LOADED_FLAG);

    local startName = Core:getCurrentSaveGameName();
    if (nil == startName or "" == startName) then
        -- Started without the menu, e.g. straight from the editor into the game.
        return false;
    end

    local playerName, saveName, isNewRun = splitStartName(startName);
    setPlayerName(playerName);

    if (true == isNewRun) then
        return false;
    end

    if (nil == saveName) then
        saveName = getLatestSaveName(playerName);
    end

    if (nil == saveName) then
        -- A new player.
        return false;
    end

    loadGame(saveName);
    return true;
end

-- Death without a save point, or a new game. Nothing of the failed run may survive: attributes,
-- collected items, defeated bosses, unlocked abilities.
function startNewRun()
    Session.isRespawning = true;

    -- The player stays the same, only his progress is gone.
    local playerName = getPlayerName();
    AppStateManager:getGameProgressModule():clearGlobalValues();
    setPlayerName(playerName);

    -- A new run must not load the old save game again in connect.
    setWorldFlag(GAME_LOADED_FLAG);

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
        level = 5, energy = 100, strength = 20, experience = 300,
        hurtsWhileVulnerable = false,   -- if true, then hurts on contact even in the window; false = the window is safe. 
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


---------------------------------------------------------------------------------------------------
-- Click2Point
--
-- The four verb buttons of the status bar (Take, Use, Pull, Talk) always act on the point of
-- interest the player is standing in front of. What that is, is decided purely by the TAG NAME of
-- the game object, so a new object needs no new code here - only an entry in the table below.
--
-- The buttons do NOT blink at a point of interest: the player is supposed to puzzle, and that only
-- works if trying things out is part of it. Which is why every wrong verb has an answer of its own
-- instead of simply doing nothing.
---------------------------------------------------------------------------------------------------

CLICK2POINT =
{
    -- How far the player may stand away from a point of interest. The horizontal reach is the
    -- important one in a 2.5D jump n run, the vertical one only keeps the floor above from counting.
    reach = 2.2,
    reachVertical = 2.0,

    -- The categories that are searched for points of interest. Collected ONCE per scene, see
    -- collectPointsOfInterest in PrehistoricLax.lua.
    poiCategories = { "Mechanics", "PointOfInterest" },

    -- Speech timing: a base time plus a bit per character, so no sentence needs its own duration and
    -- a long one still stays readable.
    speechBaseTime = 1.2,
    speechTimePerCharacter = 0.045,
    speechMaxTime = 7.0,

    -- The verbs, exactly the names of the MyGUIButtonComponents without the "Button" ending.
    verbs = { "Take", "Use", "Pull", "Talk" }
};

-- What is possible where. The key is the TagName of the point of interest, then the verb.
--
--   action = "..."   does something, see performAction in PrehistoricLax.lua
--   speech = "..."   the player says this and nothing happens
--
-- A verb that is missing here lands in CLICK2POINT_REFUSALS, so only the interesting answers have to
-- be written out.
CLICK2POINT_INTERACTIONS =
{
    Lever =
    {
        Pull = { action = "pullLever" },
        Take = { speech = "The lever is bolted into the rock. It stays where it is." },
        Use  = { speech = "I have nothing that would fit on a lever. Pulling it should do." },
        Talk = { speech = "A lever is a bad listener." }
    },

    LockYellow =
    {
        Use  = { action = "unlock" },
        Pull = { speech = "It does not budge. This one wants a key, not muscle." },
        Take = { speech = "The lock is part of the gate." },
        Talk = { speech = "The lock says nothing. Locks rarely do." }
    },

    LockRed =
    {
        Use  = { action = "unlock" },
        Pull = { speech = "It does not budge. This one wants a key, not muscle." },
        Take = { speech = "The lock is part of the gate." },
        Talk = { speech = "The lock says nothing. Locks rarely do." }
    },

    -- Anything lying around that carries an InventoryItemComponent: shoes, the flower, keys.
    Pickup =
    {
        Take = { action = "takeItem" },
        Use  = { speech = "First I should pick it up." },
        Pull = { speech = "Pulling will not help here." },
        Talk = { speech = "It does not answer. Which is a relief, really." }
    },

    -- Someone to talk to. The line itself comes from the speech bubble of the NPC, not from here.
    Npc =
    {
        Talk = { action = "talkToNpc" },
        Take = { speech = "I am fairly sure that is not allowed." },
        Use  = { speech = "Using people. What a thought." },
        Pull = { speech = "Pulling at strangers never ends well." }
    }
};

-- Said when the verb does nothing at this spot, or when there is nothing in front of the player at
-- all. Picked at random, so trying things out does not sound like a broken record.
CLICK2POINT_REFUSALS =
{
    Take =
    {
        "I can not take this.",
        "That stays where it is.",
        "Not everything that is loose belongs in my bag."
    },
    Use =
    {
        "I can not use this.",
        "That does not fit anywhere.",
        "Using it on what, exactly?"
    },
    Pull =
    {
        "There is nothing to pull here.",
        "Pulling that would only look silly.",
        "It does not move. And it is not supposed to."
    },
    Talk =
    {
        "No answer. As expected.",
        "Talking to that would be a new low."
    }
};

-- Talk with nothing in front of the player: the hero talks to himself. Pure flavour - and a hint
-- that the button does work.
CLICK2POINT_TALK_LINES =
{
    "Talking to myself again. Classic.",
    "Somewhere here there must be a way on.",
    "If my mother could see me now. Hunting crystals in a cave.",
    "The air smells of old stone and older bones.",
    "I should have brought more food.",
    "Hello? ... Nothing. Good.",
    "Whoever built this place had a strange sense of humour.",
    "One day I will tell this story and nobody will believe a word.",
    "Quiet. Too quiet, really.",
    "Right. Think, Lax. Think."
};

-- Which key fits which lock. The key is the TagName of the lock, 'item' the resource name in the
-- inventory, exactly as it is written in the inventory xml.
CLICK2POINT_LOCKS =
{
    LockYellow = { item = "YellowKeyItem", speech = "The yellow key fits. It really fits!" },
    LockRed    = { item = "RedKeyItem",    speech = "The red key turns. Something heavy moves behind the wall." }
};

-- Picks a random entry out of a list of sentences.
function randomLine(lines)
    return lines[math.random(1, #lines)];
end

-- Long sentences need to stay up longer. Saves a duration per line.
function speechDurationFor(text)
    local duration = CLICK2POINT.speechBaseTime + string.len(text) * CLICK2POINT.speechTimePerCharacter;
    if (duration > CLICK2POINT.speechMaxTime) then
        duration = CLICK2POINT.speechMaxTime;
    end
    return duration;
end
