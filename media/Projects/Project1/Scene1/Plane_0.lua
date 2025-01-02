module("Plane_0", package.seeall);
-- physicsActiveComponent = nil;
Plane_0 = {}

-- Scene: Scene1

require("init");

plane_0 = nil

Plane_0["connect"] = function(gameObject)
	--plane_0 = AppStateManager:getGameObjectController():castGameObject(gameObject);
	--physicsActiveComponent = gameObject:getPhysicsActiveComponent();
end

Plane_0["disconnect"] = function()

end

--Plane_0["update"] = function(dt)
	--physicsActiveComponent:applyForce(Vector3(0, 20, 0));
--end