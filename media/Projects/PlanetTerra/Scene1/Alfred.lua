module("Alfred", package.seeall);
-- Scene: Scene1

require("init");

local alfred = nil;
local alfredPhysics = nil;
local aiLuaComponent = nil;
local animationBlender = nil;
local movingBehavior = nil;
local playerGo = nil;
local speechBubbleComp = nil;
local mainGo = nil;

Alfred = {}

Alfred["connect"] = function(gameObject)
    alfred = AppStateManager:getGameObjectController():castGameObject(gameObject);
    alfredPhysics = alfred:getPhysicsActiveComponent();
    aiLuaComponent = alfred:getAiLuaComponent();
    animationBlender = alfred:getAnimationComponentV2():getAnimationBlender();
    movingBehavior = aiLuaComponent:getMovingBehavior();
    speechBubbleComp = alfred:getSpeechBubbleComponent();
    mainGo = AppStateManager:getGameObjectController():getGameObjectFromId(MAIN_GAMEOBJECT_ID);
    -- Difference between NONE and STOP. 
    -- If none is on, add no force, so that other behaviors can still move the agent! Only stop adds force, even if its null
    movingBehavior:setBehavior(BehaviorType.STOP);
    movingBehavior:setGoalRadius(2);
    
    animationBlender:registerAnimation(AnimationBlender.ANIM_IDLE_1, "idle-01");
    animationBlender:registerAnimation(AnimationBlender.ANIM_IDLE_2, "idle-02");
    animationBlender:registerAnimation(AnimationBlender.ANIM_IDLE_3, "joke");
    animationBlender:registerAnimation(AnimationBlender.ANIM_WALK_NORTH, "walk-01");
    animationBlender:registerAnimation(AnimationBlender.ANIM_WALK_SOUTH, "walk-01");
    animationBlender:registerAnimation(AnimationBlender.ANIM_WALK_WEST, "walk-01");
    animationBlender:registerAnimation(AnimationBlender.ANIM_WALK_EAST, "walk-01");
    animationBlender:registerAnimation(AnimationBlender.ANIM_JUMP_START, "jump-01");
    animationBlender:registerAnimation(AnimationBlender.ANIM_JUMP_WALK, "jump-0p");
    animationBlender:registerAnimation(AnimationBlender.ANIM_HIGH_JUMP_END, "jump-pose");
    animationBlender:registerAnimation(AnimationBlender.ANIM_JUMP_END, "jump-pose");
    animationBlender:registerAnimation(AnimationBlender.ANIM_FALL, "jump-pose");
    animationBlender:registerAnimation(AnimationBlender.ANIM_RUN, "run-01");
    --animationBlender:registerAnimation(AnimationBlender.ANIM_SNEAK, "Take_damage");
    --animationBlender:registerAnimation(AnimationBlender.ANIM_DUCK, "Land2");
    --animationBlender:registerAnimation(AnimationBlender.ANIM_HALT, "Halt");
    animationBlender:registerAnimation(AnimationBlender.ANIM_ATTACK_1, "attack-01");
    animationBlender:registerAnimation(AnimationBlender.ANIM_ATTACK_2, "attack-02");
    animationBlender:registerAnimation(AnimationBlender.ANIM_TALK_1, "talk-01");
    
    animationBlender:init1(AnimationBlender.ANIM_RUN, true);
    
    if (EventType.QuesterFoundEvent ~= nil) then
        AppStateManager:getScriptEventManager():registerEventListener(EventType.QuesterFoundEvent, Alfred["onQuesterFound"]);
    end
end

Alfred["disconnect"] = function()

end

Alfred["update"] = function(dt)

end

Alfred["onQuesterFound"] = function(eventData)
    local inArea = eventData["inArea"];
    local playerId = eventData["playerId"];
    playerGo = AppStateManager:getGameObjectController():getGameObjectFromId(playerId);
    if (inArea) then
        --log("###onEnemyDead: " .. id);
        aiLuaComponent:changeState(TalkState);
    else
       aiLuaComponent:changeState(WanderState);
    end
end

WanderState = { };

WanderState["enter"] = function(gameObject)
   movingBehavior:setBehavior(BehaviorType.WANDER);
    --movingBehavior:setPathFindData(1, 1, true);
    movingBehavior:setGoalRadius(2);
    movingBehavior:setAutoAnimation(true);
    movingBehavior:setAutoOrientation(true);
    movingBehavior:setTargetAgentId("");
    speechBubbleComp:setActivated(false);
end

WanderState["execute"] = function(gameObject, dt)
    
end

WanderState["exit"] = function(gameObject)
     --movingBehavior:setAutoAnimation(false);
end

TalkState = { };

TalkState["enter"] = function(gameObject)
    movingBehavior:setAutoOrientation(false);
    movingBehavior:setBehavior(BehaviorType.NONE);
    --movingBehavior:setBehavior(BehaviorType.STOP);
    animationBlender:blend5(AnimationBlender.ANIM_TALK_1, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);
    
    speechBubbleComp:setActivated(true);
    mainGo:getAttributesComponentFromName("Quest1"):changeValueBool("BringStoneTable", true);
end

TalkState["execute"] = function(gameObject, dt)
    if (playerGo ~= nil) then
        local resultOrientation = MathHelper:faceTargetOnPlanet(alfred:getSceneNode(), playerGo:getSceneNode(), alfredPhysics:getUp(), alfred:getDefaultDirection());
        alfredPhys:applyOmegaForceRotateTo(resultOrientation, Vector3.UNIT_SCALE, 1);
    end
end

TalkState["exit"] = function(gameObject)
     aiLuaComponent:getMovingBehavior():setAutoAnimation(true);
end