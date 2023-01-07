
-------------------------------------------------------------------------------

-- Create the MoveRandomly state

-------------------------------------------------------------------------------

--local namespace = {}
--setmetatable(namespace, {__index = _G})
--VictimScript = namespace;
--setfenv(1, namespace);

MoveRandomlyState = { }

MoveRandomlyState["enter"] = function(gameObject)
    log("Victim Enter MoveRandomlyState: " .. gameObject:getName());
    aiComponent = gameObject:getAiLuaComponent();
    aiComponent:setBehavior("Wander");
    aiComponent:getMovingBehavior():setObstacleAvoidanceData("Wall", 5);
    aiComponent:addBehavior("ObstacleAvoidance");

    enemy = GameObjectController:getGameObjectFromName("heavy_infantry#0");
    aiComponent:changeTargetGameObject(enemy:getName());

    gameObject:getPhysicsActiveComponent():setSpeed(3);

    titleComponent = gameObject:getGameObjectTitleComponent();
end

MoveRandomlyState["execute"] = function(gameObject, dt)
    -- log("[Lua]: Execute move randomly: " .. gameObject:getName());
    titleComponent:setCaption(gameObject:getName() .. ": " ..aiComponent:getMovingBehavior():getCurrentBehavior());

    if gameObject:getPosition():squaredDistance(enemy:getPosition()) <= 15 * 15 then
        aiComponent:getStateMaschine():changeState(FleeState);
    end
    -- log("Wander " .. gameObject:getName() .. " pos: " .. toString(gameObject:getPosition()) .. " ori: " .. toString(gameObject:getOrientation()));
end

MoveRandomlyState["exit"] = function(gameObject)
    -- log("[Lua]: Exit move randomly");
end

-------------------------------------------------------------------------------

-- Create the Flee state

-------------------------------------------------------------------------------

FleeState = { }

FleeState["enter"] = function(gameObject)
    log("Victim Enter FleeState: " .. gameObject:getName());
    enemy = GameObjectController:getGameObjectFromName("heavy_infantry#0");
    aiComponent = gameObject:getAiLuaComponent();
    aiComponent:changeTargetGameObject(enemy:getName());
    aiComponent:setBehavior("Evade");
    aiComponent:getMovingBehavior():setObstacleAvoidanceData("Wall", 5);
    aiComponent:addBehavior("ObstacleAvoidance");
    gameObject:getPhysicsActiveComponent():setSpeed(3.9);
end

FleeState["execute"] = function(gameObject, dt)

    titleComponent:setCaption(gameObject:getName() .. ": " ..aiComponent:getMovingBehavior():getCurrentBehavior());
    if gameObject:getPosition():squaredDistance(enemy:getPosition()) > 16 * 16 then
        aiComponent:getStateMaschine():changeState(MoveRandomlyState);
    end
    --log("Evade " .. gameObject:getName() .. " pos: " .. toString(gameObject:getPosition()) .. " ori: " .. toString(gameObject:getOrientation()));
end

FleeState["exit"] = function(gameObject)

end