module("Scene1_QuatBike_0", package.seeall);

Scene1_QuatBike_0 = {}

-- Scene: Scene1

require("init");

scene1_QuatBike_0 = nil
steerAmount = 0;
physicsActiveVehicleComponent = nil;
rollSound = nil;
skidSound = nil;

Scene1_QuatBike_0["connect"] = function(gameObject)
    AppStateManager:getGameObjectController():activatePlayerController(true, gameObject:getId(), true);
    physicsActiveVehicleComponent = gameObject:getPhysicsActiveVehicleComponent();
    rollSound = gameObject:getSimpleSoundComponentFromIndex(0);
    rollSound:setVolume(200);
    
    skidSound = gameObject:getSimpleSoundComponentFromIndex(1);
end

Scene1_QuatBike_0["disconnect"] = function()
    AppStateManager:getGameObjectController():undoAll();
end

Scene1_QuatBike_0["update"] = function(dt)
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
end


Scene1_QuatBike_0["onSteeringAngleChanged"] = function(vehicleDrivingManipulation, dt)
    if InputDeviceModule:isActionDown(NOWA_A_LEFT) then
        if (steerAmount <= 45) then
            steerAmount = steerAmount + dt * 30;
        end
        vehicleDrivingManipulation:setSteerAngle(steerAmount);
    elseif InputDeviceModule:isActionDown(NOWA_A_RIGHT) then
        if (steerAmount >= -45) then
            steerAmount = steerAmount - dt * 30;
        end
        
        --var heading = physicsActiveVehicleComponent:getOrientation().getYaw(false).valueRadians();
        --var pitch = physicsActiveVehicleComponent:getOrientation().getPitch(false).valueRadians();
            
    vehicleDrivingManipulation:setSteerAngle(steerAmount);
    else
        if (steerAmount > 0) then
            steerAmount = steerAmount - dt * 30;
            vehicleDrivingManipulation:setSteerAngle(steerAmount);
        elseif (steerAmount < 0) then
            steerAmount = steerAmount + dt * 30;
            vehicleDrivingManipulation:setSteerAngle(steerAmount);
        end
    end
end

Scene1_QuatBike_0["onMotorForceChanged"] = function(vehicleDrivingManipulation, dt)
    if InputDeviceModule:isActionDown(NOWA_A_UP) then
        vehicleDrivingManipulation:setMotorForce(5000 * 120 * dt);
    elseif InputDeviceModule:isActionDown(NOWA_A_ACTION) then
        vehicleDrivingManipulation:setMotorForce(10000 * 120 * dt);
    elseif InputDeviceModule:isActionDown(NOWA_A_DOWN) then
        vehicleDrivingManipulation:setMotorForce(-4500 * 120 * dt);
    end
end

Scene1_QuatBike_0["onHandBrakeChanged"] = function(vehicleDrivingManipulation, dt)
    -- Jump: Space
    if InputDeviceModule:isActionDown(NOWA_A_JUMP) then
        vehicleDrivingManipulation:setHandBrake(5.5);
        if (physicsActiveVehicleComponent:getVelocity():squaredLength() > 100) then
            skidSound:setActivated(true);
        end
    end
end

Scene1_QuatBike_0["onBrakeChanged"] = function(vehicleDrivingManipulation, dt)
    -- Cover: x
    if InputDeviceModule:isActionDown(NOWA_A_COWER) then
        vehicleDrivingManipulation:setBrake(7.5);
        if (physicsActiveVehicleComponent:getVelocity():squaredLength() > 100) then
            skidSound:setActivated(true);
        end
    end
end

Scene1_QuatBike_0["onTireContact"] = function(tireName, hitPhysicsComponent, contactPosition, contactNormal, penetration)
    if (physicsActiveVehicleComponent:getVelocity():squaredLength() > 40) then
        local soundComponent = hitPhysicsComponent:getOwner():getSimpleSoundComponent();
        --log("onTireContact " .. hitPhysicsComponent:getOwner():getName() .. " pos: " .. toString(contactPosition) .. " normal: " .. toString(contactNormal) .. " penetration: " .. penetration);
        if (soundComponent ~= nil) then
            soundComponent:setActivated(true);
        end
    end
end