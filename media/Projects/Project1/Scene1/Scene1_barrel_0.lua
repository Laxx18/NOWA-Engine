module("Scene1_barrel_0", package.seeall);

 
Scene1_barrel_0 = {}

physicsActiveComponent = nil;

scene1_barrel_0 = nil

Scene1_barrel_0["connect"] = function(gameObject)
    local nodeGameObjects = AppStateManager:getGameObjectController():getGameObjectsFromComponent("NodeComponent");
     local i = 0
    while i <= #nodeGameObjects do
        local gameObject = nodeGameObjects[i];
       
        i = i + 1  -- Increment the counter
    end
    
     --for i = 0, #nodeGameObjects do
         --local gameObject = nodeGameObjects[i];
            
     --end
    AppStateManager:getGameObjectController():activatePlayerController(true, "id_" .. scene1_barrel_0:getName()
     scene1_barrel_0 = AppStateManager:getGameObjectController():castGameObject(gameObject);
     scene1_barrel_0:getPhysicsActiveComponent()
     scene1_barrel_0:getReferenceComponentFromIndex(1)
     physicsActiveComponent = scene1_barrel_0:getPhysicsActiveComponent();
     scene1_barrel_0:getCompositorEffectSharpenEdgesComponentFromName("blub")
     physicsActiveComponent:setDirection(Vector3.NEGATIVE_UNIT_X, Vector3.UNIT_Y);
     
    -- Note: getCollisionSize is Vector3, hence match that, so temp would be of type Vector3
    local temp=scene1_barrel_0:getPhysicsActiveComponent():getCollisionSize();
   local temp3 = nil;
   local temp2 = temp; temp3=temp2;
   
end

function attachWeapon()
    local physicsRagdollComponent = scene1_barrel_0:getPhysicsRagDollComponent();
    --physicsRagdollComponent
     
end

Scene1_barrel_0["disconnect"] = function()

end

Scene1_barrel_0["update"] = function(dt)
    physicsActiveComponent:applyForce(Vector3.NEGATIVE_UNIT_X * 200);
end

Scene1_barrel_0["onContactOnce"] = function(gameObject0, gameObject1, contact)
    local b = nil contact = AppStateManager:getGameObjectController():castContact(contact);
    local otherGameObject = nil;
    
    if (gameObject0 ~= terraGameObject) then
        otherGameObject = gameObject0;
    else
        otherGameObject = gameObject1;
    end
    
     
     playerController = prehistoricLax:getPlayerControllerJumpNRunLuaComponent();
     walkSound = playerController:getOwner():getSimpleSoundComponentFromIndex(0);
     jumpSound = playerController:getOwner():getSimpleSoundComponentFromIndex(1);
     smokeParticle = playerController:getOwner():getParticleUniverseComponentFromIndex(0);
    
    if (a == Vector3) then
    
    end
    
    otherGameObject = AppStateManager:getGameObjectController():castGameObject(otherGameObject);
    otherGameObject:getAiFlockingComponent():getOwner()
    
    log("data: " .. contact:print());
   
    if (contact:getNormalSpeed() > 30) then
        local positionAndNormal = contact:getPositionAndNormal();
    
        local position = positionAndNormal[0];
        local normal = positionAndNormal[1];

        terraGameObject:getTerraComponent():modifyTerrainLoop(position, -1000, 5);
    end
end