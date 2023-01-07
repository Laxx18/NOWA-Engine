
-------------------------------------------------------------------------------

-- Create the Patrol state
-- https://wiki.jc-mp.com/Lua/Tutorials/Class_basics

-------------------------------------------------------------------------------

--local namespace = {}
--setmetatable(namespace, {__index = _G})
--EnemyScript = namespace;
--setfenv(1, namespace);

PatrolState = { }

PatrolState["enter"] = function(gameObject)
    log("Enemy: Enter wander: " .. gameObject:getName());
    aiComponent = gameObject:getAiLuaComponent();
    victim = GameObjectController:getGameObjectFromName("smithy#0");
    aiComponent:changeTargetGameObject(victim:getName());
    aiComponent:setBehavior("Wander");
    aiComponent:getMovingBehavior():setObstacleAvoidanceData("Wall", 5);
    aiComponent:addBehavior("ObstacleAvoidance");

    titleComponent = gameObject:getGameObjectTitleComponent();
end

PatrolState["execute"] = function(gameObject, dt)
    titleComponent:setCaption(gameObject:getName() .. ": " ..aiComponent:getMovingBehavior():getCurrentBehavior());

    local distance = gameObject:getPosition():squaredDistance(victim:getPosition());
    if distance <= 15 * 15 then
        aiComponent:getStateMaschine():changeState(PursuitState);
    end
    --log("Wander " .. gameObject:getName() .. " pos: " .. toString(gameObject:getPosition()) .. " ori: " .. toString(gameObject:getOrientation()));
end

PatrolState["exit"] = function(gameObject)

end

-------------------------------------------------------------------------------

-- Create the Pursuit state

-------------------------------------------------------------------------------

PursuitState = { }

PursuitState["enter"] = function(gameObject)
    log("Enemy: Enter PursuitState: " .. gameObject:getName());
    aiComponent = gameObject:getAiLuaComponent();
    victim = GameObjectController:getGameObjectFromName("smithy#0");
    aiComponent:changeTargetGameObject(victim:getName());
    aiComponent:setBehavior("Pursuit");
    aiComponent:getMovingBehavior():setObstacleAvoidanceData("Wall", 5);
    aiComponent:addBehavior("ObstacleAvoidance");
end

PursuitState["execute"] = function(gameObject, dt)
    titleComponent:setCaption(gameObject:getName() .. ": " ..aiComponent:getMovingBehavior():getCurrentBehavior());

    if gameObject:getPosition():squaredDistance(victim:getPosition()) > 15 * 15 then
        aiComponent:getStateMaschine():changeState(PatrolState);
    end
    --log("Seek " .. gameObject:getName() .. " pos: " .. toString(gameObject:getPosition()) .. " ori: " .. toString(gameObject:getOrientation()));
end

PursuitState["exit"] = function(gameObject)
    --log("Exit wander: " .. gameObject:getName());
end