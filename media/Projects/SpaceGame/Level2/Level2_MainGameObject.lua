module("Level2_MainGameObject", package.seeall);
-- physicsActiveComponent = nil;
level2_MainGameObject = {}

-- Scene: Level1

require("init");

thisGameObject = nil
originAsteroid = nil;
asteroidSpawn = nil;
energyProgress = nil;
scoreText = nil;
energy = nil;
score = nil;
gameOverText = nil;
timeLine = nil;
fighterJet = nil;

function init()
    fighterJet:getPhysicsComponent():setCollidable(true);
    asteroidSpawn:setActivated(false);
    
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
    thisGameObject:getCompositorKeyholeComponent():setActivated(true);
    local keyholeAttributeEffect = thisGameObject:getAttributeEffectComponent();
    keyholeAttributeEffect:reactOnEndOfEffect(function()
        AppStateManager:popAppState();
    end, false);
    keyholeAttributeEffect:setActivated(true);
end

level2_MainGameObject["connect"] = function(gameObject)
    thisGameObject = AppStateManager:getGameObjectController():castGameObject(gameObject);
    asteroidSpawn = thisGameObject:getSpawnComponentFromName("asteroidSpawn");
    asteroidSpawn:setKeepAliveSpawnedGameObjects(true);
    asteroidSpawn:reactOnSpawn(function(spawnedGameObject, originGameObject) 
        spawnedGameObject:getPhysicsComponent():setCollidable(true);
        spawnedGameObject:getPhysicsActiveComponent():setPosition(Vector3(math.random(-100, 100), 0, -150));
        spawnedGameObject:getPhysicsActiveComponent():setOrientation(Quaternion(Degree(math.random(180)), Vector3.UNIT_Y));
        local direction = Vector3(math.random(-0.5, 0.5), 0, math.random(1));
        spawnedGameObject:getPhysicsActiveComponent():applyRequiredForceForVelocity(direction * 100);
    end);
    
    healthSpawn = thisGameObject:getSpawnComponentFromName("healthSpawn");

    fighterJet = AppStateManager:getGameObjectController():getGameObjectFromId("2248180869");
    local laserSpawn = fighterJet:getSpawnComponentFromName("laserSpawn");
    laserSpawn:reactOnSpawn(function(spawnedGameObject, originGameObject) 
        spawnedLaserGameObject:getPhysicsComponent():setCollidable(true);
        local laserBillboard = spawnedLaserGameObject:getBillboardComponent();
        laserBillboard:setActivated(true);
        local shootSound = spawnedLaserGameObject:getSimpleSoundComponent();
        shootSound:setActivated(true);
        spawnedLaserGameObject:getPhysicsActiveComponent():applyRequiredForceForVelocity(Vector3(0, 0, -100));
    end);
    energy = fighterJet:getAttributesComponent():getAttributeValueByName("Energy");
    score = fighterJet:getAttributesComponent():getAttributeValueByName("Score");
    
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
    
    timeLine = thisGameObject:getTimeLineComponent();
    
    init()
end

level2_MainGameObject["disconnect"] = function()
    init()
    
    originAsteroid:setVisible(true);
    timeLine:setCurrentTimeSec(0);
end

level2_MainGameObject["onAsteriodTimePoint"] = function(timePointSec)
    asteroidSpawn:setActivated(true);
end

level2_MainGameObject["onEnemyLaserContactOnce"] = function(gameObject0, gameObject1, contact)
    contact = AppStateManager:getGameObjectController():castContactData(contact);

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
    
    thisEnemy:getPhysicsComponent():setCollidable(false);
    thisEnemy:setVisible(false);
    thisEnemy:getPhysicsActiveComponent():translate(Vector3(0, -5, 0));
    thisEnemy:getSimpleSoundComponent():setActivated(true);
    thisEnemy:getParticleUniverseComponentFromName("ExplosionParticle"):setActivated(true);
    thisEnemy:getPhysicsActiveComponent():setActivated(false);
    AppStateManager:getGameObjectController():deleteDelayedGameObject(thisEnemy:getId(), 3);

    score:setValueNumber(score:getValueNumber() + 20);
    scoreText:setCaption("Score: " .. toString(score:getValueNumber()));

end

level2_MainGameObject["onPlayerEnemyContactOnce"] = function(gameObject0, gameObject1, contact)
    
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

    thisEnemy:getPhysicsActiveComponent():setCollidable(false);
    thisEnemy:setVisible(false);
    thisEnemy:getPhysicsActiveComponent():translate(Vector3(0, -5, 0));
    --thisEnemy:setScale(Vector3(0.1, 0.1, 0.1));
    thisEnemy:getSimpleSoundComponent():setActivated(true);
    thisEnemy:getParticleUniverseComponentFromName("ExplosionParticle"):setActivated(true);
    thisEnemy:getPhysicsActiveComponent():setActivated(false);
    -- Do not delete directly, because a particle effect needs time to play
    AppStateManager:getGameObjectController():deleteDelayedGameObject(thisEnemy:getId(), 3);
end