module("PrehistoricLax", package.seeall);
-- Scene: Level1

require("init");

-- Combat values of all enemy types, see EnemyProfiles.lua.
local EnemyProfiles = require("EnemyProfiles");

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
local moneySound = nil;
local hurtSound = nil;

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

-- True while the player lies in the ragdoll. The short invulnerability after a hit (i-frames) is
-- tracked separately in invulnerableTimer, so the two can never switch each other off.
local isInvulnerable = false;

local energy = nil;
local energyProgress = nil;
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
local ATTACK_FALLBACK_DURATION = 1;

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

-- The player's own front ray hit detection in update(). The cudgel's kinematic contact
-- (WeaponStick.lua) does the job now. With both active an enemy was damaged TWICE per swing, and
-- whichever of the two killed it decided whether the EnemyDeadEvent carried a hitDirection or not -
-- one more reason the dead rhino flew off in random directions. Switch this back on only if the
-- cudgel is not attached.
local USE_FRONT_RAY_HIT = false;

---------------------------------------------------------------------------------------------------
-- Enemy contact combat
--
-- What the simple "touch = lose energy" version lacks, and what action games do instead:
--
--   1. Telegraph. Touching an enemy starts ITS attack (ANIM_ATTACK_1). The damage does not land
--      on the first touch, but at the impact moment of that attack (impactDelay), and only if the
--      player is still within reach. A player who jumps or steps away in time dodges the kick.
--      Set impactDelay to 0 for an enemy that should hurt on contact immediately.
--
--   2. Trade rule. If the player's own swing is in its active window and he faces the enemy, the
--      player wins the exchange: the enemy's attack is not started (on contact) resp. does not
--      land (at impact). Timing the swing is rewarded.
--
--   3. Hit reaction. Energy loss, ANIM_TAKE_DAMAGE, hurt sound and particle, a knockback away from
--      the enemy and a short stagger in which the input is locked. Getting hit also cancels the
--      player's own swing.
--
--   4. I-frames. After a hit the player is invulnerable for IFRAME_TIME seconds and blinks, so one
--      enemy (or a group) cannot drain him in a row.
--
--   5. Pressure. The enemy attacks again after its cooldown as long as the player stays in reach -
--      standing still next to an enemy is not safe.
--
-- Everything that differs per enemy type (damage, timing, reach, knockback) lives in
-- EnemyProfiles.lua, keyed by the enemy's tag name. A new enemy type only needs an entry there.
---------------------------------------------------------------------------------------------------

-- Invulnerability after a hit, and how fast the player blinks meanwhile.
local IFRAME_TIME = 1.2;
local IFRAME_BLINK = true;
local IFRAME_BLINK_INTERVAL = 0.08;

local invulnerableTimer = 0;
local blinkTimer = 0;
local playerBlinkVisible = true;
local blinkApiAvailable = true;

local staggerTimer = 0;
local staggerDuration = 0;
local isStaggerLocked = false;

-- Only used when applyRequiredForceForJumpVelocity is not bound for lua, see applyKnockback().
local knockbackFallbackX = 0;
local knockbackApiLogged = false;

-- Per enemy attack state, keyed by the enemy's game object id. Only the id is stored and the
-- game object is looked up again on every use: an enemy can be deleted at any time (killed), and
-- a stored game object reference would then dangle.
local enemyCombat = {};

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
    energyProgress:setValue(value);
end

-- +1 when the player faces right, -1 when he faces left.
function getFacingSign()
    if (facingDirection == DIR_LEFT) then
        return -1;
    end
    return 1;
end

-- Central damage entry point, so an enemy, a trap or a hard landing all go through one place.
-- Only energy and death are handled here. The visible reaction (animation, knockback, i-frames)
-- is up to the caller: an enemy hit uses hitPlayer(), a damage-over-time contact reacts itself.
function applyDamage(amount)
    if (isInvulnerable == true or invulnerableTimer > 0) then
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
    end
end

-- Whether an enemy attack may hurt the player right now.
function canPlayerBeHit()
    if (playerController == nil) then
        return false;
    end
    if (true == isInvulnerable or invulnerableTimer > 0) then
        return false;
    end
    -- No hits while ragdolling or walking through a portal.
    if (playerController:isInState("WalkingStateJumpNRun") == false) then
        return false;
    end
    return true;
end

-- Shows or hides the player for the i-frame blink. Guarded, since not every build may bind
-- GameObject:setVisible for lua - then the blink is simply skipped.
function setPlayerVisible(visible)
    playerBlinkVisible = visible;

    if (false == blinkApiAvailable or prehistoricLax == nil) then
        do return end;
    end

    if (pcall(function() prehistoricLax:setVisible(visible); end) == false) then
        blinkApiAvailable = false;
        log("[PrehistoricLax] GameObject:setVisible is not available for lua in this build - the i-frame blink is skipped.");
    end
end

function releaseStagger()
    staggerTimer = 0;
    staggerDuration = 0;
    knockbackFallbackX = 0;

    if (true == isStaggerLocked and playerController ~= nil) then
        playerController:lockMovement("hit", false);
    end
    isStaggerLocked = false;
end

-- Throws the player away from the enemy.
--
-- resetForce() first: it clears the velocity the walking state latched in the physics component.
-- That latch is re-applied on every physics substep and would pull the player straight back.
-- Then ONE impulse with applyRequiredForceForJumpVelocity - the same one shot command the jump
-- uses - and gravity does the rest. While the stagger locks the input, the walking state sends
-- no new velocity, so nothing overwrites the flight.
function applyKnockback(directionX, horizontal, up)
    local physicsComponent = playerController:getPhysicsComponent();
    if (physicsComponent == nil) then
        do return end;
    end

    local resetOk = pcall(function() physicsComponent:resetForce(); end);

    local velocity = Vector3(directionX * horizontal, up, 0);
    local impulseOk = pcall(function() physicsComponent:applyRequiredForceForJumpVelocity(velocity); end);

    if (false == impulseOk) then
        -- Fallback for a build without that binding: update() drives the horizontal part during
        -- the stagger with the latched velocity command instead. No upward kick then.
        knockbackFallbackX = directionX * horizontal;
    end

    if ((false == resetOk or false == impulseOk) and false == knockbackApiLogged) then
        knockbackApiLogged = true;
        log("[PrehistoricLax] Knockback runs in fallback mode. resetForce bound: " .. toString(resetOk) .. " applyRequiredForceForJumpVelocity bound: " .. toString(impulseOk));
    end
end

-- The complete hit reaction of the player. Returns true if the hit was taken.
function hitPlayer(amount, sourcePosition, profile)
    if (false == canPlayerBeHit()) then
        return false;
    end

    -- Getting hit cancels the own swing.
    stopAttack();

    applyDamage(amount);

    local playerPosition = prehistoricLax:getPosition();

    if (hitParticle ~= nil) then
        hitParticle:setGlobalPosition(playerPosition);
        if (hitParticle:isPlaying() == false or hitParticle:isActivated() == false) then
            hitParticle:setActivated(true);
        end
    end

    if (hurtSound ~= nil) then
        hurtSound:setActivated(true);
    end

    log("[PrehistoricLax] Player hit for " .. toString(amount) .. " -> energy: " .. toString(getEnergy()));

    -- The last hit: applyDamage already requested the RagDollState, which takes over from here.
    if (getEnergy() <= 0) then
        return true;
    end

    invulnerableTimer = IFRAME_TIME;
    blinkTimer = IFRAME_BLINK_INTERVAL;

    staggerTimer = profile.staggerTime;
    staggerDuration = profile.staggerTime;
    if (false == isStaggerLocked) then
        playerController:lockMovement("hit", true);
        isStaggerLocked = true;
    end

    animationBlender:blend5(AnimationBlender.ANIM_TAKE_DAMAGE, AnimationBlender.BLEND_WHILE_ANIMATING, 0.05, false);

    -- Away from the enemy. Standing exactly on top of each other: backwards from the facing.
    local directionX = -getFacingSign();
    if (playerPosition.x > sourcePosition.x) then
        directionX = 1;
    elseif (playerPosition.x < sourcePosition.x) then
        directionX = -1;
    end

    applyKnockback(directionX, profile.knockbackHorizontal, profile.knockbackUp);

    return true;
end

function getEnemyProfile(enemyGameObject)
    return EnemyProfiles.get(enemyGameObject:getTagName());
end

function isEnemyAlive(enemyGameObject)
    local enemyAttributes = enemyGameObject:getAttributesComponent();
    if (enemyAttributes == nil) then
        return true;
    end

    local enemyEnergy = enemyAttributes:getAttributeValueByName("Energy");
    if (enemyEnergy == nil) then
        return true;
    end

    return enemyEnergy:getValueNumber() > 0;
end

function isPlayerInReach(enemyGameObject, profile)
    local delta = prehistoricLax:getPosition() - enemyGameObject:getPosition();
    return math.abs(delta.x) <= profile.reach and math.abs(delta.y) <= profile.reachVertical;
end

-- How far the current swing has come, 0 at the first frame of the punch clip and 1 at its last.
function getAttackProgress()
    if (true == overlayTimingAvailable) then
        return animationBlender:getOverlayProgress();
    end
    return attackTime / ATTACK_FALLBACK_DURATION;
end

-- Trade rule: the player's swing is in its active window and he faces the enemy.
function isPlayerSwingBeatingEnemy(enemyGameObject)
    if (false == isAttacking) then
        return false;
    end

    local progress = getAttackProgress();
    if (progress < ATTACK_HIT_START or progress > ATTACK_HIT_END) then
        return false;
    end

    local towardsEnemyX = enemyGameObject:getPosition().x - prehistoricLax:getPosition().x;
    return towardsEnemyX * getFacingSign() > 0;
end

function getEnemyAnimationBlender(enemyGameObject)
    local animationComponent = enemyGameObject:getAnimationComponentV2();
    if (animationComponent == nil) then
        return nil;
    end
    return animationComponent:getAnimationBlender();
end

-- The moment the enemy's attack hurts.
function resolveEnemyImpact(enemyGameObject, profile)
    if (false == isPlayerInReach(enemyGameObject, profile)) then
        -- Dodged.
        do return end;
    end

    if (true == isPlayerSwingBeatingEnemy(enemyGameObject)) then
        -- The player's swing won the exchange.
        do return end;
    end

    hitPlayer(profile.contactDamage, enemyGameObject:getPosition(), profile);
end

function startEnemyAttack(enemyGameObject, profile)
    local enemyId = enemyGameObject:getId();
    local state = enemyCombat[enemyId];

    -- Still attacking or cooling down.
    if (state ~= nil and (state.cooldownTimer > 0 or state.recoverTimer > 0)) then
        do return end;
    end

    local enemyBlender = getEnemyAnimationBlender(enemyGameObject);
    if (enemyBlender ~= nil) then
        enemyBlender:blend5(AnimationBlender.ANIM_ATTACK_1, AnimationBlender.BLEND_WHILE_ANIMATING, 0.1, false);
    end

    state = {};
    state.profile = profile;
    state.cooldownTimer = profile.attackCooldown;
    state.impactTimer = profile.impactDelay;
    state.recoverTimer = profile.attackDuration;
    enemyCombat[enemyId] = state;

    if (profile.impactDelay <= 0) then
        state.impactTimer = -1;
        resolveEnemyImpact(enemyGameObject, profile);
    end
end

-- Advances one enemy's attack. Returns false if the entry can be dropped.
function updateSingleEnemyCombat(enemyId, state, dt)
    local enemyGameObject = AppStateManager:getGameObjectController():getGameObjectFromId(enemyId);
    if (enemyGameObject == nil) then
        return false;
    end

    if (false == isEnemyAlive(enemyGameObject)) then
        return false;
    end

    local profile = state.profile;

    if (state.cooldownTimer > 0) then
        state.cooldownTimer = state.cooldownTimer - dt;
    end

    if (state.impactTimer >= 0) then
        state.impactTimer = state.impactTimer - dt;
        if (state.impactTimer <= 0) then
            state.impactTimer = -1;
            resolveEnemyImpact(enemyGameObject, profile);
        end
    end

    if (state.recoverTimer > 0) then
        state.recoverTimer = state.recoverTimer - dt;
        if (state.recoverTimer <= 0) then
            state.recoverTimer = 0;

            -- The attack is not looping, so without this the enemy would freeze in its last frame.
            -- Back to its locomotion clip: walking by default, or whatever the profile names.
            local enemyBlender = getEnemyAnimationBlender(enemyGameObject);
            if (enemyBlender ~= nil) then
                local locomotionAnimationId = AnimationBlender.ANIM_WALK_NORTH;
                if (profile.locomotionAnimation ~= nil) then
                    locomotionAnimationId = AnimationBlender[profile.locomotionAnimation];
                end
                enemyBlender:blend5(locomotionAnimationId, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);
            end
        end
    end

    local isBusy = (state.recoverTimer > 0 or state.impactTimer >= 0);

    if (false == isBusy and state.cooldownTimer <= 0) then
        if (true == canPlayerBeHit() and true == isPlayerInReach(enemyGameObject, profile)) then
            -- Pressure: the player is still next to the enemy - attack again.
            startEnemyAttack(enemyGameObject, profile);
        else
            -- Nothing left to do for this enemy until the next contact.
            return false;
        end
    end

    return true;
end

function updateEnemyCombat(dt)
    local finishedEnemyIds = {};

    for enemyId, state in pairs(enemyCombat) do
        if (false == updateSingleEnemyCombat(enemyId, state, dt)) then
            table.insert(finishedEnemyIds, enemyId);
        end
    end

    for i = 1, #finishedEnemyIds do
        enemyCombat[finishedEnemyIds[i]] = nil;
    end
end

-- I-frames, blink and stagger.
function updateHitReaction(dt)
    if (invulnerableTimer > 0) then
        invulnerableTimer = invulnerableTimer - dt;

        if (true == IFRAME_BLINK) then
            blinkTimer = blinkTimer - dt;
            if (blinkTimer <= 0) then
                blinkTimer = IFRAME_BLINK_INTERVAL;
                setPlayerVisible(false == playerBlinkVisible);
            end
        end

        if (invulnerableTimer <= 0) then
            invulnerableTimer = 0;
            setPlayerVisible(true);
        end
    end

    if (staggerTimer > 0) then
        staggerTimer = staggerTimer - dt;

        if (knockbackFallbackX ~= 0 and staggerDuration > 0) then
            local factor = staggerTimer / staggerDuration;
            if (factor < 0) then
                factor = 0;
            end
            playerController:getPhysicsComponent():applyRequiredForceForVelocity(Vector3(knockbackFallbackX * factor, 0, 0));
        end

        if (staggerTimer <= 0) then
            releaseStagger();
        end
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
            -- Without it the enemy's script could not know where the blow came from.
            eventData["hitDirection"] = Vector3(getFacingSign(), 0, 0);
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
    -- The cudgel uses it as the knockback direction of a killing blow.
    eventData["attackDirectionX"] = getFacingSign();
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
    energyProgress = mainGameObject:getMyGUIProgressBarComponentFromName("EnergyProgress");
    hitParticle = mainGameObject:getParticleFxComponentFromName("HitParticle");

    playerController = prehistoricLax:getPlayerControllerJumpNRunComponent();
    animationBlender = playerController:getAnimationBlender();
    moneySound = prehistoricLax:getSimpleSoundComponentFromName("Money");
    hurtSound = prehistoricLax:getSimpleSoundComponentFromName("Hurt");

    isAttacking = false;
    attackTime = 0;
    hasHitThisSwing = false;

    isInvulnerable = false;
    invulnerableTimer = 0;
    blinkTimer = 0;
    playerBlinkVisible = true;
    staggerTimer = 0;
    staggerDuration = 0;
    isStaggerLocked = false;
    knockbackFallbackX = 0;
    enemyCombat = {};

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
    animationBlender:registerAnimation(AnimationBlender.ANIM_IDLE_1, "Boy 1 Idle");
    animationBlender:registerAnimation(AnimationBlender.ANIM_IDLE_2, "Boy 1 Idle Turn Left");
    animationBlender:registerAnimation(AnimationBlender.ANIM_IDLE_3, "Boy 1 Idle Turn Right");
    animationBlender:registerAnimation(AnimationBlender.ANIM_WALK_NORTH, "Boy 1 Walk");
    animationBlender:registerAnimation(AnimationBlender.ANIM_WALK_SOUTH, "Boy 1 Walk Backwards");
    animationBlender:registerAnimation(AnimationBlender.ANIM_WALK_WEST, "Boy 1 Walk Turn Left");
    animationBlender:registerAnimation(AnimationBlender.ANIM_WALK_EAST, "Boy 1 Walk Turn Right");
    animationBlender:registerAnimation(AnimationBlender.ANIM_JUMP_START, "Boy 1 Jump Up1");
    animationBlender:registerAnimation(AnimationBlender.ANIM_JUMP_WALK, "Boy 1 Jump Up1");
    animationBlender:registerAnimation(AnimationBlender.ANIM_HIGH_JUMP_END, "Boy 1 Get Up");
    animationBlender:registerAnimation(AnimationBlender.ANIM_JUMP_END, "Boy 1 Land");
    animationBlender:registerAnimation(AnimationBlender.ANIM_FALL, "Boy 1 Damage");
    animationBlender:registerAnimation(AnimationBlender.ANIM_RUN, "Boy 1 Run");
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
        if (fallTime > 0.5) then
            local smokeParticle = prehistoricLax:getParticleFxComponentFromName("DustLand");
            if (smokeParticle ~= nil) then
                smokeParticle:setActivated(true);
            end
        end
    end);
    
    playerController:reactOnAccelerationChanged(function(tempSpeed, topSpeed)
        local smokeParticle = prehistoricLax:getParticleFxComponentFromName("DustStep");
            if (smokeParticle ~= nil) then
                smokeParticle:setActivated(true);
            end
    end);

    --TODO: In physicsmaterialcomponent!
    --playerController:reactOnWallContact(function(otherGameObject, wallNormal)
        -- Always called, no matter what 'Use Wall Separation Mode' is set to. With the flag
        -- on, C++ has already dropped the movement input pointing into the wall, so this is
        -- purely for effects - or for a ledge grab once the flag is switched off.
        --if (otherGameObject == nil) then
        --    do return end;
        --end

       -- otherGameObject = AppStateManager:getGameObjectController():castGameObject(otherGameObject);
       -- if (otherGameObject:getTagName() == "Spikes") then
            --applyDamage(20);
       -- end
    --end);

    -----------------------------------------------------------------------------------------
    -- Pickups
    -----------------------------------------------------------------------------------------
    areaOfInterestComponent:reactOnEnter(function(otherGameObject)
        otherGameObject = AppStateManager:getGameObjectController():castGameObject(otherGameObject);
        if (otherGameObject:getCategory() == "Item") then
            if (otherGameObject:getTagName() == "Coin") then
                AppStateManager:getGameObjectController():deleteGameObject(otherGameObject:getId());
                attributesComponent:addAttributeNumber("Coins", 1);
                moneySound:setActivated(true);
            elseif (otherGameObject:getTagName() == "Energy") then
                AppStateManager:getGameObjectController():deleteGameObject(otherGameObject:getId());
                setEnergy(getEnergy() + 25);
            end
        elseif (otherGameObject:getCategory() == "Enemy") then
            -- Intentionally nothing: enemy damage goes through onPlayerEnemyContactOnce and the
            -- enemy's attack now. Damaging here as well would hit the player twice, and without
            -- the i-frames and the knockback of hitPlayer().
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

    -- Release the stagger lock and make sure the player is not left invisible by the i-frame
    -- blink, before the references are dropped.
    releaseStagger();
    invulnerableTimer = 0;
    setPlayerVisible(true);
    enemyCombat = {};

    playerController = nil;
    animationBlender = nil;
end

---------------------------------------------------------------------------------------------------
-- The swing timing. The walking state stays completely untouched and keeps animating the player
-- while he hits.
---------------------------------------------------------------------------------------------------
function updateAttack(dt)
    if (false == isAttacking) then
        do return end;
    end

    -- Going down or stepping into a portal cancels the swing.
    if (playerController:isInState("WalkingStateJumpNRun") == false) then
        stopAttack();
        do return end;
    end

    attackTime = attackTime + dt;

    -- How far the swing has come, 0 at the first frame of the punch clip and 1 at its last.
    local progress = getAttackProgress();

    -----------------------------------------------------------------------------------------
    -- Hit detection via the front rays - only as a fallback, see USE_FRONT_RAY_HIT.
    -----------------------------------------------------------------------------------------
    if (true == USE_FRONT_RAY_HIT and false == hasHitThisSwing and progress >= ATTACK_HIT_START and progress <= ATTACK_HIT_END) then
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

---------------------------------------------------------------------------------------------------
-- Called by the LuaScriptComponent every frame.
--
-- Attention: the former version returned right at the top while 'isDamaged' was true - and that
-- flag was set by onPlayerDangerContact but never cleared again. After the first touch of
-- something dangerous the swing was never timed again. The hit reaction has its own timers now.
---------------------------------------------------------------------------------------------------
PrehistoricLax["update"] = function(dt)
    if (playerController == nil) then
        do return end;
    end

    updateHitReaction(dt);
    updateEnemyCombat(dt);
    updateAttack(dt);
end

PrehistoricLax["onPlayerDead"] = function(eventData)
    playerController:requestState("RagDollState");
end

-- Damage over time while touching something dangerous (spikes, lava). No knockback and no
-- i-frames of its own - but it respects the i-frames of an enemy hit.
PrehistoricLax["onPlayerDangerContact"] = function(gameObject0, gameObject1, contact)
    if (false == canPlayerBeHit()) then
        do return end;
    end

    applyDamage(0.2);

    if (animationBlender:isAnimationActive(AnimationBlender.ANIM_TAKE_DAMAGE) == false) then
        animationBlender:blend5(AnimationBlender.ANIM_TAKE_DAMAGE, AnimationBlender.BLEND_WHILE_ANIMATING, 0.1, false);
        if (hurtSound ~= nil) then
            hurtSound:setActivated(true);
        end
    end
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

    -- The ragdoll owns the body now: no stagger lock, no knockback fallback, no blinking.
    releaseStagger();
    invulnerableTimer = 0;
    setPlayerVisible(true);

    -- No input while lying on the floor. The owner name matters: only 'ragdoll' can release
    -- this lock again, so a pickup animation running at the same time cannot unlock it.
    playerController:lockMovement("ragdoll", true);
    local ragDollComponent = playerController:getPhysicsRagDollComponent();
    ragDollComponent:setState("Ragdolling");
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

        --playerController:requestState("WalkingStateJumpNRun");
    end
end

RagDollState["exit"] = function(gameObject)
    local ragDollComponent = playerController:getPhysicsRagDollComponent();
    if (ragDollComponent ~= nil) then
        ragDollComponent:setState("Inactive");
    end

    -- The ragdoll left the body with whatever velocity and spin the push gave it. Without this
    -- the walking state would inherit both and the player would walk off sideways, spinning.
    --playerController:getPhysicsComponent():resetForce();

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

---------------------------------------------------------------------------------------------------
-- Called once when the player's body starts touching an enemy's body. See the "Enemy contact
-- combat" block at the top: the touch starts the ENEMY's attack, the damage lands at its impact.
---------------------------------------------------------------------------------------------------
PrehistoricLax["onPlayerEnemyContactOnce"] = function(gameObject0, gameObject1, contact)
    if (prehistoricLax == nil or playerController == nil) then
        do return end;
    end

    local enemyGameObject = nil;

    if (gameObject0 ~= nil) then
        local candidate = AppStateManager:getGameObjectController():castGameObject(gameObject0);
        if (candidate:getCategory() == "Enemy") then
            enemyGameObject = candidate;
        end
    end

    if (enemyGameObject == nil and gameObject1 ~= nil) then
        local candidate = AppStateManager:getGameObjectController():castGameObject(gameObject1);
        if (candidate:getCategory() == "Enemy") then
            enemyGameObject = candidate;
        end
    end

    if (enemyGameObject == nil) then
        do return end;
    end

    -- A dying enemy (energy 0, deleted delayed) does not attack anymore.
    if (false == isEnemyAlive(enemyGameObject)) then
        do return end;
    end

    if (false == canPlayerBeHit()) then
        do return end;
    end

    -- Trade rule: the player's swing is already in its active window and faces the enemy - the
    -- cudgel hits first, the enemy does not get to attack.
    if (true == isPlayerSwingBeatingEnemy(enemyGameObject)) then
        do return end;
    end

    startEnemyAttack(enemyGameObject, getEnemyProfile(enemyGameObject));
end