module("FutureFighterJet_0", package.seeall);
-- Scene: Scene1

require("init");

-- =========================================================================
--  Components
-- =========================================================================

local go           = nil
local inputDev     = nil
local physComp     = nil
local particleComp = nil
local soundComp    = nil

-- =========================================================================
--  Flight tuning
-- =========================================================================

-- Forward thrust speeds (units/s)
local THRUST_SPEED     = 80.0
local BOOST_SPEED      = 180.0
local BRAKE_SPEED      = 0.0
local JUMP_BOOST_SPEED = 220.0 -- Space: afterburner

-- Rotation rates (rad/s at full deflection)
-- applyOmegaForce sets angular velocity directly — Newton normalizes dt internally.
local PITCH_RATE       = 1.2
local YAW_RATE         = 1.4
local ROLL_RATE        = 2.0
local AUTO_ROLL_FACTOR = 0.3    -- yaw -> auto-banking roll coupling

-- Input smoothing — dt IS valid here because this only drives a
-- dimensionless [-1..1] lerp for feel, not a physics quantity.
local INPUT_SMOOTH     = 9.0
local THRUST_SMOOTH    = 4.0

-- Sound
local SOUND_PITCH_IDLE = 0.55
local SOUND_PITCH_MAX  = 1.80
local SOUND_PITCH_BOOST = 2.50

-- =========================================================================
--  Runtime state
-- =========================================================================

local boostActive  = false

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
    targetSpeed = 0.0;

end

FutureFighterJet_0["disconnect"] = function()
    if particleComp ~= nil then particleComp:setActivated(false); end
    if soundComp    ~= nil then soundComp:setActivated(false);    end
end

-- =========================================================================
--  Main flight loop
-- =========================================================================

FutureFighterJet_0["update"] = function(dt)

    -- ----------------------------------------------------------------
    --  1. INPUT
    --
    --  Arrow Up/Down    → pitch (nose down / nose up)
    --  Arrow Left/Right → yaw (nose left / right), also auto-banks roll
    --  RShift / E       → manual roll left / right
    --  C (ACTION)       → thrust forward
    --  C + RCtrl (RUN)  → cruise boost
    --  RCtrl alone      → brake
    --  Space (JUMP)     → afterburner boost (overrides everything)
    -- ----------------------------------------------------------------

    -- Pitch: Arrow Up = nose down (-), Arrow Down = nose up (+)
    local pitchRaw = 0.0;
    if inputDev:isActionDown(NOWA_A_UP)   then pitchRaw = -1.0; end
    if inputDev:isActionDown(NOWA_A_DOWN) then pitchRaw =  1.0; end

    -- Yaw: Arrow Left / Right
    local yawRaw = 0.0;
    if inputDev:isActionDown(NOWA_A_LEFT)  then yawRaw =  1.0; end
    if inputDev:isActionDown(NOWA_A_RIGHT) then yawRaw = -1.0; end

    -- Analog stick yaw override
    local steer = inputDev:getSteerAxis();
    if math.abs(steer) > 0.10 then
        yawRaw = -steer;
    end

    -- Manual roll: RShift (ATTACK_1) = roll left, E (ATTACK_2) = roll right
    local rollRaw = 0.0;
    if inputDev:isActionDown(NOWA_A_ATTACK_1) then rollRaw = -1.0; end
    if inputDev:isActionDown(NOWA_A_ATTACK_2) then rollRaw =  1.0; end

    -- Thrust keys
    local actionHeld = inputDev:isActionDown(NOWA_A_ACTION);
    local runHeld    = inputDev:isActionDown(NOWA_A_RUN);
    local jumpHeld   = inputDev:isActionDown(NOWA_A_JUMP);

    -- ----------------------------------------------------------------
    --  2. TARGET SPEED
    --     Space (JUMP)     → afterburner (overrides everything)
    --     ACTION + RUN     → cruise boost
    --     ACTION           → cruise
    --     RUN alone        → brake
    --     nothing          → coast (linear damping in XML bleeds velocity)
    -- ----------------------------------------------------------------

    local speed = 0.0;

    if jumpHeld then
        speed       = JUMP_BOOST_SPEED;
        boostActive = true;
    elseif actionHeld and runHeld then
        speed       = BOOST_SPEED;
        boostActive = true;
    elseif actionHeld then
        speed       = THRUST_SPEED;
        boostActive = false;
    elseif runHeld then
        speed       = BRAKE_SPEED;
        boostActive = false;
    else
        speed       = 0.0;
        boostActive = false;
    end

    -- ----------------------------------------------------------------
    --  3. FORWARD FORCE
    --
    --  applyRequiredForceForVelocity: Newton handles dt internally.
    -- ----------------------------------------------------------------

    local orient  = go:getOrientation();
    local forward = orient:zAxis();
    physComp:applyRequiredForceForVelocity(forward * speed);

    -- ----------------------------------------------------------------
    --  4. ROTATION
    --
    --  Auto-banking: yaw induces roll for cinematic fighter feel.
    --  applyOmegaForce sets angular velocity directly — Newton handles
    --  dt internally. Angular damping in XML decelerates on key release.
    -- ----------------------------------------------------------------

    local autoRoll  = -yawRaw * AUTO_ROLL_FACTOR;
    local totalRoll = autoRoll + rollRaw;

    local desiredLocal = Vector3(
        pitchRaw  * PITCH_RATE,
        yawRaw    * YAW_RATE,
        totalRoll * ROLL_RATE
    );

    local desiredWorld = orient * desiredLocal;
    physComp:applyOmegaForce(desiredWorld);

    -- ----------------------------------------------------------------
    --  5. EFFECTS
    -- ----------------------------------------------------------------

    --if particleComp ~= nil then
        --if particleComp:isActivated() ~= boostActive then
            --particleComp:setActivated(boostActive);
        --end
    --end

    if soundComp ~= nil then
        local maxSpeed = BOOST_SPEED;
        local pitchMax = SOUND_PITCH_MAX;
        if jumpHeld then
            maxSpeed = JUMP_BOOST_SPEED;
            pitchMax = SOUND_PITCH_BOOST;
        end
        local speedRatio = math.abs(speed) / maxSpeed;
        if speedRatio > 1.0 then speedRatio = 1.0; end
        soundComp:setPitch(SOUND_PITCH_IDLE + speedRatio * (pitchMax - SOUND_PITCH_IDLE));
    end

end