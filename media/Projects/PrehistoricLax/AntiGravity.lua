module("AntiGravity", package.seeall);
-- Scene: Level2

require("init");

local antiGravity = nil

local physicsTriggerComponent = nil;

AntiGravity = {}

AntiGravity["connect"] = function(gameObject)
    antiGravity = AppStateManager:getGameObjectController():castGameObject(gameObject);
    physicsTriggerComponent = antiGravity:getPhysicsTriggerComponent();
    physicsTriggerComponent:reactOnEnter(function(visitorGameObject)
          visitorGameObject = AppStateManager:getGameObjectController():castGameObject(visitorGameObject);
          local phyiscsActiveComponent = visitorGameObject:getPhysicsActiveComponent();
          if (phyiscsActiveComponent) then
              local gravity = phyiscsActiveComponent:getGravity();
              phyiscsActiveComponent:setGravity(Vector3(-gravity.x * 0.5, -gravity.y  * 0.5, -gravity.z  * 0.5));
              antiGravity:getSimpleSoundComponent():setActivated(true);
          end
    end);
    
    physicsTriggerComponent:reactOnLeave(function(visitorGameObject)
          visitorGameObject = AppStateManager:getGameObjectController():castGameObject(visitorGameObject);
          local phyiscsActiveComponent = visitorGameObject:getPhysicsActiveComponent();
          if (phyiscsActiveComponent) then
              local gravity = phyiscsActiveComponent:getGravity();
              phyiscsActiveComponent:setGravity(Vector3(-gravity.x * 2, -gravity.y * 2, -gravity.z * 2));
              antiGravity:getSimpleSoundComponent():setActivated(false);
          end
        
    end);
end

AntiGravity["disconnect"] = function()

end

--AntiGravity["update"] = function(dt)
    --physicsActiveComponent:applyOmegaForce(Vector3(0, 10, 0));
--end