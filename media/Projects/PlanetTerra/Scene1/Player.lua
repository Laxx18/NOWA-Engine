module("Player", package.seeall);
-- Scene: Scene1

require("init");

playerGo = nil;
physicsActiveComponent = nil;

Player = {}

Player["connect"] = function(gameObject)
    playerGo = AppStateManager:getGameObjectController():castGameObject(gameObject);
    
    local universumGameObject = AppStateManager:getGameObjectController():getGameObjectFromId("1826795857");
    local universumComponent = universumGameObject:getUniversumComponent();
    
    local fadeWindowComponent = universumGameObject:getMyGUIFadeAlphaControllerComponent();
    local landStartButton = universumGameObject:getMyGUIButtonComponentFromName("LandStartButton");
    
    local fighterGo = AppStateManager:getGameObjectController():getGameObjectFromId("2599276278");
 
    -- Button toggles between Land and Start (takeoff).
    landStartButton:reactOnMouseButtonClick(function()
        if landStartButton:getCaption() == "Take Off" then
            landStartButton:setCaption("Land");
            universumComponent:requestTakeoff();
            fighterGo:getPhysicsActiveComponent():setGravitySourceCategory("");
            fighterGo:getInputDeviceComponent():setActivated(true);
            fighterGo:getPhysicsActiveComponent():setActivated(true);
            fighterGo:getParticleFxComponent():setActivated(true);
            playerGo:setVisible(false);
            AppStateManager:getGameObjectController():activatePlayerController(true, fighterGo:getId(), true);
            playerGo:getPhysicsActiveComponent():setActivated(false);
        end
    end);
    
    physicsActiveComponent = playerGo:getPhysicsActiveComponent();
    physicsActiveComponent:setContactSolvingEnabled(true);

    physicsActiveComponent:reactOnContactSolving(function(otherGameObject, contact)
        if (otherGameObject:getCategory() == "Spaceship") then
            if (fadeWindowComponent:getAlpha() ~= 1) then
                fadeWindowComponent:setAlpha(1);
                fadeWindowComponent:setActivated(true);
            end
            landStartButton:setCaption("Take Off");
        end
    end);
end

Player["disconnect"] = function()
    physicsActiveComponent:setContactSolvingEnabled(false);
end