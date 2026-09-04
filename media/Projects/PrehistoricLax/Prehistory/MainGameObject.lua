module("MainGameObject", package.seeall);
-- Scene: Prehistory

require("init");

mainGameObject = nil

local cameraComponent = nil;
local atmosphereComonent = nil;
local lax = nil;
local emma = nil;
local oldMan = nil;
local bed = nil;
local luizius = nil;
local animationBlenderLax = nil;
local animationBlenderEmma = nil;
local pathFollowEmma = nil;
local physicsLuizius = nil;
local pathFollowLuizius = nil;

MainGameObject = {}

MainGameObject["connect"] = function(gameObject)
    PointerManager:showMouse(false);
    mainGameObject = AppStateManager:getGameObjectController():castGameObject(gameObject);
    AppStateManager:getCameraManager():setMoveCameraWeight(0);
    AppStateManager:getCameraManager():setRotateCameraWeight(0);
    
    cameraComponent = AppStateManager:getGameObjectController():getGameObjectFromName("GameCamera"):getCameraComponent();
    --cameraComponent:setActivated(true);
    
    atmosphereComonent = cameraComponent:getOwner():getAtmosphereComponent();
    
    lax = AppStateManager:getGameObjectController():getGameObjectFromId("169236464");
    emma = AppStateManager:getGameObjectController():getGameObjectFromId("757446456");
    oldMan = AppStateManager:getGameObjectController():getGameObjectFromId("524695244");
    luizius = AppStateManager:getGameObjectController():getGameObjectFromId("3895382773");
    bed = AppStateManager:getGameObjectController():getGameObjectFromId("3438074172");
    
    animationBlenderLax = lax:getAnimationSequenceComponent():getAnimationBlender();
    animationBlenderEmma = emma:getAnimationComponentV2():getAnimationBlender();
    pathFollowEmma = emma:getAiPathFollowComponent();
    pathFollowEmma:setActivated(false);
    
    physicsLuizius =  luizius:getPhysicsActiveComponent();
    pathFollowLuizius = luizius:getJointPathFollowComponent();
    
    animationBlenderLax:registerAnimation(AnimationBlender.ANIM_IDLE_1, "Boy 1 Idle");
    animationBlenderLax:registerAnimation(AnimationBlender.ANIM_IDLE_2, "Boy 1 Idle Turn Left");
    animationBlenderLax:registerAnimation(AnimationBlender.ANIM_IDLE_3, "Boy 1 Idle Turn Right");
    animationBlenderLax:registerAnimation(AnimationBlender.ANIM_WALK_NORTH, "Boy 1 Walk");
    animationBlenderLax:registerAnimation(AnimationBlender.ANIM_WALK_SOUTH, "Boy 1 Walk Backwards");
    animationBlenderLax:registerAnimation(AnimationBlender.ANIM_WALK_WEST, "Boy 1 Walk Turn Left");
    animationBlenderLax:registerAnimation(AnimationBlender.ANIM_WALK_EAST, "Boy 1 Walk Turn Right");
    animationBlenderLax:registerAnimation(AnimationBlender.ANIM_JUMP_START, "Boy 1 Jump Up1");
    animationBlenderLax:registerAnimation(AnimationBlender.ANIM_JUMP_WALK, "Boy 1 Jump Up1");
    animationBlenderLax:registerAnimation(AnimationBlender.ANIM_HIGH_JUMP_END, "Boy 1 Get Up");
    animationBlenderLax:registerAnimation(AnimationBlender.ANIM_JUMP_END, "Boy 1 Land");
    animationBlenderLax:registerAnimation(AnimationBlender.ANIM_FALL, "Boy 1 Damage");
    animationBlenderLax:registerAnimation(AnimationBlender.ANIM_RUN, "Boy 1 Run");
    --animationBlenderLax:registerAnimation(AnimationBlender.ANIM_SNEAK, "Take_damage");
    --animationBlenderLax:registerAnimation(AnimationBlender.ANIM_DUCK, "Land2");
    --animationBlenderLax:registerAnimation(AnimationBlender.ANIM_HALT, "Halt");
    animationBlenderLax:registerAnimation(AnimationBlender.ANIM_ATTACK_1, "Boy 1 Punch");
    animationBlenderLax:registerAnimation(AnimationBlender.ANIM_ATTACK_2, "Boy 1 Heavy Kick");
    animationBlenderLax:registerAnimation(AnimationBlender.ANIM_TALK_1, "Boy 1 Idle");
    animationBlenderLax:registerAnimation(AnimationBlender.ANIM_SALTO, "Boy 1 Air Flip");
    animationBlenderLax:registerAnimation(AnimationBlender.ANIM_PICKUP_1, "Boy 1 Idle Pick Up Item");
    
    animationBlenderEmma:registerAnimation(AnimationBlender.ANIM_IDLE_1, "Girl 1 Idle");
    animationBlenderEmma:registerAnimation(AnimationBlender.ANIM_IDLE_2, "Girl 1 Idle Turn Left");
    animationBlenderEmma:registerAnimation(AnimationBlender.ANIM_IDLE_3, "Girl 1 Idle Turn Right");
    animationBlenderEmma:registerAnimation(AnimationBlender.ANIM_WALK_NORTH, "Girl 1 Walk");
    animationBlenderEmma:registerAnimation(AnimationBlender.ANIM_WALK_SOUTH, "Girl 1 Walk Backwards");
    animationBlenderEmma:registerAnimation(AnimationBlender.ANIM_WALK_WEST, "Girl 1 Walk Turn Left");
    animationBlenderEmma:registerAnimation(AnimationBlender.ANIM_WALK_EAST, "Girl 1 Walk Turn Right");
    animationBlenderEmma:registerAnimation(AnimationBlender.ANIM_JUMP_START, "Girl 1 Jump Up1");
    animationBlenderEmma:registerAnimation(AnimationBlender.ANIM_JUMP_WALK, "Girl 1 Jump Up1");
    animationBlenderEmma:registerAnimation(AnimationBlender.ANIM_HIGH_JUMP_END, "Girl 1 Get Up");
    animationBlenderEmma:registerAnimation(AnimationBlender.ANIM_JUMP_END, "Girl 1 Land");
    animationBlenderEmma:registerAnimation(AnimationBlender.ANIM_FALL, "Girl 1 Damage");
    animationBlenderEmma:registerAnimation(AnimationBlender.ANIM_RUN, "Girl 1 Run");
    --animationBlenderEmma:registerAnimation(AnimationBlender.ANIM_SNEAK, "Take_damage");
    --animationBlenderEmma:registerAnimation(AnimationBlender.ANIM_DUCK, "Land2");
    --animationBlenderEmma:registerAnimation(AnimationBlender.ANIM_HALT, "Halt");
    animationBlenderEmma:registerAnimation(AnimationBlender.ANIM_ATTACK_1, "Girl 1 Punch");
    animationBlenderEmma:registerAnimation(AnimationBlender.ANIM_ATTACK_2, "Girl 1 Heavy Kick");
    animationBlenderEmma:registerAnimation(AnimationBlender.ANIM_TALK_1, "Girl 1 Idle");
    animationBlenderEmma:registerAnimation(AnimationBlender.ANIM_SALTO, "Girl 1 Air Flip");
    animationBlenderEmma:registerAnimation(AnimationBlender.ANIM_PICKUP_1, "Girl 1 Idle Pick Up Item");
    
    animationBlenderLax:init1(AnimationBlender.ANIM_IDLE_1, true);
    
    animationBlenderEmma:init1(AnimationBlender.ANIM_IDLE_1, true);
    
    lax:getAnimationSequenceComponent():setActivated(false);
end

MainGameObject["disconnect"] = function()
    PointerManager:showMouse(true);
    cameraComponent:setActivated(false);
    AppStateManager:getCameraManager():setMoveCameraWeight(1);
    AppStateManager:getCameraManager():setRotateCameraWeight(1);
    AppStateManager:getGameObjectController():undoAll();
end

MainGameObject["WorkTimePoint"] = function(timePointSec)
    lax:getAnimationSequenceComponent():setActivated(true);
    log("--->WorkTimePoint: " .. toString(timePointSec));
end

MainGameObject["GoToTimePoint"] = function(timePointSec)
    log("--->GoToTimePoint: " .. toString(timePointSec));
    lax:getAnimationSequenceComponent():setActivated(false);
    animationBlenderLax:blend5(AnimationBlender.ANIM_IDLE_1, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);
    pathFollowEmma:setActivated(true);
    animationBlenderEmma:blend5(AnimationBlender.ANIM_WALK_NORTH, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);
    
    pathFollowEmma:reactOnPathGoalReached(function()
        animationBlenderEmma:blend5(AnimationBlender.ANIM_IDLE_1, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);
    end)
    
    local resultOrientation = MathHelper:faceTarget(lax:getSceneNode(), emma:getSceneNode());
    lax:getPhysicsActiveComponent():applyOmegaForceRotateTo(resultOrientation, Vector3.UNIT_Y, 1000);
end

MainGameObject["DarkTimePoint"] = function(timePointSec)
    log("--->DarkTimePoint: " .. toString(timePointSec));
    atmosphereComonent:setTimeMultiplicator(0.5);
    mainGameObject:getLuaScriptComponent():callMethodOnce("StopMusic",  function()
         mainGameObject:getSimpleSoundComponentFromIndex(0):setActivated(false);
         cameraComponent:getOwner():getHdrEffectComponent():setEffectName("Neon Night");
    end);
end

MainGameObject["SleepTimePoint"] = function(timePointSec)
     log("--->SleepTimePoint: " .. toString(timePointSec));
    mainGameObject:getLuaScriptComponent():callMethodOnce("TeleportLax",  function()
        log("--->Delayed method: ");
        lax:getPhysicsActiveComponent():setConstraintDirection(Vector3.ZERO);
        lax:getPhysicsComponent():setPosition(Vector3(-11.2204, 1.33183, -14.744));
        lax:getPhysicsComponent():setOrientation(MathHelper:degreesToQuat(Vector3(-85, 90, 0)));
        mainGameObject:getSimpleSoundComponentFromIndex(1):setActivated(true);
        bed:getParticleFxComponent():setActivated(true);
    end);
    
    atmosphereComonent:setTimeMultiplicator(0.0001);
    emma:setVisible(false);
    oldMan:setVisible(false);
    oldMan:getSpeechBubbleComponent():setActivated(false);
end

MainGameObject["CameraDriveTimePoint"] = function(timePointSec)
    log("--->CameraDriveTimePoint: " .. toString(timePointSec));
    mainGameObject:getLuaScriptComponent():callMethodOnce("CameraDrive", function()
        atmosphereComonent:setTimeMultiplicator(0.0001);
        cameraComponent:getOwner():getNodeTrackComponent():setActivated(true);
    end)
end

MainGameObject["BreakInTimePoint"] = function(timePointSec)
    -- Cap the speed: applying force along the tangent every frame with nothing
    -- opposing it accelerates without bound. Once the body moves further than one
    -- spline segment per solver step, FindClosestKnot() latches onto the wrong knot
    -- and the joint yanks it back - a likely source of the instability.
    local maxSpeed = 8.0;

    local direction = pathFollowLuizius:getCurrentMoveDirection();
    local velocity = physicsLuizius:getVelocity();

    if velocity:length() < maxSpeed then
        physicsLuizius:applyForce(direction * 50);
    end
end

MainGameObject["update"] = function(dt)
    
end