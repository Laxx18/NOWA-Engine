module("barrel_1", package.seeall);
physicsActiveComponent = nil;

function connect(gameObject)
	physicsActiveComponent = gameObject:getPhysicsActiveComponent();
	--log("---->connect: " .. gameObject:getName());
end

function disconnect()
	--log("---->disconnect: " .. gameObject:getName());
end

function update(dt)
	physicsActiveComponent:applyForce(Vector3(math.random(300), math.random(300), math.random(300)));
	--log("---->update: " .. gameObject:getName());
end