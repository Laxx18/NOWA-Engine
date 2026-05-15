module("Player", package.seeall);
require("init");

local player                  = nil
local physicsActiveComponent  = nil
local pathNavigationComponent = nil

Player = {}

Player["connect"] = function(gameObject)
    player                  = AppStateManager:getGameObjectController():castGameObject(gameObject)
    physicsActiveComponent  = player:getPhysicsActiveComponent()
    pathNavigationComponent = player:getAiRecastPathNavigationComponent()

    log("[Player] connect: player id=" .. tostring(player:getId()))

    pathNavigationComponent:setRepeat(false)
    pathNavigationComponent:setDirectionChange(false)

    -- Each worker activates itself — path slot is assigned by MainGameObject
    -- when the worker is selected as part of a group
    AppStateManager:getGameObjectController():activatePlayerController(true, player:getId(), false)

    log("[Player] connect complete.")
end

Player["disconnect"] = function()
    log("[Player] disconnect")
end

Player["update"] = function(dt)
end