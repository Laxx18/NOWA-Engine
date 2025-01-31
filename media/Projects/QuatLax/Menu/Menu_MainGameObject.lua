module("Menu_MainGameObject", package.seeall);

Menu_MainGameObject = {}

-- Scene: Menu

require("init");

menu_MainGameObject = nil;
player1 = nil;
player2 = nil;
player1Transform = nil;
player2Transform = nil;
player1Input = nil;
player2Input = nil;

onePlayerCombo = nil;
twoPlayerButton = nil;
twoPlayerPositionController = nil;
onePlayerColorButton = nil;
twoPlayerColorButton = nil;
onePlayerName = nil;
twoPlayerName = nil;
onePlayerPush = nil;
twoPlayerPush = nil;

-- Sets racer images
local images = {}
for i = 1, 20 do
    table.insert(images, string.format("racer_%02dD.png", i))
end

local currentIndexPlayer1 = 0
local currentIndexPlayer2 = 0

Menu_MainGameObject["connect"] = function(gameObject)
    menu_MainGameObject = AppStateManager:getGameObjectController():castGameObject(gameObject);
    local onePlayerButton = menu_MainGameObject:getMyGUIButtonComponentFromName("OnePlayerButton");
    onePlayerCombo = menu_MainGameObject:getMyGUIComboBoxComponentFromName("OnePlayerCombo");
    onePlayerColorButton= menu_MainGameObject:getMyGUIButtonComponentFromName("OnePlayerColor");
    
    local twoPlayerButton = menu_MainGameObject:getMyGUIButtonComponentFromName("TwoPlayerButton");
    twoPlayerCombo = menu_MainGameObject:getMyGUIComboBoxComponentFromName("TwoPlayerCombo");
    twoPlayerPositionController = menu_MainGameObject:getMyGUIPositionControllerComponentFromName("TwoPlayerColorController");
    twoPlayerPositionController:setActivated(false);
    twoPlayerColorButton = menu_MainGameObject:getMyGUIButtonComponentFromName("TwoPlayerColor");
    twoPlayerColorButton:setActivated(false);
    onePlayerName = menu_MainGameObject:getMyGUITextComponentFromName("OnePlayerName");
    twoPlayerName = menu_MainGameObject:getMyGUITextComponentFromName("TwoPlayerName");
    twoPlayerName:setActivated(false);
    onePlayerPush = menu_MainGameObject:getMyGUITextComponentFromName("OnePlayerPush");
    twoPlayerPush = menu_MainGameObject:getMyGUITextComponentFromName("TwoPlayerPush");
    
    local startButton = menu_MainGameObject:getMyGUIButtonComponentFromName("StartButton");
    startButton:setEnabled(false);
  
    player1 = AppStateManager:getGameObjectController():getGameObjectFromId("449055701");
    player1:getDatablockPbsComponent():setDiffuseTextureName(nextColorPlayer1());
    player2 = AppStateManager:getGameObjectController():getGameObjectFromId("1220359006");
    player2:getDatablockPbsComponent():setDiffuseTextureName(nextColorPlayer2());
    
    player1Input = player1:getInputDeviceComponent();
    player2Input = player2:getInputDeviceComponent();
    
        -- todo: React if a button on corresponding input device has been pushed and use react function to change animation to jump_up
    player1Transform = player1:getTransformEaseComponent();
    player2Transform = player2:getTransformEaseComponent();
    
    actualizeDevices1();
    actualizeDevices2();
   
    onePlayerCombo:reactOnSelected(function(index)
       
       local selectedDevice = onePlayerCombo:getItemText(index);
       if (player2Input:getDeviceName() == selectedDevice) then
           actualizeDevices1();
       else
           player1Input:setDeviceName(selectedDevice);
       end
       
       if (player1Input:hasValidDevice()) then
            if (player2Input:hasValidDevice() == false) then
                actualizeDevices2();
            end
            AppStateManager:getGameProgressModule():setGlobalStringValue("PlayerOneDeviceName", selectedDevice);
            -- If for two player check if player two has also a valid device
            if (AppStateManager:getGameProgressModule():getGlobalValue("TwoPlayer")) then
                 startButton:setEnabled(player2Input:hasValidDevice());
                 if (player1Input:getInputDeviceModule():isKeyboardDevice()) then
                     onePlayerPush:setCaption("Press return key");
                 else
                     onePlayerPush:setCaption("Press A button");
                 end
                 onePlayerPush:setActivated(player1Input:hasValidDevice());
            else
                 if (player1Input:getInputDeviceModule():isKeyboardDevice()) then
                     onePlayerPush:setCaption("Press return key");
                 else
                     onePlayerPush:setCaption("Press A button");
                 end
                 onePlayerPush:setActivated(player1Input:hasValidDevice());
                startButton:setEnabled(true);
            end
       else
               startButton:setEnabled(false);
       end
    end);
    
    twoPlayerCombo:reactOnSelected(function(index)
       
       local selectedDevice =  twoPlayerCombo:getItemText(index);
       if (player1Input:getDeviceName() == selectedDevice) then
           actualizeDevices2();
       else
           player2Input:setDeviceName(selectedDevice);
       end
       
       if (player2Input:hasValidDevice()) then
             if (player1Input:hasValidDevice() == false) then
                actualizeDevices1();
            end
            AppStateManager:getGameProgressModule():setGlobalStringValue("PlayerTwoDeviceName", selectedDevice);
            startButton:setEnabled(player2Input:hasValidDevice());
             if (player2Input:getInputDeviceModule():isKeyboardDevice()) then
                     twoPlayerPush:setCaption("Press return key");
                 else
                     twoPlayerPush:setCaption("Press A button");
                 end
            twoPlayerPush:setActivated(player2Input:hasValidDevice());
       else
           startButton:setEnabled(false);
       end
    end);
    
    player2:setVisible(false);
    twoPlayerCombo:setActivated(false);
    
    local menuCamera = AppStateManager:getGameObjectController():getGameObjectFromId("2904886742");
    menuCamera:getCameraComponent():setActivated(true);
    
    onePlayerButton:reactOnMouseButtonClick(function() 
       onePlayerButton:setEnabled(false);
       twoPlayerButton:setEnabled(true);
       player2:setVisible(false);
       twoPlayerCombo:setActivated(false);
       twoPlayerPositionController:setActivated(false);
       twoPlayerColorButton:setActivated(false);
       twoPlayerName:setActivated(false);
       player2:getSpeechBubbleComponent():setActivated(false);
       
        AppStateManager:getGameProgressModule():setGlobalBoolValue("TwoPlayer", false);
    end);
    
    twoPlayerButton:reactOnMouseButtonClick(function() 
       twoPlayerButton:setEnabled(false);
       onePlayerButton:setEnabled(true);
       player2:setVisible(true);
       twoPlayerCombo:setActivated(true);
       twoPlayerPositionController:setActivated(true);
       twoPlayerColorButton:setActivated(true);
       twoPlayerName:setActivated(true);
       player2:getSpeechBubbleComponent():setActivated(true);

       AppStateManager:getGameProgressModule():setGlobalBoolValue("TwoPlayer", true);
    end);
    
    startButton:reactOnMouseButtonClick(function() 
        if (Core:isGame() == true) then
            if (onePlayerName:getCaption() ~= "Your Name") then
                AppStateManager:changeAppState("GameState");
            end
        end
    end);
    
    onePlayerName:reactOnEditTextChanged(function()
        player1:getSpeechBubbleComponent():setCaption(onePlayerName:getCaption());
    end);
    
    twoPlayerName:reactOnEditTextChanged(function()
        player2:getSpeechBubbleComponent():setCaption(twoPlayerName:getCaption());
    end);
    
    onePlayerColorButton:reactOnMouseButtonClick(function() 
       player1:getDatablockPbsComponent():setDiffuseTextureName(nextColorPlayer1());
    end);
    
    twoPlayerColorButton:reactOnMouseButtonClick(function() 
       player2:getDatablockPbsComponent():setDiffuseTextureName(nextColorPlayer2());
    end);
end

Menu_MainGameObject["disconnect"] = function()
    AppStateManager:getGameObjectController():undoAll();
end

Menu_MainGameObject["update"] = function(dt)
    if (player1Input:hasValidDevice()) then
        if (player1Input:getInputDeviceModule():isActionDown(NOWA_A_ATTACK_1) == true) then
            --player1Transform:reactOnFunctionFinished(function()
            --    player1:getAnimationComponent():setAnimationName("jump_up");
            --end);
            player1Transform:setActivated(false);
            player1:getAnimationComponent():setAnimationName("jump_up");
        end
    end
    
    if (player2Input:hasValidDevice()) then
        if (player2Input:getInputDeviceModule():isActionDown(NOWA_A_ATTACK_1) == true) then
             --player2Transform:reactOnFunctionFinished(function()
            --    player2:getAnimationComponent():setAnimationName("jump_up");
            --end);
            player2Transform:setActivated(false);
            player2:getAnimationComponent():setAnimationName("jump_up");
        end
    end
    
end

function actualizeDevices1()
    local devices1 = player1Input:getActualizedDeviceList();
    onePlayerCombo:setItemCount(0);
    local i = 0
    while i <= #devices1 do
        local deviceName = devices1[i];
        if (i == 0) then
            onePlayerCombo:setCaption(deviceName);
        end
        onePlayerCombo:addItem(deviceName);
        i = i + 1;
    end
end

function actualizeDevices2()
    local devices2 = player2Input:getActualizedDeviceList();
    twoPlayerCombo:setItemCount(0);
    local i = 0
    while i <= #devices2 do
       local deviceName = devices2[i];
        if (i == 0) then
            twoPlayerCombo:setCaption(deviceName);
        end
       twoPlayerCombo:addItem(deviceName);
       i = i + 1;
    end
end

-- Function to get the next image
function nextColorPlayer1()
    -- Increment the index
    currentIndexPlayer1 = currentIndexPlayer1 + 1
    
    -- Reset index if it exceeds the number of images
    if currentIndexPlayer1 > #images then
        currentIndexPlayer1 = 1
    end
    
    -- Get the selected image
    local selectedImage = images[currentIndexPlayer1]
    
    -- Return the selected image
    return selectedImage
end

-- Function to get the next image
function nextColorPlayer2()
    -- Increment the index
    currentIndexPlayer2 = currentIndexPlayer2 + 1
    
    -- Reset index if it exceeds the number of images
    if currentIndexPlayer2 > #images then
        currentIndexPlayer2 = 1
    end
    
    -- Get the selected image
    local selectedImage = images[currentIndexPlayer2]
    
    -- Return the selected image
    return selectedImage
end