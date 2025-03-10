module("Inventory1_Flubber_1", package.seeall);

Inventory1_Flubber_1 = {}

-- Scene: Inventory1

require("init");

flubber = nil;
inventory1_Flubber_1 = nil

Inventory1_Flubber_1["connect"] = function(gameObject)
    flubber = AppStateManager:getGameObjectController():castGameObject(gameObject);
    flubber:getMyGUIItemBoxComponent():setActivated(false);
    
    flubber:getMyGUIItemBoxComponent():reactOnDropItemRequest(function(dragDropData) 
        log("[Lua]: Blue Inventory resourceName: " .. dragDropData:getResourceName() .. " sender name: " .. AppStateManager:getGameObjectController():getGameObjectFromId(dragDropData:getSenderInventoryId()):getName());
        dragDropData:setCanDrop(true);
    end);
end

Inventory1_Flubber_1["disconnect"] = function()

end

Inventory1_Flubber_1["onContactFriction"] = function(gameObject0, gameObject1, playerContact)
    playerContact:setResultFriction(2);
end

Inventory1_Flubber_1["onContact"] = function(gameObject0, gameObject1, playerContact)
    gameObject0 = AppStateManager:getGameObjectController():castGameObject(gameObject0);
    gameObject1 = AppStateManager:getGameObjectController():castGameObject(gameObject1);
    
    if (gameObject1:getCategory() ~= "Item") then
        return;
    end
    
    local inventoryItem = gameObject1:getInventoryItemComponent();
    if (inventoryItem ~= nil) then
        inventoryItem:addQuantityToInventory(flubber:getId(), 5, true);
        AppStateManager:getGameObjectController():deleteGameObject(gameObject1:getId());
    end
end