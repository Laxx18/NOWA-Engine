module("Explosion2", package.seeall); 

function onTimerSecondTick(originGameObject)
    --local soundComponent = originGameObject:getSimpleSoundComponent();
    --soundComponent:setActivated(true);
	--soundComponent:connect();
end

function onExplode(originGameObject)
   --local soundComponent = originGameObject:getSimpleSoundComponent();
   --soundComponent:setActivated(true);
   --soundComponent:connect();
   --local particleComponent = originGameObject:getParticleUniverseComponent();
   --particleComponent:setActivated(particleComponent:getFirstParticleName(), true);
    log("\n[Lua]: gameobject visible: " .. originGameObject:getName());
   originGameObject:getSceneNode():setVisible(false, true);
   --GameObjectController:deleteGameObject(originGameObject:getId());
end

function onExplodeAffectedGameObject(originGameObject, affectedGameObject, distanceToBomb, detonationStrength)
   
end