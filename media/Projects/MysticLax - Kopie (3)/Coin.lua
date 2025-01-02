module("Coin", package.seeall);

Coin = {}
mainGameObject = nil;
coinAttribute = nil;
coinText = nil;
attributesComponent = nil;
coinGameObject = nil;

Coin["connect"] = function(gameObject)
	mainGameObject = AppStateManager:getGameObjectController():getGameObjectFromId(MAIN_GAMEOBJECT_ID);
	coinGameObject = gameObject;
	coinGameObject:setVisible(true);
	
	coinText = mainGameObject:getMyGUITextComponentFromIndex(1);
	attributesComponent = mainGameObject:getAttributesComponent();
	coinAttribute = attributesComponent:getAttributeValueByName("Coins");
end

Coin["disconnect"] = function()
	--coinGameObject:setVisible(true);
end

Coin["onTriggerEnter"] = function(visitorGameObject)
    --log("[Lua]: onTriggerEnter: " .. visitorGameObject:getName());
	--log("[Lua]: coinGameObject: " .. coinGameObject:getName());
	
	--if (visitorGameObject:getName() == "PrehistoricLax_0") then
		local inventoryItem = coinGameObject:getInventoryItemComponent();
		inventoryItem:addQuantityToInventory(mainGameObject:getId(), 1);
		
		GameObjectController:deleteGameObject(coinGameObject:getId());
		--coinGameObject:setVisible(false);
		--coinGameObject:removePhysicsTriggerComponentWithUndo();
		--GameObjectController:removeComponentWithUndo(coinGameObject:getId(), 3);
	--end
end