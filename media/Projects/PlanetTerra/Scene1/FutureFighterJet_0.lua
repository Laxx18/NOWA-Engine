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

local THRUST_SPEED     = 80.0
local BOOST_SPEED      = 180.0
local BRAKE_SPEED      = 0.0
local JUMP_BOOST_SPEED = 1000.0

local PITCH_RATE       = 1.2
local YAW_RATE         = 1.4
local ROLL_RATE        = 2.0

-- Auto-bank tuning.
-- MAX_BANK_ANGLE : maximum lean in radians (~25 deg at 0.44).
--                 The ship never exceeds this tilt regardless of
--                 how long the player holds yaw.
-- BANK_RATE      : how fast (rad/s) the bank moves toward its target.
--                 Matches or slightly exceeds YAW_RATE so the lean
--                 keeps pace with the turn.
-- BANK_RETURN    : how fast (rad/s) the bank springs back to zero
--                 when yaw input is released.
local MAX_BANK_ANGLE = 0.44
local BANK_RATE      = 1.6
local BANK_RETURN    = 2.5

local THRUST_SMOOTH  = 8.0

local SOUND_PITCH_IDLE  = 0.55
local SOUND_PITCH_MAX   = 1.80
local SOUND_PITCH_BOOST = 2.50

-- =========================================================================
--  Runtime state
-- =========================================================================

local boostActive   = false
local smoothSpeed   = 0.0
local isOnPlanet    = false
local maxSpeed      = 1000.0

-- currentBank tracks the cosmetic bank angle in radians.
-- Positive = right-wing-down (leaning into a left yaw turn).
-- This is our own accumulator -- we drive the physics roll torque
-- to match it, not the other way around.
local currentBank   = 0.0

-- =========================================================================
--  Entry points
-- =========================================================================

FutureFighterJet_0 = {}

FutureFighterJet_0["connect"] = function(gameObject)

    go = AppStateManager:getGameObjectController():castGameObject(gameObject)
    AppStateManager:getGameObjectController():activatePlayerController(true, go:getId(), true)

    inputDev     = go:getInputDeviceComponent():getInputDeviceModule()
    physComp     = go:getPhysicsActiveComponent()
    particleComp = go:getParticleFxComponentFromIndex(0)
    soundComp    = go:getSimpleSoundComponent()
    attribComp   = go:getAttributesComponent()

    physComp:setGravitySourceCategory("")

    maxSpeed = physComp:getMaxSpeed()
    if maxSpeed <= 0.0 then
        maxSpeed = JUMP_BOOST_SPEED
    end
    if JUMP_BOOST_SPEED > maxSpeed then
        JUMP_BOOST_SPEED = maxSpeed
    end

    if attribComp ~= nil then
        attribComp:reactOnAttributeChanged(function(attributeName, attribute)
            if attributeName == "isOnPlanet" then
                isOnPlanet = attribute:getValueBool()
                if isOnPlanet then
                    boostActive = false
                    smoothSpeed = math.min(smoothSpeed, THRUST_SPEED)
                else
                    physComp:setGravity(Vector3(0, 0, 0))
                end
            end
        end)
    end

    physComp:setGravity(Vector3(0, 0, 0))

    if particleComp ~= nil then
        particleComp:setActivated(true)
    end
    if soundComp ~= nil then
        soundComp:setLoop(true)
        soundComp:setVolume(70)
        soundComp:setActivated(true)
    end

    smoothSpeed  = 0.0
    isOnPlanet   = false
    currentBank  = 0.0
end

FutureFighterJet_0["disconnect"] = function()
    particleComp:setActivated(false)
    soundComp:setActivated(false)
    physComp:setGravitySourceCategory("")
end

-- =========================================================================
--  Main flight loop
-- =========================================================================

FutureFighterJet_0["update"] = function(dt)

    if (go:getInputDeviceComponent():isDeviceLocked()) then
       return;
    end
    -- ----------------------------------------------------------------
    --  1. INPUT
    -- ----------------------------------------------------------------

    local pitchRaw = 0.0
    if inputDev:isActionDown(NOWA_A_UP)   then pitchRaw = -1.0 end
    if inputDev:isActionDown(NOWA_A_DOWN) then pitchRaw =  1.0 end

    local yawRaw = 0.0
    if inputDev:isActionDown(NOWA_A_LEFT)  then yawRaw =  1.0 end
    if inputDev:isActionDown(NOWA_A_RIGHT) then yawRaw = -1.0 end

    local steer = inputDev:getSteerAxis()
    if math.abs(steer) > 0.10 then
        yawRaw = -steer
    end

    local rollRaw = 0.0
    if inputDev:isActionDown(NOWA_A_ATTACK_1) then rollRaw = -1.0 end
    if inputDev:isActionDown(NOWA_A_ATTACK_2) then rollRaw =  1.0 end

    local actionHeld = inputDev:isActionDown(NOWA_A_ACTION)
    local runHeld    = inputDev:isActionDown(NOWA_A_RUN)
    local jumpHeld   = inputDev:isActionDown(NOWA_A_JUMP)

    local anyRotInput = (pitchRaw ~= 0.0) or (yawRaw ~= 0.0) or (rollRaw ~= 0.0)

    -- ----------------------------------------------------------------
    --  2. TARGET SPEED
    -- ----------------------------------------------------------------

    local targetSpeed = 0.0

    if jumpHeld and not isOnPlanet then
        targetSpeed = JUMP_BOOST_SPEED
        boostActive = true
    elseif actionHeld and runHeld then
        targetSpeed = BOOST_SPEED
        boostActive = true
    elseif actionHeld then
        targetSpeed = THRUST_SPEED
        boostActive = false
    elseif runHeld then
        targetSpeed = BRAKE_SPEED
        boostActive = false
    else
        targetSpeed = 0.0
        boostActive = false
    end

    -- ----------------------------------------------------------------
    --  3. SPEED SMOOTHING + FORWARD FORCE
    -- ----------------------------------------------------------------

    local ks = 1.0 - math.exp(-THRUST_SMOOTH * dt)
    smoothSpeed = smoothSpeed + (targetSpeed - smoothSpeed) * ks

    local orient  = physComp:getOrientation()
    local forward = orient:zAxis()
    physComp:applyRequiredForceForVelocity(forward * smoothSpeed)

    -- ----------------------------------------------------------------
    --  4. AUTO-BANK STATE MACHINE
    --
    --  We maintain currentBank ourselves as a scalar (radians).
    --  Each frame we move it toward the target:
    --    - While yawing: target = yawRaw * MAX_BANK_ANGLE (clamped)
    --    - While not yawing: target = 0 (spring back to level)
    --  The rate of change is clamped to BANK_RATE or BANK_RETURN.
    --
    --  We then convert the CHANGE in bank angle this frame into an
    --  angular velocity around the ship's local Z axis and feed that
    --  into the rotation torque below.
    --  This way the bank never exceeds MAX_BANK_ANGLE and always
    --  returns to zero when yaw input stops -- like an airplane.
    -- ----------------------------------------------------------------

    local targetBank
    if yawRaw ~= 0.0 then
        -- Lean into the turn. yawRaw > 0 = turning left = right-wing-down.
        targetBank = -yawRaw * MAX_BANK_ANGLE
    else
        targetBank = 0.0
    end

    -- How far we still need to move.
    local bankError = targetBank - currentBank

    -- Choose rate: returning to level uses BANK_RETURN (snappier),
    -- leaning into turn uses BANK_RATE.
    local rate = (yawRaw ~= 0.0) and BANK_RATE or BANK_RETURN

    -- Maximum change this frame.
    local maxDelta = rate * dt

    -- Clamp the step so we never overshoot.
    local bankDelta
    if math.abs(bankError) <= maxDelta then
        bankDelta   = bankError          -- close enough, snap to target
        currentBank = targetBank
    else
        bankDelta   = maxDelta * (bankError > 0 and 1.0 or -1.0)
        currentBank = currentBank + bankDelta
    end

    -- Convert the per-frame bank delta to an angular velocity (rad/s)
    -- around local Z. applyOmegaForce takes rad/s, not rad/frame.
    -- bankDelta is already in radians-this-frame, so divide by dt.
    local bankOmega = (dt > 0.0001) and (bankDelta / dt) or 0.0

    -- ----------------------------------------------------------------
    --  5. ROTATION
    --
    --  Pitch  -> ship local X  (always correct)
    --  Yaw    -> WORLD Y       (never tilts, always steers the nose)
    --  Roll   -> ship local Z  (explicit player input)
    --  Bank   -> ship local Z  (auto cosmetic, derived from bankOmega)
    -- ----------------------------------------------------------------

    local localX = orient:xAxis()
    local localZ = orient:zAxis()

    -- Pitch around local X.
    local px = localX.x * pitchRaw * PITCH_RATE
    local py = localX.y * pitchRaw * PITCH_RATE
    local pz = localX.z * pitchRaw * PITCH_RATE

    -- Yaw around world Y.
    local yx = 0.0
    local yy = yawRaw * YAW_RATE
    local yz = 0.0

    -- Explicit roll around local Z.
    local rx = localZ.x * rollRaw * ROLL_RATE
    local ry = localZ.y * rollRaw * ROLL_RATE
    local rz = localZ.z * rollRaw * ROLL_RATE

    -- Auto-bank around local Z (capped, self-returning).
    local bx = localZ.x * bankOmega
    local by = localZ.y * bankOmega
    local bz = localZ.z * bankOmega

    -- Only apply torque when there is something to do.
    if anyRotInput or math.abs(bankOmega) > 0.001 then
        physComp:applyOmegaForce(Vector3(
            px + yx + rx + bx,
            py + yy + ry + by,
            pz + yz + rz + bz
        ))
    else
        physComp:applyOmegaForce(Vector3(0.0, 0.0, 0.0))
    end

    -- ----------------------------------------------------------------
    --  6. EFFECTS
    -- ----------------------------------------------------------------

    if soundComp ~= nil then
        local effectiveMax = isOnPlanet and BOOST_SPEED or JUMP_BOOST_SPEED
        local pitchMax     = (jumpHeld and not isOnPlanet) and SOUND_PITCH_BOOST or SOUND_PITCH_MAX
        local speedRatio   = math.abs(smoothSpeed) / effectiveMax
        if speedRatio > 1.0 then speedRatio = 1.0 end
        soundComp:setPitch(SOUND_PITCH_IDLE + speedRatio * (pitchMax - SOUND_PITCH_IDLE))
    end

end