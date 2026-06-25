module("Viper_0", package.seeall);
-- Scene: Scene1

require("init");

viper_0 = nil

Viper_0 = {}

require("init");

steerAmount = 0;
physicsActiveVehicleComponent = nil;
rollSound = nil;
skidSound = nil;
player = nil;
inputDeviceComponent = nil;
inputDeviceModule = nil;

Viper_0["connect"] = function(gameObject)
    player = AppStateManager:getGameObjectController():castGameObject(gameObject);
    --AppStateManager:getGameObjectController():activatePlayerController(true, gameObject:getId(), true);
    physicsActiveVehicleComponent = gameObject:getPhysicsActiveVehicleComponentV2();
    --rollSound = gameObject:getSimpleSoundComponentFromIndex(0);
    --rollSound:setVolume(200);
    
    --skidSound = player:getSimpleSoundComponentFromIndex(1);
    inputDeviceComponent = player:getInputDeviceComponent();
    inputDeviceModule = inputDeviceComponent:getInputDeviceModule();
   
end

Viper_0["disconnect"] = function()
    --AppStateManager:getGameObjectController():undoAll();
end

Viper_0["update"] = function(dt)
    if (inputDeviceComponent:isDeviceLocked()) then
       return;
    end

    -- log("inAir: " .. toString(physicsActiveVehicleComponent:isAirborne()));
    --log("vel: " .. toString(physicsActiveVehicleComponent:getVelocity():squaredLength()));
    local motion = physicsActiveVehicleComponent:getVelocity():squaredLength() + 40;
    if motion > 40 + (0.2 * 0.2) then
        --rollSound:setActivated(true);
        -- If started to drive, motor engine sound shall remain in a deep voice
        if (motion < 40) then
            motion = 40;
        end
        --rollSound:setPitch(motion * 0.01);
    --else
    --    rollSound:setActivated(false);
    end
end


Viper_0["onSteeringAngleChanged"] = function(vehicleDrivingManipulation, dt)
    if (inputDeviceComponent:isDeviceLocked()) then
       return;
    end
    
    inputDeviceModule = inputDeviceComponent:getInputDeviceModule();
    
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

Viper_0["onMotorForceChanged"] = function(vehicleDrivingManipulation, dt)
    if (inputDeviceComponent:isDeviceLocked()) then
       return;
    end
    
    if inputDeviceModule:isActionDown(NOWA_A_UP) then
        vehicleDrivingManipulation:setMotorForce(500 * 120 * dt);
    elseif inputDeviceModule:isActionDown(NOWA_A_DOWN) then
        vehicleDrivingManipulation:setMotorForce(-250 * 120 * dt);
    end
    
     if inputDeviceModule:isActionDown(NOWA_A_ACTION) then
         physicsActiveVehicleComponent:applyPitch(-100, dt);
     end
end

Viper_0["onHandBrakeChanged"] = function(vehicleDrivingManipulation, dt)
     if (inputDeviceComponent:isDeviceLocked()) then
       return;
    end
    
    -- Jump: Space
    if inputDeviceModule:isActionDown(NOWA_A_JUMP) then
        vehicleDrivingManipulation:setHandBrake(5.5);
        if (physicsActiveVehicleComponent:getVelocity():squaredLength() > 100) then
            --skidSound:setActivated(true);
        end
    end
end

Viper_0["onBrakeChanged"] = function(vehicleDrivingManipulation, dt)
    if (inputDeviceComponent:isDeviceLocked()) then
       return;
    end
    
    -- Cover: x
    if inputDeviceModule:isActionDown(NOWA_A_COWER) then
        vehicleDrivingManipulation:setBrake(7.5);
        if (physicsActiveVehicleComponent:getVelocity():squaredLength() > 100) then
            --skidSound:setActivated(true);
        end
    end
end