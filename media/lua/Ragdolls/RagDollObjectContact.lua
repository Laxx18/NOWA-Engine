--local namespace = {}
--setmetatable(namespace, {__index = _G})
--RagDollObjectContactScript = namespace;
--setfenv(1, namespace);

--local ragDollObjectContact = {}
--setfenv(1, ragDollObjectContact)

--module("RagDollObjectContact")

function onAABBOverlap(gameObject0, gameObject1)

end

function onContact(gameObject0, gameObject1, contact)
    
end

function onContactOnce(gameObject0, gameObject1, contact)
     log("Calling onContactOnce in RagDollObjectContact.lua");
     local audioHolder = GameObjectController:getGameObjectFromName("Plane#0");
     
     if audioHolder ~= nil then
        local soundComponent = audioHolder:getSimpleSoundComponent();
        soundComponent:setActivated('Hit', true);
        local hitSound = soundComponent:getSound('Hit');
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