module("Case1_0", package.seeall);
-- physicsActiveComponent = nil;

function connect(gameObject)
	--physicsActiveComponent = gameObject:getPhysicsActiveComponent();
end

function disconnect()

end

function update(dt)
	--physicsActiveComponent:applyForce(Vector3(0, 20, 0));
end

function onTriggerEnter(gameObject)
	log("onTriggerEnter: " .. gameObject:getName());
	--playerControllerClickToPointComponent = gameObject:getPlayerControllerClickToPointComponent();
	--playerControllerClickToPointComponent:getMovingBehavior():setBehavior(eBehaviorType.NONE);
	--physicsActiveComponent = gameObject:getPhysicsActiveComponent();
	--physicsActiveComponent:applyAngularVelocity(Vector3.ZERO);
end

function onTriggerLeave(gameObject)
	log("onTriggerLeave: " .. gameObject:getName());
end