module("BossAreaMechanics", package.seeall);
-- Scene: Level5

require("init");

local bossAreaMechanics = nil

BossAreaMechanics = {}

BossAreaMechanics["connect"] = function(gameObject)
    bossAreaMechanics = AppStateManager:getGameObjectController():castGameObject(gameObject);
    physicsTriggerComponent = bossAreaMechanics:getPhysicsTriggerComponent();
    
    physicsTriggerComponent:reactOnEnter(function(visitorGameObject)
          visitorGameObject = AppStateManager:getGameObjectController():castGameObject(visitorGameObject);
          
        local areaDoors = AppStateManager:getGameObjectController():getGameObjectsFromCategory("BossAreaDoor");

        for key, areaDoorGameObject in pairs(areaDoors) do
            -- The table holds the raw pointer. Without the cast lua only sees part of the class, which
            -- is why getTagName() worked and getName() came back nil.
            local areaDoorGameObject = AppStateManager:getGameObjectController():castGameObject(areaDoorGameObject);

            areaDoorGameObject:getJointSliderActuatorComponent():setTargetPosition(0);
            areaDoorGameObject:getJointSliderActuatorComponent():setLinearRate(-2);
            areaDoorGameObject:getJointSliderActuatorComponent():setActivated(true);
            AppStateManager:getGameObjectController():activateGameObjectComponentsFromReferenceId(areaDoorGameObject:getReferenceId(), true);
            
            -- Wakes the end boss up. Until this event the pterodactyl only hovers at its start position
            -- and ignores the player completely.
            local eventData = {};
            AppStateManager:getScriptEventManager():queueEvent(EventType.BossFightStartEvent, eventData);
        end
    end);
end

BossAreaMechanics["disconnect"] = function()

end