module("MainGameObject", package.seeall);
-- Scene: Road1

require("init");

mainGameObject = nil;
forTwoPlayer = nil;
player1 = nil;
player2 = nil;

MainGameObject = {}

MainGameObject["connect"] = function(gameObject)
    mainGameObject = AppStateManager:getGameObjectController():castGameObject(gameObject);
    
    if (Core:isGame() == true) then
        forTwoPlayer = AppStateManager:getGameProgressModule():getGlobalValue("TwoPlayer");
        if (forTwoPlayer) then
            player1 = AppStateManager:getGameObjectController():getGameObjectFromId("1026796640");
            local deviceName1 = AppStateManager:getGameProgressModule():getGlobalValue("PlayerOneDeviceName");
            player1:getInputDeviceComponent():setDeviceName(deviceName1);
            player2 = AppStateManager:getGameObjectController():getGameObjectFromId("2474775319");
            local deviceName2 = AppStateManager:getGameProgressModule():getGlobalValue("PlayerTwoDeviceName");
            player2:getInputDeviceComponent():setDeviceName(deviceName2);
            
             local colorPlayer1 = AppStateManager:getGameProgressModule():getGlobalValue("PlayerOneColor");
             player1:getDatablockPbsComponent():setDiffuseTextureName(colorPlayer1);
             local colorPlayer2 = AppStateManager:getGameProgressModule():getGlobalValue("PlayerTwoColor");
             player2:getDatablockPbsComponent():setDiffuseTextureName(colorPlayer2);
             
             AppStateManager:getGameObjectController():activatePlayerControllerForCamera(true, player1:getId(), "2083676694", true);
             AppStateManager:getGameObjectController():activatePlayerControllerForCamera(true, player2:getId(), "252923812", true);
             
        else
            player1 = AppStateManager:getGameObjectController():getGameObjectFromId("1026796640");
            local deviceName1 = AppStateManager:getGameProgressModule():getGlobalValue("PlayerOneDeviceName");
            player1:getInputDeviceComponent():setDeviceName(deviceName1);
            local colorPlayer1 = AppStateManager:getGameProgressModule():getGlobalValue("PlayerOneColor");
            player1:getDatablockPbsComponent():setDiffuseTextureName(colorPlayer1);
            
            AppStateManager:getGameObjectController():activatePlayerControllerForCamera(true, player1:getId(), "2083676694", true);
            
        end
    else
        -- For debug simulation
        player1 = AppStateManager:getGameObjectController():getGameObjectFromId("1026796640");
        local deviceName1 = "Win32InputManager";
        player1:getInputDeviceComponent():setDeviceName(deviceName1);
        player2 = AppStateManager:getGameObjectController():getGameObjectFromId("2474775319");
        local deviceName2 = "USB Gamepad _0";
        player2:getInputDeviceComponent():setDeviceName(deviceName2);
        
        AppStateManager:getGameObjectController():activatePlayerControllerForCamera(true, player1:getId(), "2083676694", true);
        AppStateManager:getGameObjectController():activatePlayerControllerForCamera(true, player2:getId(), "252923812", true);
        
         log("---->Menu set devices");
    end
end

MainGameObject["disconnect"] = function()

end