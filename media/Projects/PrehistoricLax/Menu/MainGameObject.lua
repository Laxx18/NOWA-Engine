module("MainGameObject", package.seeall);

-- Scene: Menu.scene

require("init");

MainGameObject = {}

mainGameObject = nil;

MainGameObject["connect"] = function(gameObject)
    mainGameObject = AppStateManager:getGameObjectController():castGameObject(gameObject);

    PointerManager:showMouse(true);
    OgreALModule:setContinue(true);
	AppStateManager:getCameraManager():setMoveCameraWeight(0);
    AppStateManager:getCameraManager():setRotateCameraWeight(0);
    
    local leftController = mainGameObject:getMyGUIPositionControllerComponentFromName("LeftController");
    local rightController = mainGameObject:getMyGUIPositionControllerComponentFromName("RightController");
    
    local continueButton = mainGameObject:getMyGUIButtonComponentFromName("ContinueButton");
    continueButton:reactOnMouseButtonClick(function() 
        leftController:setActivated(false);
        rightController:setSourceId(continueButton:getId());
        rightController:setCoordinate(Vector4(0.4, 0.2, 0, 0));
        rightController:setActivated(true);
        AppStateManager:popAppState();
    end);
    
    continueButton:reactOnMouseEnter(function() 
        rightController:setActivated(false);
        leftController:setSourceId(continueButton:getId());
        leftController:setCoordinate(Vector4(0.37, 0.2, 0, 0));
        leftController:setActivated(true);

        local clickSound = mainGameObject:getSimpleSoundComponent();
        if clickSound ~= nil then
            clickSound:setActivated(true);
        end
    end);
    
    continueButton:reactOnMouseLeave(function() 
        leftController:setActivated(false);
        rightController:setSourceId(continueButton:getId());
        rightController:setCoordinate(Vector4(0.4, 0.2, 0, 0));
        rightController:setActivated(true);

        local clickSound = mainGameObject:getSimpleSoundComponent();
        if clickSound ~= nil then
            clickSound:setActivated(false);
        end
    end);
    
    local newButton = mainGameObject:getMyGUIButtonComponentFromName("NewButton");
    newButton:reactOnMouseButtonClick(function() 
        AppStateManager:popAllAndPushAppState("PrehistoryState");
    end);
    
    newButton:reactOnMouseEnter(function() 
        rightController:setActivated(false);
        leftController:setSourceId(newButton:getId());
        leftController:setCoordinate(Vector4(0.37, 0.35, 0, 0));
        leftController:setActivated(true);

        local clickSound = mainGameObject:getSimpleSoundComponent();
        if clickSound ~= nil then
            clickSound:setActivated(true);
        end
    end);
    
    newButton:reactOnMouseLeave(function() 
        leftController:setActivated(false);
        rightController:setSourceId(newButton:getId());
        rightController:setCoordinate(Vector4(0.4, 0.35, 0, 0));
        rightController:setActivated(true);

        local clickSound = mainGameObject:getSimpleSoundComponent();
        if clickSound ~= nil then
            clickSound:setActivated(false);
        end
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
        if clickSound ~= nil then
            clickSound:setActivated(true);
        end
    end);
    
    loadButton:reactOnMouseLeave(function() 
        leftController:setActivated(false);
        rightController:setSourceId(loadButton:getId());
        rightController:setCoordinate(Vector4(0.4, 0.5, 0, 0));
        rightController:setActivated(true);

        local clickSound = mainGameObject:getSimpleSoundComponent();
        if clickSound ~= nil then
            clickSound:setActivated(false);
        end
    end);
    
    local configurationButton = mainGameObject:getMyGUIButtonComponentFromName("ConfigurationButton");
    configurationButton:reactOnMouseButtonClick(function() 
        AppStateManager:pushAppState("ConfigurationState");
    end);
    
    configurationButton:reactOnMouseEnter(function() 
        rightController:setActivated(false);
        leftController:setSourceId(configurationButton:getId());
        leftController:setCoordinate(Vector4(0.37, 0.65, 0, 0));
        leftController:setActivated(true);

        local clickSound = mainGameObject:getSimpleSoundComponent();
        if clickSound ~= nil then
            clickSound:setActivated(true);
        end
    end);
    
    configurationButton:reactOnMouseLeave(function() 
        leftController:setActivated(false);
        rightController:setSourceId(configurationButton:getId());
        rightController:setCoordinate(Vector4(0.4, 0.65, 0, 0));
        rightController:setActivated(true);

        local clickSound = mainGameObject:getSimpleSoundComponent();
        if clickSound ~= nil then
            clickSound:setActivated(false);
        end
    end);
    
    local exitButton = mainGameObject:getMyGUIButtonComponentFromName("ExitButton");
    exitButton:reactOnMouseButtonClick(function() 
        AppStateManager:exitGame();
    end);
    
    exitButton:reactOnMouseEnter(function() 
        rightController:setActivated(false);
        leftController:setSourceId(exitButton:getId());
        leftController:setCoordinate(Vector4(0.37, 0.8, 0, 0));
        leftController:setActivated(true);

        local clickSound = mainGameObject:getSimpleSoundComponent();
        if clickSound ~= nil then
            clickSound:setActivated(true);
        end
    end);
    
    exitButton:reactOnMouseLeave(function()
        leftController:setActivated(false);
        rightController:setSourceId(exitButton:getId());
        rightController:setCoordinate(Vector4(0.4, 0.8, 0, 0));
        rightController:setActivated(true);

        local clickSound = mainGameObject:getSimpleSoundComponent();
        if clickSound ~= nil then
            clickSound:setActivated(false);
        end
    end);
    
    -- If game has already started and was just paused, continue
    if (AppStateManager:hasAppStateStarted("GameState") == true) then
        continueButton:setActivated(true);
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
	
	AppStateManager:getCameraManager():setMoveCameraWeight(1);
    AppStateManager:getCameraManager():setRotateCameraWeight(1);

    mainGameObject:getMyGUIButtonComponentFromName("ContinueButton"):setActivated(false);
    
    AppStateManager:getGameObjectController():undoAll();
end