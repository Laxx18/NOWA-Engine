module("Scene1_QuatBike_0_0", package.seeall);

Scene1_QuatBike_0_0 = {}

-- Scene: Scene1

require("init");

scene1_QuatBike_0 = nil
steerAmount = 0;
physicsActiveVehicleComponent = nil;
raceGoalComponent = nil;
speedText = nil;
lapText = nil;
lapTimeList = nil;
wrongDirectionText = nil;
finishedText = nil;
rollSound = nil;
skidSound = nil;

Scene1_QuatBike_0_0["connect"] = function(gameObject)
    AppStateManager:getGameObjectController():activatePlayerController(true, gameObject:getId(), true);
    physicsActiveVehicleComponent = gameObject:getPhysicsActiveVehicleComponent();
    raceGoalComponent = gameObject:getRaceGoalComponent();
    
    local mainGameObject = AppStateManager:getGameObjectController():getGameObjectFromId(MAIN_GAMEOBJECT_ID);
    speedText = mainGameObject:getMyGUITextComponentFromName("SpeedText");
    lapText = mainGameObject:getMyGUITextComponentFromName("LapText");
    lapTimeList = mainGameObject:getMyGUIListBoxComponentFromName("LapTimeList");
    lapTimeList:setItemCount(raceGoalComponent:getLapsCount());
    wrongDirectionText = mainGameObject:getMyGUITextComponentFromName("WrongDirectionText");
    finishedText = mainGameObject:getMyGUITextComponentFromName("FinishedText");
    
    rollSound = gameObject:getSimpleSoundComponent2(0);
    rollSound:setVolume(200);
    
    skidSound = gameObject:getSimpleSoundComponent2(1);
end

Scene1_QuatBike_0_0["disconnect"] = function()
    
end

Scene1_QuatBike_0_0["update"] = function(dt)
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

    speedText:setCaption("Speed: " .. raceGoalComponent:getSpeedInKmh() .. "km/h");
end


Scene1_QuatBike_0_0["onSteeringAngleChanged"] = function(vehicleDrivingManipulation, dt)
    if InputDeviceModule:isActionDown(NOWA_A_LEFT) then
        if (steerAmount <= 35) then
            steerAmount = steerAmount + dt * 20;
        end
        vehicleDrivingManipulation:setSteerAngle(steerAmount);
    elseif InputDeviceModule:isActionDown(NOWA_A_RIGHT) then
        if (steerAmount >= -35) then
            steerAmount = steerAmount - dt * 20;
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

Scene1_QuatBike_0_0["onMotorForceChanged"] = function(vehicleDrivingManipulation, dt)
    if InputDeviceModule:isActionDown(NOWA_A_UP) then
        vehicleDrivingManipulation:setMotorForce(5000 * 120 * dt);
    elseif InputDeviceModule:isActionDown(NOWA_A_ACTION) then
        vehicleDrivingManipulation:setMotorForce(10000 * 120 * dt);
    elseif InputDeviceModule:isActionDown(NOWA_A_DOWN) then
        vehicleDrivingManipulation:setMotorForce(-4500 * 120 * dt);
    end
end

Scene1_QuatBike_0_0["onHandBrakeChanged"] = function(vehicleDrivingManipulation, dt)
    -- Jump: Space
    if InputDeviceModule:isActionDown(NOWA_A_JUMP) then
        vehicleDrivingManipulation:setHandBrake(5.5);
        if (physicsActiveVehicleComponent:getVelocity():squaredLength() > 100) then
            skidSound:setActivated(true);
        end
    end
end

Scene1_QuatBike_0_0["onBrakeChanged"] = function(vehicleDrivingManipulation, dt)
    -- Cover: x
    if InputDeviceModule:isActionDown(NOWA_A_COWER) then
        vehicleDrivingManipulation:setBrake(7.5);
        if (physicsActiveVehicleComponent:getVelocity():squaredLength() > 100) then
            skidSound:setActivated(true);
        end
    end
end

Scene1_QuatBike_0_0["onTireContact"] = function(tireName, hitPhysicsComponent, contactPosition, contactNormal, penetration)
    
    if (physicsActiveVehicleComponent:getVelocity():squaredLength() > 40 or contactNormal.z >= -0.85) then
        local soundComponent = hitPhysicsComponent:getOwner():getSimpleSoundComponent();
        --log("onTireContact " .. hitPhysicsComponent:getOwner():getName() .. " pos: " .. toString(contactPosition) .. " normal: " .. toString(contactNormal) .. " penetration: " .. penetration);
        if (soundComponent ~= nil) then
            soundComponent:setActivated(true);
        end
    end
end

Scene1_QuatBike_0_0["onFeedbackRace"] = function(wrongDirection, currentLap, lapTimeSec, finished)
    speedText:setCaption("Lap: " .. currentLap);
    lapTimeList:setItemText(currentLap, "Elapsed time: " .. lapTimeSec);
    wrongDirectionText:setActivated(wrongDirection == true);
    finishedText:setActivated(finished);
end