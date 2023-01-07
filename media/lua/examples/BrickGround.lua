module("BrickGround", package.seeall);

function onAABBOverlap(gameObject0, gameObject1)

end

function onContact(gameObject0, gameObject1, contact)

end

function onContactOnce(gameObject0, gameObject1, contact)
	--example code :
	--local targetGameObject = nil;
	-- if gameObject0:getCategory() == "Player" then
		-- targetGameObject = gameObject0;
	-- else
		--targetGameObject = gameObject1;
--	end

	-- if targetGameObject ~= nil then
		-- local soundComponent = targetGameObject:getSimpleSoundComponent();
		--soundComponent:setActivated(true);
		--soundComponent:connect();
		--local walkSound = soundComponent:getSound();
		--walkSound:setPitch(0.5);
		--walkSound:setGain(contact : getNormalSpeed() * 0.1);
	--end
end
