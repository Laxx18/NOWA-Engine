module("SpaceGun2_2", package.seeall);
-- physicsActiveComponent = nil;
SpaceGun2_2 = {}

-- Scene: Scene4

require("init");

scene4_SpaceGun2_2 = nil
originLaser = nil;
leftLaserSpawnComponent = nil;
rightLaserSpawnComponent = nil;
thisGameObject = nil;

speed = 100

SpaceGun2_2["connect"] = function(gameObject)
    thisGameObject = gameObject;
    originLaser = AppStateManager:getGameObjectController():getGameObjectFromId("791472684");
    originLaser:setVisible(false);
    leftLaserSpawnComponent = thisGameObject:getSpawnComponentFromName("LeftLaserSpawn");
    leftLaserSpawnComponent:setKeepAliveSpawnedGameObjects(true);
    leftLaserSpawnComponent:reactOnSpawn(function(spawnedLaserGameObject, originGameObject) 
        spawnedLaserGameObject:setVisible(true);
        --Note: Laser gun has different height as the player, hence move the laser in  y = 0 in order to be able to hit the player
        local directionCanon = originGameObject:getOrientation() * originGameObject:getDefaultDirection();
        -- Laser is kinematic component!
        spawnedLaserGameObject:getPhysicsActiveComponent():setVelocity(Vector3(directionCanon.x, 0, directionCanon.z) * speed);
        --spawnedLaserGameObject:getPhysicsActiveComponent():applyRequiredForceForVelocity(Vector3(direction.x, yComponent, direction.z) * speed);
    end);
    
    rightLaserSpawnComponent = thisGameObject:getSpawnComponentFromName("RightLaserSpawn");
    rightLaserSpawnComponent:setKeepAliveSpawnedGameObjects(true);
    rightLaserSpawnComponent:reactOnSpawn(function(spawnedLaserGameObject, originGameObject) 
        spawnedLaserGameObject:setVisible(true);
        --Note: Laser gun has different height as the player, hence move the laser in  y = 0 in order to be able to hit the player
        local directionCanon = originGameObject:getOrientation() * originGameObject:getDefaultDirection();
        -- Laser is kinematic component!
        spawnedLaserGameObject:getPhysicsActiveComponent():setVelocity(Vector3(directionCanon.x, 0, directionCanon.z) * speed);
        --spawnedLaserGameObject:getPhysicsActiveComponent():applyRequiredForceForVelocity(Vector3(direction.x, yComponent, direction.z) * speed);
    end);
end

SpaceGun2_2["disconnect"] = function()
    --originLaser:setVisible(true);
    --leftLaserSpawnComponent:setActivated(false);
    --rightLaserSpawnComponent:setActivated(false);
    --thisGameObject:getJointHingeActuatorComponent():setActivated(false);
    --thisGameObject:setVisible(true);
end