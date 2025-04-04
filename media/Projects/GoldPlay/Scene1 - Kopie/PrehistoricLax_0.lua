module("PrehistoricLax_0", package.seeall);
-- Scene: Scene1

require("init");

prehistoricLax_0 = nil
pathFollowComponent = nil;
inputDeviceModule = nil;
animationComponent = nil;
animationBlender = nil;
waypointReached = false;
physicsActiveComponent = nil;

PrehistoricLax_0 = {}

PrehistoricLax_0["connect"] = function(gameObject)
    prehistoricLax_0 = AppStateManager:getGameObjectController():castGameObject(gameObject);
    physicsActiveComponent = prehistoricLax_0:getPhysicsPlayerControllerComponent();
    pathFollowComponent = prehistoricLax_0:getAiPathFollowComponent();
    local inputDeviceComponent = prehistoricLax_0:getInputDeviceComponent();
    inputDeviceModule = inputDeviceComponent:getInputDeviceModule();
    animationComponent = prehistoricLax_0:getAnimationComponent();
    animationBlender = animationComponent:getAnimationBlender();
    
    animationBlender:registerAnimation(AnimID.ANIM_IDLE_1, "Idle");
    animationBlender:registerAnimation(AnimID.ANIM_IDLE_2, "Greet");
    animationBlender:registerAnimation(AnimID.ANIM_IDLE_3, "Bored");
    animationBlender:registerAnimation(AnimID.ANIM_WALK_NORTH, "Walk");
    animationBlender:registerAnimation(AnimID.ANIM_WALK_SOUTH, "Walk_backwards");
    animationBlender:registerAnimation(AnimID.ANIM_WALK_WEST, "Walk");
    animationBlender:registerAnimation(AnimID.ANIM_WALK_EAST, "Walk");
    animationBlender:registerAnimation(AnimID.ANIM_JUMP_START, "Jump3");
    animationBlender:registerAnimation(AnimID.ANIM_JUMP_WALK, "Jump");
    animationBlender:registerAnimation(AnimID.ANIM_HIGH_JUMP_END, "Land");
    animationBlender:registerAnimation(AnimID.ANIM_JUMP_END, "Land2");
    animationBlender:registerAnimation(AnimID.ANIM_FALL, "Falling");
    animationBlender:registerAnimation(AnimID.ANIM_RUN, "Run");
    animationBlender:registerAnimation(AnimID.ANIM_SNEAK, "Take_damage");
    animationBlender:registerAnimation(AnimID.ANIM_DUCK, "Land2");
    animationBlender:registerAnimation(AnimID.ANIM_HALT, "Halt");
    animationBlender:registerAnimation(AnimID.ANIM_NO_IDEA, "Roar");
    animationBlender:registerAnimation(AnimID.ANIM_PICKUP_1, "Pickup2");
    animationBlender:registerAnimation(AnimID.ANIM_ACTION_1, "Knockdown");
    animationBlender:registerAnimation(AnimID.ANIM_ATTACK_1, "Getup");
    animationBlender:registerAnimation(AnimID.ANIM_ATTACK_2, "Melee_attack2");
    animationBlender:registerAnimation(AnimID.ANIM_ATTACK_3, "Melee_attack3");
     
    animationBlender:init1(AnimID.ANIM_WALK_NORTH, true);
    
    --animationComponent:setActivated(true);
    
    pathFollowComponent:reactOnPathGoalReached(function()
        animationBlender:blend5(AnimID.ANIM_IDLE_1, BlendingTransition.BLEND_WHILE_ANIMATING, 0.1, true);
        waypointReached = true;
    end);
end

PrehistoricLax_0["disconnect"] = function()
    AppStateManager:getGameObjectController():undoAll();
end

PrehistoricLax_0["update"] = function(dt)

    if (inputDeviceModule:isActionDown(NOWA_A_LEFT)) then
        if (pathFollowComponent:getWaypointId(0) ~= "1378398386") then
            pathFollowComponent:setWaypointId(0, "1378398386");
            pathFollowComponent:setActivated(true);
            pathFollowComponent:setAnimationSpeed(2);
            pathFollowComponent:setAutoAnimation(true);
            animationBlender:blend5(AnimID.ANIM_WALK_NORTH, BlendingTransition.BLEND_WHILE_ANIMATING, 0.1, true);
            waypointReached = false;
        end
    elseif (inputDeviceModule:isActionDown(NOWA_A_RIGHT)) then
        if (pathFollowComponent:getWaypointId(0) ~= "1091560280") then
            pathFollowComponent:setWaypointId(0, "1091560280");
            pathFollowComponent:setActivated(true);
           pathFollowComponent:setAnimationSpeed(2);
            pathFollowComponent:setAutoAnimation(true);
            animationBlender:blend5(AnimID.ANIM_WALK_NORTH, BlendingTransition.BLEND_WHILE_ANIMATING, 0.1, true);
            waypointReached = false;
        end
     elseif (inputDeviceModule:isActionDown(NOWA_A_ACTION)) then
        if (waypointReached) then
            local resultOrientation = MathHelper:faceDirectionSlerp(prehistoricLax_0:getOrientation(), Vector3.NEGATIVE_UNIT_X, prehistoricLax_0:getDefaultDirection(), dt, 600);
            local heading = resultOrientation:getYaw(true);
            physicsActiveComponent:move(0, 0, heading);
            pathFollowComponent:setAnimationSpeed(4);
            animationBlender:blend5(AnimID.ANIM_PICKUP_1, BlendingTransition.BLEND_WHILE_ANIMATING, 0.1, true);
            
        end
    end
end