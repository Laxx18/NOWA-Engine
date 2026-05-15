-- =============================================================================
-- Player.lua
-- Attached to: Player
--
-- Required components on Player:
--   PhysicsActiveComponent
--   AiLuaGoalComponent              (RootGoalName = "BuildingRootGoal")
--   AiRecastPathNavigationComponent (handles both walk-to and patrol)
--   MyGUIItemBoxComponent           (contains "TownHallItem", "HouseItem")
--   LuaScriptComponent
--
-- Required other GameObjects (set up in NOWA-Design):
--   TownhallTemplate    : building mesh, MeshConstructionComponent, SpawnComponent
--   HouseTemplate       : building mesh, MeshConstructionComponent, SpawnComponent
--   MainGameObject      : carries MyGUITextComponent + GameObjectPlaceComponent
--
-- How navigation works:
--   Walk phase  : pathNavigationComponent:setTargetId(clonedBuildingId) → recast finds shortest path
--   Patrol phase: pathNavigationComponent moving behavior gets 4 waypoints around the building
--                 and loops via FOLLOW_PATH + setRepeat(true)
-- =============================================================================

module("Player", package.seeall);

require("init");

-- ── Component references ───────────────────────────────────────────────────
local player                                 = nil
local physicsActiveComponent    = nil
local aiGoalComp                       = nil
local pathNavigationComponent = nil 
local itemBoxComponent           = nil
local townhallTemplate              = nil
local houseTemplate                  = nil
local activeTemplate                  = nil
local statusText                         = nil
local gameObjectPlaceComponent  = nil

-- ── Build state ────────────────────────────────────────────────────────────
local buildPosition                   = nil   -- Vector3
local clonedBuildingId             = nil   -- string id of the just-placed clone
local workerSpawned             = false
local pendingResourceName  = nil

Player = {}

Player["connect"] = function(gameObject)
    player                                               = AppStateManager:getGameObjectController():castGameObject(gameObject)
    physicsActiveComponent                  = player:getPhysicsActiveComponent()
    aiGoalComp                                      = player:getAiLuaGoalComponent()
    pathNavigationComponent                 = player:getAiRecastPathNavigationComponent()

    itemBoxComponent                         = player:getMyGUIItemBoxComponent()

    local mainGameObject                     = AppStateManager:getGameObjectController():getGameObjectFromId(MAIN_GAMEOBJECT_ID)
    gameObjectPlaceComponent          = mainGameObject:getGameObjectPlaceComponent()
    selectGameObjectsComponent       = mainGameObject:getSelectGameObjectsComponent();
    statusText                                       = mainGameObject:getMyGUITextComponentFromName("StatusText")

    townhallTemplate                           = AppStateManager:getGameObjectController():getGameObjectFromName("TownhallTemplate")
    houseTemplate                               = AppStateManager:getGameObjectController():getGameObjectFromName("HouseTemplate")
    townhallTemplate:setVisible(false)
    houseTemplate:setVisible(false)
    
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
    -- Uses legacy reactOnMouseButtonClick which fires on left-click release only.
    -- Right-click is intentionally NOT used here — it rotates the camera.
    itemBoxComponent:reactOnMouseButtonClick(function(resourceName, mouseButtonId)
        -- mouseButtonId is always MB_Left here (legacy callback filters it)
        if itemBoxComponent:getQuantity(resourceName) <= 0 then
            statusText:setCaption("No " .. resourceName .. " left in inventory!")
            return
        end
    
        pendingResourceName = resourceName
        gameObjectPlaceComponent:setActivated(true)
    
        if resourceName == "TownHallItem" then
            gameObjectPlaceComponent:activatePlacement(gameObjectPlaceComponent:getGameObjectId(0))
            statusText:setCaption("Move mouse to spot, left-click to place Townhall. Right-click to cancel.")
        elseif resourceName == "HouseItem" then
            gameObjectPlaceComponent:activatePlacement(gameObjectPlaceComponent:getGameObjectId(1))
            statusText:setCaption("Move mouse to spot, left-click to place House. Right-click to cancel.")
        end
    
        log("[Player] Placement mode activated for: " .. resourceName)
    end)

    -- ── Placement confirmed: player left-clicked a world position ─────────
    gameObjectPlaceComponent:reactOnGameObjectPlaced(function(newGameObjectId)
        log("[Player] Building placed, clone id: " .. newGameObjectId)
    
        clonedBuildingId = newGameObjectId
    
        -- Grab the actual world position from the freshly cloned object
        local clonedGO = AppStateManager:getGameObjectController():getGameObjectFromId(newGameObjectId)
        if clonedGO ~= nil then
            buildPosition = clonedGO:getPosition()
        end
    
        activeTemplate = (pendingResourceName == "TownHallItem") and townhallTemplate or houseTemplate
    
        -- Tell recast to navigate to the cloned building via shortest path
        aiGoalComp:getMovingBehavior():addWayPoint(buildPosition)
        
        pathNavigationComponent:setGoalRadius(1.5)
        pathNavigationComponent:setActualizePathDelay(2000)    -- recalculate every 2000 ms if target moves
    
        statusText:setCaption("Walking to build site...")
        log("[Player] Navigating to build position: " .. toString(buildPosition))
    
        -- Start the AI goal chain
        aiGoalComp:setActivated(true)
    end)
    
    gameObjectPlaceComponent:reactOnPlacementCancelled(function()
        pendingResourceName = nil
        statusText:setCaption("Placement cancelled.")
        log("[Player] Placement cancelled")
    end)
    
    -- ── AI goal component setup ───────────────────────────────────────────
    -- Recast drives the actual movement; goal component watches arrival state
    aiGoalComp:getMovingBehavior():setGoalRadius(1.5)
    aiGoalComp:getMovingBehavior():setAutoOrientation(true)
    aiGoalComp:setActivated(false)  -- activated only after build site is confirmed
    
    -- Recast starts stopped; activated via setTargetId in the placed callback
    pathNavigationComponent:setRepeat(false)
    pathNavigationComponent:setDirectionChange(false)
    
    AppStateManager:getGameObjectController():activatePlayerController(true, player:getId(), true)
    
    statusText:setCaption("Right-click an item in the inventory to begin building.")
end

Player["disconnect"] = function()
    buildPosition        = nil
    clonedBuildingId     = nil
    pendingResourceName  = nil
    workerSpawned        = false
    activeTemplate       = nil

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
-- Recast is already navigating via setTargetId set in the placed callback.
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
    -- Stop recast target following; patrol will take over after construction
    pathNavigationComponent:setTargetId("0")
    WalkToBuildSiteGoal._arrived = false
end

-- =============================================================================
-- GOAL 2 : ConstructBuildingGoal
-- =============================================================================
ConstructBuildingGoal = {}
ConstructBuildingGoal._done = false

ConstructBuildingGoal["activate"] = function(gameObject, goalResult)
    goalResult = AppStateManager:getGameObjectController():castGoalResult(goalResult)
    log("[ConstructBuildingGoal] activate")
    ConstructBuildingGoal._done = false
    goalResult:setStatus(GoalResult.ACTIVE)

    activeTemplate:getPhysicsActiveComponent():setPosition(buildPosition)
    activeTemplate:setVisible(true)
    itemBoxComponent:removeQuantity(pendingResourceName, 1)

    local mc = activeTemplate:getMeshConstructionComponent()
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

    local spawnComp = activeTemplate:getSpawnComponent()
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
    -- The offset distance (10 units) can be tuned per building type.
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
    buildPosition        = nil
    clonedBuildingId     = nil
    workerSpawned        = false
    activeTemplate       = nil
    pendingResourceName  = nil
end