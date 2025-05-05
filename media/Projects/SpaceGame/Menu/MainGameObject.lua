module("MainGameObject", package.seeall);

-- Scene: Menu.scene

require("init");

MainGameObject = {}

mainGameObject = nil;

MainGameObject["connect"] = function(gameObject)
    mainGameObject = AppStateManager:getGameObjectController():castGameObject(gameObject);

    PointerManager:showMouse(true);
    OgreALModule:setContinue(true);
    
    local leftController = mainGameObject:getMyGUIPositionControllerComponentFromName("LeftController");
    local rightController = mainGameObject:getMyGUIPositionControllerComponentFromName("RightController");
	
	local continueButton = mainGameObject:getMyGUIButtonComponentFromName("ContinueButton");
	continueButton:reactOnMouseButtonClick(function() 
		AppStateManager:popAppState();
	end);
	
	continueButton:reactOnMouseEnter(function() 
        rightController:setActivated(false);
		leftController:setSourceId(continueButton:getId());
		leftController:setCoordinate(Vector4(0.37, 0.2, 0, 0));
		leftController:setActivated(true);

		local clickSound = mainGameObject:getSimpleSoundComponent();
		clickSound:setActivated(true);
	end);
	
	continueButton:reactOnMouseLeave(function() 
        leftController:setActivated(false);
		rightController:setSourceId(continueButton:getId());
		rightController:setCoordinate(Vector4(0.4, 0.2, 0, 0));
		rightController:setActivated(true);

		local clickSound = mainGameObject:getSimpleSoundComponent();
		clickSound:setActivated(false);
	end);
	
	local newButton = mainGameObject:getMyGUIButtonComponentFromName("NewButton");
	newButton:reactOnMouseButtonClick(function() 
		AppStateManager:popAllAndPushAppState("GameState");
	end);
	
	newButton:reactOnMouseEnter(function() 
        rightController:setActivated(false);
		leftController:setSourceId(newButton:getId());
		leftController:setCoordinate(Vector4(0.37, 0.35, 0, 0));
		leftController:setActivated(true);

		local clickSound = mainGameObject:getSimpleSoundComponent();
		clickSound:setActivated(true);
	end);
	
	newButton:reactOnMouseLeave(function() 
        leftController:setActivated(false);
		rightController:setSourceId(newButton:getId());
		rightController:setCoordinate(Vector4(0.4, 0.35, 0, 0));
		rightController:setActivated(true);

		local clickSound = mainGameObject:getSimpleSoundComponent();
		clickSound:setActivated(false);
	end);
	
	local loadButton = mainGameObject:getMyGUIButtonComponentFromName("LoadButton");
	loadButton:reactOnMouseButtonClick(function() 
		AppStateManager:pushAppState("LoadMenuState");
	end);
	
	loadButton:reactOnMouseEnter(function() 
        rightController:setActivated(false);
		leftController:setSourceId(loadButton:getId());
		leftController:setCoordinate(Vector4(0.37, 0.5, 0, 0));
		leftController:setActivated(true);

		local clickSound = mainGameObject:getSimpleSoundComponent();
		clickSound:setActivated(true);
	end);
	
	loadButton:reactOnMouseLeave(function() 
        leftController:setActivated(false);
		rightController:setSourceId(loadButton:getId());
		rightController:setCoordinate(Vector4(0.4, 0.5, 0, 0));
		rightController:setActivated(true);

		local clickSound = mainGameObject:getSimpleSoundComponent();
		clickSound:setActivated(false);
	end);
	
	local saveButton = mainGameObject:getMyGUIButtonComponentFromName("SaveButton");
	saveButton:reactOnMouseButtonClick(function() 
		AppStateManager:pushAppState("SaveMenuState");
	end);
	
	saveButton:reactOnMouseEnter(function() 
        rightController:setActivated(false);
		leftController:setSourceId(saveButton:getId());
		leftController:setCoordinate(Vector4(0.37, 0.65, 0, 0));
		leftController:setActivated(true);

		local clickSound = mainGameObject:getSimpleSoundComponent();
		clickSound:setActivated(true);
	end);
	
	saveButton:reactOnMouseLeave(function() 
        leftController:setActivated(false);
		rightController:setSourceId(saveButton:getId());
		rightController:setCoordinate(Vector4(0.4, 0.65, 0, 0));
		rightController:setActivated(true);

		local clickSound = mainGameObject:getSimpleSoundComponent();
		clickSound:setActivated(false);
	end);
	
	local exitButton = mainGameObject:getMyGUIButtonComponentFromName("ExitButton");
	exitButton:reactOnMouseButtonClick(function() 
		--AppStateManager:exitGame();
	end);
	
	exitButton:reactOnMouseEnter(function() 
        rightController:setActivated(false);
		leftController:setSourceId(exitButton:getId());
		leftController:setCoordinate(Vector4(0.37, 0.8, 0, 0));
		leftController:setActivated(true);

		local clickSound = mainGameObject:getSimpleSoundComponent();
		clickSound:setActivated(true);
	end);
	
	exitButton:reactOnMouseLeave(function()
        leftController:setActivated(false);
		rightController:setSourceId(exitButton:getId());
		rightController:setCoordinate(Vector4(0.4, 0.8, 0, 0));
		rightController:setActivated(true);

		local clickSound = mainGameObject:getSimpleSoundComponent();
		clickSound:setActivated(false);
	end);
	
    -- If game has already started and was just paused, continue
    if (AppStateManager:hasAppStateStarted("GameState") == true) then
        continueButton:setActivated(true);
        saveButton:setEnabled(true);
    end
end

MainGameObject["disconnect"] = function()
    OgreALModule:setContinue(false);
    
    local leftController = mainGameObject:getMyGUIPositionControllerComponentFromName("LeftController");
    local rightController = mainGameObject:getMyGUIPositionControllerComponentFromName("RightController");
    leftController:setSourceId("0");
    rightController:setSourceId("0");
    leftController:setActivated(false);
    rightController:setActivated(false);

    mainGameObject:getMyGUIButtonComponentFromName("ContinueButton"):setActivated(false);
    mainGameObject:getMyGUIButtonComponentFromName("SaveButton"):setEnabled(false);
end