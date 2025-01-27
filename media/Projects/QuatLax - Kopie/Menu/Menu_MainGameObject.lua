module("Menu_MainGameObject", package.seeall);

Menu_MainGameObject = {}

-- Scene: Menu

require("init");

menu_MainGameObject = nil

Menu_MainGameObject["connect"] = function(gameObject)
    menu_MainGameObject = AppStateManager:getGameObjectController():castGameObject(gameObject);
    local onePlayerButton = menu_MainGameObject:getMyGUIButtonComponentFromName("OnePlayerButton");
    local twoPlayerButton = menu_MainGameObject:getMyGUIButtonComponentFromName("TwoPlayerButton");
    local startButton = menu_MainGameObject:getMyGUIButtonComponentFromName("StartButton");
    
    onePlayerButton:reactOnMouseButtonClick(function() 
       onePlayerButton:setEnabled(false);
       twoPlayerButton:setEnabled(true);
       
        --local eventData = {};
        --eventData["playerCount"] = 1;
        --AppStateManager:getScriptEventManager():queueEvent(EventType.PlayerCountEvent, eventData);
        AppStateManager:getGameProgressModule():setGlobalBoolValue("TwoPlayer", false);
    end);
    
    twoPlayerButton:reactOnMouseButtonClick(function() 
       twoPlayerButton:setEnabled(false);
       onePlayerButton:setEnabled(true);
       
      -- local eventData = {};
       --eventData["playerCount"] = 2;
       --AppStateManager:getScriptEventManager():queueEvent(EventType.PlayerCountEvent, eventData);
       
       AppStateManager:getGameProgressModule():setGlobalBoolValue("TwoPlayer", true);
    end);
    
    startButton:reactOnMouseButtonClick(function() 
        if (Core:isGame() == true) then
            AppStateManager:changeAppState("GameState");
        end
    end);
end

Menu_MainGameObject["disconnect"] = function()

end

-- in lua script of a course:

 --if (EventType.PlayerCountEvent ~= nil) then
 --           AppStateManager:getScriptEventManager():registerEventListener(EventType.PlayerCountEvent, Course1["onPlayerCountEvent"]);
 --end

--Course1["onPlayerCountEvent"] = function(eventData)
--    local playerCount = eventData["playerCount"];
--    --log("###onRemoveLaser: " .. id);
--    add split screen if playerCount > 1
--end