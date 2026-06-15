module("Universum_0", package.seeall);
-- Scene: Scene1

require("init");

-- =========================================================================
-- Universum_0 Lua controller
--
-- Connects the FutureFighterJet (id 2599276278) to the UniversumComponent
-- AOI detection system. When the fighter enters a planet's atmosphere zone
-- (radius = planet_radius * 2), orbital motion pauses automatically and
-- the reactOnPlanetEntered callback fires. On exit, orbit resumes and
-- reactOnPlanetLeft fires.
-- =========================================================================

local FIGHTER_GO_ID = "2599276278";
local PLAYER_GO_ID       = "1959649159";

local universum_0    = nil;
local universumComp  = nil;
local playerGo = nil;
local fighterGo = nil;

Universum_0 = {}

local function activatePlayer()

    local spawnPos   = nil;
    local spawnOrient = Quaternion.IDENTITY;
 
    if fighterGo ~= nil then
        local shipPos    = fighterGo:getPosition();
        local shipOrient = fighterGo:getOrientation();
        -- Deactivate, so that physics are not processed
        fighterGo:getPhysicsActiveComponent():setGhost(true);
        fighterGo:getPhysicsActiveComponent():setGravitySourceCategory("Planets");
        fighterGo:getParticleFxComponent():setActivated(false);
        
        -- Place the player 3 units to the right of the ship in ship-local space
        -- and 1 unit up so they stand on the surface next to the ship.
        local shipRight = shipOrient:xAxis();
        spawnPos = Vector3(shipPos.x + shipRight.x * 6.0, shipPos.y + 1.0, shipPos.z + shipRight.z * 6.0);
        -- Face the same direction as the ship.
        spawnOrient = shipOrient;
    elseif lastContactPos ~= nil and lastContactNorm ~= nil then
        -- Fallback: no ship reference, use contact data directly.
        spawnPos = Vector3(lastContactPos.x + lastContactNorm.x * PLAYER_STAND_HEIGHT, lastContactPos.y + lastContactNorm.y * PLAYER_STAND_HEIGHT, lastContactPos.z + lastContactNorm.z * PLAYER_STAND_HEIGHT);
    else
        log("[Universum_0] landPlayer: no position reference available");
        return;
    end
 
    -- Make player visible and move them to the spawn point.
    playerGo:setVisible(true);
 
    local physComp = playerGo:getPhysicsActiveComponent();
    if physComp ~= nil then
        -- setPosition/setOrientation move both the physics body and the scene node.
        physComp:setActivated(true);
        physComp:setPosition(spawnPos);
        physComp:setOrientation(spawnOrient);
    end
 
    playerGo:getPhysicsActiveComponent():setGravitySourceCategory("Planets");
    playerGo:getInputDeviceComponent():setActivated(true);
    
    AppStateManager:getGameObjectController():activatePlayerController(true, playerGo:getId(), true);
   
    log("[Universum_0] Player spawned beside ship at " .. tostring(spawnPos));
end

Universum_0["connect"] = function(gameObject)
 
    universum_0 = AppStateManager:getGameObjectController():castGameObject(gameObject);
 
    universumComp = universum_0:getUniversumComponent();
    if universumComp == nil then
        log("[Universum_0] ERROR: UniversumComponent not found on " .. universum_0:getName());
        return;
    end
 
    universumComp:setPlayerGameObjectId(FIGHTER_GO_ID);
    universumComp:setAutoPauseOrbit(true);
    
    fighterGo = AppStateManager:getGameObjectController():getGameObjectFromId(FIGHTER_GO_ID);
    playerGo = AppStateManager:getGameObjectController():getGameObjectFromId(PLAYER_GO_ID);
    playerGo:getPhysicsActiveComponent():setActivated(false);
    
    local fadeWindowComponent = universum_0:getMyGUIFadeAlphaControllerComponent();
    local landStartButton     = universum_0:getMyGUIButtonComponentFromName("LandStartButton");
 
    -- Button toggles between Land and Start (takeoff).
    landStartButton:reactOnMouseButtonClick(function()
        if landStartButton:getCaption() == "Land" then
            universumComp:requestLanding();
        end
    end);
 
    universumComp:reactOnPlanetEntered(function(planetGameObject, enteredGameObject)
        planetGameObject  = AppStateManager:getGameObjectController():castGameObject(planetGameObject);
        enteredGameObject = AppStateManager:getGameObjectController():castGameObject(enteredGameObject);
        local attrib = enteredGameObject:getAttributesComponent();
        if attrib ~= nil then
            attrib:changeValueBool("isOnPlanet", true);
        end
        
        log("[Universum] " .. enteredGameObject:getName() .. " entered planet " .. planetGameObject:getName());
    end)
 
    universumComp:reactOnPlanetLeft(function(planetGameObject, enteredGameObject)
        planetGameObject  = AppStateManager:getGameObjectController():castGameObject(planetGameObject);
        enteredGameObject = AppStateManager:getGameObjectController():castGameObject(enteredGameObject);
        
        log("[Universum] " .. enteredGameObject:getName() .. " left planet " .. planetGameObject:getName());
        
        local attrib = enteredGameObject:getAttributesComponent();
        if attrib ~= nil then
            attrib:changeValueBool("isOnPlanet", false);
        end
        
        landStartButton:setCaption("Land");
        
        if fadeWindowComponent ~= nil then
            fadeWindowComponent:setAlpha(0);
            fadeWindowComponent:setActivated(true);
        end
    end)
 
    -- Fired when ship is close enough to land (within landingAltitudeThreshold).
    -- Show "Land" button here, or auto-trigger for testing.
    universumComp:reactOnLanding(function(bodyGO, shipGO)
        if fadeWindowComponent ~= nil then
            fadeWindowComponent:setAlpha(1);
            fadeWindowComponent:setActivated(true);
        end
        
        log("[Universum] reactOnLanding fired");
    end)
 
    -- Fired when autopilot has fully settled the ship.
    universumComp:reactOnLanded(function(bodyGO, shipGO)
        log("[Universum] reactOnLanded fired -- spawning player");
        activatePlayer();
        fadeWindowComponent:setAlpha(0);
        fadeWindowComponent:setActivated(true);
    end)
 
end

Universum_0["disconnect"] = function()
    AppStateManager:getGameObjectController():undoAll();
end

-- No per-frame logic needed here -- the UniversumComponent handles
-- orbital motion, far-clip adaptation, and sun steering in its own update().