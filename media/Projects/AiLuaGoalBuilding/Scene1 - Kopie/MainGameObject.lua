module("MainGameObject", package.seeall);
-- Scene: Scene1

require("init");

mainGameObject = nil

MainGameObject = {}

MainGameObject["connect"] = function(gameObject)
    --mainGameObject = AppStateManager:getGameObjectController():castGameObject(gameObject);

end

MainGameObject["disconnect"] = function()
    AppStateManager:getGameObjectController():undoAll()
end