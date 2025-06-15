module("Level1_MainGameObject", package.seeall);
-- physicsActiveComponent = nil;
Level1_MainGameObject = {}

-- Scene: Level1

require("init");

thisGameObject = nil
originAsteroid = nil;
originEnemy1 = nil;
originHealth = nil;
asteroidSpawn = nil;
enemy1Spawn = nil;
healthSpawn = nil;
boss1 = nil;
boss1LeftCanon = nil;
boss1RightCanon = nil;
boss1AiLua = nil;
boss1Shield = nil;
boss1ShieldAttributeEffect = nil;
boss1ShieldEnergy = nil;
energyProgress = nil;
scoreText = nil;
energy = nil;
score = nil;
gameOverText = nil;
timeLine = nil;
fighterJet = nil;
shockwaveSound = nil;

function init()
    fighterJet:getPhysicsComponent():setCollidable(true);
    asteroidSpawn:setActivated(false);
    enemy1Spawn:setActivated(false);
    healthSpawn:setActivated(false);
    --boss1:setVisible(true);
    --boss1:getPhysicsComponent():setCollidable(true);
    --boss1Shield:setVisible(true);
   -- boss1Shield:getPhysicsComponent():setCollidable(true);
    boss1ShieldEnergy:setValueNumber(30);
    boss1AiLua:setActivated(false);
    boss1ShieldAttributeEffect:setActivated(false);

    boss1LeftCanon:getJointHingeActuatorComponent():setActivated(false);
    boss1LeftCanon:getSpawnComponentFromName("LeftLaserSpawn"):setActivated(false);
    boss1LeftCanon:getSpawnComponentFromName("RightLaserSpawn"):setActivated(false);

    boss1RightCanon:getJointHingeActuatorComponent():setActivated(false);
    boss1RightCanon:getSpawnComponentFromName("LeftLaserSpawn"):setActivated(false);
    boss1RightCanon:getSpawnComponentFromName("RightLaserSpawn"):setActivated(false);
    
    gameOverText:setActivated(false);
    
    -- Set default values, and if maybe a game is loaded, the values will be overwritten
    energy:setValueNumber(100);
    score:setValueNumber(0);
    
    energyProgress:setValue(energy:getValueNumber());
    scoreText:setCaption("Score: " .. toString(score:getValueNumber()));
end

function checkGameOver()
    if (energy:getValueNumber() <= 0) then
        gameOverText:setActivated(true);
        fighterJet:getParticleUniverseComponentFromName("ExplosionParticle"):setActivated(true);
        fighterJet:setVisible(false);
        fighterJet:getPhysicsComponent():setCollidable(false);
    end
end

function levelCleared()
    --thisGameObject:getCompositorKeyholeComponent():setActivated(true);
    --local keyholeAttributeEffect = thisGameObject:getAttributeEffectComponent();
    --keyholeAttributeEffect:reactOnEndOfEffect(function()
        --AppStateManager:popAppState();
        AppStateManager:getGameProgressModule():changeScene('Level2');
    --end, false);
    --keyholeAttributeEffect:setActivated(true);
end

Level1_MainGameObject["connect"] = function(gameObject)
    thisGameObject = AppStateManager:getGameObjectController():castGameObject(gameObject);
    asteroidSpawn = thisGameObject:getSpawnComponentFromName("asteroidSpawn");
    asteroidSpawn:setKeepAliveSpawnedGameObjects(true);

    enemy1Spawn = thisGameObject:getSpawnComponentFromName("enemy1Spawn");
    enemy1Spawn:setKeepAliveSpawnedGameObjects(true);
    
    healthSpawn = thisGameObject:getSpawnComponentFromName("healthSpawn");
    healthSpawn:reactOnSpawn(function(spawnedGameObject, originGameObject) 
        spawnedGameObject:setVisible(true);
        spawnedGameObject:getPhysicsActiveKinematicComponent():setPosition(Vector3(math.random(-80, 80), 0, -130));
        spawnedGameObject:getPhysicsActiveKinematicComponent():setVelocity(Vector3(0, 0, 25));
    end);
    
    asteroidSpawn:reactOnSpawn(function(spawnedGameObject, originGameObject) 
        spawnedGameObject:getPhysicsComponent():setCollidable(true);
        spawnedGameObject:setVisible(true);
        spawnedGameObject:getPhysicsActiveComponent():setPosition(Vector3(math.random(-100, 100), 0, -150));
        spawnedGameObject:getPhysicsActiveComponent():setOrientation(Quaternion(Degree(math.random(180)), Vector3.UNIT_Y));
        local direction = Vector3(math.random(-0.5, 0.5), 0, math.random(1));
        spawnedGameObject:getPhysicsActiveComponent():applyRequiredForceForVelocity(direction * 100);
    end);
    
    enemy1Spawn:reactOnSpawn(function(spawnedGameObject, originGameObject) 
        spawnedGameObject:setVisible(true);
        spawnedGameObject:getPhysicsActiveComponent():setPosition(Vector3(math.random(-150, 150), 0, -130));
        spawnedGameObject:getMoveMathFunctionComponent():setActivated(true);
    end);

    fighterJet = AppStateManager:getGameObjectController():getGameObjectFromId("2248180869");
    --local laserSpawn = fighterJet:getSpawnComponentFromName("laserSpawn");
    --laserSpawn:reactOnSpawn(function(spawnedGameObject, originGameObject) 
    --    spawnedGameObject:getPhysicsComponent():setCollidable(true);
    --    local laserBillboard = spawnedGameObject:getBillboardComponent();
    --    laserBillboard:setActivated(true);
    --    local shootSound = spawnedGameObject:getSimpleSoundComponent();
    --    shootSound:setActivated(true);
    --    spawnedGameObject:getPhysicsActiveComponent():applyRequiredForceForVelocity(Vector3(0, 0, -100));
    --end);
    
    energy = fighterJet:getAttributesComponent():getAttributeValueByName("Energy");
    log("--->energy: " .. energy:getValueNumber());
    score = fighterJet:getAttributesComponent():getAttributeValueByName("Score");
    
    -- New
    boss1 = AppStateManager:getGameObjectController():getGameObjectFromId("481476523");
    boss1:getPhysicsComponent():setCollidable(false);
    
    boss1AiLua = boss1:getAiLuaComponent();
    boss1LeftCanon = AppStateManager:getGameObjectController():getGameObjectFromId("2663873632");
    boss1RightCanon = AppStateManager:getGameObjectController():getGameObjectFromId("2673984934");
    boss1LeftCanon:setVisible(false);
    
    boss1RightCanon:setVisible(false);
   
    boss1AiLua = boss1:getAiLuaComponent();
    boss1Shield = AppStateManager:getGameObjectController():getGameObjectFromId("1854220515");
    boss1Shield:getPhysicsComponent():setCollidable(false);

    boss1ShieldAttributeEffect = boss1Shield:getAttributeEffectComponent();
    boss1ShieldAttributeEffect:reactOnEndOfEffect(function()
        boss1ShieldAttributeEffect:setActivated(false);
    end, false);

    boss1ShieldEnergy = boss1Shield:getAttributesComponent():getAttributeValueByName("Energy");
    
    energyProgress = thisGameObject:getMyGUIProgressBarComponent();
    energyProgress:setValue(energy:getValueNumber());
    scoreText = thisGameObject:getMyGUITextComponentFromName("ScoreText");
    scoreText:setCaption("Score: " .. toString(score:getValueNumber()));

    gameOverText = AppStateManager:getGameObjectController():getGameObjectFromId(MAIN_GAMEOBJECT_ID):getMyGUITextComponentFromName("GameOverText");

     -- Load a possible save game
    local success = AppStateManager:getGameProgressModule():loadProgress(Core:getCurrentSaveGameName(), false, false);

    originAsteroid = AppStateManager:getGameObjectController():getGameObjectFromId("2557366868");
    originAsteroid:setVisible(false);
    originAsteroid:getPhysicsActiveComponent():translate(Vector3(0, -5, 0));
    originEnemy1 = AppStateManager:getGameObjectController():getGameObjectFromId("2217483413");
    originEnemy1:setVisible(false);
    originEnemy1:getPhysicsActiveComponent():translate(Vector3(0, -5, 0));
    originHealth = AppStateManager:getGameObjectController():getGameObjectFromId("1274523399");
    originHealth:setVisible(false);
    originHealth:getPhysicsActiveKinematicComponent():translate(Vector3(0, -5, 0));
    
    timeLine = thisGameObject:getTimeLineComponent();
    -- Test setting timeline to last boss
    --timeLine:setCurrentTimeSec(93)
    
    shockwaveSound = thisGameObject:getSimpleSoundComponentFromName("ShockwaveSound");
    
    init()
end

Level1_MainGameObject["disconnect"] = function()
    --init()
    
    --originAsteroid:setVisible(true);
    --originEnemy1:setVisible(true);
    --originHealth:setVisible(true);
    --timeLine:setCurrentTimeSec(0);
    
    AppStateManager:getGameObjectController():undoAll();
end

--Level1_MainGameObject["update"] = function(dt)
--physicsActiveComponent:applyForce(Vector3(0, 20, 0));
--end

Level1_MainGameObject["onAsteriodTimePoint"] = function(timePointSec)
    asteroidSpawn:setActivated(true);
end

Level1_MainGameObject["onEnemy1TimePoint"] = function(timePointSec)
    asteroidSpawn:setActivated(false);
    enemy1Spawn:setActivated(true);
end

Level1_MainGameObject["onHealthTimePoint"] = function(timePointSec)
    enemy1Spawn:setActivated(false);
    healthSpawn:setActivated(true);
end

Level1_MainGameObject["onBoss1TimePoint"] = function(timePointSec)
    boss1:getPhysicsComponent():setCollidable(true);
    boss1Shield:getPhysicsComponent():setCollidable(true);
    boss1Shield:setVisible(true);
    boss1:setVisible(true);
    boss1LeftCanon:setVisible(true);
    boss1LeftCanon:getJointHingeActuatorComponent():setActivated(true);
    boss1RightCanon:setVisible(true);
    boss1RightCanon:getJointHingeActuatorComponent():setActivated(true);
    boss1AiLua:setActivated(true);
end

Level1_MainGameObject["onEnemyLaserContactOnce"] = function(gameObject0, gameObject1, contact)
    contact = AppStateManager:getGameObjectController():castContact(contact);

    local thisLaser = nil;
    local thisEnemy = nil;

    if gameObject0:getCategory() == "Enemy" then
        thisEnemy = gameObject0;
        thisLaser = gameObject1;
    else
        thisEnemy = gameObject1;
        thisLaser = gameObject0;
    end
    
    local eventData = {};
    eventData["laserId"] = thisLaser:getId();
    AppStateManager:getScriptEventManager():queueEvent(EventType.RemoveLaser, eventData);
    
    thisLaser:getPhysicsComponent():setCollidable(false);
    if (thisEnemy:getTagName() == "MotherShipCannon") then
        return;
    end
    
    if (thisEnemy:getTagName() == "Boss") then
        local bossEnergy = thisEnemy:getAttributesComponent():getAttributeValueByName("Energy");
        bossEnergy:setValueNumber(bossEnergy:getValueNumber() - 10);
        local valueBarComponent = thisEnemy:getValueBarComponent();
        valueBarComponent:setCurrentValue(bossEnergy:getValueNumber());
        local energyText = thisEnemy:getGameObjectTitleComponent();
        energyText:setCaption(toString(bossEnergy:getValueNumber()) .. "%");
        
        log("--->energy: " .. bossEnergy:getValueNumber());
        
        if (bossEnergy:getValueNumber() <= 0) then
            local bossLeftCanon = AppStateManager:getGameObjectController():getGameObjectFromId("2663873632");
            bossLeftCanon:setVisible(false);
            bossLeftCanon:getSpawnComponent():setActivated(false);
            local bossRightCanon = AppStateManager:getGameObjectController():getGameObjectFromId("2673984934");
            bossRightCanon:setVisible(false);
            bossRightCanon:getSpawnComponent():setActivated(false);
            thisEnemy:setVisible(false);
            valueBarComponent:setActivated(false);
            thisEnemy:getPhysicsActiveComponent():translate(Vector3(0, -50, 0));
            shockwaveSound:setActivated(true);
            thisEnemy:getParticleUniverseComponentFromName("ExplosionParticle"):setActivated(true);
            thisEnemy:getPhysicsActiveComponent():setActivated(false);
            --thisEnemy:disconnect();
            levelCleared();
        end
    else
        thisEnemy:getPhysicsComponent():setCollidable(false);
        thisEnemy:setVisible(false);
        thisEnemy:getPhysicsActiveComponent():translate(Vector3(0, -5, 0));
        shockwaveSound:setActivated(true);
        thisEnemy:getParticleUniverseComponentFromName("ExplosionParticle"):setActivated(true);
        thisEnemy:getPhysicsActiveComponent():setActivated(false);
        AppStateManager:getGameObjectController():deleteDelayedGameObject(thisEnemy:getId(), 3);
    end

    score:setValueNumber(score:getValueNumber() + 20);
    scoreText:setCaption("Score: " .. toString(score:getValueNumber()));

end

Level1_MainGameObject["onPlayerEnemyContactOnce"] = function(gameObject0, gameObject1, contact)
    
    local thisPlayer = nil;
    local thisEnemy = nil;
    
    -- Decrease energy
    if gameObject0:getCategory() == "Player" then
        thisPlayer = gameObject0;
        thisEnemy = gameObject1;
    else
        thisPlayer = gameObject1;
        thisEnemy = gameObject0;
    end
    
    if (thisEnemy:getTagName() == "Stone") then
        energy:setValueNumber(energy:getValueNumber() - 2);
    elseif (thisEnemy:getTagName() == "Ship1") then
        energy:setValueNumber(energy:getValueNumber() - 5);
    end

    energyProgress:setValue(energy:getValueNumber());
    
    if (energy:getValueNumber() < 50 and energy:getValueNumber() > 20) then
        thisPlayer:getParticleUniverseComponentFromName("SmokeParticle"):setActivated(true);
    else
        thisPlayer:getParticleUniverseComponentFromName("SmokeParticle"):setPlaySpeed(20);
    end

    if (energy:getValueNumber() <= 0) then
        gameOverText:setActivated(true);
        thisPlayer:getParticleUniverseComponentFromName("ExplosionParticle"):setActivated(true);
        thisPlayer:setVisible(false);
    end
    
    --log("----->Dump: " .. dump(felsites));

    if (thisEnemy:getTagName() ~= "Boss") then
        thisEnemy:getPhysicsActiveComponent():setCollidable(false);
        thisEnemy:setVisible(false);
        thisEnemy:getPhysicsActiveComponent():translate(Vector3(0, -5, 0));
        --thisEnemy:setScale(Vector3(0.1, 0.1, 0.1));
        shockwaveSound:setActivated(true);
        thisEnemy:getParticleUniverseComponentFromName("ExplosionParticle"):setActivated(true);
        thisEnemy:getPhysicsActiveComponent():setActivated(false);
        -- Do not delete directly, because a particle effect needs time to play
        AppStateManager:getGameObjectController():deleteDelayedGameObject(thisEnemy:getId(), 3);
    end
end

Level1_MainGameObject["onPlayerEnemyLaserContactOnce"] = function(gameObject0, gameObject1, contact)
    --log("---->onPlayerEnemyLaserContactOnce ");
    
    local thisPlayer = nil;
    local thisEnemyLaser = nil;
    
    -- Decrease energy
    if gameObject0:getCategory() == "Player" then
        thisPlayer = gameObject0;
        thisEnemyLaser = gameObject1;
    else
        thisPlayer = gameObject1;
        thisEnemyLaser = gameObject0;
    end
    
    thisEnemyLaser:getPhysicsComponent():setCollidable(false);
    thisEnemyLaser:getParticleUniverseComponentFromName("FireParticle"):setActivated(true);
    thisEnemyLaser:setVisible(false);
    thisEnemyLaser:getPhysicsActiveKinematicComponent():translate(Vector3(0, -5, 0));
    
    if (thisEnemyLaser:getTagName() == "LaserShip1") then
        energy:setValueNumber(energy:getValueNumber() - 10);
    end
    
    if (energy:getValueNumber() < 50 and energy:getValueNumber() > 20) then
        thisPlayer:getParticleUniverseComponentFromName("SmokeParticle"):setActivated(true);
    else
        thisPlayer:getParticleUniverseComponentFromName("SmokeParticle"):setPlaySpeed(20);
    end

    energyProgress:setValue(energy:getValueNumber());
    
    AppStateManager:getGameObjectController():deleteDelayedGameObject(thisEnemyLaser:getId(), 3);
    
    checkGameOver();
end

Level1_MainGameObject["onPlayerHealthContactOnce"] = function(gameObject0, gameObject1, contact)
    local thisPlayer = nil;
    local thisHealth = nil;
    
    -- Increase energy
    if gameObject0:getCategory() == "Player" then
        thisPlayer = gameObject0;
        thisHealth = gameObject1;
    else
        thisPlayer = gameObject1;
        thisHealth = gameObject0;
    end
    
    thisHealth:getPhysicsActiveKinematicComponent():translate(Vector3(0, -5, 0));
    thisHealth:getSimpleSoundComponent():setActivated(true);
    thisHealth:setVisible(false);
    
    if (thisHealth:getTagName() == "Health") then
        energy:setValueNumber(energy:getValueNumber() + 40);
    end
    
    if (energy:getValueNumber() > 100) then
        energy:setValueNumber(100);
    end

    energyProgress:setValue(energy:getValueNumber());
    
    AppStateManager:getGameObjectController():deleteDelayedGameObject(thisHealth:getId(), 2);
end

Level1_MainGameObject["onLaserShieldContactOnce"] = function(gameObject0, gameObject1, contact)
    local thisLaser = nil;
    local thisShield = nil;
   
    if gameObject0:getCategory() == "Laser" then
        thisLaser = gameObject0;
        thisShield = gameObject1;
    else
        thisLaser = gameObject1;
        thisShield = gameObject0;
    end
    
    local eventData = {};
    eventData["laserId"] = thisLaser:getId();
    AppStateManager:getScriptEventManager():queueEvent(EventType.RemoveLaser, eventData);
    
    thisLaser:getPhysicsComponent():setCollidable(false);
    
    boss1ShieldAttributeEffect:setActivated(true);
    boss1ShieldEnergy:setValueNumber(boss1ShieldEnergy:getValueNumber() - 10);
    if (boss1ShieldEnergy:getValueNumber() <= 0) then
        log("-->no shield anymore");
        thisShield:getPhysicsComponent():setCollidable(false);
        thisShield:setVisible(false);
        --AppStateManager:getGameObjectController():deleteGameObjectWithUndo(thisEnemy:getId());
    end
end

Level1_MainGameObject["onPlayerShieldContactOnce"] = function(gameObject0, gameObject1, contact)
    local thisPlayer = nil;
    local thisShield = nil;
    
    -- Decrease energy
    if gameObject0:getCategory() == "Player" then
        thisPlayer = gameObject0;
        thisShield = gameObject1;
    else
        thisPlayer = gameObject1;
        thisShield = gameObject0;
    end
    
    energy:setValueNumber(energy:getValueNumber() - 10000);
    
    checkGameOver();
end