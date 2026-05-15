-- =============================================================================
-- Player.lua
-- Attached to: Player
--
-- Required components on Player:
--   PhysicsActiveComponent
--   AiLuaGoalComponent              (RootGoalName = "BuildingRootGoal")
--   AiRecastPathNavigationComponent (handles both walk-to and patrol)
--   MyGUIItemBoxComponent           (slots: each slot has ResourceName + GameObjectId set in NOWA-Design)
--   LuaScriptComponent
--
-- Required other GameObjects (set up in NOWA-Design):
--   <Any building templates> : building mesh, MeshConstructionComponent, SpawnComponent
--                              Their IDs are entered directly into the inventory slot's
--                              "GameObjectId" field in NOWA-Design — no Lua name mapping needed.
--   MainGameObject           : carries MyGUITextComponent + GameObjectPlaceComponent
--
-- How navigation works:
--   Walk phase  : pathNavigationComponent gets a waypoint at the build position
--   Patrol phase: pathNavigationComponent moving behavior gets 4 waypoints around the building
--                 and loops via FOLLOW_PATH + setRepeat(true)
--
-- How the template is resolved generically:
--   In NOWA-Design set each MyGUIItemBoxComponent slot's "GameObjectId" field to the id of
--   the corresponding template GameObject.  reactOnMouseButtonClick then receives that id
--   directly → activeTemplateGameObject is set without any name/id table in Lua.
-- =============================================================================

module("Player", package.seeall);

require("init");

-- ── Component references ───────────────────────────────────────────────────
local player                        = nil
local physicsActiveComponent        = nil
local aiGoalComp                    = nil
local pathNavigationComponent       = nil   -- AiRecastPathNavigationComponent
local itemBoxComponent              = nil
local activeTemplateGameObject      = nil   -- resolved generically at placement time from slot's GameObjectId
local statusText                    = nil
local gameObjectPlaceComponent      = nil

-- ── Build state ────────────────────────────────────────────────────────────
local buildPosition                 = nil   -- Vector3
local clonedBuildingId              = nil   -- string id of the just-placed clone
local workerSpawned                 = false
local pendingResourceName           = nil   -- "TownHallItem" / "HouseItem" / … (whatever the slot holds)

Player = {}

Player["connect"] = function(gameObject)
    player                    = AppStateManager:getGameObjectController():castGameObject(gameObject)
    physicsActiveComponent    = player:getPhysicsActiveComponent()
    aiGoalComp                = player:getAiLuaGoalComponent()
    pathNavigationComponent   = player:getAiRecastPathNavigationComponent()
    itemBoxComponent          = player:getMyGUIItemBoxComponent()

    local mainGameObject               = AppStateManager:getGameObjectController():getGameObjectFromId(MAIN_GAMEOBJECT_ID)
    gameObjectPlaceComponent           = mainGameObject:getGameObjectPlaceComponent()
    selectGameObjectsComponent         = mainGameObject:getSelectGameObjectsComponent()
    statusText                         = mainGameObject:getMyGUITextComponentFromName("StatusText")

    -- ── SelectGameObjectsComponent: show/hide itembox based on selection ──
    selectGameObjectsComponent:reactOnGameObjectsSelected(function(selectedGameObjects)
        local playerSelected = false

        for index, currentAgent in pairs(selectedGameObjects) do
            if currentAgent:getId() == player:getId() then
                playerSelected = true
                break
            end
        end

        if playerSelected then
            itemBoxComponent:setActivated(true)
            statusText:setCaption("Left-click an item in the inventory to place a building.")
        else
            itemBoxComponent:setActivated(false)
            -- Cancel any active placement if player is deselected
            if gameObjectPlaceComponent:isActivated() then
                gameObjectPlaceComponent:cancelPlacement()
            end
            statusText:setCaption("")
        end
    end)

    -- ── Drag-drop: allow all drops within same inventory ──────────────────
    itemBoxComponent:reactOnDropItemRequest(function(dragDropData)
        log("[Player] reactOnDropItemRequest resource: " .. dragDropData:getResourceName() .. " sender: " .. AppStateManager:getGameObjectController():getGameObjectFromId(dragDropData:getSenderInventoryId()):getName())
        dragDropData:setCanDrop(true)
    end)

    -- ── LEFT-click inventory item → enter placement mode ──────────────────
    -- reactOnMouseButtonClick fires on left-click press only (legacy callback).
    -- Signature: function(resourceName, gameObjectId, buttonId)
    --   gameObjectId  – the id entered in NOWA-Design for this slot ("0" when not set).
    --                   Use it to resolve activeTemplateGameObject without any name table.
    itemBoxComponent:reactOnMouseButtonClick(function(resourceName, gameObjectId, mouseButtonId)
        if itemBoxComponent:getQuantity(resourceName) <= 0 then
            statusText:setCaption("No " .. resourceName .. " left in inventory!")
            return
        end

        -- Resolve the associated template GameObject directly from the slot id.
        -- No name-to-template mapping table needed — the designer wires it up in NOWA-Design.
        if gameObjectId == "0" then
            statusText:setCaption("Slot '" .. resourceName .. "' has no template GameObject id configured.")
            log("[Player] WARNING: slot for " .. resourceName .. " has no GameObjectId set.")
            return
        end

        activeTemplateGameObject = AppStateManager:getGameObjectController():getGameObjectFromId(gameObjectId)
        if activeTemplateGameObject == nil then
            statusText:setCaption("Template GameObject id " .. gameObjectId .. " not found in scene.")
            log("[Player] ERROR: template id " .. gameObjectId .. " not found.")
            return
        end

        -- Hide the template so only the placed clone is visible
        activeTemplateGameObject:setVisible(false)

        pendingResourceName = resourceName
        gameObjectPlaceComponent:setActivated(true)
        gameObjectPlaceComponent:activatePlacement(gameObjectId)

        statusText:setCaption("Move mouse to spot, left-click to place " .. resourceName .. ". Right-click to cancel.")
        log("[Player] Placement mode activated for: " .. resourceName .. " (template id: " .. gameObjectId .. ")")
    end)

    -- ── Placement confirmed: player left-clicked a world position ─────────
    gameObjectPlaceComponent:reactOnGameObjectPlaced(function(newGameObjectId)
        log("[Player] Building placed, clone id: " .. newGameObjectId)

        clonedBuildingId = newGameObjectId

        -- Grab the actual world position from the freshly cloned object
        local clonedGO = AppStateManager:getGameObjectController():getGameObjectFromId(newGameObjectId)
        if clonedGO ~= nil then
            buildPosition = clonedGO:getPosition()
            -- Activates all components
            clonedGO:setActivated(true);
        end

        -- Tell recast to navigate to the cloned building
        --aiGoalComp:getMovingBehavior():getPath():addWayPoint(buildPosition);

        --pathNavigationComponent:setGoalRadius(1.5)
        --pathNavigationComponent:setActualizePathDelay(2000)

        --statusText:setCaption("Walking to build site...")
        --log("[Player] Navigating to build position: " .. toString(buildPosition))

        -- Start the AI goal chain
        --aiGoalComp:setActivated(true)
    end)

    gameObjectPlaceComponent:reactOnPlacementCancelled(function()
        pendingResourceName         = nil
        activeTemplateGameObject    = nil
        statusText:setCaption("Placement cancelled.")
        log("[Player] Placement cancelled")
    end)

    -- ── AI goal component setup ───────────────────────────────────────────
    aiGoalComp:getMovingBehavior():setGoalRadius(1.5)
    aiGoalComp:getMovingBehavior():setAutoOrientation(true)
    aiGoalComp:setActivated(false)  -- activated only after build site is confirmed

    -- Recast starts stopped
    pathNavigationComponent:setRepeat(false)
    pathNavigationComponent:setDirectionChange(false)

    AppStateManager:getGameObjectController():activatePlayerController(true, player:getId(), true)

    statusText:setCaption("Left-click an item in the inventory to begin building.")
end

Player["disconnect"] = function()
    buildPosition               = nil
    clonedBuildingId            = nil
    pendingResourceName         = nil
    workerSpawned               = false
    activeTemplateGameObject    = nil

    if itemBoxComponent ~= nil then
        itemBoxComponent:setActivated(false)
    end
    if gameObjectPlaceComponent ~= nil then
        gameObjectPlaceComponent:cancelPlacement()
        gameObjectPlaceComponent:setActivated(false)
    end
end

Player["update"] = function(dt)
    -- All logic is event-driven; nothing needed here.
end

-- =============================================================================
-- ROOT GOAL  (RootGoalName in NOWA-Design must be "BuildingRootGoal")
-- =============================================================================
BuildingRootGoal = {}

BuildingRootGoal["activate"] = function(gameObject, goalResult)
    goalResult = AppStateManager:getGameObjectController():castGoalResult(goalResult)
    log("[BuildingRootGoal] activate")
    aiGoalComp:addSubGoal(WalkToBuildSiteGoal)
    aiGoalComp:addSubGoal(ConstructBuildingGoal)
    aiGoalComp:addSubGoal(SpawnWorkerGoal)
end

BuildingRootGoal["terminate"] = function(gameObject, goalResult)
    goalResult = AppStateManager:getGameObjectController():castGoalResult(goalResult)
    log("[BuildingRootGoal] terminate")
end

-- =============================================================================
-- GOAL 1 : WalkToBuildSiteGoal
-- Recast is already navigating via the waypoint set in the placed callback.
-- This goal just waits until the player is close enough.
-- =============================================================================
WalkToBuildSiteGoal = {}
WalkToBuildSiteGoal._arrived = false

WalkToBuildSiteGoal["activate"] = function(gameObject, goalResult)
    goalResult = AppStateManager:getGameObjectController():castGoalResult(goalResult)
    log("[WalkToBuildSiteGoal] activate — recast already navigating")
    WalkToBuildSiteGoal._arrived = false
    goalResult:setStatus(GoalResult.ACTIVE)

    -- Listen for recast arrival event
    pathNavigationComponent:reactOnTargetReached(function()
        log("[WalkToBuildSiteGoal] arrived at build site")
        WalkToBuildSiteGoal._arrived = true
    end)
end

WalkToBuildSiteGoal["process"] = function(gameObject, dt, goalResult)
    goalResult = AppStateManager:getGameObjectController():castGoalResult(goalResult)
    goalResult:setStatus(WalkToBuildSiteGoal._arrived and GoalResult.COMPLETED or GoalResult.ACTIVE)
end

WalkToBuildSiteGoal["terminate"] = function(gameObject, goalResult)
    goalResult = AppStateManager:getGameObjectController():castGoalResult(goalResult)
    log("[WalkToBuildSiteGoal] terminate")
    pathNavigationComponent:setTargetId("0")
    WalkToBuildSiteGoal._arrived = false
end

-- =============================================================================
-- GOAL 2 : ConstructBuildingGoal
-- Works on activeTemplateGameObject — fully generic, no type checks needed.
-- =============================================================================
ConstructBuildingGoal = {}
ConstructBuildingGoal._done = false

ConstructBuildingGoal["activate"] = function(gameObject, goalResult)
    goalResult = AppStateManager:getGameObjectController():castGoalResult(goalResult)
    log("[ConstructBuildingGoal] activate")
    ConstructBuildingGoal._done = false
    goalResult:setStatus(GoalResult.ACTIVE)

    activeTemplateGameObject:getPhysicsActiveComponent():setPosition(buildPosition)
    activeTemplateGameObject:setVisible(true)
    itemBoxComponent:removeQuantity(pendingResourceName, 1)

    local mc = activeTemplateGameObject:getMeshConstructionComponent()
    mc:setConstructionTime(5.0)
    mc:setInvert(false)
    mc:setActivated(true)
    mc:reactOnConstructionDone(function()
        log("[ConstructBuildingGoal] construction complete")
        ConstructBuildingGoal._done = true
    end)

    statusText:setCaption("Constructing " .. pendingResourceName .. "... (5 s)")
end

ConstructBuildingGoal["process"] = function(gameObject, dt, goalResult)
    goalResult = AppStateManager:getGameObjectController():castGoalResult(goalResult)
    goalResult:setStatus(ConstructBuildingGoal._done and GoalResult.COMPLETED or GoalResult.ACTIVE)
end

ConstructBuildingGoal["terminate"] = function(gameObject, goalResult)
    goalResult = AppStateManager:getGameObjectController():castGoalResult(goalResult)
    log("[ConstructBuildingGoal] terminate")
    statusText:setCaption(pendingResourceName .. " built!")
end

-- =============================================================================
-- GOAL 3 : SpawnWorkerGoal
-- After completion switches to recast patrol around the building.
-- =============================================================================
SpawnWorkerGoal = {}

SpawnWorkerGoal["activate"] = function(gameObject, goalResult)
    goalResult = AppStateManager:getGameObjectController():castGoalResult(goalResult)
    log("[SpawnWorkerGoal] activate")
    workerSpawned = false
    goalResult:setStatus(GoalResult.ACTIVE)

    local spawnComp = activeTemplateGameObject:getSpawnComponent()
    spawnComp:setKeepAliveSpawnedGameObjects(true)
    spawnComp:reactOnSpawn(function(spawnedGO, originGO)
        spawnedGO:setVisible(true)
        spawnedGO:getPhysicsActiveComponent():setPosition(buildPosition + Vector3(2, 0, 0))
        spawnedGO:getPhysicsActiveComponent():setCollidable(true)
        log("[SpawnWorkerGoal] worker spawned: " .. spawnedGO:getName())
        workerSpawned = true
    end)
    spawnComp:setActivated(true)
end

SpawnWorkerGoal["process"] = function(gameObject, dt, goalResult)
    goalResult = AppStateManager:getGameObjectController():castGoalResult(goalResult)
    goalResult:setStatus(workerSpawned and GoalResult.COMPLETED or GoalResult.ACTIVE)
end

SpawnWorkerGoal["terminate"] = function(gameObject, goalResult)
    goalResult = AppStateManager:getGameObjectController():castGoalResult(goalResult)
    log("[SpawnWorkerGoal] terminate — switching to patrol via recast")
    statusText:setCaption("Building operational! Player now patrols.")

    -- Set up a square patrol around the building using recast's moving behavior.
    local offset = 10
    local p = buildPosition
    local mb = pathNavigationComponent:getMovingBehavior()
    mb:getPath():addWayPoint(Vector3(p.x - offset, p.y, p.z - offset))
    mb:getPath():addWayPoint(Vector3(p.x + offset, p.y, p.z - offset))
    mb:getPath():addWayPoint(Vector3(p.x + offset, p.y, p.z + offset))
    mb:getPath():addWayPoint(Vector3(p.x - offset, p.y, p.z + offset))
    mb:setBehavior(BehaviorType.FOLLOW_PATH)

    pathNavigationComponent:setRepeat(true)
    pathNavigationComponent:setDirectionChange(false)

    -- Shut down the goal component; recast now owns locomotion
    aiGoalComp:getMovingBehavior():setBehavior(BehaviorType.NONE)
    aiGoalComp:setActivated(false)

    -- Reset build state so a new building can be queued immediately
    buildPosition               = nil
    clonedBuildingId            = nil
    workerSpawned               = false
    activeTemplateGameObject    = nil
    pendingResourceName         = nil
end