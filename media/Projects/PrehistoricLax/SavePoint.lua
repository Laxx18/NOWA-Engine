module("SavePoint", package.seeall);
-- Scene: Level1

require("init");

local savePoint = nil;
local physicsTriggerComponent = nil;

SavePoint = {}

SavePoint["connect"] = function(gameObject)
    savePoint = AppStateManager:getGameObjectController():castGameObject(gameObject);
    physicsTriggerComponent = savePoint:getPhysicsTriggerComponent();

    physicsTriggerComponent:reactOnEnter(function(visitorGameObject)
        visitorGameObject = AppStateManager:getGameObjectController():castGameObject(visitorGameObject);

        -- Only the player saves the game, not an enemy or a crate that is pushed in here.
        if (visitorGameObject:getName() ~= PLAYER_NAME) then
            do return end;
        end

        savePoint:getParticleFxComponent():setActivated(true);
        savePoint:getSimpleSoundComponent():setActivated(true);

        -- The player attributes live on the main game object, see init.lua. saveGame pushes them into the
        -- global values first and then writes the save game including the scene snapshot.
        local mainGameObject = AppStateManager:getGameObjectController():getGameObjectFromId(MAIN_GAMEOBJECT_ID);
        saveGame(mainGameObject:getAttributesComponent());

        log("[SavePoint] Game saved at: " .. savePoint:getName());
    end);
end

SavePoint["disconnect"] = function()
    savePoint = nil;
    physicsTriggerComponent = nil;
end
