module("coin_moraf_0", package.seeall);

animationComponent = nil;
simpeSoundComponent = nil;

function connect(gameObject)
	
end

function disconnect()

end

function update(dt)
	
end

function onEnter(otherGameObject)
    log("onEnter: " .. otherGameObject:getName());
	if otherGameObject:getName() == "Chest2_0" then
		animationComponent = otherGameObject:getAnimationComponent();
		if nil ~= animationComponent then
		  log("set activated: true");
				animationComponent:setAnimationName("Open");
		   animationComponent:setRepeat(false);
		   animationComponent:setActivated(true);
		end
		simpeSoundComponent = otherGameObject:getSimpleSoundComponent();
		if nil ~= simpeSoundComponent then
			simpeSoundComponent:setActivated(true);
		end
	end
end

function onLeave(otherGameObject)
	if otherGameObject:getName() == "Chest2_0" then
		-- Only deactivate if leave is called for the chest!
		if nil ~= animationComponent then
			animationComponent:setAnimationName("Close");
			animationComponent:setActivated(true);
		--	log("set activated: false");
		end
		if nil ~= simpeSoundComponent then
			simpeSoundComponent:setActivated(true);
		end
	end
end