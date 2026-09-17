module("MainGameObject", package.seeall);
-- Scene: Level1Sector

require("init");

local mainGameObject = nil

local luizius = nil;

local cameraComponent = nil;
local animationBlenderLuizius = nil;

MainGameObject = {}

MainGameObject["connect"] = function(gameObject)
    PointerManager:showMouse(false);
    mainGameObject = AppStateManager:getGameObjectController():castGameObject(gameObject);
    
    cameraComponent = AppStateManager:getGameObjectController():getGameObjectFromName("GameCamera"):getCameraComponent();
    -- Important: Camera switch
    cameraComponent:setActivated(true);
    
    luizius = AppStateManager:getGameObjectController():getGameObjectFromId("1085244942");

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
    
    luizius:getCameraBehaviorFollow2DComponent():setActivated(true);
    
    luizius:getNodeTrackComponent():reactOnEndOfPathReached(function()
        mainGameObject:getFadeComponentFromIndex(1):setActivated(true);
    end);
    
     --mainGameObject:getFadeComponent():setActivated(false);
end

MainGameObject["disconnect"] = function()
    PointerManager:showMouse(true);
    cameraComponent:setActivated(false);
    luizius:getCameraBehaviorFollow2DComponent():setActivated(false);
    AppStateManager:getGameObjectController():undoAll();
end