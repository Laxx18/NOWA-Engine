module("PrehistoricLax", package.seeall);
-- Scene: Level1

require("init");

-- File scope locals are fine here: the state tables below live in the SAME chunk, so they
-- close over these as upvalues. What does NOT work is declaring them inside connect() - the
-- state functions cannot see those, which is the trap the old PrehistoricLax_0 script ran
-- into ("variables created in connect are out of scope in WalkState execute").
local prehistoricLax = nil;
local cameraComponent = nil;
local areaOfInterestComponent = nil;
local attributesComponent = nil;
local mainGameObject = nil;
local playerController = nil;
local animationBlender = nil;

-- Mirrors NOWA::Direction from PlayerControllerComponents.h.
-- Attention: this order is NOT the one the old script used - RIGHT comes before LEFT here.
local DIR_NONE, DIR_RIGHT, DIR_LEFT, DIR_UP, DIR_DOWN = 0, 1, 2, 3, 4;

-- Which way the player is facing, kept up to date by reactOnDirectionChanged, so a state can
-- hold the facing or throw the ragdoll in the right direction without looking at the input.
local facingDirection = DIR_RIGHT;

-- Counted up on every swing and sent along with the PlayerAttackEvent. The cudgel script uses
-- it to make sure ONE swing can only ever cost an enemy energy ONCE, no matter how many
-- frames the kinematic contact keeps firing.
local attackId = 0;

local isInvulnerable = false;

local energy = nil;
local hitParticle = nil;
local baseDamage = 10;

---------------------------------------------------------------------------------------------------
-- Attack
--
-- The swing is NOT a state and not a child state any more. It is an OVERLAY animation that is
-- layered on top of whatever the walking state is playing, plus a timer that lives right here.
--
-- Why the overlay and not a state:
--
--   A state - child state or not - owns the animation. While it ran, the walking state was not
--   allowed to blend, so the player kept the punch pose while the physics carried on moving him:
--   he was sliding across the floor in a punch. Layering solves exactly that. The walking state
--   keeps doing its job - input, velocity, facing, the locomotion clip and addTime() - and the
--   punch is added on top of it. That is the "oben schlagen, unten laufen" case:
--
--       virtual void setOverlayAnimation(AnimID animationId, Ogre::Real blendInTime = 0.2f);
--       @brief  Starts an overlay animation on top of the current one using per-bone weights.
--               Useful for upper-body actions (attacks, reloads) while legs keep playing
--               locomotion.
--
--   The overlay also does not go through AnimationBlenderV2::internalBlend(), which refuses every
--   new blend while a non looping clip is still running:
--
--       if (false == this->complete && nullptr != this->previousSource) { return; }
--
--   That guard is what made the second press do nothing for a second or two. An overlay has no
--   such gate, so a follow-up swing starts the moment the window below allows it.
--
-- The timer is driven from PrehistoricLax["update"], which the LuaScriptComponent calls every
-- frame - the very function the NOWA-Design template already generates commented out at the
-- bottom of every object script.
--
-- Attention: this needs the fixed AnimationBlenderV2. In the old one the overlay driving code sat
-- inside the completion branch of a NON LOOPING main clip, so for a walking character it never ran
-- at all: the overlay was enabled with weight 0, its bones were muted on every other animation and
-- nothing advanced it - the upper body froze in the bind pose and the overlay never cleared itself.
---------------------------------------------------------------------------------------------------

-- Which bone chain the swing owns. That bone and ALL of its children are driven by the punch,
-- everything below it keeps walking.
--
-- "Boy 1 Pelvis" is the root of this skeleton and the parent of BOTH "Boy 1 Spine" and the two
-- thighs, so the spine is exactly where the upper body splits off from the legs:
--
--     Boy 1 Pelvis
--       Boy 1 Spine  ->  Spine1 -> Spine2 -> Neck/Head, clavicles, arms, hands, hoodie, backpack
--       Boy 1 L Thigh -> L Calf -> L Foot -> L Toe0
--       Boy 1 R Thigh -> R Calf -> R Foot -> R Toe0
--
-- The pelvis itself stays with the walk animation, which is what keeps the hip movement of the
-- walk cycle intact while the arms swing.
--
-- An empty string falls back to "whatever bones the punch clip animates". That is only the upper
-- body if the clip really keys nothing else; a full body mocap punch keys the legs and the root
-- too, and then the player punches with his whole body again - which is exactly the
-- "er schlaegt ODER er laeuft" symptom.
local ATTACK_OVERLAY_BONE = "Boy 1 Spine";

-- Prints, at the start of every swing, the clip length, the frame rate, the overlay speed and the
-- whole bone hierarchy with a marker on each bone the overlay owns, plus one line per frame while
-- the swing runs. If a leg bone comes out marked, the chain root is wrong.
local ATTACK_DEBUG = false;

-- How strongly the swing takes over.
--
-- ATTACK_CHAIN_INFLUENCE is how much of the spine chain the punch owns. 1.0 means the walk or idle
-- clip has no say above the pelvis at all, which is what a punch wants.
--
-- ATTACK_BODY_INFLUENCE is how far the punch reaches into the pelvis and the legs. 0 keeps the
-- locomotion completely untouched down there. A small value lets the whole body lean into the
-- swing while the legs keep walking, which is what makes it read as heavy rather than as an arm
-- waving on a separate torso. 0.25 to 0.35 is the sweet spot.
--
-- Attention: the pelvis is where the mocap clip carries its root motion, so turning this up too far
-- pulls the character backwards on every swing. Above 0.4 it starts to show.
local ATTACK_CHAIN_INFLUENCE = 1.0;
local ATTACK_BODY_INFLUENCE = 0.3;

-- Playback speed of the punch, 1.0 being the authored speed.
--
-- Attention: this is NOT the player controller's animation speed. That one is driven from the
-- walking speed to keep the feet in sync with the movement and sits well below 1.0 most of the
-- time; the overlay used to inherit it, which is why a 0.87 second punch took two to three
-- seconds.
local ATTACK_SPEED = 1.0;

-- How long the overlay fades in. The same value is used to fade it out again when the clip is
-- over. Short, so the swing feels immediate, but not zero - a hard cut makes the arm jump.
local ATTACK_BLEND_IN = 0.08;

-- The part of the swing that actually hurts, as a fraction of the punch clip. The window starts
-- after the wind-up and closes before the arm is pulled back, so walking into an enemy with the
-- cudgel dangling from the hand costs him nothing.
local ATTACK_HIT_START = 0.35;
local ATTACK_HIT_END = 0.75;

-- A swing ALWAYS plays to the end of the clip. A press while one is running is dropped, it does
-- not rewind the clip and it does not queue a second swing.
--
-- The previous version let a press from halfway through restart the clip with
-- setOverlayTimePosition(0). Two presses in quick succession then looked exactly like "he starts
-- to punch and is snapped back to the start pose", because that is literally what happened.
--
-- Length of the punch clip in seconds. Only used when the build has no overlay timing functions,
-- see OVERLAY_TIMING_AVAILABLE below - otherwise the real clip length is used.
local ATTACK_FALLBACK_DURATION = 0.87;

-- Pure safety net. The swing ends on the clip, not on this - but if the blender is never ticked
-- for some reason, this keeps the player from being stuck in "attacking" forever.
local ATTACK_TIMEOUT = 2.0;

local isAttacking = false;
local attackTime = 0;
local hasHitThisSwing = false;

-- Whether this binary exposes getOverlayProgress / isOverlayAnimationActive.
--
-- Probed once in connect() rather than assumed. The overlay functions were added to the binding
-- in several rounds, and a lua script that calls one the running build does not have dies on the
-- spot - in connect() that takes the whole player down with it. With the probe the swing simply
-- falls back to its own timer, which is a little less exact and otherwise identical.
local overlayTimingAvailable = false;

---------------------------------------------------------------------------------------------------
-- Helpers
---------------------------------------------------------------------------------------------------

function getEnergy()
    if (energy == nil) then
        return 100;
    end
    return energy:getValueNumber();
end

function setEnergy(value)
    if (energy == nil) then
        do return end;
    end
    energy:setValueNumber(value);
end

-- Central damage entry point, so an enemy, a trap or a hard landing all go through one place.
function applyDamage(amount)
    if (isInvulnerable == true) then
        do return end;
    end

    local newEnergy = getEnergy() - amount;
    if (newEnergy < 0) then
        newEnergy = 0;
    end
    setEnergy(newEnergy);

    if (newEnergy <= 0) then
        -- requestState is queued and applied at the top of the next update, so it is safe to
        -- call from any closure - even from one that fires while the walking state is still
        -- in the middle of its own update.
        playerController:requestState("RagDollState");
    else
        animationBlender:blend5(AnimationBlender.ANIM_TAKE_DAMAGE, AnimationBlender.BLEND_WHILE_ANIMATING, 0.1, false);
    end
end

-- Hits an enemy once. Kept here rather than in the state, so a trap or a thrown rock can
-- use the very same path later.
function damageEnemy(enemyGameObject)
    local enemyAttributes = enemyGameObject:getAttributesComponent();
    if (enemyAttributes == nil) then
        do return end;
    end

    local enemyEnergy = enemyAttributes:getAttributeValueByName("Energy");
    if (enemyEnergy == nil) then
        do return end;
    end

    local damage = baseDamage;
    if (mainGameObject ~= nil) then
        local strength = mainGameObject:getAttributesComponent():getAttributeValueByName("Strength");
        if (strength ~= nil) then
            damage = baseDamage * strength:getValueNumber();
        end
    end

    enemyEnergy:decrementValueNumber(damage);

    log("[PrehistoricLax] Hit " .. enemyGameObject:getName() .. " for " .. toString(damage) .. " -> energy: " .. toString(enemyEnergy:getValueNumber()));

    if (hitParticle ~= nil) then
        hitParticle:setGlobalPosition(enemyGameObject:getPosition());
        if (hitParticle:isPlaying() == false or hitParticle:isActivated() == false) then
            hitParticle:setActivated(true);
        end
    end

    if (enemyEnergy:getValueNumber() <= 0) then
        enemyEnergy:setValueNumber(0);

        if (mainGameObject ~= nil) then
            local killedEnemies = mainGameObject:getAttributesComponent():getAttributeValueByName("KilledEnemies");
            if (killedEnemies ~= nil) then
                killedEnemies:incrementValueNumber(1);
            end
        end

        if (EventType.EnemyDeadEvent ~= nil) then
            local eventData = {};
            eventData["enemyId"] = enemyGameObject:getId();
            AppStateManager:getScriptEventManager():queueEvent(EventType.EnemyDeadEvent, eventData);
        end
    end
end

-- Tells the cudgel script whether a real swing is going on. Without it the cudgel would cost an
-- enemy energy just by brushing past him while the player walks.
function sendAttackEvent(isActive)
    if (EventType.PlayerAttackEvent == nil) then
        do return end;
    end

    local eventData = {};
    eventData["isActive"] = isActive;
    eventData["attackId"] = attackId;
    AppStateManager:getScriptEventManager():queueEvent(EventType.PlayerAttackEvent, eventData);
end

function startAttack()
    -- A new swing gets a new id. The cudgel remembers per enemy which id it already hit with,
    -- so a kinematic contact firing on every frame of the swing still costs energy only once.
    attackId = attackId + 1;
    attackTime = 0;
    hasHitThisSwing = false;

    if (ATTACK_OVERLAY_BONE ~= "") then
        animationBlender:setOverlayAnimationForBoneChain1(AnimationBlender.ANIM_ATTACK_1, ATTACK_OVERLAY_BONE, ATTACK_BLEND_IN, false);
    else
        animationBlender:setOverlayAnimation3(AnimationBlender.ANIM_ATTACK_1, ATTACK_BLEND_IN, false);
    end

    isAttacking = true;

    sendAttackEvent(true);
end

function stopAttack()
    if (false == isAttacking) then
        do return end;
    end

    isAttacking = false;
    attackTime = 0;
    hasHitThisSwing = false;

    -- A non looping overlay fades itself out when the clip is over, so this is only needed when
    -- the swing is cut short - by a ragdoll, a portal or the safety timeout.
    if (true == animationBlender:isOverlayAnimationActive()) then
        animationBlender:clearOverlayAnimation(ATTACK_BLEND_IN);
    end

    sendAttackEvent(false);
end

---------------------------------------------------------------------------------------------------

PrehistoricLax = {}

PrehistoricLax["connect"] = function(gameObject)
    PointerManager:showMouse(false);
    prehistoricLax = AppStateManager:getGameObjectController():castGameObject(gameObject);
    -- Sets the camera id for the camera behavior
    local cameraGameObject = AppStateManager:getGameObjectController():getGameObjectFromName("GameCamera");
    prehistoricLax:getCameraBehaviorComponent():setCameraGameObjectId(cameraGameObject:getId());

    mainGameObject = AppStateManager:getGameObjectController():getGameObjectFromId(MAIN_GAMEOBJECT_ID);

    cameraComponent = AppStateManager:getGameObjectController():getGameObjectFromName("GameCamera"):getCameraComponent();
    -- Important: Camera switch
    cameraComponent:setActivated(true);

    AppStateManager:getGameObjectController():activatePlayerController(true, prehistoricLax:getId(), true);

    areaOfInterestComponent = prehistoricLax:getAreaOfInterestComponent();
    attributesComponent = mainGameObject:getAttributesComponent();

    -- The player's energy lives on the main game object, next to score and level, exactly
    -- like in main.lua - the HUD reads it from there.
    energy = mainGameObject:getAttributesComponent():getAttributeValueByName("Energy");
    hitParticle = mainGameObject:getParticleFxComponentFromName("HitParticle");

    playerController = prehistoricLax:getPlayerControllerJumpNRunComponent();
    animationBlender = playerController:getAnimationBlender();

    isAttacking = false;
    attackTime = 0;
    hasHitThisSwing = false;

    -- The punch runs at its own speed. Without this it would inherit the locomotion speed.
    animationBlender:setOverlaySpeed(ATTACK_SPEED);

    -- Guarded on purpose: setOverlayInfluence is the newest of the overlay functions, so a
    -- binary that was built before it was bound has everything else but not this one. Without
    -- the guard the missing function takes the whole connect() down with it and nothing works
    -- at all - with it, the swing simply runs at the default weighting.
    if (pcall(function() animationBlender:setOverlayInfluence(ATTACK_CHAIN_INFLUENCE, ATTACK_BODY_INFLUENCE); end) == false) then
        log("[PrehistoricLax] setOverlayInfluence is not available in this build - the swing uses the default weighting. Check that bindAnimationComponent has the .def line and rebuild.");
    end

    -- Same probe for the two functions the swing is TIMED on. Without them it runs on its own
    -- clock instead.
    overlayTimingAvailable = pcall(function()
        local unusedProgress = animationBlender:getOverlayProgress();
        local unusedActive = animationBlender:isOverlayAnimationActive();
    end);

    if (false == overlayTimingAvailable) then
        log("[PrehistoricLax] The overlay timing functions are not available in this build - the swing falls back to a fixed " .. toString(ATTACK_FALLBACK_DURATION) .. " second timer.");
    end

    if (true == ATTACK_DEBUG) then
        animationBlender:setDebugLog(true);
    end

    -----------------------------------------------------------------------------------------
    -- Extra animations
    --
    -- The component registers the 14 locomotion clips from its own attributes. Everything a
    -- custom state needs on top is registered here.
    -----------------------------------------------------------------------------------------
    animationBlender:registerAnimation(AnimationBlender.ANIM_ATTACK_1, "Boy 1 Punch");
    animationBlender:registerAnimation(AnimationBlender.ANIM_ATTACK_2, "Boy 1 Heavy Kick");
    animationBlender:registerAnimation(AnimationBlender.ANIM_ATTACK_3, "Boy 1 Light Kick");
    animationBlender:registerAnimation(AnimationBlender.ANIM_TAKE_DAMAGE, "Boy 1 Damage");
    animationBlender:registerAnimation(AnimationBlender.ANIM_PICKUP_1, "Boy 1 Idle Pick Up Item");
    animationBlender:registerAnimation(AnimationBlender.ANIM_ACTION_1, "Boy 1 Pass Out");
    animationBlender:registerAnimation(AnimationBlender.ANIM_NO_IDEA, "Boy 1 Look Side");

    -----------------------------------------------------------------------------------------
    -- State setup
    --
    -- The walking state is C++ and already registered by the component. Everything below is
    -- authored here in lua and only made ADDRESSABLE - registerLuaState does not switch.
    --
    -- Attention: only states that really take the player over belong here. The attack does
    -- NOT, it is an overlay, see the block at the top.
    -----------------------------------------------------------------------------------------
    playerController:registerLuaState("RagDollState", RagDollState);
    playerController:registerLuaState("PortalState", PortalState);

    playerController:reactOnStateChanged(function(oldStateName, newStateName)
        log("[PrehistoricLax] State: " .. oldStateName .. " -> " .. newStateName);
    end);

    -----------------------------------------------------------------------------------------
    -- Action key
    --
    -- C++ only reports WHAT is in front of the player on the rising edge of the key. What
    -- that object is - a lever, a portal, nothing at all - is decided here.
    -----------------------------------------------------------------------------------------
    playerController:setActionKey(NOWA_A_ATTACK_1);

    playerController:reactOnActionPressed(function(otherGameObject)
        -- No swinging while ragdolling or walking through a portal.
        if (playerController:isInState("WalkingStateJumpNRun") == false) then
            do return end;
        end

        if (otherGameObject ~= nil) then
            otherGameObject = AppStateManager:getGameObjectController():castGameObject(otherGameObject);

            if (otherGameObject:getTagName() == "Portal") then
                playerController:setInteractionGameObject(otherGameObject);
                playerController:requestState("PortalState");
                do return end;
            end
        end

        -- One swing at a time, and it always finishes.
        if (true == isAttacking) then
            do return end;
        end

        startAttack();
    end);

    -----------------------------------------------------------------------------------------
    -- Movement reactions
    -----------------------------------------------------------------------------------------
    playerController:reactOnDirectionChanged(function(oldDirection, newDirection)
        facingDirection = newDirection;
    end);

    playerController:reactOnJump(function(jumpCount)
        -- jumpCount is 1 for the jump off the ground, 2 and up for every air jump.
        if (jumpCount >= 2) then
            local smokeParticle = prehistoricLax:getParticleFxComponentFromIndex(0);
            if (smokeParticle ~= nil) then
                smokeParticle:setActivated(true);
            end
        end
    end);

    playerController:reactOnLand(function(fallTime)
        -- The longer the fall, the harder the landing. Below half a second it is an ordinary
        -- hop and must not cost anything.
        if (fallTime > 1.5) then
            applyDamage(math.floor(fallTime * 10));
        elseif (fallTime > 0.5) then
            local smokeParticle = prehistoricLax:getParticleFxComponentFromIndex(0);
            if (smokeParticle ~= nil) then
                smokeParticle:setActivated(true);
            end
        end
    end);

    playerController:reactOnWallContact(function(otherGameObject, wallNormal)
        -- Always called, no matter what 'Use Wall Separation Mode' is set to. With the flag
        -- on, C++ has already dropped the movement input pointing into the wall, so this is
        -- purely for effects - or for a ledge grab once the flag is switched off.
        if (otherGameObject == nil) then
            do return end;
        end

        otherGameObject = AppStateManager:getGameObjectController():castGameObject(otherGameObject);
        if (otherGameObject:getTagName() == "Spikes") then
            applyDamage(20);
        end
    end);

    -----------------------------------------------------------------------------------------
    -- Pickups
    -----------------------------------------------------------------------------------------
    areaOfInterestComponent:reactOnEnter(function(otherGameObject)
        otherGameObject = AppStateManager:getGameObjectController():castGameObject(otherGameObject);
        if (otherGameObject:getCategory() == "Item") then
            if (otherGameObject:getTagName() == "Coin") then
                AppStateManager:getGameObjectController():deleteGameObject(otherGameObject:getId());
                attributesComponent:addAttributeNumber("Coins", 1);
            elseif (otherGameObject:getTagName() == "Energy") then
                AppStateManager:getGameObjectController():deleteGameObject(otherGameObject:getId());
                setEnergy(getEnergy() + 25);
            end
        elseif (otherGameObject:getCategory() == "Enemy") then
            applyDamage(25);
        elseif (otherGameObject:getCategory() == "Quester") then

        end
    end);

    -- Sent by main.lua when the enemy's weapon took the last energy away.
    if (EventType.PlayerDeadEvent ~= nil) then
        AppStateManager:getScriptEventManager():registerEventListener(EventType.PlayerDeadEvent, PrehistoricLax["onPlayerDead"]);
    end
end

PrehistoricLax["disconnect"] = function()
    PointerManager:showMouse(true);
    cameraComponent:setActivated(false);
    AppStateManager:getGameObjectController():undoAll();

    isAttacking = false;
    playerController = nil;
    animationBlender = nil;
end

---------------------------------------------------------------------------------------------------
-- Called by the LuaScriptComponent every frame. This is where the swing is timed, so the
-- walking state stays completely untouched and keeps animating the player while he hits.
---------------------------------------------------------------------------------------------------
PrehistoricLax["update"] = function(dt)
    if (false == isAttacking) then
        do return end;
    end

    -- Going down or stepping into a portal cancels the swing.
    if (playerController:isInState("WalkingStateJumpNRun") == false) then
        stopAttack();
        do return end;
    end

    attackTime = attackTime + dt;

    -----------------------------------------------------------------------------------------
    -- Hit detection
    --
    -- The front rays are cast by PlayerControllerComponent::update() every frame anyway, so
    -- reading them here costs nothing. Once the cudgel's kinematic contact works, this block
    -- can go - the cudgel's own script already does the same thing off the contact, gated by
    -- the very same PlayerAttackEvent.
    -----------------------------------------------------------------------------------------
    -- How far the swing has come, 0 at the first frame of the punch clip and 1 at its last.
    local progress = attackTime / ATTACK_FALLBACK_DURATION;
    if (true == overlayTimingAvailable) then
        progress = animationBlender:getOverlayProgress();
    end

    if (false == hasHitThisSwing and progress >= ATTACK_HIT_START and progress <= ATTACK_HIT_END) then
        local target = playerController:getHitGameObjectFront();
        if (target ~= nil) then
            target = AppStateManager:getGameObjectController():castGameObject(target);
            if (target:getCategory() == "Enemy") then
                hasHitThisSwing = true;
                damageEnemy(target);
            end
        end
    end

    -- The swing is over only once the overlay is completely gone - clip finished AND faded out.
    --
    -- Attention: isOverlayBlendingOut() must NOT be used here. It goes true the moment the last
    -- frame of the clip is reached, while the fade back to the locomotion pose is still running,
    -- and ending the swing there cuts off exactly the part that makes the punch look finished.
    local swingIsOver = false;

    if (true == overlayTimingAvailable) then
        swingIsOver = (false == animationBlender:isOverlayAnimationActive());
    else
        swingIsOver = (attackTime >= ATTACK_FALLBACK_DURATION);
    end

    if (true == swingIsOver) then
        stopAttack();
        do return end;
    end

    if (attackTime >= ATTACK_TIMEOUT) then
        log("[PrehistoricLax] Attack safety timeout - the animation blender was not ticked.");
        stopAttack();
    end
end

PrehistoricLax["onPlayerDead"] = function(eventData)
    playerController:requestState("RagDollState");
end

---------------------------------------------------------------------------------------------------
-- States
--
-- Same shape the lua state machine has always used: enter(gameObject),
-- execute(gameObject, dt) and exit(gameObject), all optional. They now run in the SAME state
-- machine as the C++ walking state, so exactly one of them is active at a time.
---------------------------------------------------------------------------------------------------

RagDollState = { };

ragDollTime = 0;

RagDollState["enter"] = function(gameObject)
    ragDollTime = 5;
    isInvulnerable = true;

    -- A swing that was still in the air when the last energy went is dropped here, so the
    -- overlay does not keep punching on a corpse.
    stopAttack();

    -- No input while lying on the floor. The owner name matters: only 'ragdoll' can release
    -- this lock again, so a pickup animation running at the same time cannot unlock it.
    playerController:lockMovement("ragdoll", true);

    local ragDollComponent = playerController:getPhysicsRagDollComponent();
    if (ragDollComponent ~= nil) then
        ragDollComponent:setBoneConfigFile("PrehistoricLax2.rag");
        ragDollComponent:setState("Ragdolling");
    end
end

RagDollState["execute"] = function(gameObject, dt)
    -- Throw the body away from the direction the player was facing, but only for the first
    -- second - after that the ragdoll is left to the physics.
    if (ragDollTime >= 4) then
        local pushDirection = Vector3(-4, 0, 0);
        if (facingDirection == DIR_LEFT) then
            pushDirection = Vector3(4, 0, 0);
        end

        playerController:getPhysicsComponent():applyRequiredForceForVelocity(pushDirection);
        playerController:getPhysicsComponent():applyOmegaForce(Vector3(0, 0, -10));
    end

    ragDollTime = ragDollTime - dt;

    if (ragDollTime <= 0) then
        if (getEnergy() <= 0) then
            setEnergy(100);
        end

        playerController:requestState("WalkingStateJumpNRun");
    end
end

RagDollState["exit"] = function(gameObject)
    local ragDollComponent = playerController:getPhysicsRagDollComponent();
    if (ragDollComponent ~= nil) then
        ragDollComponent:setBoneConfigFile("PrehistoricLaxPartial.rag");
        ragDollComponent:setState("PartialRagdolling");
    end

    -- The ragdoll left the body with whatever velocity and spin the push gave it. Without this
    -- the walking state would inherit both and the player would walk off sideways, spinning.
    playerController:getPhysicsComponent():resetForce();

    playerController:lockMovement("ragdoll", false);
    isInvulnerable = false;
end

---------------------------------------------------------------------------------------------------

PortalState = { };

PortalState["enter"] = function(gameObject)
    stopAttack();

    playerController:lockMovement("portal", true);

    local portal = playerController:getInteractionGameObject();
    if (portal == nil) then
        playerController:lockMovement("portal", false);
        playerController:requestState("WalkingStateJumpNRun");
        do return end;
    end

    local pathFollow = portal:getAiPathFollowComponent();
    if (pathFollow == nil) then
        playerController:lockMovement("portal", false);
        playerController:requestState("WalkingStateJumpNRun");
        do return end;
    end

    local referenceId = portal:getReferenceId();
    AppStateManager:getGameObjectController():activateGameObjectComponentsFromReferenceId(referenceId, true);

    pathFollow:setActivated(true);
    pathFollow:reactOnPathGoalReached(function()
        pathFollow:setActivated(false);
        AppStateManager:getGameObjectController():activateGameObjectComponentsFromReferenceId(referenceId, false);

        -- Back to wherever the player came from, instead of hardcoding the walk state.
        playerController:requestPreviousState();
    end);
end

PortalState["execute"] = function(gameObject, dt)
    if (animationBlender:isAnimationActive(AnimationBlender.ANIM_WALK_NORTH) == false) then
        animationBlender:blend5(AnimationBlender.ANIM_WALK_NORTH, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);
    end

    animationBlender:addTime(dt * playerController:getAnimationSpeed() / animationBlender:getLength(), "PlayerControllerJumpNRunComponent");
end

PortalState["exit"] = function(gameObject)
    playerController:lockMovement("portal", false);
    playerController:setInteractionGameObject(nil);
end