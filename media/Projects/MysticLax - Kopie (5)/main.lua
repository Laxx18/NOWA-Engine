module("main", package.seeall);

main = { }
mainGameObject = nil;
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
attackForce = 0.1;
oldAttackForce = 0.1;
cummulatedAttackForce = 0;

attackForcePlayer = 0.1;
oldAttackForcePlayer = 0.1;
cummulatedAttackForcePlayer = 0;
isPlayerAttacking = false;

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
				energyText:setCaption("Energy: " .. energy:getValueNumber());
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
    
    if (EventType.PlayerAttackEvent ~= nil) then
		AppStateManager:getScriptEventManager():registerEventListener(EventType.PlayerAttackEvent, main["onPlayerAttacking"]);
	end
end

--main["ActivatePlayerControllerDelayed"] = function(gameObject)
--	log("ActivatePlayerControllerDelayed");
--	GameObjectController:activatePlayerController(true, 929302007, true);
--end

main["disconnect"] = function()

    -- Reset everything!
	-- Attributes, that have been stored can be loaded/saved, if in game state (isInGameState)
	--prehistoricLax:getPhysicsActiveComponent():setConstraintAxis(Vector3(0, 0, 1));
	prehistoricLax:getPlayerControllerJumpNRunLuaComponent():lockMovement("mechanics", false);
    --prehistoricLax:getAreaOfInterestComponentFromName("MechanicsArea"):setActivated(false);

	--statusWindow:setActivated(false);
	--inventoryBox:setActivated(false);
	--actionsWindow:setActivated(false);
	--energy:setValueInt(100);
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
	--attributesComponent:saveValue("MysticLax", energy, false);
	
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
	--log("#####OnCharacterEnemyContact GO0: " .. gameObject0:getName() .. " GO1: " .. gameObject1:getName());

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
    
    local character = nil;
    local enemy = nil;
    
    if (gameObject1:getCategory() == "Enemy") then
        enemy = gameObject1;
        character = gameObject0;
    else
        enemy = gameObject0;
        character = gameObject1;
    end
    
    if (normal.x > -0.1 and normal.x < 0.21) then
        -- Character above enemy
        --local aiLuaComponent = enemy:getAiLuaComponent();
        --local movingBehavior = aiLuaComponent:getMovingBehavior();
        --movingBehavior:setBehavior(MovingBehavior.STOP);
        local playerController = prehistoricLax:getPlayerControllerJumpNRunLuaComponent();
        playerController:getPhysicsComponent():applyRequiredForceForVelocity(playerController:getPhysicsComponent():getVelocity() * Vector3(0, 1, 0) + Vector3(-5, 0, 0));
        
    end
    
	
	--energy:setValueInt(energy:getValueInt() - 1);
	--energyText:setCaption("Energy: " .. energy:getValueInt());
	
	-- Play enemy attack
	--local enemyAnimation = gameObject1:getAnimationComponent():getAnimationBlender();
	--enemyAnimation:blendAndContinue(AnimationBlender.ANIM_ATTACK_1, AnimationBlender.BLEND_WHILE_ANIMATING);
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

main["OnEnemyWeaponCharacterContact"] = function(gameObject0, gameObject1, contactData)
--    if (gameObject1 ~= nil) then
--        log("OnEnemyWeaponCharacterContact GO0: " .. gameObject0:getName() .. " GO1: " .. gameObject1:getName());

--        local enemy = gameObject0:getConnectedGameObject();
--        if (enemy) then
--            log("Enemy: " .. enemy:getName());
--            --attackAttribute = gameObject1:getAttributesComponent():getAttributeValueByName("CanAttack");
--            --attackAttribute:setValueBool(true);
--            energy:decrementValueNumber(1);
--            energyText:setCaption("Energy: " .. energy:getValueNumber());
--            --playerController = gameObject1:getPlayerControllerJumpNRunLuaComponent();
--            -- here delayprocess
--            -- playerController:setMoveWeight(0);
--            -- Smash player
--            local direction = enemy:getOrientation() * Vector3.NEGATIVE_UNIT_Z;
--            local physicsComponent = gameObject1:getPhysicsRagDollComponent();
--            physicsComponent:setVelocity(physicsComponent:getVelocity() * Vector3(0, 1, 0) + direction * 2);
--        end
--    end

    gameObject0 = AppStateManager:getGameObjectController():castGameObject(gameObject0);
    gameObject1 = AppStateManager:getGameObjectController():castGameObject(gameObject1);
    
    local enemyWeapon = nil;
    local character = nil;
    
    if (gameObject1:getCategory() == "Character") then
        character = gameObject1;
        enemyWeapon = gameObject0;
    else
        character = gameObject0;
        enemyWeapon = gameObject1;
    end
    
    local toGameObject = character:getPosition() - enemyWeapon:getPosition();
    local heading = enemyWeapon:getOrientation() * enemyWeapon:getDefaultDirection();
    local dot = heading:dotProduct(toGameObject:normalisedCopy());
    
    local velocity0 = character:getPhysicsComponent():getVelocity();
    local velocity1 = enemyWeapon:getPhysicsComponent():getVelocity();
    local velocityDiff = velocity0 - velocity1;
    local velocityDot = velocity1:dotProduct(velocityDiff:normalisedCopy());
    
    oldAttackForce = attackForce;
    attackForce = MathHelper:lowPassFilter(5.0 - (dot * velocityDot), oldAttackForce, 0.01);
    
    local enemy = enemyWeapon:getReferenceComponent():getTargetGameObject();
    if (enemy == nil) then
       do return end; 
    end
    local strength = enemy:getAttributesComponent():getAttributeValueByName("Strength");
    
    --cummulatedAttackForce, fraction = math.modf(cummulatedAttackForce + attackForce * strength:getValueNumber());
    --local finalAttackForce = cummulatedAttackForce + fraction;
    
    
    cummulatedAttackForce = cummulatedAttackForce + attackForce * strength:getValueNumber();
    local finalAttackForce = cummulatedAttackForce;
    
    local animationBlender = enemy:getAnimationComponent():getAnimationBlender();
    if (finalAttackForce > 1 and animationBlender:getTimePosition() >= animationBlender:getLength() - 0.2) then
       cummulatedAttackForce = 0;
       finalAttackForce = 1;
       
       log("Weapon:  Collision between: " .. enemyWeapon:getName() .. " vs. " .. character:getName() .. " attack force: " .. toString(finalAttackForce));
       
       local data = contactData:getPositionAndNormal();
       local contactPosition = data[0];
       local hitParticle = mainGameObject:getParticleUniverseComponentFromName("HitParticle");
       
       hitParticle:setGlobalPosition(contactPosition);
       if (hitParticle:isPlaying() == false or hitParticle:isActivated() == false) then
            hitParticle:setActivated(true);
       end

       energy:decrementValueNumber(finalAttackForce);
       if (energy:getValueNumber() < 0) then
           energy:setValueNumber(100);
           
           -- in ragdoll state, load game after 5 seconds
           
           -- Sends event to prehistoric lax, to go to ragdoll state and enemy shall not attack anymore
           local eventData = {};
           AppStateManager:getScriptEventManager():queueEvent(EventType.PlayerDeadEvent, eventData);
           AppStateManager:getScriptEventManager():queueEvent(EventType.EnemyIdleEvent, eventData);
       end
    
       energyProgress:setValue(energy:getValueNumber());
       energyText:setCaption("Energy: " .. energy:getValueNumber());
    end

--    local animationBlender = enemy:getAnimationComponent():getAnimationBlender();
--    --if (animationBlender:isAnimationActive(AnimationBlender.ANIM_ATTACK_1) == true or animationBlender:isAnimationActive(AnimationBlender.ANIM_ATTACK_2) == true) then
    
--    if (animationBlender:getTimePosition() >= animationBlender:getLength() - 0.4) then
--       local finalAttackForce = math.floor(attackForce * strength:getValueNumber();
        
--       log("Weapon:  Collision between: " .. enemyWeapon:getName() .. " vs. " .. character:getName() .. " attack force: " .. toString(finalAttackForce));
    
--       energy:decrementValueNumber(finalAttackForce);
--       energyProgress:setValue(finalAttackForce);
--       energyText:setCaption("Energy: " .. energy:getValueNumber());
--    end
end

main["onPlayerAttacking"] = function(eventData)
	isPlayerAttacking = eventData["isActive"];
    log("#############isInAttackState: " .. (isPlayerAttacking and 'true' or 'false'));
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
    
    local player = weapon:getReferenceComponent():getTargetGameObject();
    local energyEnemy = enemy:getAttributesComponent():getAttributeValueByName("Energy");
    
    local toGameObject = enemy:getPosition() - weapon:getPosition();
    local heading = weapon:getOrientation() * weapon:getDefaultDirection();
    local dot = heading:dotProduct(toGameObject:normalisedCopy());
    
    local velocity0 = enemy:getPhysicsComponent():getVelocity();
    local velocity1 = weapon:getPhysicsComponent():getVelocity();
    local velocityDiff = velocity0 - velocity1;
    local velocityDot = velocity1:dotProduct(velocityDiff:normalisedCopy());
    
    oldAttackForcePlayer = attackForcePlayer;
    attackForcePlayer = MathHelper:lowPassFilter(5.0 - (dot * velocityDot), oldAttackForcePlayer, 0.01);
    
    --cummulatedAttackForce, fraction = math.modf(cummulatedAttackForce + attackForce * strength:getValueNumber());
    --local finalAttackForce = cummulatedAttackForce + fraction;
    
    
    cummulatedAttackForcePlayer = cummulatedAttackForcePlayer + attackForcePlayer * strength:getValueNumber();
    local finalAttackForcePlayer = cummulatedAttackForcePlayer;
    
    if (isPlayerAttacking == true) then
       cummulatedAttackForcePlayer = 0;
       finalAttackForcePlayer = 1;
       
       log("Weapon:  Collision between: " .. weapon:getName() .. " vs. " .. player:getName() .. " attack force: " .. toString(finalAttackForcePlayer));
       
       energyEnemy:decrementValueNumber(finalAttackForcePlayer);
       if (energyEnemy:getValueNumber() == 0) then
           killedEnemies:incrementValueNumber(1);
           killedEnemiesText:setCaption("Killed Enemies: " .. killedEnemies:getValueNumber());
           local eventData = {};
           eventData["enemyId"] = enemy:getId();
           AppStateManager:getScriptEventManager():queueEvent(EventType.EnemyDeadEvent, eventData);
       end
       
    end
end