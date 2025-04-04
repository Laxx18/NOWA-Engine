module("MainGameObject", package.seeall);
-- Scene: Scene1

require("init");

mainGameObject = nil

cameraComponent = nil;

MainGameObject = {}

MainGameObject["connect"] = function(gameObject)
    mainGameObject = AppStateManager:getGameObjectController():castGameObject(gameObject);
    cameraComponent = AppStateManager:getGameObjectController():getGameObjectFromId("2674670936"):getCameraComponent();
    cameraComponent:setActivated(true);
    
     PointerManager:showMouse(false);
end

MainGameObject["disconnect"] = function()
     cameraComponent:setActivated(false);
     PointerManager:showMouse(true);
end

MainGameObject["update"] = function(dt)
   
end