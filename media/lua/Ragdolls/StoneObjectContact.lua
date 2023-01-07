--local namespace = {}
--setmetatable(namespace, {__index = _G})
--RagDollObjectContactScript = namespace;
--setfenv(1, namespace);

--local stoneObjectContact = {}
--setfenv(1, stoneObjectContact)

--module("StoneObjectContact")


function onAABBOverlap(gameObject0, gameObject1)

end

function onContact(gameObject0, gameObject1, contact)
    
end

function onContactOnce(gameObject0, gameObject1, contact)
     log("Calling onContactOnce in StoneObjectContact.lua");
     local audioHolder = GameObjectController:getGameObjectFromName("Plane#0");
     
     if audioHolder ~= nil then
        local soundComponent = audioHolder:getSimpleSoundComponent();
        soundComponent:setActivated('Stone', true);
        local hitSound = soundComponent:getSound('Stone');
        if gameObject0 ~= nil then
			hitSound:setPosition(gameObject0:getPosition());
		elseif gameObject1 ~= nil then
			hitSound:setPosition(gameObject1:getPosition());
		end
		hitSound:setPitch(0.5);
        -- max 1.0f
		hitSound:setGain(contact:getNormalSpeed() * 0.1); 
    end
end