module("PotionRed", package.seeall);

PotionRed = { }
mainGameObject = nil;
energyAttribute = nil;
energyText = nil;
attributesComponent = nil;
energyGameObject = nil;

PotionRed["connect"] = function(gameObject)
	mainGameObject = AppStateManager:getGameObjectController():getGameObjectFromId(MAIN_GAMEOBJECT_ID);
	energyGameObject = gameObject;
	
	energyText = mainGameObject:getMyGUITextComponent();
	attributesComponent = mainGameObject:getAttributesComponent();
	energyAttribute = attributesComponent:getAttributeValueByName("Energy");
end

PotionRed["disconnect"] = function()
	--energyGameObject:setVisible(true);
end

PotionRed["onTriggerEnter"] = function(visitorGameObject)
    log("[Lua]: onTriggerEnter: " .. visitorGameObject:getName());
	log("[Lua]: energyGameObject: " .. energyGameObject:getName());
	
	--local inventoryItem = energyGameObject:getInventoryItemComponent();
	--inventoryItem:addQuantityToInventory(mainGameObject:getId(), 1);
	
	--GameObjectController:deleteGameObject(energyGameObject:getId());
end

--PotionRed["onTriggerInside"] = function(visitorGameObject)
    --energyGameObject:setVisible(true);
--	log("[Lua]: onTriggerInside " .. visitorGameObject:getName());
--end

PotionRed["onTriggerLeave"] = function(visitorGameObject)
    --energyGameObject:setVisible(true);
	log("[Lua]: onTriggerLeave " .. visitorGameObject:getName());
end