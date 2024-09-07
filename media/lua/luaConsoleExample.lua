local thisGameObject = AppStateManager:getGameObjectController():getGameObjectFromId("3252972965");
local purePursuitComp = thisGameObject:getPurePursuitComponent();
local nodeGameObjects = AppStateManager:getGameObjectController():getGameObjectsFromComponent("NodeComponent");

purePursuitComp:setWaypointsCount(#nodeGameObjects + 1);
for i = 0, #nodeGameObjects do
    local gameObject = nodeGameObjects[i];
    purePursuitComp:setWaypointId(i, gameObject:getId());
end
purePursuitComp:reorderWaypoints();

-- print length:
-- purePursuitComp:setWaypointsCount(#nodeGameObjects);


-- Or use tables this way:

local tbl = {"value1", "value2", "value3"}

for i = 0, #tbl do
    print(i, tbl[i])
end