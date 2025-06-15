module("Level1_MotherShip2_0", package.seeall);

boss1 = nil
aiLuaComponent = nil;
boss1LeftCanon = nil;
boss1RightCanon = nil;
moveMathFunctionComponent = nil;
energy = nil;

Level1_MotherShip2_0 = {}

Level1_MotherShip2_0["connect"] = function(gameObject)
    boss1 = AppStateManager:getGameObjectController():castGameObject(gameObject);
    aiLuaComponent = boss1:getAiLuaComponent();
    moveMathFunctionComponent = boss1:getMoveMathFunctionComponent();
    energy = boss1:getAttributesComponent():getAttributeValueByName("Energy");
    energy:setValueNumber(100);
    boss1:getValueBarComponent():setCurrentValue(100);
    boss1:getGameObjectTitleComponent():setCaption("100%");
    boss1LeftCanon = AppStateManager:getGameObjectController():getGameObjectFromId("2663873632");
    boss1RightCanon = AppStateManager:getGameObjectController():getGameObjectFromId("2673984934");
    -- Difference between NONE and STOP. 
    -- If none is on, add no force, so that other behaviors can still move the agent! Only stop adds force, even if its null
    aiLuaComponent:getMovingBehavior():setBehavior(BehaviorType.STOP);
    aiLuaComponent:getMovingBehavior():setGoalRadius(2);
    aiLuaComponent:getMovingBehavior():setStuckCheckTime(1);
    aiLuaComponent:getMovingBehavior():setAutoOrientation(false);
end

Level1_MotherShip2_0["disconnect"] = function()
    --boss1:setVisible(true);
    --boss1OldPosition = boss1:getPhysicsActiveComponent():getPosition();
    --boss1OldOrientation = boss1:getPhysicsActiveComponent():getOrientation();
    --aiLuaComponent:setActivated(false);
    --moveMathFunctionComponent:setActivated(false);
    --boss1:getValueBarComponent():setCurrentValue(100);
    --energy:setValueNumber(100);
    --boss1:getGameObjectTitleComponent():setCaption("100%");
    --boss1:getSimpleSoundComponent():setActivated(false);
    --boss1:getParticleUniverseComponent():setActivated(false);
    --boss1:getGameObjectTitleComponent():setActivated(false);
end

Level1_MotherShip2_0["update"] = function(dt)
    --log("--> behavior: " .. aiLuaComponent:getMovingBehavior():getCurrentBehavior());
    --log("[Lua]: collidable: " .. (boss1:getPhysicsActiveComponent():getCollidable() and 'true' or 'false'));
end

AppearState = {}

AppearState["enter"] = function(gameObject)
    log("Enemy Enter AppearState: " .. gameObject:getName());
    -- aiLuaComponent:getMovingBehavior():getPath():addWayPoint(boss1:getPosition() + Vector3(0, 0, 70));
    aiLuaComponent:getMovingBehavior():getPath():addWayPoint(Vector3(-30, 0, -70));
    aiLuaComponent:getMovingBehavior():setBehavior(BehaviorType.FOLLOW_PATH);
    
    aiLuaComponent:reactOnPathGoalReached(function() 
            log("[Lua]: AppearEndReached");
            aiLuaComponent:changeState(MoveState1);
        end);
end



AppearState["exit"] = function(gameObject)
    --aiLuaComponent:getMovingBehavior():setBehavior(BehaviorType.NONE);
end

-----------------------------------------------------------------------------------------------------

MoveState1 = {}

MoveState1["enter"] = function(gameObject)
    log("Enemy Enter MoveState1: " .. gameObject:getName());
    aiLuaComponent:setGlobalState(ShootState);
    
    aiLuaComponent:getMovingBehavior():getPath():addWayPoint(Vector3(boundsLeft + 10, 0, -70));
    aiLuaComponent:getMovingBehavior():getPath():addWayPoint(Vector3(boundsRight - 10, 0, -70));
    aiLuaComponent:getMovingBehavior():getPath():addWayPoint(Vector3(0, 0, -70));
    aiLuaComponent:getMovingBehavior():setBehavior(BehaviorType.FOLLOW_PATH);
    
    aiLuaComponent:reactOnAgentStuck(function() 
            log("[Lua]: AgentStuck");
            aiLuaComponent:getMovingBehavior():setBehavior(MovingBehavior.STOP);
        end);
    
    aiLuaComponent:reactOnPathGoalReached(function() 
            log("[Lua]: PathGoalReached");
            aiLuaComponent:changeState(MoveState2);
        end);
end

MoveState1["exit"] = function(gameObject)
    aiLuaComponent:getMovingBehavior():setBehavior(MovingBehavior.NONE);
end

-----------------------------------------------------------------------------------------------------

MoveState2 = {}

MoveState2["enter"] = function(gameObject)
    --log("Enemy Enter MoveState2: " .. gameObject:getName());
    moveMathFunctionComponent:setActivated(true);
    moveMathFunctionComponent:reactOnFunctionFinished(function(hitGameObject)
        log("[Lua]: CircleFunctionFinished");
        aiLuaComponent:changeState(MoveState3);
    end);
end

MoveState2["exit"] = function(gameObject)
    moveMathFunctionComponent:setActivated(false);
end

-----------------------------------------------------------------------------------------------------

MoveState3 = {}

MoveState3["enter"] = function(gameObject)
    --log("Enemy Enter MoveState3: " .. gameObject:getName());
    boss1:getPhysicsActiveComponent():setSpeed(30);
    local player = AppStateManager:getGameObjectController():getGameObjectFromId("2248180869");
    aiLuaComponent:getMovingBehavior():getPath():addWayPoint(player:getPosition());
    aiLuaComponent:getMovingBehavior():setBehavior(BehaviorType.FOLLOW_PATH);
    aiLuaComponent:reactOnPathGoalReached(function(hitGameObject)
        log("[Lua]: PathGoalReached");
        aiLuaComponent:getMovingBehavior():setBehavior(MovingBehavior.STOP);
        aiLuaComponent:changeState(MoveState1);
    end);
end

MoveState3["exit"] = function(gameObject)
   boss1:getPhysicsActiveComponent():setSpeed(15);
end

-----------------------------------------------------------------------------------------------------

ShootState = {}

ShootState["enter"] = function(gameObject)
    --log("Enemy Enter ShootState: " .. gameObject:getName());
    boss1LeftCanon:getSpawnComponentFromName("LeftLaserSpawn"):setActivated(true);
    boss1LeftCanon:getSpawnComponentFromName("RightLaserSpawn"):setActivated(true);
    
    boss1RightCanon:getSpawnComponentFromName("LeftLaserSpawn"):setActivated(true);
    boss1RightCanon:getSpawnComponentFromName("RightLaserSpawn"):setActivated(true);
end

ShootState["exit"] = function(gameObject)
    boss1LeftCanon:getSpawnComponentFromName("LeftLaserSpawn"):setActivated(false);
    boss1LeftCanon:getSpawnComponentFromName("RightLaserSpawn"):setActivated(false);
    
    boss1RightCanon:getSpawnComponentFromName("LeftLaserSpawn"):setActivated(false);
    boss1RightCanon:getSpawnComponentFromName("RightLaserSpawn"):setActivated(false);
end

