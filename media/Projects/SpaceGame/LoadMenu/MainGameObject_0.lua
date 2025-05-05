module("MainGameObject_0", package.seeall);

-- Scene: LoadMenu.scene

MainGameObject_0 = {}

mainGameObject = nil;
listBox = nil;
loadButton = nil;

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

MainGameObject_0["connect"] = function(gameObject)
    mainGameObject = AppStateManager:getGameObjectController():castGameObject(gameObject);
    listBox = mainGameObject:getMyGUIListBoxComponent();
	listBox:reactOnSelected(function(index) 
		if (listBox:getSelectedIndex() ~= -1) then
			loadButton:setEnabled(true);
		end
	end);
	
    loadButton = mainGameObject:getMyGUIButtonComponentFromName("LoadButton");
	loadButton:reactOnMouseButtonClick(function() 
		-- Name of selected list item
		local loadGameText = listBox:getItemText(listBox:getSelectedIndex());
		Core:setCurrentSaveGameName(loadGameText);

		AppStateManager:popAllAndPushAppState("GameState");
	end);
	
	local buttonComponent = gameObject:getMyGUIButtonComponent();
	buttonComponent:reactOnMouseButtonClick(function() 
		AppStateManager:popAppState();
	end);

    if (Core:isGame() == true) then
        loadListItems()
    end
end

MainGameObject_0["disconnect"] = function()
    loadButton:setEnabled(false);
end