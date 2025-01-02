module("Scene3_Plane_0", package.seeall);
-- physicsActiveComponent = nil;
Scene3_Plane_0 = {}

-- Scene: Scene3

require("init");

scene3_Plane_0 = nil

Scene3_Plane_0["connect"] = function(gameObject)
	
    local coinGameObject = AppStateManager:getGameObjectController():getGameObjectFromId("1758440971");
    local mainGameObject = AppStateManager:getGameObjectController():getGameObjectFromId(MAIN_GAMEOBJECT_ID);
    
    mainGameObject:getSimpleSoundComponent():setActivated(true);
    AppStateManager:getGameObjectController():deleteGameObject(coinGameObject:getId());
    
end

Scene3_Plane_0["disconnect"] = function()
    AppStateManager:getGameObjectController():undoAll();
end
