module("FloorPlayerContact", package.seeall);

function onAABBOverlap(gameObject0, gameObject1)

end

function onContact(gameObject0, gameObject1, contact)

end

function onContactOnce(gameObject0, gameObject1, contact)
	local targetGameObject = nil;
	if gameObject0:getCategory() == "Player" then
		targetGameObject = gameObject0;
	else
		targetGameObject = gameObject1;
	end

	if targetGameObject ~= nil then
		local physicsActiveComponent = targetGameObject:getPhysicsActiveComponent();
		physicsActiveComponent:setVelocity(physicsActiveComponent->getVelocity() * Vector3(1, 0, 1));
		--soundComponent:setActivated(true);
		--soundComponent:connect();
		--local walkSound = soundComponent:getSound();
		--walkSound:setPitch(0.5);
		--walkSound:setGain(contact : getNormalSpeed() * 0.1);
	end
end
