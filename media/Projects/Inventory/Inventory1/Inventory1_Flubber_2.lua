module("Inventory1_Flubber_2", package.seeall);

Inventory1_Flubber_2 = {}

-- Scene: Inventory1

require("init");

thisGameObject = nil
itemBox = nil;
areaOfInterest = nil;
myGuiController = nil;
thisVisitorGameObject = nil;

Inventory1_Flubber_2["connect"] = function(gameObject)
    thisGameObject = AppStateManager:getGameObjectController():castGameObject(gameObject);
    itemBox = thisGameObject:getMyGUIItemBoxComponent();
    areaOfInterest = thisGameObject:getAreaOfInterestComponent();
    myGuiController = thisGameObject:getMyGUIFadeAlphaControllerComponent();
    
    itemBox:setActivated(false);
    myGuiController:setAlpha(0);
    
    areaOfInterest:reactOnEnter(function(visitorGameObject)
            myGuiController:setAlpha(1);
            myGuiController:setActivated(true);
            itemBox:setActivated(true);
            thisVisitorGameObject = visitorGameObject;
        end);
    
    areaOfInterest:reactOnLeave(function(visitorGameObject)
            myGuiController:setAlpha(0);
            myGuiController:setActivated(true);
            itemBox:setActivated(false);
            thisVisitorGameObject = nil;
        end);
end

Inventory1_Flubber_2["disconnect"] = function()
     myGuiController:setActivated(false);
end

Inventory1_Flubber_2["update"] = function(dt)
    if (thisVisitorGameObject) then
        log("--->hier");
        local resultOrientation = MathHelper:faceTarget(thisGameObject:getSceneNode(), thisVisitorGameObject:getSceneNode(), thisGameObject:getDefaultDirection());
        thisGameObject:getPhysicsComponent():setOmegaVelocityRotateTo(resultOrientation, Vector3.UNIT_Y, 0.1);
    end
end