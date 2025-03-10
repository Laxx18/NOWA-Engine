module("Inventory1_MainGameObject", package.seeall);
Inventory1_MainGameObject = {}

-- Scene: Inventory1

timeSinceLastSwitch = 0.2;
canSwitch = false;

oldGameObject = nil;
nextGameObject = nil;
mainGameObject = nil;
inputDeviceModule = nil;

require("init");

inventory1_MainGameObject = nil

Inventory1_MainGameObject["connect"] = function(gameObject)
    gameObject = AppStateManager:getGameObjectController():castGameObject(gameObject);
    mainGameObject = gameObject;
    inputDeviceModule = mainGameObject:getInputDeviceComponent():getInputDeviceModule();
end

Inventory1_MainGameObject["disconnect"] = function()
    AppStateManager:getGameObjectController():undoAll();
end

Inventory1_MainGameObject["update"] = function(dt)
    if timeSinceLastSwitch >= 0 then
        timeSinceLastSwitch = timeSinceLastSwitch - dt;
    end

    if timeSinceLastSwitch <= 0 then
        if inputDeviceModule:isActionDown(NOWA_A_SELECT) == false then
            canSwitch = true;
        end
        
        if inputDeviceModule:isActionDown(NOWA_A_SELECT) and canSwitch == true then
            oldGameObject = nextGameObject;
            nextGameObject = AppStateManager:getGameObjectController():getNextGameObject("Player");
            
            local oldEmmisiveColor = nextGameObject:getDatablockPbsComponent():getEmissiveColor();
            
            nextGameObject:getAttributeEffectComponent():setActivated(true);
            nextGameObject:getMyGUIItemBoxComponent():setActivated(true);
        
            if (oldGameObject ~= nil) then
                mainGameObject:getSelectGameObjectsComponent():select(oldGameObject:getId(), false);
                oldGameObject:getMyGUIItemBoxComponent():setActivated(false);
            end
            
            log("Start effect!");
            nextGameObject:getAttributeEffectComponent():reactOnEndOfEffect(function(functionResult)
                nextGameObject:getAttributeEffectComponent():setActivated(false);
                nextGameObject:getDatablockPbsComponent():setEmissiveColor(oldEmmisiveColor);
                log("End effect!");
            end, false);
        
            canSwitch = false;
            timeSinceLastSwitch = 0.2;
            
            mainGameObject:getSelectGameObjectsComponent():select(nextGameObject:getId(), true);
            
            AppStateManager:getGameObjectController():activatePlayerController(true, nextGameObject:getId(), false);
        end
    end
end