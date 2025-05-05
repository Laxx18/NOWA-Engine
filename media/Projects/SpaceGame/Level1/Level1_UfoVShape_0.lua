module("Level1_UfoVShape_0", package.seeall);
Level1_UfoVShape_0 = {}

-- Scene: Level1

require("init");

originLaser = nil;
laserSpawnComponent = nil;
--laserSoundComponent = nil;

Level1_UfoVShape_0["connect"] = function(gameObject)
    gameObject = AppStateManager:getGameObjectController():castGameObject(gameObject);
    originLaser = AppStateManager:getGameObjectController():getGameObjectFromId("791472684");
    originLaser:setVisible(false);
    originLaser:getPhysicsActiveKinematicComponent():translate(Vector3(0, -5, 0));
    laserSpawnComponent = gameObject:getSpawnComponent();
    
    local mainGameObject = AppStateManager:getGameObjectController():getGameObjectFromId(MAIN_GAMEOBJECT_ID);
    --laserSoundComponent = mainGameObject:getSimpleSoundComponentFromName("LaserSound");
    
    laserSpawnComponent:reactOnSpawn(function(spawnedGameObject, originGameObject) 
        spawnedGameObject:setVisible(true);

        --laserSoundComponent:setActivated(true);
        
        local direction = originGameObject:getOrientation() * originGameObject:getDefaultDirection();
        -- clamp to y 0 the flight direction, so that the laser can hit the player
        direction = direction * Vector3(1, 0, 1);
        spawnedGameObject:getPhysicsActiveKinematicComponent():setVelocity(direction * spawnedGameObject:getPhysicsActiveKinematicComponent():getSpeed());
        spawnedGameObject:getPhysicsActiveKinematicComponent():setOmegaVelocity(direction);
        --spawnedGameObject:getPhysicsActiveComponent():applyForce(direction * spawnedGameObject:getPhysicsActiveComponent():getSpeed());
        --spawnedGameObject:getPhysicsActiveComponent():applyOmegaForceRotateTo(originGameObject:getOrientation(), Vector3.UNIT_Y, 1);
        --log("onLaserShip1Spawned: " .. toString(spawnedGameObject:getPosition()));
    end);
end

Level1_UfoVShape_0["cloned"] = function(gameObject)
    -- Activate spawning, only just when the ship1 has been cloned
    laserSpawnComponent = gameObject:getSpawnComponent();
    laserSpawnComponent:setActivated(true);
    laserSpawnComponent:setKeepAliveSpawnedGameObjects(true);
end

Level1_UfoVShape_0["disconnect"] = function()
    originLaser:getPhysicsActiveKinematicComponent():translate(Vector3(0, 5, 0));
    originLaser:setVisible(true);
    --laserSpawnComponent:setActivated(false);
end