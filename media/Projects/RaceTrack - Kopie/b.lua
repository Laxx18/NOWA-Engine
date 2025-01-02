module("b", package.seeall);

b = {}

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
playerController = nil;
animationBlender = nil;
height = 0;

b["connect"] = function(gameObject)
    --AppStateManager:getGameObjectController():activatePlayerController(true, gameObject:getId(), true);
    physicsActiveVehicleComponent = gameObject:getPhysicsActiveVehicleComponent();
    purePursuitComponent = gameObject:getPurePursuitComponent();
    rollSound = gameObject:getSimpleSoundComponentFromIndex(0);
    rollSound:setVolume(200);
    
    skidSound = gameObject:getSimpleSoundComponentFromIndex(1);
    
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
    animationBlender:registerAnimation(AnimationBlender.ANIM_ACTION_1, "drive_trick_05");
    animationBlender:registerAnimation(AnimationBlender.ANIM_ATTACK_1, "drive_trick_02");
    animationBlender:registerAnimation(AnimationBlender.ANIM_ATTACK_2, "drive_trick_03");
    animationBlender:registerAnimation(AnimationBlender.ANIM_ATTACK_3, "drive_trick_04");
    
    animationBlender:init1(AnimationBlender.ANIM_IDLE_1, true);
end

b["disconnect"] = function()
    AppStateManager:getGameObjectController():undoAll();
end

b["update"] = function(dt)
    height = playerController:getHeight();
    
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
    
    --physicsActiveVehicleComponent:applyOmegaForceRotateTo(Quaternion(Degree(pitchAmount), Vector3(0, 0, 1)), Vector3(0, 0, 1), 1);
    
    animationBlender:addTime(dt * 1 / animationBlender:getLength());
end


b["onSteeringAngleChanged"] = function(vehicleDrivingManipulation, dt)
    
    --log("steerAmountBefore: " .. steerAmount);
--    if (steerAmount > 45) then
--        steerAmount = 45;
--    elseif (steerAmount < -45) then
--            steerAmount = 45;
--    end
    
    --log("steerAmount: " .. steerAmount);
    --log("pitchAmount: " .. pitchAmount);
    

    vehicleDrivingManipulation:setSteerAngle(steerAmount);
    if (steerAmount > 0) then
       if (animationBlender:isAnimationActive(AnimationBlender.ANIM_WALK_WEST) == false) then
            animationBlender:blend5(AnimationBlender.ANIM_WALK_WEST, AnimationBlender.BLEND_WHILE_ANIMATING, 0.5, false);
       end
    elseif (steerAmount < 0) then
        if (animationBlender:isAnimationActive(AnimationBlender.ANIM_WALK_EAST) == false) then
            animationBlender:blend5(AnimationBlender.ANIM_WALK_EAST, AnimationBlender.BLEND_WHILE_ANIMATING, 0.5, false);
       end
    else
        if (animationBlender:isAnimationActive(AnimationBlender.ANIM_WALK_NORTH) == false) then
			animationBlender:blend5(AnimationBlender.ANIM_WALK_NORTH, AnimationBlender.BLEND_WHILE_ANIMATING, 0.5, false);
		end
    end
end

b["onMotorForceChanged"] = function(vehicleDrivingManipulation, dt)
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

b["onHandBrakeChanged"] = function(vehicleDrivingManipulation, dt)
    -- Jump: Space
--    if InputDeviceModule:isActionDown(NOWA_A_JUMP) then
--        vehicleDrivingManipulation:setHandBrake(5.5);
--        if (physicsActiveVehicleComponent:getVelocity():squaredLength() > 100) then
--            skidSound:setActivated(true);
--        end
--    end
end

b["onBrakeChanged"] = function(vehicleDrivingManipulation, dt)
    -- Cover: x
--    if InputDeviceModule:isActionDown(NOWA_A_COWER) then
--        vehicleDrivingManipulation:setBrake(7.5);
--        if (physicsActiveVehicleComponent:getVelocity():squaredLength() > 100) then
--            skidSound:setActivated(true);
--        end
--    end
end

b["onTireContact"] = function(tireName, hitPhysicsComponent, contactPosition, contactNormal, penetration)
     if (physicsActiveVehicleComponent:getVelocity():squaredLength() > 40 or contactNormal.z >= -0.85) then
        local soundComponent = hitPhysicsComponent:getOwner():getSimpleSoundComponent();
        --log("onTireContact " .. hitPhysicsComponent:getOwner():getName() .. " pos: " .. toString(contactPosition) .. " normal: " .. toString(contactNormal) .. " penetration: " .. penetration);
        if (soundComponent ~= nil) then
            soundComponent:setActivated(true);
        end
    end
end