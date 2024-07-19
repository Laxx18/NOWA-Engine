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
racePositionText = nil;
countdownText = nil;
rollSound = nil;
skidSound = nil;
playerController = nil;
animationBlender = nil;
height = 0;

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
    racePositionText = mainGameObject:getMyGUITextComponentFromName("RacePositionText");
    countdownText = mainGameObject:getMyGUITextComponentFromName("CountdownText");
    countdownText:setActivated(true);
    
    rollSound = gameObject:getSimpleSoundComponent2(0);
    rollSound:setVolume(200);
    
    skidSound = gameObject:getSimpleSoundComponent2(1);
    
    playerController = gameObject:getPlayerControllerComponent();
    animationBlender = playerController:getAnimationBlender();
    animationBlender:registerAnimation(AnimationBlender.ANIM_IDLE_1, "idle_01");
    animationBlender:registerAnimation(AnimationBlender.ANIM_IDLE_2, "idle_02");
    animationBlender:registerAnimation(AnimationBlender.ANIM_IDLE_3, "idle_03");
    animationBlender:registerAnimation(AnimationBlender.ANIM_WALK_NORTH, "drive");
    animationBlender:registerAnimation(AnimationBlender.ANIM_WALK_SOUTH, "Walk_backwards");
    animationBlender:registerAnimation(AnimationBlender.ANIM_WALK_WEST, "drive_turn_left");
    animationBlender:registerAnimation(AnimationBlender.ANIM_WALK_EAST, "drive_turn_right");
    animationBlender:registerAnimation(AnimationBlender.ANIM_JUMP_START, "jump_up");
    --animationBlender:registerAnimation(AnimationBlender.ANIM_JUMP_WALK, "Jump");
    animationBlender:registerAnimation(AnimationBlender.ANIM_HIGH_JUMP_END, "salto");
    animationBlender:registerAnimation(AnimationBlender.ANIM_JUMP_END, "jump_down");
    --animationBlender:registerAnimation(AnimationBlender.ANIM_FALL, "Falling");
    animationBlender:registerAnimation(AnimationBlender.ANIM_RUN, "drive_fast");
    --animationBlender:registerAnimation(AnimationBlender.ANIM_SNEAK, "Take_damage");
    animationBlender:registerAnimation(AnimationBlender.ANIM_DUCK, "360");
    animationBlender:registerAnimation(AnimationBlender.ANIM_HALT, "drive_obstacle_01");
    animationBlender:registerAnimation(AnimationBlender.ANIM_NO_IDEA, "drive_kick_left");
    animationBlender:registerAnimation(AnimationBlender.ANIM_PICKUP_1, "drive_kick_right");
    animationBlender:registerAnimation(AnimationBlender.ANIM_ACTION_1, "drive_trick_02");
    animationBlender:registerAnimation(AnimationBlender.ANIM_ACTION_2, "drive_trick_03");
    animationBlender:registerAnimation(AnimationBlender.ANIM_ACTION_3, "drive_trick_04");
    animationBlender:registerAnimation(AnimationBlender.ANIM_ACTION_4, "drive_trick_05");
    
    animationBlender:registerAnimation(AnimationBlender.ANIM_ATTACK_1, "drive_trick_06");
    animationBlender:registerAnimation(AnimationBlender.ANIM_ATTACK_2, "drive_trick_07");
    animationBlender:registerAnimation(AnimationBlender.ANIM_ATTACK_3, "drive_trick_08");
    animationBlender:registerAnimation(AnimationBlender.ANIM_ATTACK_4, "drive_trick_09");
    
    animationBlender:init1(AnimationBlender.ANIM_IDLE_1, true);
end

Scene1_QuatBike_0_0["disconnect"] = function()
    
end

Scene1_QuatBike_0_0["update"] = function(dt)
    
    height = playerController:getHeight();
    --log("-->height: " .. height);
    -- ANIM_ACTION_4
    if (height > 1.5) then
        animationBlender:blendExclusive5(AnimationBlender.ANIM_ACTION_4, AnimationBlender.BLEND_WHILE_ANIMATING, 0.1, false);
    end
    
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
    if (raceGoalComponent:getRacingPosition() ~= -1) then
        racePositionText:setCaption("Racing position: " .. raceGoalComponent:getRacingPosition());
    end
    
    -- F for wheelie
    if InputDeviceModule:isActionDown(NOWA_A_FLASH_LIGHT) then
        physicsActiveVehicleComponent:applyOmegaForce(physicsActiveVehicleComponent:getOrientation():Inverse() * Vector3(0, 0, 2));
    end
    
    -- Drift jump (Space)
    --if InputDeviceModule:isActionPressed(NOWA_A_JUMP, dt, 0.2) then
    if InputDeviceModule:isActionDownAmount(NOWA_A_JUMP, dt, 0.2) then
        physicsActiveVehicleComponent:applyForce(Vector3(0, 50000, 0));
        
        if InputDeviceModule:isActionDown(NOWA_A_LEFT) then
            physicsActiveVehicleComponent:applyOmegaForce(Vector3(0, 2, 0));
        elseif InputDeviceModule:isActionDown(NOWA_A_RIGHT) then
            physicsActiveVehicleComponent:applyOmegaForce(Vector3(0, -2, 0));
        end
        
        if (animationBlender:isAnimationActive(AnimationBlender.ANIM_JUMP_START) == false) then
            animationBlender:blend5(AnimationBlender.ANIM_JUMP_START, AnimationBlender.BLEND_WHILE_ANIMATING, 0.5, false);
        end
    end
    
    if (animationBlender:isAnimationActive() == false) then
        if (animationBlender:isAnimationActive(AnimationBlender.ANIM_IDLE_1) == false) then
            animationBlender:blendExclusive1(AnimationBlender.ANIM_IDLE_1, AnimationBlender.BLEND_WHILE_ANIMATING);
        end
    end
    
    animationBlender:addTime(dt * 1 / animationBlender:getLength());
end


Scene1_QuatBike_0_0["onSteeringAngleChanged"] = function(vehicleDrivingManipulation, dt)
    
    if InputDeviceModule:isActionDown(NOWA_A_LEFT) then
        if (steerAmount <= 35) then
            steerAmount = steerAmount + dt * 20;
            
            if (animationBlender:isAnimationActive(AnimationBlender.ANIM_WALK_WEST) == false) then
                animationBlender:blend5(AnimationBlender.ANIM_WALK_WEST, AnimationBlender.BLEND_WHILE_ANIMATING, 0.5, false);
            end
            
        end
        
        --vehicleDrivingManipulation:setSteerAngle(steerAmount);
    elseif InputDeviceModule:isActionDown(NOWA_A_RIGHT) then
        if (steerAmount >= -35) then
            steerAmount = steerAmount - dt * 20;
            
            if (animationBlender:isAnimationActive(AnimationBlender.ANIM_WALK_EAST) == false) then
                animationBlender:blend5(AnimationBlender.ANIM_WALK_EAST, AnimationBlender.BLEND_WHILE_ANIMATING, 0.5, false);
            end
            
        end
        
        --vehicleDrivingManipulation:setSteerAngle(steerAmount);
    else
        if (steerAmount > 0) then
            steerAmount = steerAmount - dt * 30;
            --vehicleDrivingManipulation:setSteerAngle(steerAmount);
        elseif (steerAmount < 0) then
            steerAmount = steerAmount + dt * 30;
            --vehicleDrivingManipulation:setSteerAngle(steerAmount);
        end
    end
    
    vehicleDrivingManipulation:setSteerAngle(steerAmount);
end

Scene1_QuatBike_0_0["onMotorForceChanged"] = function(vehicleDrivingManipulation, dt)
    
    if InputDeviceModule:isActionDown(NOWA_A_UP) then
        vehicleDrivingManipulation:setMotorForce(5000 * 120 * dt);
        if (animationBlender:isAnimationActive(AnimationBlender.ANIM_WALK_NORTH) == false) then
			animationBlender:blend5(AnimationBlender.ANIM_WALK_NORTH, AnimationBlender.BLEND_WHILE_ANIMATING, 0.5, false);
		end
        -- Key E
    elseif InputDeviceModule:isActionDown(NOWA_A_ACTION) then
        vehicleDrivingManipulation:setMotorForce(10000 * 120 * dt);
        if (animationBlender:isAnimationActive(AnimationBlender.ANIM_RUN) == false) then
			animationBlender:blend5(AnimationBlender.ANIM_RUN, AnimationBlender.BLEND_WHILE_ANIMATING, 0.5, false);
		end
    elseif InputDeviceModule:isActionDown(NOWA_A_DOWN) then
        vehicleDrivingManipulation:setMotorForce(-4500 * 120 * dt);
    end
end

Scene1_QuatBike_0_0["onHandBrakeChanged"] = function(vehicleDrivingManipulation, dt)
    
    -- Jump: Space
    --if InputDeviceModule:isActionDown(NOWA_A_JUMP) then
    --    vehicleDrivingManipulation:setHandBrake(5.5);
    --    if (physicsActiveVehicleComponent:getVelocity():squaredLength() > 100) then
    --        skidSound:setActivated(true);
    --    end
    --end
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

Scene1_QuatBike_0_0["onFeedbackRace"] = function(currentLap, lapTimeSec, finished)
    speedText:setCaption("Lap: " .. currentLap);
    lapTimeList:setItemText(currentLap - 1, "Lap: " .. currentLap .. " Elapsed time: " .. lapTimeSec .. " seconds");
    finishedText:setActivated(finished);
    lapText:setCaption("Lap: " .. currentLap);
end

Scene1_QuatBike_0_0["onWrongDirectionDriving"] = function(wrongDirection)
    wrongDirectionText:setActivated(wrongDirection == true);
end

Scene1_QuatBike_0_0["onCountdown"] = function(countdownNumber)
    
    if (countdownNumber > 0) then
        countdownText:setCaption(" " .. countdownNumber);
    else
        countdownText:setActivated(false);
    end
end