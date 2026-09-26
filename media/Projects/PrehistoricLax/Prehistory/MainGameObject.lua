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
local spellBall2 = nil;
local doorPhysics = nil;
local crystalOrb = nil;
local bat = nil;
local animationBlenderLax = nil;
local animationBlenderEmma = nil;
local animationBlenderLuizius = nil;
local pathFollowEmma = nil;
local agathePhysicsRagComp = nil;

local laxShouldTurn = false;
local luiziusShouldTurn = false;


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
    spellBall2 = AppStateManager:getGameObjectController():getGameObjectFromId("1998158100");
    doorPhysics = AppStateManager:getGameObjectController():getGameObjectFromId("3406634031");
    bed = AppStateManager:getGameObjectController():getGameObjectFromId("3438074172");
    crystalOrb = AppStateManager:getGameObjectController():getGameObjectFromId("2292878869");
    bat = AppStateManager:getGameObjectController():getGameObjectFromId("1371565728");
    
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
    animationBlenderLax:registerAnimation(AnimationBlender.ANIM_GETUP, "Boy 1 Get Up"); 
    
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
    agathePhysicsRagComp:setState("Inactive");
    agathe:getAnimationComponentV2():setActivated(true);
    bed:getParticleFxComponent():setActivated(false);
    spellBall:getParticleFxComponent():setActivated(false);
    spellBall2:getParticleFxComponent():setActivated(false);
    agathe:getParticleFxComponent():setActivated(false);
    
    mainGameObject:getMyGUIButtonComponentFromName("SkipButton"):reactOnMouseButtonClick(function()
        AppStateManager:changeAppState("GameState");
    end);
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
end

MainGameObject["GoToTimePoint"] = function(timePointSec)
    lax:getAnimationSequenceComponent():setActivated(false);
    animationBlenderLax:blend5(AnimationBlender.ANIM_IDLE_1, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);

    pathFollowEmma:setActivated(true);
    animationBlenderEmma:blend5(AnimationBlender.ANIM_WALK_NORTH, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);

    pathFollowEmma:reactOnPathGoalReached(function()
        animationBlenderEmma:blend5(AnimationBlender.ANIM_IDLE_1, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);

        -- Only arms the turning; the work happens frame by frame below. Lax must not
        -- start turning before emma has actually arrived.
        laxShouldTurn = true;
        mainGameObject:getLuaScriptComponent():callDelayedMethod(function()
           laxShouldTurn = false;
           animationBlenderLax:blend5(AnimationBlender.ANIM_IDLE_1, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);
        end, 3);
        
    end);
end

MainGameObject["DarkTimePoint"] = function(timePointSec)
    atmosphereComonent:setTimeMultiplicator(0.5);
     mainGameObject:getSimpleSoundComponentFromIndex(0):setActivated(false);
     cameraComponent:getOwner():getHdrEffectComponent():setEffectName("Neon Night");
end

MainGameObject["SleepTimePoint"] = function(timePointSec)
    lax:getPhysicsActiveComponent():setConstraintDirection(Vector3.ZERO);
    lax:getPhysicsComponent():setPosition(Vector3(-11.2204, 1.33183, -14.744));
    lax:getPhysicsComponent():setOrientation(MathHelper:degreesToQuat(Vector3(-85, 90, 0)));
    mainGameObject:getSimpleSoundComponentFromIndex(1):setActivated(true);
    bed:getParticleFxComponent():setActivated(true);
    
    atmosphereComonent:setTimeMultiplicator(0.0001);
    emma:setVisible(false);
    oldMan:setVisible(false);
    oldMan:getSpeechBubbleComponent():setActivated(false);
end

MainGameObject["CameraDriveTimePoint"] = function(timePointSec)
    atmosphereComonent:setTimeMultiplicator(0.0001);
    cameraComponent:getOwner():getNodeTrackComponent():setActivated(true);
end

MainGameObject["LuiziusTimePoint"] = function(timePointSec)
    luizius:getNodeTrackComponentFromName("AppearNodeTrack"):setActivated(true);
    luizius:getSimpleSoundComponent():setActivated(true);
end

MainGameObject["CastSleepSpellTimePoint"] = function(timePointSec)
    spellBall:setVisible(true);
    spellBall:getParticleFxComponent():setActivated(true);
    animationBlenderLuizius:blend5(AnimationBlender.ANIM_CAST_SPELL_1, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);
    
    mainGameObject:getLuaScriptComponent():callDelayedMethod(function()
        spellBall:getNodeTrackComponent():setActivated(true);
        
        spellBall:getNodeTrackComponent():reactOnEndOfPathReached(function(trackedGameObject)
              agathePhysicsRagComp:setState("Ragdolling");
              agathe:getSimpleSoundComponent():setActivated(true);
              
              mainGameObject:getLuaScriptComponent():callDelayedMethod(function()
                  agathe:getParticleFxComponent():setActivated(true);
                  agathePhysicsRagComp:applyForce(Vector3(0, 1000, 1000));
                  spellBall:setVisible(false);
                  spellBall:getParticleFxComponent():setActivated(false);
                  
                  spellBall2:setVisible(true);
                  spellBall2:getParticleFxComponent():setActivated(true);
                  spellBall2:getNodeTrackComponent():setActivated(true);
                  
                  spellBall2:getNodeTrackComponent():reactOnEndOfPathReached(function(trackedGameObject)
                      spellBall2:setVisible(false);
                      spellBall2:getParticleFxComponent():setActivated(false);
                      doorPhysics:getJointHingeComponent():setBreakForce(100000);
                      doorPhysics:getPhysicsActiveComponent():applyForce(Vector3(1000, 1000, 1000));
                      doorPhysics:getSimpleSoundComponent():setActivated(true);
                  end);
              end, 1)
        end);
    end, 2)
end

MainGameObject["BreakDoorTimePoint"] = function(timePointSec)
     animationBlenderLuizius:blend5(AnimationBlender.ANIM_CAST_SPELL_2, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);
end

MainGameObject["BreakInTimePoint"] = function(timePointSec)
    luizius:getNodeTrackComponentFromName("BreakInNodeTrack"):setActivated(true);
    animationBlenderLuizius:blend5(AnimationBlender.ANIM_IDLE_2, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);

    luizius:getNodeTrackComponentFromName("BreakInNodeTrack"):reactOnEndOfPathReached(function(trackedGameObject)
        luizius:getSimpleSoundComponent():setActivated(true);
        luizius:getTagPointComponent():setSourceId("2292878869");

        -- Only arms the turning here; the work happens above, frame by frame.
        luiziusShouldTurn = true;

        mainGameObject:getLuaScriptComponent():callDelayedMethod(function()
            luiziusShouldTurn = false;
            luizius:getNodeTrackComponentFromName("LeaveNodeTrack"):setActivated(true);
            crystalOrb:getParticleFxComponent():setActivated(false);
        end, 3);
    end);
end

MainGameObject["CameraDriveBackTimePoint"] = function(timePointSec)
    cameraComponent:getOwner():getNodeTrackComponent():setReverse(true);
    cameraComponent:getOwner():getNodeTrackComponent():setActivated(true);
end

MainGameObject["LaxGetUpTimePoint"] = function(timePointSec)
    bed:getParticleFxComponent():setActivated(false);
    animationBlenderLax:blend5(AnimationBlender.ANIM_GETUP, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);
    
    lax:getPhysicsComponent():setPosition(Vector3(-11.5038, 0.64152, -14.1525));
    lax:getPhysicsComponent():setOrientation(MathHelper:degreesToQuat(Vector3(0, 20, 0)));
    
    mainGameObject:getLuaScriptComponent():callDelayedMethod(function()
          animationBlenderLax:blend5(AnimationBlender.ANIM_IDLE_2, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);
          lax:getSpeechBubbleComponentFromIndex(0):setActivated(true);
          -- Set bat to hand
          local handPosition = lax:getTagPointComponent():getBonePosition("Boy 1 R Hand");
          local handOrientation = lax:getTagPointComponent():getBoneOrientation("Boy 1 R Hand");
          bat:getSceneNode():setPosition(Vector3(handPosition.x, handPosition.y - 0.2, handPosition.z));
          bat:getSceneNode():setOrientation(handOrientation);
          
          lax:getTagPointComponent():setSourceId("1371565728");
     end, 2);
end

MainGameObject["LaxFollow1TimePoint"] = function(timePointSec)
    animationBlenderLax:blend5(AnimationBlender.ANIM_RUN, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);
    animationBlenderLax:setAnimationSpeed(1.5);
    lax:getAiPathFollowComponentFromIndex(0):setActivated(true);
    lax:getAiPathFollowComponentFromIndex(0):reactOnPathGoalReached(function()
       lax:getSpeechBubbleComponentFromIndex(0):setActivated(false);
       lax:getSpeechBubbleComponentFromIndex(1):setActivated(true);
       animationBlenderLax:blend5(AnimationBlender.ANIM_IDLE_1, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);
       lax:getAiPathFollowComponentFromIndex(0):setActivated(false);
    end);
    cameraComponent:getOwner():getNodeTrackComponent():setReverse(false);
    cameraComponent:getOwner():getNodeTrackComponent():setActivated(true);
end

MainGameObject["LaxFollow2TimePoint"] = function(timePointSec)
    lax:getAiPathFollowComponentFromIndex(1):setActivated(true);
    animationBlenderLax:blend5(AnimationBlender.ANIM_RUN, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);
end

MainGameObject["LaxFollow2EndTimePoint"] = function(timePointSec)
    mainGameObject:getFadeComponentFromIndex(1):setActivated(true);
    mainGameObject:getFadeComponentFromIndex(1):reactOnFadeCompleted(function() 
         AppStateManager:changeAppState("Level1SectorState");
    end);
end

MainGameObject["update"] = function(dt)
   if (true == laxShouldTurn) then
        local toEmma = emma:getPosition() - lax:getPosition();
        local resultOrientation = MathHelper:faceDirection(lax:getOrientation(), toEmma, lax:getDefaultDirection());
        
        mainGameObject:getLuaScriptComponent():callMethodOnce("TurnAnim", function()
            animationBlenderLax:blend5(AnimationBlender.ANIM_IDLE_2, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);
        end);

        -- UNIT_Y, not UNIT_SCALE: this is a flat world, so only yaw is allowed - no tilting.
        -- Strength 3 rather than 10, because applyOmegaForceRotateTo clamps at
        -- MAX_OMEGA = 2.0 rad/s anyway and anything above roughly 2 makes no difference.
        lax:getPhysicsActiveComponent():applyOmegaForceRotateTo(resultOrientation, Vector3.UNIT_Y, 10);
  end
  if (true == luiziusShouldTurn) then
        local resultQuat = MathHelper:faceDirectionSlerp(luizius:getOrientation(), Vector3(1, 0, 0), luizius:getDefaultDirection(), dt, 10);
        luizius:getSceneNode():setOrientation(resultQuat);
   end
end