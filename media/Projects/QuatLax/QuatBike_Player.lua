module("QuatBike_Player", package.seeall);

QuatBike_Player = {}

-- Scene: Scene1

require("init");

player = nil
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
inputDeviceModule = nil;

QuatBike_Player["connect"] = function(gameObject)
    player = AppStateManager:getGameObjectController():castGameObject(gameObject);
    
    physicsActiveVehicleComponent = player:getPhysicsActiveVehicleComponent();
    raceGoalComponent = player:getRaceGoalComponent();
    
    
    inputDeviceModule = player:getInputDeviceComponent():getInputDeviceModule();
    log("---->Player: " .. player:getName());
    
    --local mainGameObject = AppStateManager:getGameObjectController():getGameObjectFromId(MAIN_GAMEOBJECT_ID);
    --speedText = mainGameObject:getMyGUITextComponentFromName("SpeedText");
    --lapText = mainGameObject:getMyGUITextComponentFromName("LapText");
    --lapTimeList = mainGameObject:getMyGUIListBoxComponentFromName("LapTimeList");
    --lapTimeList:setItemCount(raceGoalComponent:getLapsCount());
    --wrongDirectionText = mainGameObject:getMyGUITextComponentFromName("WrongDirectionText");
    --finishedText = mainGameObject:getMyGUITextComponentFromName("FinishedText");
    --racePositionText = mainGameObject:getMyGUITextComponentFromName("RacePositionText");
    --countdownText = mainGameObject:getMyGUITextComponentFromName("CountdownText");
    --countdownText:setActivated(true);
    
    rollSound = player:getSimpleSoundComponentFromIndex(0);
    rollSound:setVolume(200);
    
    skidSound = player:getSimpleSoundComponentFromIndex(1);
    
    playerController = player:getPlayerControllerComponent();
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

QuatBike_Player["disconnect"] = function()
    
end

QuatBike_Player["update"] = function(dt)
    local height = playerController:getHeight();
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
    --    rollSound:setActivated(false);
    end

    --speedText:setCaption("Speed: " .. raceGoalComponent:getSpeedInKmh() .. "km/h");
    --if (raceGoalComponent:getRacingPosition() ~= -1) then
       --racePositionText:setCaption("Racing position: " .. raceGoalComponent:getRacingPosition());
    --end
    
    
    
    -- F for wheelie
    if inputDeviceModule:isActionDown(NOWA_A_FLASH_LIGHT) then
        physicsActiveVehicleComponent:applyWheelie(2);
    end
    
    -- Drift jump (Space)
    --if inputDeviceModule:isActionPressed(NOWA_A_JUMP, dt, 0.2) then
    if inputDeviceModule:isActionDownAmount(NOWA_A_JUMP, dt, 0.2) then

        if inputDeviceModule:isActionDown(NOWA_A_LEFT) then
            physicsActiveVehicleComponent:applyDrift(true, 40000, 2);
            
        elseif inputDeviceModule:isActionDown(NOWA_A_RIGHT) then
            physicsActiveVehicleComponent:applyDrift(false, 40000, 2);
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


QuatBike_Player["onSteeringAngleChanged"] = function(vehicleDrivingManipulation, dt)
    
    if inputDeviceModule:isActionDown(NOWA_A_LEFT) then
        if (steerAmount <= 35) then
            steerAmount = steerAmount + dt * 20;
            
            if (animationBlender:isAnimationActive(AnimationBlender.ANIM_WALK_WEST) == false) then
                animationBlender:blend5(AnimationBlender.ANIM_WALK_WEST, AnimationBlender.BLEND_WHILE_ANIMATING, 0.5, false);
            end
            
        end
        
        --vehicleDrivingManipulation:setSteerAngle(steerAmount);
    elseif inputDeviceModule:isActionDown(NOWA_A_RIGHT) then
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

QuatBike_Player["onMotorForceChanged"] = function(vehicleDrivingManipulation, dt)
    
    if inputDeviceModule:isActionDown(NOWA_A_UP) then
        vehicleDrivingManipulation:setMotorForce(5000 * 120 * dt);
        if (animationBlender:isAnimationActive(AnimationBlender.ANIM_WALK_NORTH) == false) then
            animationBlender:blend5(AnimationBlender.ANIM_WALK_NORTH, AnimationBlender.BLEND_WHILE_ANIMATING, 0.5, false);
        end
        -- Key E
    elseif inputDeviceModule:isActionDown(NOWA_A_ACTION) then
        vehicleDrivingManipulation:setMotorForce(10000 * 120 * dt);
        if (animationBlender:isAnimationActive(AnimationBlender.ANIM_RUN) == false) then
            animationBlender:blend5(AnimationBlender.ANIM_RUN, AnimationBlender.BLEND_WHILE_ANIMATING, 0.5, false);
        end
    elseif inputDeviceModule:isActionDown(NOWA_A_DOWN) then
        vehicleDrivingManipulation:setMotorForce(-4500 * 120 * dt);
    end
end

QuatBike_Player["onHandBrakeChanged"] = function(vehicleDrivingManipulation, dt)
    
    -- Jump: Space
    --if inputDeviceModule:isActionDown(NOWA_A_JUMP) then
    --    vehicleDrivingManipulation:setHandBrake(5.5);
    --    if (physicsActiveVehicleComponent:getVelocity():squaredLength() > 100) then
    --        skidSound:setActivated(true);
    --    end
    --end
end

QuatBike_Player["onBrakeChanged"] = function(vehicleDrivingManipulation, dt)
    
    -- Cover: x
    if inputDeviceModule:isActionDown(NOWA_A_COWER) then
        vehicleDrivingManipulation:setBrake(7.5);
        if (physicsActiveVehicleComponent:getVelocity():squaredLength() > 100) then
            skidSound:setActivated(true);
        end
    end
end

QuatBike_Player["onTireContact"] = function(tireName, hitPhysicsComponent, contactPosition, contactNormal, penetration)
    
    if (physicsActiveVehicleComponent:getVelocity():squaredLength() > 40 or contactNormal.z >= -0.85) then
        local soundComponent = hitPhysicsComponent:getOwner():getSimpleSoundComponent();
        --log("onTireContact " .. hitPhysicsComponent:getOwner():getName() .. " pos: " .. toString(contactPosition) .. " normal: " .. toString(contactNormal) .. " penetration: " .. penetration);
        if (soundComponent ~= nil) then
            soundComponent:setActivated(true);
        end
    end
end

QuatBike_Player["onFeedbackRace"] = function(currentLap, lapTimeSec, finished)
    speedText:setCaption("Lap: " .. currentLap);
    lapTimeList:setItemText(currentLap - 1, "Lap: " .. currentLap .. " Elapsed time: " .. lapTimeSec .. " seconds");
    finishedText:setActivated(finished);
    lapText:setCaption("Lap: " .. currentLap);
end

QuatBike_Player["onWrongDirectionDriving"] = function(wrongDirection)
    wrongDirectionText:setActivated(wrongDirection == true);
end

QuatBike_Player["onCountdown"] = function(countdownNumber)
    
    if (countdownNumber > 0) then
        countdownText:setCaption(" " .. countdownNumber);
    else
        countdownText:setActivated(false);
    end
end