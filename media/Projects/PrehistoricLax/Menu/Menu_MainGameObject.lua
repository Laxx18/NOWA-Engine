module("Menu_MainGameObject", package.seeall);

-- Scene: Menu.scene

require("init");

Menu_MainGameObject = {}

mainGameObject = nil;

---------------------------------------------------------------------------------------------------
-- Player selection. Everything below is new, the rest of this script is as it was.
---------------------------------------------------------------------------------------------------
local playerWindow = nil;
local nameEdit = nil;
local playerList = nil;

-- The player who has been chosen. Nil as long as the name entry is shown.
local currentPlayer = nil;
-- The player the message box asks about.
local playerToDelete = nil;

-- The edit box is a MyGUITextComponent with ReadOnly = false. Both are the only places that know it.
local function getEnteredText()
    return nameEdit:getCaption();
end

local function setEnteredText(text)
    nameEdit:setCaption(text);
end

-- The result of the message box comes in as the second argument of the result function. Is
-- MessageBoxStyle not bound to Lua, nothing is deleted: better a click too little than a lost player.
local function isYes(result)
    if (nil ~= MessageBoxStyle) then
        return result == MessageBoxStyle.Yes;
    end

    log("[Menu] MessageBoxStyle is not bound to Lua - the message box counts as NOT confirmed.");
    return false;
end

local function fillPlayerList()
    local players = readPlayers();
    table.sort(players, function(a, b) return string.lower(a) < string.lower(b); end);

    playerList:setItemCount(0);
    for i = 1, #players do
        playerList:addItem(players[i]);
    end
end

-- The menu points slide in only now. In the scene the controllers of New, Load, Configuration and Exit are
-- NOT activated, this does it. The controller of Continue stays as it was: Continue is only for a game that is
-- running and was paused.
local function showMenu()
    playerWindow:setActivated(false);

    local controllerNames = { "NewController", "LoadController", "ConfigurationController", "ExitController" };
    for i = 1, #controllerNames do
        mainGameObject:getMyGUIPositionControllerComponentFromName(controllerNames[i]):setActivated(true);
    end
end

-- The game starts with its own, empty global values. Without this a second game in the same session would think
-- the first one is still running. What is started is handed over in the Core: the name of the player alone
-- means his newest save game, or a new run if he has none, see splitStartName in init.lua.
local function startGame(startName, stateName)
    local gameProgressModule = AppStateManager:getGameProgressModule2(stateName);
    if (nil ~= gameProgressModule) then
        gameProgressModule:clearGlobalValues();
    end

    Core:setCurrentSaveGameName(startName or DEFAULT_PLAYER_NAME);
    AppStateManager:popAllAndPushAppState(stateName);
end

-- Called by the message box, its ResultEventName in the scene is "onDeletePlayerResult".
Menu_MainGameObject["onDeletePlayerResult"] = function(messageBox, result)
    if (nil ~= playerToDelete and true == isYes(result)) then
        deletePlayer(playerToDelete);
        setEnteredText("");
        fillPlayerList();
    end

    playerToDelete = nil;
end

local function setupPlayerSelection(gameIsRunning)
    currentPlayer = nil;
    playerToDelete = nil;

    playerWindow = mainGameObject:getMyGUIWindowComponentFromName("PlayerWindow");
    nameEdit = mainGameObject:getMyGUITextComponentFromName("PlayerNameEdit");
    playerList = mainGameObject:getMyGUIListBoxComponentFromName("PlayerList");

    -- Picking a player from the list puts his name into the edit box.
    playerList:reactOnSelected(function(index)
        if (playerList:getSelectedIndex() ~= -1) then
            setEnteredText(playerList:getItemText(playerList:getSelectedIndex()));
        end
    end);

    local function confirmPlayer()
        local playerName = cleanPlayerName(getEnteredText());
        if ("" == playerName) then
            do return end;
        end

        -- A known name is a returning player, an unknown one a new player.
        local storedName = findPlayer(playerName);
        if (nil == storedName) then
            storedName = addPlayer(playerName);
        end

        currentPlayer = storedName;

        -- The load menu and the game read from the Core who is playing.
        Core:setCurrentSaveGameName(storedName);

        showMenu();
    end

    mainGameObject:getMyGUIButtonComponentFromName("PlayerPlayButton"):reactOnMouseButtonClick(function()
        confirmPlayer();
    end);

    -- The virtual keyboard (gamepad / Steam Deck). The pad focus clicks the ABC button like any other button.
    local keyboard = mainGameObject:getVirtualKeyboardComponent();
    keyboard:setTargetId(tostring(nameEdit:getId()));
    keyboard:setMaxLength(PLAYER_NAME_MAX_LENGTH);

    -- No spaces in a player name: a typed space is removed again at once. setText fires this callback once more,
    -- but then without a space, so it ends there.
    keyboard:reactOnTextChanged(function(text)
        if (nil ~= string.find(text, "%s")) then
            keyboard:setText((string.gsub(text, "%s", "")));
        end
    end);

    -- OK on the keyboard: the typed text is already in the edit box, so go on like a click on Play. Cancel
    -- restores the old text by itself.
    keyboard:reactOnClosed(function(accepted, text)
        if (true == accepted) then
            confirmPlayer();
        end
    end);

    mainGameObject:getMyGUIButtonComponentFromName("PlayerKeyboardButton"):reactOnMouseButtonClick(function()
        keyboard:open();
    end);

    mainGameObject:getMyGUIButtonComponentFromName("PlayerDeleteButton"):reactOnMouseButtonClick(function()
        local storedName = findPlayer(cleanPlayerName(getEnteredText()));
        if (nil == storedName) then
            do return end;
        end

        playerToDelete = storedName;

        local messageBox = mainGameObject:getMyGUIMessageBoxComponent();
        messageBox:setTitle("Delete player");
        messageBox:setStylesCount(2);
        messageBox:setStyle(0, "Yes");
        messageBox:setStyle(1, "No");
        messageBox:setMessage("Delete the player '" .. storedName .. "' and ALL of his save games?\nThis can not be undone.");
        messageBox:setActivated(true);
    end);

    if (true == gameIsRunning) then
        -- A game that was only paused: the player is known, no name entry. The Core still holds what was handed
        -- over when the game was started.
        local startName = Core:getCurrentSaveGameName();
        if (nil ~= startName and "" ~= startName) then
            currentPlayer = splitStartName(startName);
            -- The load menu reads the player from the Core, which still holds the full save game name.
            Core:setCurrentSaveGameName(currentPlayer);
        end

        showMenu();
        do return end;
    end

    fillPlayerList();
end


Menu_MainGameObject["connect"] = function(gameObject)
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
        if (true == hasSaveGame(currentPlayer)) then
            -- Known player: straight into the game, loadGameOnStart takes his newest save game.
            startGame(currentPlayer, "GameState");
        else
            -- New player: the intro first, it hands over to the GameState afterwards.
            startGame(currentPlayer, "PrehistoryState");
        end
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
    local gameIsRunning = AppStateManager:hasAppStateStarted("GameState") == true;
    if (gameIsRunning == true) then
        continueButton:setActivated(true);
    end

    -- Who is playing? Before the menu points slide in the player has to say so.
    setupPlayerSelection(gameIsRunning);
end

Menu_MainGameObject["disconnect"] = function()
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