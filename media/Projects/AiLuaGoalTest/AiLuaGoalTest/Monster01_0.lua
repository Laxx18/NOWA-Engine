module("Monster01_0", package.seeall);

aiLuaGoalComponent = nil;
objectTitleComponent = nil;
victims = { };

Monster01_0 = {}

monster = nil;

compositeGoal = nil;

Monster01_0["connect"] = function(gameObject)
    monster = AppStateManager:getGameObjectController():castGameObject(gameObject);
    aiLuaGoalComponent = monster:getAiLuaGoalComponent();
    objectTitleComponent = monster:getGameObjectTitleComponent();
     -- Difference between NONE and STOP. 
    -- If none is on, add no force, so that other behaviors can still move the agent! Only stop adds force, even if its null
    aiLuaGoalComponent:getMovingBehavior():setBehavior(MovingBehavior.STOP);
    aiLuaGoalComponent:getMovingBehavior():setGoalRadius(1);
	
	-- Create composite goal
	compositeGoal = LuaGoalComposite(gameObject, PatrolCompositeGoal)
	
	aiLuaGoalComponent:setRootGoal(compositeGoal);
	
end

Monster01_0["disconnect"] = function()
    
end

---------------------------------------------------------------------------------------------

MoveToGoal = { }

MoveToGoal["activate"] = function(gameObject, goalResult)
    goalResult = AppStateManager:getGameObjectController():castGoalResult(goalResult);
    log("Activating MoveToGoal for " .. tostring(monster))
    -- monster:getMovingBehavior():moveTo(targetPosition)
end

MoveToGoal["process"] = function(gameObject, dt)
    log("Processing MoveToGoal for " .. tostring(gameObject))
    --if monster:getMovingBehavior():isAtTarget() then
    --    return GoalResult.COMPLETED
    --end
    return GoalResult.ACTIVE
end

MoveToGoal["terminate"] = function(gameObject, goalResult)
    goalResult = AppStateManager:getGameObjectController():castGoalResult(goalResult);
    log("Terminating MoveToGoal for " .. tostring(monster))
    --monster:getMovingBehavior():stop()
end

---------------------------------------------------------------------------------------------

PatrolCompositeGoal = {}

PatrolCompositeGoal["activate"] = function(gameObject, goalResult)
    goalResult = AppStateManager:getGameObjectController():castGoalResult(goalResult);
    log("Activating PatrolCompositeGoal for " .. tostring(monster))

    -- Create subgoals
    local moveToPointA = LuaGoal(monster, MoveToGoal)
    moveToPointA:activate()

    local moveToPointB = LuaGoal(monster, MoveToGoal)
    moveToPointB:activate()

    -- Add subgoals to the composite goal
    compositeGoal:addSubGoal(moveToPointA)
    compositeGoal:addSubGoal(moveToPointB)
end

-- Necessary?
--LuaGoalComposite["process"] = function(gameObject, dt)
--    log("Processing LuaGoalComposite for " .. tostring(monster))
--    return compositeGoal:processSubGoals(dt)
--end

PatrolCompositeGoal["terminate"] = function(gameObject, goalResult)
    goalResult = AppStateManager:getGameObjectController():castGoalResult(goalResult);
    log("Terminating PatrolCompositeGoal for " .. tostring(monster))
    compositeGoal:removeAllSubGoals()
end