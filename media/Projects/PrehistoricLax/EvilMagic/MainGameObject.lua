module("MainGameObject", package.seeall);
-- Scene: EvilMagic

require("init");

local mainGameObject = nil

local luizius = nil;
local innerSphere = nil;
local sphere = nil;

local cameraComponent = nil;
local animationBlenderLuizius = nil;

MainGameObject = {}

MainGameObject["connect"] = function(gameObject)
    PointerManager:showMouse(false);
    mainGameObject = AppStateManager:getGameObjectController():castGameObject(gameObject);
    AppStateManager:getCameraManager():setMoveCameraWeight(0);
    AppStateManager:getCameraManager():setRotateCameraWeight(0);
    
    cameraComponent = AppStateManager:getGameObjectController():getGameObjectFromName("GameCamera"):getCameraComponent();
    -- Important: Camera switch
    cameraComponent:setActivated(true);
    
    luizius = AppStateManager:getGameObjectController():getGameObjectFromId("1741058361");
    innerSphere = AppStateManager:getGameObjectController():getGameObjectFromId("3809806162");
    sphere = AppStateManager:getGameObjectController():getGameObjectFromId("1858869321");
    
    animationBlenderLuizius = luizius:getAnimationComponentV2():getAnimationBlender();
    
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
    
    sphere:getAttributeEffectComponent():setActivated(false);
    sphere:getPhysicsExplosionComponent():setActivated(false);
    sphere:getParticleFxComponent():setActivated(false);
     luizius:getSimpleSoundComponentFromName("Speak"):setActivated(true);
     luizius:getSimpleSoundComponentFromName("Laugh"):setActivated(false);
     luizius:getTransformEaseComponent():setActivated(true);
     mainGameObject:getFadeComponent():setActivated(false);
     
     mainGameObject:getMyGUIButtonComponentFromName("SkipButton"):reactOnMouseButtonClick(function()
        AppStateManager:changeAppState("Level1");
    end);
end

MainGameObject["disconnect"] = function()
    PointerManager:showMouse(true);
    cameraComponent:setActivated(false);
    AppStateManager:getCameraManager():setMoveCameraWeight(1);
    AppStateManager:getCameraManager():setRotateCameraWeight(1);
    AppStateManager:getGameObjectController():undoAll();
end

MainGameObject["ReleaseCreaturesTimePoint"] = function(timePointSec)
    sphere:getAttributeEffectComponent():setActivated(true);
    -- Delete inner sphere
    AppStateManager:getGameObjectController():deleteDelayedGameObject(innerSphere:getId(), 1);
    mainGameObject:getLuaScriptComponent():callDelayedMethod(function()
        sphere:getPhysicsExplosionComponent():setActivated(true);
        sphere:getParticleFxComponent():setActivated(false);
        luizius:getSimpleSoundComponentFromName("Speak"):setActivated(false);
        luizius:getSimpleSoundComponentFromName("Laugh"):setActivated(true);
        luizius:getTransformEaseComponent():setActivated(false);
        animationBlenderLuizius:blend5(AnimationBlender.ANIM_IDLE_3, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);
    end, 2);
end

MainGameObject["FadeOutTimePoint"] = function(timePointSec)
    mainGameObject:getFadeComponentFromIndex(1):setActivated(true);
    mainGameObject:getFadeComponentFromIndex(1):reactOnFadeCompleted(function() 
         AppStateManager:changeAppState("GameState");
    end);
end

--MainGameObject["update"] = function(dt)
    --physicsActiveComponent:applyOmegaForce(Vector3(0, 10, 0));
--end