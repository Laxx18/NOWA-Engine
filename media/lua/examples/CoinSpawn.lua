
function onSpawn(spawnedGameObject, originGameObject)
    local coinComponent = spawnedGameObject:getPhysicsActiveComponent();
    if coinComponent ~= nil then
        -- add impulse to each coin so that it flies in the direction where the cannon is actually rotated (note, its configured, that the coin is automatically rotated as the cannon)
        local direction = coinComponent:getOrientation():yAxis();
        --direction = coinComponent:getOrientation() * Vector3.UNIT_Y;
        coinComponent:addImpulse(direction * 20);
        coinComponent:setOrientation(Quaternion.IDENTITY);

        -- play sound
        --local soundComponent = originGameObject:getSimpleSoundComponent();
        --soundComponent:setActivated(soundComponent:getFirstSoundName(), true);

        -- give some recoil to the cannon 
        local spawnerComponent = originGameObject:getPhysicsActiveComponent();
        spawnerComponent:addImpulse(direction * -50);

        -- add a smoke particle effect
        --local particleComponent = originGameObject:getParticleUniverseComponent();
        --particleComponent:setActivated(particleComponent:getFirstParticleName(), true);
    end
end

function onVanish(spawnedGameObject, originGameObject)
     log("---->vanish: " .. spawnedGameObject:getName());
end