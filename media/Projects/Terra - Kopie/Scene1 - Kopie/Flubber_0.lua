module("Flubber_0", package.seeall);
physicsActiveComponent = nil;
Flubber_0 = {}

-- Scene: Scene1

require("init");

flubber_0 = nil;

Flubber_0["connect"] = function(gameObject)
   flubber_0 = AppStateManager:getGameObjectController():castGameObject(gameObject);
   physicsActiveComponent = gameObject:getPhysicsActiveComponent();
   
   local eventData = {};
   eventData["activated"] = true;
   AppStateManager:getScriptEventManager():queueEvent(EventType.ActivateFly, eventData);
end

Flubber_0["disconnect"] = function()

end

Flubber_0["update"] = function(dt)
    physicsActiveComponent:applyForce(Vector3(0, 500, 0));
end