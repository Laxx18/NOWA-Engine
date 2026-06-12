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

local boostActive     = false
local boostBlurActive = false
local smoothSpeed     = 0.0
local isOnPlanet      = false
local maxSpeed        = 1000.0
local currentBank     = 0.0

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
                log("reactOnAttributeChanged: " .. toString(isOnPlanet));
                if isOnPlanet then
                    boostActive = false
                    smoothSpeed = math.min(smoothSpeed, THRUST_SPEED)
                    -- Kill blur immediately when landing
                    if boostBlurActive then
                        boostBlurActive = false
                        go:getCompositorEffectRadialBlurComponent():setActivated(false)
                    end
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

    smoothSpeed   = 0.0
    isOnPlanet    = false
    currentBank   = 0.0
    boostBlurActive = false
    go:getCompositorEffectRadialBlurComponent():setActivated(false)
end

FutureFighterJet_0["disconnect"] = function()
    particleComp:setActivated(false)
    soundComp:setActivated(false)
    if boostBlurActive then
        boostBlurActive = false
        go:getCompositorEffectRadialBlurComponent():setActivated(false)
    end
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
        if not boostBlurActive then
            boostBlurActive = true
            go:getCompositorEffectRadialBlurComponent():setActivated(true)
        end
    elseif actionHeld and runHeld and not isOnPlanet then
        targetSpeed = BOOST_SPEED
        boostActive = true
        if boostBlurActive then
            boostBlurActive = false
            go:getCompositorEffectRadialBlurComponent():setActivated(false)
        end
    elseif actionHeld then
        targetSpeed = THRUST_SPEED
        boostActive = false
        if boostBlurActive then
            boostBlurActive = false
            go:getCompositorEffectRadialBlurComponent():setActivated(false)
        end
    elseif runHeld then
        targetSpeed = BRAKE_SPEED
        boostActive = false
        if boostBlurActive then
            boostBlurActive = false
            go:getCompositorEffectRadialBlurComponent():setActivated(false)
        end
    else
        targetSpeed = 0.0
        boostActive = false
        if boostBlurActive then
            boostBlurActive = false
            go:getCompositorEffectRadialBlurComponent():setActivated(false)
        end
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
    -- ----------------------------------------------------------------

    local targetBank
    if yawRaw ~= 0.0 then
        targetBank = -yawRaw * MAX_BANK_ANGLE
    else
        targetBank = 0.0
    end

    local bankError = targetBank - currentBank
    local rate = (yawRaw ~= 0.0) and BANK_RATE or BANK_RETURN
    local maxDelta = rate * dt

    local bankDelta
    if math.abs(bankError) <= maxDelta then
        bankDelta   = bankError
        currentBank = targetBank
    else
        bankDelta   = maxDelta * (bankError > 0 and 1.0 or -1.0)
        currentBank = currentBank + bankDelta
    end

    local bankOmega = (dt > 0.0001) and (bankDelta / dt) or 0.0

    -- ----------------------------------------------------------------
    --  5. ROTATION
    -- ----------------------------------------------------------------

    local localX = orient:xAxis()
    local localZ = orient:zAxis()

    local px = localX.x * pitchRaw * PITCH_RATE
    local py = localX.y * pitchRaw * PITCH_RATE
    local pz = localX.z * pitchRaw * PITCH_RATE

    local yx = 0.0
    local yy = yawRaw * YAW_RATE
    local yz = 0.0

    local rx = localZ.x * rollRaw * ROLL_RATE
    local ry = localZ.y * rollRaw * ROLL_RATE
    local rz = localZ.z * rollRaw * ROLL_RATE

    local bx = localZ.x * bankOmega
    local by = localZ.y * bankOmega
    local bz = localZ.z * bankOmega

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