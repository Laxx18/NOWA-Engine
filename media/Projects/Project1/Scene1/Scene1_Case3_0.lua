module("Scene1_Case3_0", package.seeall);
physicsActiveComponent = nil;
Scene1_Case3_0 = {}

-- Scene: Scene1

require("init");

scene1_Case3_0 = nil

Scene1_Case3_0["connect"] = function(gameObject)
   scene1_Case3_0 = AppStateManager:getGameObjectController():castGameObject(gameObject);
   
   physicsActiveComponent = scene1_Case3_0:getPhysicsActiveComponent();
end

Scene1_Case3_0["disconnect"] = function()

end

Scene1_Case3_0["update"] = function(dt)
    physicsActiveComponent:applyForce(Vector3(0, 200, 0));
end