module("Menu_MainGameObject", package.seeall);

Menu_MainGameObject = {}

-- Scene: Menu

require("init");

menu_MainGameObject = nil;
player1 = nil;
player2 = nil;
onePlayerCombo = nil;
twoPlayerButton = nil;

Menu_MainGameObject["connect"] = function(gameObject)
    menu_MainGameObject = AppStateManager:getGameObjectController():castGameObject(gameObject);
    local onePlayerButton = menu_MainGameObject:getMyGUIButtonComponentFromName("OnePlayerButton");
    onePlayerCombo = menu_MainGameObject:getMyGUIComboBoxComponentFromName("OnePlayerCombo");
    local twoPlayerButton = menu_MainGameObject:getMyGUIButtonComponentFromName("TwoPlayerButton");
    twoPlayerCombo = menu_MainGameObject:getMyGUIComboBoxComponentFromName("TwoPlayerCombo");
    local startButton = menu_MainGameObject:getMyGUIButtonComponentFromName("StartButton");
  
    player1 = AppStateManager:getGameObjectController():getGameObjectFromId("449055701");
    player2 = AppStateManager:getGameObjectController():getGameObjectFromId("1220359006");
    
    actualizeDevices1();
    actualizeDevices2();
   
    onePlayerCombo:reactOnSelected(function(index)
       
       local selectedDevice = onePlayerCombo:getItemText(index);
       player1:getInputDeviceComponent():setDeviceName(selectedDevice);
       
       if ( player1:getInputDeviceComponent():hasValidDevice()) then
            actualizeDevices2();
            AppStateManager:getGameProgressModule():setGlobalStringValue("PlayerOneDeviceName", selectedDevice);
       end
    end);
    
    twoPlayerCombo:reactOnSelected(function(index)
       
       local selectedDevice = twoPlayerCombo:getItemText(index);
       player2:getInputDeviceComponent():setDeviceName(selectedDevice);
       
       if ( player2:getInputDeviceComponent():hasValidDevice()) then
            actualizeDevices1();
            AppStateManager:getGameProgressModule():setGlobalStringValue("PlayerTwoDeviceName", selectedDevice);
       end
    end);
    
    player2:setVisible(false);
    twoPlayerCombo:setActivated(false);
    
    local menuCamera = AppStateManager:getGameObjectController():getGameObjectFromId("2904886742");
    menuCamera:getCameraComponent():setActivated(true);
    
    onePlayerButton:reactOnMouseButtonClick(function() 
       onePlayerButton:setEnabled(false);
       twoPlayerButton:setEnabled(true);
       player2:setVisible(false);
       twoPlayerCombo:setActivated(false);
       
        AppStateManager:getGameProgressModule():setGlobalBoolValue("TwoPlayer", false);
    end);
    
    twoPlayerButton:reactOnMouseButtonClick(function() 
       twoPlayerButton:setEnabled(false);
       onePlayerButton:setEnabled(true);
       player2:setVisible(true);
       twoPlayerCombo:setActivated(true);

       AppStateManager:getGameProgressModule():setGlobalBoolValue("TwoPlayer", true);
    end);
    
    startButton:reactOnMouseButtonClick(function() 
        if (Core:isGame() == true) then
            AppStateManager:changeAppState("GameState");
        end
    end);
end

Menu_MainGameObject["disconnect"] = function()
    AppStateManager:getGameObjectController():undoAll();
end

Menu_MainGameObject["update"] = function(dt)

end

function actualizeDevices1()
    local devices1 = player1:getInputDeviceComponent():getActualizedDeviceList();
    onePlayerCombo:setItemCount(0);
    local i = 0
    while i <= #devices1 do
        local deviceName = devices1[i];
        if (i == 0) then
            onePlayerCombo:setCaption(deviceName);
        end
        onePlayerCombo:addItem(deviceName);
        i = i + 1;
    end
end

function actualizeDevices2()
    local devices2 = player2:getInputDeviceComponent():getActualizedDeviceList();
    twoPlayerCombo:setItemCount(0);
    local i = 0
    while i <= #devices2 do
       local deviceName = devices2[i];
        if (i == 0) then
            twoPlayerCombo:setCaption(deviceName);
        end
       twoPlayerCombo:addItem(deviceName);
       i = i + 1;
    end
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