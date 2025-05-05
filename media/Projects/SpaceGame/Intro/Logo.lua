module("Logo", package.seeall);

physicsActiveComponent = nil;
cameraComponent = nil;
soundComponent = nil;
introSoundFxComponent = nil;
particleComponent = nil;
spotLightJoint = nil;
fadeComponent = nil;

Logo = {}

-- Scene: Intro

require("init");

Logo["connect"] = function(gameObject)
    PointerManager:showMouse(false);
    gameObject = AppStateManager:getGameObjectController():castGameObject(gameObject);
    physicsActiveComponent = gameObject:getPhysicsActiveComponent();
    soundComponent = gameObject:getSimpleSoundComponent();
    particleComponent = gameObject:getParticleUniverseComponentFromName("Smoke1");
    fadeComponent = gameObject:getFadeComponent();
    
    cameraComponent = AppStateManager:getGameObjectController():getGameObjectFromName("IntroCamera"):getCameraComponent();
    cameraComponent:setActivated(true);
    
    local spotLightGameObject = AppStateManager:getGameObjectController():getGameObjectFromName("SpotLight");
    spotLightJoint = spotLightGameObject:getJointSliderActuatorComponent();
    introSoundFxComponent = spotLightGameObject:getSimpleSoundComponent();
    
    local fadeComponent = gameObject:getFadeComponent();
    fadeComponent:reactOnFadeCompleted(function() 
        if (Core:isGame() == true) then
            AppStateManager:changeAppState("MenuState");
        end
    end);
end

Logo["disconnect"] = function()
    PointerManager:showMouse(true);
    cameraComponent:setActivated(false);
    fadeComponent:setActivated(false);
    AppStateManager:getGameObjectController():getGameObjectFromName("MainCamera"):getCameraComponent():setActivated(true);
    soundComponent:setActivated(false);
    introSoundFxComponent:setActivated(false);
    particleComponent:setActivated(false);
    spotLightJoint:setActivated(false);
end

Logo["onHitFloorContact"] = function(gameObject0, gameObject1, contact)
    soundComponent:setActivated(true);
    particleComponent:setActivated(true);
end

Logo["MoveLightTimePoint"] = function(timePointSec)
    spotLightJoint:setActivated(true);
    introSoundFxComponent:setActivated(true);
    if (Core:isGame() == true) then
        fadeComponent:setActivated(true);
    end
end