module("MainGameObject", package.seeall);
-- Scene: Road1

require("init");

mainGameObject = nil;
forTwoPlayer = nil;

MainGameObject = {}

MainGameObject["connect"] = function(gameObject)
    mainGameObject = AppStateManager:getGameObjectController():castGameObject(gameObject);
    forTwoPlayer = AppStateManager:getGameProgressModule():getGlobalValue("TwoPlayer");
    if (forTwoPlayer) then
        log("--->For two Player: " .. toString(forTwoPlayer:getValueBool()));
    else
        log("--->For two Player: false");
    end
end

MainGameObject["disconnect"] = function()

end