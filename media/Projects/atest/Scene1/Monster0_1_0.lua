module("Monster0_1_0", package.seeall);
-- physicsActiveComponent = nil;
Monster0_1_0 = {}

-- Scene: CollisionDirection

require("init");

monsterGameObject = nil
monsterController = nil;
monsterAnimation = nil;
terraGameObject = nil;

moveHorizontal = 0;
rotation = 0;
oldRotation = 0;
isActive = false;
firstTimeStart = true;
firstTimeEnd = true;
inputDeviceModule = nil;

Monster0_1_0["connect"] = function(gameObject)
    monsterGameObject = AppStateManager:getGameObjectController():castGameObject(gameObject);
    monsterController = monsterGameObject:getPhysicsPlayerControllerComponent();
    -- Helps the ide to identify the component type for better intelli sense results
    monsterController = AppStateManager:getGameObjectController():castPhysicsPlayerControllerComponent(monsterController);
    
    monsterAnimation = gameObject:getAnimationComponentV2():getAnimationBlender();
    monsterAnimation:registerAnimation(AnimationBlender.ANIM_IDLE_1, "T-pose");
    monsterAnimation:registerAnimation(AnimationBlender.ANIM_IDLE_2, "idle-03");
    monsterAnimation:registerAnimation(AnimationBlender.ANIM_WALK_NORTH, "walk-01");
    monsterAnimation:registerAnimation(AnimationBlender.ANIM_WALK_SOUTH, "walk-back");

    monsterAnimation:init1(AnimationBlender.ANIM_IDLE_2, true);
    
    inputDeviceModule = monsterGameObject:getInputDeviceComponent():getInputDeviceModule();
    
    --monsterAnimation:setDebugLog(true);
    
    --AppStateManager:getCameraManager():setMoveCameraWeight(0);
    --AppStateManager:getCameraManager():setRotateCameraWeight(0);
    
    terraGameObject = AppStateManager:getGameObjectController():getGameObjectFromId("3440706923");

    local brushNames = terraGameObject:getTerraComponent():getAllBrushNames();
    
    for i = 0, #brushNames do
        local brushName = brushNames[i];
        log("Brushname: " .. brushName);
    end
    terraGameObject:getTerraComponent():setBrushName("Circular.png");
    terraGameObject:getTerraComponent():setBrushSize(16);
end

Monster0_1_0["disconnect"] = function()
    rotation = 0;
    oldRotation = 0;
    monsterAnimation:blend5(AnimationBlender.ANIM_IDLE_1, AnimationBlender.BLEND_WHILE_ANIMATING, 0.1, true);
    --AppStateManager:getCameraManager():setMoveCameraWeight(1);
    --AppStateManager:getCameraManager():setRotateCameraWeight(1);
    
    AppStateManager:getGameObjectController():undoAll();
end

Monster0_1_0["update"] = function(dt)
    moveHorizontal = 0;
    isActive = false;
    -- local rotation = 0;
    speed = monsterController:getSpeed();
    
    oldRotation = rotation;
    
    if inputDeviceModule:isActionDown(NOWA_A_LEFT) then
        rotation = rotation - 3;
        isActive = true;
    elseif inputDeviceModule:isActionDown(NOWA_A_RIGHT) then
        rotation = rotation + 3;
        isActive = true;
    end
    
    local isRotating = oldRotation ~= rotation;
    
    if inputDeviceModule:isActionDown(NOWA_A_UP) then
        moveHorizontal = speed;
        isActive = true;
        firstTimeEnd = true;
    elseif inputDeviceModule:isActionDown(NOWA_A_DOWN) then
        moveHorizontal = -speed;
        isActive = true;
        firstTimeEnd = true;
    end
    
    if inputDeviceModule:isActionDown(NOWA_A_JUMP) then
       monsterController:jump();
    end
    
    if (moveHorizontal == speed or isRotating == true) then
        if (monsterAnimation:isAnimationActive(AnimationBlender.ANIM_IDLE_2) or monsterAnimation:isComplete()) then
            monsterAnimation:blend5(AnimationBlender.ANIM_WALK_NORTH, AnimationBlender.BLEND_SWITCH, 0.01, false);
        end
    elseif (moveHorizontal == -speed or isRotating == true) then
        if (monsterAnimation:isAnimationActive(AnimationBlender.ANIM_IDLE_2) or monsterAnimation:isComplete()) then
            monsterAnimation:blend5(AnimationBlender.ANIM_WALK_SOUTH, AnimationBlender.BLEND_SWITCH, 0.01, false);
        end
    else
        if (monsterAnimation:isComplete()) then
            monsterAnimation:blend5(AnimationBlender.ANIM_IDLE_2, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);
        end
    end
    
    local rotationQuat = Quaternion(Degree(rotation), Vector3.UNIT_Y);
    
    monsterController:move(moveHorizontal, 0.0, rotationQuat:getYaw(true));
    
    --monsterKinematic:setOmegaVelocity(Vector3(0, rotation, 0));
    
    -- local monsterDirection = monsterGameObject:getOrientation() * monsterGameObject:getDefaultDirection();
    -- monsterKinematic:setVelocity(Vector3(monsterDirection.x, 0, monsterDirection.z) * moveHorizontal);
    -- Same as:
    --monsterKinematic:setDirectionVelocity(moveHorizontal);
end

Monster0_1_0["onContactFriction"] = function(gameObject0, gameObject1, playerContact)
    local otherGameObject = nil;
    
    --playerContact = AppStateManager:getGameObjectController():castPlayerContact(playerContact);
    
    if (gameObject0 ~= monsterGameObject) then
        otherGameObject = gameObject0;
    else
        otherGameObject = gameObject1;
    end
    
    if (firstTimeStart == true) then
		terraGameObject:getTerraComponent():beginCommandTransaction("Lua Paint Terrain");
        terraGameObject:getTerraComponent():paintTerrainStart(playerContact:getPosition(), 10, 1);
        firstTimeStart = false;
    elseif (isActive == false and firstTimeEnd == true) then
        firstTimeEnd = false;
        terraGameObject:getTerraComponent():paintTerrainEnd();
		terraGameObject:getTerraComponent():endCommandTransaction();
    end
    
    if (monsterController:getVelocity():squaredLength() > 10) then
        -- To expensive, as each time because of undo redo, fps dropdown
        --terraGameObject:getTerraComponent():paintTerrainLoop(playerContact:getPosition(), 10, 2, 1);
        terraGameObject:getTerraComponent():paintTerrain(playerContact:getPosition(), 10, 2);
    end
	
	-- Lua nested undo example:
	-- cmd.beginTransaction("Create Bridge")

	-- road:modifyRoadStart(...);
	-- road:modifyRoadEnd();

	-- wall:modifyWallStart(...);
	-- wall:modifyWallEnd();
	-- cmd.beginTransaction();
end