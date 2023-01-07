-- luaobjects.lua

--get an entity object and call its methods

local gameObject = GameObjectController:getGameObjectFromId(1);
if gameObject ~= nil then
local physicsObject = getPhysicsObjectFromCast(GOC:getGameObjectFromId(1));

--print ("\n[Lua]: Entity has "..entity:getId().. " id.");

physicsObject:setPosition(Vector3(0, 0, 0));
--physicsObject:setOrientation(Quaternion(Degree(90), Vector3(0, 0, 1)));
-- does not work at the moment somehow
--physicsObject:setLinearDamping(1);

print ("\n[Lua]: Entity is " ..physicsObject:getId().. " years old.");

--print ("\n[Lua]: Entity is "..entity:getAge().. " years old.");

end