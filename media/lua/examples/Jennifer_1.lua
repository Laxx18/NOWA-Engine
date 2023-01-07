module("Jennifer_1", package.seeall);
playerControllerClickToPointComponent = nil;
physicsActiveComponent = nil;

function connect(gameObject)
	
end

function disconnect()

end

function update(dt)
	
end

function onEnter(gameObject)
	log("onEnter 1: " .. gameObject:getName());
	--playerControllerClickToPointComponent = gameObject:getPlayerControllerClickToPointComponent();
	--playerControllerClickToPointComponent:getMovingBehavior():setBehavior(eBehaviorType.NONE);
	--physicsActiveComponent = gameObject:getPhysicsActiveComponent();
	--physicsActiveComponent:applyAngularVelocity(Vector3.ZERO);
end

function onLeave(gameObject)

end