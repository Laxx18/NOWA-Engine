module("Savepoint", package.seeall);
-- Scene: Level5

require("init");

local savepoint = nil

Savepoint = {}

Savepoint["connect"] = function(gameObject)
	savepoint = AppStateManager:getGameObjectController():castGameObject(gameObject);
	local physicsTriggerComponent = bossAreaMechanics:getPhysicsTriggerComponent();
    
    physicsTriggerComponent:reactOnEnter(function(visitorGameObject)
	
		if (Core:isGame() == true) then
          visitorGameObject = AppStateManager:getGameObjectController():castGameObject(visitorGameObject);
		  
		end
    end);
end