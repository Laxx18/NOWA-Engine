module("PrehistoricLax", package.seeall);
-- Scene: Level1

require("init");

local prehistoricLax = nil;
local cameraComponent = nil;

PrehistoricLax = {}

PrehistoricLax["connect"] = function(gameObject)
    PointerManager:showMouse(false);
    prehistoricLax = AppStateManager:getGameObjectController():castGameObject(gameObject);
    -- Sets the camera id for the camera behavior
    local cameraGameObject = AppStateManager:getGameObjectController():getGameObjectFromName("GameCamera");
    prehistoricLax:getCameraBehaviorComponent():setCameraGameObjectId(cameraGameObject:getId());
    
    cameraComponent = AppStateManager:getGameObjectController():getGameObjectFromName("GameCamera"):getCameraComponent();
    -- Important: Camera switch
    cameraComponent:setActivated(true);
    
    AppStateManager:getGameObjectController():activatePlayerController(true, prehistoricLax:getId(), true);
end

PrehistoricLax["disconnect"] = function()
    PointerManager:showMouse(true);
    cameraComponent:setActivated(false);
    AppStateManager:getGameObjectController():undoAll();
end

--PrehistoricLax["update"] = function(dt)

--end