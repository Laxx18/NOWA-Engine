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
local agathe = nil;
local spellBall = nil;
local animationBlenderLax = nil;
local animationBlenderEmma = nil;
local animationBlenderLuizius = nil;
local pathFollowEmma = nil;
local agathePhysicsRagComp = nil;


MainGameObject = {}

MainGameObject["connect"] = function(gameObject)
    PointerManager:showMouse(false);
    mainGameObject = AppStateManager:getGameObjectController():castGameObject(gameObject);
    AppStateManager:getCameraManager():setMoveCameraWeight(0);
    AppStateManager:getCameraManager():setRotateCameraWeight(0);
    
    cameraComponent = AppStateManager:getGameObjectController():getGameObjectFromName("GameCamera"):getCameraComponent();
    -- Important: Camera switch
    cameraComponent:setActivated(true);
    
    atmosphereComonent = cameraComponent:getOwner():getAtmosphereComponent();
    
    lax = AppStateManager:getGameObjectController():getGameObjectFromId("169236464");
    emma = AppStateManager:getGameObjectController():getGameObjectFromId("757446456");
    agathe =  AppStateManager:getGameObjectController():getGameObjectFromId("3425782733");
    oldMan = AppStateManager:getGameObjectController():getGameObjectFromId("524695244");
    luizius = AppStateManager:getGameObjectController():getGameObjectFromId("3895382773");
    spellBall = AppStateManager:getGameObjectController():getGameObjectFromId("2737582806");
    bed = AppStateManager:getGameObjectController():getGameObjectFromId("3438074172");
    
    animationBlenderLax = lax:getAnimationSequenceComponent():getAnimationBlender();
    animationBlenderEmma = emma:getAnimationComponentV2():getAnimationBlender();
    animationBlenderLuizius = luizius:getAnimationComponentV2():getAnimationBlender();
    pathFollowEmma = emma:getAiPathFollowComponent();
    pathFollowEmma:setActivated(false);
    agathePhysicsRagComp = agathe:getPhysicsRagDollComponentV2();
    
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
    
    animationBlenderLuizius:registerAnimation(AnimationBlender.ANIM_IDLE_1, "idle-01");
    animationBlenderLuizius:registerAnimation(AnimationBlender.ANIM_IDLE_2, "idle-02");
    animationBlenderLuizius:registerAnimation(AnimationBlender.ANIM_IDLE_3, "joke");
    animationBlenderLuizius:registerAnimation(AnimationBlender.ANIM_WALK_NORTH, "Boy 1 Walk");
    animationBlenderLuizius:registerAnimation(AnimationBlender.ANIM_WALK_SOUTH, "Boy 1 Walk Backwards");
    
    animationBlenderLuizius:registerAnimation(AnimationBlender.ANIM_WALK_NORTH, "walk-01");
    animationBlenderLuizius:registerAnimation(AnimationBlender.ANIM_WALK_SOUTH, "walk-01");
    animationBlenderLuizius:registerAnimation(AnimationBlender.ANIM_WALK_WEST, "walk-01");
    animationBlenderLuizius:registerAnimation(AnimationBlender.ANIM_WALK_EAST, "walk-01");
    animationBlenderLuizius:registerAnimation(AnimationBlender.ANIM_JUMP_START, "jump-01");
    animationBlenderLuizius:registerAnimation(AnimationBlender.ANIM_JUMP_WALK, "jump-0p");
    animationBlenderLuizius:registerAnimation(AnimationBlender.ANIM_HIGH_JUMP_END, "jump-pose");
    animationBlenderLuizius:registerAnimation(AnimationBlender.ANIM_JUMP_END, "jump-pose");
    animationBlenderLuizius:registerAnimation(AnimationBlender.ANIM_FALL, "jump-pose");
    animationBlenderLuizius:registerAnimation(AnimationBlender.ANIM_RUN, "run-01");
    animationBlenderLuizius:registerAnimation(AnimationBlender.ANIM_ATTACK_1, "attack-01");
    animationBlenderLuizius:registerAnimation(AnimationBlender.ANIM_ATTACK_2, "attack-02");
    animationBlenderLuizius:registerAnimation(AnimationBlender.ANIM_CAST_SPELL_1, "cast-01");
    animationBlenderLuizius:registerAnimation(AnimationBlender.ANIM_CAST_SPELL_2, "cast-02");
    animationBlenderLuizius:registerAnimation(AnimationBlender.ANIM_CAST_SPELL_3, "cast-03");
    
    animationBlenderLuizius:init1(AnimationBlender.ANIM_IDLE_1, true);
    
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
    
    --mainGameObject:getLuaScriptComponent():callMethodOnce("RotateLaxToEmma",  function()
       --   local resultOrientation = MathHelper:faceTarget(lax:getSceneNode(), emma:getSceneNode());
     --     lax:getPhysicsActiveComponent():applyOmegaForceRotateTo(resultOrientation, Vector3.UNIT_Y, 10);
    --end);
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

MainGameObject["LuiziusTimePoint"] = function(timePointSec)
     log("--->LuiziusTimePoint: " .. toString(timePointSec));
    mainGameObject:getLuaScriptComponent():callMethodOnce("ApearLuizius", function()
        luizius:getNodeTrackComponentFromName("AppearNodeTrack"):setActivated(true);
    end)
end

MainGameObject["CastSleepSpellTimePoint"] = function(timePointSec)
    mainGameObject:getLuaScriptComponent():callMethodOnce("CastSpeelLuizius", function()
        spellBall:setVisible(true);
        spellBall:getParticleFxComponent():setActivated(true);
        animationBlenderLuizius:blend5(AnimationBlender.ANIM_CAST_SPELL_1, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);
    end)
    
    mainGameObject:getLuaScriptComponent():callDelayedMethod(function()
        spellBall:getNodeTrackComponent():setActivated(true);
        
        spellBall:getNodeTrackComponent():reactOnEndOfPathReached(function(trackedGameObject)
              agathePhysicsRagComp:setState("Ragdolling");
              mainGameObject:getLuaScriptComponent():callDelayedMethod(function()
                  agathe:getParticleFxComponent():setActivated(true);
              end, 1)
        end);
    end, 2)
end

MainGameObject["BreakDoorTimePoint"] = function(timePointSec)
     log("--->BreakDoorTimePoint: " .. toString(timePointSec));
    mainGameObject:getLuaScriptComponent():callMethodOnce("BreakDoorLuizius", function()
         animationBlenderLuizius:blend5(AnimationBlender.ANIM_CAST_SPELL_2, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);
    end)
end

MainGameObject["BreakInTimePoint"] = function(timePointSec)
    log("--->BreakInTimePoint: " .. toString(timePointSec));
    mainGameObject:getLuaScriptComponent():callMethodOnce("ActivateLuizius", function()
        luizius:getNodeTrackComponentFromName("AppearNodeTrack"):setActivated(false);
        luizius:getNodeTrackComponentFromName("BreakInNodeTrack"):setActivated(true);
        animationBlenderLuizius:blend5(AnimationBlender.ANIM_IDLE_2, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);
    end)
   
end