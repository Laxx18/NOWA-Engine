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
end

MainGameObject["disconnect"] = function()
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