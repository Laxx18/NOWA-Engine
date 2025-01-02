module("PrehistoricLax_0", package.seeall);

-- Enums
NONE, UP, DOWN, LEFT, RIGHT = 0, 1, 2, 3, 4

prehistoricLax = nil;
playerController = nil;
walkSound = nil;
jumpSound = nil;
smokeParticle = nil;
animationBlender = nil;
characterSliderMaterial = nil;
leftFoot = nil;
rightFoot = nil;
leftFootOriginOrientation = nil;
rightFootOriginOrientation = nil;
tempAnimationSpeed = 1;

directionMove = Vector3.ZERO;
direction = NONE;
directionChanged = false;
oldDirection = NONE;
boringTimer = 5;
noMoveTimer = 0;
jumpForce = 15;
inAir = false;
nearGround = true;
isAttacking = false;
tryJump = false;
highFalling = false;
jumpKeyPressed = false;
jumpCount = 0;
canDoubleJump = false;
walkCount = 0;
isOnRope = false;
groundedOnce = true;
duckedOnce = false;
acceleration = 0.5;
oldPositionY = 0;
relativeHeight = 0;
playerWeapon = nil;

 -- later control double jump via attributes component
doubleJump = true;
runAfterWalkTime = 3; 

timeSinceLastPush = 1;
canPush = true;

rightHand = nil;
rightLowerArm = nil;
rightLowerArmJoint = nil;
rightUpperArm = nil;
rightUpperArmJoint = nil;

timeSinceLastAttack = 0.2;
canAttack = false;

function attachSword()
    local physicsRagdollComponent = prehistoricLax:getPhysicsRagDollComponent();
    
    playerWeapon = AppStateManager:getGameObjectController():getGameObjectFromName("PlayerWeapon");
    playerWeapon:getPhysicsActiveComponent():setMass(0);

    rightUpperArm = physicsRagdollComponent:getRagBone("RightUpperArm");
    rightUpperArmJoint = rightUpperArm:getJointBallAndSocketComponent();
    rightLowerArm = physicsRagdollComponent:getRagBone("RightLowerArm");
    rightLowerArmJoint = rightLowerArm:getJointHingeComponent();
    --rightHand = physicsRagdollComponent:getRagBone("RightHand");
    
    --playerWeapon:getPhysicsActiveComponent():setPosition(rightLowerArm:getPosition() + Vector3(0, -1, -0.05));
    
    playerWeapon:getPhysicsActiveComponent():setPosition(rightLowerArm:getPosition() + (rightLowerArm:getOrientation() * Vector3(0, 0.4, -0.05)));
    playerWeapon:getPhysicsActiveComponent():setOrientation(rightLowerArm:getOrientation() * Quaternion(Degree(-90), Vector3.UNIT_Y) * Quaternion(Degree(-90), Vector3.UNIT_Z));
    
    prehistoricLax:getLuaScriptComponent():callDelayedMethod(function()
		
         
        --local playerWeapon = AppStateManager:getGameObjectController():getGameObjectFromName("PlayerWeapon");
        -- --playerWeapon:getPhysicsActiveComponent():setPosition(playerWeapon:getPhysicsActiveComponent():getOrientation() * (rightLowerArm:getPosition() + Vector3(0.4, -0.4, 0))); -- + Vector3(0.05, -0.15, 0.1)
        --playerWeapon:getPhysicsActiveComponent():setPosition(rightLowerArm:getPosition() + Vector3(0.05, -0.24, 0.28));
        -- -- --playerWeapon:getPhysicsActiveComponent():setOrientation(rightLowerArm:getOrientation());
        -- playerWeapon:getJointHingeComponent():setFriction(12);
        -- --local jointPin = (physicsRagdollComponent:getOrientation() * rightLowerArm:getOrientation()) * Vector3(0, 0, 1);
        -- local jointPin = Vector3(0, 0, 1);
        -- playerWeapon:getJointHingeComponent():setPin(jointPin);
        --playerWeapon:getJointHingeComponent():setPredecessorId(rightHandJoint:getId());
        --playerWeapon:createJoint(); is done automatically
        --playerWeapon:getJointHingeComponent():setAnchorPosition(Vector3(-1, 0, 0));
        
        --playerWeapon:getPhysicsActiveComponent():setPosition(rightLowerArm:getPosition() + Vector3(0, -1, -0.05));
        
        playerWeapon:getPhysicsActiveComponent():setPosition(rightLowerArm:getPosition() + (rightLowerArm:getOrientation() * Vector3(0, 0.4, -0.05)));
        playerWeapon:getPhysicsActiveComponent():setOrientation(rightLowerArm:getOrientation() * Quaternion(Degree(-90), Vector3.UNIT_Y) * Quaternion(Degree(-90), Vector3.UNIT_Z));
        playerWeapon:getJointHingeComponent():setPredecessorId(rightLowerArmJoint:getId());
        playerWeapon:getJointHingeComponent():connect();
        playerWeapon:getJointHingeComponent():createJoint(Vector3.ZERO);
        
        rightUpperArm = physicsRagdollComponent:getRagBone("RightUpperArm");
        rightUpperArmJoint = rightUpperArm:getJointBallAndSocketComponent();
        rightLowerArm = physicsRagdollComponent:getRagBone("RightLowerArm");
        rightLowerArmJoint = rightLowerArm:getJointHingeComponent();
        
        -- Attach picker to the currently equipped sword (Note: Weapon may change, hence equipment component is involved, which stores the currently id of the currently attached weapon)
        local picker = prehistoricLax:getPickerComponentFromName("RightHandPicker");
        picker:setActivated(true);
        picker:setTargetId(prehistoricLax:getEquipmentComponentFromName("RightHandWeapon"):getTargetId());
        
        picker:reactOnDraggingStart(function() 
                if (playerController:getStateMachine():getCurrentState() ~= RagDollState) then
                    playerController:getStateMachine():changeState(AttackState);
                end
            end);
        
        picker:reactOnDraggingEnd(function() 
                playerController:getStateMachine():changeState(WalkState);
            end);
        
        playerWeapon:getPhysicsActiveComponent():setMass(10);
        
    --prehistoricLax:getPickerComponentFromName("RightHandPicker"):setTargetBody(rightLowerArm:getBody());
        
        --playerWeapon:getJointTargetTransformComponent():setOffsetPosition(Vector3(0.05, -0.35, 0));
        --playerWeapon:getJointTargetTransformComponent():setOffsetOrientation(Quaternion(Degree(-90), Vector3.UNIT_Y));
    end, 1.5);
end

--function attachSword()
--    prehistoricLax:getLuaScriptComponent():callDelayedMethod(function()
		
--        local tagPointComponent = prehistoricLax:getTagPointComponent();

--        tagPointComponent:connect();
--    end, 1);
--end

PrehistoricLax_0 = {};

-- Seems not to work combining connect and states table! because the variables created in connect, are out of scope WalkState["execute"]!
PrehistoricLax_0["connect"] = function(gameObject)

	local mainGameObject = AppStateManager:getGameObjectController():getGameObjectFromId(MAIN_GAMEOBJECT_ID);
   
    local attributesComponent = mainGameObject:getAttributesComponent();
    local score = attributesComponent:getAttributeValueByName("Score");
    local scoreText = mainGameObject:getMyGUITextComponentFromName("ScoreText");
    local scoreSound = mainGameObject:getSimpleSoundComponentFromName("ScoreSound");
   
    timeSinceLastAttack = 0.2;
	timeSinceLastPush = 1;
	 
	prehistoricLax = gameObject;
    
    prehistoricLax:getAreaOfInterestComponentFromName("MainArea"):reactOnEnter(function(hitGameObject) 
		if (hitGameObject ~= nil) then
            if (hitGameObject:getTagName() == "Score") then
                scoreSound:setActivated(true);
                score:incrementValueNumber(1);
                scoreText:setCaption("Score: " .. toString(score:getValueNumber()));
                AppStateManager:getGameObjectController():deleteGameObject(hitGameObject:getId());
            end
        end
    end);
    
	prehistoricLax:getAreaOfInterestComponentFromName("MechanicsArea"):reactOnEnter(function(hitGameObject) 
        if (hitGameObject ~= nil) then
			
            log("--->Activate name: " .. hitGameObject:getName() .. " tag name: " .. hitGameObject:getTagName()); 
			if (hitGameObject:getTagName() == "Lever") then
                -- Lever will only work from the right side
                if (direction == LEFT) then
                    do return end;
                end
            
				local hingeActuator = hitGameObject:getJointHingeActuatorComponent();
				local attributesComponent = hitGameObject:getAttributesComponent();
				local firstTimeActivated = hitGameObject:getAttributesComponent():getAttributeValueByName("FirstTimeActivated");
				log("firstTimeActivated: " .. (firstTimeActivated:getValueBool() and 'true' or 'false')); 
				-- Activate lever motion (but only once)
                
				if (hingeActuator ~= nil and firstTimeActivated:getValueBool() == true) then
					log("--->Activate hinge joint!"); 
					hingeActuator:setActivated(true);
					animationBlender:blendAndContinue1(AnimationBlender.ANIM_ACTION_1);
					-- Pulling the lever should only be possible one time!
					firstTimeActivated:setValueBool(false);
					-- React when animation is finished
					playerController:reactOnAnimationFinished(function()

						log("---->Action finished");
						playerController:lockMovement("mechanics", false);
						-- TODO: Later set via tagname! Like score below
						if matches(hitGameObject:getName(), "Lever*") then
							local hingeActuator = hitGameObject:getJointHingeActuatorComponent();
							-- Deactivate everything again
							hingeActuator:setActivated(false);
						end
						
						local referenceId = hitGameObject:getReferenceId();
						AppStateManager:getGameObjectController():activateGameObjectComponentsFromReferenceId(referenceId, false);
						
					end, true);
					-- Activate some references
					local referenceId = hitGameObject:getReferenceId();
					AppStateManager:getGameObjectController():activateGameObjectComponentsFromReferenceId(referenceId, true);
					-- Lock player input
					playerController:lockMovement("mechanics", true);
				end
                
            elseif (hitGameObject:getTagName() == "Handle") then
                -- Handle will only work from the right side
                --if (direction == LEFT) then
                --    do return end
                --end
                
                local handle = hitGameObject;
                
                -- First go to waypoint
                animationBlender:blendExclusive1(AnimationBlender.ANIM_WALK_NORTH, AnimationBlender.BLEND_WHILE_ANIMATING);
                handle:getAiPathFollowComponent():setActivated(true);
                handle:getAiPathFollowComponent():reactOnPathGoalReached(function()
                    log("--->Activate Handle!"); 
                   
                    if (direction == LEFT) then
                        -- Turns the player to the correct side for pulling something
                        direction = RIGHT;
                    end
                    
                    handle:getAiPathFollowComponent():setActivated(false);
                    
                    handle:getJointHingeComponent():setActivated(true);
                    local targetPosition = handle:getJointHingeComponent():getUpdatedJointPosition();
                    log("--->kinematic target position: " .. toString(targetPosition)); 
                    playerWeapon:getJointKinematicComponent():setTargetPosition(targetPosition);
                    
                    playerWeapon:getJointKinematicComponent():reactOnTargetPositionReached(function()
                        playerWeapon:getJointHingeComponentFromName("HingeAttach"):setPredecessorId(hitGameObject:getJointHingeComponent():getId());
                        playerWeapon:getJointHingeComponentFromName("HingeAttach"):setActivated(true);
                        -- Activates the actuator which helps rotating the winder
                        handle:getJointHingeComponent():getPredecessorJointComponent():setActivated(true);
                    end);
                    
                    -- Lock player input
                    playerController:lockMovement("mechanics", true);
                    prehistoricLax:getAreaOfInterestComponentFromName("MechanicsArea"):setActivated(false);
                    
                    -- HingeActuator two rotations finished
                    handle:getJointHingeComponent():getPredecessorJointComponent():reactOnTargetAngleReached(function(newAngle)
                        log("--->Deactivate mechanics: Angle: " .. toString(newAngle)); 
                        playerWeapon:getJointKinematicComponent():setActivated(false);
                        playerWeapon:getJointHingeComponentFromName("HingeAttach"):setActivated(false);
                        playerWeapon:getJointHingeComponentFromName("HingeAttach"):setPredecessorId("");
                        --hitGameObject:getJointHingeComponent():getPredecessorJointComponent():setActivated(false);
                        --hitGameObject:getJointHingeComponent():setActivated(false);
                        
                        playerController:lockMovement("mechanics", false);
                        attachSword();
                    end);
                end);
                
                
				-- TODO: Later set via tagname! Like score below
			elseif (hitGameObject:getTagName() == "Portal") then
                
				-- Activate some references
				local referenceId = hitGameObject:getReferenceId();
				AppStateManager:getGameObjectController():activateGameObjectComponentsFromReferenceId(referenceId, true);
				playerController:getStateMachine():changeState(PortalState);
				-- TODO: Later set via tagname! Like score below
			else
			
				local inventoryItem = hitGameObject:getInventoryItemComponent();
				if (inventoryItem ~= nil) then
					animationBlender:blendAndContinue1(AnimationBlender.ANIM_PICKUP_1);
					
					playerController:reactOnAnimationFinished(function()
                        if (hitGameObject:getTagName() == "Money") then
                            inventoryItem:addQuantityToInventory(MAIN_GAMEOBJECT_ID, math.random(5, 10), true);
                            AppStateManager:getGameObjectController():deleteGameObject(hitGameObject:getId());
                        elseif (hitGameObject:getTagName() == "Energy") then
                            inventoryItem:addQuantityToInventory(MAIN_GAMEOBJECT_ID, 1, true);
							AppStateManager:getGameObjectController():deleteGameObject(hitGameObject:getId());
                        end
						playerController:lockMovement("mechanics", false);
						
					end, true);
					
					playerController:lockMovement("mechanics", true);
				end
			end
		else
			animationBlender:blendAndContinue1(AnimationBlender.ANIM_NO_IDEA);
		end
	end);

    prehistoricLax:getAreaOfInterestComponentFromName("MechanicsArea"):reactOnLeave(function(hitGameObject) 
        
        end);

    -- Attach weapon to hand
	attachSword();
	
end

PrehistoricLax_0["disconnect"] = function(gameObject)
	 --gameObject:getPlayerController():getPhysicsRagDollComponent():setState("Inactive");
end

---------------------------------------------------------------------------------------------------

WalkState = { };

WalkState["enter"] = function(gameObject)
	 prehistoricLax = gameObject;
     
	 playerController = prehistoricLax:getPlayerControllerJumpNRunLuaComponent();
	 walkSound = playerController:getOwner():getSimpleSoundComponentFromIndex(0);
	 jumpSound = playerController:getOwner():getSimpleSoundComponentFromIndex(1);
	 smokeParticle = playerController:getOwner():getParticleUniverseComponentFromIndex(0);
	 smokeParticle:setActivated(false);
	 --smokeParticle:setPlaySpeed(10);
	 smokeParticle:setPlayTimeMS(50);
	 --smokeParticle:setScale(Vector3(0.05, 0.05, 0.05));
	 walkSound:setActivated(false);
	 jumpSound:setActivated(false);
	 walkSound:setPitch(0.65);
	 walkSound:setVolume(15);
	 jumpSound:setVolume(50);
	 
	 animationBlender = playerController:getAnimationBlender();
	 local mainGameObject = AppStateManager:getGameObjectController():getGameObjectFromId(MAIN_GAMEOBJECT_ID);
	 characterSliderMaterial = mainGameObject:getPhysicsMaterialComponentFromIndex(2);
	 
	 if (animationBlender ~= nil) then
		log("enter: animationBlender ok");
	 else
		log("enter: animationBlender nil");
	 end
	 animationBlender:registerAnimation(AnimationBlender.ANIM_IDLE_1, "Idle");
	 animationBlender:registerAnimation(AnimationBlender.ANIM_IDLE_2, "Greet");
	 animationBlender:registerAnimation(AnimationBlender.ANIM_IDLE_3, "Bored");
	 animationBlender:registerAnimation(AnimationBlender.ANIM_WALK_NORTH, "Walk");
	 animationBlender:registerAnimation(AnimationBlender.ANIM_WALK_SOUTH, "Walk_backwards");
	 animationBlender:registerAnimation(AnimationBlender.ANIM_WALK_WEST, "Walk");
	 animationBlender:registerAnimation(AnimationBlender.ANIM_WALK_EAST, "Walk");
	 animationBlender:registerAnimation(AnimationBlender.ANIM_JUMP_START, "Jump3");
	 animationBlender:registerAnimation(AnimationBlender.ANIM_JUMP_WALK, "Jump");
	 animationBlender:registerAnimation(AnimationBlender.ANIM_HIGH_JUMP_END, "Land");
	 animationBlender:registerAnimation(AnimationBlender.ANIM_JUMP_END, "Land2");
	 animationBlender:registerAnimation(AnimationBlender.ANIM_FALL, "Falling");
	 animationBlender:registerAnimation(AnimationBlender.ANIM_RUN, "Run");
	 animationBlender:registerAnimation(AnimationBlender.ANIM_SNEAK, "Take_damage");
	 animationBlender:registerAnimation(AnimationBlender.ANIM_DUCK, "Land2");
	 animationBlender:registerAnimation(AnimationBlender.ANIM_HALT, "Halt");
	 animationBlender:registerAnimation(AnimationBlender.ANIM_NO_IDEA, "Roar");
	 animationBlender:registerAnimation(AnimationBlender.ANIM_PICKUP_1, "Pickup");
	 animationBlender:registerAnimation(AnimationBlender.ANIM_ACTION_1, "Cheer");
	 animationBlender:registerAnimation(AnimationBlender.ANIM_ATTACK_1, "Melee_attack");
	 animationBlender:registerAnimation(AnimationBlender.ANIM_ATTACK_2, "Melee_attack2");
	 animationBlender:registerAnimation(AnimationBlender.ANIM_ATTACK_3, "Melee_attack3");
	 
	 
	 animationBlender:init1(AnimationBlender.ANIM_IDLE_1, true);
     --animationBlender:blend1(AnimationBlender.ANIM_IDLE_1, AnimationBlender.BLEND_THEN_ANIMATE, 0.2, true);
	 
	 --leftFoot = animationBlender:getBone("Foot_L");
	 --rightFoot = animationBlender:getBone("Foot_R");
	 
	 --leftFoot:reset();
	 --rightFoot:reset();
	 
	 --leftFootOriginOrientation = leftFoot:getOrientation();
	 --rightFootOriginOrientation = rightFoot:getOrientation();
	 
	 --local leftFootAbsoluteWorldOrientation = leftFoot:getDerivedOrientation();
	 -- Set inherit orientation to false
	 --leftFoot:setManuallyControlled(true);
	 --leftFoot:setInheritOrientation(false);
	 -- Set the absolute world orientation
	 --leftFoot:setOrientation(leftFootAbsoluteWorldOrientation);
	 
	 --local rightFootAbsoluteWorldOrientation = rightFoot:getDerivedOrientation();
	 -- Set inherit orientation to false
	 --rightFoot:setManuallyControlled(true);
	 --rightFoot:setInheritOrientation(false);
	 -- Set the absolute world orientation
	 --rightFoot:setOrientation(rightFootAbsoluteWorldOrientation);
	 
	 --animationBlender:setBoneWeight(leftFoot, 0);
	 --animationBlender:setBoneWeight(rightFoot, 0);
     
    tempAnimationSpeed = playerController:getAnimationSpeed();

    boringTimer = 0;
    noMoveTimer = 0;
end

WalkState["execute"] = function(gameObject, dt)
	
	if (noMoveTimer > 0.1) then
		playerController:setMoveWeight(0);
		playerController:setJumpWeight(0);
		noMoveTimer = noMoveTimer - dt;
	elseif (noMoveTimer < 0.1) then
		-- Call this only once, hence noMoveTimer = 0.1
		playerController:setMoveWeight(1);
		playerController:setJumpWeight(1);
		noMoveTimer = 0.1;
	end
	
	local tempSpeed = 0;
	local heading = playerController:getOrientation():getYaw(true);
	local height = playerController:getHeight();
	local onSlider = false;
	local onTrampolin = false;
	local atObstacle = false;
	local keyDirection = Vector3.ZERO;
	directionMove = Vector3.ZERO;
	
	if (height <= 0) then
		inAir = true;
        nearGround = false;
	else
		--inAir = height - playerController:getOwner():getBottomOffset().y > 0.2;
		inAir = height > 0.2;
	end
    
    nearGround = height > 2;

	-- Store the old direction, the check later whether a direction changed or not
	oldDirection = direction;
	-- Store the old position y, to check later the relative height
	if (inAir == false) then
		oldPositionY = playerController:getPhysicsComponent():getPosition().y;
	end
	relativeHeight = playerController:getPhysicsComponent():getPosition().y - oldPositionY;
	
	-- Set omega by default directly to zero, else there is an ugly small rotation...
--playerController:getPhysicsComponent():setOmegaVelocity(Vector3.ZERO);
	
	--log("In air: " .. (inAir and 'true' or 'false')); -- inAir is bool and somehow cannot be printed
	--log("Height: " .. height);
	--log("RelativeHeight: " .. relativeHeight);
	 
	------------------------------------------Ray Checks----------------------------------------------------
	-- Check what is below the player
	local hitGameObjectBelow = playerController:getHitGameObjectBelow();
	if (hitGameObjectBelow ~= nil) then
        
		if (hitGameObjectBelow:getCategory() == "Slider") then
			if (inAir == false) then
				onSlider = true;
				local horizontalSlider = hitGameObjectBelow:getJointActiveSliderComponent();
				if (horizontalSlider:getPin().x == 1) then
					directionMove = horizontalSlider:getPin() * horizontalSlider:getCurrentDirection() * horizontalSlider:getMoveSpeed();
					--playerController:getPhysicsComponent():setVelocity(hitGameObjectBelow:getPhysicsActiveComponent():getVelocity());
				end
			end
		elseif (hitGameObjectBelow:getCategory() == "Trampolin") then
			onTrampolin = true;
		end
	end
	
	-- Check what is in front of the player
	local hitGameObjectFront = playerController:getHitGameObjectFront();
	--log("hitGameObjectFront: ");
	if (hitGameObjectFront ~= nil) then
		walkCount = 0;
		if (inAir == true) then
			atObstacle = true;
		end
-- Attention: Here maybe check which category, if something is weird in game
		--log("hitGameObjectFront: " .. hitGameObjectFront:getName());
		-- Halt when its a static collision object
		if (hitGameObjectFront:getPhysicsArtifactComponent() ~= nil or (inAir == true and hitGameObjectFront:getCategory() ~= "NoCollide")) then
			--playerController:setMoveWeight(0);
			playerController:getPhysicsComponent():applyRequiredForceForVelocity(playerController:getPhysicsComponent():getVelocity() * Vector3(0, 1, 0) + playerController:getPhysicsComponent():getGravity());
		end
	end
	
	-- Check what is above  the player
	local hitGameObjectAbove = playerController:getHitGameObjectUp();
	if (hitGameObjectAbove ~= nil) then
		--log("hitGameObjectFront: ");
		if (hitGameObjectAbove:getPhysicsArtifactComponent() or (inAir == true and hitGameObjectAbove:getCategory() ~= "NoCollide")) then
			walkCount = 0;
			playerController:setMoveWeight(0);
			playerController:getPhysicsComponent():applyRequiredForceForVelocity(playerController:getPhysicsComponent():getVelocity() * Vector3(0, 1, 0) + playerController:getPhysicsComponent():getGravity());
			atObstacle = true;
		end
	end
    
    ------------------------------------------Checks if in mechanics--------------------------------------
    
    if (playerController:getMoveWeight() == 0) then
        local resultOrientation = Quaternion(Degree(90), Vector3.UNIT_Y);
        if (direction == LEFT) then
            resultOrientation = Quaternion(Degree(-90), Vector3.UNIT_Y);
        end
        
        playerController:getPhysicsComponent():applyOmegaForceRotateTo(resultOrientation, Vector3.UNIT_Y, 10);
    end
	
	------------------------------------------Foot orientation--------------------------------------------
	
	--leftFoot:reset();
	--rightFoot:reset();
	
	
	--leftFootOriginOrientation = leftFoot:convertLocalToWorldOrientation(leftFootOriginOrientation);
	--rightFootOriginOrientation = rightFoot:convertLocalToWorldOrientation(rightFootOriginOrientation);
	
	--leftFoot:setDerivedOrientation(leftFootOriginOrientation * Quaternion(Degree(playerController:getSlope()), Vector3.UNIT_X)); -- wrong
	--rightFoot:setDerivedOrientation(rightFootOriginOrientation * Quaternion(Degree(playerController:getSlope()), Vector3.UNIT_X));
	--leftFoot:setDerivedOrientation(Quaternion(Degree(playerController:getSlope()), Vector3.UNIT_X) * leftFootOriginOrientation:Inverse()); -- crazy rotation
	--rightFoot:setDerivedOrientation(Quaternion(Degree(playerController:getSlope()), Vector3.UNIT_X) * rightFootOriginOrientation:Inverse());
	
	--leftFoot:setDerivedOrientation(Quaternion(Degree(playerController:getSlope()), Vector3.UNIT_X) * leftFoot:getOrientation());
	--rightFoot:setDerivedOrientation(Quaternion(Degree(playerController:getSlope()), Vector3.UNIT_X) * rightFoot:getOrientation());
	
	--leftFoot:setDerivedOrientation(Quaternion(Degree(playerController:getSlope()), Vector3.UNIT_X) * leftFoot:getParent():getOrientation():Inverse());
	--rightFoot:setDerivedOrientation(Quaternion(Degree(playerController:getSlope()), Vector3.UNIT_X) * rightFoot:getParent():getOrientation():Inverse());
	
	if (relativeHeight == 0) then
		local normal = playerController:getNormal();
		-- If player is on ground and the rise at an acceptable value, adapt the foot orientation
		if (normal ~= Vector3.UNIT_SCALE * 100) then
			if (direction == RIGHT) then
				--local offset = normal:getRotationTo(Quaternion(Degree(-125), Vector3.UNIT_Y):xAxis(), Vector3.ZERO):xAxis();
				--leftFoot:setDirection(offset, Vector3.UNIT_Y);
				--rightFoot:setDirection(offset, Vector3.UNIT_Y);

				--leftFoot:setOrientation(leftFoot:getOrientation() * Quaternion(Degree(-30), Vector3.UNIT_Y) * leftFoot:getParent():getOrientation():Inverse());
				--rightFoot:setOrientation(rightFoot:getOrientation() * Quaternion(Degree(-30), Vector3.UNIT_Y) * rightFoot:getParent():getOrientation():Inverse());
				--leftFoot:rotate(normal);
				--rightFoot:rotate(Quaternion(Degree(-30), Vector3.UNIT_Y));
			elseif (direction == LEFT) then
				--local offset = normal:getRotationTo(Quaternion(Degree(125), Vector3.UNIT_Y):xAxis(), Vector3.ZERO):xAxis();
				--leftFoot:setDirection(offset, Vector3.NEGATIVE_UNIT_Y);
				--rightFoot:setDirection(offset, Vector3.NEGATIVE_UNIT_Y);
				
				--leftFoot:setOrientation(leftFoot:getOrientation() * Quaternion(Degree(45), Vector3.UNIT_Y) * leftFoot:getParent():getOrientation():Inverse());
				--rightFoot:setOrientation(rightFoot:getOrientation() * Quaternion(Degree(45), Vector3.UNIT_Y) * rightFoot:getParent():getOrientation():Inverse());
				--leftFoot:rotate(Quaternion(Degree(30), Vector3.NEGATIVE_UNIT_Y));
				--rightFoot:rotate(Quaternion(Degree(30), Vector3.NEGATIVE_UNIT_Y));
			end
		end
	end
	
	--if (playerController:getSlope() > 45 and playerController:getMoveWeight() ~= 0) then
	--	noMoveTimer = 5;
	--	log("Ragdolling");
	--	playerController:getPhysicsRagDollComponent():setState("Ragdolling");
	--elseif (noMoveTimer == 0.1) then
	--	playerController:getPhysicsRagDollComponent():setState("Inactive");
	--	log("Inactive");
	--end
	 
	-----------------------------------------Handle idle--------------------------------------------------
	-- If no key is pressed or player is not able to move (moveWeight = 0), go to idle state
	if ((playerController:getMoveWeight() == 0) or (InputDeviceModule:isActionDown(NOWA_A_DOWN) == false
			and InputDeviceModule:isActionDown(NOWA_A_LEFT) == false and InputDeviceModule:isActionDown(NOWA_A_RIGHT) == false
			and jumpKeyPressed == false)) then
	
		walkCount = 0;
		keyDirection = Vector3.ZERO;
		-- *&& playerController:getAnimationBlender()->isComplete()*/ ??
		if (inAir == false) then
			animationBlender:blendExclusive1(AnimationBlender.ANIM_IDLE_1, AnimationBlender.BLEND_WHILE_ANIMATING);
		end

		tempAnimationSpeed = playerController:getAnimationSpeed() * 0.5;
-- Attention: Due to rotation of 90 degree at once, commented this out
		--direction = NONE;
			
		-- No acceleration, move with usual constant speed
		if (playerController:getAcceleration() == 0) then
			acceleration = 1;
		else
			acceleration = acceleration - playerController:getAcceleration() * dt;
			if (acceleration <= 0.5) then
				acceleration = 0.5;
			end
		end

		boringTimer = boringTimer + dt;
		if (inAir == true) then
			boringTimer = 0;
		end
		
		-- If the user does nothing for 10 seconds, choose a random boring animation state
		if (boringTimer >= 10) then
			boringTimer = 0;
			local id = MathHelper:getRandomNumberInt(0, 3);
			local animID;
			if (0 == id) then
				animID = AnimationBlender.ANIM_IDLE_1;
			elseif (1 == id) then
				animID = AnimationBlender.ANIM_IDLE_2;
			else
				animID = AnimationBlender.ANIM_IDLE_3;
			end
			
			if (animationBlender:hasAnimation(animID)) then
                if (animationBlender:isAnimationActive(animID) == true or animationBlender:isComplete() == true) then
                    tempAnimationSpeed = playerController:getAnimationSpeed() * 0.5;
                    animationBlender:blend5(animID, AnimationBlender.BLEND_WHILE_ANIMATING, 0.5, true);
                end
			end
		end
		
		--if (playerController:isIdle() and playerController:getPhysicsComponent():getHasBuoyancy() == false) then
			--playerController:getPhysicsComponent():setVelocity(playerController:getPhysicsComponent():getVelocity() * Vector3(0, 1, 0));
		--end
	end
	
	--if (playerController:getMoveWeight() ~= 0 and InputDeviceModule:isActionDown(NOWA_A_ATTACK_1) == true) then
	--	playerController:getPhysicsRagDollComponent():setBoneRotation("RightUpperArm", Vector3.UNIT_Z, 2000);
	--end
	
	-------------------------------------------Handle movement--------------------------------
	if (InputDeviceModule:isActionDown(NOWA_A_LEFT) or InputDeviceModule:isActionDown(NOWA_A_RIGHT) and atObstacle == false) then
			
		boringTimer = 0;

		-- No acceleration, move with usual constant speed
		if (playerController:getAcceleration() == 0) then
			acceleration = 1;
		else
			acceleration = acceleration + playerController:getAcceleration() * dt;
			if (acceleration >= 1) then
				acceleration = 1;
			--else
			--	Ogre::Real weight = acceleration + 0.5;
			--	if (weight > 1)
			--		weight = 1;
			--	// playerController:getAnimationBlender()->getSource()->setWeight(weight);
			end
		end
		
		-- walk if left or right is being pressed
		boringTimer = 0;

		local animId = AnimationBlender.ANIM_NONE;

		-- JumpNRun 2D treatement: Rotate til 90 degree at once
		if (InputDeviceModule:isActionDown(NOWA_A_LEFT)) then
			if (runAfterWalkTime > 0 and inAir == false) then
				walkCount = walkCount + dt;
			end
			-- Walked long enough without pause : run
			if (walkCount >= runAfterWalkTime) then
				animId = AnimationBlender.ANIM_RUN;
				tempSpeed = playerController:getPhysicsComponent():getMaxSpeed() * playerController:getMoveWeight();
				tempAnimationSpeed = tempAnimationSpeed * 0.5;
			else
				tempSpeed = playerController:getPhysicsComponent():getSpeed() * playerController:getMoveWeight();
				animId = AnimationBlender.ANIM_WALK_NORTH;
			end
			direction = LEFT;
			keyDirection = Vector3(-1, 0, 0);
		elseif (InputDeviceModule:isActionDown(NOWA_A_RIGHT)) then
			if (runAfterWalkTime > 0 and inAir == false) then
				walkCount = walkCount + dt;
			end
			-- Walked long enough without pause : run
			if (walkCount >= runAfterWalkTime) then
				animId = AnimationBlender.ANIM_RUN;
				tempSpeed = playerController:getPhysicsComponent():getMaxSpeed() * playerController:getMoveWeight();
				tempAnimationSpeed = tempAnimationSpeed * 0.5;
			else
				tempSpeed = playerController:getPhysicsComponent():getSpeed() * playerController:getMoveWeight();
				animId = AnimationBlender.ANIM_WALK_NORTH;
			end
			direction = RIGHT;
			keyDirection = Vector3(1, 0, 0);
		end
		
		-- If player is stuck play corresponding animation
		--if (playerController:getMoveWeight() == 0) then
		--	animId = AnimationBlender.ANIM_HALT;
		--end
			
		if (animationBlender:isAnimationActive(animId) == false
			and (animationBlender:isComplete() or animationBlender:isAnimationActive(AnimationBlender.ANIM_IDLE_1)
				or animationBlender:isAnimationActive(AnimationBlender.ANIM_IDLE_2)
				or animationBlender:isAnimationActive(AnimationBlender.ANIM_IDLE_3))
			and inAir == false and jumpKeyPressed == false and playerController:getMoveWeight() == 1) then
			
			tempAnimationSpeed = playerController:getAnimationSpeed();
			-- 0.02f: Immediately blend to walk
---------------------------------------------------------------------------------------------------
			animationBlender:blend5(animId, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);
---------------------------------------------------------------------------------------------------
		end
			
	elseif (InputDeviceModule:isActionDown(NOWA_A_DUCK)) then
		boringTimer = 0;
		if (duckedOnce == false) then
			duckedOnce = true;
			animationBlender:blendExclusive5(AnimationBlender.ANIM_DUCK, AnimationBlender.BLEND_WHILE_ANIMATING, 0.1, false);
			-- playerController:getPhysicsComponent()->getBody()->scaleCollision(Ogre::Vector3(1.0f, 0.5f, 1.0f));
		end
		if (animationBlender:getTimePosition() >= animationBlender:getLength() - 0.3) then
			animationBlender:setTimePosition(0.7);
		end
	elseif (InputDeviceModule:isActionDown(NOWA_A_DUCK) == false and duckedOnce == true) then
		boringTimer = 0;
		-- playerController:getPhysicsComponent()->getBody()->scaleCollision(Ogre::Vector3(1.0f, 1.0f, 1.0f));
		duckedOnce = false;
	end
    
    directionMove = keyDirection * tempSpeed * acceleration;
	
	---------------------------------Player Rotation---------------------------------------------------------------
	-- If the old direction does not match the current direction
	-- the player changed direction, but this only set the direction change to true, never to false, which is done, after a 90 degree turn has been accomplished
	if (oldDirection ~= direction) then
		directionChanged = true;
		walkCount = 0;
	end
		
	-- If the player changed direction, rotate the player from x to -x and vice version (+- 180 degree), after that directionChanged is set to false
	if (directionChanged == true) then
		boringTimer = 0;
		local yawAtSpeed = 0;
		local currentDegree = playerController:getPhysicsComponent():getOrientation():getYaw(true):valueDegrees();
		if (direction == RIGHT) then
			yawAtSpeed = playerController:getRotationSpeed();
			if (currentDegree >= 90) then
				directionChanged = false;
				yawAtSpeed = 0;
			end
		elseif (direction == LEFT) then
			yawAtSpeed = -playerController:getRotationSpeed();
			if (currentDegree <= -90) then
				directionChanged = false;
				yawAtSpeed = 0;
			end
		end

		playerController:getPhysicsComponent():applyOmegaForce(Vector3(0, yawAtSpeed, 0));
		
	else
		-- player should not rotate without intention
		-- playerController:getPhysicsComponent():setOrientation(Quaternion(Degree(-90), Vector3.UNIT_Y)); does corrupt ragdoll! Arm will fly around!!
		
		local resultOrientation = Quaternion(Degree(90), Vector3.UNIT_Y);
		if (direction == LEFT) then
			resultOrientation = Quaternion(Degree(-90), Vector3.UNIT_Y);
		end
		
        -- Cool rotation in air!
        --if (inAir == false) then
            playerController:getPhysicsComponent():applyOmegaForceRotateTo(resultOrientation, Vector3.UNIT_Y, 10);
        --end
	end
	
    -- Deactivated: Else its not possible to jump and afterwards move forwards
	--if (inAir == false and isJumping == false and playerController:getPhysicsComponent():getVelocity().y < 2 and onTrampolin == false and onSlider == false) then
    playerController:getPhysicsComponent():applyRequiredForceForVelocity(playerController:getPhysicsComponent():getVelocity() * Vector3(0, 1, 0) + directionMove);

	-- /*!walkSound:isPlaying() &&*/
	if (inAir == false and direction ~= NONE) then
		-- walkSound:setSecondOffset(2);
		walkSound:setPitch(1);
		walkSound:setVolume(55);
		walkSound:setVelocity(playerController:getPhysicsComponent():getVelocity());
		-----------------------------walkSound:setActivated(true);
		-- walkSound:setRolloffFactor()
	end
	
	---------------------------------Ragdoll Test------------------------------------------------------------------
	
	if timeSinceLastPush >= 0 then
		timeSinceLastPush = timeSinceLastPush - dt;
	end

	-- Test
	if timeSinceLastPush <= 0 then
		if (InputDeviceModule:isKeyDown(KeyEvent.KC_T) and canPush == true) then
			timeSinceLastPush = 1;
			-- Change to rag doll state, so that no rays etc. are calculated and player cannot move for 5 seconds
			playerController:getStateMachine():changeState(RagDollState);
			
			canPush = false;
		elseif (InputDeviceModule:isKeyDown(KeyEvent.KC_O) and canPush == true) then
			timeSinceLastPush = 1;
			
			playerController:getPhysicsRagDollComponent():setBoneConfigFile("PrehistoricLaxPartial.rag");
			playerController:getPhysicsRagDollComponent():setState("PartialRagdolling");
			
			
			canPush = false;
		elseif (InputDeviceModule:isKeyDown(KeyEvent.KC_N) and canPush == true) then
			timeSinceLastPush = 1;
			
			playerController:getPhysicsRagDollComponent():setBoneConfigFile("PrehistoricLax2.rag");
			playerController:getPhysicsRagDollComponent():setState("Inactive");
			
			
			canPush = false;
		end
	else
		canPush = true;
	end
	
	-----------------------------------------------Handle Jump---------------------------------------------------
	jumpKeyPressed = false;
	if (InputDeviceModule:isActionDown(NOWA_A_JUMP)) then
		boringTimer = 0;
		if (canDoubleJump == true) then
			
			if (jumpCount == 0) then
				jumpCount = jumpCount + 1;
				-- When a double jump is made, it must be made in air, else if the player presses the jump key immediately a second time, the player will jump to high
			elseif (relativeHeight > 2.5) then
				jumpCount = jumpCount + 1;
			end
			canDoubleJump = false;
			if (jumpCount > 2) then
				jumpCount = 0;
			end
		end
		if (tryJump == false) then
			jumpKeyPressed = true;
		end
	else
		-- In order that the player does not jump again and again if the jump key is hold, only set isJumping to false if the player released the jump key
		-- so that he can jump if jump key is pressed again
		tryJump = false;
		canDoubleJump = true;
	end
    
    if (playerController:getJumpWeight() == 1 and (jumpKeyPressed == true and inAir == false or (doubleJump == true and jumpCount == 2))) then
        boringTimer = 0;
		if (playerController:getPhysicsComponent():getVelocity().y <= jumpForce * jumpCount) then
            --playerController:getPhysicsComponent():applyRequiredForceForVelocity(playerController:getPhysicsComponent():getVelocity() * Vector3(0, 1, 0) + Vector3(directionMove.x, jumpForce * playerController:getJumpWeight() * playerController:getMoveWeight(), 0));
            -- Using applyRequiredForceForVelocity, player will not always jump because of some force anomalities... Hence velocity jump is used
            
            playerController:getPhysicsComponent():addImpulse(Vector3(0, jumpForce * playerController:getJumpWeight() * playerController:getMoveWeight(), 0));
			--playerController:setJumpWeight(1);
			if (jumpCount >= 2) then
				jumpCount = 0;
			end
		else
			playerController:setJumpWeight(0);
		end
	end
	
	-- If the player is on ground and not jumping and the jump key pressed 
	-- /*|| playerController:getPhysicsComponent()->getHasBuoyancy()*/
	if (jumpKeyPressed == true and inAir == false) then
		tryJump = true;
        if (animationBlender:isAnimationActive(AnimationBlender.ANIM_JUMP_START) == false) then
			animationBlender:blend5(AnimationBlender.ANIM_JUMP_START, AnimationBlender.BLEND_WHILE_ANIMATING, 0.5, false);
            animationBlender:blendAndContinue1(AnimationBlender.ANIM_JUMP_START);
		end
		jumpSound:setActivated(true);
		boringTimer = 0;
	end
	-- /*height >= 0.6f && height <= 0.7f*/
	--if (inAir and isJumping) then
	--{
	--	// walkSound:play();
	--	// walkSound:setPitch(0.5f);
	--	// MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Window>("manipulationWindow")->setCaption("Plop");
	--}
	
	if (nearGround == true and playerController:getPhysicsComponent():getVelocity().y < 1 and onSlider == false) then
		-- if (false == inAir && !jumpKeyPressed && playerController:getPhysicsComponent()->getVelocity().y < -1.0)
		jumpCount = 0;
		-- MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Window>("manipulationWindow")->setCaption("Falling");
		--if (animationBlender:isAnimationActive(AnimationBlender.ANIM_FALL) == false) then
			animationBlender:blendExclusive5(AnimationBlender.ANIM_FALL, AnimationBlender.BLEND_WHILE_ANIMATING, 0.5, true);
		--end
		jumpKeyPressed = false;
		-- When falling allow to control via move keys, so that player can change direction to move during falling
        playerController:getPhysicsComponent():applyRequiredForceForVelocity(playerController:getPhysicsComponent():getVelocity() * Vector3(0, 1, 0) + directionMove);
	end
	
	-- Attention: HighFalling false, because when jumping from one platform to another, there is a gab between the plattforms that is more than 4 in any case
	-- so the player will always play the ANIM_JUMP_END, even the player started at the same y position! So take y position into account!
	-- Signal that player is high falling for jump end animation
	-- relativeHeight < 0 -> Player must land lower as he started!
    if (relativeHeight < 0 and height > 7 + gameObject:getMiddle().y) then
		highFalling = true;
	end

	-- Player was high falling and is now at the floor
	if (highFalling == true and height < 0.7) then
		boringTimer = 0;
		acceleration = -0.5;
		tempAnimationSpeed = playerController:getAnimationSpeed() * 4;
        animationBlender:blendAndContinue1(AnimationBlender.ANIM_HIGH_JUMP_END);
        playerController:getPhysicsComponent():applyRequiredForceForVelocity(playerController:getPhysicsComponent():getVelocity() * Vector3(0, 1, 0) + directionMove);

        highFalling = false;
	end
	
	if (inAir == false and groundedOnce == false) then
		--if (animationBlender:isAnimationActive(AnimationBlender.ANIM_FALL) == false) then
		--	animationBlender:blend5(AnimationBlender.ANIM_FALL, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, false);
		--end
		boringTimer = 0;
		-- Play only once!
		walkSound:setPitch(0.35);
		walkSound:setVolume(55);
		walkSound:setActivated(true);
		--smokeParticle:setOffsetPosition(Vector3(0, -0.1, 1));
		--smokeParticle:setActivated(true);
		groundedOnce = true;
	end
	
	if (inAir == true) then
		groundedOnce = false;
	end
    
    if timeSinceLastAttack >= 0 then
		timeSinceLastAttack = timeSinceLastAttack - dt;
	end
    
    if InputDeviceModule:isActionDown(NOWA_A_ATTACK_1) == false then
            canAttack = true;
    end
        
    if InputDeviceModule:isActionDown(NOWA_A_ATTACK_1) and canAttack == true then
        playerController:getStateMachine():changeState(AttackState);
        
        canAttack = false;
        timeSinceLastAttack = 0.2;
    end


	--if (!InputDeviceModule::getInstance()->isKeyDown(NOWA_K_ACTION_1))
	--{
	--	isAttacking = false;
	--}

	--if (InputDeviceModule::getInstance()->isKeyDown(NOWA_K_UP) && !isOnRope)
	--{
	--	for (auto& getGameObjectsFront : playerController:getGameObjectsFront())
	--	{
	--		if (nullptr != getGameObjectsFront && getGameObjectsFront:getCategory() == "Rope")
	--		{
	--			playerController:getStateMachine():changeState(RopeState2D::getName());
	--		}
	--	}
	--}
	--if (!InputDeviceModule::getInstance()->isKeyDown(NOWA_K_UP))
	--{
	--	isOnRope = false;
	--}
	
	animationBlender:addTime(dt * tempAnimationSpeed / animationBlender:getLength());
    
    --log("relHeight: " .. toString(relativeHeight) .. " highFalling: " .. (highFalling and 'true' or 'false'));
end

WalkState["exit"] = function(gameObject)
	 -- Reset orientation in local space
	 --leftFoot:setManuallyControlled(false);
	 --leftFoot:setInheritOrientation(true);
	 --leftFoot:setOrientation(leftFootOriginOrientation);
	 
	 --rightFoot:setManuallyControlled(false);
	 --rightFoot:setInheritOrientation(true);
	 --rightFoot:setOrientation(rightFootOriginOrientation);
end

--WalkState["onEvent"] = function(playerController, event)
--   
--end

-------------------------------------------------------------------------------------

RagDollState = { }

ragDollTime = 5;

RagDollState["enter"] = function(gameObject)
	playerController = gameObject:getPlayerControllerJumpNRunLuaComponent();
	ragDollTime = 5;
    
    prehistoricLax:getPickerComponentFromName("RightHandPicker"):setActivated(false);
    
    playerController:getPhysicsRagDollComponent():setBoneConfigFile("PrehistoricLax2.rag");
	playerController:getPhysicsRagDollComponent():setState("Ragdolling");
    
end

RagDollState["execute"] = function(gameObject, dt)
	-- Let the ragdoll set the velocity last set from movement for 1 second
	if (ragDollTime >= 4) then
		playerController:getPhysicsComponent():applyRequiredForceForVelocity(directionMove);
	end
	if (ragDollTime > 0) then
		ragDollTime = ragDollTime - dt;
	else
		playerController:getStateMachine():changeState(WalkState);
	end
end

RagDollState["exit"] = function(gameObject)
   playerController:getPhysicsRagDollComponent():setBoneConfigFile("PrehistoricLaxPartial.rag");
   playerController:getPhysicsRagDollComponent():setState("PartialRagdolling");
   --playerController:getPhysicsRagDollComponent():setState("Inactive");
   
    -- Attach weapon to hand
    attachSword();
end

-------------------------------------------------------------------------------------

AttackState = { }

AttackState["enter"] = function(gameObject)
	animationBlender:blend5(AnimationBlender.ANIM_WALK_NORTH, AnimationBlender.BLEND_THEN_ANIMATE, 0.2, false);
    animationBlender:setWeight(0.5);
end

AttackState["execute"] = function(gameObject, dt)
    
    -- Prevents to much rotation while fighting etc.
    local resultOrientation = Quaternion(Degree(90), Vector3.UNIT_Y);
    if (direction == LEFT) then
        resultOrientation = Quaternion(Degree(-90), Vector3.UNIT_Y);
    end
    
    playerController:getPhysicsComponent():applyOmegaForceRotateTo(resultOrientation, Vector3.UNIT_Y, 10);
    
	animationBlender:addTime(dt * tempAnimationSpeed / animationBlender:getLength());
end

AttackState["exit"] = function(gameObject)
    animationBlender:setWeight(1);
end

--------------------------------------------------------------------------------------------

PortalState = { }

PortalState["enter"] = function(gameObject)
	-- Release constraint axis
	prehistoricLax:getPhysicsActiveComponent():setConstraintAxis(Vector3.ZERO);
	-- Set walk animation
	--playerController:getAnimationBlender():blendAndContinue1(AnimationBlender.ANIM_WALK_NORTH);
	--playerController:getAnimationBlender():blend5(AnimationBlender.ANIM_WALK_NORTH, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);
	
    local portal = AppStateManager:getGameObjectController():getGameObjectsFromTagName("Portal")[0];
	
	-- Lock player input
	--playerController:lockMovement("mechanics", true);
	
	-- Activate path follow and react when goal is reached
	portal:getAiPathFollowComponent():setActivated(true);
	portal:getAiPathFollowComponent():reactOnPathGoalReached(function()
		local referenceId = gameObject:getReferenceId();
		AppStateManager:getGameObjectController():activateGameObjectComponentsFromReferenceId(referenceId, false);
        portal:getAiPathFollowComponent():setActivated(false);
		playerController:getStateMachine():revertToPreviousState();
	end);
end

PortalState["execute"] = function(gameObject, dt)
	
	if (animationBlender:isAnimationActive(AnimationBlender.ANIM_WALK_NORTH) == false) then
		tempAnimationSpeed = playerController:getAnimationSpeed();
		animationBlender:blend5(AnimationBlender.ANIM_WALK_NORTH, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);
	end
	
	animationBlender:addTime(dt * tempAnimationSpeed / animationBlender:getLength());
end

PortalState["exit"] = function(gameObject)
    prehistoricLax:getAiPathFollowComponent():setActivated(false);
	
	-- Here load next portal world? Or via exit component, but be aware: if world is loaded to fast, maybe this exit state is never reached?
	
	-- playerController:lockMovement("mechanics", false);
end