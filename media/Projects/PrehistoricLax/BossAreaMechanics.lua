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

        for key, areaDoor in pairs(areaDoors) do
            -- The table holds the raw pointer. Without the cast lua only sees part of the class, which
            -- is why getTagName() worked and getName() came back nil.
            local areaDoorGameObject = AppStateManager:getGameObjectController():castGameObject(areaDoor);
 
            -- Attention: NO setActivated(true) here, and no activateGameObjectComponentsFromReferenceId
            -- either. Activating the component releases the joint and creates a new one, and a slider
            -- actuator measures its position relative to the frame it was created in. A door standing
            -- open would therefore sit at position 0 again, a target of 0 would already be reached and
            -- nothing moves at all. The joint is alive since the door opened, so only the new target is
            -- pushed into it.
            --
            -- The rate is positive: the direction comes from the difference between the current position
            -- and the target, not from the sign of the rate (ndOgreSliderActuator::SetLinearRate applies
            -- ndAbs to it anyway).
            areaDoorGameObject:getJointSliderActuatorComponent():setLinearRate(10);
            areaDoorGameObject:getJointSliderActuatorComponent():setTargetPosition(0);
            
            local eventData = {};
            AppStateManager:getScriptEventManager():queueEvent(EventType.BossFightStartEvent, eventData);
 
            log("--> close door: " .. areaDoorGameObject:getName());
        end
    end);
end

BossAreaMechanics["disconnect"] = function()

end