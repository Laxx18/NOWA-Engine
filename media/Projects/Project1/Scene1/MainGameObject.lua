module("MainGameObject", package.seeall);
MainGameObject = {}

-- Scene: Scene3

require("init");

MainGameObject["connect"] = function(gameObject)
    
	local saveButton = gameObject:getMyGUIButtonComponentFromName("SaveButton");
	saveButton:reactOnMouseButtonClick(function() 
		local saveGameText = Core:getCurrentDateAndTime() .. "_Scene3";
        --Core::setCurrentSaveGameName(saveGameText);
        local success = AppStateManager:getGameProgressModule():saveProgress(saveGameText, false, true);
	end);
    
    local loadButton = gameObject:getMyGUIButtonComponentFromName("LoadButton");
	loadButton:reactOnMouseButtonClick(function() 
        local saveSnapshots = Core:getSceneSnapshotsInProject(Core:getProjectName());
        --for key, val in pairs(saveSnapshots) do
        --end
        --local loadGameText = Core::getCurrentSaveGameName();    
		local saveGameText = saveSnapshots[0];
        local success = AppStateManager:getGameProgressModule():loadProgress(saveGameText, true, false);
	end);
end

MainGameObject["disconnect"] = function()

end