module("MainGameObject", package.seeall);
-- Scene: Level1

require("init");

local mainGameObject = nil;
local explorationMap = nil;
local inputDeviceComp = nil;

local timeSinceLastToggle = 1;

MainGameObject = {}

MainGameObject["connect"] = function(gameObject)
    mainGameObject = AppStateManager:getGameObjectController():castGameObject(gameObject);
    explorationMap = mainGameObject:getExplorationMapComponent();

    local prehistoricLax = AppStateManager:getGameObjectController():getGameObjectFromName(PLAYER_NAME);
    inputDeviceComp = prehistoricLax:getInputDeviceComponent();

    timeSinceLastToggle = 1;

    -- Todo: not here but in menu state either via load several games -> player name required. Continue is just if in game and then went to menu and wants back to resume
    -- This script is connected BEFORE PrehistoricLax.lua, so the world is already in its final state when
    -- the player starts and collects its points of interest.
    Session.isRespawning = false;

    -- The attributes of the player live on this game object. A global value that is already there wins:
    -- that is the running game or a save game that has just been loaded.
    pullPlayerAttributesFromProgress(mainGameObject:getAttributesComponent());

    -- Taken items are gone for good, items that wait for their boss are hidden.
    applyWorldState();

    -- Pulled levers stay pulled. Was switched off in PrehistoricLax.lua, enable it here once the gates
    -- behave after a scene change.
    -- applyPulledLevers();
end

MainGameObject["disconnect"] = function()
    -- Hand the current values over to the next scene. Skipped while the dead player respawns: there the
    -- values either come from the save game or are reset completely, and the dead player must not overwrite
    -- them.
    if (false == Session.isRespawning) then
        pushPlayerAttributesToProgress(mainGameObject:getAttributesComponent());
    end

    AppStateManager:getGameObjectController():undoAll();
end

MainGameObject["update"] = function(dt)
    if (timeSinceLastToggle > 0) then
        timeSinceLastToggle = timeSinceLastToggle - dt;
    elseif inputDeviceComp:isActionDown(NOWA_A_MAP) then
        explorationMap:toggleFullMap();
        timeSinceLastToggle = 1;
    end
end

MainGameObject["onPlayerWaterContact"] = function(gameObject0, gameObject1, contact)
     local player = nil;
    
    gameObject0 = AppStateManager:getGameObjectController():castGameObject(gameObject0);
    gameObject1 = AppStateManager:getGameObjectController():castGameObject(gameObject1);
    
    if (gameObject1:getCategory() == "Player") then
        player = gameObject1;
    else
        player = gameObject0;
    end
    
    local waterParticle = player:getParticleFxComponentFromName("WaterParticle");
    --waterParticle:setGlobalPosition(player:getPosition());
    
    if (player:getPhysicsActiveComponent():getForce().x > 0.1) then
        --if (waterParticle:isPlaying() == false or waterParticle:isActivated() == false) then
            waterParticle:setActivated(true);
        --end
    end
end
