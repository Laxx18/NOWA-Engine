module("HollowSphere1_0", package.seeall);
-- Scene: EvilMagic

require("init");

local hollowSphere1_0 = nil

local physicsExplosionComponent = nil;

HollowSphere1_0 = {}

HollowSphere1_0["connect"] = function(gameObject)
    hollowSphere1_0 = AppStateManager:getGameObjectController():castGameObject(gameObject);
    physicsExplosionComponent = hollowSphere1_0:getPhysicsExplosionComponent();
    
    hollowSphere1_0:getSimpleSoundComponent():setActivated(false);
    
    physicsExplosionComponent:reactOnExplode(function()
        hollowSphere1_0:getSimpleSoundComponent():setActivated(true);
    end);
end

HollowSphere1_0["disconnect"] = function()

end