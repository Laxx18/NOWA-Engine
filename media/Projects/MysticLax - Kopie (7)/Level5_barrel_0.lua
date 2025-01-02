module("Level5_barrel_0", package.seeall);
physicsActiveComponent = nil;
Level5_barrel_0 = {}

-- Scene: Level5

require("init");

level5_barrel_0 = nil

Level5_barrel_0["connect"] = function(gameObject)
	level5_barrel_0 = AppStateManager:getGameObjectController():castGameObject(gameObject);
	physicsActiveComponent = gameObject:getPhysicsActiveComponent();
end

Level5_barrel_0["disconnect"] = function()

end

Level5_barrel_0["update"] = function(dt)
	physicsActiveComponent:applyForce(Vector3(0, 2000, 0));
end