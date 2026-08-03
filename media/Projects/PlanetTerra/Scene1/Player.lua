module("Player", package.seeall);
-- Scene: Scene1

require("init");

local playerGo = nil;
local areaOfInterestComp = nil;
local planetMinimapGo = nil;
local mainGo = nil;
local inputDeviceComp = nil;
local flashLightComp = nil;
local toggleFlashLight = false;

PLANET_MINIMAP_GO_ID = "1527832358";

Player = {}

Player["connect"] = function(gameObject)
    playerGo = AppStateManager:getGameObjectController():castGameObject(gameObject);
    inputDeviceComp = playerGo:getInputDeviceComponent();
    local flashLightGo = AppStateManager:getGameObjectController():getGameObjectFromId("2467979983");
    flashLightComp = flashLightGo:getLightSpotComponent();
    flashLightComp:setActivated(false);
    
    local universumGameObject = AppStateManager:getGameObjectController():getGameObjectFromId("1061997306");
    local universumComponent = universumGameObject:getUniversumComponent();
    
    local fadeWindowComponent = universumGameObject:getMyGUIFadeAlphaControllerComponent();
    local landStartButton = universumGameObject:getMyGUIButtonComponentFromName("LandStartButton");
    
    local fighterGo = AppStateManager:getGameObjectController():getGameObjectFromId("2599276278");
    
    planetMinimapGo = AppStateManager:getGameObjectController():getGameObjectFromId(PLANET_MINIMAP_GO_ID);
    mainGo = AppStateManager:getGameObjectController():getGameObjectFromId(MAIN_GAMEOBJECT_ID);
 
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
            
            planetMinimapGo:getPlanetMinimapComponent():setCompassGameObjectId(0, "0");
            planetMinimapGo:getPlanetMinimapComponent():setCompassGameObjectId(1, "0");
            planetMinimapGo:getPlanetMinimapComponent():setActivated(false);
            
            mainGo:getSimpleSoundComponentFromName("FlyMusic"):setActivated(true);
        end
    end);
    
    areaOfInterestComp = playerGo:getAreaOfInterestComponent();
    areaOfInterestComp:reactOnEnter(function(otherGameObject) 
        otherGameObject = AppStateManager:getGameObjectController():castGameObject(otherGameObject);
        if (otherGameObject:getCategory() == "Spaceship") then
             if (fadeWindowComponent:getAlpha() ~= 1) then
                fadeWindowComponent:setAlpha(1);
                fadeWindowComponent:setActivated(true);
            end
            landStartButton:setCaption("Take Off");
        elseif (otherGameObject:getCategory() == "Quester") then
            log("->hier in quester");
            local eventData = {};
            eventData["inArea"] = true;
            eventData["playerId"] = playerGo:getId();
            AppStateManager:getScriptEventManager():queueEvent(EventType.QuesterFoundEvent, eventData);
        end
    end);
    
    areaOfInterestComp:reactOnLeave(function(otherGameObject) 
        otherGameObject = AppStateManager:getGameObjectController():castGameObject(otherGameObject);
        if (otherGameObject:getCategory() == "Spaceship") then
            fadeWindowComponent:setAlpha(0);
            fadeWindowComponent:setActivated(true);
        elseif (otherGameObject:getCategory() == "Quester") then
            local eventData = {};
            eventData["inArea"] = false;
            eventData["playerId"] = "0";
            AppStateManager:getScriptEventManager():queueEvent(EventType.QuesterFoundEvent, eventData);
            log("->quester raus");
        end
    end);
    
end

Player["update"] = function(dt)

    if (inputDeviceComp:isDeviceLocked() == false) then
        if inputDeviceComp:isActionDownPressed(NOWA_A_FLASH_LIGHT, dt, 0.2) then
           toggleFlashLight = not toggleFlashLight;
           flashLightComp:setActivated(toggleFlashLight);
        end
    end
end