module("Scene1Viper_Viper_0", package.seeall);
-- physicsActiveComponent = nil;
Scene1Viper_Viper_0 = {}

-- Scene: Scene1Viper

require("init");

scene1Viper_Viper_0 = nil

Scene1Viper_Viper_0["connect"] = function(gameObject)

end

Scene1Viper_Viper_0["disconnect"] = function()

end

Scene1Viper_Viper_0["onSteeringAngleChanged"] = function(gameObject, vehicleDrivingManipulation, dt)
    if InputDeviceModule:isActionDown(NOWA_A_LEFT) then
        vehicleDrivingManipulation:setSteerAngle(45);
    elseif InputDeviceModule:isActionDown(NOWA_A_RIGHT) then
        vehicleDrivingManipulation:setSteerAngle(-45);
    end
end

Scene1Viper_Viper_0["onMotorForceChanged"] = function(gameObject, vehicleDrivingManipulation, dt)
    if InputDeviceModule:isActionDown(NOWA_A_UP) then
        vehicleDrivingManipulation:setMotorForce(5000 * 120 * dt);
    elseif InputDeviceModule:isActionDown(NOWA_A_DOWN) then
        vehicleDrivingManipulation:setMotorForce(-4500 * 120 * dt);
    end
end

Scene1Viper_Viper_0["onHandBrakeChanged"] = function(gameObject, vehicleDrivingManipulation, dt)
    -- Jump: Space
    if InputDeviceModule:isActionDown(NOWA_A_JUMP) then
        vehicleDrivingManipulation:setHandBrake(5.5);
    end
end

Scene1Viper_Viper_0["onBrakeChanged"] = function(gameObject, vehicleDrivingManipulation, dt)
    -- Cover: x
    if InputDeviceModule:isActionDown(NOWA_A_COWER) then
        vehicleDrivingManipulation:setBrake(7.5);
    end
end