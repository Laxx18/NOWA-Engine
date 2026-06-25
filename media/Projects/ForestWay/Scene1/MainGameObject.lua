module("MainGameObject", package.seeall);
-- Scene: Scene1

require("init");

mainGameObject = nil;
selectGameObjectsComponent = nil;

MainGameObject = {}

MainGameObject["connect"] = function(gameObject)
    mainGameObject = AppStateManager:getGameObjectController():castGameObject(gameObject);
    selectGameObjectsComponent = mainGameObject:getSelectGameObjectsComponent();

    selectGameObjectsComponent:reactOnGameObjectsSelected(function(selectedGameObjects)

        local selectedCharacter = nil
        local selectedVehicle = nil

        for _, selectedGO in pairs(selectedGameObjects) do
            selectedGO = AppStateManager:getGameObjectController():castGameObject(selectedGO)

            if selectedGO:getCategory() == "Character" then
                selectedCharacter = selectedGO
            elseif selectedGO:getCategory() == "Vehicle" then
                selectedVehicle = selectedGO
            end
        end

        -- Vehicle wins over Character
        if selectedVehicle ~= nil then
            selectedVehicle:getInputDeviceComponent():setActivated(true);
            AppStateManager:getGameObjectController():activatePlayerController(true, selectedVehicle:getId(), true);
            
        elseif selectedCharacter ~= nil then
            selectedCharacter:getInputDeviceComponent():setActivated(true);
            AppStateManager:getGameObjectController():activatePlayerController(true, selectedCharacter:getId(), true)
        end
    end)
end

MainGameObject["disconnect"] = function()
    AppStateManager:getGameObjectController():undoAll();
end