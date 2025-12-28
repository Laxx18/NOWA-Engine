module("Scene1_QuatBike_0", package.seeall);

Scene1_QuatBike_0 = {}

-- Scene: Scene1

require("init");

scene1_QuatBike_0 = nil
steerAmount = 0;
physicsActiveVehicleComponent = nil;
rollSound = nil;
skidSound = nil;
player = nil;
inputDeviceModule = nil;

Scene1_QuatBike_0["connect"] = function(gameObject)
    player = AppStateManager:getGameObjectController():castGameObject(gameObject);
    AppStateManager:getGameObjectController():activatePlayerController(true, gameObject:getId(), true);
    physicsActiveVehicleComponent = gameObject:getPhysicsActiveVehicleComponent();
    rollSound = gameObject:getSimpleSoundComponentFromIndex(0);
    rollSound:setVolume(200);
    
    skidSound = player:getSimpleSoundComponentFromIndex(1);
    
    inputDeviceModule = player:getInputDeviceComponent():getInputDeviceModule();
	
	inputDeviceModule:setAnalogActionThreshold(0.55)
end

Scene1_QuatBike_0["disconnect"] = function()
    AppStateManager:getGameObjectController():undoAll();
end

Scene1_QuatBike_0["update"] = function(dt)
    -- log("inAir: " .. toString(physicsActiveVehicleComponent:isAirborne()));
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
    --    rollSound:setActivated(false);
    end
end


Scene1_QuatBike_0["onSteeringAngleChanged"] = function(vehicleDrivingManipulation, dt)
    local maxAngle = 45.0 -- degrees

    -- unified analog axis from keyboard OR left stick: [-1..1]
    local axis = inputDeviceModule:getSteerAxis()

    -- Optional: clamp (just in case)
    if axis > 1.0 then axis = 1.0 end
    if axis < -1.0 then axis = -1.0 end

    -- Optional: small deadzone on Lua side (extra safety)
    local dz = 0.10
    if math.abs(axis) < dz then
        axis = 0.0
    end

    -- Smooth steering to feel vehicle-like (not twitchy)
    -- Higher response = faster steering, lower = more stable
    local response = 10.0
    local targetAngle = -axis * maxAngle
    steerAmount = steerAmount + (targetAngle - steerAmount) * (1.0 - math.exp(-response * dt))

    vehicleDrivingManipulation:setSteerAngle(steerAmount)
end

Scene1_QuatBike_0["onMotorForceChanged"] = function(vehicleDrivingManipulation, dt)
    if inputDeviceModule:isActionDown(NOWA_A_UP) then
        vehicleDrivingManipulation:setMotorForce(10000 * 120 * dt);
    elseif inputDeviceModule:isActionDown(NOWA_A_DOWN) then
        vehicleDrivingManipulation:setMotorForce(-4500 * 120 * dt);
    end
    
     if inputDeviceModule:isActionDown(NOWA_A_ACTION) then
         physicsActiveVehicleComponent:applyPitch(-100, dt);
     end
end

Scene1_QuatBike_0["onHandBrakeChanged"] = function(vehicleDrivingManipulation, dt)
    -- Jump: Space
    if inputDeviceModule:isActionDown(NOWA_A_JUMP) then
        vehicleDrivingManipulation:setHandBrake(5.5);
        if (physicsActiveVehicleComponent:getVelocity():squaredLength() > 100) then
            skidSound:setActivated(true);
        end
    end
end

Scene1_QuatBike_0["onBrakeChanged"] = function(vehicleDrivingManipulation, dt)
    -- Cover: x
    if inputDeviceModule:isActionDown(NOWA_A_COWER) then
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