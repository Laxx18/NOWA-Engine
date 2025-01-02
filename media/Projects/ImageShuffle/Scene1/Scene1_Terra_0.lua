module("Scene1_Terra_0", package.seeall);

Scene1_Terra_0 = {}

-- Scene: Scene1

require("init");

terraGameObject = nil

Scene1_Terra_0["connect"] = function(gameObject)
	terraGameObject = AppStateManager:getGameObjectController():castGameObject(gameObject);

    local brushNames = terraGameObject:getTerraComponent():getAllBrushNames();
    
    for i = 0, #brushNames do
        local brushName = brushNames[i];
        log("Brushname: " .. brushName);
    end
    terraGameObject:getTerraComponent():setBrushName("Circular.png");
    terraGameObject:getTerraComponent():setBrushSize(32);
    terraGameObject:getTerraComponent():setImageLayerId(1);
end

Scene1_Terra_0["disconnect"] = function()
    AppStateManager:getGameObjectController():undoAll();
end

--Scene1_Terra_0["update"] = function(dt)
	--physicsActiveComponent:applyForce(Vector3(0, 20, 0));
--end

Scene1_Terra_0["onContactOnce"] = function(gameObject0, gameObject1, contact)
    local otherGameObject = nil;
    
    if (gameObject0 ~= terraGameObject) then
        otherGameObject = gameObject0;
    else
        otherGameObject = gameObject1;
    end
    
    log("data: " .. contact:print());
    
    if (contact:getNormalSpeed() > 30) then
        local positionAndNormal = contact:getPositionAndNormal();
	
        local position = positionAndNormal[0];
        local normal = positionAndNormal[1];
        
        terraGameObject:getTerraComponent():modifyTerrainLoop(position, -1000, 5);
    end
end