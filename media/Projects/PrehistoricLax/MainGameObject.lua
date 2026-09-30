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

end

MainGameObject["update"] = function(dt)
    if (timeSinceLastToggle > 0) then
        timeSinceLastToggle = timeSinceLastToggle - dt;
    elseif inputDeviceComp:isActionDown(NOWA_A_MAP) then
        explorationMap:toggleFullMap();
        timeSinceLastToggle = 1;
    end
end