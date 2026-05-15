module("MainGameObject", package.seeall);
require("init");

local mainGameObject             = nil
local statusText                 = nil
local gameObjectPlaceComponent   = nil
local selectGameObjectsComponent = nil
local currentActiveItemBox       = nil
local currentlySelectedBuilding  = nil
local activeTemplateGameObject   = nil

MainGameObject = {}

-- Shared click handler for both worker inventory and townhall inventory
local function onInventoryItemClicked(resourceName, gameObjectId, mouseButtonId)
    log("[MainGameObject] onInventoryItemClicked: resource=" .. tostring(resourceName)
        .. " gameObjectId=" .. tostring(gameObjectId)
        .. " building=" .. tostring(currentlySelectedBuilding ~= nil))

    -- Townhall selected -> spawn a worker
    if currentlySelectedBuilding ~= nil then
        if gameObjectId == "0" then
            statusText:setCaption("No worker template configured for slot '" .. resourceName .. "'.")
            return
        end
        local workerTemplate = AppStateManager:getGameObjectController():getGameObjectFromId(gameObjectId)
        if workerTemplate == nil then
            statusText:setCaption("Worker template id " .. gameObjectId .. " not found.")
            return
        end
        local spawnPos = currentlySelectedBuilding:getPosition() + Vector3(4, 0, 4)
        AppStateManager:getGameObjectController():clone(workerTemplate:getId(), nil, "0", spawnPos, Quaternion.IDENTITY, Vector3.UNIT_SCALE)
        statusText:setCaption("Worker spawned!")
        return
    end

    -- Worker selected -> enter building placement mode
    if gameObjectId == "0" then
        statusText:setCaption("Slot '" .. resourceName .. "' has no building template configured.")
        return
    end
    activeTemplateGameObject = AppStateManager:getGameObjectController():getGameObjectFromId(gameObjectId)
    if activeTemplateGameObject == nil then
        statusText:setCaption("Building template id " .. gameObjectId .. " not found.")
        return
    end
    activeTemplateGameObject:setVisible(false)
    gameObjectPlaceComponent:setActivated(true)
    gameObjectPlaceComponent:activatePlacement(gameObjectId)
    statusText:setCaption("Move mouse to spot, left-click to place " .. resourceName .. ". Right-click to cancel.")
    log("[MainGameObject] Placement mode activated for: " .. resourceName)
end

MainGameObject["connect"] = function(gameObject)
    mainGameObject             = AppStateManager:getGameObjectController():castGameObject(gameObject)
    statusText                 = mainGameObject:getMyGUITextComponentFromName("StatusText")
    gameObjectPlaceComponent   = mainGameObject:getGameObjectPlaceComponent()
    selectGameObjectsComponent = mainGameObject:getSelectGameObjectsComponent()

    -- Building placed callback lives here now — MainGameObject owns placement
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
        log("[MainGameObject] reactOnGameObjectsSelected fired")

        if currentActiveItemBox ~= nil then
            currentActiveItemBox:setActivated(false)
            currentActiveItemBox = nil
        end
        currentlySelectedBuilding = nil

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
                
                 --or via loop of tables which go into this function:
                --local workerIds = {}
                --for index, selectedGO in pairs(selectedGameObjects) do
                    --selectedGO = AppStateManager:getGameObjectController():castGameObject(selectedGO)
                    --if selectedGO:getTagName() == "Worker" then
                        --table.insert(workerIds, tostring(selectedGO:getId()))
                    --end
                --end
                
                --if #workerIds > 0 then
                    --AppStateManager:getGameObjectController():activatePlayerControllers(workerIds, false)
                --end
                
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
            currentActiveItemBox = firstWorker:getMyGUIItemBoxComponent()
            currentActiveItemBox:setActivated(true)
            -- onInventoryItemClicked is now in this same scope — no nil error
            currentActiveItemBox:reactOnMouseButtonClick(onInventoryItemClicked)
            statusText:setCaption("Left-click to place a building.")

        elseif foundTownhall ~= nil then
            currentlySelectedBuilding = foundTownhall
            currentActiveItemBox = foundTownhall:getMyGUIItemBoxComponent()
            currentActiveItemBox:setActivated(true)
            currentActiveItemBox:reactOnMouseButtonClick(onInventoryItemClicked)
            statusText:setCaption("Left-click an item to spawn a worker.")
        else
            log("[MainGameObject] No relevant selection")
            statusText:setCaption("")
        end
    end)

    statusText:setCaption("Select a worker to start building.")
    log("[MainGameObject] connect complete.")
end

MainGameObject["disconnect"] = function()
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