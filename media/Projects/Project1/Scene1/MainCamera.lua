module("MainCamera", package.seeall);
-- physicsActiveComponent = nil;
MainCamera = {}

-- Scene: Scene1

require("init");

mainCamera = nil

MainCamera["connect"] = function(gameObject)
	--mainCamera = AppStateManager:getGameObjectController():castGameObject(gameObject);
	--physicsActiveComponent = gameObject:getPhysicsActiveComponent();
end

MainCamera["disconnect"] = function()

end

--MainCamera["update"] = function(dt)
	--physicsActiveComponent:applyForce(Vector3(0, 20, 0));
--end