module("Explosion", package.seeall); 

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
	--originGameObject:getSceneNode():setVisible(false, true);
	--log("\n[Lua]: gameobject translate: " .. originGameObject:getName());
	--originGameObject:getPhysicsActiveComponent():translate(Vector3(2, 0, 0));
	--GameObjectController:deleteGameObject(originGameObject:getId());
end

function onExplodeAffectedGameObject(originGameObject, affectedGameObject, distanceToBomb, detonationStrength)
	--affectedGameObject:getSceneNode():setVisible(false, true);
end