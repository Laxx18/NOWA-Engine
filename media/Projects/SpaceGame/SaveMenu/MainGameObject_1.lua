module("MainGameObject_1", package.seeall);

-- Scene: SaveMenu.scene

MainGameObject_1 = {}

mainGameObject = nil;
listBox = nil;

function loadListItems()
    local success = AppStateManager:getGameProgressModule():loadProgress("SpaceGameSaveMenu", false);
    if (success) then
        local itemCount = AppStateManager:getGameProgressModule():getGlobalValue("ItemCount");
        if (itemCount ~= nil) then
            
            listBox:setItemCount(0);
            
            for i = 0, itemCount:getValueNumber() - 1 do
                value = AppStateManager:getGameProgressModule():getGlobalValue("SaveItem" .. toString(i));
                if (value ~= nil) then
                    listBox:addItem(value:getValueString());
                end
            end
        end
    end
end

MainGameObject_1["connect"] = function(gameObject)
    mainGameObject = AppStateManager:getGameObjectController():castGameObject(gameObject);
    listBox = mainGameObject:getMyGUIListBoxComponent();
	listBox:reactOnSelected(function(index) 
		if (Core:isGame() == true) then
			local saveGameText = listBox:getItemText(listBox:getSelectedIndex());
			if (saveGameText == "New") then
				saveGameText = Core:getCurrentDateAndTime() .. "_SpaceGame";
				-- false: not crypted for debugging purposes
				local success = AppStateManager:getGameProgressModule2("GameState"):saveProgress(saveGameText, false, false);
				listBox:addItem(saveGameText);
				
				saveEntries();
			else
				local messageBox = mainGameObject:getMyGUIMessageBoxComponent();
				messageBox:setMessage("The file '" .. saveGameText .. "' does already exist.\nDo you want to overwrite the file?");
				messageBox:setActivated(true);
			end
		end
	end);
	
	local buttonComponent = gameObject:getMyGUIButtonComponent();
	buttonComponent:reactOnMouseButtonClick(function() 
		AppStateManager:popAppState();
	end);

    if (Core:isGame() == true) then
        loadListItems()
    end

end

MainGameObject_1["disconnect"] = function()

end

MainGameObject_1["onMessageOverwriteEnd"] = function(thisComponent, result)
    if (result == MessageBoxStyle.Yes) then
        local saveGameText = Core:getCurrentDateAndTime() .. "_SpaceGame";
        listBox:setItemTExt(listBox:getSelectedIndex(), saveGameText);
        saveEntries();
    end
end

function saveEntries()
    AppStateManager:getGameProgressModule():setGlobalNumberValue("ItemCount", listBox:getItemCount());
    for i = 0, listBox:getItemCount() - 1 do
        AppStateManager:getGameProgressModule():setGlobalStringValue("SaveItem" .. toString(i), listBox:getItemText(i));
    end
    AppStateManager:getGameProgressModule():saveProgress("SpaceGameSaveMenu", false, false);
end