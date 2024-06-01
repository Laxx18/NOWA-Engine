module("Scene1_QuatBike_0", package.seeall);

Scene1_QuatBike_0 = {}

-- Scene: Scene1

require("init");

scene1_QuatBike_0 = nil
yawAtSpeed = 0;

Scene1_QuatBike_0["connect"] = function(gameObject)
    AppStateManager:getGameObjectController():activatePlayerController(true, gameObject:getId(), true);
end

Scene1_QuatBike_0["disconnect"] = function()

end

Scene1_QuatBike_0["onSteeringAngleChanged"] = function(gameObject, vehicleDrivingManipulation, dt)
    if InputDeviceModule:isActionDown(NOWA_A_LEFT) then
        if (yawAtSpeed <= 45) then
            yawAtSpeed = yawAtSpeed + dt * 60;
        end
        vehicleDrivingManipulation:setSteerAngle(yawAtSpeed);
    elseif InputDeviceModule:isActionDown(NOWA_A_RIGHT) then
        if (yawAtSpeed >= -45) then
            yawAtSpeed = yawAtSpeed - dt * 60;
        end
            
    vehicleDrivingManipulation:setSteerAngle(yawAtSpeed);
    else
        if (yawAtSpeed > 0) then
            yawAtSpeed = yawAtSpeed - dt * 60;
            vehicleDrivingManipulation:setSteerAngle(yawAtSpeed);
        elseif (yawAtSpeed < 0) then
            yawAtSpeed = yawAtSpeed + dt * 60;
            vehicleDrivingManipulation:setSteerAngle(yawAtSpeed);
        end
    end
end

Scene1_QuatBike_0["onMotorForceChanged"] = function(gameObject, vehicleDrivingManipulation, dt)
    if InputDeviceModule:isActionDown(NOWA_A_UP) then
        vehicleDrivingManipulation:setMotorForce(5000 * 120 * dt);
    elseif InputDeviceModule:isActionDown(NOWA_A_ACTION) then
        vehicleDrivingManipulation:setMotorForce(10000 * 120 * dt);
    elseif InputDeviceModule:isActionDown(NOWA_A_DOWN) then
        vehicleDrivingManipulation:setMotorForce(-4500 * 120 * dt);
    end
end

Scene1_QuatBike_0["onHandBrakeChanged"] = function(gameObject, vehicleDrivingManipulation, dt)
    -- Jump: Space
    if InputDeviceModule:isActionDown(NOWA_A_JUMP) then
        vehicleDrivingManipulation:setHandBrake(5.5);
    end
end

Scene1_QuatBike_0["onBrakeChanged"] = function(gameObject, vehicleDrivingManipulation, dt)
    -- Cover: x
    if InputDeviceModule:isActionDown(NOWA_A_COWER) then
        vehicleDrivingManipulation:setBrake(7.5);
    end
end