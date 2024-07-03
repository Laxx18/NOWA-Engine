module("Scene1_MainGameObject", package.seeall);

Scene1_MainGameObject = {}

-- Scene: Scene1

require("init");

scene1_MainGameObject = nil;
timeSinceLastSwitch = 0.2;
canSwitch = false;

oldGameObject = nil;
nextGameObject = nil;
mainGameObject = nil;

Scene1_MainGameObject["connect"] = function(gameObject)
    mainGameObject = gameObject;
end

Scene1_MainGameObject["disconnect"] = function()
    AppStateManager:getGameObjectController():undoAll();
end

Scene1_MainGameObject["update"] = function(dt)
   if timeSinceLastSwitch >= 0 then
		timeSinceLastSwitch = timeSinceLastSwitch - dt;
	end

    if timeSinceLastSwitch <= 0 then
        if InputDeviceModule:isActionDown(NOWA_A_SELECT) == false then
            canSwitch = true;
        end
        
        if InputDeviceModule:isActionDown(NOWA_A_SELECT) and canSwitch == true then
            oldGameObject = nextGameObject;
            nextGameObject = AppStateManager:getGameObjectController():getNextGameObject("Quat");
            

            if (oldGameObject ~= nil) then
                mainGameObject:getSelectGameObjectsComponent():select(oldGameObject:getId(), false);
            end
        
            canSwitch = false;
            timeSinceLastSwitch = 0.2;
            
            mainGameObject:getSelectGameObjectsComponent():select(nextGameObject:getId(), true);
            
            AppStateManager:getGameObjectController():activatePlayerController(true, nextGameObject:getId(), false);
        end
    end
end