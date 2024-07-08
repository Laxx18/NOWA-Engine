module("Scene1_QuatBike_Computer_0", package.seeall);

Scene1_QuatBike_Computer_0 = {}

-- Scene: Scene1

require("init");

scene1_QuatBike_0 = nil
steerAmount = 0;
pitchAmount = 0;
motorForce = 0;
physicsActiveVehicleComponent = nil;
purePursuitComponent = nil;
rollSound = nil;
skidSound = nil;

Scene1_QuatBike_Computer_0["connect"] = function(gameObject)
    --AppStateManager:getGameObjectController():activatePlayerController(true, gameObject:getId(), true);
    physicsActiveVehicleComponent = gameObject:getPhysicsActiveVehicleComponent();
    purePursuitComponent = gameObject:getPurePursuitComponent();
    rollSound = gameObject:getSimpleSoundComponent2(0);
    rollSound:setVolume(200);
    
    skidSound = gameObject:getSimpleSoundComponent2(1);
end

Scene1_QuatBike_Computer_0["disconnect"] = function()
    AppStateManager:getGameObjectController():undoAll();
end

Scene1_QuatBike_Computer_0["update"] = function(dt)
    --log("vel: " .. toString(physicsActiveVehicleComponent:getVelocity():squaredLength()));
    local motion = physicsActiveVehicleComponent:getVelocity():squaredLength() + 40;
    if motion > 40 + (0.2 * 0.2) then
		rollSound:setActivated(true);
        -- If started to drive, motor engine sound shall remain in a deep voice
        if (motion < 40) then
            motion = 40;
        end
		rollSound:setPitch(motion * 0.01);
        --else
        --	rollSound:setActivated(false);
    end

    --local heading = physicsActiveVehicleComponent:getOrientation():getYaw(false):valueRadians();
    --local pitch = physicsActiveVehicleComponent:getOrientation():getPitch(false):valueRadians();

    --steerAmount = purePursuitComponent:calculateSteeringAngle(physicsActiveVehicleComponent:getPosition(), heading);
    --pitchAmount = purePursuitComponent:calculatePitchAngle(physicsActiveVehicleComponent:getPosition(), pitch);
    --motorForce = purePursuitComponent:calculateMotorForce(physicsActiveVehicleComponent:getPosition());
    
    steerAmount = purePursuitComponent:getSteerAmount();
    pitchAmount = purePursuitComponent:getPitchAmount();
    motorForce = purePursuitComponent:getMotorForce();
    
end


Scene1_QuatBike_Computer_0["onSteeringAngleChanged"] = function(vehicleDrivingManipulation, dt)
    
    --log("steerAmountBefore: " .. steerAmount);
--    if (steerAmount > 45) then
--        steerAmount = 45;
--    elseif (steerAmount < -45) then
--            steerAmount = 45;
--    end
    
    --log("steerAmount: " .. steerAmount);
    --log("pitchAmount: " .. pitchAmount);
    

    vehicleDrivingManipulation:setSteerAngle(steerAmount);
end

Scene1_QuatBike_Computer_0["onMotorForceChanged"] = function(vehicleDrivingManipulation, dt)
    --log("motorForce: " .. motorForce);
    
    vehicleDrivingManipulation:setMotorForce(motorForce);
--    if InputDeviceModule:isActionDown(NOWA_A_UP) then
--        vehicleDrivingManipulation:setMotorForce(5000 * 120 * dt);
--    elseif InputDeviceModule:isActionDown(NOWA_A_ACTION) then
--        vehicleDrivingManipulation:setMotorForce(10000 * 120 * dt);
--    elseif InputDeviceModule:isActionDown(NOWA_A_DOWN) then
--        vehicleDrivingManipulation:setMotorForce(-4500 * 120 * dt);
--    end
end

Scene1_QuatBike_Computer_0["onHandBrakeChanged"] = function(vehicleDrivingManipulation, dt)
    -- Jump: Space
--    if InputDeviceModule:isActionDown(NOWA_A_JUMP) then
--        vehicleDrivingManipulation:setHandBrake(5.5);
--        if (physicsActiveVehicleComponent:getVelocity():squaredLength() > 100) then
--            skidSound:setActivated(true);
--        end
--    end
end

Scene1_QuatBike_Computer_0["onBrakeChanged"] = function(vehicleDrivingManipulation, dt)
    -- Cover: x
--    if InputDeviceModule:isActionDown(NOWA_A_COWER) then
--        vehicleDrivingManipulation:setBrake(7.5);
--        if (physicsActiveVehicleComponent:getVelocity():squaredLength() > 100) then
--            skidSound:setActivated(true);
--        end
--    end
end

Scene1_QuatBike_Computer_0["onTireContact"] = function(tireName, hitPhysicsComponent, contactPosition, contactNormal, penetration)
     if (physicsActiveVehicleComponent:getVelocity():squaredLength() > 40 or contactNormal.z >= -0.85) then
        local soundComponent = hitPhysicsComponent:getOwner():getSimpleSoundComponent();
        --log("onTireContact " .. hitPhysicsComponent:getOwner():getName() .. " pos: " .. toString(contactPosition) .. " normal: " .. toString(contactNormal) .. " penetration: " .. penetration);
        if (soundComponent ~= nil) then
            soundComponent:setActivated(true);
        end
    end
end