module("Inventory1_Flubber_0", package.seeall);

Inventory1_Flubber_0 = {}

-- Scene: Inventory1

require("init");

flubber = nil;
inventory1_Flubber_0 = nil

Inventory1_Flubber_0["connect"] = function(gameObject)
    flubber = AppStateManager:getGameObjectController():castGameObject(gameObject);
    
    flubber:getMyGUIItemBoxComponent():setActivated(true);
    
    flubber:getMyGUIItemBoxComponent():reactOnDropItemRequest(function(dragDropData) 
            log("[Lua]: reactOnDropItemRequest Green Inventory resourceName: " .. dragDropData:getResourceName() .. " sender name: " .. AppStateManager:getGameObjectController():getGameObjectFromId(dragDropData:getSenderInventoryId()):getName());
            if (dragDropData:getSenderReceiverIsSame() == true) then
                --log("[Lua]: 1 reactOnDropItemRequest can drop: " .. (dragDropData:getCanDrop() and 'true' or 'false'));
                dragDropData:setCanDrop(true);
            elseif (dragDropData:getSenderInventoryId() == "2838543028") then -- Flubber_1
                dragDropData:setCanDrop(false);
            elseif (dragDropData:getSenderInventoryId() == "3968932022") then -- Flubber_2 Shop
                if (dragDropData:getResourceName() == "CoinItem") then
                    dragDropData:setCanDrop(false);
                else
                    dragDropData:setCanDrop(true);
                end
            else
                dragDropData:setCanDrop(true);
            end
        end);
    
    flubber:getMyGUIItemBoxComponent():reactOnDropItemAccepted(function(dragDropData) 
            log("[Lua]: reactOnDropItemAccepted Green Inventory resourceName: " .. dragDropData:getResourceName() .. " sender name: " .. AppStateManager:getGameObjectController():getGameObjectFromId(dragDropData:getSenderInventoryId()):getName());
            if (dragDropData:getSenderInventoryId() == "3968932022") then -- Flubber_2 Shop
                
                local coinQuantity = flubber:getMyGUIItemBoxComponent():getQuantity("CoinItem");
                local coinSellValue = flubber:getMyGUIItemBoxComponent():getSellValue("CoinItem");
                local coinValue = coinQuantity * coinSellValue;
                
                local shopOwner = AppStateManager:getGameObjectController():getGameObjectFromId(dragDropData:getSenderInventoryId());
                local speechBubble = shopOwner:getSpeechBubbleComponent();
                
                log("-->Money: " .. toString(coinValue) .. " Required: " .. dragDropData:getBuyValue())
                
                -- Checks if player has enough money to buy the item
                if (coinValue >= dragDropData:getBuyValue()) then
                    log("-->Diff Money: " .. coinValue - dragDropData:getBuyValue());

                    flubber:getMyGUIItemBoxComponent():removeQuantity("CoinItem", dragDropData:getBuyValue());
                    
                    if (dragDropData:getResourceName() == "FastShoesItem") then
                        flubber:getPhysicsPlayerControllerComponent():setMaxSpeed(10);
                        flubber:getPhysicsPlayerControllerComponent():setSpeed(10);
                    end
                    -- Shop gets the coins
                    local shopItemBox = shopOwner:getMyGUIItemBoxComponent();
                    shopItemBox:increaseQuantity("CoinItem", dragDropData:getBuyValue());
                    -- And the bought item gets decreased in the shop
                    shopItemBox:decreaseQuantity(dragDropData:getResourceName(), 1);
                    
                    speechBubble:setCaption("Wonderful!");
                    speechBubble:setActivated(true);
                else
                    speechBubble:setCaption("Sorry, but you have\n not enough money!");
                    speechBubble:setActivated(true);
                    dragDropData:setCanDrop(false);
                end
            else
                dragDropData:setCanDrop(true);
            end
        end);
end

Inventory1_Flubber_0["disconnect"] = function()

end

Inventory1_Flubber_0["onContactFriction"] = function(gameObject0, gameObject1, playerContact)
    playerContact:setResultFriction(2);
end

Inventory1_Flubber_0["onContact"] = function(gameObject0, gameObject1, playerContact)
    gameObject0 = AppStateManager:getGameObjectController():castGameObject(gameObject0);
    gameObject1 = AppStateManager:getGameObjectController():castGameObject(gameObject1);
    
    if (gameObject1:getCategory() ~= "Item") then
        return;
    end
    
    local inventoryItem = gameObject1:getInventoryItemComponent();
    if (inventoryItem ~= nil) then
        inventoryItem:addQuantityToInventory(flubber:getId(), 5, true);
        --AppStateManager:getGameObjectController():deleteGameObjectWithUndo(gameObject1:getId());
        AppStateManager:getGameObjectController():deleteGameObject(gameObject1:getId());
    end
end