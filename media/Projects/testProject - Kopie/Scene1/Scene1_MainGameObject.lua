module("Scene1_MainGameObject", package.seeall);
-- physicsActiveComponent = nil;
Scene1_MainGameObject = {}

-- Scene: Scene1

require("init");

handle = nil;
rightLowerArm = nil;

scene1_MainGameObject = nil

Scene1_MainGameObject["connect"] = function(gameObject)
    handle = AppStateManager:getGameObjectController():getGameObjectFromId("1435198980");
    rightLowerArm = AppStateManager:getGameObjectController():getGameObjectFromId("2697824071");
    
    -- Just sets the arm to the handle
    local targetPosition = handle:getJointHingeComponent():getUpdatedJointPosition() - Vector3(rightLowerArm:getSize().x * 0.5, 0, 0);
    rightLowerArm:getJointKinematicComponent():setTargetPosition(targetPosition);
    
    rightLowerArm:getJointKinematicComponent():setTargetRotation(handle:getOrientation());
    --rightLowerArm:getJointKinematicComponent():setTargetRotation(Quaternion(Degree(90), Vector3.UNIT_Z));
    
    rightLowerArm:getJointKinematicComponent():reactOnTargetPositionReached(function() 
            -- Remove, it has just been used to set the target position physically
            --rightLowerArm:getJointKinematicComponent():setActivated(false);
            rightLowerArm:getJointHingeComponent():setActivated(true);
            end);
end

Scene1_MainGameObject["disconnect"] = function()
    AppStateManager:getGameObjectController():undoAll();
end

Scene1_MainGameObject["update"] = function(dt)
	
end