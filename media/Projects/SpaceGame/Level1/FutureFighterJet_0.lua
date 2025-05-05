module("FutureFighterJet_0", package.seeall);

require("init");

attributesComponent = nil;
physicsActiveComponent = nil;
cameraComponent = nil;
cameraGameObject = nil;
flySound = nil;
timeSinceLastLaserShoot = 0.2;
canShoot = false;
laserSpawnComponent = nil;
originLaser = nil;
thisGameObject = nil;
inputDeviceModule = nil;

FutureFighterJet_0 = {};

FutureFighterJet_0["connect"] = function(gameObject)
    thisGameObject = AppStateManager:getGameObjectController():castGameObject(gameObject);
    
    PointerManager:showMouse(false);

    inputDeviceModule = thisGameObject:getInputDeviceComponent():getInputDeviceModule();
    physicsActiveComponent = thisGameObject:getPhysicsActiveComponent();
    attributesComponent = thisGameObject:getAttributesComponent();
    laserSpawnComponent = thisGameObject:getSpawnComponent();
    laserSpawnComponent:setActivated(false);
    laserSpawnComponent:setKeepAliveSpawnedGameObjects(true);
    originLaser = AppStateManager:getGameObjectController():getGameObjectFromId("1543222174");
    originLaser:setVisible(false);

    timeSinceLastLaserShoot = 0.2;

    flySound = thisGameObject:getSimpleSoundComponent();
    flySound:setVolume(80);
    flySound:setActivated(true);

    cameraGameObject = AppStateManager:getGameObjectController():getGameObjectFromName("GameCamera");
    cameraComponent = cameraGameObject:getCameraComponent();
    cameraComponent:setCameraPosition(Vector3(0, 80, -35));
    cameraComponent:setCameraDegreeOrientation(Vector3(-90, 180, 180));
    cameraComponent:setActivated(true);
    
    thisGameObject:getParticleUniverseComponentFromName("ExplosionParticle"):setActivated(false);
    thisGameObject:getParticleUniverseComponentFromName("SmokeParticle"):setActivated(false);
    laserSpawnComponent:reactOnSpawn(function(spawnedGameObject, originGameObject)
            spawnedGameObject:getPhysicsComponent():setCollidable(true);
            local laserBillboard = spawnedGameObject:getBillboardComponent();
            laserBillboard:setActivated(true);
            local shootSound = spawnedGameObject:getSimpleSoundComponent();
            shootSound:setActivated(true);
            spawnedGameObject:getPhysicsActiveComponent():applyRequiredForceForVelocity(Vector3(0, 0, -100));
        end);

    if (EventType.RemoveLaser ~= nil) then
        AppStateManager:getScriptEventManager():registerEventListener(EventType.RemoveLaser, FutureFighterJet_0["onRemoveLaser"]);
    end
end

FutureFighterJet_0["disconnect"] = function()
    PointerManager:showMouse(true);

    flySound:setActivated(false);
    --cameraComponent:setCameraPosition(Vector3(0, 160, -35));
    laserSpawnComponent:setActivated(false);
    originLaser:setVisible(true);
    
    thisGameObject:getParticleUniverseComponentFromName("SmokeParticle"):setActivated(false);
    thisGameObject:getParticleUniverseComponentFromName("SmokeParticle"):setPlaySpeed(10);
    
    thisGameObject:getParticleUniverseComponentFromName("ExplosionParticle"):setActivated(false);
    thisGameObject:setVisible(true);

    cameraComponent:setActivated(false);
    AppStateManager:getGameObjectController():getGameObjectFromName("MainCamera"):getCameraComponent():setActivated(true);
end

FutureFighterJet_0["update"] = function(dt)

    local moveHorizontal = 0;
    local moveVertical = 0;
    local speed = 20;

    if inputDeviceModule:isActionDown(NOWA_A_UP) then
        moveVertical = -1;
    elseif inputDeviceModule:isActionDown(NOWA_A_DOWN) then
        moveVertical = 1;
    end
    if inputDeviceModule:isActionDown(NOWA_A_LEFT) then
        moveHorizontal = -1;
    elseif inputDeviceModule:isActionDown(NOWA_A_RIGHT) then
        moveHorizontal = 1;
    end

    if inputDeviceModule:isActionDown(NOWA_A_START) then
        AppStateManager:pushAppState("MenuState");
    end

    flySound:setPitch(0.2 + physicsActiveComponent:getForce():length() * 0.01);
    
    physicsActiveComponent:setBounds(Vector3(boundsLeft, 0.0, boundsTop), Vector3(boundsRight, 0.0, boundsBottom));

    local movement = Vector3(moveHorizontal, 0, moveVertical);

    physicsActiveComponent:applyRequiredForceForVelocity(movement * speed); -- jerky movement
    
    --physicsActiveComponent:setVelocity(movement * speed);
    --physicsActiveComponent:addImpulse(movement * speed); -- does work correctly

    --physicsActiveComponent:applyOmegaForceRotateTo(Quaternion(Degree(180), Vector3.UNIT_Y), Vector3.UNIT_Y, 100);
    --physicsActiveComponent:setOrientation(Quaternion(Degree(180), Vector3.UNIT_Y));
    -- Add some decline
    --local force = physicsActiveComponent:getForce();
    --if (force == Vector3.ZERO) then
    --    physicsActiveComponent:applyOmegaForceRotateTo(Quaternion(Degree(0), Vector3.UNIT_X), Vector3.UNIT_X, 100);
    --else
    --    physicsActiveComponent:applyOmegaForce(Vector3(0, 0, -force * 0.0001));
    --end

    shoot(dt);
    
end

function shoot(dt)
    if timeSinceLastLaserShoot >= 0 then
        timeSinceLastLaserShoot = timeSinceLastLaserShoot - dt;
    end

    if timeSinceLastLaserShoot <= 0 then
        if inputDeviceModule:isActionDown(NOWA_A_ATTACK_1) == false then
            canShoot = true;
        end
        
        if inputDeviceModule:isActionDown(NOWA_A_ATTACK_1) and canShoot == true then
            laserSpawnComponent:setActivated(true);
            
            canShoot = false;
            timeSinceLastLaserShoot = 0.2;
        end
    end
    
    --log("-->can shoot: " .. (canShoot and 'true' or 'false'));
end

FutureFighterJet_0["onRemoveLaser"] = function(eventData)
    local id = eventData["laserId"];
    --log("###onRemoveLaser: " .. id);
    AppStateManager:getGameObjectController():deleteGameObject(id);
end