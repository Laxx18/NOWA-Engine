module("main", package.seeall);

main = { }
mainGameObject = nil;
energyAttribute = nil;
coinAttribute = nil;
energyText = nil;
coinText = nil;
statusWindow = nil;
inventoryBox = nil;
actionsWindow = nil;
attributesComponent = nil;
miniMapComponent = nil;
prehistoricLax = nil;
backgroundMusic = nil;


score = nil;
scoreText = nil;
strength = nil;
strengthText = nil;
speed = nil;
speedText = nil;
playerLevel = nil;
playerLevelText = nil;
killedEnemies = nil;
killedEnemiesText = nil;

energy = nil;
energyText = nil;
energyProgress = nil;
experience = nil;
experienceText = nil;
experienceProgress = nil;

toggleMap = false;
timeSinceLastToggle = 1;

main["connect"] = function(gameObject)
	
	--local success = AppStateManager:getGameProgressModule:loadProgress("MysticLax", false);
	
	mainGameObject = AppStateManager:getGameObjectController():castGameObject(gameObject);
	
    statusWindow = mainGameObject:getMyGUIWindowComponentFromName("StatusWindow");
	inventoryBox = mainGameObject:getMyGUIItemBoxComponentFromName("Inventory");
	actionsWindow = mainGameObject:getMyGUIWindowComponentFromName("ActionsWindow");
    
	miniMapComponent = mainGameObject:getMyGUIMiniMapComponent();

	backgroundMusic = mainGameObject:getSimpleSoundComponent();
	backgroundMusic:setActivated(false);
	
	-- Does not work
	--AppStateManager.setSlowMotion(100);
	
	statusWindow:setActivated(true);
	inventoryBox:setActivated(true);
	actionsWindow:setActivated(true);
	
	prehistoricLax = AppStateManager:getGameObjectController():getGameObjectFromId("929302007");
	
	--mainGameObject:getLuaScriptComponent():callDelayedMethod("ActivatePlayerControllerDelayed", nil, 2);

	AppStateManager:getGameObjectController():activatePlayerController(true, "929302007", true);
	
	--local circleFadeGameObject = AppStateManager:getGameObjectController():getGameObjectFromName("CircleFadeGameObject_0");
	--local compositorEffectOldTvComponent = circleFadeGameObject:getCompositorEffectOldTvComponent();
	--compositorEffectOldTvComponent:setActivated(true);
	--local attributeEffectComponent = circleFadeGameObject:getAttributeEffectComponent();
	--attributeEffectComponent:setActivated(true);
	
	inventoryBox:reactOnMouseButtonClick(function(resourceName, mouseButtonId) 
		-- Rightclick, use potion and remove a quantity from inventory
		if (mouseButtonId == Mouse.MB_RIGHT) then
			local energyResourceName = "EnergyItem";
			if (resourceName == energyResourceName) then
				energy:incrementValueNumber(10);
                energyProgress:setValue(energy:getValueNumber());
				energyText:setCaption("Energy: " .. energyAttribute:getValueNumber());
				inventoryBox:removeQuantity(energyResourceName, 1);
			end
		end
	end);
	
	local takeButton = gameObject:getMyGUIButtonComponentFromName("TakeButton");
	takeButton:reactOnMouseButtonClick(function() 
		-- Activate area search if player is at an items
        log("---> Enter mechanics again!");
		prehistoricLax:getAreaOfInterestComponentFromName("MechanicsArea"):setActivated(true);
	end);
	
	local useButton = gameObject:getMyGUIButtonComponentFromName("UseButton");
	useButton:reactOnMouseButtonClick(function() 
		
	end);
	
	local talkToButton = gameObject:getMyGUIButtonComponentFromName("TalkToButton");
	talkToButton:reactOnMouseButtonClick(function() 
		
	end);
	
	local actionButton = gameObject:getMyGUIButtonComponentFromName("ActionButton");
	actionButton:reactOnMouseButtonClick(function() 
		-- Activate area search if player is at an mechanics
		prehistoricLax:getAreaOfInterestComponentFromName("MechanicsArea"):setActivated(true);
	end);

    attributesComponent = mainGameObject:getAttributesComponent();

    energy = attributesComponent:getAttributeValueByName("Energy");
    score = attributesComponent:getAttributeValueByName("Score");
    strength = attributesComponent:getAttributeValueByName("Strength");
    experience = attributesComponent:getAttributeValueByName("Experience");
    speed = attributesComponent:getAttributeValueByName("Speed");
    playerLevel = attributesComponent:getAttributeValueByName("PlayerLevel");
    killedEnemies = attributesComponent:getAttributeValueByName("KilledEnemies");
    
    
    -- Sets initial values
    energy:setValueNumber(100);
	score:setValueNumber(0);
    strength:setValueNumber(1);
    speed:setValueNumber(8);
    playerLevel:setValueNumber(1);
    killedEnemies:setValueNumber(0);
    
    -- Loads something
    --local success = attributesComponent:loadValue("MysticLax", energy);
    --success = attributesComponent:loadValue("MysticLax", score);
    
    -- Load a possible save game
    --if (Core:getIsGame() == false) then
    -- later test with true, because inventory (which items and quantity?) -> Debug, never tested!
        --local success = AppStateManager:getGameProgressModule():loadProgress(Core:getCurrentSaveGameName(), false);
    --end
    energyProgress = mainGameObject:getMyGUIProgressBarComponentFromName("EnergyProgress");
    energyProgress:setValue(energy:getValueNumber());
    energyText = mainGameObject:getMyGUITextComponentFromName("EnergyText");
	energyText:setCaption("Energy: " .. toString(energy:getValueNumber()));
    scoreText = mainGameObject:getMyGUITextComponentFromName("ScoreText");
	scoreText:setCaption("Score: " .. toString(score:getValueNumber()));
    strengthText = mainGameObject:getMyGUITextComponentFromName("StrengthText");
	strengthText:setCaption("Strength: " .. toString(strength:getValueNumber()));
    experienceProgress = mainGameObject:getMyGUIProgressBarComponentFromName("ExperienceProgress");
    experienceProgress:setValue(experience:getValueNumber());
    experienceText = mainGameObject:getMyGUITextComponentFromName("ExperienceText");
	experienceText:setCaption("Experience: " .. toString(experience:getValueNumber()));
    speedText = mainGameObject:getMyGUITextComponentFromName("SpeedText");
	speedText:setCaption("Speed: " .. toString(speed:getValueNumber()));
    playerLevelText = mainGameObject:getMyGUITextComponentFromName("PlayerLevelText");
	playerLevelText:setCaption("Level: " .. toString(playerLevel:getValueNumber()));
    killedEnemiesText = mainGameObject:getMyGUITextComponentFromName("KilledEnemiesText");
	killedEnemiesText:setCaption("Killed Enemies: " .. toString(killedEnemies:getValueNumber()));
end

--main["ActivatePlayerControllerDelayed"] = function(gameObject)
--	log("ActivatePlayerControllerDelayed");
--	GameObjectController:activatePlayerController(true, 929302007, true);
--end

main["disconnect"] = function()

    -- Reset everything!
	-- Attributes, that have been stored can be loaded/saved, if in game state (isInGameState)
	prehistoricLax:getPhysicsActiveComponent():setConstraintAxis(Vector3(0, 0, 1));
	prehistoricLax:getPlayerControllerJumpNRunLuaComponent():lockMovement("mechanics", false);
    prehistoricLax:getAreaOfInterestComponentFromName("MechanicsArea"):setActivated(false);

	statusWindow:setActivated(false);
	inventoryBox:setActivated(false);
	actionsWindow:setActivated(false);
	--energyAttribute:setValueInt(100);
	--energyText:setCaption("Energy: " .. 100);
	--scoreText:setCaption("Score: " .. 0);
    --killedEnemiesText:setCaption("Killed Enemies: " .. 0);
	--inventoryBox:setActivated(false);
	--actionsWindow:setActivated(false);

	--inventoryBox:clearItems();
	
	--GameObjectController:activatePlayerController(false, "929302007", true);
	
    -- later test with true because of inventory items and quantity to save and load
	--AppStateManager:getGameProgressModule():saveProgress("MysticLax", false, false);
	--attributesComponent:saveValues("MysticLax", false);
	--attributesComponent:saveValue("MysticLax", energyAttribute, false);
	
	--backgroundMusic:setActivated(false);
    
    AppStateManager:getGameObjectController():undoAll();
end

main["update"] = function(dt)
	
	if (timeSinceLastToggle > 0) then
		timeSinceLastToggle = timeSinceLastToggle - dt;
	elseif InputDeviceModule:isActionDown(NOWA_A_MAP) then
			toggleMap = not toggleMap;
			miniMapComponent:showMiniMap(toggleMap);
			timeSinceLastToggle = 1;
	end
end

main["OnEnemyDefaultContact"] = function(gameObject0, gameObject1, contactData)
	--log("OnEnemyDefaultContact GO0: " .. gameObject0:getName() .. " GO1: " .. gameObject1:getName());

	local positionAndNormal = contactData:getPositionAndNormal();
	
	local position = positionAndNormal[0];
	local normal = positionAndNormal[1];
    
    contactData:setFrictionCoefficient(1, 1, 0);
    contactData:setFrictionState(1, 0);
    contactData:setFrictionCoefficient(1, 0, 1);
    contactData:setFrictionState(1, 1);
	
	-- contact in front of a wall, remove any friction, so that the enemy will not stuck in air
	if (normal.x > 0.8 or normal.x < -0.8) then
		contactData:setFrictionCoefficient(0, 0, 0);
        contactData:setFrictionState(0, 0);
		contactData:setFrictionCoefficient(0, 0, 1);
        contactData:setFrictionState(0, 1);
	end
	
    --log(" pos: " .. toString(position) .. " norm: " .. toString(normal));
end

main["OnCharacterDefaultContact"] = function(gameObject0, gameObject1, contactData)
	--log("OnCharacterDefaultContact GO0: " .. gameObject0:getName() .. " GO1: " .. gameObject1:getName());

	local positionAndNormal = contactData:getPositionAndNormal();
	
	local position = positionAndNormal[0];
	local normal = positionAndNormal[1];
    
    contactData:setFrictionCoefficient(1, 1, 0);
    contactData:setFrictionState(1, 0);
    contactData:setFrictionCoefficient(1, 0, 1);
    contactData:setFrictionState(1, 1);
	
	-- contact in front of a wall, remove any friction, so that the enemy will not stuck in air
	if (normal.x > 0.9 or normal.x < -0.9) then
		contactData:setFrictionCoefficient(0, 0, 0);
        contactData:setFrictionState(0, 0);
		contactData:setFrictionCoefficient(0, 0, 1);
        contactData:setFrictionState(0, 1);
	end
	
    --log("--->Normal: ".. toString(normal));
end

main["OnCharacterEnemyContact"] = function(gameObject0, gameObject1, contactData)
	--log("OnCharacterEnemyContact GO0: " .. gameObject0:getName() .. " GO1: " .. gameObject1:getName());

	local positionAndNormal = contactData:getPositionAndNormal();
	
	local position = positionAndNormal[0];
	local normal = positionAndNormal[1];
    
    contactData:setFrictionCoefficient(1, 1, 0);
    contactData:setFrictionState(1, 0);
    contactData:setFrictionCoefficient(1, 0, 1);
    contactData:setFrictionState(1, 1);
	
	if (normal.x > 0.8 or normal.x < -0.8) then
		contactData:setFrictionCoefficient(0, 0, 0);
        contactData:setFrictionState(0, 0);
		contactData:setFrictionCoefficient(0, 0, 1);
        contactData:setFrictionState(0, 1);
	end
	
	--energyAttribute:setValueInt(energyAttribute:getValueInt() - 1);
	--energyText:setCaption("Energy: " .. energyAttribute:getValueInt());
	
	-- Play enemy attack
	--local enemyAnimation = gameObject1:getAnimationComponent():getAnimationBlender();
	--enemyAnimation:blendAndContinue(AnimationBlender.ANIM_ATTACK_1, AnimationBlender.BLEND_WHILE_ANIMATING);
end

main["OnEnemyWeaponCharacterContact"] = function(gameObject0, gameObject1, contactData)
	log("OnEnemyWeaponCharacterContact GO0: " .. gameObject0:getName() .. " GO1: " .. gameObject1:getName());

	local enemy = gameObject0:getConnectedGameObject();
	if (enemy) then
		log("Enemy: " .. enemy:getName());
		--attackAttribute = gameObject1:getAttributesComponent():getAttributeValueByName("CanAttack");
		--attackAttribute:setValueBool(true);
		energy:decrementValueNumber(1);
		energyText:setCaption("Energy: " .. energy:getValueNumber());
		--playerController = gameObject1:getPlayerControllerJumpNRunLuaComponent();
		-- here delayprocess
		-- playerController:setMoveWeight(0);
		-- Smash player
		local direction = enemy:getOrientation() * Vector3.NEGATIVE_UNIT_Z;
		local physicsComponent = gameObject1:getPhysicsRagDollComponent();
		physicsComponent:setVelocity(physicsComponent:getVelocity() * Vector3(0, 1, 0) + direction * 2);
	end
end

main["OnCharacterSliderContact"] = function(gameObject0, gameObject1, contactData)
    local positionAndNormal = contactData:getPositionAndNormal();
	
	local position = positionAndNormal[0];
	local normal = positionAndNormal[1];
    
    contactData:setFrictionCoefficient(1, 1, 0);
    contactData:setFrictionState(1, 0);
    contactData:setFrictionCoefficient(1, 0, 1);
    contactData:setFrictionState(1, 1);
	
	-- contact in front of a wall, remove any friction, so that the enemy will not stuck in air
	if (normal.x > 0.5 or normal.x < -0.5) then
        contactData:setFrictionCoefficient(0, 0, 0);
        contactData:setFrictionState(0, 0);
		contactData:setFrictionCoefficient(0, 0, 1);
        contactData:setFrictionState(0, 1);
        --contactData:setNormalAcceleration(100);
        --log("--->Normal: " .. toString(normal.x));
	end
    
end

main["OnCharacterPlatformContact"] = function(gameObject0, gameObject1, contactData)
    local positionAndNormal = contactData:getPositionAndNormal();
	
	local position = positionAndNormal[0];
	local normal = positionAndNormal[1];
    
    contactData:setFrictionCoefficient(1, 1, 0);
    contactData:setFrictionState(1, 0);
    contactData:setFrictionCoefficient(1, 0, 1);
    contactData:setFrictionState(1, 1);
	
	-- contact in front of a wall, remove any friction, so that the enemy will not stuck in air
	if (normal.x > 0.8 or normal.x < -0.8) then
		contactData:setFrictionCoefficient(0, 0, 0);
        contactData:setFrictionState(0, 0);
		contactData:setFrictionCoefficient(0, 0, 1);
        contactData:setFrictionState(0, 1);
	end
end

main["OnPlayerWeaponEnemyContact"] = function(gameObject0, gameObject1, contact)
    --log("--->OnPlayerWeaponEnemyContact");
    
    gameObject0 = AppStateManager:getGameObjectController():castGameObject(gameObject0);
    gameObject1 = AppStateManager:getGameObjectController():castGameObject(gameObject1);
    -- log("Collision between: " .. gameObject0:getName() .. " vs. " .. gameObject1:getName());
    
    local weapon = nil;
    local enemy = nil;
    
    if (gameObject1:getCategory() == "Enemy") then
        enemy = gameObject1;
        weapon = gameObject0;
    else
        enemy = gameObject0;
        weapon = gameObject1;
    end
    
    local toGameObject = enemy:getPosition() - weapon:getPosition();
    local heading = weapon:getOrientation() * weapon:getDefaultDirection();
    local dot = heading:dotProduct(toGameObject:normalisedCopy());
    
    local velocity0 = enemy:getPhysicsComponent():getVelocity();
    local velocity1 = weapon:getPhysicsComponent():getVelocity();
    local velocityDiff = velocity0 - velocity1;
    local velocityDot = velocity1:dotProduct(velocityDiff:normalisedCopy());
    
    log("Weapon:  Collision between: " .. weapon:getName() .. " vs. " .. enemy:getName() .. " velDiff: " .. toString(5.0 - (dot * velocityDot)));
    
    --oldHeight = rectangleComponent:getHeight(0);
    --local height = MathHelper:lowPassFilter(5.0 - (dot * velocityDot), oldHeight, 0.01);
end