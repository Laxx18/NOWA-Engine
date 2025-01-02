module("Plane_0_5", package.seeall);
-- physicsActiveComponent = nil;
Plane_0_5 = {}

-- Scene: Scene1

require("init");

plane_0_5 = nil

Plane_0_5["connect"] = function(gameObject)
	--plane_0_5 = AppStateManager:getGameObjectController():castGameObject(gameObject);
	--physicsActiveComponent = gameObject:getPhysicsActiveComponent();
end

Plane_0_5["disconnect"] = function()

end

--Plane_0_5["update"] = function(dt)
	--physicsActiveComponent:applyForce(Vector3(0, 20, 0));
--end