module("LoadMenu_MainGameObject", package.seeall);

-- Scene: LoadMenu.scene

require("init");

LoadMenu_MainGameObject = {}

mainGameObject = nil;
listBox = nil;
loadButton = nil;
screenshotImage = nil;

-- The save game names in the order of the list, so the list can show a readable date.
local saveNames = {};

-- The menu has put the name of the player into the Core, see showMenu in the script of the menu.
local playerName = nil;

local GAME_STATE_NAME = "GameState";

-- "Max_2026-10-09_16-33-31" -> "09.10.2026  16:33"
local function formatSaveName(saveName)
    local y, mo, d, h, mi = string.match(saveName, "(%d%d%d%d)%-(%d%d)%-(%d%d)_(%d%d)%-(%d%d)%-%d%d$");
    if (nil == y) then
        return saveName;
    end
    return d .. "." .. mo .. "." .. y .. "  " .. h .. ":" .. mi;
end

-- The save game that is selected in the list, or nil. The list index is 0 based, -1 = nothing.
local function getSelectedSaveName()
    local index = listBox:getSelectedIndex();
    if (index < 0) then
        return nil;
    end
    return saveNames[index + 1];
end

-- The image box looks the file up in a RESOURCE GROUP, so only the file name is handed over, not the path.
-- The folder of the save games must therefore be a resource location.
local function showScreenshot(saveName)
    local fileName = saveName .. SCREENSHOT_ENDING;

    local file = io.open(getSavePath(saveName, SCREENSHOT_ENDING), "rb");
    if (nil == file) then
        screenshotImage:setActivated(false);
        do return end;
    end
    file:close();

    screenshotImage:setImageFileName(fileName);
    screenshotImage:setActivated(true);
end

-- The save games of the player, the newest first. They are named "<Player>_<date>_<time>".
function loadListItems()
    listBox:setItemCount(0);
    saveNames = {};

    local names = readSaveIndex(playerName);
    for i = #names, 1, -1 do
        if (true == AppStateManager:getGameProgressModule():hasSaveGame(names[i])) then
            saveNames[#saveNames + 1] = names[i];
            listBox:addItem(formatSaveName(names[i]));
        end
    end
end

LoadMenu_MainGameObject["connect"] = function(gameObject)
    mainGameObject = AppStateManager:getGameObjectController():castGameObject(gameObject);
    -- Whatever the Core holds (player or full save game name), the player is the part in front.
    playerName = (splitStartName(Core:getCurrentSaveGameName() or DEFAULT_PLAYER_NAME));

    listBox = mainGameObject:getMyGUIListBoxComponent();
    screenshotImage = mainGameObject:getMyGUIImageBoxComponentFromName("ScreenshotImage");
    screenshotImage:setActivated(false);

    listBox:reactOnSelected(function(index)
        local saveName = getSelectedSaveName();
        if (nil ~= saveName) then
            loadButton:setEnabled(true);
            showScreenshot(saveName);
        end
    end);

    loadButton = mainGameObject:getMyGUIButtonComponentFromName("LoadButton");
    loadButton:reactOnMouseButtonClick(function()
        -- The name of the selected save game. The game sees that it is exactly one and loads that one.
        local saveGameName = getSelectedSaveName();
        if (nil == saveGameName) then
            do return end;
        end

        -- The game starts with its own, empty global values.
        local gameProgressModule = AppStateManager:getGameProgressModule2(GAME_STATE_NAME);
        if (nil ~= gameProgressModule) then
            gameProgressModule:clearGlobalValues();
        end

        Core:setCurrentSaveGameName(saveGameName);
        AppStateManager:popAllAndPushAppState(GAME_STATE_NAME);
    end);

    local buttonComponent = gameObject:getMyGUIButtonComponent();
    buttonComponent:reactOnMouseButtonClick(function()
        AppStateManager:popAppState();
    end);

    loadListItems();
end

LoadMenu_MainGameObject["disconnect"] = function()
    loadButton:setEnabled(false);
end