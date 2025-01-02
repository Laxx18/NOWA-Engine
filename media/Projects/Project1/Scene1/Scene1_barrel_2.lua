module("Scene1_barrel_2", package.seeall);
-- physicsActiveComponent = nil;
Scene1_barrel_2 = {}

-- Scene: Scene1

require("init");

scene1_barrel_0 = nil

Scene1_barrel_2["connect"] = function(gameObject)
	--scene1_barrel_0 = AppStateManager:getGameObjectController():castGameObject(gameObject);
	--physicsActiveComponent = gameObject:getPhysicsActiveComponent();
end

Scene1_barrel_2["disconnect"] = function()

end

--Scene1_barrel_2["update"] = function(dt)
	--physicsActiveComponent:applyForce(Vector3(0, 20, 0));
--end