module("MainGameObject", package.seeall);
require("init");

local mainGameObject             = nil
local statusText                 = nil
local gameObjectPlaceComponent   = nil
local selectGameObjectsComponent = nil
local currentActiveItemBox       = nil
local currentlySelectedBuilding  = nil
local activeTemplateGameObject   = nil
local isProcessingSlotClick      = false  -- blocks selection re-fire during slot click

MainGameObject = {}

local function activateItemBox(itemBox, isBuilding)
    if currentActiveItemBox ~= nil and currentActiveItemBox ~= itemBox then
        currentActiveItemBox:setActivated(false)
    end
    currentActiveItemBox = itemBox
    currentActiveItemBox:setActivated(true)
    currentActiveItemBox:reactOnMouseButtonClick(function(resourceName, gameObjectId, mouseButtonId)
        -- Block selection system from reacting to this click
        isProcessingSlotClick = true

        log("[MainGameObject] onInventoryItemClicked: resource=" .. tostring(resourceName)
            .. " gameObjectId=" .. tostring(gameObjectId)
            .. " building=" .. tostring(isBuilding))

        if isBuilding then
            -- Townhall selected -> spawn a worker
            if gameObjectId == "0" then
                statusText:setCaption("No worker template configured for slot '" .. resourceName .. "'.")
                isProcessingSlotClick = false
                return
            end
            local workerTemplate = AppStateManager:getGameObjectController():getGameObjectFromId(gameObjectId)
            if workerTemplate == nil then
                statusText:setCaption("Worker template id " .. tostring(gameObjectId) .. " not found.")
                isProcessingSlotClick = false
                return
            end
            local spawnPos = currentlySelectedBuilding:getPosition() + Vector3(4, 0, 4)
            AppStateManager:getGameObjectController():clone(workerTemplate:getId(), nil, "0", spawnPos, Quaternion.IDENTITY, Vector3.UNIT_SCALE)
            statusText:setCaption("Worker spawned!")
        else
            -- Worker selected -> enter building placement mode
            if gameObjectId == "0" then
                statusText:setCaption("Slot '" .. resourceName .. "' has no building template configured.")
                isProcessingSlotClick = false
                return
            end
            activeTemplateGameObject = AppStateManager:getGameObjectController():getGameObjectFromId(gameObjectId)
            if activeTemplateGameObject == nil then
                statusText:setCaption("Building template id " .. tostring(gameObjectId) .. " not found.")
                isProcessingSlotClick = false
                return
            end
            activeTemplateGameObject:setVisible(false)
            gameObjectPlaceComponent:setActivated(true)
            gameObjectPlaceComponent:activatePlacement(gameObjectId)
            statusText:setCaption("Move mouse to spot, left-click to place " .. resourceName .. ". Right-click to cancel.")
            log("[MainGameObject] Placement mode activated for: " .. resourceName)
        end

        isProcessingSlotClick = false
    end)
end

MainGameObject["connect"] = function(gameObject)
    mainGameObject             = AppStateManager:getGameObjectController():castGameObject(gameObject)
    statusText                 = mainGameObject:getMyGUITextComponentFromName("StatusText")
    gameObjectPlaceComponent   = mainGameObject:getGameObjectPlaceComponent()
    selectGameObjectsComponent = mainGameObject:getSelectGameObjectsComponent()

    gameObjectPlaceComponent:reactOnGameObjectPlaced(function(newGameObjectId)
        log("[MainGameObject] reactOnGameObjectPlaced: id=" .. tostring(newGameObjectId))
        local clonedGO = AppStateManager:getGameObjectController():getGameObjectFromId(newGameObjectId)
        if clonedGO == nil then
            log("[MainGameObject] ERROR: cloned GO id " .. tostring(newGameObjectId) .. " not found.")
            return
        end
        clonedGO:setActivated(true)
        local buildingItemBox = clonedGO:getMyGUIItemBoxComponent()
        if buildingItemBox ~= nil then
            buildingItemBox:setActivated(false)
        end
        statusText:setCaption("Building placed! Click it to interact.")
    end)

    gameObjectPlaceComponent:reactOnPlacementCancelled(function()
        activeTemplateGameObject = nil
        statusText:setCaption("Placement cancelled.")
        log("[MainGameObject] Placement cancelled.")
    end)

    selectGameObjectsComponent:reactOnGameObjectsSelected(function(selectedGameObjects)
        -- A slot click inside the ItemBox also triggers selection —
        -- ignore re-selection while we are processing a slot click
        if isProcessingSlotClick then
            log("[MainGameObject] reactOnGameObjectsSelected suppressed during slot click")
            return
        end

        log("[MainGameObject] reactOnGameObjectsSelected fired")

        if gameObjectPlaceComponent:isActivated() then
            gameObjectPlaceComponent:cancelPlacement()
        end

        local firstWorker   = nil
        local foundTownhall = nil

        for index, selectedGO in pairs(selectedGameObjects) do
            selectedGO = AppStateManager:getGameObjectController():castGameObject(selectedGO)
            local tag  = selectedGO:getTagName()
            log("[MainGameObject] Selected GO: name=" .. selectedGO:getName() .. " tag=" .. tostring(tag))

            if tag == "Worker" then
                AppStateManager:getGameObjectController():activatePlayerController(true, selectedGO:getId(), false)
                if firstWorker == nil then
                    firstWorker = selectedGO
                end
            elseif tag == "Townhall" and foundTownhall == nil then
                foundTownhall = selectedGO
                AppStateManager:getGameObjectController():activatePlayerController(true, selectedGO:getId(), true)
            end
        end

        if firstWorker ~= nil then
            currentlySelectedBuilding = nil
            activateItemBox(firstWorker:getMyGUIItemBoxComponent(), false)
            statusText:setCaption("Left-click to place a building.")
        elseif foundTownhall ~= nil then
            currentlySelectedBuilding = foundTownhall
            activateItemBox(foundTownhall:getMyGUIItemBoxComponent(), true)
            statusText:setCaption("Left-click an item to spawn a worker.")
        else
            -- Nothing relevant selected — hide active itembox
            if currentActiveItemBox ~= nil then
                currentActiveItemBox:setActivated(false)
                currentActiveItemBox = nil
            end
            currentlySelectedBuilding = nil
            statusText:setCaption("")
            log("[MainGameObject] No relevant selection")
        end
    end)

    statusText:setCaption("Select a worker to start building.")
    log("[MainGameObject] connect complete.")
end

MainGameObject["disconnect"] = function()
    isProcessingSlotClick = false

    if currentActiveItemBox ~= nil then
        currentActiveItemBox:setActivated(false)
        currentActiveItemBox = nil
    end
    if gameObjectPlaceComponent ~= nil then
        gameObjectPlaceComponent:cancelPlacement()
        gameObjectPlaceComponent:setActivated(false)
    end
    currentlySelectedBuilding = nil
    activeTemplateGameObject  = nil
    AppStateManager:getGameObjectController():undoAll()
end