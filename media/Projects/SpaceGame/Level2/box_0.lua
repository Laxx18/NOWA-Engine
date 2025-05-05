module("box_0", package.seeall);

local laserScript = require("Laser")

box_0 = {};

box_0["onTriggerEnter"] = function(otherGameObject)
	if otherGameObject:getCategory() == "Laser" then
		laserScript.removeLaser(otherGameObject:getId());
		GameObjectController:deleteDelayedGameObject(otherGameObject:getId(), 1);
	end
end

box_0["onTriggerLeave"] = function(otherGameObject)
	
end