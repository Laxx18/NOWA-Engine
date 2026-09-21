module("MainGameObject", package.seeall);
-- Scene: Level1

require("init");

mainGameObject = nil

-- physicsActiveComponent = nil;

MainGameObject = {}

MainGameObject["connect"] = function(gameObject)
	--mainGameObject = AppStateManager:getGameObjectController():castGameObject(gameObject);
	--physicsActiveComponent = mainGameObject:getPhysicsActiveComponent();
end

MainGameObject["disconnect"] = function()

end

--MainGameObject["update"] = function(dt)
	--physicsActiveComponent:applyOmegaForce(Vector3(0, 10, 0));
--end