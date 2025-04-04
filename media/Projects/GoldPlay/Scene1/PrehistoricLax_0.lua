module("PrehistoricLax_0", package.seeall);
-- Scene: Scene1

require("init");

prehistoricLax_0 = nil
pathFollowComponent = nil;
inputDeviceModule = nil;
playerControllerComponent = nil;
goldText = nil;
goldAttribute = nil;
animationBlender = nil;
waypointReached = false;
pullAction = false;
physicsActiveComponent = nil;
currentSpawnComponent = nil;
attributesComponent = nil;
leftBucket = nil;
rightBucket = nil;
currentBucket = nil;
canPull = true;
gold = nil;
strone = nil;
success = false;
failure = false;
round = 0;

moneyCountSound = nil;
moneyHitSound = nil;
rockHitSound = nil;
laughSound = nil;
sighSound = nil;

goldAnimIsActive = false;
goldAnimStart = 0;
goldAnimTarget = 0;
goldAnimElapsed = 0;
goldAnimDuration = 1;

PrehistoricLax_0 = {}

PrehistoricLax_0["connect"] = function(gameObject)
    prehistoricLax_0 = AppStateManager:getGameObjectController():castGameObject(gameObject);
    physicsActiveComponent = prehistoricLax_0:getPhysicsPlayerControllerComponent();
    pathFollowComponent = prehistoricLax_0:getAiPathFollowComponent();
    local inputDeviceComponent = prehistoricLax_0:getInputDeviceComponent();
    inputDeviceModule = inputDeviceComponent:getInputDeviceModule();
    playerControllerComponent = prehistoricLax_0:getPlayerControllerComponent();
    playerControllerComponent:setActivated(true);
    animationBlender = playerControllerComponent:getAnimationBlender();
    spawnComponent = AppStateManager:getGameObjectController():getGameObjectFromId("2984599155"):getSpawnComponent();
    attributesComponent = prehistoricLax_0:getAttributesComponent();
    
    
    local mainGameObject = AppStateManager:getGameObjectController():getGameObjectFromId(MAIN_GAMEOBJECT_ID);
    goldText = mainGameObject:getMyGUITextComponentFromName("GoldText");
    
    goldAttribute = attributesComponent:getAttributeValueByName("Gold");
    goldText:setCaption("Gold: " .. toString(goldAttribute:getValueNumber()));
    
    gold = AppStateManager:getGameObjectController():getGameObjectFromId("2377376211");
    stone = AppStateManager:getGameObjectController():getGameObjectFromId("1528694117");
    
    leftBucket = AppStateManager:getGameObjectController():getGameObjectFromId("195266620");
    rightBucket = AppStateManager:getGameObjectController():getGameObjectFromId("3578308553");
    currentBucket = leftBucket;
    currentSpawnComponent  = currentBucket:getSpawnComponent();
    
    
    moneyCountSound = prehistoricLax_0:getSimpleSoundComponentFromName("MoneyCountSound");
    moneyHitSound = prehistoricLax_0:getSimpleSoundComponentFromName("MoneyHitSound");
    rockHitSound = prehistoricLax_0:getSimpleSoundComponentFromName("RockHitSound");
    laughSound = prehistoricLax_0:getSimpleSoundComponentFromName("LaughSound");
    sighSound = prehistoricLax_0:getSimpleSoundComponentFromName("SighSound");
    
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
    animationBlender:registerAnimation(AnimID.ANIM_TAKE_DAMAGE, "Take_damage");
    animationBlender:registerAnimation(AnimID.ANIM_LAND_2, "Land2");
    animationBlender:registerAnimation(AnimID.ANIM_HALT, "Halt");
    animationBlender:registerAnimation(AnimID.ANIM_CRY, "Roar");
    animationBlender:registerAnimation(AnimID.ANIM_PICKUP_2, "Pickup2");
    animationBlender:registerAnimation(AnimID.ANIM_KNOCK_DOWN, "Knockdown");
    animationBlender:registerAnimation(AnimID.ANIM_STAND_UP, "Getup");
    animationBlender:registerAnimation(AnimID.ANIM_LAUGH, "Laugh");
    animationBlender:registerAnimation(AnimID.ANIM_SIGH, "Sigh");
    animationBlender:registerAnimation(AnimID.ANIM_ATTACK_2, "Melee_attack2");
    animationBlender:registerAnimation(AnimID.ANIM_ATTACK_3, "Melee_attack3");
     
    animationBlender:init1(AnimID.ANIM_WALK_NORTH, true);
    
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
        pullAction = false;
        if (pathFollowComponent:getWaypointId(0) ~= "1378398386") then
            pathFollowComponent:setWaypointId(0, "1378398386");
            pathFollowComponent:getMovingBehavior():setBehavior(BehaviorType.FOLLOW_PATH);
            pathFollowComponent:setActivated(true);
            animationBlender:blend5(AnimID.ANIM_WALK_NORTH, BlendingTransition.BLEND_WHILE_ANIMATING, 0.1, true);
            waypointReached = false;
            currentBucket = leftBucket;
            currentSpawnComponent  = currentBucket:getSpawnComponent();
            laughSound:setActivated(false);
            sighSound:setActivated(false);
        end
    elseif (inputDeviceModule:isActionDown(NOWA_A_RIGHT)) then
        pullAction = false;
        if (pathFollowComponent:getWaypointId(0) ~= "1091560280") then
            pathFollowComponent:setWaypointId(0, "1091560280");
            pathFollowComponent:getMovingBehavior():setBehavior(BehaviorType.FOLLOW_PATH);
            pathFollowComponent:setActivated(true);
            animationBlender:blend5(AnimID.ANIM_WALK_NORTH, BlendingTransition.BLEND_WHILE_ANIMATING, 0.1, true);
            waypointReached = false;
            currentBucket = rightBucket;
            currentSpawnComponent  = currentBucket:getSpawnComponent();
            laughSound:setActivated(false);
            sighSound:setActivated(false);
        end
     elseif (inputDeviceModule:isActionDown(NOWA_A_ACTION)) then
        if (waypointReached and canPull) then
            canPull = false;
            pullAction = true;
            
             local randomValue = math.random(0, 1)

            if randomValue == 0 then
                currentSpawnComponent:setSpawnTargetId(gold:getId())
            else
                currentSpawnComponent:setSpawnTargetId(stone:getId())
            end
            
            -- Must be set, because moving behavior does always set move and heading of the character so no other rotations are possible.
            -- Setting it to NONE, internally the update function is not called anymore, so below the faceDirectionSlerp can take place
            pathFollowComponent:getMovingBehavior():setBehavior(BehaviorType.NONE);
            animationBlender:blend5(AnimID.ANIM_PICKUP_2, BlendingTransition.BLEND_WHILE_ANIMATING, 0.1, false);
            
            failure = false;
            success = false;
            
            playerControllerComponent:reactOnAnimationFinished(function()
                 currentBucket:getJointHingeActuatorComponent():setActivated(true);
                 currentSpawnComponent:setActivated(true);
             end, true);
            
            prehistoricLax_0:getLuaScriptComponent():callDelayedMethod(function()
                animationBlender:blend5(AnimID.ANIM_KNOCK_DOWN, BlendingTransition.BLEND_WHILE_ANIMATING, 0.1, false);
                 if (currentSpawnComponent:getSpawnTargetId() == gold:getId()) then
                    moneyHitSound:setActivated(true);
                 else
                    rockHitSound:setActivated(true);
                 end

                prehistoricLax_0:getLuaScriptComponent():callDelayedMethod(function()
                        animationBlender:blend5(AnimID.ANIM_STAND_UP, BlendingTransition.BLEND_WHILE_ANIMATING, 0.1, false);
                        
                         prehistoricLax_0:getLuaScriptComponent():callDelayedMethod(function()
                            canPull = true;
                            animationBlender:blend5(AnimID.ANIM_IDLE_1, BlendingTransition.BLEND_SWITCH, 0.001, true);
                            
                             if (currentSpawnComponent:getSpawnTargetId() == gold:getId()) then
                                 success = true;
                                 animationBlender:blend5(AnimID.ANIM_LAUGH, BlendingTransition.BLEND_SWITCH, 0.001, true);
                                 startGoldDoubleAnimation();
                                 laughSound:setActivated(true);
                             else
                                 failure = true;
                                 animationBlender:blend5(AnimID.ANIM_SIGH, BlendingTransition.BLEND_SWITCH, 0.001, true);
                                 startGoldHalveAnimation();
                                 sighSound:setActivated(true);
                             end
                             
                             prehistoricLax_0:getLuaScriptComponent():callDelayedMethod(function()
                                 round = round +1;
                             end, 2);
                        end, 1.5);
                    end, 2);
           end, 2);
        end
    end
    
    if (pullAction) then
        local resultOrientation = MathHelper:faceDirectionSlerp(prehistoricLax_0:getOrientation(), Vector3.NEGATIVE_UNIT_Z, prehistoricLax_0:getDefaultDirection(), dt, 1200);
        local heading = resultOrientation:getYaw(true);
        physicsActiveComponent:move(0, 0, heading);
    end
    if(success or failure) then
        local resultOrientation = MathHelper:faceDirectionSlerp(prehistoricLax_0:getOrientation(), Vector3.UNIT_Z, prehistoricLax_0:getDefaultDirection(), dt, 1200);
        local heading = resultOrientation:getYaw(true);
        physicsActiveComponent:move(0, 0, heading);
    end
    
    if (round == 3) then
        pathFollowComponent:setWaypointId(0, "3208787552");
        pathFollowComponent:getMovingBehavior():setBehavior(BehaviorType.FOLLOW_PATH);
        pathFollowComponent:setActivated(true);
        if (animationBlender:isAnimationActive(AnimID.ANIM_WALK_NORTH) == false) then
            animationBlender:blend5(AnimID.ANIM_WALK_NORTH, BlendingTransition.BLEND_WHILE_ANIMATING, 0.1, true);
        end
        pullAction = false;
        canPull = false;
    end
    
    animationBlender:addTime(dt * 2 / animationBlender:getLength());
    
    if (goldAnimIsActive) then
        goldAnimElapsed = goldAnimElapsed + dt;
        local t = math.min(goldAnimElapsed / goldAnimDuration, 1.0);
        local current = math.floor(goldAnimStart + (goldAnimTarget - goldAnimStart) * t);
        goldAttribute:setValueNumber(current);
        goldText:setCaption("Gold: " .. toString(current));

        if (t >= 1.0) then
            goldAnimIsActive = false;
            moneyCountSound:setActivated(false);
        end
    end
end

function startGoldDoubleAnimation()
    goldAnimIsActive = true;
    goldAnimStart = goldAttribute:getValueNumber();
    goldAnimTarget = goldAnimStart * 2;
    goldAnimElapsed = 0;
    moneyCountSound:setActivated(true);
end

function startGoldHalveAnimation()
    goldAnimIsActive = true;
    goldAnimStart = goldAttribute:getValueNumber();
    goldAnimTarget = math.floor(goldAnimStart / 2);
    goldAnimElapsed = 0;
    moneyCountSound:setActivated(true);
end

