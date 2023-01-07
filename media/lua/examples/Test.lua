-- luaobjects.lua

--get an entity object and call its methods

log("Hello from LUA!")

local gameObject = GameObjectController:getGameObjectFromName("Ton#4");
if gameObject ~= nil then
	local physicsActiveComponent = gameObject:getPhysicsActiveComponent();

	--print ("\n[Lua]: Entity has "..entity:getId().. " id.");
    -- set position macht dumm!
    v = Vector3(5,10,0);
    t = v:length();
    log("\n[Lua]: Vector t length: " .. t);
    physicsActiveComponent:setPosition(Vector3(30, 15, 0));
    --physicsActiveComponent:setOrientation(Quaternion(Degree(45), Vector3.UNIT_Z));
	--physicsActiveComponent:setOrientation(Quaternion(Degree(90), Vector3(0, 0, 1)));
    --physicsActiveComponent:setOrientation(Quaternion(1, 0, 0, 0.5));
	-- does not work at the moment somehow
	--physicsComponent:setLinearDamping(1);

	--print ("\n[Lua]: Name " ..physicsComponent:getName().. " blub.");

	--print ("\n[Lua]: Entity is "..entity:getAge().. " years old.");

    GameObjectController:getGameObjectsFromNamePrefix("Ton*", tons);
    for ton in tons do
        logMessage(ton:getName());
    end

    -- for map
--    for key,value in pairs(myTable) do --actualcode
--        myTable[key] = "foobar"
--    end
end

--barrel = GameObjectController:getGameObjectFromName("barrel#0");
--if barrel ~= nil then
--   motorizedJoint = barrel:getPhysicsActiveComponent();
--   jointProperties = sJointProperties();
--   jointProperties.pin = Vector3(0, 0, 1);
--   jointProperties.rotateSpeed = 0.3;
--   jointProperties.activate = false;
--   motorizedJoint:applyJointProperties(jointProperties, "Motorized");
--end