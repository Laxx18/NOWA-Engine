module("FutureFighterJet_0", package.seeall);
-- Scene: Scene1

require("init");

-- =========================================================================
--  Components
-- =========================================================================

local go            = nil
local inputDev      = nil
local physComp      = nil
local particleComp  = nil
local soundComp     = nil
local attribComp    = nil

-- =========================================================================
--  Flight tuning
-- =========================================================================

-- Forward thrust speeds (units/s)
local THRUST_SPEED     = 80.0
local BOOST_SPEED      = 180.0
local BRAKE_SPEED      = 0.0
local JUMP_BOOST_SPEED = 1000.0  -- Space: afterburner (disabled on planet)

-- Rotation rates (rad/s at full deflection)
local PITCH_RATE       = 1.2
local YAW_RATE         = 1.4
local ROLL_RATE        = 2.0
local AUTO_ROLL_FACTOR = 0.3

local THRUST_SMOOTH    = 8.0

-- Sound
local SOUND_PITCH_IDLE  = 0.55
local SOUND_PITCH_MAX   = 1.80
local SOUND_PITCH_BOOST = 2.50

-- =========================================================================
--  Runtime state
-- =========================================================================

local boostActive  = false;
local smoothSpeed  = 0.0;
local isOnPlanet   = false;   -- set via reactOnAttributeChanged
local maxSpeed     = 1000.0;  -- read from PhysicsActiveComponent at connect

-- =========================================================================
--  Entry points
-- =========================================================================

FutureFighterJet_0 = {}

FutureFighterJet_0["connect"] = function(gameObject)

    go = AppStateManager:getGameObjectController():castGameObject(gameObject);
    AppStateManager:getGameObjectController():activatePlayerController(true, go:getId(), true);

    inputDev     = go:getInputDeviceComponent():getInputDeviceModule();
    physComp     = go:getPhysicsActiveComponent();
    particleComp = go:getParticleFxComponentFromIndex(0);
    soundComp    = go:getSimpleSoundComponent();
    attribComp   = go:getAttributesComponent();
    
    physComp:setGravitySourceCategory("");

    -- Read MaxSpeed from PhysicsActiveComponent so JUMP_BOOST_SPEED
    -- never exceeds what the physics body is configured to allow.
    maxSpeed = physComp:getMaxSpeed();
    if maxSpeed <= 0.0 then
        maxSpeed = JUMP_BOOST_SPEED;
    end

    -- Clamp afterburner to physics max speed.
    if JUMP_BOOST_SPEED > maxSpeed then
        JUMP_BOOST_SPEED = maxSpeed;
    end

    -- Listen for attribute changes — specifically "isOnPlanet".
    if attribComp ~= nil then
        attribComp:reactOnAttributeChanged(function(attributeName, attribute)
            if attributeName == "isOnPlanet" then
                isOnPlanet = attribute:getValueBool();
                if isOnPlanet then
                    -- Landing: kill afterburner immediately and bleed speed down.
                    boostActive = false;
                    smoothSpeed = math.min(smoothSpeed, BOOST_SPEED);
                else
                   boostActive = true;
                   physComp:setGravity(Vector3(0, 0, 0));
                end
            end
        end);
    end

    -- No gravity in space
    physComp:setGravity(Vector3(0, 0, 0));

    if particleComp ~= nil then
        particleComp:setActivated(true);
    end

    if soundComp ~= nil then
        soundComp:setLoop(true);
        soundComp:setVolume(70);
        soundComp:setActivated(true);
    end

    smoothSpeed = 0.0;
    isOnPlanet  = false;

end

FutureFighterJet_0["disconnect"] = function()
    particleComp:setActivated(false);
    soundComp:setActivated(false);
    physComp:setGravitySourceCategory("");
end

-- =========================================================================
--  Main flight loop
-- =========================================================================

FutureFighterJet_0["update"] = function(dt)

    -- ----------------------------------------------------------------
    --  1. INPUT
    -- ----------------------------------------------------------------

    local pitchRaw = 0.0;
    if inputDev:isActionDown(NOWA_A_UP)   then pitchRaw = -1.0; end
    if inputDev:isActionDown(NOWA_A_DOWN) then pitchRaw =  1.0; end

    local yawRaw = 0.0;
    if inputDev:isActionDown(NOWA_A_LEFT)  then yawRaw =  1.0; end
    if inputDev:isActionDown(NOWA_A_RIGHT) then yawRaw = -1.0; end

    local steer = inputDev:getSteerAxis();
    if math.abs(steer) > 0.10 then
        yawRaw = -steer;
    end

    local rollRaw = 0.0;
    if inputDev:isActionDown(NOWA_A_ATTACK_1) then rollRaw = -1.0; end
    if inputDev:isActionDown(NOWA_A_ATTACK_2) then rollRaw =  1.0; end

    local actionHeld = inputDev:isActionDown(NOWA_A_ACTION);
    local runHeld    = inputDev:isActionDown(NOWA_A_RUN);
    local jumpHeld   = inputDev:isActionDown(NOWA_A_JUMP);

    local anyRotInput = (pitchRaw ~= 0.0) or (yawRaw ~= 0.0) or (rollRaw ~= 0.0);

    -- ----------------------------------------------------------------
    --  2. TARGET SPEED
    --     Afterburner (JUMP) is suppressed when on a planet to prevent
    --     the ship moving too fast to collide with the planet surface.
    -- ----------------------------------------------------------------

    local targetSpeed = 0.0;

    if jumpHeld and not isOnPlanet then
        targetSpeed = JUMP_BOOST_SPEED;
        boostActive = true;
    elseif actionHeld and runHeld then
        targetSpeed = BOOST_SPEED;
        boostActive = true;
    elseif actionHeld then
        targetSpeed = THRUST_SPEED;
        boostActive = false;
    elseif runHeld then
        targetSpeed = BRAKE_SPEED;
        boostActive = false;
    else
        targetSpeed = 0.0;
        boostActive = false;
    end

    -- ----------------------------------------------------------------
    --  3. SPEED SMOOTHING + FORWARD FORCE
    -- ----------------------------------------------------------------

    local ks = 1.0 - math.exp(-THRUST_SMOOTH * dt);
    smoothSpeed = smoothSpeed + (targetSpeed - smoothSpeed) * ks;

    local orient  = physComp:getOrientation();
    local forward = orient:zAxis();
    physComp:applyRequiredForceForVelocity(forward * smoothSpeed);

    -- ----------------------------------------------------------------
    --  4. ROTATION
    -- ----------------------------------------------------------------

    if anyRotInput then
        local autoRoll  = -yawRaw * AUTO_ROLL_FACTOR;
        local totalRoll = autoRoll + rollRaw;

        local desiredLocal = Vector3(
            pitchRaw  * PITCH_RATE,
            yawRaw    * YAW_RATE,
            totalRoll * ROLL_RATE
        );
        local desiredWorld = orient * desiredLocal;
        physComp:applyOmegaForce(desiredWorld);
    else
        physComp:applyOmegaForce(Vector3(0.0, 0.0, 0.0));
    end

    -- ----------------------------------------------------------------
    --  5. EFFECTS
    -- ----------------------------------------------------------------

    if soundComp ~= nil then
        -- Use actual max reachable speed for ratio so pitch scales correctly
        -- whether afterburner is available or not.
        local effectiveMax = isOnPlanet and BOOST_SPEED or JUMP_BOOST_SPEED;
        local pitchMax     = (jumpHeld and not isOnPlanet) and SOUND_PITCH_BOOST or SOUND_PITCH_MAX;
        local speedRatio   = math.abs(smoothSpeed) / effectiveMax;
        if speedRatio > 1.0 then speedRatio = 1.0; end
        soundComp:setPitch(SOUND_PITCH_IDLE + speedRatio * (pitchMax - SOUND_PITCH_IDLE));
    end

end