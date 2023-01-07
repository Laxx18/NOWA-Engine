module("ellipsoid_0", package.seeall);
physicsActiveComponent = nil;
cameraBehaviorThirdPersonComponent = nil;

function connect(gameObject)
	physicsActiveComponent = gameObject:getPhysicsActiveComponent();
	cameraBehaviorThirdPersonComponent = gameObject:getCameraBehaviorThirdPersonComponent();
	-- Activate the 3rd person camera
	--cameraBehaviorThirdPersonComponent:setActivated(true);
end

function disconnect()
	-- Go back to normal editor camera
	--cameraBehaviorThirdPersonComponent:setActivated(false);
end

function update(dt)
	local angularVel = Vector3(0, 0, 0);
	if Core:getKeyboard():isKeyDown(NOWA_K_LEFT) then
		angularVel = angularVel + Vector3(0, 8, 0);
	elseif Core:getKeyboard():isKeyDown(KeyEvent.KC_RIGHT) then
		angularVel = angularVel + Vector3(0, -8, 0);
	end
	
	local direction = physicsActiveComponent:getOrientation():xAxis();
	direction = direction * Vector3(1, 0, 1);
	direction:normalise();
	
	if Core:getKeyboard():isKeyDown(KeyEvent.KC_UP) then
		angularVel = angularVel + direction * -10;
	elseif Core:getKeyboard():isKeyDown(KeyEvent.KC_DOWN) then
		angularVel = angularVel + direction * 10;
	end
	
	physicsActiveComponent:applyAngularVelocity(angularVel);
	
	if Core:getKeyboard():isKeyDown(KeyEvent.KC_SPACE) then
	    local direction = physicsActiveComponent:getOrientation():yAxis();
		physicsActiveComponent:applyForce(Vector3(0, 1000, 0));
	end
end