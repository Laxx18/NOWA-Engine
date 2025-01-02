module("Flubber_2", package.seeall);
physicsActiveComponent = nil;
Flubber_2 = {}

-- Scene: Scene1

require("init");

flubber_0 = nil;

canFly = false;

Flubber_2["connect"] = function(gameObject)
   flubber_0 = AppStateManager:getGameObjectController():castGameObject(gameObject);
   physicsActiveComponent = gameObject:getPhysicsActiveComponent();
   
   if (EventType.ActivateFly ~= nil) then
         AppStateManager:getScriptEventManager():registerEventListener(EventType.ActivateFly, Flubber_2["onActivateFly"]);
     end
end

Flubber_2["disconnect"] = function()

end

Flubber_2["onActivateFly"] = function(eventData)
    local activated = eventData["activated"];
    canFly = activated;
    log("--->activated called");
end

Flubber_2["update"] = function(dt)
    if (canFly) then
        physicsActiveComponent:applyForce(Vector3(0, 500, 0));
    end
end