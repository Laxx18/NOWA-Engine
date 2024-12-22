return {

	AiComponent =
	{
		type = "class",
		description = "Base class for ai components with some attributes. Note: The game Object must have a PhysicsActiveComponent.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets the ai component is activated. If true, it will move according to its specified behavior.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether ai component is activated. If true, it will move according to its specified behavior.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setRotationSpeed =
			{
				type = "method",
				description = "Sets the rotation speed for the ai controller game object.",
				args = "(number rotationSpeed)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRotationSpeed =
			{
				type = "function",
				description = "Gets the rotation speed for the ai controller game object.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setFlyMode =
			{
				type = "method",
				description = "Sets the ai controller game object should be clamped at the floor or can fly.",
				args = "(boolean flyMode)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getIsInFlyMode =
			{
				type = "function",
				description = "Gets whether the ai controller game object should is clamped at the floor or can fly.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getMovingBehavior =
			{
				type = "function",
				description = "Gets the moving behavior for direct ai manipulation.",
				args = "()",
				returns = "(MovingBehavior)",
				valuetype = "MovingBehavior"
			},
			reactOnPathGoalReached =
			{
				type = "method",
				description = "Sets whether to react the agent reached the goal.",
				args = "(func closureFunction)",
				returns = "(nil)",
				valuetype = "nil"
			},
			reactOnAgentStuck =
			{
				type = "method",
				description = "Sets whether to react the agent got stuck.",
				args = "(func closureFunction)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setAutoOrientation =
			{
				type = "method",
				description = "Sets whether the agent should be auto orientated during ai movement.",
				args = "(boolean autoOrientation)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setAutoAnimation =
			{
				type = "method",
				description = "Sets whether to use auto animation during ai movement. That is, the animation speed is adapted dynamically depending the velocity, which will create a much more realistic effect. Note: The game object must have a proper configured animation component.",
				args = "(boolean autoAnimation)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setAgentId =
			{
				type = "method",
				description = "Sets agent id, which shall be moved.",
				args = "(string agentId)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	AiFlockingComponent =
	{
		type = "class",
		description = "Requirements: A kind of physics active component must exist. Usage: Move a group of agents according flocking behavior. Note: Several AiComponents for an agent can be combined. Requirements: A kind of physics active component must exist and all the agents must belong to the same category.",
		inherits = "AiComponent",
		childs = 
		{
			setNeighborDistance =
			{
				type = "method",
				description = "Sets the minimum distance each flock does have to its neighbor. Default valus is 1.6. Works in conjunction with separation behavior.",
				args = "(number neighborDistance)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getNeighborDistance =
			{
				type = "function",
				description = "Gets the minimum distance each flock does have to its neighbor.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setCohesionBehavior =
			{
				type = "method",
				description = "Sets whether to use cohesion behavior. That is each game object goes towards a common center of mass of all other game objects.",
				args = "(boolean cohesion)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCohesionBehavior =
			{
				type = "function",
				description = "Gets whether to use cohesion behavior.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setSeparationBehavior =
			{
				type = "method",
				description = "Sets whether to use separation. That is the minimum distance each flock does have to its neighbor.",
				args = "(boolean separation)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSeparationBehavior =
			{
				type = "function",
				description = "Gets twhether to use separation.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setAlignmentBehavior =
			{
				type = "method",
				description = "Sets whether to use alignment behavior. This is a common direction all flocks have.",
				args = "(boolean alignment)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAlignmentBehavior =
			{
				type = "function",
				description = "Gets whether to use alignment behavior.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setObstacleBehavior =
			{
				type = "method",
				description = "Sets whether to use a obstacle behavior. That is, a flock will change its direction when comming near an obstacle.",
				args = "(boolean obstacle)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getObstacleBehavior =
			{
				type = "function",
				description = "Gets whether to use a obstacle behavior.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setFleeBehavior =
			{
				type = "method",
				description = "Sets whether to use flee behavior. That is the flock does flee from a target game object (target id).",
				args = "(boolean flee)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getFleeBehavior =
			{
				type = "function",
				description = "Gets whether to use flee behavior.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setSeekBehavior =
			{
				type = "method",
				description = "Sets whether to use seek behavior. That is the flock does seek a target game object (target id).",
				args = "(boolean seek)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSeekBehavior =
			{
				type = "function",
				description = "Gets the wander distance.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setTargetId =
			{
				type = "method",
				description = "Sets target id, that is used for flee or seek behavior.",
				args = "(string id)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTargetId =
			{
				type = "function",
				description = "Gets the target id, that is used for flee or seek behavior.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			}
		}
	},
	AiHideComponent =
	{
		type = "class",
		description = "Requirements: A kind of physics active component must exist. Usage: Hides an agent against a target. Note: Several AiComponents for an agent can be combined. Requirements: A kind of physics active component must exist and a valid target GameObject.",
		inherits = "AiComponent",
		childs = 
		{
			setTargetId =
			{
				type = "method",
				description = "Sets a target id game object, this game object is hiding at.",
				args = "(string targetId)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTargetId =
			{
				type = "function",
				description = "Gets the target id game object, this game object is hiding at.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setObstacleRangeRadius =
			{
				type = "method",
				description = "Sets the radius, at which the game object keeps distance at an obstacle.",
				args = "(number radius)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getObstacleRangeRadius =
			{
				type = "function",
				description = "Gets the radius, at which the game object keeps distance at an obstacle.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setObstacleCategories =
			{
				type = "method",
				description = "Sets categories, that are considered as obstacle for keeping a distance. Note: Combined categories can be used like 'ALL-Player' etc.",
				args = "(string categories)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getObstacleCategories =
			{
				type = "function",
				description = "Gets categories, that are considered as obstacle for keeping a distance.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			}
		}
	},
	AiLuaComponent =
	{
		type = "class",
		description = "Usage: Define lua script tables with the corresponding state and logic. Requirements: A kind of physics active component must exist and a LuaScriptComponent, which references the script file.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Activates the components behaviour, so that the lua script will be executed.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether the component behaviour is activated or not.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setRotationSpeed =
			{
				type = "method",
				description = "Sets the agent rotation speed. Default value is 0.1.",
				args = "(number speed)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRotationSpeed =
			{
				type = "function",
				description = "Gets the agent rotation speed. Default value is 0.1.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setFlyMode =
			{
				type = "method",
				description = "Sets whether to use fly mode (taking y-axis into account). Note: this can be used e.g. for birds flying around.",
				args = "(boolean flyMode)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isInFlyMode =
			{
				type = "function",
				description = "Gets whether the agent is in flying mode (taking y-axis into account).",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setStartStateName =
			{
				type = "method",
				description = "Sets the start state name, which will be loaded in lua script and executed.",
				args = "(string stateName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getStartStateName =
			{
				type = "function",
				description = "Gets the start state name, which is loaded in lua script and executed.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			getMovingBehavior =
			{
				type = "function",
				description = "Gets the moving behavior instance for this agent.",
				args = "()",
				returns = "(MovingBehavior)",
				valuetype = "MovingBehavior"
			},
			setGlobalState =
			{
				type = "method",
				description = "Sets the global state name, which will be loaded in lua script and executed besides the current state. Important: Do not use state function in 'connect' function, because at this time the AiLuaComponent is not ready!",
				args = "(Table stateName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			exitGlobalState =
			{
				type = "method",
				description = "Exits the global state, if it does exist.",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCurrentState =
			{
				type = "function",
				description = "Gets the current state name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			getPreviousState =
			{
				type = "function",
				description = "Gets the previous state name, that has been loaded bevor the current state name. Important: Do not use state function in 'connect' function, because at this time the AiLuaComponent is not ready!",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			getGlobalState =
			{
				type = "function",
				description = "Gets the global state name, if existing, else empty string. Important: Do not use state function in 'connect' function, because at this time the AiLuaComponent is not ready!",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			changeState =
			{
				type = "method",
				description = "Changes the current state name to a new one. Calles 'exit' function on the current state and 'enter' on the new state in lua script. Important: Do not use state function in 'connect' function, because at this time the AiLuaComponent is not ready!",
				args = "(Table newStateName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			revertToPreviousState =
			{
				type = "method",
				description = "Changes the current state name to the previous one. Calles 'exit' function on the current state and 'enter' on the previous state in lua script. Important: Do not use state function in 'connect' function, because at this time the AiLuaComponent is not ready!",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			},
			reactOnPathGoalReached =
			{
				type = "method",
				description = "Sets whether to react the agent reached the goal.",
				args = "(func closure, GameObject targetGameObject, string functionName, GameObject gameObject)",
				returns = "(nil)",
				valuetype = "nil"
			},
			reactOnAgentStuck =
			{
				type = "method",
				description = "Sets whether to react the agent got stuck.",
				args = "(func closure, GameObject targetGameObject, string functionName, GameObject gameObject)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	AiLuaGoalComponent =
	{
		type = "class",
		description = "Usage: Define lua script tables with the corresponding goals, composite goals and logic. Requirements: A kind of physics active component must exist and a LuaScriptComponent, which references the script file.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Activates the components behaviour, so that the lua script will be executed.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether the component behaviour is activated or not.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setRotationSpeed =
			{
				type = "method",
				description = "Sets the agent rotation speed. Default value is 0.1.",
				args = "(number speed)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRotationSpeed =
			{
				type = "function",
				description = "Gets the agent rotation speed. Default value is 0.1.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setFlyMode =
			{
				type = "method",
				description = "Sets whether to use fly mode (taking y-axis into account). Note: this can be used e.g. for birds flying around.",
				args = "(boolean flyMode)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isInFlyMode =
			{
				type = "function",
				description = "Gets whether the agent is in flying mode (taking y-axis into account).",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setRootGoalName =
			{
				type = "method",
				description = "Sets the root goal name, which will be loaded in lua script and executed.",
				args = "(string rootGoalName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRootGoalName =
			{
				type = "function",
				description = "Gets the root goal name, which is loaded in lua script and executed.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			getMovingBehavior =
			{
				type = "function",
				description = "Gets the moving behavior instance for this agent.",
				args = "()",
				returns = "(MovingBehavior)",
				valuetype = "MovingBehavior"
			},
			setRootGoal =
			{
				type = "method",
				description = "Sets the root goal for this AI component. Can be a LuaGoalComposite or any derived goal.",
				args = "(Table rootGoal)",
				returns = "(nil)",
				valuetype = "nil"
			},
			addSubGoal =
			{
				type = "method",
				description = "Adds a subgoal to the root goal. Can be a LuaGoal or LuaGoalComposite.",
				args = "(Table subGoal)",
				returns = "(nil)",
				valuetype = "nil"
			},
			reactOnPathGoalReached =
			{
				type = "method",
				description = "Sets whether to react the agent reached the goal.",
				args = "(func closure, GameObject targetGameObject, string functionName, GameObject gameObject)",
				returns = "(nil)",
				valuetype = "nil"
			},
			reactOnAgentStuck =
			{
				type = "method",
				description = "Sets whether to react the agent got stuck.",
				args = "(func closure, GameObject targetGameObject, string functionName, GameObject gameObject)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	AiMoveComponent =
	{
		type = "class",
		description = "Requirements: A kind of physics active component must exist. Usage: Move an agent according the specified behavior. Note: Several AiComponents for an agent can be combined. Requirements: A kind of physics active component must exist.",
		inherits = "AiComponent",
		childs = 
		{
			setBehaviorType =
			{
				type = "method",
				description = "Sets the behavior type what this ai component should do. Possible values are 'Move', 'Seek', 'Flee', 'Arrive', 'Pursuit', 'Evade'",
				args = "(string behaviorType)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getBehaviorType =
			{
				type = "function",
				description = "Gets the used behavior type. Possible values are 'Move', 'Seek', 'Flee', 'Arrive', 'Pursuit', 'Evade'.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setTargetId =
			{
				type = "method",
				description = "Sets the target game object id. This is used for 'Seek', 'Flee', 'Arrive', 'Pursuit', 'Evade'.",
				args = "(string targetId)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTargetId =
			{
				type = "function",
				description = "Gets the target game object id.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			}
		}
	},
	AiMoveComponent2D =
	{
		type = "class",
		description = "Requirements: A kind of physics active component must exist. Usage: Move an agent according the specified behavior. Note: Several AiComponent2Ds for an agent can be combined. Requirements: A kind of physics active component must exist.",
		childs = 
		{
			setBehaviorType =
			{
				type = "method",
				description = "Sets the behavior type what this ai component should do. Possible values are 'Seek2D', 'Flee2D', 'Arrive2D'",
				args = "(string behaviorType)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getBehaviorType =
			{
				type = "function",
				description = "Gets the used behavior type. Possible values are 'Seek2D', 'Flee2D', 'Arrive2D'.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setTargetId =
			{
				type = "method",
				description = "Sets the target game object id. This is used for 'Seek2D', 'Flee2D', 'Arrive2D'.",
				args = "(string targetId)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTargetId =
			{
				type = "function",
				description = "Gets the target game object id.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			}
		}
	},
	AiObstacleAvoidanceComponent =
	{
		type = "class",
		description = "Requirements: A kind of physics active component must exist. Usage: Move an agent and avoid obstacles. Note: Several AiComponents for an agent can be combined. Requirements: A kind of physics active component must exist.",
		inherits = "AiComponent",
		childs = 
		{
			setAvoidanceRadius =
			{
				type = "method",
				description = "Sets the radius, at which an obstacle is avoided.",
				args = "(number radius)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAvoidanceRadius =
			{
				type = "function",
				description = "Gets the radius, at which an obstacle is avoided.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setObstacleCategories =
			{
				type = "method",
				description = "Sets categories, that are considered as obstacle for hiding. Note: Combined categories can be used like 'ALL-Player' etc.",
				args = "(string categories)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getObstacleCategories =
			{
				type = "function",
				description = "Gets categories, that are considered as obstacle for hiding.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			}
		}
	},
	AiPathFollowComponent =
	{
		type = "class",
		description = "Requirements: A kind of physics active component must exist. Usage: Move an agent according a specfied waypoint path. The waypoints are created through GameObjects with NodeComponents and can be linked by their GameObject Ids. Note: Several AiComponents for an agent can be combined. Requirements: A kind of physics active component must exist.",
		inherits = "AiComponent",
		childs = 
		{
			setWaypointsCount =
			{
				type = "method",
				description = "Sets the way points count.",
				args = "(number count)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWaypointsCount =
			{
				type = "function",
				description = "Gets the way points count.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setWaypointId =
			{
				type = "method",
				description = "Sets the id of the GameObject with the NodeComponent for the given waypoint index.",
				args = "(number index, string id)",
				returns = "(nil)",
				valuetype = "nil"
			},
			addWaypointId =
			{
				type = "method",
				description = "Adds the id of the GameObject with the NodeComponent.",
				args = "(string id)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWaypointId =
			{
				type = "function",
				description = "Gets the way point id from the given index.",
				args = "(number index)",
				returns = "(string)",
				valuetype = "string"
			},
			setRepeat =
			{
				type = "method",
				description = "Sets whether to repeat the path, when the game object reached the last way point.",
				args = "(boolean repeat)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRepeat =
			{
				type = "function",
				description = "Gets whether to repeat the path, when the game object reached the last way point.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setDirectionChange =
			{
				type = "method",
				description = "Sets whether to change the direction of the path follow when the game object reached the last waypoint.",
				args = "(boolean directionChange)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDirectionChange =
			{
				type = "function",
				description = "Gets whether to change the direction of the path follow when the game object reached the last waypoint.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setGoalRadius =
			{
				type = "method",
				description = "Sets the goal radius at which the game object is considered within the next waypoint range. Default value is 0.2.",
				args = "(number radius)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getGoalRadius =
			{
				type = "function",
				description = "Gets the goal radius at which the game object is considered within the next waypoint range.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	AiPathFollowComponent2D =
	{
		type = "class",
		description = "Requirements: A kind of physics active component must exist. Usage: Move an agent according a specfied waypoint path. The waypoints are created through GameObjects with NodeComponent2Ds and can be linked by their GameObject Ids. Note: Several AiComponent2Ds for an agent can be combined. Requirements: A kind of physics active component must exist.",
		childs = 
		{
			setWaypointsCount =
			{
				type = "method",
				description = "Sets the way points count.",
				args = "(number count)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWaypointsCount =
			{
				type = "function",
				description = "Gets the way points count.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setWaypointId =
			{
				type = "method",
				description = "Sets the id of the GameObject with the NodeComponent for the given waypoint index.",
				args = "(number index, string id)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWaypointId =
			{
				type = "function",
				description = "Gets the way point id from the given index.",
				args = "(number index)",
				returns = "(string)",
				valuetype = "string"
			},
			setRepeat =
			{
				type = "method",
				description = "Sets whether to repeat the path, when the game object reached the last way point.",
				args = "(boolean repeat)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRepeat =
			{
				type = "function",
				description = "Gets whether to repeat the path, when the game object reached the last way point.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setDirectionChange =
			{
				type = "method",
				description = "Sets whether to change the direction of the path follow when the game object reached the last waypoint.",
				args = "(boolean directionChange)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDirectionChange =
			{
				type = "function",
				description = "Gets whether to change the direction of the path follow when the game object reached the last waypoint.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setGoalRadius =
			{
				type = "method",
				description = "Sets the goal radius at which the game object is considered within the next waypoint range. Default value is 0.2.",
				args = "(number radius)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getGoalRadius =
			{
				type = "function",
				description = "Gets the goal radius at which the game object is considered within the next waypoint range.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	AiRecastPathNavigationComponent =
	{
		type = "class",
		description = "Requirements: A kind of physics active component must exist. Usage: Move an agent along an optimal path generated by navigation mesh. Note: Several AiComponents for an agent can be combined. Requirements: A kind of physics active component must exist and a valid navigation mesh through OgreRecast.",
		inherits = "AiComponent",
		childs = 
		{
			showDebugData =
			{
				type = "method",
				description = "Shows the valid navigation area and the shortest path line from origin to goal.",
				args = "(boolean show)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setRepeat =
			{
				type = "method",
				description = "Sets whether to repeat the path, when the game object reached the last way point.",
				args = "(boolean repeat)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRepeat =
			{
				type = "function",
				description = "Gets whether to repeat the path, when the game object reached the last way point.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setDirectionChange =
			{
				type = "method",
				description = "Sets whether to change the direction of the path follow when the game object reached the last waypoint.",
				args = "(boolean directionChange)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDirectionChange =
			{
				type = "function",
				description = "Gets whether to change the direction of the path follow when the game object reached the last waypoint.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setGoalRadius =
			{
				type = "method",
				description = "Sets the goal radius at which the game object is considered within the next waypoint range. Default value is 0.2.",
				args = "(number radius)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getGoalRadius =
			{
				type = "function",
				description = "Gets the goal radius at which the game object is considered within the next waypoint range.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setTargetId =
			{
				type = "method",
				description = "Sets a target id game object as goal to go to walk to that game object on shortest path. Note: The target game object may move and the path is recalculated at @getActualizePathDelay rate.",
				args = "(string targetId)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTargetId =
			{
				type = "function",
				description = "Gets the target id, which this game object does walk to.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setActualizePathDelay =
			{
				type = "method",
				description = "Sets the path recalculation delay in milliseconds. This is the rate the path is recalculated due to performance reasons when the target id game object changes its position. Default is set to -1 (off)",
				args = "(number delayMS)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getActualizePathDelay =
			{
				type = "function",
				description = "Gets the path recalculation delay in milliseconds.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setPathSlot =
			{
				type = "method",
				description = "Sets path slot index. Note: This is necessary, since there can be several paths created for different slots.",
				args = "(number pathSlot)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPathSlot =
			{
				type = "function",
				description = "Gets the current path slot index.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setTargetPathSlot =
			{
				type = "method",
				description = "Sets target path slot index. Note: This is necessary, since there can be several paths created for different slots.",
				args = "(number targetPathSlot)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTargetPathSlot =
			{
				type = "function",
				description = "Gets the current target path slot index.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	AiWanderComponent =
	{
		type = "class",
		description = "Requirements: A kind of physics active component must exist. Usage: Let an agent wander randomly. Note: Several AiComponents for an agent can be combined. Requirements: A kind of physics active component must exist.",
		inherits = "AiComponent",
		childs = 
		{
			setWanderJitter =
			{
				type = "method",
				description = "Sets the wander jitter. That is how often the game object does change its direction. Default valus is 1.",
				args = "(number wanderJitter)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWanderJitter =
			{
				type = "function",
				description = "Gets the wander jitter.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setWanderRadius =
			{
				type = "method",
				description = "Sets wander radius. That is how much does the game object change its direction. Default valus is 1.2.",
				args = "(number wanderRadius)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWanderRadius =
			{
				type = "function",
				description = "Gets wander radius.",
				args = "(number index)",
				returns = "(number)",
				valuetype = "number"
			},
			setWanderDistance =
			{
				type = "method",
				description = "Sets the wander distance. That is how long does the game object follow a point until direction changes occur. Default valus is 20.",
				args = "(number wanderDistance)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWanderDistance =
			{
				type = "function",
				description = "Gets the wander distance.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	AiWanderComponent2D =
	{
		type = "class",
		description = "Requirements: A kind of physics active component must exist. Usage: Let an agent wander randomly. Note: Several AiComponent2Ds for an agent can be combined. Requirements: A kind of physics active component must exist.",
		childs = 
		{
			setWanderJitter =
			{
				type = "method",
				description = "Sets the wander jitter. That is how often the game object does change its direction. Default valus is 1.",
				args = "(number wanderJitter)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWanderJitter =
			{
				type = "function",
				description = "Gets the wander jitter.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setWanderRadius =
			{
				type = "method",
				description = "Sets wander radius. That is how much does the game object change its direction. Default valus is 1.2.",
				args = "(number wanderRadius)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWanderRadius =
			{
				type = "function",
				description = "Gets wander radius.",
				args = "(number index)",
				returns = "(number)",
				valuetype = "number"
			},
			setWanderDistance =
			{
				type = "method",
				description = "Sets the wander distance. That is how long does the game object follow a point until direction changes occur. Default valus is 20.",
				args = "(number wanderDistance)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWanderDistance =
			{
				type = "function",
				description = "Gets the wander distance.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	Align =
	{
		type = "class",
		description = "Align class for MyGUI widget positioning.",
		childs = 
		{
			H_CENTER =
			{
				type = "value"
			},
			V_CENTER =
			{
				type = "value"
			},
			CENTER =
			{
				type = "value"
			},
			LEFT =
			{
				type = "value"
			},
			RIGHT =
			{
				type = "value"
			},
			H_STRECH =
			{
				type = "value"
			},
			TOP =
			{
				type = "value"
			},
			Bottom =
			{
				type = "value"
			},
			V_STRETCH =
			{
				type = "value"
			},
			STRETCH =
			{
				type = "value"
			},
			DEFAULT =
			{
				type = "value"
			}
		}
	},
	AnimID =
	{
		type = "class",
		description = "AnimID class",
		childs = 
		{
			ANIM_IDLE_1 =
			{
				type = "value"
			},
			ANIM_IDLE_2 =
			{
				type = "value"
			},
			ANIM_IDLE_3 =
			{
				type = "value"
			},
			ANIM_IDLE_4 =
			{
				type = "value"
			},
			ANIM_IDLE_5 =
			{
				type = "value"
			},
			ANIM_WALK_NORTH =
			{
				type = "value"
			},
			ANIM_WALK_SOUTH =
			{
				type = "value"
			},
			ANIM_WALK_WEST =
			{
				type = "value"
			},
			ANIM_WALK_EAST =
			{
				type = "value"
			},
			ANIM_RUN =
			{
				type = "value"
			},
			ANIM_CLIMB =
			{
				type = "value"
			},
			ANIM_SNEAK =
			{
				type = "value"
			},
			ANIM_HANDS_CLOSED =
			{
				type = "value"
			},
			ANIM_HANDS_RELAXED =
			{
				type = "value"
			},
			ANIM_DRAW_WEAPON =
			{
				type = "value"
			},
			ANIM_SLICE_VERTICAL =
			{
				type = "value"
			},
			ANIM_SLICE_HORIZONTAL =
			{
				type = "value"
			},
			ANIM_JUMP_START =
			{
				type = "value"
			},
			ANIM_JUMP_LOOP =
			{
				type = "value"
			},
			ANIM_JUMP_END =
			{
				type = "value"
			},
			ANIM_HIGH_JUMP_END =
			{
				type = "value"
			},
			ANIM_JUMP_WALK =
			{
				type = "value"
			},
			ANIM_FALL =
			{
				type = "value"
			},
			ANIM_EAT_1 =
			{
				type = "value"
			},
			ANIM_EAT_2 =
			{
				type = "value"
			},
			ANIM_PICKUP_1 =
			{
				type = "value"
			},
			ANIM_PICKUP_2 =
			{
				type = "value"
			},
			ANIM_ATTACK_1 =
			{
				type = "value"
			},
			ANIM_ATTACK_2 =
			{
				type = "value"
			},
			ANIM_ATTACK_3 =
			{
				type = "value"
			},
			ANIM_ATTACK_4 =
			{
				type = "value"
			},
			ANIM_SWIM =
			{
				type = "value"
			},
			ANIM_THROW_1 =
			{
				type = "value"
			},
			ANIM_THROW_2 =
			{
				type = "value"
			},
			ANIM_DEAD_1 =
			{
				type = "value"
			},
			ANIM_DEAD_2 =
			{
				type = "value"
			},
			ANIM_DEAD_3 =
			{
				type = "value"
			},
			ANIM_SPEAK_1 =
			{
				type = "value"
			},
			ANIM_SPEAK_2 =
			{
				type = "value"
			},
			ANIM_SLEEP =
			{
				type = "value"
			},
			ANIM_DANCE =
			{
				type = "value"
			},
			ANIM_DUCK =
			{
				type = "value"
			},
			ANIM_CROUCH =
			{
				type = "value"
			},
			ANIM_HALT =
			{
				type = "value"
			},
			ANIM_ROAR =
			{
				type = "value"
			},
			ANIM_SIGH =
			{
				type = "value"
			},
			ANIM_GREETINGS =
			{
				type = "value"
			},
			ANIM_NO_IDEA =
			{
				type = "value"
			},
			ANIM_ACTION_1 =
			{
				type = "value"
			},
			ANIM_ACTION_2 =
			{
				type = "value"
			},
			ANIM_ACTION_3 =
			{
				type = "value"
			},
			ANIM_ACTION_4 =
			{
				type = "value"
			},
			ANIM_NONE =
			{
				type = "value"
			}
		}
	},
	AnimationBlender =
	{
		type = "class",
		description = "This class can be used for more complex animations and transitions between them.",
		childs = 
		{
			init1 =
			{
				type = "method",
				description = "Inits the animation blender, also sets and start the first current animation id.",
				args = "(AnimID animationId, boolean loop)",
				returns = "(nil)",
				valuetype = "nil"
			},
			init2 =
			{
				type = "method",
				description = "Inits the animation blender, also sets and start the first current animation name.",
				args = "(string animationName, boolean loop)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAllAvailableAnimationNames =
			{
				type = "function",
				description = "Gets all available animation names. A string list with all animation names. If none found, empty list will be delivered.",
				args = "(boolean skipLogging)",
				returns = "(Table[string])",
				valuetype = "Table[string]"
			},
			blend1 =
			{
				type = "method",
				description = "Blends from the current animation to the new animationId.",
				args = "(AnimID animationId, BlendingTransition transition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			blend2 =
			{
				type = "method",
				description = "Blends from the current animation to the new animation name.",
				args = "(string animationName, BlendingTransition transition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			blend3 =
			{
				type = "method",
				description = "Blends from the current animation to the new animationId and sets whether the animation should be looped.",
				args = "(AnimID animationId, BlendingTransition transition, boolean loop)",
				returns = "(nil)",
				valuetype = "nil"
			},
			blend4 =
			{
				type = "method",
				description = "Blends from the current animation to the new animation name and sets whether the animation should be looped.",
				args = "(string animationName, BlendingTransition transition, boolean loop)",
				returns = "(nil)",
				valuetype = "nil"
			},
			blend5 =
			{
				type = "method",
				description = "Blends to the given animation id with a transition. Sets how long the animation should be played and if the animation should be looped.",
				args = "(AnimID animationId, BlendingTransition blendingTransition, number duration, boolean loop)",
				returns = "(nil)",
				valuetype = "nil"
			},
			blend6 =
			{
				type = "method",
				description = "Blends to the given animation name with a transition. Sets how long the animation should be played and if the animation should be looped.",
				args = "(string animationName, BlendingTransition blendingTransition, number duration, boolean loop)",
				returns = "(nil)",
				valuetype = "nil"
			},
			blendAndContinue1 =
			{
				type = "method",
				description = "Blends to the given animation id with a transition. After the animation is finished, the previous one will continue.",
				args = "(AnimID animationId)",
				returns = "(nil)",
				valuetype = "nil"
			},
			blendAndContinue2 =
			{
				type = "method",
				description = "Blends to the given animation name with a transition. After the animation is finished, the previous one will continue.",
				args = "(string animationName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			blendAndContinue3 =
			{
				type = "method",
				description = "Blends to the given animation id with a transition. Sets how long the animation should be played. After the animation is finished, the previous one will continue.",
				args = "(AnimID animationId, number duration)",
				returns = "(nil)",
				valuetype = "nil"
			},
			blendAndContinue4 =
			{
				type = "method",
				description = "Blends to the given animation name with a transition. Sets how long the animation should be played. After the animation is finished, the previous one will continue.",
				args = "(string animationName, number duration)",
				returns = "(nil)",
				valuetype = "nil"
			},
			blendExclusive1 =
			{
				type = "method",
				description = "Blends to the given animation id with a transition exclusively. The animation will only be blend, if it is not active currently.",
				args = "(AnimID animationId, BlendingTransition blendingTransition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			blendExclusive2 =
			{
				type = "method",
				description = "Blends to the given animation name with a transition exclusively. The animation will only be blend, if it is not active currently.",
				args = "(string animationName, BlendingTransition blendingTransition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			blendExclusive3 =
			{
				type = "method",
				description = "Blends to the given animation id with a transition exclusively and optionally loops the animation. The animation will only be blend, if it is not active currently.",
				args = "(AnimID animationId, BlendingTransition blendingTransition, boolean loop)",
				returns = "(nil)",
				valuetype = "nil"
			},
			blendExclusive4 =
			{
				type = "method",
				description = "Blends to the given animation name with a transition exclusively and optionally loops the animation. The animation will only be blend, if it is not active currently.",
				args = "(string animationName, BlendingTransition blendingTransition, boolean loop)",
				returns = "(nil)",
				valuetype = "nil"
			},
			blendExclusive5 =
			{
				type = "method",
				description = "Blends to the given animation id with a transition exclusively. Sets how long the animation should be played. The animation will only be blend, if it is not active currently.",
				args = "(AnimID animationId, BlendingTransition blendingTransition, number duration)",
				returns = "(nil)",
				valuetype = "nil"
			},
			blendExclusive6 =
			{
				type = "method",
				description = "Blends to the given animation name with a transition exclusively. Sets how long the animation should be played. The animation will only be blend, if it is not active currently.",
				args = "(string animationName, BlendingTransition blendingTransition, number duration)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getProgress =
			{
				type = "function",
				description = "Gets the progress of the currently played animation in ms.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			isCompleted =
			{
				type = "function",
				description = "Gets whether the currently played animation has completed or not.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			registerAnimation =
			{
				type = "method",
				description = "Registers the animation name and maps it with the given animation id.",
				args = "(string animationName, AnimID animationId)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAnimationIdFromString =
			{
				type = "function",
				description = "Gets the mapped animation id from the given animation name.",
				args = "(string animationName)",
				returns = "(AnimID)",
				valuetype = "AnimID"
			},
			hasAnimation =
			{
				type = "function",
				description = "Gets whether the given animation name does exist.",
				args = "(string animationName)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			isAnimationActive =
			{
				type = "function",
				description = "Gets whether the given animation name is being currently played.",
				args = "(string animationName)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			isAnyAnimationActive =
			{
				type = "function",
				description = "Gets whether any animation is currently played.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setTimePosition =
			{
				type = "method",
				description = "Sets the time position for the animation.",
				args = "(number timePosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTimePosition =
			{
				type = "function",
				description = "Gets the current animation time position.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getLength =
			{
				type = "function",
				description = "Gets the animation length.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setWeight =
			{
				type = "method",
				description = "Sets the animation weight. The more less the weight the more less all bones are moved",
				args = "(number weight)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWeight =
			{
				type = "function",
				description = "Gets the current animation weight.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	AnimationComponent =
	{
		type = "class",
		description = "Usage: Play one animation. Requirements: Entity must have a skeleton with animations.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets whether this component should be activated or not (Start the animations).",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this component is activated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setAnimationName =
			{
				type = "method",
				description = "Sets the to be played animation name. If it does not exist, the animation cannot be played later.",
				args = "(string animationName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAnimationName =
			{
				type = "function",
				description = "Gets currently used animation name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setSpeed =
			{
				type = "method",
				description = "Sets the animation speed for the current animation.",
				args = "(number speed)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSpeed =
			{
				type = "function",
				description = "Gets the animation speed for currently used animation.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setRepeat =
			{
				type = "method",
				description = "Sets whether the current animation should be repeated when finished.",
				args = "(boolean repeat)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRepeat =
			{
				type = "function",
				description = "Gets whether the current animation will be repeated when finished.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			isComplete =
			{
				type = "function",
				description = "Gets whether the current animation has finished.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			getAnimationBlender =
			{
				type = "function",
				description = "Gets animation blender to manipulate animations directly.",
				args = "()",
				returns = "(AnmationBlender)",
				valuetype = "AnmationBlender"
			},
			getBone =
			{
				type = "function",
				description = "Gets the bone by the given bone name for direct manipulation. Nil is delivered, if the bone name does not exist.",
				args = "(string boneName)",
				returns = "(Bone)",
				valuetype = "Bone"
			},
			setTimePosition =
			{
				type = "method",
				description = "Sets the time position for the animation.",
				args = "(number timePosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTimePosition =
			{
				type = "function",
				description = "Gets the current animation time position.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getLength =
			{
				type = "function",
				description = "Gets the animation length.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setWeight =
			{
				type = "method",
				description = "Sets the animation weight. The more less the weight the more less all bones are moved.",
				args = "(number weight)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWeight =
			{
				type = "function",
				description = "Gets the current animation weight.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			reactOnAnimationFinished =
			{
				type = "method",
				description = "Sets whether to react when the given animation has finished.",
				args = "(func closureFunction, boolean oneTime)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	AnimationComponentV2 =
	{
		type = "class",
		description = "Usage: Play one animation with Ogre Item (v2). Requirements: Item must have a skeleton with animations.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets whether this component should be activated or not.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this component is activated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			reactOnAnimationFinished =
			{
				type = "method",
				description = "Sets whether to react when the given animation has finished.",
				args = "(func closureFunction, boolean oneTime)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAnimationBlender =
			{
				type = "function",
				description = "Gets the animation blender for direction manipulation.",
				args = "()",
				returns = "(AnimationBlender)",
				valuetype = "AnimationBlender"
			}
		}
	},
	AnimationSequenceComponent =
	{
		type = "class",
		description = "Its possible to add several animations which are played in sequence, triggered by transitions. Requirements: Entity must have animations.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets whether this component should be activated or not (Start the animations).",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this component is activated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			getAnimationCount =
			{
				type = "function",
				description = "Gets the number of used animations.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setBlendTransition =
			{
				type = "method",
				description = "Sets the blending transition for the given animation by index. Possible values are: 'BlendWhileAnimating', 'BlendSwitch', 'BlendThenAnimate'",
				args = "(number index, string strBlendTransition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getBlendTransition =
			{
				type = "function",
				description = "Gets the blending transition for the given animation by index. Possible values are: 'BlendWhileAnimating', 'BlendSwitch', 'BlendThenAnimate'",
				args = "(number index)",
				returns = "(string)",
				valuetype = "string"
			},
			setDuration =
			{
				type = "method",
				description = "Sets the duration in seconds for the given animation by index.",
				args = "(number index, number duration)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDuration =
			{
				type = "function",
				description = "Gets the duration in seconds for the given animation by index.",
				args = "(number index)",
				returns = "(number)",
				valuetype = "number"
			},
			setTimePosition =
			{
				type = "method",
				description = "Sets the time position in seconds for the given animation by index.",
				args = "(number index, number timePosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTimePosition =
			{
				type = "function",
				description = "Gets the time position in seconds for the given animation by index.",
				args = "(number index)",
				returns = "(number)",
				valuetype = "number"
			},
			setSpeed =
			{
				type = "method",
				description = "Sets the speed for the given animation by index.",
				args = "(number index, number speed)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSpeed =
			{
				type = "function",
				description = "Gets the speed for the given animation by index.",
				args = "(number index)",
				returns = "(number)",
				valuetype = "number"
			},
			setRepeat =
			{
				type = "method",
				description = "Sets whether to repeat the whole sequence, after it has been played.",
				args = "(boolean repeat)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRepeat =
			{
				type = "function",
				description = "Gets whether the whole sequence is repeated, after it has been played.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			getAnimationBlender =
			{
				type = "function",
				description = "Gets animation blender to manipulate animations directly.",
				args = "()",
				returns = "(AnmationBlender)",
				valuetype = "AnmationBlender"
			},
			getBone =
			{
				type = "function",
				description = "Gets the bone by the given bone name for direct manipulation. Nil is delivered, if the bone name does not exist.",
				args = "(string boneName)",
				returns = "(Bone)",
				valuetype = "Bone"
			}
		}
	},
	AnimationState =
	{
		type = "class",
		description = "The v1 animation state.",
		childs = 
		{
			getTimePosition =
			{
				type = "function",
				description = "Gets the current time position of this animation state.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getLength =
			{
				type = "function",
				description = "Gets the animation length.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setTimePosition =
			{
				type = "method",
				description = "Sets the time position of this animation state.",
				args = "(number timePosition)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	AppStateManager =
	{
		type = "class",
		description = "Some functions for NOWA game loop and applictation state management. That is, normally only one scene can be running at a time. But putting a scene into an own state. Its possible to push another scene at the top of the current scene. Like pushing 'MenuState', so that the user can configure something and when he is finished, he pops the 'MenuState', so that the 'GameState' will continue.",
		childs = 
		{
			setSlowMotion =
			{
				type = "method",
				description = "Sets whether to use slow motion rendering.",
				args = "(boolean slowMotion)",
				returns = "(nil)",
				valuetype = "nil"
			},
			changeAppState =
			{
				type = "method",
				description = "Changes this application state to the new given one. Details: Calls 'exit' on the prior state and 'enter' on the new state.",
				args = "(string stateName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			pushAppState =
			{
				type = "function",
				description = "Pushes a new application state at the top of this one and calls 'enter'.",
				args = "(string stateName)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			popAppState =
			{
				type = "function",
				description = "Pops the current state (calls 'exit'), and calls 'resume' on a prior state. If no state does exist anymore, the appliction will be shut down.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			popAllAndPushAppState =
			{
				type = "method",
				description = "Pops all actives application states (internally calls 'exit' for all active states and pushes a new one and calls 'enter'.",
				args = "(string stateName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			hasAppStateStarted =
			{
				type = "function",
				description = "Gets whether the given AppState has started. Note: This can be used to ask from another AppState like a MenuState, whether a GameState has already started, so that e.g. a continue button can be shown in the Menu.",
				args = "(string stateName)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			exitGame =
			{
				type = "method",
				description = "Exits the game and destroys all application states.",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCameraManager =
			{
				type = "function",
				description = "Gets the module for the current AppState.",
				args = "()",
				returns = "(CameraManagerModule)",
				valuetype = "CameraManagerModule"
			},
			getCameraManager2 =
			{
				type = "function",
				description = "Gets the module for the given application state name, or null if the application state does not exist.",
				args = "(string stateName)",
				returns = "(CameraManagerModule)",
				valuetype = "CameraManagerModule"
			},
			getGameObjectController =
			{
				type = "function",
				description = "Gets the module for the current AppState.",
				args = "()",
				returns = "(GameObjectController)",
				valuetype = "GameObjectController"
			},
			getGameObjectController2 =
			{
				type = "function",
				description = "Gets the module for the given application state name, or null if the application state does not exist.",
				args = "(string stateName)",
				returns = "(GameObjectController)",
				valuetype = "GameObjectController"
			},
			getGameProgressModule =
			{
				type = "function",
				description = "Gets the module for the current AppState.",
				args = "()",
				returns = "(GameProgressModule)",
				valuetype = "GameProgressModule"
			},
			getGameProgressModule2 =
			{
				type = "function",
				description = "Gets the module for the given application state name, or null if the application state does not exist.",
				args = "(string stateName)",
				returns = "(GameProgressModule)",
				valuetype = "GameProgressModule"
			},
			getOgreNewtModule =
			{
				type = "function",
				description = "Gets the module for the current AppState.",
				args = "()",
				returns = "(OgreNewtModule)",
				valuetype = "OgreNewtModule"
			},
			getOgreNewtModule2 =
			{
				type = "function",
				description = "Gets the module for the given application state name, or null if the application state does not exist.",
				args = "(string stateName)",
				returns = "(OgreNewtModule)",
				valuetype = "OgreNewtModule"
			},
			getScriptEventManager =
			{
				type = "function",
				description = "Gets the script event manager for the current AppState.",
				args = "()",
				returns = "(ScriptEventManager)",
				valuetype = "ScriptEventManager"
			},
			getScriptEventManager2 =
			{
				type = "function",
				description = "Gets the module for the given application state name, or null if the application state does not exist.",
				args = "(string stateName)",
				returns = "(ScriptEventManager)",
				valuetype = "ScriptEventManager"
			}
		}
	},
	AreaOfInterestComponent =
	{
		type = "class",
		description = "Usage: Triggers when GameObjects from the given category (Can also be combined categories, or all) come into the radius of this GameObject. Note: This component can also be used in conjunction with a LuaScriptComponent, in which the corresponding methods: onEnter(gameObject) and onLeave(gameObject) are triggered. Requirements: A kind of physics active component must exist and a LuaScriptComponent, which references the script file.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets whether this component should be activated or not (Start checking for objects within area radius).",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this component is activated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setRadius =
			{
				type = "method",
				description = "Sets the area check radius in meters.",
				args = "(number radius)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRadius =
			{
				type = "function",
				description = "Gets the area check radius in meters.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			reactOnEnter =
			{
				type = "method",
				description = "Sets whether to react at the moment when a game object enters the area.",
				args = "(func closure, otherGameObject)",
				returns = "(nil)",
				valuetype = "nil"
			},
			reactOnLeave =
			{
				type = "method",
				description = "Sets whether to react at the moment when a game object leaves the area. Always check if the game object does exist. It may also be null.",
				args = "(func closure, otherGameObject)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	AtmosphereComponent =
	{
		type = "class",
		description = "Usage: Shows an atmospheric effect on the scene. The technique is called Atmosphere NPR(non - physically - based - render) Component. Fog is also applied to objects based on the atmosphere.It supports blending between multiple presets at different Times of Day (day & night).This allows fine tunning the brightness at different timedays, specially the fake GImultipliers in LDR.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "If activated, the atmospheric effects will take place.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this component is activated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setEnableSky =
			{
				type = "method",
				description = "Sets whether sky is enabled and visible.",
				args = "(boolean enableSky)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getEnableSky =
			{
				type = "function",
				description = "Gets whether sky is enabled and visible.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setTimeMultiplicator =
			{
				type = "method",
				description = "Sets the time multiplier. How long a day lasts. Default value is 1. Setting e.g. to 2, the day goes by twice as fast. Range [0.1; 5].",
				args = "(number timeMultiplicator)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTimeMultiplicator =
			{
				type = "function",
				description = "Gets the time multiplier. How long a day lasts. Default value is 1. Setting e.g. to 2, the day goes by twice as fast. Range [0.1; 5].",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setPresetCount =
			{
				type = "method",
				description = "Sets the count of the atmosphere presets.",
				args = "(number presetCount)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPresetCount =
			{
				type = "function",
				description = "Gets the count of the atmosphere presets.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setTime =
			{
				type = "method",
				description = "Sets the time for the preset in the format hh:mm, e.g. 16:45. The time must be in range [0; 24[.",
				args = "(number index, ´string time)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTime =
			{
				type = "function",
				description = "Gets the time for the preset in the format hh:mm, e.g. 16:45. The time must be in range [0; 24[.",
				args = "(number index)",
				returns = "(string)",
				valuetype = "string"
			},
			setDensityCoefficient =
			{
				type = "method",
				description = " Sets the time for the preset in the format hh:mm, e.g. 16:45. The time must be in range [0; 24[.",
				args = "(number index, number densityCoefficient)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDensityCoefficient =
			{
				type = "function",
				description = "Gets the normalized time for the preset. The time must be in range [-1; 1] where range [-1; 0] is night and [0; 1] is day.",
				args = "(number index)",
				returns = "(number)",
				valuetype = "number"
			},
			setDensityDiffusion =
			{
				type = "method",
				description = "Sets the density diffusion for the preset. Valid values are in range [0; 1].",
				args = "(number index, number densityDiffusion)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDensityDiffusion =
			{
				type = "function",
				description = "Gets the density diffusion for the preset. Valid values are in range [0; 1]..",
				args = "(number index)",
				returns = "(number)",
				valuetype = "number"
			},
			setHorizonLimit =
			{
				type = "method",
				description = "Sets the horizon limit for the preset. Valid values are in range [0; 1]. Most relevant in sunsets and sunrises.",
				args = "(number index, number horizonLimit)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getHorizonLimit =
			{
				type = "function",
				description = "Gets the horizon limit for the preset. Valid values are in range [0; 1]. Most relevant in sunsets and sunrises.",
				args = "(number index)",
				returns = "(number)",
				valuetype = "number"
			},
			setSunPower =
			{
				type = "method",
				description = "Sets the sun power for the preset. Valid values are in range [0; ]. For HDR (affects the sun on the sky).",
				args = "(number index, number sunPower)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSunPower =
			{
				type = "function",
				description = "Gets the sun power for the preset. Valid values are in range [0; ]. For HDR (affects the sun on the sky).",
				args = "(number index)",
				returns = "(number)",
				valuetype = "number"
			},
			setSkyPower =
			{
				type = "method",
				description = "Sets the sky power for the preset. Valid values are in range [0; 100). For HDR (affects skyColour). Sky must be enabled.",
				args = "(number index, number skyPower)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSkyPower =
			{
				type = "function",
				description = "Gets the sky power for the preset. Valid values are in range [0; 100). For HDR (affects skyColour). Sky must be enabled.",
				args = "(number index)",
				returns = "(number)",
				valuetype = "number"
			},
			setSkyColor =
			{
				type = "method",
				description = "Sets the sky color for the preset. In range [0; 1], [0; 1], [0; 1]. Sky must be enabled.",
				args = "(number index, Vector3 skyColor)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSkyColor =
			{
				type = "function",
				description = "Gets the sky color for the preset. In range [0; 1], [0; 1], [0; 1]. Sky must be enabled.",
				args = "(number index)",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setFogDensity =
			{
				type = "method",
				description = "Sets the fog density for the preset. In range [0; 1]. Affects objects' fog (not sky).",
				args = "(number index, number fogDensity)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getFogDensity =
			{
				type = "function",
				description = "Gets the fog density for the preset. In range [0; 1]. Affects objects' fog (not sky).",
				args = "(number index)",
				returns = "(number)",
				valuetype = "number"
			},
			setFogBreakMinBrightness =
			{
				type = "method",
				description = "Sets the fog break min brightness for the preset. Very bright objects (i.e. reflect lots of light) manage to 'breakthrough' the fog.A value of fogBreakMinBrightness = 3 means that pixels that have a luminance of >= 3 (i.e.in HDR) will start becoming more visible.Range: [0; 100).",
				args = "(number index, number fogBreakMinBrightness)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getFogBreakMinBrightness =
			{
				type = "function",
				description = "Gets the fog break min brightness for the preset. Very bright objects (i.e. reflect lots of light) manage to 'breakthrough' the fog.A value of fogBreakMinBrightness = 3 means that pixels that have a luminance of >= 3 (i.e.in HDR) will start becoming more visible.Range: [0; 100).",
				args = "(number index)",
				returns = "(number)",
				valuetype = "number"
			},
			setFogBreakFalloff =
			{
				type = "method",
				description = "Sets the fog break falloff for the preset. How fast bright objects appear in fog.Small values make objects appear very slowly after luminance > fogBreakMinBrightness. Large values make objects appear very quickly. Range: (0; 100).",
				args = "(number index, number fogBreakFalloff)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getFogBreakFalloff =
			{
				type = "function",
				description = "Gets the fog break falloff for the preset. How fast bright objects appear in fog.Small values make objects appear very slowly after luminance > fogBreakMinBrightness. Large values make objects appear very quickly. Range: (0; 100).",
				args = "(number index)",
				returns = "(number)",
				valuetype = "number"
			},
			setLinkedLightPower =
			{
				type = "method",
				description = "Sets the linked light power for the preset. Power scale for the linked light. Range: (0; 100).",
				args = "(number index, number linkedLightPower)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getLinkedLightPower =
			{
				type = "function",
				description = "Gets the linked light power for the preset. Power scale for the linked light. Range: (0; 100).",
				args = "(number index)",
				returns = "(number)",
				valuetype = "number"
			},
			setLinkedAmbientUpperPower =
			{
				type = "method",
				description = "Sets the linked ambient upper power for the preset. Power scale for the upper hemisphere ambient light. Range: (0; 100).",
				args = "(number index, number linkedAmbientUpperPower)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getLinkedAmbientUpperPower =
			{
				type = "function",
				description = "Gets the linked ambient upper power for the preset. Power scale for the upper hemisphere ambient light. Range: (0; 100).",
				args = "(number index)",
				returns = "(number)",
				valuetype = "number"
			},
			setLinkedAmbientLowerPower =
			{
				type = "method",
				description = "Sets the linked ambient lower power for the preset. Power scale for the lower hemisphere ambient light. Range: (0; 100).",
				args = "(number index, number linkedAmbientLowerPower)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getLinkedAmbientLowerPower =
			{
				type = "function",
				description = "Gets the linked ambient lower power for the preset. Power scale for the lower hemisphere ambient light. Range: (0; 100).",
				args = "(number index)",
				returns = "(number)",
				valuetype = "number"
			},
			setEnvmapScale =
			{
				type = "method",
				description = "Sets the envmap scale for the preset. Value to send to SceneManager::setAmbientLight. Range: (0; 1].",
				args = "(number index, number envmapScale)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getEnvmapScale =
			{
				type = "function",
				description = "Gets the envmap scale for the preset. Value to send to SceneManager::setAmbientLight. Range: (0; 1].",
				args = "(number index)",
				returns = "(number)",
				valuetype = "number"
			},
			getCurrentTimeOfDay =
			{
				type = "function",
				description = "Gets current time of day as string.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			getCurrentTimeOfDayMinutes =
			{
				type = "function",
				description = "Gets current time of day in minutes.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setStartTime =
			{
				type = "method",
				description = "Sets the start time in format hh:mm e.g. 03:15 is at night and 13:30 is at day and 23:59 is midnight.",
				args = "(string startTime)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getStartTime =
			{
				type = "function",
				description = "Gets the start time in format hh:mm e.g. 03:15 is at night and 13:30 is at day and 23:59 is midnight..",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			}
		}
	},
	AttributeEffectComponent =
	{
		type = "class",
		description = "Usage: Manipulates any game object or component's attribute by the given mathematical functions. Important: It always just works for the next prior component! So place it beyond the the be manipulated component!",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets whether this component should be activated or not (Start math function calculation).",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this component is activated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			reactOnEndOfEffect =
			{
				type = "method",
				description = "Sets whether the target game object should be notified at the end of the attribute effect. One time means, that the nofication is done only once.",
				args = "(func closureFunction, number notificationValue, boolean oneTime)",
				returns = "(nil)",
				valuetype = "nil"
			},
			reactOnEndOfEffect =
			{
				type = "method",
				description = "Sets whether the target game object should be notified at the end of the attribute effect. One time means, that the nofication is done only once.",
				args = "(func closureFunction, boolean oneTime)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	AttributesComponent =
	{
		type = "class",
		description = "Usage: A GameObject may have custom attributes like energy, coins etc. These attributes can also be used in lua scripts. They can also be saved and loaded.",
		inherits = "GameObjectComponent",
		childs = 
		{
			getAttributeValueByIndex =
			{
				type = "function",
				description = "Gets the attribute value as variant by the given index which corresponds the occurence in the attributes component.",
				args = "(number index)",
				returns = "(Variant)",
				valuetype = "Variant"
			},
			getAttributeValueByName =
			{
				type = "function",
				description = "Gets the attribute value as variant by the given name.",
				args = "(string name)",
				returns = "(Variant)",
				valuetype = "Variant"
			},
			getAttributeValueBool =
			{
				type = "function",
				description = "Gets the bool value by the given index which corresponds the occurence in the attributes component.",
				args = "(number index)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			getAttributeValueNumber =
			{
				type = "function",
				description = "Gets the number value by the given index which corresponds the occurence in the attributes component.",
				args = "(number index)",
				returns = "(number)",
				valuetype = "number"
			},
			getAttributeValueString =
			{
				type = "function",
				description = "Gets the string value by the given index which corresponds the occurence in the attributes component.",
				args = "(number index)",
				returns = "(string)",
				valuetype = "string"
			},
			getAttributeValueId =
			{
				type = "function",
				description = "Gets the id value by the given index which corresponds the occurence in the attributes component.",
				args = "(number index)",
				returns = "(string)",
				valuetype = "string"
			},
			getAttributeValueVector2 =
			{
				type = "function",
				description = "Gets the Vector2 value by the given index which corresponds the occurence in the attributes component.",
				args = "(number index)",
				returns = "(Vector2)",
				valuetype = "Vector2"
			},
			getAttributeValueVector3 =
			{
				type = "function",
				description = "Gets the Vector3 value by the given index which corresponds the occurence in the attributes component.",
				args = "(number index)",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			getAttributeValueVector4 =
			{
				type = "function",
				description = "Gets the Vector4 value by the given index which corresponds the occurence in the attributes component.",
				args = "(number index)",
				returns = "(Vector4)",
				valuetype = "Vector4"
			},
			getAttributeValueBool =
			{
				type = "function",
				description = "Gets the bool value by the given name",
				args = "(string name)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			getAttributeValueNumber =
			{
				type = "function",
				description = "Gets the number value by the given name",
				args = "(string name)",
				returns = "(number)",
				valuetype = "number"
			},
			getAttributeValueString =
			{
				type = "function",
				description = "Gets the string value by the given name",
				args = "(string name)",
				returns = "(string)",
				valuetype = "string"
			},
			getAttributeValueId =
			{
				type = "function",
				description = "Gets the id value by the given name",
				args = "(string name)",
				returns = "(string)",
				valuetype = "string"
			},
			getAttributeValueVector2 =
			{
				type = "function",
				description = "Gets the Vector2 value by the given name",
				args = "()",
				returns = "(Vector2)",
				valuetype = "Vector2"
			},
			getAttributeValueVector3 =
			{
				type = "function",
				description = "Gets the Vector3 value by the given name",
				args = "(string name)",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			getAttributeValueVector4 =
			{
				type = "function",
				description = "Gets the Vector4 value by the given name",
				args = "(string name)",
				returns = "(Vector4)",
				valuetype = "Vector4"
			},
			addAttributeBool =
			{
				type = "method",
				description = "Adds an attribute bool with the name and the given value.",
				args = "(string name, boolean value)",
				returns = "(nil)",
				valuetype = "nil"
			},
			addAttributeNumber =
			{
				type = "method",
				description = "Adds an attribute number with the name and the given value.",
				args = "(string name, number value)",
				returns = "(nil)",
				valuetype = "nil"
			},
			addAttributeString =
			{
				type = "method",
				description = "Adds an attribute string with the name and the given value.",
				args = "(string name, string value)",
				returns = "(nil)",
				valuetype = "nil"
			},
			addAttributeId =
			{
				type = "method",
				description = "Adds an attribute id with the name and the given value.",
				args = "(string name, string id)",
				returns = "(nil)",
				valuetype = "nil"
			},
			addAttributeVector2 =
			{
				type = "method",
				description = "Adds an attribute Vector2 with the name and the given value.",
				args = "(string name, Vector2 value)",
				returns = "(nil)",
				valuetype = "nil"
			},
			addAttributeVector3 =
			{
				type = "method",
				description = "Adds an attribute Vector3 with the name and the given value.",
				args = "(string name, Vector3 value)",
				returns = "(nil)",
				valuetype = "nil"
			},
			addAttributeVector4 =
			{
				type = "method",
				description = "Adds an attribute Vector4 with the name and the given value.",
				args = "(string name, Vector4 value)",
				returns = "(nil)",
				valuetype = "nil"
			},
			changeValueBool =
			{
				type = "method",
				description = "Changes the attribute bool with the name and the given value.",
				args = "(string name, boolean value)",
				returns = "(nil)",
				valuetype = "nil"
			},
			changeValueNumber =
			{
				type = "method",
				description = "Changes the attribute number with the name and the given value.",
				args = "(string name, number value)",
				returns = "(nil)",
				valuetype = "nil"
			},
			changeValueString =
			{
				type = "method",
				description = "Changes the attribute string with the name and the given value.",
				args = "(string name, string value)",
				returns = "(nil)",
				valuetype = "nil"
			},
			changeValueId =
			{
				type = "method",
				description = "Changes the attribute id with the name and the given value.",
				args = "(string name, string id)",
				returns = "(nil)",
				valuetype = "nil"
			},
			changeValueVector2 =
			{
				type = "method",
				description = "Changes the attribute Vector2 with the name and the given value.",
				args = "(string name, Vector2 value)",
				returns = "(nil)",
				valuetype = "nil"
			},
			changeValueVector3 =
			{
				type = "method",
				description = "Changes the attribute Vector3 with the name and the given value.",
				args = "(string name, Vector3 value)",
				returns = "(nil)",
				valuetype = "nil"
			},
			changeValueVector4 =
			{
				type = "method",
				description = "Changes the attribute Vector4 with the name and the given value.",
				args = "(string name, Vector4 value)",
				returns = "(nil)",
				valuetype = "nil"
			},
			saveValues =
			{
				type = "method",
				description = "Saves all values from this component. Note: if crypted is set to true, the content will by set in a not readable form.",
				args = "(string saveName, boolean crypted)",
				returns = "(nil)",
				valuetype = "nil"
			},
			saveValue =
			{
				type = "method",
				description = "Save the value by the given index. Note: if crypted is set to true, the content will by set in a not readable form.",
				args = "(string saveName, number index, boolean crypted)",
				returns = "(nil)",
				valuetype = "nil"
			},
			saveValue2 =
			{
				type = "method",
				description = "Save the value by the given name. Note: if crypted is set to true, the content will by set in a not readable form.",
				args = "(string saveName, string name, boolean crypted)",
				returns = "(nil)",
				valuetype = "nil"
			},
			loadValues =
			{
				type = "function",
				description = "Loads all values for this component.",
				args = "(string saveName)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			loadValue =
			{
				type = "function",
				description = "Loads the value by the given index.",
				args = "(string saveName, number index)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			loadValue2 =
			{
				type = "function",
				description = "Loads the value by the given name.",
				args = "(string saveName, string name)",
				returns = "(boolean)",
				valuetype = "boolean"
			}
		}
	},
	AudioProcessor =
	{
		type = "class",
		description = "The audio processor for spectrum analysis."
	},
	Axis =
	{
		type = "class",
		description = "Axis of OIS mouse.",
		childs = 
		{
			abs =
			{
				type = "value"
			},
			rel =
			{
				type = "value"
			}
	},	},
	BehaviorType =
	{
		type = "class",
		description = "BehaviorType class",
		childs = 
		{
			STOP =
			{
				type = "value"
			},
			NONE =
			{
				type = "value"
			},
			MOVE =
			{
				type = "value"
			},
			MOVE_RANDOMLY =
			{
				type = "value"
			},
			SEEK =
			{
				type = "value"
			},
			FLEE =
			{
				type = "value"
			},
			ARRIVE =
			{
				type = "value"
			},
			WANDER =
			{
				type = "value"
			},
			PATH_FINDING_WANDER =
			{
				type = "value"
			},
			PURSUIT =
			{
				type = "value"
			},
			EVADE =
			{
				type = "value"
			},
			HIDE =
			{
				type = "value"
			},
			FOLLOW_PATH =
			{
				type = "value"
			},
			OBSTACLE_AVOIDANCE =
			{
				type = "value"
			},
			FLOCKING_COHESION =
			{
				type = "value"
			},
			FLOCKING_SEPARATION =
			{
				type = "value"
			},
			FLOCKING_ALIGNMENT =
			{
				type = "value"
			},
			FLOCKING_OBSTACLE_AVOIDANCE =
			{
				type = "value"
			},
			FLOCKING_FLEE =
			{
				type = "value"
			},
			FLOCKING_SEEK =
			{
				type = "value"
			},
			FLOCKING =
			{
				type = "value"
			},
			SEEK_2D =
			{
				type = "value"
			},
			FLEE_2D =
			{
				type = "value"
			},
			ARRIVE_2D =
			{
				type = "value"
			},
			PATROL_2D =
			{
				type = "value"
			},
			WANDER_2D =
			{
				type = "value"
			},
			FOLLOW_PATH_2D =
			{
				type = "value"
			}
		}
	},
	BezierCurve2 =
	{
		type = "class",
		description = "Generates a procedural bezier curve spline2."
	},
	BezierCurve3 =
	{
		type = "class",
		description = "Generates a procedural bezier curve spline3."
	},
	BillboardComponent =
	{
		type = "class",
		description = "Usage: Creates billboard effect.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets whether this billboard is activated or not.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this billboard is activated or not.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setDatablockName =
			{
				type = "method",
				description = "Sets the data block name to used for the billboard representation. Note: It must be a unlit datablock.",
				args = "(string datablockName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDatablockName =
			{
				type = "function",
				description = "Gets the used datablock name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setPosition =
			{
				type = "method",
				description = "Sets relative position offset where the billboard should be painted.",
				args = "(Vector3 position)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPosition =
			{
				type = "function",
				description = "Gets the relative position offset of the billboard.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setDimensions =
			{
				type = "method",
				description = "Sets the dimensions (size x, y) of the billboard.",
				args = "(Vector2 dimensions)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDimensions =
			{
				type = "function",
				description = "Gets the dimensions of the billboard.",
				args = "()",
				returns = "(Vecto2)",
				valuetype = "Vecto2"
			},
			setColor =
			{
				type = "method",
				description = "Sets the color (r, g, b) of the billboard.",
				args = "(Vector3 color)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	BlendingTransition =
	{
		type = "class",
		description = "BlendingTransition class",
		childs = 
		{
			BLEND_SWITCH =
			{
				type = "value"
			},
			BLEND_WHILE_ANIMATING =
			{
				type = "value"
			},
			BLEND_THEN_ANIMATE =
			{
				type = "value"
			}
		}
	},
	Body =
	{
		type = "class",
		description = "OgreNewt physics body."
	},
	Boolean =
	{
		type = "class",
		description = "Uses a boolean operation on two procedural meshes."
	},
	BoxGenerator =
	{
		type = "class",
		description = "Generates a procedural mesh box.",
		childs = 
		{
			setEnableNormals =
			{
				type = "method",
				description = "Sets whether normals are enabled.",
				args = "(boolean enableNormals)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setNumTexCoordSet =
			{
				type = "method",
				description = "Sets the number of texture coordinate sets.",
				args = "(number numTexCoordSet)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setSwitchUV =
			{
				type = "method",
				description = "Sets whether the uv coordinates should be switched.",
				args = "(boolean switchUV)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	BoxUVModifier =
	{
		type = "class",
		description = "Modifies UV on a procedural box."
	},
	Button =
	{
		type = "class",
		description = "MyGUI button class.",
		childs = 
		{
			setStateCheck =
			{
				type = "method",
				description = "Sets whether the button is checked (check box functionality)",
				args = "(boolean check)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getStateCheck =
			{
				type = "function",
				description = "Gets whether the button is checked (check box functionality)",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			}
		}
	},
	CalculateNormalsModifier =
	{
		type = "class",
		description = "Calculates and modifies normals on a procedural mesh."
	},
	Camera =
	{
		type = "class",
		description = "A viewpoint from which the scene will be rendered.",
		childs = 
		{
			setPosition =
			{
				type = "method",
				description = "Sets the position of the camera.",
				args = "(Vector3 vector)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setPosition =
			{
				type = "method",
				description = "Sets the position of the camera.",
				args = "(number x, number y, number z)",
				returns = "(nil)",
				valuetype = "nil"
			},
			lookAt =
			{
				type = "method",
				description = "Looks at the given vector.",
				args = "(Vector3 vector)",
				returns = "(nil)",
				valuetype = "nil"
			},
			lookAt2 =
			{
				type = "method",
				description = "Looks at the given vector.",
				args = "(number x, number y, number z)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setNearClipDistance =
			{
				type = "method",
				description = "Sets the near clip distance at which the scene is being rendered.",
				args = "(number distance)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setFarClipDistance =
			{
				type = "method",
				description = "Sets the far clip distance until which the scene is being rendered.",
				args = "(number distance)",
				returns = "(nil)",
				valuetype = "nil"
			},
			move =
			{
				type = "method",
				description = "?",
				args = "(Vector3 vector)",
				returns = "(nil)",
				valuetype = "nil"
			},
			moveRelative =
			{
				type = "method",
				description = "?",
				args = "(Vector3 vector)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setDirection =
			{
				type = "method",
				description = "Sets at which direction vector the camera should be rotated.",
				args = "(Vector3 vector)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setDirection2 =
			{
				type = "method",
				description = "Sets at which direction vector the camera should be rotated.",
				args = "(number x, number y, number z)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDirection =
			{
				type = "function",
				description = "Gets the direction of the camera.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			rotate =
			{
				type = "method",
				description = "Rotates the camera relative by the given radian and the given vector axis.",
				args = "(Vector3 vector, Radian radian)",
				returns = "(nil)",
				valuetype = "nil"
			},
			rotate2 =
			{
				type = "method",
				description = "Rotates the camera relative by the given quaternion.",
				args = "(Quaternion quaternion)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setOrientation =
			{
				type = "method",
				description = "Sets the absolute orientation of the camera.",
				args = "(Quaternion quaternion)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getOrientation =
			{
				type = "function",
				description = "Gets the absolute orientation of the camera.",
				args = "()",
				returns = "(Quaternion)",
				valuetype = "Quaternion"
			}
		}
	},
	CameraBehaviorBaseComponent =
	{
		type = "class",
		description = "Requirements: A kind of player controller component must exist.",
		inherits = "CameraBehaviorComponent",
		childs = 
		{
			setMoveSpeed =
			{
				type = "method",
				description = "Sets the camera move speed.",
				args = "(number moveSpeed)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMoveSpeed =
			{
				type = "function",
				description = "Gets the camera move speed.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setRotationSpeed =
			{
				type = "method",
				description = "Sets the camera rotation speed.",
				args = "(number rotationSpeed)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRotationSpeed =
			{
				type = "function",
				description = "Gets the camera rotation speed.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setSmoothValue =
			{
				type = "method",
				description = "Sets the camera value for more smooth transform. Note: Setting to 0, camera transform is not smooth, setting to 1 would be to smooth and lag behind, a good value is 0.1",
				args = "(number smoothValue)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSmoothValue =
			{
				type = "function",
				description = "Gets the camera value for more smooth transform.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	CameraBehaviorComponent =
	{
		type = "class",
		description = "Requirements: A kind of player controller component must exist.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets the camera behavior component is activated. If true, the camera will do its work according the used behavior.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether camera behavior component is activated. If true, the camera will do its work according the used behavior.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			getCamera =
			{
				type = "function",
				description = "Gets the used camera pointer for direct manipulation.",
				args = "()",
				returns = "(Camera)",
				valuetype = "Camera"
			},
			setCameraControlLocked =
			{
				type = "method",
				description = "Sets whether the camera can be moved by keys. Note: This should be locked, if a behavior is active, since the camera will be moved automatically.",
				args = "(boolean locked)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCameraControlLocked =
			{
				type = "function",
				description = "Gets the rotation speed for the ai controller game object. Note: This should be locked, if a behavior is active, since the camera will be moved automatically.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			}
		}
	},
	CameraBehaviorFirstPersonComponent =
	{
		type = "class",
		description = "Requirements: A kind of player controller component must exist.",
		inherits = "CameraBehaviorComponent",
		childs = 
		{
			setSmoothValue =
			{
				type = "method",
				description = "Sets the camera value for more smooth transform. Note: Setting to 0, camera transform is not smooth, setting to 1 would be to smooth and lag behind, a good value is 0.1",
				args = "(number smoothValue)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSmoothValue =
			{
				type = "function",
				description = "Gets the camera value for more smooth transform.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setRotationSpeed =
			{
				type = "method",
				description = "Sets the camera rotation speed.",
				args = "(number rotationSpeed)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRotationSpeed =
			{
				type = "function",
				description = "Gets the camera rotation speed.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setOffsetPosition =
			{
				type = "method",
				description = "Sets the camera offset position, it should be away from the game object.",
				args = "(Vector3 offsetPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getOffsetPosition =
			{
				type = "function",
				description = "Gets the offset position, the camera is away from the game object.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			}
		}
	},
	CameraBehaviorFollow2DComponent =
	{
		type = "class",
		description = "Requirements: A kind of player controller component must exist.",
		inherits = "CameraBehaviorComponent",
		childs = 
		{
			setSmoothValue =
			{
				type = "method",
				description = "Sets the camera value for more smooth transform. Note: Setting to 0, camera transform is not smooth, setting to 1 would be to smooth and lag behind, a good value is 0.1",
				args = "(number smoothValue)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSmoothValue =
			{
				type = "function",
				description = "Gets the camera value for more smooth transform.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setOffsetPosition =
			{
				type = "method",
				description = "Sets the camera offset position, it should be away from the game object.",
				args = "(Vector3 offsetPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getOffsetPosition =
			{
				type = "function",
				description = "Gets the offset position, the camera is away from the game object.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			}
		}
	},
	CameraBehaviorThirdPersonComponent =
	{
		type = "class",
		description = "Requirements: A kind of player controller component must exist. Note: If player is not in front of camera, adjust the game object's global mesh direction.",
		inherits = "CameraBehaviorComponent",
		childs = 
		{
			setYOffset =
			{
				type = "method",
				description = "Sets the camera y-offset, the camera is placed above the game object.",
				args = "(number yOffset)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getYOffset =
			{
				type = "function",
				description = "Gets the camera y-offset, the camera is placed above the game object.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setLookAtOffset =
			{
				type = "method",
				description = "Sets the camera look at game object offset.",
				args = "(Vector3 lookAtOffset)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getLookAtOffset =
			{
				type = "function",
				description = "Gets the camera look at game object offset.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setSpringForce =
			{
				type = "method",
				description = "Sets the camera spring force, that is, when the game object is rotated the camera is moved to the same direction but with a spring effect.",
				args = "(number springForce)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSpringForce =
			{
				type = "function",
				description = "Gets the camera spring force.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setFriction =
			{
				type = "method",
				description = "Sets the camera friction during movement.",
				args = "(number friction)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getFriction =
			{
				type = "function",
				description = "Gets the camera friction during movement.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setSpringLength =
			{
				type = "method",
				description = "Sets the camera spring length during movement.",
				args = "(number springLength)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSpringLength =
			{
				type = "function",
				description = "Gets the camera spring length during movement.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	CameraBehaviorZoomComponent =
	{
		type = "class",
		description = "Zoom a camera for all game objects of the given category. Note: Camera position and orientation is important! Else World could be clipped away. A good orientation value for the camera is: -80 -60 0. Requirements: Camera must be in orthogonal mode.",
		inherits = "CameraBehaviorComponent",
		childs = 
		{
			setCategory =
			{
				type = "method",
				description = "Sets the category, for which the bounds are calculated and the zoom adapted, so that all GameObjects of this category are visible.",
				args = "(string category)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCategory =
			{
				type = "function",
				description = "Gets the category, for which the bounds are calculated and the zoom adapted, so that all GameObjects of this category are visible.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setSmoothValue =
			{
				type = "method",
				description = "Sets the camera value for more smooth transform. Note: Setting to 0, camera transform is not smooth, setting to 1 would be to smooth and lag behind, a good value is 0.1",
				args = "(number smoothValue)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSmoothValue =
			{
				type = "function",
				description = "Gets the camera value for more smooth transform.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setGrowMultiplicator =
			{
				type = "method",
				description = "Sets a grow multiplicator how fast the orthogonal camera window size will increase, so that the game objects remain in view.Play with this value, because it also depends e.g. how fast your game objects are moving.",
				args = "(number growMultiplicator)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getGrowMultiplicator =
			{
				type = "function",
				description = "Gets a grow multiplicator how fast the orthogonal camera window size will increase, so that the game objects remain in view.Play with this value, because it also depends e.g. how fast your game objects are moving.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	CameraComponent =
	{
		type = "class",
		description = "Usage: Creates a camera for camera switching or viewport tiling or vr etc.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Activates this camera and deactivates another camera (when was active).",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether camera is activated or not.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setNearClipDistance =
			{
				type = "method",
				description = "Sets the near clip distance for the camera.",
				args = "(number nearClipDistance)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getNearClipDistance =
			{
				type = "function",
				description = "Gets the near clip distance for the camera.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setFarClipDistance =
			{
				type = "method",
				description = "Sets the far clip distance for the camera.",
				args = "(number farClipDistance)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getFarClipDistance =
			{
				type = "function",
				description = "Gets the far clip distance for the camera.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setOrthographic =
			{
				type = "method",
				description = "Sets whether to use orthographic projection. Default is off (3D perspective projection).",
				args = "(boolean orthographic)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getIsOrthographic =
			{
				type = "function",
				description = "Gets whether to use orthographic projection. Default is off (3D perspective projection).",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setFovy =
			{
				type = "method",
				description = "Sets the field of view angle for the camera.",
				args = "(number angle)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getFovy =
			{
				type = "function",
				description = "Gets the field of view angle for the camera.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getCamera =
			{
				type = "function",
				description = "Gets the used camera pointer for direct manipulation.",
				args = "()",
				returns = "(Camera)",
				valuetype = "Camera"
			}
		}
	},
	CameraManager =
	{
		type = "class",
		description = "CameraManager for some camera utilities operations.",
		childs = 
		{
			setMoveCameraWeight =
			{
				type = "method",
				description = "Sets the move camera weight. Default value is 1. If set to 0, the current camera will not be moved.",
				args = "(number moveCameraWeight)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setRotateCameraWeight =
			{
				type = "method",
				description = "Sets the rotate camera weight. Default value is 1. If set to 0, the current camera will not be rotated.",
				args = "(number rotateCameraWeight)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	CapsuleGenerator =
	{
		type = "class",
		description = "Generates a procedural mesh capsule."
	},
	CatmullRomSpline2 =
	{
		type = "class",
		description = "Generates a procedural catmull rom spline2."
	},
	CatmullRomSpline3 =
	{
		type = "class",
		description = "Generates a procedural catmull rom spline3."
	},
	CircleShape =
	{
		type = "class",
		description = "Generates a procedural circle shape mesh."
	},
	ColorValue =
	{
		type = "class",
		description = "ColorValue class.",
		childs = 
		{
			r =
			{
				type = "value"
			},
			g =
			{
				type = "value"
			},
			b =
			{
				type = "value"
			},
			a =
			{
				type = "value"
			},
			saturate =
			{
				type = "method",
				description = "Clamps the value to the range [0, 1].",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	CompositorEffectBaseComponent =
	{
		type = "class",
		description = "Requirements: This component must be placed somewhere under a CameraComponent.",
		inherits = "GameObjectComponent"
	},
	CompositorEffectBlackAndWhiteComponent =
	{
		type = "class",
		description = "Requirements: A camera component must exist.",
		inherits = "CompositorEffectBaseComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Activates the given compositor effect.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether the compositor effect is activated or not.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setColor =
			{
				type = "method",
				description = "Sets the gray scale color for the black and white effect.",
				args = "(Vector3 grayScale)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getColor =
			{
				type = "function",
				description = "Gets the gray scale color.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	CompositorEffectBloomComponent =
	{
		type = "class",
		description = "Requirements: This component must be placed somewhere under a CameraComponent.",
		inherits = "CompositorEffectBaseComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Activates the given compositor effect.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether the compositor effect is activated or not.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setImageWeight =
			{
				type = "method",
				description = "Sets the image weight for the bloom effect.",
				args = "(number imageWeight)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getImageWeight =
			{
				type = "function",
				description = "Gets the image weight.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setBlurWeight =
			{
				type = "method",
				description = "Sets the blur weight for the bloom effect.",
				args = "(number blurWeight)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getblurWeight =
			{
				type = "function",
				description = "Gets the blur weight.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	CompositorEffectColorComponent =
	{
		type = "class",
		description = "Requirements: A camera component must exist.",
		inherits = "CompositorEffectBaseComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Activates the given compositor effect.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether the compositor effect is activated or not.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setColor =
			{
				type = "method",
				description = "Sets the gray scale color for the color effect.",
				args = "(Vector3 grayScale)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getColor =
			{
				type = "function",
				description = "Gets the gray scale color.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	CompositorEffectEmbossedComponent =
	{
		type = "class",
		description = "Requirements: A camera component must exist.",
		inherits = "CompositorEffectBaseComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Activates the given compositor effect.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether the compositor effect is activated or not.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setWeight =
			{
				type = "method",
				description = "Sets the embossed weight.",
				args = "(number weight)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWeight =
			{
				type = "function",
				description = "Gets the embossed weight.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	CompositorEffectGlassComponent =
	{
		type = "class",
		description = "Requirements: This component must be placed somewhere under a CameraComponent.",
		inherits = "CompositorEffectBaseComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Activates the given compositor effect.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether the compositor effect is activated or not.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setGlassWeight =
			{
				type = "method",
				description = "Sets the glass weight for the glass effect.",
				args = "(number glassWeight)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getGlassWeight =
			{
				type = "function",
				description = "Gets the glass weight.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	CompositorEffectOldTvComponent =
	{
		type = "class",
		description = "Requirements: A camera component must exist.",
		inherits = "CompositorEffectBaseComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Activates the given compositor effect.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether the compositor effect is activated or not.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setDistortionFrequency =
			{
				type = "method",
				description = "Sets the disortion frequency for the old tv effect.",
				args = "(number distortionFrequency)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDistortionFrequency =
			{
				type = "function",
				description = "Gets the disortion frequency.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setDistortionScale =
			{
				type = "method",
				description = "Sets the distortion scale for the old tv effect.",
				args = "(number distortionScale)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDistortionScale =
			{
				type = "function",
				description = "Gets the distortion scale.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setDistortionRoll =
			{
				type = "method",
				description = "Sets the distortion roll for the old tv effect.",
				args = "(number distortionRoll)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDistortionRoll =
			{
				type = "function",
				description = "Gets the distortion roll.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setInterference =
			{
				type = "method",
				description = "Sets the interference for the old tv effect.",
				args = "(number interference)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getInterference =
			{
				type = "function",
				description = "Gets the interference.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setFrameLimit =
			{
				type = "method",
				description = "Sets the frame limit for the old tv effect.",
				args = "(number frameLimit)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getFrameLimit =
			{
				type = "function",
				description = "Gets the frame limit.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setFrameShape =
			{
				type = "method",
				description = "Sets the frame shape for the old tv effect.",
				args = "(number frameShape)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getFrameShape =
			{
				type = "function",
				description = "Gets the frame shape.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setFrameSharpness =
			{
				type = "method",
				description = "Sets the frame sharpness for the old tv effect.",
				args = "(number frameSharpness)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getFrameSharpness =
			{
				type = "function",
				description = "Gets the frame sharpness.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setTime =
			{
				type = "method",
				description = "Sets the time frequency for the old tv effect.",
				args = "(Vector3 time)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTime =
			{
				type = "function",
				description = "Gets the time frequency.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setSinusTime =
			{
				type = "method",
				description = "Sets the since time frequency for the old tv effect.",
				args = "(Vector3 sinusTime)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSinusTime =
			{
				type = "function",
				description = "Gets the sinus time frequency.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			}
		}
	},
	CompositorEffectSharpenEdgesComponent =
	{
		type = "class",
		description = "Requirements: A camera component must exist.",
		inherits = "CompositorEffectBaseComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Activates the given compositor effect.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether the compositor effect is activated or not.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setWeight =
			{
				type = "method",
				description = "Sets the sharpen edges weight.",
				args = "(number weight)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWeight =
			{
				type = "function",
				description = "Gets the sharpen edges weight.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	ConeGenerator =
	{
		type = "class",
		description = "Generates a procedural mesh cone."
	},
	Contact =
	{
		type = "class",
		description = "Contact can be used, to get details information when a collision of two bodies occured and to control, what should happen with them.",
		childs = 
		{
			getNormalSpeed =
			{
				type = "function",
				description = "Gets the speed at the contact normal.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setContactPosition =
			{
				type = "method",
				description = "Sets the contact position.",
				args = "(Vector3 contactPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPositionAndNormal =
			{
				type = "function",
				description = "Gets the contact position and normal. Usage: local data = contact:getPositionAndNormal(); local position = data[0]; local normal = data[1]",
				args = "()",
				returns = "(Table[Vector3][Vector3])",
				valuetype = "Table[Vector3][Vector3]"
			},
			getTangentDirections =
			{
				type = "function",
				description = "Gets the contact tangent vector. Returns the contact primary tangent vector and the contact secondary tangent vector.",
				args = "()",
				returns = "(Table[Vector3][Vector3])",
				valuetype = "Table[Vector3][Vector3]"
			},
			getTangentSpeed =
			{
				type = "function",
				description = "Gets the speed of this contact along the tangent vector of the contact.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setSoftness =
			{
				type = "method",
				description = "Overrides the default softness for this contact.",
				args = "(number softness)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setElasticity =
			{
				type = "method",
				description = "Overrides the default elasticity for this contact.",
				args = "(number elasticity)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setFrictionState =
			{
				type = "method",
				description = "Enables or disables friction calculation for this contact. Note: State 0 makes the contact frictionless along the index tangent vector. The index 0 is for primary tangent vector or 1 for the secondary tangent.",
				args = "(number state, number index)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setFrictionCoefficient =
			{
				type = "method",
				description = "Overrides the default value of the kinetic and static coefficient of friction for this contact. Note: Static friction coefficient must be positive. Kinetic friction coeeficient must be positive. The index 0 is for primary tangent vector or 1 for the secondary tangent.",
				args = "(number static, number kinetic, number index)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setTangentAcceleration =
			{
				type = "method",
				description = "Forces the contact point to have a non-zero acceleration along the surface plane. Note: The index 0 is for primary tangent vector or 1 for the secondary tangent.",
				args = "(number acceleration)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setRotateTangentDirections =
			{
				type = "method",
				description = "Rotates the tangent direction of the contacts until the primary direction is aligned with the align Vector. Details: This function rotates the tangent vectors of the contact point until the primary tangent vector and the align vector are perpendicular (ex. when the dot product between the primary tangent vector and the alignVector is 1.0). This function can be used in conjunction with @setTangentAcceleration in order to create special effects. For example, conveyor belts, cheap low LOD vehicles, slippery surfaces, etc.",
				args = "(Vector3 direction)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setNormalDirection =
			{
				type = "method",
				description = "Sets the new direction of the for this contact point. Note: This function changes the basis of the contact point to one where the contact normal is aligned to the new direction vector and the tangent direction are recalculated to be perpendicular to the new contact normal.",
				args = "(Vector3 direction)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setNormalAcceleration =
			{
				type = "method",
				description = "Forces the contact point to have a non-zero acceleration aligned this the contact normal. Note: This function can be used for special effects like implementing jump, of explosive contact in a call back.",
				args = "(number acceleration)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getContactForce =
			{
				type = "function",
				description = "Gets the contact force vector in global space. Note: The contact force value is only valid when calculating resting contacts. This means if two bodies collide with non zero relative velocity, the reaction force will be an impulse, which is not a reaction force, this will return zero vector.  This function will only return meaningful values when the colliding bodies are at rest.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getContactMaxNormalImpact =
			{
				type = "function",
				description = "Gets the maximum normal impact of the collision.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setContactTangentFriction =
			{
				type = "method",
				description = "Sets the contact tangent friction along the surface plane. Note: State 0 makes the contact frictionless along the index tangent vector. The index 0 is for primary tangent vector or 1 for the secondary tangent.",
				args = "(number friction, number index)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getContactMaxTangentImpact =
			{
				type = "function",
				description = "Gets the maximum tangent impact of the collision.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setContactPruningTolerance =
			{
				type = "method",
				description = "Sets the pruning tolerance for this contact.",
				args = "(number tolerance)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getContactPruningTolerance =
			{
				type = "function",
				description = "Gets the pruning tolerance for this contact.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getContactPenetration =
			{
				type = "function",
				description = "Gets the contact penetration.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getClosestDistance =
			{
				type = "function",
				description = "Gets the closest distance between the two GameObjects at this contact.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			print =
			{
				type = "function",
				description = "Gets all the contact properties as string in order to print to log for debugging and analysis reasons.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			}
		}
	},
	ContactData =
	{
		type = "class",
		description = "Class, that holds contact data after a physics ray cast. See @getContactToDirection, @getContactBelow.",
		childs = 
		{
			getHitGameObject =
			{
				type = "function",
				description = "Gets the hit game object, if nothing has been hit, null will be returned.",
				args = "()",
				returns = "(GameObject)",
				valuetype = "GameObject"
			},
			getHeight =
			{
				type = "function",
				description = "Gets the height, at which the ray hit another game object. Note: This is usefull to check if a player is on ground.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getNormal =
			{
				type = "function",
				description = "Gets normal of the hit game object. Note: This can be used to check if a ground is to steep.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getSlope =
			{
				type = "function",
				description = "Gets the slope in degree to the hit game object.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	Core =
	{
		type = "class",
		description = "Some functions for NOWA core functionality.",
		childs = 
		{
			getCurrentWorldBoundLeftNear =
			{
				type = "function",
				description = "Gets left near bounding box of the currently loaded scene.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			getCurrentWorldBoundRightFar =
			{
				type = "function",
				description = "Gets right far bounding box of the currently loaded scene.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			getIsGame =
			{
				type = "function",
				description = "Gets whether the engine is used in a game and not in an editor. Note: Can be used, e.g. if set to false (editor mode) to each time reset game save data etc.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			getCurrentDateAndTime =
			{
				type = "function",
				description = "Gets the current date and time. The default format is in the form Year_Month_Day_Hour_Minutes_Seconds.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			getCurrentDateAndTime2 =
			{
				type = "function",
				description = "Gets the current date and time. E.g. format = '%Y_%m_%d_%X' which formats the value as Year_Month_Day_Hour_Minutes_Seconds.",
				args = "(string format)",
				returns = "(string)",
				valuetype = "string"
			},
			setCurrentSaveGameName =
			{
				type = "method",
				description = "Sets the current global save game name, which can be used for loading a game in an application state.",
				args = "(string saveGameName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCurrentSaveGameName =
			{
				type = "function",
				description = "Gets the current save game name, or empty string, if does not exist.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			getSaveFilePathName =
			{
				type = "function",
				description = "Gets the save file path name, or empty string, if does not exist. Note: This is usefull for debug purposes, to see, where a game does store its content and open those files for analysis etc.",
				args = "(string saveGameName)",
				returns = "(string)",
				valuetype = "string"
			},
			getProjectName =
			{
				type = "function",
				description = "Gets the current project name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			getWorldName =
			{
				type = "function",
				description = "Gets the current world (scene) name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			getSceneSnapshotsInProject =
			{
				type = "function",
				description = "Gets a list of saved game scene snapshots from the save directory for the given project name.",
				args = "(string projectName)",
				returns = "(Table[number][string])",
				valuetype = "Table[number][string]"
			},
			getSaveNamesInProject =
			{
				type = "function",
				description = "Gets a list of saved game saves (*.sav) file names from the save directory for the given project name.",
				args = "(string projectName)",
				returns = "(Table[number][string])",
				valuetype = "Table[number][string]"
			}
		}
	},
	CrowdComponent =
	{
		type = "class",
		description = "Usage: This component is used, in order to tag this game object, that it will be part of OgreRecast crowd system. Its possible to move an army of game objects as crowd.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets whether this component should be activated or not.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this component is activated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setControlled =
			{
				type = "method",
				description = "Set whether this character is controlled by an agent or whether it will position itself independently based on the requested velocity. Set to true to let the character be controlled by an agent.Set to false to manually control it without agent.",
				args = "(boolean controlled)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isControlled =
			{
				type = "function",
				description = "Gets whether this component is controlled manually.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			}
		}
	},
	CubicHermiteSpline2 =
	{
		type = "class",
		description = "Generates a procedural cubic hermite spline2."
	},
	CubicHermiteSpline3 =
	{
		type = "class",
		description = "Generates a procedural cubic ermite spline3."
	},
	CylinderGenerator =
	{
		type = "class",
		description = "Generates a procedural mesh cylinder."
	},
	CylinderUVModifier =
	{
		type = "class",
		description = "Modifies UV on a procedural cylinder."
	},
	DatablockPbsComponent =
	{
		type = "class",
		description = "Usage: Enables more detailed rendering customization for the mesh according physically based shading. Requirements: A kind of game object with mesh.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setSubEntityIndex =
			{
				type = "method",
				description = "Sets the sub entity index, this data block should be used for.",
				args = "(number index)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSubEntityIndex =
			{
				type = "function",
				description = "Gets the sub entity index this data block is used for.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setWorkflow =
			{
				type = "method",
				description = "Sets the data block work flow. Possible values are: 'SpecularWorkflow', 'SpecularAsFresnelWorkflow', 'MetallicWorkflow'",
				args = "(string workflow)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWorkflow =
			{
				type = "function",
				description = "Gets the data block work flow.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setMetalness =
			{
				type = "method",
				description = "Sets the metalness in a metallic workflow. Overrides any fresnel value. Only visible/usable when metallic workflow is active. Range: [0; 1].",
				args = "(number metalness)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMetalness =
			{
				type = "function",
				description = "Gets the metalness value.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setRoughness =
			{
				type = "method",
				description = "Sets roughness value. Range: [0; 1].",
				args = "(number roughness)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRoughness =
			{
				type = "function",
				description = "Gets the roughness value.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setFresnel =
			{
				type = "method",
				description = "Sets the fresnel (F0) directly. The fresnel of the material for each color component. When separateFresnel = false, the Y and Z components are ignored. Note: Only visible/usable when fresnel workflow is active.",
				args = "(Vector3 fresnel, boolean separateFresnel)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getFresnel =
			{
				type = "function",
				description = "Gets the fresnel values (Vector3 for fresnel color and bool if fresnes is separated).",
				args = "()",
				returns = "(Vector4)",
				valuetype = "Vector4"
			},
			setIndexOfRefraction =
			{
				type = "method",
				description = "Calculates fresnel (F0 in most books) based on the IOR. Note: If 'separateFresnel' was different from the current setting, it will call flushRenderables. If the another shader must be created, it could cause a stall. The index of refraction of the material for each colour component. When separateFresnel = false, the Y and Z components are ignored. Whether to use the same fresnel term for RGB channel, or individual ones.",
				args = "(Vector3 colorIndex, boolean separateFresnel)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getIndexOfRefraction =
			{
				type = "function",
				description = "Gets the fresnel values (Vector3 for fresnel color and bool if fresnes is separated).",
				args = "()",
				returns = "(Vector4)",
				valuetype = "Vector4"
			},
			setRefractionStrength =
			{
				type = "method",
				description = "Sets the strength of the refraction, i.e. how much displacement in screen space. Note: This value is not physically based. Only used when 'setTransparency' was set to HlmsPbsDatablock::Refractive",
				args = "(number refractionStrength)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRefractionStrength =
			{
				type = "function",
				description = "Gets the roughness value..",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setBrdf =
			{
				type = "method",
				description = "Sets brdf mode. Possible values are: Default (Most physically accurate BRDF performance heavy), CookTorrance (Ideal for silk (use high roughness values), synthetic fabric), BlinnPhong (performance. It's cheaper, while still looking somewhat similar to Default), DefaultUncorrelated (auses edges to be dimmer and is less correct, like in Unity), DefaultSeparateDiffuseFresnel (for complex refractions and reflections like glass, transparent plastics, fur, and surface with refractions and multiple rescattering, achieve a perfect mirror effect, raise the fresnel term to 1 and the diffuse color to black), BlinnPhongSeparateDiffuseFresnel (like DefaultSeparateDiffuseFresnel, but uses BlinnPhong as base), BlinnPhongLegacyMath (looks more like a 2000-2005's game, Ignores fresnel completely)",
				args = "(string brdf)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getBrdf =
			{
				type = "function",
				description = "Gets the currently used brdf model.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setTwoSidedLighting =
			{
				type = "method",
				description = "Allows support for two sided lighting. Disabled by default (faster)",
				args = "(boolean twoSidedLighting)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTwoSidedLighting =
			{
				type = "function",
				description = "Gets whether two sided lighting is used.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setOneSidedShadowCastCullingMode =
			{
				type = "method",
				description = "Sets the one sided shadow cast culling mode. While CULL_NONE is usually the 'correct' option, setting CULL_ANTICLOCKWISE can prevent ugly self-shadowing on interiors.",
				args = "(string cullingMode)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getOneSidedShadowCastCullingMode =
			{
				type = "function",
				description = "Gets the currently used one sided shadow cast culling mode.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setAlphaTestThreshold =
			{
				type = "method",
				description = "Sets threshold for alpha. Alpha testing works on the alpha channel of the diffuse texture. If there is no diffuse texture, the first diffuse detail map after.",
				args = "(number threshold)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAlphaTestThreshold =
			{
				type = "function",
				description = "Gets the alpha threshold.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setReceiveShadows =
			{
				type = "method",
				description = "Sets whether the model of this data block should receive shadows or not.",
				args = "(boolean receiveShadows)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getReceiveShadows =
			{
				type = "function",
				description = "Gets whether the model of this data block should receive shadows or not.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setDiffuseColor =
			{
				type = "method",
				description = "Sets the diffuse color (final multiplier). The color will be divided by PI for energy conservation.",
				args = "(Vector3 diffuseColor)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDiffuseColor =
			{
				type = "function",
				description = "Gets the diffuse color.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setBackgroundColor =
			{
				type = "method",
				description = "Sets the diffuse background color. When no diffuse texture is present, this solid color replaces it, and can act as a background for the detail maps.",
				args = "(Vector3 backgroundColor)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getBackgroundColor =
			{
				type = "function",
				description = "Gets the background color.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setSpecularColor =
			{
				type = "method",
				description = "Sets the specular color.",
				args = "(Vector3 specularColor)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSpecularColor =
			{
				type = "function",
				description = "Gets the specular color.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setEmissiveColor =
			{
				type = "method",
				description = "Sets the emissive color.",
				args = "(Vector3 emissiveColor)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getEmissiveColor =
			{
				type = "function",
				description = "Gets the emissive color.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setDiffuseTextureName =
			{
				type = "method",
				description = "Sets the diffuse texture name.",
				args = "(string diffuseTextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDiffuseTextureName =
			{
				type = "function",
				description = "Gets the diffuse texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setNormalTextureName =
			{
				type = "method",
				description = "Sets the normal texture name.",
				args = "(string normalTextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getNormalTextureName =
			{
				type = "function",
				description = "Gets the normal texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setNormalMapWeight =
			{
				type = "method",
				description = "Sets the normal mapping weight. Note: A value of 0 means no effect (tangent space normal is 0, 0, 1); and would be. A value of 1 means full normal map effect. A value outside the [0; 1] range extrapolates.",
				args = "(number normalMapWeight)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getNormalMapWeight =
			{
				type = "function",
				description = "Gets the normal map weight.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setClearCoat =
			{
				type = "method",
				description = "Sets the strength of the of the clear coat layer. Note: This should be treated as a binary value, set to either 0 or 1. Intermediate values are useful to control transitions between parts of the surface that have a clear coat layers and parts that don't.",
				args = "(number clearCoat)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getClearCoat =
			{
				type = "function",
				description = "Gets clear coat value.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setClearCoatRoughness =
			{
				type = "method",
				description = "Sets the roughness of the of the clear coat layer.",
				args = "(number clearCoatRoughness)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getClearCoatRoughness =
			{
				type = "function",
				description = "Gets clear coat roughness.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setSpecularTextureName =
			{
				type = "method",
				description = "Sets the specular texture name.",
				args = "(string specularTextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSpecularTextureName =
			{
				type = "function",
				description = "Gets the specular texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setMetallicTextureName =
			{
				type = "method",
				description = "Sets the metallic texture name.",
				args = "(string metallicTextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMetallicTextureName =
			{
				type = "function",
				description = "Gets the metallic texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setRoughnessTextureName =
			{
				type = "method",
				description = "Sets the roughness texture name.",
				args = "(string roughnessTextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRoughnessTextureName =
			{
				type = "function",
				description = "Gets the roughness texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setDetailWeightTextureName =
			{
				type = "method",
				description = "Sets the detail weight texture name.",
				args = "(string detailWeightTextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDetailWeightTextureName =
			{
				type = "function",
				description = "Gets the detail weight texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setDetail0TextureName =
			{
				type = "method",
				description = "Sets the detail 0 texture name.",
				args = "(string detail0TextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDetail0TextureName =
			{
				type = "function",
				description = "Gets the detail 0 texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setBlendMode0 =
			{
				type = "method",
				description = "Sets the blend mode 0. The following values are possible: 'PBSM_BLEND_NORMAL_NON_PREMUL', 'PBSM_BLEND_NORMAL_PREMUL''PBSM_BLEND_ADD', 'PBSM_BLEND_SUBTRACT', 'PBSM_BLEND_MULTIPLY', 'PBSM_BLEND_MULTIPLY2X', 'PBSM_BLEND_SCREEN', 'PBSM_BLEND_OVERLAY', 'PBSM_BLEND_LIGHTEN', 'PBSM_BLEND_DARKEN''PBSM_BLEND_GRAIN_EXTRACT', 'PBSM_BLEND_GRAIN_MERGE', 'PBSM_BLEND_DIFFERENCE'",
				args = "(string blendMode0)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getBlendMode0 =
			{
				type = "function",
				description = "Gets the blend mode 0.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setDetail1TextureName =
			{
				type = "method",
				description = "Sets the detail 1 texture name.",
				args = "(string detail1TextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDetail1TextureName =
			{
				type = "function",
				description = "Gets the detail 1 texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setBlendMode1 =
			{
				type = "method",
				description = "Sets the blend mode 1. The following values are possible: 'PBSM_BLEND_NORMAL_NON_PREMUL', 'PBSM_BLEND_NORMAL_PREMUL''PBSM_BLEND_ADD', 'PBSM_BLEND_SUBTRACT', 'PBSM_BLEND_MULTIPLY', 'PBSM_BLEND_MULTIPLY2X', 'PBSM_BLEND_SCREEN', 'PBSM_BLEND_OVERLAY', 'PBSM_BLEND_LIGHTEN', 'PBSM_BLEND_DARKEN''PBSM_BLEND_GRAIN_EXTRACT', 'PBSM_BLEND_GRAIN_MERGE', 'PBSM_BLEND_DIFFERENCE'",
				args = "(string blendMode1)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getBlendMode1 =
			{
				type = "function",
				description = "Gets the blend mode 1.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setDetail2TextureName =
			{
				type = "method",
				description = "Sets the detail 2 texture name.",
				args = "(string detail2TextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDetail2TextureName =
			{
				type = "function",
				description = "Gets the detail 2 texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setBlendMode2 =
			{
				type = "method",
				description = "Sets the blend mode 2. The following values are possible: 'PBSM_BLEND_NORMAL_NON_PREMUL', 'PBSM_BLEND_NORMAL_PREMUL''PBSM_BLEND_ADD', 'PBSM_BLEND_SUBTRACT', 'PBSM_BLEND_MULTIPLY', 'PBSM_BLEND_MULTIPLY2X', 'PBSM_BLEND_SCREEN', 'PBSM_BLEND_OVERLAY', 'PBSM_BLEND_LIGHTEN', 'PBSM_BLEND_DARKEN''PBSM_BLEND_GRAIN_EXTRACT', 'PBSM_BLEND_GRAIN_MERGE', 'PBSM_BLEND_DIFFERENCE'",
				args = "(string blendMode2)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getBlendMode2 =
			{
				type = "function",
				description = "Gets the blend mode 2.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setDetail3TextureName =
			{
				type = "method",
				description = "Sets the detail 3 texture name.",
				args = "(string detail3TextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDetail3TextureName =
			{
				type = "function",
				description = "Gets the detail 3 texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setBlendMode3 =
			{
				type = "method",
				description = "Sets the blend mode 3. The following values are possible: 'PBSM_BLEND_NORMAL_NON_PREMUL', 'PBSM_BLEND_NORMAL_PREMUL''PBSM_BLEND_ADD', 'PBSM_BLEND_SUBTRACT', 'PBSM_BLEND_MULTIPLY', 'PBSM_BLEND_MULTIPLY2X', 'PBSM_BLEND_SCREEN', 'PBSM_BLEND_OVERLAY', 'PBSM_BLEND_LIGHTEN', 'PBSM_BLEND_DARKEN''PBSM_BLEND_GRAIN_EXTRACT', 'PBSM_BLEND_GRAIN_MERGE', 'PBSM_BLEND_DIFFERENCE'",
				args = "(string blendMode3)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getBlendMode3 =
			{
				type = "function",
				description = "Gets the blend mode 3.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setDetail0NMTextureName =
			{
				type = "method",
				description = "Sets the detail 0 normal map texture name.",
				args = "(string detail0NMTextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDetail0NMTextureName =
			{
				type = "function",
				description = "Gets the detail 0 normal map texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setDetail1NMTextureName =
			{
				type = "method",
				description = "Sets the detail 1 normal map texture name.",
				args = "(string detail1NMTextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDetail1NMTextureName =
			{
				type = "function",
				description = "Gets the detail 1 normal map texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setDetail2NMTextureName =
			{
				type = "method",
				description = "Sets the detail 2 normal map texture name.",
				args = "(string detail2NMTextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDetail2NMTextureName =
			{
				type = "function",
				description = "Gets the detail 2 normal map texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setDetail3NMTextureName =
			{
				type = "method",
				description = "Sets the detail 3 normal map texture name.",
				args = "(string detail3NMTextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDetail3NMTextureName =
			{
				type = "function",
				description = "Gets the detail 3 normal map texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setReflectionTextureName =
			{
				type = "method",
				description = "Sets the reflection texture name.",
				args = "(string reflectionTextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getReflectionTextureName =
			{
				type = "function",
				description = "Gets the reflection texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setEmissiveTextureName =
			{
				type = "method",
				description = "Sets the emissive texture name.",
				args = "(string emissiveTextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getEmissiveTextureName =
			{
				type = "function",
				description = "Gets the emissive texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setTransparencyMode =
			{
				type = "method",
				description = "Sets the transparency mode. Possible values are: 'Transparent', 'Fade'",
				args = "(string transparencyMode)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTransparencyMode =
			{
				type = "function",
				description = "Gets the transparency mode.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setTransparency =
			{
				type = "method",
				description = "Makes the material transparent, and sets the amount of transparency. Note: Value in range [0; 1] where 0 = full transparency and 1 = fully opaque",
				args = "(number transparency)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTransparency =
			{
				type = "function",
				description = "Gets the transparency value for the material.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setUseAlphaFromTextures =
			{
				type = "method",
				description = "Sets whether to use transparency from textures or not. When false, the alpha channel of the diffuse maps and detail maps will be ignored. It's a GPU performance optimization.",
				args = "(boolean useAlphaFromTextures)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getUseAlphaFromTextures =
			{
				type = "function",
				description = "Gets whether to use transparency from textures or not.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setUseEmissiveAsLightMap =
			{
				type = "method",
				description = "When set, it treats the emissive map as a lightmap; which means it will be multiplied against the diffuse component.",
				args = "(boolean useEmissiveAsLightMap)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getUseEmissiveAsLightMap =
			{
				type = "function",
				description = "Gets whether to use the emissive as light map.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setBringToFront =
			{
				type = "method",
				description = "When set, it brings this mesh to front of all other meshes.",
				args = "(boolean bringToFront)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getIsInFront =
			{
				type = "function",
				description = "Gets whether this mesh is in front of all other meshes.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setCutOff =
			{
				type = "method",
				description = "When set, it cuts off this mesh from all other meshes which are touching this mesh.",
				args = "(boolean cutOff)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCutOff =
			{
				type = "function",
				description = "Gets whether this mesh is cut off from all other meshes which are touching this mesh..",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			}
		}
	},
	DatablockTerraComponent =
	{
		type = "class",
		description = "Usage: Enables more detailed rendering customization for the mesh according physically based shading.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setDiffuseColor =
			{
				type = "method",
				description = "Sets the diffuse color (final multiplier). The color will be divided by PI for energy conservation.",
				args = "(Vector3 diffuseColor)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDiffuseColor =
			{
				type = "function",
				description = "Gets the diffuse color.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setDiffuseTextureName =
			{
				type = "method",
				description = "Sets the diffuse texture name.",
				args = "(string diffuseTextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDiffuseTextureName =
			{
				type = "function",
				description = "Gets the diffuse texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setDetailWeightTextureName =
			{
				type = "method",
				description = "Sets the detail weight texture name.",
				args = "(string detailWeightTextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDetailWeightTextureName =
			{
				type = "function",
				description = "Gets the detail weight texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setDetail0TextureName =
			{
				type = "method",
				description = "Sets the detail 0 texture name.",
				args = "(string detail0TextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDetail0TextureName =
			{
				type = "function",
				description = "Gets the detail 0 texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			getBlendMode0 =
			{
				type = "function",
				description = "Gets the blend mode 0.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setDetail1TextureName =
			{
				type = "method",
				description = "Sets the detail 1 texture name.",
				args = "(string detail1TextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDetail1TextureName =
			{
				type = "function",
				description = "Gets the detail 1 texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			getBlendMode1 =
			{
				type = "function",
				description = "Gets the blend mode 1.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setDetail2TextureName =
			{
				type = "method",
				description = "Sets the detail 2 texture name.",
				args = "(string detail2TextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDetail2TextureName =
			{
				type = "function",
				description = "Gets the detail 2 texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			getBlendMode2 =
			{
				type = "function",
				description = "Gets the blend mode 2.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setDetail3TextureName =
			{
				type = "method",
				description = "Sets the detail 3 texture name.",
				args = "(string detail3TextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDetail3TextureName =
			{
				type = "function",
				description = "Gets the detail 3 texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			getBlendMode3 =
			{
				type = "function",
				description = "Gets the blend mode 3.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setDetail0NMTextureName =
			{
				type = "method",
				description = "Sets the detail 0 normal map texture name.",
				args = "(string detail0NMTextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDetail0NMTextureName =
			{
				type = "function",
				description = "Gets the detail 0 normal map texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setDetail1NMTextureName =
			{
				type = "method",
				description = "Sets the detail 1 normal map texture name.",
				args = "(string detail1NMTextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDetail1NMTextureName =
			{
				type = "function",
				description = "Gets the detail 1 normal map texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setDetail2NMTextureName =
			{
				type = "method",
				description = "Sets the detail 2 normal map texture name.",
				args = "(string detail2NMTextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDetail2NMTextureName =
			{
				type = "function",
				description = "Gets the detail 2 normal map texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setDetail3NMTextureName =
			{
				type = "method",
				description = "Sets the detail 3 normal map texture name.",
				args = "(string detail3NMTextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDetail3NMTextureName =
			{
				type = "function",
				description = "Gets the detail 3 normal map texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setDetail0RTextureName =
			{
				type = "method",
				description = "Sets the detail 0 roughness map texture name.",
				args = "(string detail0RTextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDetail0RTextureName =
			{
				type = "function",
				description = "Gets the detail 0 roughness map texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setDetail1RTextureName =
			{
				type = "method",
				description = "Sets the detail 1 roughness map texture name.",
				args = "(string detail1RTextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDetail1RTextureName =
			{
				type = "function",
				description = "Gets the detail 1 roughness map texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setDetail2RTextureName =
			{
				type = "method",
				description = "Sets the detail 2 roughness map texture name.",
				args = "(string detail2RTextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDetail2RTextureName =
			{
				type = "function",
				description = "Gets the detail 2 roughness map texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setDetail3RTextureName =
			{
				type = "method",
				description = "Sets the detail 3 roughness map texture name.",
				args = "(string detail3RTextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDetail3RTextureName =
			{
				type = "function",
				description = "Gets the detail 3 roughness map texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setDetail0MTextureName =
			{
				type = "method",
				description = "Sets the detail 0 metallic map texture name.",
				args = "(string detail0MTextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDetail0MTextureName =
			{
				type = "function",
				description = "Gets the detail 0 metallic map texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setDetail1MTextureName =
			{
				type = "method",
				description = "Sets the detail 1 metallic map texture name.",
				args = "(string detail1MTextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDetail1MTextureName =
			{
				type = "function",
				description = "Gets the detail 1 metallic map texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setDetail2MTextureName =
			{
				type = "method",
				description = "Sets the detail 2 metallic map texture name.",
				args = "(string detail2MTextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDetail2MTextureName =
			{
				type = "function",
				description = "Gets the detail 2 metallic map texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setDetail3MTextureName =
			{
				type = "method",
				description = "Sets the detail 3 metallic map texture name.",
				args = "(string detail3MTextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDetail3MTextureName =
			{
				type = "function",
				description = "Gets the detail 3 metallic map texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setReflectionTextureName =
			{
				type = "method",
				description = "Sets the reflection texture name.",
				args = "(string reflectionTextureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getReflectionTextureName =
			{
				type = "function",
				description = "Gets the reflection texture name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			}
		}
	},
	DatablockUnlitComponent =
	{
		type = "class",
		description = "Usage: Enables more detailed rendering customization for the mesh without lighting.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setSubEntityIndex =
			{
				type = "method",
				description = "Sets the sub entity index, this data block should be used for.",
				args = "(number index)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSubEntityIndex =
			{
				type = "function",
				description = "Gets the sub entity index this data block is used for.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setBackgroundColor =
			{
				type = "method",
				description = "Sets the diffuse background color. When no diffuse texture is present, this solid color replaces it, and can act as a background for the detail maps.",
				args = "(Vector3 backgroundColor)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getBackgroundColor =
			{
				type = "function",
				description = "Gets the background color.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			}
		}
	},
	Degree =
	{
		type = "class",
		description = "Degree class.",
		childs = 
		{
			valueDegrees =
			{
				type = "function",
				description = "Gets the degrees as float from degree.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			valueRadians =
			{
				type = "function",
				description = "Gets the radians as float from degree.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			valueAngleUnits =
			{
				type = "function",
				description = "Gets the angle units as float from degree.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	DistortionComponent =
	{
		type = "class",
		description = "Usage: Distortion setup can be used to create blastwave effects, mix with fire particle effects to get heat distortion etc.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets whether this component should be activated or not.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this component is activated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setStrength =
			{
				type = "method",
				description = "Sets the distortion strength. Valid values are from 0.5 to 3.0. Default is 0.5.",
				args = "(number strength)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getStrength =
			{
				type = "function",
				description = "Gets the distortion strength.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	DistributedComponent =
	{
		type = "class",
		description = "Usage: Marks this game object as distributed via network, so that math transform and attributes are synced in a network scenario.",
		inherits = "GameObjectComponent"
	},
	DragDropData =
	{
		type = "class",
		description = "DragDropData class",
		childs = 
		{
			DragDropData =
			{
				type = "value"
			},
			getResourceName =
			{
				type = "function",
				description = "Gets the resource name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			getQuantity =
			{
				type = "function",
				description = "Gets resource quantity.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getSellValue =
			{
				type = "function",
				description = "Gets the sell value of the resource.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getBuyValue =
			{
				type = "function",
				description = "Gets the buy value of the resource.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getSenderReceiverIsSame =
			{
				type = "function",
				description = "Gets whether the inventory sender and receiver is the same. E.g. moving the item within the same inventory.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			getSenderInventoryId =
			{
				type = "function",
				description = "Gets the sender inventory id.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setCanDrop =
			{
				type = "method",
				description = "Sets whether the item can be dropped.",
				args = "(boolean canDrop)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCanDrop =
			{
				type = "function",
				description = "Gets whether the item can be dropped.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			}
		}
	},
	EditBox =
	{
		type = "class",
		description = "MyGUI edit box class.",
		childs = 
		{
			getTextRegion =
			{
				type = "function",
				description = "Gets the text region.",
				args = "()",
				returns = "(IntCoord)",
				valuetype = "IntCoord"
			},
			getTextSize =
			{
				type = "function",
				description = "Gets the size of the text.",
				args = "()",
				returns = "(IntSize)",
				valuetype = "IntSize"
			},
			setCaption =
			{
				type = "method",
				description = "Sets the caption of the text.",
				args = "(string caption)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCaption =
			{
				type = "function",
				description = "Gets the caption of the text.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setFontName =
			{
				type = "method",
				description = "Sets the font name of the text. Note: The corresponding font must be installed.",
				args = "(string fontName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getFontName =
			{
				type = "function",
				description = "Gets the font name of the text.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setFontHeight =
			{
				type = "method",
				description = "Sets the font height of the text. Note: The corresponding font must be installed.",
				args = "(string fontName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getFontHeight =
			{
				type = "function",
				description = "Gets the font height of the text.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setTextAlign =
			{
				type = "method",
				description = "Sets the align of the text.",
				args = "(Align align)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTextAlign =
			{
				type = "function",
				description = "Gets the align of the text.",
				args = "()",
				returns = "(Align)",
				valuetype = "Align"
			},
			setTextColor =
			{
				type = "method",
				description = "Sets the color of the text.",
				args = "(Vector4 color)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTextColor =
			{
				type = "function",
				description = "Gets the color of the text.",
				args = "()",
				returns = "(Vector4)",
				valuetype = "Vector4"
			},
			setCaptionWithReplacing =
			{
				type = "method",
				description = "Sets the caption of the text and keywords are replaced, from a translation table (e.g. printing text in the currently defined language.",
				args = "(string textWithReplacing)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setTextShadowColor =
			{
				type = "method",
				description = "Sets the shadow color of the text.",
				args = "(Vector3 shadowColor)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTextColor =
			{
				type = "function",
				description = "Gets the shadow color of the text.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setEditStatic =
			{
				type = "method",
				description = "Sets whether the text can be edited or not.",
				args = "(boolean editStatic)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getEditStatic =
			{
				type = "function",
				description = "Gets whether the text can be edited or not.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			}
		}
	},
	EllipseShape =
	{
		type = "class",
		description = "Generates a procedural ellipse shape mesh."
	},
	Entity =
	{
		type = "class",
		description = "Base class for an Ogre mesh object.",
		childs = 
		{
			setVisible =
			{
				type = "method",
				description = "Sets the entity visible (rendered) or not.",
				args = "(boolean visible)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getVisible =
			{
				type = "function",
				description = "Gets whether the entity is visible or not.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			getCastShadows =
			{
				type = "function",
				description = "Gets whether shadows are casted or not.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			getName =
			{
				type = "function",
				description = "Gets the name of the entity.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			}
		}
	},
	Extruder =
	{
		type = "class",
		description = "Extrudes a procedural meshes."
	},
	FadeComponent =
	{
		type = "class",
		description = "Creates a fading effect for the scene.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Activates the fading effect.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether fading is active or not.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setFadeMode =
			{
				type = "method",
				description = "Sets the fade mode. Possible values are: 'FadeIn', 'FadeOut'",
				args = "(string mode)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getFogMode =
			{
				type = "function",
				description = "Gets the fog mode.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setDurationSec =
			{
				type = "method",
				description = "Sets the duration in seconds for the fading.",
				args = "(number durationSec)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDurationSec =
			{
				type = "function",
				description = "Gets the duration in seconds.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			reactOnFadeCompleted =
			{
				type = "method",
				description = "Sets whether to react at the moment when fade has completed.",
				args = "(func closure)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	FloatCoord =
	{
		type = "class",
		description = "Relative coordinates class.",
		childs = 
		{
			right =
			{
				type = "function",
				description = "The right relative coordinate.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			bottom =
			{
				type = "function",
				description = "The bottom relative coordinate.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			clear =
			{
				type = "method",
				description = "Clears the coordinate.",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			},
			set =
			{
				type = "method",
				description = "Sets the relative coordinate.",
				args = "(number x1, number y1, number x2, number y2)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	FloatSize =
	{
		type = "class",
		description = "Relative size class.",
		childs = 
		{
			clear =
			{
				type = "method",
				description = "Clears the size.",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			},
			set =
			{
				type = "method",
				description = "Sets the width and height.",
				args = "(number width, number height)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	FogComponent =
	{
		type = "class",
		description = "Creates fog for the scene.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets whether fog is active or not.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether fog is active or not.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setFogMode =
			{
				type = "method",
				description = "Sets the fog mode. Possible values are: 'Linear', 'Exponential', 'Exponential 2'",
				args = "(string mode)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getFogMode =
			{
				type = "function",
				description = "Gets the fog mode.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setColor =
			{
				type = "method",
				description = "Sets the color (r, g, b) of the fog. Either set this to the same as your viewport background color, or to blend in with a skybox.",
				args = "(Vector3 color)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getColor =
			{
				type = "function",
				description = "Gets the diffuse color (r, g, b) of the fog.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setExpDensity =
			{
				type = "method",
				description = "The density of the fog in 'Exponential' or 'Exponential 2' mode, as a value between 0 and 1. The default is 0.001. ",
				args = "(number expDensity)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getExpDensity =
			{
				type = "function",
				description = "Gets the density of the fog in 'Exponential' or 'Exponential 2' mode.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setLinearStart =
			{
				type = "method",
				description = "Sets the distance in world units at which linear fog starts to encroach. Only applicable if mode is 'Linear'.",
				args = "(number linearStart)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getLinearStart =
			{
				type = "function",
				description = "Gets the distance in world units at which linear fog starts to encroach. Only applicable if mode is 'Linear'.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setLinearEnd =
			{
				type = "method",
				description = "Sets ths distance in world units at which linear fog becomes completely opaque. Only applicable if mode is 'Linear'.",
				args = "(number linearEnd)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getLinearEnd =
			{
				type = "function",
				description = "Gets ths distance in world units at which linear fog becomes completely opaque. Only applicable if mode is 'Linear'.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	GameObject =
	{
		type = "class",
		description = "Main NOWA type. A game object is a visual representation in the scene and can be composed of several components.",
		childs = 
		{
			getId =
			{
				type = "function",
				description = "Gets the id of this game object. Note: Id is the primary key, which uniquely identifies a game object.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			getName =
			{
				type = "function",
				description = "Gets the name of this game object.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			getUniqueName =
			{
				type = "function",
				description = "Gets the unique name, that is its name and the id.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			getSceneManager =
			{
				type = "function",
				description = "Gets the scene manager of this game object.",
				args = "()",
				returns = "(SceneManager)",
				valuetype = "SceneManager"
			},
			getSceneNode =
			{
				type = "function",
				description = "Gets the scene node of this game object.",
				args = "()",
				returns = "(SceneNode)",
				valuetype = "SceneNode"
			},
			getEntity =
			{
				type = "function",
				description = "Gets the entity of this game object.",
				args = "()",
				returns = "(Entity)",
				valuetype = "Entity"
			},
			getDefaultDirection =
			{
				type = "function",
				description = "Gets the direction the game object's entity is modelled.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			getCategory =
			{
				type = "function",
				description = "Gets the category to which this game object does belong. Note: This is useful when doing ray-casts on graphics base or physics base or creating physics materials between categories.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			getCategoryId =
			{
				type = "function",
				description = "Gets the tag name of game object. Note: Tags are like sub-categories. E.g. several game objects may belong to the category 'Enemy', but one group may have a tag name 'Stone', the other 'Ship1', 'Ship2' etc. This is useful when doing ray-casts on graphics base or physics base or creating physics materials between categories, to further distinquish, which tag has been hit in order to remove different energy amount.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			getTagName =
			{
				type = "function",
				description = "Gets the category id to which this game object does belong. Note: Categories are specified in a level editor and are read from XML. This is useful when doing ray-casts on graphics base or physics base or creating physics materials between categories.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			changeCategory =
			{
				type = "method",
				description = "Changes the category name, this game object belongs to.",
				args = "(string oldCategory, string newCategory)",
				returns = "(nil)",
				valuetype = "nil"
			},
			changeCategory2 =
			{
				type = "method",
				description = "Changes the category name, this game object belongs to.",
				args = "(string newCategory)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPosition =
			{
				type = "function",
				description = "Gets the position of this game object.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			getScale =
			{
				type = "function",
				description = "Gets the scale of this game object.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			getOrientation =
			{
				type = "function",
				description = "Gets the Orientation of this game object.",
				args = "()",
				returns = "(Quaternion)",
				valuetype = "Quaternion"
			},
			getControlledByClientID =
			{
				type = "function",
				description = "Gets the client id, by which this game object will be controller. This is used in a network scenario.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getSize =
			{
				type = "function",
				description = "Gets the bounding box size of this game object.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			getCenterOffset =
			{
				type = "function",
				description = "Gets offset vector by which the game object is away from the center of its bounding box.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setVisible =
			{
				type = "method",
				description = "Sets whether this game object should be visible.",
				args = "(boolean visible)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getVisible =
			{
				type = "function",
				description = "Gets whether this game object is visible.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setClampY =
			{
				type = "method",
				description = "Sets whether to clamp the y coordinate at the next lower game object. Note: If there is no one the next above game object is taken into accout.",
				args = "(boolean clampY)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getClampY =
			{
				type = "function",
				description = "Gets whether to clamp the y coordinate of this game object.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setReferenceId =
			{
				type = "method",
				description = "Sets the reference id. Note: If this game object is referenced by another game object, a component with the same id as this game object can be get for activation etc.",
				args = "(string referenceId)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getReferenceId =
			{
				type = "function",
				description = "Gets the reference if another game object does reference it.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setDynamic =
			{
				type = "method",
				description = "Sets whether this game object is frequently moved (dynamic), if not set false here, this will increase the rendering performance.",
				args = "(boolean dynamic)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setUseReflection =
			{
				type = "method",
				description = "Sets whether this game object uses shader reflection for visual effects.",
				args = "(boolean useReflection)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getUseReflection =
			{
				type = "function",
				description = "Gets whether this game object uses shader reflection for visual effects.",
				args = "(boolean getUseReflection)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			showBoundingBox =
			{
				type = "function",
				description = "Shows a bounding box for this game object if set to true.",
				args = "(boolean show)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setMaskId =
			{
				type = "method",
				description = "Manages all game objects that will be visible on a specifig camera.If the mask id is set to 0, the game object is not visible to any camera.  If set to e.g. 1, and e.g.the minimap camera has its id set to 1, it will be rendered also on the minimap. If set to e.g. 2 but minimap comopnent has set to 1, it will not be rendered on the minimap. Default value is 1 and is visible to any camera.",
				args = "(number maskId)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMaskId =
			{
				type = "function",
				description = "Manages all game objects that will be visible on a specifig camera.If the mask id is set to 0, the game object is not visible to any camera.  If set to e.g. 1, and e.g.the minimap camera has its id set to 1, it will be rendered also on the minimap. If set to e.g. 2 but minimap comopnent has set to 1, it will not be rendered on the minimap. Default value is 1 and is visible to any camera.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			showDebugData =
			{
				type = "function",
				description = "Shows a debug data (if exists) for this game object and all its components. If called a second time debug data will not be shown.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			getGameObjectComponentByIndex =
			{
				type = "function",
				description = "Gets the game object component by the given index. If the index is out of bounds, nil will be delivered.",
				args = "(number index)",
				returns = "(GameObjectComponent)",
				valuetype = "GameObjectComponent"
			},
			getComponentByIndex =
			{
				type = "function",
				description = "Gets the component from the game objects by the index.",
				args = "(GameObject gameObject, number index)",
				returns = "(GameObjectComponent)",
				valuetype = "GameObjectComponent"
			},
			getIndexFromComponent =
			{
				type = "function",
				description = "Gets index of the given component.",
				args = "(GameObjectComponent component)",
				returns = "(number)",
				valuetype = "number"
			},
			deleteComponent =
			{
				type = "function",
				description = "Deletes the component by the given class name and optionally with its occurrence index.",
				args = "(string componentClassName, number occurrenceIndex)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			deleteComponentByIndex =
			{
				type = "function",
				description = "Deletes the component by the given index.",
				args = "(number index)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			getGameObjectTitleComponent =
			{
				type = "function",
				description = "Gets the game object title component.",
				args = "()",
				returns = "(GameObjectTitleComponent)",
				valuetype = "GameObjectTitleComponent"
			},
			getAnimationComponentFromIndex =
			{
				type = "function",
				description = "Gets the animation component by the given occurence index, since a game object may have besides other components several animation components.",
				args = "(number occurrenceIndex)",
				returns = "(AnimationComponent)",
				valuetype = "AnimationComponent"
			},
			getAnimationComponent =
			{
				type = "function",
				description = "Gets the animation component. This can be used if the game object just has one animation component.",
				args = "()",
				returns = "(AnimationComponent)",
				valuetype = "AnimationComponent"
			},
			getAttributesComponent =
			{
				type = "function",
				description = "Gets the attributes component.",
				args = "()",
				returns = "(AttributesComponent)",
				valuetype = "AttributesComponent"
			},
			getAttributeEffectComponentComponentFromIndex =
			{
				type = "function",
				description = "Gets the attribute effect component by the given occurence index, since a game object may have besides other components several animation components.",
				args = "(number occurrenceIndex)",
				returns = "(AttributeEffectComponent)",
				valuetype = "AttributeEffectComponent"
			},
			getAttributeEffectComponentComponent =
			{
				type = "function",
				description = "Gets the attribute effect component. This can be used if the game object just has one attribute effect component.",
				args = "()",
				returns = "(AttributeEffectComponent)",
				valuetype = "AttributeEffectComponent"
			},
			getBuoyancyComponent =
			{
				type = "function",
				description = "Gets the physics buoyancy component.",
				args = "()",
				returns = "(BuoyancyComponent)",
				valuetype = "BuoyancyComponent"
			},
			getDistributedComponent =
			{
				type = "function",
				description = "Gets the distributed component. Usage in a network scenario.",
				args = "()",
				returns = "(DistributedComponent)",
				valuetype = "DistributedComponent"
			},
			getExitComponent =
			{
				type = "function",
				description = "Gets the exit component.",
				args = "()",
				returns = "(ExitComponent)",
				valuetype = "ExitComponent"
			},
			getAiMoveComponent =
			{
				type = "function",
				description = "Gets the ai move component. Requirements: A physics active component.",
				args = "()",
				returns = "(AiMoveComponent)",
				valuetype = "AiMoveComponent"
			},
			getAiMoveRandomlyComponent =
			{
				type = "function",
				description = "Gets the ai move randomly component. Requirements: A physics active component.",
				args = "()",
				returns = "(AiMoveRandomlyComponent)",
				valuetype = "AiMoveRandomlyComponent"
			},
			getAiPathFollowComponent =
			{
				type = "function",
				description = "Gets the ai path follow component. Requirements: A physics active component.",
				args = "()",
				returns = "(AiPathFollowComponent)",
				valuetype = "AiPathFollowComponent"
			},
			getAiWanderComponent =
			{
				type = "function",
				description = "Gets the ai wander component. Requirements: A physics active component.",
				args = "()",
				returns = "(AiWanderComponent)",
				valuetype = "AiWanderComponent"
			},
			getAiFlockingComponent =
			{
				type = "function",
				description = "Gets the ai flocking component. Requirements: A physics active component.",
				args = "()",
				returns = "(AiFlockingComponent)",
				valuetype = "AiFlockingComponent"
			},
			getAiRecastPathNavigationComponent =
			{
				type = "function",
				description = "Gets the ai recast path navigation component. Requirements: A physics active component.",
				args = "()",
				returns = "(AiRecastPathNavigationComponent)",
				valuetype = "AiRecastPathNavigationComponent"
			},
			getAiObstacleAvoidanceComponent =
			{
				type = "function",
				description = "Gets the ai obstacle avoidance component. Requirements: A physics active component.",
				args = "()",
				returns = "(AiObstacleAvoidanceComponent)",
				valuetype = "AiObstacleAvoidanceComponent"
			},
			getAiHideComponent =
			{
				type = "function",
				description = "Gets the ai hide component. Requirements: A physics active component.",
				args = "()",
				returns = "(AiHideComponent)",
				valuetype = "AiHideComponent"
			},
			getAiMoveComponent2D =
			{
				type = "function",
				description = "Gets the ai move component for 2D. Requirements: A physics active component.",
				args = "()",
				returns = "(AiMoveComponent2D)",
				valuetype = "AiMoveComponent2D"
			},
			getAiPathFollowComponent2D =
			{
				type = "function",
				description = "Gets the ai path follow component for 2D. Requirements: A physics active component.",
				args = "()",
				returns = "(AiPathFollowComponent2D)",
				valuetype = "AiPathFollowComponent2D"
			},
			getAiWanderComponent2D =
			{
				type = "function",
				description = "Gets the ai wander component for 2D. Requirements: A physics active component.",
				args = "()",
				returns = "(AiWanderComponent2D)",
				valuetype = "AiWanderComponent2D"
			},
			getCameraBehaviorBaseComponent =
			{
				type = "function",
				description = "Gets the camera behavior base component.",
				args = "()",
				returns = "(CameraBehaviorBaseComponent)",
				valuetype = "CameraBehaviorBaseComponent"
			},
			getCameraBehaviorFirstPersonComponent =
			{
				type = "function",
				description = "Gets the camera behavior first person component.",
				args = "()",
				returns = "(CameraBehaviorFirstPersonComponent)",
				valuetype = "CameraBehaviorFirstPersonComponent"
			},
			getCameraBehaviorThirdPersonComponent =
			{
				type = "function",
				description = "Gets the camera third first person component.",
				args = "()",
				returns = "(CameraBehaviorThirdPersonComponent)",
				valuetype = "CameraBehaviorThirdPersonComponent"
			},
			getCameraBehaviorFollow2DComponent =
			{
				type = "function",
				description = "Gets the camera follow 2D component.",
				args = "()",
				returns = "(CameraBehaviorFollow2DComponent)",
				valuetype = "CameraBehaviorFollow2DComponent"
			},
			getCameraBehaviorZoomComponent =
			{
				type = "function",
				description = "Gets the camera zoom component.",
				args = "()",
				returns = "(CameraBehaviorZoomComponent)",
				valuetype = "CameraBehaviorZoomComponent"
			},
			getCameraComponent =
			{
				type = "function",
				description = "Gets the camera component.",
				args = "()",
				returns = "(CameraComponent)",
				valuetype = "CameraComponent"
			},
			getCompositorEffectBloomComponent =
			{
				type = "function",
				description = "Gets the compositor effect bloom component.",
				args = "()",
				returns = "(CompositorEffectBloomComponent)",
				valuetype = "CompositorEffectBloomComponent"
			},
			getCompositorEffectGlassComponent =
			{
				type = "function",
				description = "Gets the compositor effect glass component.",
				args = "()",
				returns = "(CompositorEffectGlassComponent)",
				valuetype = "CompositorEffectGlassComponent"
			},
			getCompositorEffectBlackAndWhiteComponent =
			{
				type = "function",
				description = "Gets the compositor effect black and white component.",
				args = "()",
				returns = "(CompositorEffectBlackAndWhiteComponent)",
				valuetype = "CompositorEffectBlackAndWhiteComponent"
			},
			getCompositorEffectColorComponent =
			{
				type = "function",
				description = "Gets the compositor effect color component.",
				args = "()",
				returns = "(CompositorEffectColorComponent)",
				valuetype = "CompositorEffectColorComponent"
			},
			getCompositorEffectEmbossedComponent =
			{
				type = "function",
				description = "Gets the compositor effect embossed component.",
				args = "()",
				returns = "(CompositorEffectEmbossedComponent)",
				valuetype = "CompositorEffectEmbossedComponent"
			},
			getCompositorEffectSharpenEdgesComponent =
			{
				type = "function",
				description = "Gets the compositor effect sharpen edges component.",
				args = "()",
				returns = "(CompositorEffectSharpenEdgesComponent)",
				valuetype = "CompositorEffectSharpenEdgesComponent"
			},
			getHdrEffectComponent =
			{
				type = "function",
				description = "Gets hdr effect component.",
				args = "()",
				returns = "(HdrEffectComponent)",
				valuetype = "HdrEffectComponent"
			},
			getDatablockPbsComponent =
			{
				type = "function",
				description = "Gets the datablock pbs (physically based shading) component.",
				args = "()",
				returns = "(DatablockPbsComponent)",
				valuetype = "DatablockPbsComponent"
			},
			getDatablockUnlitComponent =
			{
				type = "function",
				description = "Gets the datablock unlit (without lighting) component.",
				args = "()",
				returns = "(DatablockUnlitComponent)",
				valuetype = "DatablockUnlitComponent"
			},
			getDatablockTerraComponent =
			{
				type = "function",
				description = "Gets the datablock terra component.",
				args = "()",
				returns = "(DatablockTerraComponent)",
				valuetype = "DatablockTerraComponent"
			},
			getTerraComponent =
			{
				type = "function",
				description = "Gets the terra component.",
				args = "()",
				returns = "(TerraComponent)",
				valuetype = "TerraComponent"
			},
			getJointComponent =
			{
				type = "function",
				description = "Gets the joint root component. Requirements: A physics component.",
				args = "()",
				returns = "(JointComponent)",
				valuetype = "JointComponent"
			},
			getJointHingeComponent =
			{
				type = "function",
				description = "Gets the joint hinge component. Requirements: A physics active component.",
				args = "()",
				returns = "(JointHingeComponent)",
				valuetype = "JointHingeComponent"
			},
			getJointTargetTransformComponent =
			{
				type = "function",
				description = "Gets the joint target transform component. Requirements: A physics active component.",
				args = "()",
				returns = "(JointTargetTransformComponent)",
				valuetype = "JointTargetTransformComponent"
			},
			getJointHingeActuatorComponent =
			{
				type = "function",
				description = "Gets the joint hinge actuator component. Requirements: A physics active component.",
				args = "()",
				returns = "(JointHingeActuatorComponent)",
				valuetype = "JointHingeActuatorComponent"
			},
			getJointBallAndSocketComponent =
			{
				type = "function",
				description = "Gets the joint ball and socket component. Requirements: A physics active component.",
				args = "()",
				returns = "(JointBallAndSocketComponent)",
				valuetype = "JointBallAndSocketComponent"
			},
			getJointPointToPointComponent =
			{
				type = "function",
				description = "Gets the joint point to point component. Requirements: A physics active component.",
				args = "()",
				returns = "(JointPointToPointComponent)",
				valuetype = "JointPointToPointComponent"
			},
			getJointPinComponent =
			{
				type = "function",
				description = "Gets the joint pin component. Requirements: A physics active component.",
				args = "()",
				returns = "(JointPinComponent)",
				valuetype = "JointPinComponent"
			},
			getJointPlaneComponent =
			{
				type = "function",
				description = "Gets the joint plane component. Requirements: A physics active component.",
				args = "()",
				returns = "(JointPlaneComponent)",
				valuetype = "JointPlaneComponent"
			},
			getJointSpringComponent =
			{
				type = "function",
				description = "Gets the joint spring component. Requirements: A physics active component.",
				args = "()",
				returns = "(JointSpringComponent)",
				valuetype = "JointSpringComponent"
			},
			getJointAttractorComponent =
			{
				type = "function",
				description = "Gets the joint attractor component. Requirements: A physics active component.",
				args = "()",
				returns = "(JointAttractorComponent)",
				valuetype = "JointAttractorComponent"
			},
			getJointCorkScrewComponent =
			{
				type = "function",
				description = "Gets the joint cork screw component. Requirements: A physics active component.",
				args = "()",
				returns = "(JointCorkScrewComponent)",
				valuetype = "JointCorkScrewComponent"
			},
			getJointPassiveSliderComponent =
			{
				type = "function",
				description = "Gets the joint passive slider component. Requirements: A physics active component.",
				args = "()",
				returns = "(JointPassiveSliderComponent)",
				valuetype = "JointPassiveSliderComponent"
			},
			getJointSliderActuatorComponent =
			{
				type = "function",
				description = "Gets the joint slider actuator component. Requirements: A physics active component.",
				args = "()",
				returns = "(JointSliderActuatorComponent)",
				valuetype = "JointSliderActuatorComponent"
			},
			getJointSlidingContactComponent =
			{
				type = "function",
				description = "Gets the joint sliding contact component. Requirements: A physics active component.",
				args = "()",
				returns = "(JointSlidingContactComponent)",
				valuetype = "JointSlidingContactComponent"
			},
			getJointActiveSliderComponent =
			{
				type = "function",
				description = "Gets the joint active slider component. Requirements: A physics active component.",
				args = "()",
				returns = "(JointActiveSliderComponent)",
				valuetype = "JointActiveSliderComponent"
			},
			getJointMathSliderComponent =
			{
				type = "function",
				description = "Gets the joint math slider component. Requirements: A physics active component.",
				args = "()",
				returns = "(JointMathSliderComponent)",
				valuetype = "JointMathSliderComponent"
			},
			getJointKinematicComponent =
			{
				type = "function",
				description = "Gets the joint kinematic component. Requirements: A physics active component.",
				args = "()",
				returns = "(JointKinematicComponent)",
				valuetype = "JointKinematicComponent"
			},
			getJointPathFollowComponent =
			{
				type = "function",
				description = "Gets the joint path follow component. Requirements: A physics active component.",
				args = "()",
				returns = "(JointPathFollowComponent)",
				valuetype = "JointPathFollowComponent"
			},
			getJointDryRollingFrictionComponent =
			{
				type = "function",
				description = "Gets the joint dry rolling friction component. Requirements: A physics active component.",
				args = "()",
				returns = "(JointDryRollingFrictionComponent)",
				valuetype = "JointDryRollingFrictionComponent"
			},
			getJointGearComponent =
			{
				type = "function",
				description = "Gets the joint gear component. Requirements: A physics active component.",
				args = "()",
				returns = "(JointGearComponent)",
				valuetype = "JointGearComponent"
			},
			getJointRackAndPinionComponent =
			{
				type = "function",
				description = "Gets the joint rack and pinion component. Requirements: A physics active component.",
				args = "()",
				returns = "(JointRackAndPinionComponent)",
				valuetype = "JointRackAndPinionComponent"
			},
			getJointWormGearComponent =
			{
				type = "function",
				description = "Gets the joint worm gear component. Requirements: A physics active component.",
				args = "()",
				returns = "(JointWormGearComponent)",
				valuetype = "JointWormGearComponent"
			},
			getJointPulleyComponent =
			{
				type = "function",
				description = "Gets the joint pulley component. Requirements: A physics active component.",
				args = "()",
				returns = "(JointPulleyComponent)",
				valuetype = "JointPulleyComponent"
			},
			getJointUniversalComponent =
			{
				type = "function",
				description = "Gets the joint universal component. Requirements: A physics active component.",
				args = "()",
				returns = "(JointUniversalComponent)",
				valuetype = "JointUniversalComponent"
			},
			getJointUniversalActuatlorComponent =
			{
				type = "function",
				description = "Gets the joint universal actuator component. Requirements: A physics active component.",
				args = "()",
				returns = "(JointUniversalActuatorComponent)",
				valuetype = "JointUniversalActuatorComponent"
			},
			getJoint6DofComponent =
			{
				type = "function",
				description = "Gets the joint 6 DOF component. Requirements: A physics active component.",
				args = "()",
				returns = "(Joint6DofComponent)",
				valuetype = "Joint6DofComponent"
			},
			getJointMotorComponent =
			{
				type = "function",
				description = "Gets the joint motor component. Requirements: A physics active component.",
				args = "()",
				returns = "(JointMotorComponent)",
				valuetype = "JointMotorComponent"
			},
			getJointWheelComponent =
			{
				type = "function",
				description = "Gets the joint wheel component. Requirements: A physics active component.",
				args = "()",
				returns = "(JointWheelComponent)",
				valuetype = "JointWheelComponent"
			},
			getLightDirectionalComponent =
			{
				type = "function",
				description = "Gets the light directional component.",
				args = "()",
				returns = "(LightDirectionalComponent)",
				valuetype = "LightDirectionalComponent"
			},
			getLightPointComponent =
			{
				type = "function",
				description = "Gets the light point component.",
				args = "()",
				returns = "(LightPointComponent)",
				valuetype = "LightPointComponent"
			},
			getLightSpotComponent =
			{
				type = "function",
				description = "Gets the light spot component.",
				args = "()",
				returns = "(LightSpotComponent)",
				valuetype = "LightSpotComponent"
			},
			getLightAreaComponent =
			{
				type = "function",
				description = "Gets the light area component.",
				args = "()",
				returns = "(LightAreaComponent)",
				valuetype = "LightAreaComponent"
			},
			getFadeComponent =
			{
				type = "function",
				description = "Gets the fade component.",
				args = "()",
				returns = "(FadeComponent)",
				valuetype = "FadeComponent"
			},
			getNavMeshComponent =
			{
				type = "function",
				description = "Gets the nav mesh component. Requirements: OgreRecast navigation must be activated.",
				args = "()",
				returns = "(NavMeshComponent)",
				valuetype = "NavMeshComponent"
			},
			getParticleUniverseComponent =
			{
				type = "function",
				description = "Gets the particle universe component by the given occurence index, since a game object may have besides other components several particle universe components.",
				args = "(number occurrenceIndex)",
				returns = "(ParticleUniverseComponent)",
				valuetype = "ParticleUniverseComponent"
			},
			getParticleUniverseComponentFromIndex =
			{
				type = "function",
				description = "Gets the particle universe component. This can be used if the game object just has one particle universe component.",
				args = "()",
				returns = "(ParticleUniverseComponent)",
				valuetype = "ParticleUniverseComponent"
			},
			getPlayerControllerComponent =
			{
				type = "function",
				description = "Gets the base player controller component. Requirements: A physics active component.",
				args = "()",
				returns = "(PlayerControllerComponent)",
				valuetype = "PlayerControllerComponent"
			},
			getPlayerControllerJumpNRunComponent =
			{
				type = "function",
				description = "Gets the player controller Jump N Run component. Requirements: A physics active component.",
				args = "()",
				returns = "(PlayerControllerJumpNRunComponent)",
				valuetype = "PlayerControllerJumpNRunComponent"
			},
			getPlayerControllerJumpNRunLuaComponent =
			{
				type = "function",
				description = "Gets the player controller Jump N Run Lua component. Requirements: A physics active component and a LuaScriptComponent.",
				args = "()",
				returns = "(PlayerControllerJumpNRunLuaComponent)",
				valuetype = "PlayerControllerJumpNRunLuaComponent"
			},
			getPlayerControllerClickToPointComponent =
			{
				type = "function",
				description = "Gets the player controller click to point component. Requirements: A physics active component.",
				args = "()",
				returns = "(PlayerControllerClickToPointComponent)",
				valuetype = "PlayerControllerClickToPointComponent"
			},
			getPhysicsComponent =
			{
				type = "function",
				description = "Gets the physics component, which is the base for all other physics components.",
				args = "()",
				returns = "(PhysicsComponent)",
				valuetype = "PhysicsComponent"
			},
			getPhysicsActiveComponent =
			{
				type = "function",
				description = "Gets the physics active component.",
				args = "()",
				returns = "(PhysicsActiveComponent)",
				valuetype = "PhysicsActiveComponent"
			},
			getPhysicsActiveCompoundComponent =
			{
				type = "function",
				description = "Gets the physics active compound component.",
				args = "()",
				returns = "(PhysicsActiveCompoundComponent)",
				valuetype = "PhysicsActiveCompoundComponent"
			},
			getPhysicsActiveDestructibleComponent =
			{
				type = "function",
				description = "Gets the physics active destructable component.",
				args = "()",
				returns = "(PhysicsActiveDestructibleComponent)",
				valuetype = "PhysicsActiveDestructibleComponent"
			},
			getPhysicsArtifactComponent =
			{
				type = "function",
				description = "Gets the physics artifact component.",
				args = "()",
				returns = "(PhysicsArtifactComponent)",
				valuetype = "PhysicsArtifactComponent"
			},
			getPhysicsRagDollComponent =
			{
				type = "function",
				description = "Gets the physics ragdoll component.",
				args = "()",
				returns = "(PhysicsRagDollComponent)",
				valuetype = "PhysicsRagDollComponent"
			},
			getPhysicsCompoundConnectionComponent =
			{
				type = "function",
				description = "Gets the physics compound connection component.",
				args = "()",
				returns = "(PhysicsCompoundConnectionComponent)",
				valuetype = "PhysicsCompoundConnectionComponent"
			},
			getPhysicsMaterialComponentFromIndex =
			{
				type = "function",
				description = "Gets the physics material component by the given occurence index, since a game object may have besides other components several physics material components.",
				args = "(number occurrenceIndex)",
				returns = "(PhysicsMaterialComponent)",
				valuetype = "PhysicsMaterialComponent"
			},
			getPhysicsMaterialComponent =
			{
				type = "function",
				description = "Gets the physics material component. This can be used if the game object just has one physics material component.",
				args = "()",
				returns = "(PhysicsMaterialComponent)",
				valuetype = "PhysicsMaterialComponent"
			},
			getPhysicsPlayerControllerComponent =
			{
				type = "function",
				description = "Gets the physics player controller component.",
				args = "()",
				returns = "(PhysicsPlayerControllerComponent)",
				valuetype = "PhysicsPlayerControllerComponent"
			},
			getPhysicsActiveVehicleComponent =
			{
				type = "function",
				description = "Gets the physics active vehicle component.",
				args = "()",
				returns = "(PhysicsActiveVehicleComponent)",
				valuetype = "PhysicsActiveVehicleComponent"
			},
			getPlaneComponent =
			{
				type = "function",
				description = "Gets the plane component.",
				args = "()",
				returns = "(PlaneComponent)",
				valuetype = "PlaneComponent"
			},
			getSimpleSoundComponentFromIndex =
			{
				type = "function",
				description = "Gets the simple sound component by the given occurence index, since a game object may have besides other components several simple sound components.",
				args = "(number occurrenceIndex)",
				returns = "(SimpleSoundComponent)",
				valuetype = "SimpleSoundComponent"
			},
			getSimpleSoundComponent =
			{
				type = "function",
				description = "Gets the simple sound component. This can be used if the game object just has one simple sound component.",
				args = "()",
				returns = "(SimpleSoundComponent)",
				valuetype = "SimpleSoundComponent"
			},
			getSoundComponentFromIndex =
			{
				type = "function",
				description = "Gets the sound component by the given occurence index, since a game object may have besides other components several sound components.",
				args = "(number occurrenceIndex)",
				returns = "(SoundComponent)",
				valuetype = "SoundComponent"
			},
			getSoundComponent =
			{
				type = "function",
				description = "Gets the sound component. This can be used if the game object just has one sound component.",
				args = "()",
				returns = "(SoundComponent)",
				valuetype = "SoundComponent"
			},
			getSpawnComponent =
			{
				type = "function",
				description = "Gets the spawn component.",
				args = "()",
				returns = "(SpawnComponent)",
				valuetype = "SpawnComponent"
			},
			getVehicleComponent =
			{
				type = "function",
				description = "Gets the physics vehicle component. Requirements: A physics active component.",
				args = "()",
				returns = "(VehicleComponent)",
				valuetype = "VehicleComponent"
			},
			getAiLuaComponent =
			{
				type = "function",
				description = "Gets the ai lua script component. Requirements: A physics active component and a lua script component.",
				args = "()",
				returns = "(AiLuaComponent)",
				valuetype = "AiLuaComponent"
			},
			getPhysicsExplosionComponent =
			{
				type = "function",
				description = "Gets the physics explosion component.",
				args = "()",
				returns = "(PhysicsExplosionComponent)",
				valuetype = "PhysicsExplosionComponent"
			},
			getMeshDecalComponent =
			{
				type = "function",
				description = "Gets the mesh decal component.",
				args = "()",
				returns = "(MeshDecalComponent)",
				valuetype = "MeshDecalComponent"
			},
			getTagPointComponentFromIndex =
			{
				type = "function",
				description = "Gets the tag point component by the given occurence index, since a game object may have besides other components several tag point components.",
				args = "(number occurrenceIndex)",
				returns = "(TagPointComponent)",
				valuetype = "TagPointComponent"
			},
			getTagPointComponent =
			{
				type = "function",
				description = "Gets the tag point component. This can be used if the game object just has one tag point component.",
				args = "()",
				returns = "(TagPointComponent)",
				valuetype = "TagPointComponent"
			},
			getTimeTriggerComponentFromIndex =
			{
				type = "function",
				description = "Gets the time trigger component by the given occurence index, since a game object may have besides other components several tag point components.",
				args = "(number occurrenceIndex)",
				returns = "(TimeTriggerComponent)",
				valuetype = "TimeTriggerComponent"
			},
			getTimeTriggerComponent =
			{
				type = "function",
				description = "Gets the time trigger component. This can be used if the game object just has one time trigger component.",
				args = "()",
				returns = "(TimeTriggerComponent)",
				valuetype = "TimeTriggerComponent"
			},
			getTimeLineComponent =
			{
				type = "function",
				description = "Gets the time line component.",
				args = "()",
				returns = "(TimeLineComponent)",
				valuetype = "TimeLineComponent"
			},
			getMoveMathFunctionComponent =
			{
				type = "function",
				description = "Gets the move math function component.",
				args = "()",
				returns = "(MoveMathFunctionComponent)",
				valuetype = "MoveMathFunctionComponent"
			},
			getTagChildNodeComponent =
			{
				type = "function",
				description = "Gets the tag child node component by the given occurence index, since a game object may have besides other components several tag child node components.",
				args = "(number occurrenceIndex)",
				returns = "(TagChildNodeComponent)",
				valuetype = "TagChildNodeComponent"
			},
			getTagChildNodeComponentFromIndex =
			{
				type = "function",
				description = "Gets the tag child node component. This can be used if the game object just has one tag child node component.",
				args = "()",
				returns = "(TagChildNodeComponent)",
				valuetype = "TagChildNodeComponent"
			},
			getNodeTrackComponent =
			{
				type = "function",
				description = "Gets the node track component.",
				args = "()",
				returns = "(NodeTrackComponent)",
				valuetype = "NodeTrackComponent"
			},
			getLineComponent =
			{
				type = "function",
				description = "Gets the line component.",
				args = "()",
				returns = "(LineComponent)",
				valuetype = "LineComponent"
			},
			getLinesComponent =
			{
				type = "function",
				description = "Gets the lines component.",
				args = "()",
				returns = "(LinesComponent)",
				valuetype = "LinesComponent"
			},
			getNodeComponent =
			{
				type = "function",
				description = "Gets the node component.",
				args = "()",
				returns = "(NodeComponent)",
				valuetype = "NodeComponent"
			},
			getManualObjectComponent =
			{
				type = "function",
				description = "Gets the manual object component.",
				args = "()",
				returns = "(ManualObjectComponent)",
				valuetype = "ManualObjectComponent"
			},
			getRectangleComponent =
			{
				type = "function",
				description = "Gets the rectangle component.",
				args = "()",
				returns = "(RectangleComponent)",
				valuetype = "RectangleComponent"
			},
			getValueBarComponent =
			{
				type = "function",
				description = "Gets the value bar component.",
				args = "()",
				returns = "(ValueBarComponent)",
				valuetype = "ValueBarComponent"
			},
			getBillboardComponent =
			{
				type = "function",
				description = "Gets the billboard component.",
				args = "()",
				returns = "(BillboardComponent)",
				valuetype = "BillboardComponent"
			},
			getRibbonTrailComponent =
			{
				type = "function",
				description = "Gets the ribbon trail component.",
				args = "()",
				returns = "(RibbonTrailComponent)",
				valuetype = "RibbonTrailComponent"
			},
			getMyGUIWindowComponentFromIndex =
			{
				type = "function",
				description = "Gets the MyGUI window component by the given occurence index, since a game object may have besides other components several MyGUI window components.",
				args = "(number occurrenceIndex)",
				returns = "(MyGUIWindowComponent)",
				valuetype = "MyGUIWindowComponent"
			},
			getMyGUIWindowComponent =
			{
				type = "function",
				description = "Gets the MyGUI window component. This can be used if the game object just has one MyGUI window component.",
				args = "()",
				returns = "(MyGUIWindowComponent)",
				valuetype = "MyGUIWindowComponent"
			},
			getMyGUIItemBoxComponent =
			{
				type = "function",
				description = "Gets the MyGUI item box component. This can be used for inventory item in conjunction with InventoryItemComponent.",
				args = "()",
				returns = "(MyGUIItemBoxComponent)",
				valuetype = "MyGUIItemBoxComponent"
			},
			getInventoryItemComponent =
			{
				type = "function",
				description = "Gets the inventory item component. This can be used for inventory item in conjunction with MyGUIItemBoxComponent.",
				args = "()",
				returns = "(InventoryItemComponent)",
				valuetype = "InventoryItemComponent"
			},
			getMyGUITextComponentFromIndex =
			{
				type = "function",
				description = "Gets the MyGUI text component by the given occurence index, since a game object may have besides other components several MyGUI text components.",
				args = "(number occurrenceIndex)",
				returns = "(MyGUITextComponent)",
				valuetype = "MyGUITextComponent"
			},
			getMyGUITextComponent =
			{
				type = "function",
				description = "Gets the MyGUI text component. This can be used if the game object just has one MyGUI text component.",
				args = "()",
				returns = "(MyGUITextComponent)",
				valuetype = "MyGUITextComponent"
			},
			getMyGUIButtonComponentFromIndex =
			{
				type = "function",
				description = "Gets the MyGUI button component by the given occurence index, since a game object may have besides other components several MyGUI button components.",
				args = "(number occurrenceIndex)",
				returns = "(MyGUIButtonComponent)",
				valuetype = "MyGUIButtonComponent"
			},
			getMyGUIButtonComponent =
			{
				type = "function",
				description = "Gets the MyGUI button component. This can be used if the game object just has one MyGUI button component.",
				args = "()",
				returns = "(MyGUIButtonComponent)",
				valuetype = "MyGUIButtonComponent"
			},
			getMyGUICheckBoxComponentFromIndex =
			{
				type = "function",
				description = "Gets the MyGUI check box component by the given occurence index, since a game object may have besides other components several MyGUI check box components.",
				args = "(number occurrenceIndex)",
				returns = "(MyGUICheckBoxComponent)",
				valuetype = "MyGUICheckBoxComponent"
			},
			getMyGUICheckBoxComponent =
			{
				type = "function",
				description = "Gets the MyGUI check box component. This can be used if the game object just has one MyGUI check box component.",
				args = "()",
				returns = "(MyGUICheckBoxComponent)",
				valuetype = "MyGUICheckBoxComponent"
			},
			getMyGUIImageBoxComponentFromIndex =
			{
				type = "function",
				description = "Gets the MyGUI image box component by the given occurence index, since a game object may have besides other components several MyGUI image box components.",
				args = "(number occurrenceIndex)",
				returns = "(MyGUIImageBoxComponent)",
				valuetype = "MyGUIImageBoxComponent"
			},
			getMyGUIImageBoxComponent =
			{
				type = "function",
				description = "Gets the MyGUI image box component. This can be used if the game object just has one MyGUI image box component.",
				args = "()",
				returns = "(MyGUIImageBoxComponent)",
				valuetype = "MyGUIImageBoxComponent"
			},
			getMyGUIProgressBarComponentFromIndex =
			{
				type = "function",
				description = "Gets the MyGUI progress bar component by the given occurence index, since a game object may have besides other components several MyGUI progress bar components.",
				args = "(number occurrenceIndex)",
				returns = "(MyGUIProgressBarComponent)",
				valuetype = "MyGUIProgressBarComponent"
			},
			getMyGUIProgressBarComponent =
			{
				type = "function",
				description = "Gets the MyGUI progress bar component. This can be used if the game object just has one MyGUI progress bar component.",
				args = "()",
				returns = "(MyGUIProgressBarComponent)",
				valuetype = "MyGUIProgressBarComponent"
			},
			getMyGUIListBoxComponentFromIndex =
			{
				type = "function",
				description = "Gets the MyGUI list box component by the given occurence index, since a game object may have besides other components several MyGUI list box components.",
				args = "(number occurrenceIndex)",
				returns = "(MyGUIListBoxComponent)",
				valuetype = "MyGUIListBoxComponent"
			},
			getMyGUIListBoxComponent =
			{
				type = "function",
				description = "Gets the MyGUI list box component. This can be used if the game object just has one MyGUI list box component.",
				args = "()",
				returns = "(MyGUIListBoxComponent)",
				valuetype = "MyGUIListBoxComponent"
			},
			getMyGUIComboBoxComponentFromIndex =
			{
				type = "function",
				description = "Gets the MyGUI combo box component by the given occurence index, since a game object may have besides other components several MyGUI combo box components.",
				args = "(number occurrenceIndex)",
				returns = "(MyGUIComboBoxComponent)",
				valuetype = "MyGUIComboBoxComponent"
			},
			getMyGUIComboBoxComponent =
			{
				type = "function",
				description = "Gets the MyGUI combo box component. This can be used if the game object just has one MyGUI combo box component.",
				args = "()",
				returns = "(MyGUIComboBoxComponent)",
				valuetype = "MyGUIComboBoxComponent"
			},
			getMyGUIMessageBoxComponentFromIndex =
			{
				type = "function",
				description = "Gets the MyGUI message box component by the given occurence index, since a game object may have besides other components several MyGUI message box components.",
				args = "(number occurrenceIndex)",
				returns = "(MyGUIMessageBoxComponent)",
				valuetype = "MyGUIMessageBoxComponent"
			},
			getMyGUIMessageBoxComponent =
			{
				type = "function",
				description = "Gets the MyGUI message box component. This can be used if the game object just has one MyGUI message box component.",
				args = "()",
				returns = "(MyGUIMessageBoxComponent)",
				valuetype = "MyGUIMessageBoxComponent"
			},
			getMyGUIPositionControllerComponentFromIndex =
			{
				type = "function",
				description = "Gets the MyGUI position controller component by the given occurence index, since a game object may have besides other components several MyGUI position controller components.",
				args = "(number occurrenceIndex)",
				returns = "(MyGUIPositionControllerComponent)",
				valuetype = "MyGUIPositionControllerComponent"
			},
			getMyGUIPositionControllerComponent =
			{
				type = "function",
				description = "Gets the MyGUI position controller component. This can be used if the game object just has one MyGUI position controller component.",
				args = "()",
				returns = "(MyGUIPositionControllerComponent)",
				valuetype = "MyGUIPositionControllerComponent"
			},
			getMyGUIFadeAlphaControllerComponentFromIndex =
			{
				type = "function",
				description = "Gets the MyGUI fade alpha controller component by the given occurence index, since a game object may have besides other components several MyGUI fade alpha components.",
				args = "(number occurrenceIndex)",
				returns = "(MyGUIFadeAlphaControllerComponent)",
				valuetype = "MyGUIFadeAlphaControllerComponent"
			},
			getMyGUIFadeAlphaControllerComponent =
			{
				type = "function",
				description = "Gets the MyGUI fade alpha controller component. This can be used if the game object just has one MyGUI fade alpha controller component.",
				args = "()",
				returns = "(MyGUIFadeAlphaControllerComponent)",
				valuetype = "MyGUIFadeAlphaControllerComponent"
			},
			getMyGUIScrollingMessageControllerComponentFromIndex =
			{
				type = "function",
				description = "Gets the MyGUI scrolling message controller component by the given occurence index, since a game object may have besides other components several MyGUI scrolling message components.",
				args = "(number occurrenceIndex)",
				returns = "(MyGUIScrollingMessageControllerComponent)",
				valuetype = "MyGUIScrollingMessageControllerComponent"
			},
			getMyGUIFScrollingMessageControllerComponent =
			{
				type = "function",
				description = "Gets the MyGUI scrolling message controller component. This can be used if the game object just has one MyGUI scrolling message controller component.",
				args = "()",
				returns = "(MyGUIScrollingMessageControllerComponent)",
				valuetype = "MyGUIScrollingMessageControllerComponent"
			},
			getMyGUIEdgeHideControllerComponentFromIndex =
			{
				type = "function",
				description = "Gets the MyGUI hide edge controller component by the given occurence index, since a game object may have besides other components several MyGUI hide edge components.",
				args = "(number occurrenceIndex)",
				returns = "(MyGUIEdgeHideControllerComponent)",
				valuetype = "MyGUIEdgeHideControllerComponent"
			},
			getMyGUIEdgeHideControllerComponent =
			{
				type = "function",
				description = "Gets the MyGUI hide edge controller component. This can be used if the game object just has one MyGUI hide edge controller component.",
				args = "()",
				returns = "(MyGUIEdgeHideControllerComponent)",
				valuetype = "MyGUIEdgeHideControllerComponent"
			},
			getMyGUIRepeatClickControllerComponentFromIndex =
			{
				type = "function",
				description = "Gets the MyGUI repeat click controller component by the given occurence index, since a game object may have besides other components several MyGUI repeat click components.",
				args = "(number occurrenceIndex)",
				returns = "(MyGUIRepeatClickControllerComponent)",
				valuetype = "MyGUIRepeatClickControllerComponent"
			},
			getMyGUIRepeatClickControllerComponent =
			{
				type = "function",
				description = "Gets the MyGUI repeat click controller component. This can be used if the game object just has one MyGUI repeat click controller component.",
				args = "()",
				returns = "(MyGUIRepeatClickControllerComponent)",
				valuetype = "MyGUIRepeatClickControllerComponent"
			},
			getMyGUIMiniMapComponent =
			{
				type = "function",
				description = "Gets the MyGUI mini map component. Note: There can only be one mini map.",
				args = "()",
				returns = "(MyGUIMiniMapComponent)",
				valuetype = "MyGUIMiniMapComponent"
			},
			getLuaScriptComponent =
			{
				type = "function",
				description = "Gets the lua script component.",
				args = "()",
				returns = "(LuaScriptComponent)",
				valuetype = "LuaScriptComponent"
			},
			getAnimationComponentFromName =
			{
				type = "function",
				description = "Gets the animation component.",
				args = "(string name)",
				returns = "(AnimationComponent)",
				valuetype = "AnimationComponent"
			},
			getAttributesComponentFromName =
			{
				type = "function",
				description = "Gets the attributes component.",
				args = "(string name)",
				returns = "(AttributesComponent)",
				valuetype = "AttributesComponent"
			},
			getAttributeEffectComponentComponentFromName =
			{
				type = "function",
				description = "Gets the attribute effect component by the given occurence index, since a game object may have besides other components several animation components.",
				args = "(string name, number occurrenceIndex)",
				returns = "(AttributeEffectComponent)",
				valuetype = "AttributeEffectComponent"
			},
			getBuoyancyComponentFromName =
			{
				type = "function",
				description = "Gets the physics buoyancy component.",
				args = "(string name)",
				returns = "(BuoyancyComponent)",
				valuetype = "BuoyancyComponent"
			},
			getDistributedComponentFromName =
			{
				type = "function",
				description = "Gets the distributed component. Usage in a network scenario.",
				args = "(string name)",
				returns = "(DistributedComponent)",
				valuetype = "DistributedComponent"
			},
			getExitComponentFromName =
			{
				type = "function",
				description = "Gets the exit component.",
				args = "(string name)",
				returns = "(ExitComponent)",
				valuetype = "ExitComponent"
			},
			getAiMoveComponentFromName =
			{
				type = "function",
				description = "Gets the ai move component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(AiMoveComponent)",
				valuetype = "AiMoveComponent"
			},
			getAiMoveRandomlyComponentFromName =
			{
				type = "function",
				description = "Gets the ai move randomly component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(AiMoveRandomlyComponent)",
				valuetype = "AiMoveRandomlyComponent"
			},
			getAiPathFollowComponentFromName =
			{
				type = "function",
				description = "Gets the ai path follow component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(AiPathFollowComponent)",
				valuetype = "AiPathFollowComponent"
			},
			getAiWanderComponentFromName =
			{
				type = "function",
				description = "Gets the ai wander component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(AiWanderComponent)",
				valuetype = "AiWanderComponent"
			},
			getAiFlockingComponentFromName =
			{
				type = "function",
				description = "Gets the ai flocking component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(AiFlockingComponent)",
				valuetype = "AiFlockingComponent"
			},
			getAiRecastPathNavigationComponentFromName =
			{
				type = "function",
				description = "Gets the ai recast path navigation component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(AiRecastPathNavigationComponent)",
				valuetype = "AiRecastPathNavigationComponent"
			},
			getAiObstacleAvoidanceComponentFromName =
			{
				type = "function",
				description = "Gets the ai obstacle avoidance component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(AiObstacleAvoidanceComponent)",
				valuetype = "AiObstacleAvoidanceComponent"
			},
			getAiHideComponentFromName =
			{
				type = "function",
				description = "Gets the ai hide component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(AiHideComponent)",
				valuetype = "AiHideComponent"
			},
			getCameraBehaviorBaseComponentFromName =
			{
				type = "function",
				description = "Gets the camera behavior base component.",
				args = "(string name)",
				returns = "(CameraBehaviorBaseComponent)",
				valuetype = "CameraBehaviorBaseComponent"
			},
			getCameraBehaviorFirstPersonComponentFromName =
			{
				type = "function",
				description = "Gets the camera behavior first person component.",
				args = "(string name)",
				returns = "(CameraBehaviorFirstPersonComponent)",
				valuetype = "CameraBehaviorFirstPersonComponent"
			},
			getCameraBehaviorThirdPersonComponentFromName =
			{
				type = "function",
				description = "Gets the camera third first person component.",
				args = "(string name)",
				returns = "(CameraBehaviorThirdPersonComponent)",
				valuetype = "CameraBehaviorThirdPersonComponent"
			},
			getCameraBehaviorFollow2DComponentFromName =
			{
				type = "function",
				description = "Gets the camera follow 2D component.",
				args = "(string name)",
				returns = "(CameraBehaviorFollow2DComponent)",
				valuetype = "CameraBehaviorFollow2DComponent"
			},
			getCameraBehaviorZoomComponentFromName =
			{
				type = "function",
				description = "Gets the camera zoom component.",
				args = "(string name)",
				returns = "(CameraBehaviorZoomComponent)",
				valuetype = "CameraBehaviorZoomComponent"
			},
			getCameraComponentFromName =
			{
				type = "function",
				description = "Gets the camera component.",
				args = "(string name)",
				returns = "(CameraComponent)",
				valuetype = "CameraComponent"
			},
			getCompositorEffectBloomComponentFromName =
			{
				type = "function",
				description = "Gets the compositor effect bloom component.",
				args = "(string name)",
				returns = "(CompositorEffectBloomComponent)",
				valuetype = "CompositorEffectBloomComponent"
			},
			getCompositorEffectGlassComponentFromName =
			{
				type = "function",
				description = "Gets the compositor effect glass component.",
				args = "(string name)",
				returns = "(CompositorEffectGlassComponent)",
				valuetype = "CompositorEffectGlassComponent"
			},
			getCompositorEffectBlackAndWhiteComponentFromName =
			{
				type = "function",
				description = "Gets the compositor effect black and white component.",
				args = "(string name)",
				returns = "(CompositorEffectBlackAndWhiteComponent)",
				valuetype = "CompositorEffectBlackAndWhiteComponent"
			},
			getCompositorEffectColorComponentFromName =
			{
				type = "function",
				description = "Gets the compositor effect color component.",
				args = "(string name)",
				returns = "(CompositorEffectColorComponent)",
				valuetype = "CompositorEffectColorComponent"
			},
			getCompositorEffectEmbossedComponentFromName =
			{
				type = "function",
				description = "Gets the compositor effect embossed component.",
				args = "(string name)",
				returns = "(CompositorEffectEmbossedComponent)",
				valuetype = "CompositorEffectEmbossedComponent"
			},
			getCompositorEffectSharpenEdgesComponentFromName =
			{
				type = "function",
				description = "Gets the compositor effect sharpen edges component.",
				args = "(string name)",
				returns = "(CompositorEffectSharpenEdgesComponent)",
				valuetype = "CompositorEffectSharpenEdgesComponent"
			},
			getHdrEffectComponentFromName =
			{
				type = "function",
				description = "Gets hdr effect component.",
				args = "(string name)",
				returns = "(HdrEffectComponent)",
				valuetype = "HdrEffectComponent"
			},
			getDatablockPbsComponentFromName =
			{
				type = "function",
				description = "Gets the datablock pbs (physically based shading) component.",
				args = "(string name)",
				returns = "(DatablockPbsComponent)",
				valuetype = "DatablockPbsComponent"
			},
			getDatablockUnlitComponentFromName =
			{
				type = "function",
				description = "Gets the datablock unlit (without lighting) component.",
				args = "(string name)",
				returns = "(DatablockUnlitComponent)",
				valuetype = "DatablockUnlitComponent"
			},
			getDatablockTerraComponentFromName =
			{
				type = "function",
				description = "Gets the datablock terra component.",
				args = "(string name)",
				returns = "(DatablockTerraComponent)",
				valuetype = "DatablockTerraComponent"
			},
			getTerraComponentFromName =
			{
				type = "function",
				description = "Gets the terra component.",
				args = "(string name)",
				returns = "(TerraComponent)",
				valuetype = "TerraComponent"
			},
			getJointComponentFromName =
			{
				type = "function",
				description = "Gets the joint root component. Requirements: A physics component.",
				args = "(string name)",
				returns = "(JointComponent)",
				valuetype = "JointComponent"
			},
			getJointHingeComponentFromName =
			{
				type = "function",
				description = "Gets the joint hinge component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(JointHingeComponent)",
				valuetype = "JointHingeComponent"
			},
			getJointTargetTransformComponentFromName =
			{
				type = "function",
				description = "Gets the joint target transform component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(JointTargetTransformComponent)",
				valuetype = "JointTargetTransformComponent"
			},
			getJointHingeActuatorComponentFromName =
			{
				type = "function",
				description = "Gets the joint hinge actuator component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(JointHingeActuatorComponent)",
				valuetype = "JointHingeActuatorComponent"
			},
			getJointBallAndSocketComponentFromName =
			{
				type = "function",
				description = "Gets the joint ball and socket component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(JointBallAndSocketComponent)",
				valuetype = "JointBallAndSocketComponent"
			},
			getJointPointToPointComponentFromName =
			{
				type = "function",
				description = "Gets the joint point to point component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(JointPointToPointComponent)",
				valuetype = "JointPointToPointComponent"
			},
			getJointPinComponentFromName =
			{
				type = "function",
				description = "Gets the joint pin component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(JointPinComponent)",
				valuetype = "JointPinComponent"
			},
			getJointPlaneComponentFromName =
			{
				type = "function",
				description = "Gets the joint plane component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(JointPlaneComponent)",
				valuetype = "JointPlaneComponent"
			},
			getJointSpringComponentFromName =
			{
				type = "function",
				description = "Gets the joint spring component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(JointSpringComponent)",
				valuetype = "JointSpringComponent"
			},
			getJointAttractorComponentFromName =
			{
				type = "function",
				description = "Gets the joint attractor component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(JointAttractorComponent)",
				valuetype = "JointAttractorComponent"
			},
			getJointCorkScrewComponentFromName =
			{
				type = "function",
				description = "Gets the joint cork screw component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(JointCorkScrewComponent)",
				valuetype = "JointCorkScrewComponent"
			},
			getJointPassiveSliderComponentFromName =
			{
				type = "function",
				description = "Gets the joint passive slider component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(JointPassiveSliderComponent)",
				valuetype = "JointPassiveSliderComponent"
			},
			getJointSliderActuatorComponentFromName =
			{
				type = "function",
				description = "Gets the joint slider actuator component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(JointSliderActuatorComponent)",
				valuetype = "JointSliderActuatorComponent"
			},
			getJointSlidingContactComponentFromName =
			{
				type = "function",
				description = "Gets the joint sliding contact component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(JointSlidingContactComponent)",
				valuetype = "JointSlidingContactComponent"
			},
			getJointActiveSliderComponentFromName =
			{
				type = "function",
				description = "Gets the joint active slider component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(JointActiveSliderComponent)",
				valuetype = "JointActiveSliderComponent"
			},
			getJointMathSliderComponentFromName =
			{
				type = "function",
				description = "Gets the joint math slider component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(JointMathSliderComponent)",
				valuetype = "JointMathSliderComponent"
			},
			getJointKinematicComponentFromName =
			{
				type = "function",
				description = "Gets the joint kinematic component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(JointKinematicComponent)",
				valuetype = "JointKinematicComponent"
			},
			getJointPathFollowComponentFromName =
			{
				type = "function",
				description = "Gets the joint path follow component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(JointPathFollowComponent)",
				valuetype = "JointPathFollowComponent"
			},
			getJointDryRollingFrictionComponentFromName =
			{
				type = "function",
				description = "Gets the joint dry rolling friction component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(JointDryRollingFrictionComponent)",
				valuetype = "JointDryRollingFrictionComponent"
			},
			getJointGearComponentFromName =
			{
				type = "function",
				description = "Gets the joint gear component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(JointGearComponent)",
				valuetype = "JointGearComponent"
			},
			getJointRackAndPinionComponentFromName =
			{
				type = "function",
				description = "Gets the joint rack and pinion component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(JointRackAndPinionComponent)",
				valuetype = "JointRackAndPinionComponent"
			},
			getJointWormGearComponentFromName =
			{
				type = "function",
				description = "Gets the joint worm gear component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(JointWormGearComponent)",
				valuetype = "JointWormGearComponent"
			},
			getJointPulleyComponentFromName =
			{
				type = "function",
				description = "Gets the joint pulley component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(JointPulleyComponent)",
				valuetype = "JointPulleyComponent"
			},
			getJointUniversalComponentFromName =
			{
				type = "function",
				description = "Gets the joint universal component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(JointUniversalComponent)",
				valuetype = "JointUniversalComponent"
			},
			getJointUniversalActuatlorComponentFromName =
			{
				type = "function",
				description = "Gets the joint universal actuator component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(JointUniversalActuatorComponent)",
				valuetype = "JointUniversalActuatorComponent"
			},
			getJoint6DofComponentFromName =
			{
				type = "function",
				description = "Gets the joint 6 DOF component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(Joint6DofComponent)",
				valuetype = "Joint6DofComponent"
			},
			getJointMotorComponentFromName =
			{
				type = "function",
				description = "Gets the joint motor component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(JointMotorComponent)",
				valuetype = "JointMotorComponent"
			},
			getJointWheelComponentFromName =
			{
				type = "function",
				description = "Gets the joint wheel component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(JointWheelComponent)",
				valuetype = "JointWheelComponent"
			},
			getLightDirectionalComponentFromName =
			{
				type = "function",
				description = "Gets the light directional component.",
				args = "(string name)",
				returns = "(LightDirectionalComponent)",
				valuetype = "LightDirectionalComponent"
			},
			getLightPointComponentFromName =
			{
				type = "function",
				description = "Gets the light point component.",
				args = "(string name)",
				returns = "(LightPointComponent)",
				valuetype = "LightPointComponent"
			},
			getLightSpotComponentFromName =
			{
				type = "function",
				description = "Gets the light spot component.",
				args = "(string name)",
				returns = "(LightSpotComponent)",
				valuetype = "LightSpotComponent"
			},
			getLightAreaComponentFromName =
			{
				type = "function",
				description = "Gets the light area component.",
				args = "(string name)",
				returns = "(LightAreaComponent)",
				valuetype = "LightAreaComponent"
			},
			getFadeComponentFromName =
			{
				type = "function",
				description = "Gets the fade component.",
				args = "(string name)",
				returns = "(FadeComponent)",
				valuetype = "FadeComponent"
			},
			getNavMeshComponentFromName =
			{
				type = "function",
				description = "Gets the nav mesh component. Requirements: OgreRecast navigation must be activated.",
				args = "(string name)",
				returns = "(NavMeshComponent)",
				valuetype = "NavMeshComponent"
			},
			getParticleUniverseComponentFromName =
			{
				type = "function",
				description = "Gets the particle universe component by the given occurence index, since a game object may have besides other components several particle universe components.",
				args = "(string namenumber occurrenceIndex)",
				returns = "(ParticleUniverseComponent)",
				valuetype = "ParticleUniverseComponent"
			},
			getPlayerControllerComponentFromName =
			{
				type = "function",
				description = "Gets the base player controller component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(PlayerControllerJumpNRunComponent)",
				valuetype = "PlayerControllerJumpNRunComponent"
			},
			getPlayerControllerJumpNRunComponentFromName =
			{
				type = "function",
				description = "Gets the player controller Jump N Run component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(PlayerControllerJumpNRunComponent)",
				valuetype = "PlayerControllerJumpNRunComponent"
			},
			getPlayerControllerJumpNRunLuaComponentFromName =
			{
				type = "function",
				description = "Gets the player controller Jump N Run Lua component. Requirements: A physics active component and a LuaScriptComponent.",
				args = "(string name)",
				returns = "(PlayerControllerJumpNRunLuaComponent)",
				valuetype = "PlayerControllerJumpNRunLuaComponent"
			},
			getPlayerControllerClickToPointComponentFromName =
			{
				type = "function",
				description = "Gets the player controller click to point component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(PlayerControllerClickToPointComponent)",
				valuetype = "PlayerControllerClickToPointComponent"
			},
			getPhysicsComponentFromName =
			{
				type = "function",
				description = "Gets the physics component, which is the base for all other physics components.",
				args = "(string name)",
				returns = "(PhysicsComponent)",
				valuetype = "PhysicsComponent"
			},
			getPhysicsActiveComponentFromName =
			{
				type = "function",
				description = "Gets the physics active component.",
				args = "(string name)",
				returns = "(PhysicsActiveComponent)",
				valuetype = "PhysicsActiveComponent"
			},
			getPhysicsActiveCompoundComponentFromName =
			{
				type = "function",
				description = "Gets the physics active compound component.",
				args = "(string name)",
				returns = "(PhysicsActiveCompoundComponent)",
				valuetype = "PhysicsActiveCompoundComponent"
			},
			getPhysicsActiveDestructibleComponentFromName =
			{
				type = "function",
				description = "Gets the physics active destructable component.",
				args = "(string name)",
				returns = "(PhysicsActiveDestructibleComponent)",
				valuetype = "PhysicsActiveDestructibleComponent"
			},
			getPhysicsArtifactComponentFromName =
			{
				type = "function",
				description = "Gets the physics artifact component.",
				args = "(string name)",
				returns = "(PhysicsArtifactComponent)",
				valuetype = "PhysicsArtifactComponent"
			},
			getPhysicsRagDollComponentFromName =
			{
				type = "function",
				description = "Gets the physics ragdoll component.",
				args = "(string name)",
				returns = "(PhysicsRagDollComponent)",
				valuetype = "PhysicsRagDollComponent"
			},
			getPhysicsCompoundConnectionComponentFromName =
			{
				type = "function",
				description = "Gets the physics compound connection component.",
				args = "(string name)",
				returns = "(PhysicsCompoundConnectionComponent)",
				valuetype = "PhysicsCompoundConnectionComponent"
			},
			getPhysicsMaterialComponentFromName =
			{
				type = "function",
				description = "Gets the physics material component.",
				args = "(string name)",
				returns = "(PhysicsMaterialComponent)",
				valuetype = "PhysicsMaterialComponent"
			},
			getPhysicsPlayerControllerComponentFromName =
			{
				type = "function",
				description = "Gets the physics player controller component.",
				args = "(string name)",
				returns = "(PhysicsPlayerControllerComponent)",
				valuetype = "PhysicsPlayerControllerComponent"
			},
			getPhysicsActiveVehicleComponentFromName =
			{
				type = "function",
				description = "Gets the physics active vehicle component.",
				args = "(string name)",
				returns = "(PhysicsActiveVehicleComponent)",
				valuetype = "PhysicsActiveVehicleComponent"
			},
			getPlaneComponentFromName =
			{
				type = "function",
				description = "Gets the plane component.",
				args = "(string name)",
				returns = "(PlaneComponent)",
				valuetype = "PlaneComponent"
			},
			getSimpleSoundComponentFromName =
			{
				type = "function",
				description = "Gets the simple sound component.",
				args = "(string name)",
				returns = "(SimpleSoundComponent)",
				valuetype = "SimpleSoundComponent"
			},
			getSoundComponentFromName =
			{
				type = "function",
				description = "Gets the sound component.",
				args = "(string name)",
				returns = "(SoundComponent)",
				valuetype = "SoundComponent"
			},
			getSpawnComponentFromName =
			{
				type = "function",
				description = "Gets the spawn component.",
				args = "(string name)",
				returns = "(SpawnComponent)",
				valuetype = "SpawnComponent"
			},
			getVehicleComponentFromName =
			{
				type = "function",
				description = "Gets the physics vehicle component. Requirements: A physics active component.",
				args = "(string name)",
				returns = "(VehicleComponent)",
				valuetype = "VehicleComponent"
			},
			getAiLuaComponentFromName =
			{
				type = "function",
				description = "Gets the ai lua script component. Requirements: A physics active component and a lua script component.",
				args = "(string name)",
				returns = "(AiLuaComponent)",
				valuetype = "AiLuaComponent"
			},
			getPhysicsExplosionComponentFromName =
			{
				type = "function",
				description = "Gets the physics explosion component.",
				args = "(string name)",
				returns = "(PhysicsExplosionComponent)",
				valuetype = "PhysicsExplosionComponent"
			},
			getMeshDecalComponentFromName =
			{
				type = "function",
				description = "Gets the mesh decal component.",
				args = "(string name)",
				returns = "(MeshDecalComponent)",
				valuetype = "MeshDecalComponent"
			},
			getTagPointComponentFromName =
			{
				type = "function",
				description = "Gets the tag point component.",
				args = "(string name)",
				returns = "(TagPointComponent)",
				valuetype = "TagPointComponent"
			},
			getTimeTriggerComponentFromName =
			{
				type = "function",
				description = "Gets the time trigger component.",
				args = "(string name)",
				returns = "(TimeTriggerComponent)",
				valuetype = "TimeTriggerComponent"
			},
			getTimeLineComponentFromName =
			{
				type = "function",
				description = "Gets the time line component.",
				args = "(string name)",
				returns = "(TimeLineComponent)",
				valuetype = "TimeLineComponent"
			},
			getMoveMathFunctionComponentFromName =
			{
				type = "function",
				description = "Gets the mvoe math function component.",
				args = "(string name)",
				returns = "(MoveMathFunctionComponent)",
				valuetype = "MoveMathFunctionComponent"
			},
			getTagChildNodeComponentFromName =
			{
				type = "function",
				description = "Gets the tag child node component by the given occurence index, since a game object may have besides other components several tag child node components.",
				args = "(string namenumber occurrenceIndex)",
				returns = "(TagChildNodeComponent)",
				valuetype = "TagChildNodeComponent"
			},
			getNodeTrackComponentFromName =
			{
				type = "function",
				description = "Gets the node track component.",
				args = "(string name)",
				returns = "(NodeTrackComponent)",
				valuetype = "NodeTrackComponent"
			},
			getLineComponentFromName =
			{
				type = "function",
				description = "Gets the line component.",
				args = "(string name)",
				returns = "(LineComponent)",
				valuetype = "LineComponent"
			},
			getLinesComponentFromName =
			{
				type = "function",
				description = "Gets the lines component.",
				args = "(string name)",
				returns = "(LinesComponent)",
				valuetype = "LinesComponent"
			},
			getNodeComponentFromName =
			{
				type = "function",
				description = "Gets the node component.",
				args = "(string name)",
				returns = "(NodeComponent)",
				valuetype = "NodeComponent"
			},
			getManualObjectComponentFromName =
			{
				type = "function",
				description = "Gets the manual object component.",
				args = "(string name)",
				returns = "(ManualObjectComponent)",
				valuetype = "ManualObjectComponent"
			},
			getRectangleComponentFromName =
			{
				type = "function",
				description = "Gets the rectangle component.",
				args = "(string name)",
				returns = "(RectangleComponent)",
				valuetype = "RectangleComponent"
			},
			getValueBarComponentFromName =
			{
				type = "function",
				description = "Gets the value bar component.",
				args = "(string name)",
				returns = "(ValueBarComponent)",
				valuetype = "ValueBarComponent"
			},
			getBillboardComponentFromName =
			{
				type = "function",
				description = "Gets the billboard component.",
				args = "(string name)",
				returns = "(BillboardComponent)",
				valuetype = "BillboardComponent"
			},
			getRibbonTrailComponentFromName =
			{
				type = "function",
				description = "Gets the ribbon trail component.",
				args = "(string name)",
				returns = "(RibbonTrailComponent)",
				valuetype = "RibbonTrailComponent"
			},
			getMyGUIWindowComponentFromName =
			{
				type = "function",
				description = "Gets the MyGUI window component.",
				args = "(string name)",
				returns = "(MyGUIWindowComponent)",
				valuetype = "MyGUIWindowComponent"
			},
			getMyGUIItemBoxComponentFromName =
			{
				type = "function",
				description = "Gets the MyGUI item box component. This can be used for inventory item in conjunction with InventoryItemComponent.",
				args = "(string name)",
				returns = "(MyGUIItemBoxComponent)",
				valuetype = "MyGUIItemBoxComponent"
			},
			getInventoryItemComponentFromName =
			{
				type = "function",
				description = "Gets the inventory item component. This can be used for inventory item in conjunction with MyGUIItemBoxComponent.",
				args = "(string name)",
				returns = "(InventoryItemComponent)",
				valuetype = "InventoryItemComponent"
			},
			getMyGUITextComponentFromName =
			{
				type = "function",
				description = "Gets the MyGUI text component.",
				args = "(string name)",
				returns = "(MyGUITextComponent)",
				valuetype = "MyGUITextComponent"
			},
			getMyGUIButtonComponentFromName =
			{
				type = "function",
				description = "Gets the MyGUI button component.",
				args = "(string name)",
				returns = "(MyGUIButtonComponent)",
				valuetype = "MyGUIButtonComponent"
			},
			getMyGUICheckBoxComponentFromName =
			{
				type = "function",
				description = "Gets the MyGUI check box component.",
				args = "(string name)",
				returns = "(MyGUICheckBoxComponent)",
				valuetype = "MyGUICheckBoxComponent"
			},
			getMyGUIImageBoxComponentFromName =
			{
				type = "function",
				description = "Gets the MyGUI image box component.",
				args = "(string name)",
				returns = "(MyGUIImageBoxComponent)",
				valuetype = "MyGUIImageBoxComponent"
			},
			getMyGUIProgressBarComponentFromName =
			{
				type = "function",
				description = "Gets the MyGUI progress bar component.",
				args = "(string name)",
				returns = "(MyGUIProgressBarComponent)",
				valuetype = "MyGUIProgressBarComponent"
			},
			getMyGUIListBoxComponentFromName =
			{
				type = "function",
				description = "Gets the MyGUI list box component.",
				args = "(string name)",
				returns = "(MyGUIListBoxComponent)",
				valuetype = "MyGUIListBoxComponent"
			},
			getMyGUIComboBoxComponentFromName =
			{
				type = "function",
				description = "Gets the MyGUI combo box component.",
				args = "(string name)",
				returns = "(MyGUIComboBoxComponent)",
				valuetype = "MyGUIComboBoxComponent"
			},
			getMyGUIMessageBoxComponentFromName =
			{
				type = "function",
				description = "Gets the MyGUI message box component.",
				args = "(string name)",
				returns = "(MyGUIMessageBoxComponent)",
				valuetype = "MyGUIMessageBoxComponent"
			},
			getMyGUIPositionControllerComponentFromName =
			{
				type = "function",
				description = "Gets the MyGUI position controller component.",
				args = "(string name)",
				returns = "(MyGUIPositionControllerComponent)",
				valuetype = "MyGUIPositionControllerComponent"
			},
			getMyGUIFadeAlphaControllerComponentFromName =
			{
				type = "function",
				description = "Gets the MyGUI fade alpha controller component.",
				args = "(string name)",
				returns = "(MyGUIFadeAlphaControllerComponent)",
				valuetype = "MyGUIFadeAlphaControllerComponent"
			},
			getMyGUIFScrollingMessageControllerComponentFromName =
			{
				type = "function",
				description = "Gets the MyGUI scrolling message controller component.",
				args = "(string name)",
				returns = "(MyGUIScrollingMessageControllerComponent)",
				valuetype = "MyGUIScrollingMessageControllerComponent"
			},
			getMyGUIEdgeHideControllerComponentFromName =
			{
				type = "function",
				description = "Gets the MyGUI hide edge controller component.",
				args = "(string name)",
				returns = "(MyGUIEdgeHideControllerComponent)",
				valuetype = "MyGUIEdgeHideControllerComponent"
			},
			getMyGUIRepeatClickControllerComponentFromName =
			{
				type = "function",
				description = "Gets the MyGUI repeat click controller component.",
				args = "(string name)",
				returns = "(MyGUIRepeatClickControllerComponent)",
				valuetype = "MyGUIRepeatClickControllerComponent"
			},
			getMyGUIMiniMapComponentFromName =
			{
				type = "function",
				description = "Gets the MyGUI mini map component. Note: There can only be one mini map.",
				args = "(string name)",
				returns = "(MyGUIMiniMapComponent)",
				valuetype = "MyGUIMiniMapComponent"
			},
			getLuaScriptComponentFromName =
			{
				type = "function",
				description = "Gets the lua script component.",
				args = "(string name)",
				returns = "(LuaScriptComponent)",
				valuetype = "LuaScriptComponent"
			},
			getAiLuaGoalComponent =
			{
				type = "function",
				description = "Gets the ai lua goal component. This can be used if the game object just has one ai lua goal component.",
				args = "()",
				returns = "(AiLuaGoalComponent)",
				valuetype = "AiLuaGoalComponent"
			},
			getAiLuaGoalComponentFromName =
			{
				type = "function",
				description = "Gets the ai lua goal component.",
				args = "(string name)",
				returns = "(AiLuaGoalComponent)",
				valuetype = "AiLuaGoalComponent"
			},
			getAnimationComponentV22 =
			{
				type = "function",
				description = "Gets the component by the given occurence index, since a game object may this component maybe several times.",
				args = "(number occurrenceIndex)",
				returns = "(AnimationComponentV2)",
				valuetype = "AnimationComponentV2"
			},
			getAnimationComponentV2 =
			{
				type = "function",
				description = "Gets the component. This can be used if the game object this component just once.",
				args = "()",
				returns = "(AnimationComponentV2)",
				valuetype = "AnimationComponentV2"
			},
			getAnimationComponentV2FromName =
			{
				type = "function",
				description = "Gets the component from name.",
				args = "(string name)",
				returns = "(AnimationComponentV2)",
				valuetype = "AnimationComponentV2"
			},
			getAnimationSequenceComponent =
			{
				type = "function",
				description = "Gets the component. This can be used if the game object this component just once.",
				args = "()",
				returns = "(AnimationSequenceComponent)",
				valuetype = "AnimationSequenceComponent"
			},
			getAnimationSequenceComponentFromName =
			{
				type = "function",
				description = "Gets the component from name.",
				args = "(string name)",
				returns = "(AnimationSequenceComponent)",
				valuetype = "AnimationSequenceComponent"
			},
			getAreaOfInterestComponentFromIndex =
			{
				type = "function",
				description = "Gets the area of interest component by the given occurence index, since a game object may have besides other components several area of interest components.",
				args = "(number occurrenceIndex)",
				returns = "(AreaOfInterestComponent)",
				valuetype = "AreaOfInterestComponent"
			},
			getAreaOfInterestComponent =
			{
				type = "function",
				description = "Gets the area of interest component. This can be used if the game object just has one area of interest component.",
				args = "()",
				returns = "(AreaOfInterestComponent)",
				valuetype = "AreaOfInterestComponent"
			},
			getAreaOfInterestComponentFromName =
			{
				type = "function",
				description = "Gets the area of interest component.",
				args = "(string name)",
				returns = "(AreaOfInterestComponent)",
				valuetype = "AreaOfInterestComponent"
			},
			getAtmosphereComponent =
			{
				type = "function",
				description = "Gets the component. This can be used if the game object this component just once.",
				args = "()",
				returns = "(AtmosphereComponent)",
				valuetype = "AtmosphereComponent"
			},
			getAtmosphereComponentFromName =
			{
				type = "function",
				description = "Gets the component from name.",
				args = "(string name)",
				returns = "(AtmosphereComponent)",
				valuetype = "AtmosphereComponent"
			},
			getCrowdComponentFromIndex =
			{
				type = "function",
				description = "Gets the component by the given occurence index, since a game object may this component maybe several times.",
				args = "(number occurrenceIndex)",
				returns = "(CrowdComponent)",
				valuetype = "CrowdComponent"
			},
			getCrowdComponent =
			{
				type = "function",
				description = "Gets the component. This can be used if the game object this component just once.",
				args = "()",
				returns = "(CrowdComponent)",
				valuetype = "CrowdComponent"
			},
			getCrowdComponentFromName =
			{
				type = "function",
				description = "Gets the component from name.",
				args = "(string name)",
				returns = "(CrowdComponent)",
				valuetype = "CrowdComponent"
			},
			getCrowdComponentFromIndex =
			{
				type = "function",
				description = "Gets the component from occurrence index.",
				args = "(number index)",
				returns = "(CrowdComponent)",
				valuetype = "CrowdComponent"
			},
			getDistortionComponent =
			{
				type = "function",
				description = "Gets the component. This can be used if the game object this component just once.",
				args = "()",
				returns = "(DistortionComponent)",
				valuetype = "DistortionComponent"
			},
			getDistortionComponentFromName =
			{
				type = "function",
				description = "Gets the component from name.",
				args = "(string name)",
				returns = "(DistortionComponent)",
				valuetype = "DistortionComponent"
			},
			getGpuParticlesComponentFromIndex =
			{
				type = "function",
				description = "Gets the component by the given occurence index, since a game object may this component maybe several times.",
				args = "(number occurrenceIndex)",
				returns = "(GpuParticlesComponent)",
				valuetype = "GpuParticlesComponent"
			},
			getGpuParticlesComponent =
			{
				type = "function",
				description = "Gets the component. This can be used if the game object this component just once.",
				args = "()",
				returns = "(GpuParticlesComponent)",
				valuetype = "GpuParticlesComponent"
			},
			getGpuParticlesComponentFromName =
			{
				type = "function",
				description = "Gets the component from name.",
				args = "(string name)",
				returns = "(GpuParticlesComponent)",
				valuetype = "GpuParticlesComponent"
			},
			getJointFlexyPipeHandleComponent =
			{
				type = "function",
				description = "Gets the joint flexy pipe handle component. This can be used if the game object just has one joint flexy pipe handle component.",
				args = "()",
				returns = "(JointFlexyPipeHandleComponent)",
				valuetype = "JointFlexyPipeHandleComponent"
			},
			getJointFlexyPipeHandleComponentFromName =
			{
				type = "function",
				description = "Gets the joint flexy pipe handle component.",
				args = "(string name)",
				returns = "(JointFlexyPipeHandleComponent)",
				valuetype = "JointFlexyPipeHandleComponent"
			},
			getJoystickRemapComponent =
			{
				type = "function",
				description = "Gets the component. This can be used if the game object this component just once.",
				args = "()",
				returns = "(JoystickRemapComponent)",
				valuetype = "JoystickRemapComponent"
			},
			getJoystickRemapComponentFromName =
			{
				type = "function",
				description = "Gets the component from name.",
				args = "(string name)",
				returns = "(JoystickRemapComponent)",
				valuetype = "JoystickRemapComponent"
			},
			getKeyboardRemapComponent =
			{
				type = "function",
				description = "Gets the component. This can be used if the game object this component just once.",
				args = "()",
				returns = "(KeyboardRemapComponent)",
				valuetype = "KeyboardRemapComponent"
			},
			getKeyboardRemapComponentFromName =
			{
				type = "function",
				description = "Gets the component from name.",
				args = "(string name)",
				returns = "(KeyboardRemapComponent)",
				valuetype = "KeyboardRemapComponent"
			},
			getKeyholeEffectComponentFromIndex =
			{
				type = "function",
				description = "Gets the component by the given occurence index, since a game object may this component maybe several times.",
				args = "(number occurrenceIndex)",
				returns = "(KeyholeEffectComponent)",
				valuetype = "KeyholeEffectComponent"
			},
			getKeyholeEffectComponent =
			{
				type = "function",
				description = "Gets the component. This can be used if the game object this component just once.",
				args = "()",
				returns = "(KeyholeEffectComponent)",
				valuetype = "KeyholeEffectComponent"
			},
			getKeyholeEffectComponentFromName =
			{
				type = "function",
				description = "Gets the component from name.",
				args = "(string name)",
				returns = "(KeyholeEffectComponent)",
				valuetype = "KeyholeEffectComponent"
			},
			getMinimapComponentFromIndex =
			{
				type = "function",
				description = "Gets the component by the given occurence index, since a game object may this component maybe several times.",
				args = "(number occurrenceIndex)",
				returns = "(MinimapComponent)",
				valuetype = "MinimapComponent"
			},
			getMinimapComponent =
			{
				type = "function",
				description = "Gets the component. This can be used if the game object this component just once.",
				args = "()",
				returns = "(MinimapComponent)",
				valuetype = "MinimapComponent"
			},
			getMinimapComponentFromName =
			{
				type = "function",
				description = "Gets the component from name.",
				args = "(string name)",
				returns = "(MinimapComponent)",
				valuetype = "MinimapComponent"
			},
			getMyGuiSpriteComponentFromIndex =
			{
				type = "function",
				description = "Gets the component by the given occurence index, since a game object may this component maybe several times.",
				args = "(number occurrenceIndex)",
				returns = "(MyGuiSpriteComponent)",
				valuetype = "MyGuiSpriteComponent"
			},
			getMyGuiSpriteComponent =
			{
				type = "function",
				description = "Gets the component. This can be used if the game object this component just once.",
				args = "()",
				returns = "(MyGuiSpriteComponent)",
				valuetype = "MyGuiSpriteComponent"
			},
			getMyGuiSpriteComponentFromName =
			{
				type = "function",
				description = "Gets the component from name.",
				args = "(string name)",
				returns = "(MyGuiSpriteComponent)",
				valuetype = "MyGuiSpriteComponent"
			},
			getParticleFxComponentFromIndex =
			{
				type = "function",
				description = "Gets the component by the given occurence index, since a game object may this component maybe several times.",
				args = "(number occurrenceIndex)",
				returns = "(ParticleFxComponent)",
				valuetype = "ParticleFxComponent"
			},
			getParticleFxComponent =
			{
				type = "function",
				description = "Gets the component. This can be used if the game object this component just once.",
				args = "()",
				returns = "(ParticleFxComponent)",
				valuetype = "ParticleFxComponent"
			},
			getParticleFxComponentFromName =
			{
				type = "function",
				description = "Gets the component from name.",
				args = "(string name)",
				returns = "(ParticleFxComponent)",
				valuetype = "ParticleFxComponent"
			},
			getPickerComponentFromIndex =
			{
				type = "function",
				description = "Gets the component by the given occurence index, since a game object may this component maybe several times.",
				args = "(number occurrenceIndex)",
				returns = "(PickerComponent)",
				valuetype = "PickerComponent"
			},
			getPickerComponent =
			{
				type = "function",
				description = "Gets the component. This can be used if the game object this component just once.",
				args = "()",
				returns = "(PickerComponent)",
				valuetype = "PickerComponent"
			},
			getPickerComponentFromName =
			{
				type = "function",
				description = "Gets the component from name.",
				args = "(string name)",
				returns = "(PickerComponent)",
				valuetype = "PickerComponent"
			},
			getPurePursuitComponentFromIndex =
			{
				type = "function",
				description = "Gets the component by the given occurence index, since a game object may this component maybe several times.",
				args = "(number occurrenceIndex)",
				returns = "(PurePursuitComponent)",
				valuetype = "PurePursuitComponent"
			},
			getPurePursuitComponent =
			{
				type = "function",
				description = "Gets the component. This can be used if the game object this component just once.",
				args = "()",
				returns = "(PurePursuitComponent)",
				valuetype = "PurePursuitComponent"
			},
			getPurePursuitComponentFromName =
			{
				type = "function",
				description = "Gets the component from name.",
				args = "(string name)",
				returns = "(PurePursuitComponent)",
				valuetype = "PurePursuitComponent"
			},
			getRaceGoalComponentFromIndex =
			{
				type = "function",
				description = "Gets the component by the given occurence index, since a game object may this component maybe several times.",
				args = "(number occurrenceIndex)",
				returns = "(RaceGoalComponent)",
				valuetype = "RaceGoalComponent"
			},
			getRaceGoalComponent =
			{
				type = "function",
				description = "Gets the component. This can be used if the game object this component just once.",
				args = "()",
				returns = "(RaceGoalComponent)",
				valuetype = "RaceGoalComponent"
			},
			getRaceGoalComponentFromName =
			{
				type = "function",
				description = "Gets the component from name.",
				args = "(string name)",
				returns = "(RaceGoalComponent)",
				valuetype = "RaceGoalComponent"
			},
			getRandomImageShuffler2 =
			{
				type = "function",
				description = "Gets the component by the given occurence index, since a game object may this component maybe several times.",
				args = "(number occurrenceIndex)",
				returns = "(RandomImageShuffler)",
				valuetype = "RandomImageShuffler"
			},
			getRandomImageShuffler =
			{
				type = "function",
				description = "Gets the component. This can be used if the game object this component just once.",
				args = "()",
				returns = "(RandomImageShuffler)",
				valuetype = "RandomImageShuffler"
			},
			getRandomImageShufflerFromName =
			{
				type = "function",
				description = "Gets the component from name.",
				args = "(string name)",
				returns = "(RandomImageShuffler)",
				valuetype = "RandomImageShuffler"
			},
			getRect2DComponentFromIndex =
			{
				type = "function",
				description = "Gets the component by the given occurence index, since a game object may this component maybe several times.",
				args = "(number occurrenceIndex)",
				returns = "(Rect2DComponent)",
				valuetype = "Rect2DComponent"
			},
			getRect2DComponent =
			{
				type = "function",
				description = "Gets the component. This can be used if the game object this component just once.",
				args = "()",
				returns = "(Rect2DComponent)",
				valuetype = "Rect2DComponent"
			},
			getRect2DComponentFromName =
			{
				type = "function",
				description = "Gets the component from name.",
				args = "(string name)",
				returns = "(Rect2DComponent)",
				valuetype = "Rect2DComponent"
			},
			getRect2DComponentFromIndex =
			{
				type = "function",
				description = "Gets the component from occurrence index.",
				args = "(number index)",
				returns = "(Rect2DComponent)",
				valuetype = "Rect2DComponent"
			},
			getReferenceComponentFromIndex =
			{
				type = "function",
				description = "Gets the component by the given occurence index, since a game object may this component maybe several times.",
				args = "(number occurrenceIndex)",
				returns = "(ReferenceComponent)",
				valuetype = "ReferenceComponent"
			},
			getReferenceComponent =
			{
				type = "function",
				description = "Gets the component. This can be used if the game object this component just once.",
				args = "()",
				returns = "(ReferenceComponent)",
				valuetype = "ReferenceComponent"
			},
			getReferenceComponentFromName =
			{
				type = "function",
				description = "Gets the component from name.",
				args = "(string name)",
				returns = "(ReferenceComponent)",
				valuetype = "ReferenceComponent"
			},
			getSelectGameObjectsComponent =
			{
				type = "function",
				description = "Gets the component. This can be used if the game object this component just once.",
				args = "()",
				returns = "(SelectGameObjectsComponent)",
				valuetype = "SelectGameObjectsComponent"
			},
			getSelectGameObjectsComponentFromName =
			{
				type = "function",
				description = "Gets the component from name.",
				args = "(string name)",
				returns = "(SelectGameObjectsComponent)",
				valuetype = "SelectGameObjectsComponent"
			},
			getSpeechBubbleComponent =
			{
				type = "function",
				description = "Gets the component. This can be used if the game object this component just once.",
				args = "()",
				returns = "(SpeechBubbleComponent)",
				valuetype = "SpeechBubbleComponent"
			},
			getSpeechBubbleComponentFromName =
			{
				type = "function",
				description = "Gets the component from name.",
				args = "(string name)",
				returns = "(SpeechBubbleComponent)",
				valuetype = "SpeechBubbleComponent"
			},
			getSplitScreenComponentFromIndex =
			{
				type = "function",
				description = "Gets the component by the given occurence index, since a game object may this component maybe several times.",
				args = "(number occurrenceIndex)",
				returns = "(SplitScreenComponent)",
				valuetype = "SplitScreenComponent"
			},
			getSplitScreenComponent =
			{
				type = "function",
				description = "Gets the component. This can be used if the game object this component just once.",
				args = "()",
				returns = "(SplitScreenComponent)",
				valuetype = "SplitScreenComponent"
			},
			getSplitScreenComponentFromName =
			{
				type = "function",
				description = "Gets the component from name.",
				args = "(string name)",
				returns = "(SplitScreenComponent)",
				valuetype = "SplitScreenComponent"
			},
			getTransformEaseComponentFromIndex =
			{
				type = "function",
				description = "Gets the component by the given occurence index, since a game object may this component maybe several times.",
				args = "(number occurrenceIndex)",
				returns = "(TransformEaseComponent)",
				valuetype = "TransformEaseComponent"
			},
			getTransformEaseComponent =
			{
				type = "function",
				description = "Gets the component. This can be used if the game object this component just once.",
				args = "()",
				returns = "(TransformEaseComponent)",
				valuetype = "TransformEaseComponent"
			},
			getTransformEaseComponentFromName =
			{
				type = "function",
				description = "Gets the component from name.",
				args = "(string name)",
				returns = "(TransformEaseComponent)",
				valuetype = "TransformEaseComponent"
			},
			getTransformHistoryComponentFromIndex =
			{
				type = "function",
				description = "Gets the component by the given occurence index, since a game object may this component maybe several times.",
				args = "(number occurrenceIndex)",
				returns = "(TransformHistoryComponent)",
				valuetype = "TransformHistoryComponent"
			},
			getTransformHistoryComponent =
			{
				type = "function",
				description = "Gets the component. This can be used if the game object this component just once.",
				args = "()",
				returns = "(TransformHistoryComponent)",
				valuetype = "TransformHistoryComponent"
			},
			getTransformHistoryComponentFromName =
			{
				type = "function",
				description = "Gets the component from name.",
				args = "(string name)",
				returns = "(TransformHistoryComponent)",
				valuetype = "TransformHistoryComponent"
			}
		}
	},
	GameObjectComponent =
	{
		type = "class",
		description = "This is the base class for all components.",
		childs = 
		{
			getOwner =
			{
				type = "function",
				description = "Gets owner game object of this component.",
				args = "()",
				returns = "(GameObject)",
				valuetype = "GameObject"
			},
			connect =
			{
				type = "function",
				description = "Connects this game object for simulation start. Normally there is no need to call this function.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			disconnect =
			{
				type = "function",
				description = "Disconnects this game object when simulation has ended. Normally there is no need to call this function.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			getPosition =
			{
				type = "function",
				description = "Gets the position of this game object component.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setActivated =
			{
				type = "method",
				description = "Sets whether this game object component should be activated or not.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getOccurrenceIndex =
			{
				type = "function",
				description = "Gets the occurence index of game object component. That is, if a game object has several components of the same time, each component that occurs more than once has a different occurence index than 0.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getIndex =
			{
				type = "function",
				description = "Gets the index of game object component (at which position the component is in the list.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	GameObjectController =
	{
		type = "class",
		description = "The game object controller manages all game objects.",
		childs = 
		{
			deleteDelayedGameObject =
			{
				type = "method",
				description = "Deletes a game object by the given id after a delay of milliseconds.",
				args = "(string gameObjectId, number delayMS)",
				returns = "(nil)",
				valuetype = "nil"
			},
			deleteDelayedGameObject =
			{
				type = "method",
				description = "Deletes a game object by the given id after 1000 milliseconds.",
				args = "(string gameObjectId)",
				returns = "(nil)",
				valuetype = "nil"
			},
			deleteGameObject =
			{
				type = "method",
				description = "Deletes a game object by the given id immediately.",
				args = "(string gameObjectId)",
				returns = "(nil)",
				valuetype = "nil"
			},
			deleteGameObjectWithUndo =
			{
				type = "method",
				description = "Deletes the game object by id.Note: The game object by the given id will not be deleted immediately but in the next update turn. This function can be used e.g. from a component of the game object, to delete it later, so that the component which calls this function will be deleted to.",
				args = "(string gameObjectId)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getGameObjects =
			{
				type = "function",
				description = "Gets all game objects as lua table. Which can be traversed via 'for key, gameObject in gameObjects do'.",
				args = "()",
				returns = "(Table[number][GameObject])",
				valuetype = "Table[number][GameObject]"
			},
			getGameObjectFromId =
			{
				type = "function",
				description = "Gets a game object by the given id.",
				args = "(string gameObjectId)",
				returns = "(GameObject)",
				valuetype = "GameObject"
			},
			getClonedGameObjectFromPriorId =
			{
				type = "function",
				description = "Gets a cloned game object by its prior id. This can be used, when a game object has been cloned, it gets a new id, but a trace of its prior id can be made, so that a matching can be done, which game object has been cloned.",
				args = "(string gameObjectId)",
				returns = "(GameObject)",
				valuetype = "GameObject"
			},
			getGameObjectsFromCategory =
			{
				type = "function",
				description = "Gets all game objects that are belonging to the given category as lua table. Which can be traversed via 'for key, gameObject in gameObjects do'.",
				args = "(string category)",
				returns = "(Table[number][GameObject])",
				valuetype = "Table[number][GameObject]"
			},
			getGameObjectsFromComponent =
			{
				type = "function",
				description = "Gets all game objects that are belonging to the given category as lua table. Which can be traversed via 'for key, gameObject in gameObjects do'.",
				args = "(string componentClassName)",
				returns = "(Table[number][GameObject])",
				valuetype = "Table[number][GameObject]"
			},
			getGameObjectsControlledByClientId =
			{
				type = "function",
				description = "Gets all game objects that are belonging to the given client id in a network scenario as lua table. Which can be traversed via 'for key, gameObject in gameObjects do'.",
				args = "(number clientId)",
				returns = "(Table[number][GameObject])",
				valuetype = "Table[number][GameObject]"
			},
			getGameObjectsFromTagName =
			{
				type = "function",
				description = "Gets all game objects that are belonging to the given tag name as lua table. Which can be traversed via 'for key, gameObject in gameObjects do'.",
				args = "(string tagName)",
				returns = "(Table[number][GameObject])",
				valuetype = "Table[number][GameObject]"
			},
			getGameObjectFromReferenceId =
			{
				type = "function",
				description = "Gets a game object by the given reference id.",
				args = "(string referenceId)",
				returns = "(GameObject)",
				valuetype = "GameObject"
			},
			getGameObjectComponentsFromReferenceId =
			{
				type = "function",
				description = "Gets a list of game object components by the given reference id.",
				args = "(string referenceId)",
				returns = "(GameObjectComponent)",
				valuetype = "GameObjectComponent"
			},
			getAllCategoriesSoFar =
			{
				type = "function",
				description = "Gets all currently created categories as lua table. Which can be traversed via 'for key, category in categories do'.",
				args = "()",
				returns = "(Table[number][string])",
				valuetype = "Table[number][string]"
			},
			getIdsFromCategory =
			{
				type = "function",
				description = "Gets all game object ids that are belonging to the given category as lua table. Which can be traversed via 'for key, category in categories do'.",
				args = "(string category)",
				returns = "(Table[number][string])",
				valuetype = "Table[number][string]"
			},
			getOtherIdsFromCategory =
			{
				type = "function",
				description = "Gets all game object ids besides the excluded one that are belonging to the given category as lua table. Which can be traversed via 'for key, category in categories do'.",
				args = "(GameObject excludedGameObject, string category)",
				returns = "(Table[number][string])",
				valuetype = "Table[number][string]"
			},
			getGameObjectFromName =
			{
				type = "function",
				description = "Gets a game object from the given name.",
				args = "(string name)",
				returns = "(GameObject)",
				valuetype = "GameObject"
			},
			getGameObjectsFromNamePrefix =
			{
				type = "function",
				description = "Gets all game objects that are matching the given pattern as lua table. Which can be traversed via 'for key, gameObject in gameObjects do'.",
				args = "(string pattern)",
				returns = "(Table[number][GameObject])",
				valuetype = "Table[number][GameObject]"
			},
			getGameObjectFromNamePrefix =
			{
				type = "function",
				description = "Gets first game object that is matching the given pattern.",
				args = "(string pattern)",
				returns = "(GameObject)",
				valuetype = "GameObject"
			},
			getGameObjectFromNamePrefix =
			{
				type = "function",
				description = "Gets all game objects which contain at least one of this component class name sorted by name.",
				args = "(string pattern)",
				returns = "(Table[number][GameObject])",
				valuetype = "Table[number][GameObject]"
			},
			getCategoryId =
			{
				type = "function",
				description = "Gets id of the given category name.",
				args = "(string categoryName)",
				returns = "(number)",
				valuetype = "number"
			},
			getMaterialIDFromCategory =
			{
				type = "function",
				description = "Gets physics material id of the given category name. In order to connect physics components together for collision detection etc.",
				args = "(string categoryName)",
				returns = "(number)",
				valuetype = "number"
			},
			activatePlayerController =
			{
				type = "method",
				description = "Activates the given player component from the given game object id. If set to true, the given player component will be activated, else deactivated. Sets whether only one player instance can be controller. If set to false more player can be controlled, that is each player, that is currently selected.",
				args = "(boolean active, string id, boolean onlyOneActive)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getNextGameObject =
			{
				type = "function",
				description = "Gets the next game object from the given group id. The category ids for filtering. Using ALL_CATEGORIES_ID, everything is selectable.",
				args = "(number categoryIds)",
				returns = "(GameObject)",
				valuetype = "GameObject"
			},
			castGameObject =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(GameObject other)",
				returns = "(GameObject)",
				valuetype = "GameObject"
			},
			castAnimationComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(AnimationComponent other)",
				returns = "(AnimationComponent)",
				valuetype = "AnimationComponent"
			},
			castAttributesComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(AttributesComponent other)",
				returns = "(AttributesComponent)",
				valuetype = "AttributesComponent"
			},
			castAttributeEffectComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(AttributeEffectComponent other)",
				returns = "(AttributeEffectComponent)",
				valuetype = "AttributeEffectComponent"
			},
			castPhysicsBuoyancyComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(PhysicsBuoyancyComponent other)",
				returns = "(PhysicsBuoyancyComponent)",
				valuetype = "PhysicsBuoyancyComponent"
			},
			castPhysicsTriggerComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(PhysicsTriggerComponent other)",
				returns = "(PhysicsTriggerComponent)",
				valuetype = "PhysicsTriggerComponent"
			},
			castDistributedComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(DistributedComponent other)",
				returns = "(DistributedComponent)",
				valuetype = "DistributedComponent"
			},
			castExitComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(ExitComponent other)",
				returns = "(ExitComponent)",
				valuetype = "ExitComponent"
			},
			castGameObjectTitleComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(GameObjectTitleComponent other)",
				returns = "(GameObjectTitleComponent)",
				valuetype = "GameObjectTitleComponent"
			},
			castAiMoveComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(AiMoveComponent other)",
				returns = "(AiMoveComponent)",
				valuetype = "AiMoveComponent"
			},
			castAiMoveRandomlyComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(AiMoveRandomlyComponent other)",
				returns = "(AiMoveRandomlyComponent)",
				valuetype = "AiMoveRandomlyComponent"
			},
			castAiPathFollowComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(AiPathFollowComponent other)",
				returns = "(AiPathFollowComponent)",
				valuetype = "AiPathFollowComponent"
			},
			castAiWanderComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(AiWanderComponent other)",
				returns = "(AiWanderComponent)",
				valuetype = "AiWanderComponent"
			},
			castAiFlockingComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(AiFlockingComponent other)",
				returns = "(AiFlockingComponent)",
				valuetype = "AiFlockingComponent"
			},
			castAiRecastPathNavigationComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(AiRecastPathNavigationComponent other)",
				returns = "(AiRecastPathNavigationComponent)",
				valuetype = "AiRecastPathNavigationComponent"
			},
			castAiObstacleAvoidanceComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(AiObstacleAvoidanceComponent other)",
				returns = "(AiObstacleAvoidanceComponent)",
				valuetype = "AiObstacleAvoidanceComponent"
			},
			castAiHideComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(AiHideComponent other)",
				returns = "(AiHideComponent)",
				valuetype = "AiHideComponent"
			},
			castAiMoveComponent2D =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(AiMoveComponent2D other)",
				returns = "(AiMoveComponent2D)",
				valuetype = "AiMoveComponent2D"
			},
			castAiPathFollowComponent2D =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(AiPathFollowComponent2D other)",
				returns = "(AiPathFollowComponent2D)",
				valuetype = "AiPathFollowComponent2D"
			},
			castAiWanderComponent2D =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(AiWanderComponent2D other)",
				returns = "(AiWanderComponent2D)",
				valuetype = "AiWanderComponent2D"
			},
			castCameraBehaviorBaseComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(CameraBehaviorBaseComponent other)",
				returns = "(CameraBehaviorBaseComponent)",
				valuetype = "CameraBehaviorBaseComponent"
			},
			castCameraBehaviorFirstPersonComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(CameraBehaviorFirstPersonComponent other)",
				returns = "(CameraBehaviorFirstPersonComponent)",
				valuetype = "CameraBehaviorFirstPersonComponent"
			},
			castCameraBehaviorThirdPersonComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(CameraBehaviorThirdPersonComponent other)",
				returns = "(CameraBehaviorThirdPersonComponent)",
				valuetype = "CameraBehaviorThirdPersonComponent"
			},
			castCameraBehaviorFollow2DComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(CameraBehaviorFollow2DComponent other)",
				returns = "(CameraBehaviorFollow2DComponent)",
				valuetype = "CameraBehaviorFollow2DComponent"
			},
			castCameraBehaviorZoomComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(CameraBehaviorZoomComponent other)",
				returns = "(CameraBehaviorZoomComponent)",
				valuetype = "CameraBehaviorZoomComponent"
			},
			castCameraComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(CameraComponent other)",
				returns = "(CameraComponent)",
				valuetype = "CameraComponent"
			},
			castCompositorEffectBloomComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(CompositorEffectBloomComponent other)",
				returns = "(CompositorEffectBloomComponent)",
				valuetype = "CompositorEffectBloomComponent"
			},
			castCompositorEffectGlassComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(CompositorEffectGlassComponent other)",
				returns = "(CompositorEffectGlassComponent)",
				valuetype = "CompositorEffectGlassComponent"
			},
			castCompositorEffectOldTvComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(CompositorEffectOldTvComponent other)",
				returns = "(CompositorEffectOldTvComponent)",
				valuetype = "CompositorEffectOldTvComponent"
			},
			castCompositorEffectBlackAndWhiteComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(CompositorEffectBlackAndWhiteComponent other)",
				returns = "(CompositorEffectBlackAndWhiteComponent)",
				valuetype = "CompositorEffectBlackAndWhiteComponent"
			},
			castCompositorEffectColorComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(CompositorEffectColorComponent other)",
				returns = "(CompositorEffectColorComponent)",
				valuetype = "CompositorEffectColorComponent"
			},
			castCompositorEffectEmbossedComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(CompositorEffectEmbossedComponent other)",
				returns = "(CompositorEffectEmbossedComponent)",
				valuetype = "CompositorEffectEmbossedComponent"
			},
			castCompositorEffectSharpenEdgesComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(CompositorEffectSharpenEdgesComponent other)",
				returns = "(CompositorEffectSharpenEdgesComponent)",
				valuetype = "CompositorEffectSharpenEdgesComponent"
			},
			castHdrEffectComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(HdrEffectComponent other)",
				returns = "(HdrEffectComponent)",
				valuetype = "HdrEffectComponent"
			},
			castDatablockPbsComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(DatablockPbsComponent other)",
				returns = "(DatablockPbsComponent)",
				valuetype = "DatablockPbsComponent"
			},
			castDatablockUnlitComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(DatablockUnlitComponent other)",
				returns = "(DatablockUnlitComponent)",
				valuetype = "DatablockUnlitComponent"
			},
			castDatablockTerraComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(DatablockTerraComponent other)",
				returns = "(DatablockTerraComponent)",
				valuetype = "DatablockTerraComponent"
			},
			castTerraComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(TerraComponent other)",
				returns = "(TerraComponent)",
				valuetype = "TerraComponent"
			},
			castJointComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(JointComponent other)",
				returns = "(JointComponent)",
				valuetype = "JointComponent"
			},
			castJointHingeComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(JointHingeComponent other)",
				returns = "(JointHingeComponent)",
				valuetype = "JointHingeComponent"
			},
			castJointTargetTransformComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(JointTargetTransformComponent other)",
				returns = "(JointTargetTransformComponent)",
				valuetype = "JointTargetTransformComponent"
			},
			castJointHingeActuatorComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(JointHingeActuatorComponent other)",
				returns = "(JointHingeActuatorComponent)",
				valuetype = "JointHingeActuatorComponent"
			},
			castJointBallAndSocketComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(JointBallAndSocketComponent other)",
				returns = "(JointBallAndSocketComponent)",
				valuetype = "JointBallAndSocketComponent"
			},
			castJointPointToPointComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(JointPointToPointComponent other)",
				returns = "(JointPointToPointComponent)",
				valuetype = "JointPointToPointComponent"
			},
			castJointPinComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(JointPinComponent other)",
				returns = "(JointPinComponent)",
				valuetype = "JointPinComponent"
			},
			castJointPlaneComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(JointPlaneComponent other)",
				returns = "(JointPlaneComponent)",
				valuetype = "JointPlaneComponent"
			},
			castJointSpringComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(JointSpringComponent other)",
				returns = "(JointSpringComponent)",
				valuetype = "JointSpringComponent"
			},
			castJointAttractorComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(JointAttractorComponent other)",
				returns = "(JointAttractorComponent)",
				valuetype = "JointAttractorComponent"
			},
			castJointCorkScrewComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(JointCorkScrewComponent other)",
				returns = "(JointCorkScrewComponent)",
				valuetype = "JointCorkScrewComponent"
			},
			castJointPassiveSliderComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(JointPassiveSliderComponent other)",
				returns = "(JointPassiveSliderComponent)",
				valuetype = "JointPassiveSliderComponent"
			},
			castJointSliderActuatorComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(JointSliderActuatorComponent other)",
				returns = "(JointSliderActuatorComponent)",
				valuetype = "JointSliderActuatorComponent"
			},
			castJointSlidingContactComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(JointSlidingContactComponent other)",
				returns = "(JointSlidingContactComponent)",
				valuetype = "JointSlidingContactComponent"
			},
			castJointActiveSliderComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(JointActiveSliderComponent other)",
				returns = "(JointActiveSliderComponent)",
				valuetype = "JointActiveSliderComponent"
			},
			castJointMathSliderComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(JointMathSliderComponent other)",
				returns = "(JointMathSliderComponent)",
				valuetype = "JointMathSliderComponent"
			},
			castJointKinematicComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(JointKinematicComponent other)",
				returns = "(JointKinematicComponent)",
				valuetype = "JointKinematicComponent"
			},
			castJointDryRollingFrictionComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(JointDryRollingFrictionComponent other)",
				returns = "(JointDryRollingFrictionComponent)",
				valuetype = "JointDryRollingFrictionComponent"
			},
			castJointPathFollowComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(JointPathFollowComponent other)",
				returns = "(JointPathFollowComponent)",
				valuetype = "JointPathFollowComponent"
			},
			castJointGearComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(JointGearComponent other)",
				returns = "(JointGearComponent)",
				valuetype = "JointGearComponent"
			},
			castJointRackAndPinionComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(JointRackAndPinionComponent other)",
				returns = "(JointRackAndPinionComponent)",
				valuetype = "JointRackAndPinionComponent"
			},
			castJointWormGearComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(JointWormGearComponent other)",
				returns = "(JointWormGearComponent)",
				valuetype = "JointWormGearComponent"
			},
			castJointPulleyComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(JointPulleyComponent other)",
				returns = "(JointPulleyComponent)",
				valuetype = "JointPulleyComponent"
			},
			castJointUniversalComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(JointUniversalComponent other)",
				returns = "(JointUniversalComponent)",
				valuetype = "JointUniversalComponent"
			},
			castJointUniversalActuatorComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(JointUniversalActuatorComponent other)",
				returns = "(JointUniversalActuatorComponent)",
				valuetype = "JointUniversalActuatorComponent"
			},
			castJoint6DofComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(Joint6DofComponent other)",
				returns = "(Joint6DofComponent)",
				valuetype = "Joint6DofComponent"
			},
			castJointMotorComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(JointMotorComponent other)",
				returns = "(JointMotorComponent)",
				valuetype = "JointMotorComponent"
			},
			castJointWheelComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(JointWheelComponent other)",
				returns = "(JointWheelComponent)",
				valuetype = "JointWheelComponent"
			},
			castLightDirectionalComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(LightDirectionalComponent other)",
				returns = "(LightDirectionalComponent)",
				valuetype = "LightDirectionalComponent"
			},
			castLightPointComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(LightPointComponent other)",
				returns = "(LightPointComponent)",
				valuetype = "LightPointComponent"
			},
			castLightSpotComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(LightSpotComponent other)",
				returns = "(LightSpotComponent)",
				valuetype = "LightSpotComponent"
			},
			castLightAreaComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(LightAreaComponent other)",
				returns = "(LightAreaComponent)",
				valuetype = "LightAreaComponent"
			},
			castFadeComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(FadeComponent other)",
				returns = "(FadeComponent)",
				valuetype = "FadeComponent"
			},
			castNavMeshComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(NavMeshComponent other)",
				returns = "(NavMeshComponent)",
				valuetype = "NavMeshComponent"
			},
			castParticleUniverseComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(ParticleUniverseComponent other)",
				returns = "(ParticleUniverseComponent)",
				valuetype = "ParticleUniverseComponent"
			},
			castPlayerControllerComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(PlayerControllerComponent other)",
				returns = "(PlayerControllerComponent)",
				valuetype = "PlayerControllerComponent"
			},
			castPlayerControllerJumpNRunComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(PlayerControllerJumpNRunComponent other)",
				returns = "(PlayerControllerJumpNRunComponent)",
				valuetype = "PlayerControllerJumpNRunComponent"
			},
			castPlayerControllerJumpNRunLuaComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(PlayerControllerJumpNRunLuaComponent other)",
				returns = "(PlayerControllerJumpNRunLuaComponent)",
				valuetype = "PlayerControllerJumpNRunLuaComponent"
			},
			castPlayerControllerClickToPointComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(PlayerControllerClickToPointComponent other)",
				returns = "(PlayerControllerClickToPointComponent)",
				valuetype = "PlayerControllerClickToPointComponent"
			},
			castPhysicsComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(PhysicsComponent other)",
				returns = "(PhysicsComponent)",
				valuetype = "PhysicsComponent"
			},
			castPhysicsActiveComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(PhysicsActiveComponent other)",
				returns = "(PhysicsActiveComponent)",
				valuetype = "PhysicsActiveComponent"
			},
			castPhysicsActiveCompoundComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(PhysicsActiveCompoundComponent other)",
				returns = "(PhysicsActiveCompoundComponent)",
				valuetype = "PhysicsActiveCompoundComponent"
			},
			castPhysicsActiveDestructableComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(PhysicsActiveDestructableComponent other)",
				returns = "(PhysicsActiveDestructableComponent)",
				valuetype = "PhysicsActiveDestructableComponent"
			},
			castPhysicsActiveKinematicComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(PhysicsActiveKinematicComponent other)",
				returns = "(PhysicsActiveKinematicComponent)",
				valuetype = "PhysicsActiveKinematicComponent"
			},
			castPhysicsArtifactComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(PhysicsArtifactComponent other)",
				returns = "(PhysicsArtifactComponent)",
				valuetype = "PhysicsArtifactComponent"
			},
			castPhysicsRagDollComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(PhysicsRagDollComponent other)",
				returns = "(PhysicsRagDollComponent)",
				valuetype = "PhysicsRagDollComponent"
			},
			castPhysicsCompoundConnectionComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(PhysicsCompoundConnectionComponent other)",
				returns = "(PhysicsCompoundConnectionComponent)",
				valuetype = "PhysicsCompoundConnectionComponent"
			},
			castPhysicsMaterialComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(PhysicsMaterialComponent other)",
				returns = "(PhysicsMaterialComponent)",
				valuetype = "PhysicsMaterialComponent"
			},
			castPhysicsPlayerControllerComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(PhysicsPlayerControllerComponent other)",
				returns = "(PhysicsPlayerControllerComponent)",
				valuetype = "PhysicsPlayerControllerComponent"
			},
			castPlayerContact =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(PlayerContact other)",
				returns = "(PlayerContact)",
				valuetype = "PlayerContact"
			},
			castPhysicsActiveVehicleComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(PhysicsActiveVehicleComponent other)",
				returns = "(PhysicsActiveVehicleComponent)",
				valuetype = "PhysicsActiveVehicleComponent"
			},
			castPlaneComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(PlaneComponent other)",
				returns = "(PlaneComponent)",
				valuetype = "PlaneComponent"
			},
			castSimpleSoundComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(SimpleSoundComponent other)",
				returns = "(SimpleSoundComponent)",
				valuetype = "SimpleSoundComponent"
			},
			castSoundComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(SoundComponent other)",
				returns = "(SoundComponent)",
				valuetype = "SoundComponent"
			},
			castSpawnComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(SpawnComponent other)",
				returns = "(SpawnComponent)",
				valuetype = "SpawnComponent"
			},
			castVehicleComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(VehicleComponent other)",
				returns = "(VehicleComponent)",
				valuetype = "VehicleComponent"
			},
			castAiLuaComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(AiLuaComponent other)",
				returns = "(AiLuaComponent)",
				valuetype = "AiLuaComponent"
			},
			castPhysicsExplosionComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(PhysicsExplosionComponent other)",
				returns = "(PhysicsExplosionComponent)",
				valuetype = "PhysicsExplosionComponent"
			},
			castMeshDecalComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(MeshDecalComponent other)",
				returns = "(MeshDecalComponent)",
				valuetype = "MeshDecalComponent"
			},
			castTagPointComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(TagPointComponent other)",
				returns = "(TagPointComponent)",
				valuetype = "TagPointComponent"
			},
			castTimeTriggerComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(TimeTriggerComponent other)",
				returns = "(TimeTriggerComponent)",
				valuetype = "TimeTriggerComponent"
			},
			castTimeLineComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(TimeLineComponent other)",
				returns = "(TimeLineComponent)",
				valuetype = "TimeLineComponent"
			},
			castMoveMathFunctionComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(MoveMathFunctionComponent other)",
				returns = "(MoveMathFunctionComponent)",
				valuetype = "MoveMathFunctionComponent"
			},
			castTagChildNodeComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(TagChildNodeComponent other)",
				returns = "(TagChildNodeComponent)",
				valuetype = "TagChildNodeComponent"
			},
			castNodeTrackComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(NodeTrackComponent other)",
				returns = "(NodeTrackComponent)",
				valuetype = "NodeTrackComponent"
			},
			castLineComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(LineComponent other)",
				returns = "(LineComponent)",
				valuetype = "LineComponent"
			},
			castLinesComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(LinesComponent other)",
				returns = "(LinesComponent)",
				valuetype = "LinesComponent"
			},
			castNodeComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(NodeComponent other)",
				returns = "(NodeComponent)",
				valuetype = "NodeComponent"
			},
			castManualObjectComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(ManualObjectComponent other)",
				returns = "(ManualObjectComponent)",
				valuetype = "ManualObjectComponent"
			},
			castRectangleComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(RectangleComponent other)",
				returns = "(RectangleComponent)",
				valuetype = "RectangleComponent"
			},
			castValueBarComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(ValueBarComponent other)",
				returns = "(ValueBarComponent)",
				valuetype = "ValueBarComponent"
			},
			castBillboardComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(BillboardComponent other)",
				returns = "(BillboardComponent)",
				valuetype = "BillboardComponent"
			},
			castRibbonTrailComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(RibbonTrailComponent other)",
				returns = "(RibbonTrailComponent)",
				valuetype = "RibbonTrailComponent"
			},
			castMyGUIWindowComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(MyGUIWindowComponent other)",
				returns = "(MyGUIWindowComponent)",
				valuetype = "MyGUIWindowComponent"
			},
			castMyGUITextComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(MyGUITextComponent other)",
				returns = "(MyGUITextComponent)",
				valuetype = "MyGUITextComponent"
			},
			castMyGUIButtonComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(MyGUIButtonComponent other)",
				returns = "(MyGUIButtonComponent)",
				valuetype = "MyGUIButtonComponent"
			},
			castMyGUICheckBoxComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(MyGUICheckBoxComponent other)",
				returns = "(MyGUICheckBoxComponent)",
				valuetype = "MyGUICheckBoxComponent"
			},
			castMyGUIImageBoxComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(MyGUIImageBoxComponent other)",
				returns = "(MyGUIImageBoxComponent)",
				valuetype = "MyGUIImageBoxComponent"
			},
			castMyGUIProgressBarComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(MyGUIProgressBarComponent other)",
				returns = "(MyGUIProgressBarComponent)",
				valuetype = "MyGUIProgressBarComponent"
			},
			castMyGUIListBoxComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(MyGUIListBoxComponent other)",
				returns = "(MyGUIListBoxComponent)",
				valuetype = "MyGUIListBoxComponent"
			},
			castMyGUIComboBoxComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(MyGUIComboBoxComponent other)",
				returns = "(MyGUIComboBoxComponent)",
				valuetype = "MyGUIComboBoxComponent"
			},
			castMyGUIMessageBoxComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(MyGUIMessageBoxComponent other)",
				returns = "(MyGUIMessageBoxComponent)",
				valuetype = "MyGUIMessageBoxComponent"
			},
			castMyGUIPositionControllerComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(MyGUIPositionControllerComponent other)",
				returns = "(MyGUIPositionControllerComponent)",
				valuetype = "MyGUIPositionControllerComponent"
			},
			castMyGUIFadeAlphaControllerComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(MyGUIFadeAlphaControllerComponent other)",
				returns = "(MyGUIFadeAlphaControllerComponent)",
				valuetype = "MyGUIFadeAlphaControllerComponent"
			},
			castMyGUIScrollingMessageControllerComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(MyGUIScrollingMessageControllerComponent other)",
				returns = "(MyGUIScrollingMessageControllerComponent)",
				valuetype = "MyGUIScrollingMessageControllerComponent"
			},
			castMyGUIEdgeHideControllerComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(MyGUIEdgeHideControllerComponent other)",
				returns = "(MyGUIEdgeHideControllerComponent)",
				valuetype = "MyGUIEdgeHideControllerComponent"
			},
			castMyGUIRepeatClickControllerComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(MyGUIRepeatClickControllerComponent other)",
				returns = "(MyGUIRepeatClickControllerComponent)",
				valuetype = "MyGUIRepeatClickControllerComponent"
			},
			castMyGUIItemBoxComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(MyGUIItemBoxComponent other)",
				returns = "(MyGUIItemBoxComponent)",
				valuetype = "MyGUIItemBoxComponent"
			},
			castMyGUIMiniMapComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(MyGUIMiniMapComponent other)",
				returns = "(MyGUIMiniMapComponent)",
				valuetype = "MyGUIMiniMapComponent"
			},
			castAiLuaGoalComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(AiLuaGoalComponent other)",
				returns = "(AiLuaGoalComponent)",
				valuetype = "AiLuaGoalComponent"
			},
			castGoalResult =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(GoalResult other)",
				returns = "(GoalResult)",
				valuetype = "GoalResult"
			},
			castAnimationComponentV2 =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(AnimationComponentV2 other)",
				returns = "(AnimationComponentV2)",
				valuetype = "AnimationComponentV2"
			},
			castAnimationSequenceComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(AnimationSequenceComponent other)",
				returns = "(AnimationSequenceComponent)",
				valuetype = "AnimationSequenceComponent"
			},
			castAreaOfInterestComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(AreaOfInterestComponent other)",
				returns = "(AreaOfInterestComponent)",
				valuetype = "AreaOfInterestComponent"
			},
			castAtmosphereComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(AtmosphereComponent other)",
				returns = "(AtmosphereComponent)",
				valuetype = "AtmosphereComponent"
			},
			castCrowdComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(CrowdComponent other)",
				returns = "(CrowdComponent)",
				valuetype = "CrowdComponent"
			},
			castDistortionComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(DistortionComponent other)",
				returns = "(DistortionComponent)",
				valuetype = "DistortionComponent"
			},
			castGpuParticlesComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(GpuParticlesComponent other)",
				returns = "(GpuParticlesComponent)",
				valuetype = "GpuParticlesComponent"
			},
			castJointFlexyPipeHandleComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(JointFlexyPipeHandleComponent other)",
				returns = "(JointFlexyPipeHandleComponent)",
				valuetype = "JointFlexyPipeHandleComponent"
			},
			castJoystickRemapComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(JoystickRemapComponent other)",
				returns = "(JoystickRemapComponent)",
				valuetype = "JoystickRemapComponent"
			},
			castKeyboardRemapComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(KeyboardRemapComponent other)",
				returns = "(KeyboardRemapComponent)",
				valuetype = "KeyboardRemapComponent"
			},
			castKeyholeEffectComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(KeyholeEffectComponent other)",
				returns = "(KeyholeEffectComponent)",
				valuetype = "KeyholeEffectComponent"
			},
			castMinimapComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(MinimapComponent other)",
				returns = "(MinimapComponent)",
				valuetype = "MinimapComponent"
			},
			castMyGuiSpriteComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(MyGuiSpriteComponent other)",
				returns = "(MyGuiSpriteComponent)",
				valuetype = "MyGuiSpriteComponent"
			},
			castParticleFxComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(ParticleFxComponent other)",
				returns = "(ParticleFxComponent)",
				valuetype = "ParticleFxComponent"
			},
			castPickerComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(PickerComponent other)",
				returns = "(PickerComponent)",
				valuetype = "PickerComponent"
			},
			castPurePursuitComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(PurePursuitComponent other)",
				returns = "(PurePursuitComponent)",
				valuetype = "PurePursuitComponent"
			},
			castRaceGoalComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(RaceGoalComponent other)",
				returns = "(RaceGoalComponent)",
				valuetype = "RaceGoalComponent"
			},
			castRandomImageShuffler =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(RandomImageShuffler other)",
				returns = "(RandomImageShuffler)",
				valuetype = "RandomImageShuffler"
			},
			castRect2DComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(Rect2DComponent other)",
				returns = "(Rect2DComponent)",
				valuetype = "Rect2DComponent"
			},
			castReferenceComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(ReferenceComponent other)",
				returns = "(ReferenceComponent)",
				valuetype = "ReferenceComponent"
			},
			castSelectGameObjectsComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(SelectGameObjectsComponent other)",
				returns = "(SelectGameObjectsComponent)",
				valuetype = "SelectGameObjectsComponent"
			},
			castSpeechBubbleComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(SpeechBubbleComponent other)",
				returns = "(SpeechBubbleComponent)",
				valuetype = "SpeechBubbleComponent"
			},
			castSplitScreenComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(SplitScreenComponent other)",
				returns = "(SplitScreenComponent)",
				valuetype = "SplitScreenComponent"
			},
			castTransformEaseComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(TransformEaseComponent other)",
				returns = "(TransformEaseComponent)",
				valuetype = "TransformEaseComponent"
			},
			castTransformHistoryComponent =
			{
				type = "function",
				description = "Casts an incoming type from function for lua auto completion.",
				args = "(TransformHistoryComponent other)",
				returns = "(TransformHistoryComponent)",
				valuetype = "TransformHistoryComponent"
			}
		}
	},
	GameObjectTitleComponent =
	{
		type = "class",
		description = "Usage: Adds a specified text to the GameObject visible in the scene.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setCaption =
			{
				type = "method",
				description = "Sets the caption text to be displayed.",
				args = "(string caption)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setAlwaysPresent =
			{
				type = "method",
				description = "Sets whether the object title text should always be visible, even if the game object is hidden behind another one.",
				args = "(boolean alwaysPresent)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setCharHeight =
			{
				type = "method",
				description = "Sets the height of the characters (default value is 0.2).",
				args = "(number charHeight)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setColor =
			{
				type = "method",
				description = "Sets the color (with alpha) for the title.",
				args = "(Vector4 color)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setAlignment =
			{
				type = "method",
				description = "Sets the alignment for the title (default value is Vector2(1, 1)).",
				args = "(Vector2 alignment)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setOffsetPosition =
			{
				type = "method",
				description = "Sets the offset position for the title. Note: Normally the title is placed at the same position as its carrying game object. Hence setting an offset is necessary.",
				args = "(Vector3 offsetPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getOffsetPosition =
			{
				type = "function",
				description = "Gets the offset position of the title.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setOffsetOrientation =
			{
				type = "method",
				description = "Sets the offset orientation vector in degrees for the title. Note: Orientation is set in the form: (degreeX, degreeY, degreeZ).",
				args = "(Vector3 offsetOrientation)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getOffsetOrientation =
			{
				type = "function",
				description = "Gets the offset orientation vector in degrees of the title. Orientation is in the form: (degreeX, degreeY, degreeZ)",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setLookAtCamera =
			{
				type = "method",
				description = "Sets whether to orientate the text always at the camera.",
				args = "(boolean lookAtCamera)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	GameProgressModule =
	{
		type = "class",
		description = "Class that is reponsible for the game progress specific topics like switching a scene at runtime or loading saving the game.",
		childs = 
		{
			getCurrentWorldName =
			{
				type = "function",
				description = "Gets the current world name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			saveProgress =
			{
				type = "method",
				description = "Saves the current progress for all game objects with its attribute components. Optionally crypts the content, so that it is not readable anymore. Optionally can saves a whole scene snapshot",
				args = "(string saveName, boolean crypted, boolean sceneSnapshot)",
				returns = "(nil)",
				valuetype = "nil"
			},
			saveValue =
			{
				type = "function",
				description = "Saves a value for the given game object id and its attribute index. Optionally crypts the content, so that it is not readable anymore.",
				args = "(string saveName, string gameObjectId, number attributeIndex, boolean crypted)",
				returns = "(string)",
				valuetype = "string"
			},
			saveValues =
			{
				type = "function",
				description = "Saves all values for the given game object id and its attribute components. Optionally crypts the content, so that it is not readable anymore.",
				args = "(string saveName, string gameObjectId, boolean crypted)",
				returns = "(string)",
				valuetype = "string"
			},
			loadProgress =
			{
				type = "function",
				description = "Loads all values for all game objects with attributes components for the given save name. Optionally can load a whole scene snapshot. If the scene is in the snapshot is the same as the current one, just values are set. Else first a whole new scene is loaded and after that the snapshot on the top. Returns false, if no save file could be found.",
				args = "(string saveName, boolean sceneSnapshot, boolean showProgress)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			loadValue =
			{
				type = "function",
				description = "Loads a value for the given game object id and attribute index and the given save name. Returns false, if no save file could be found or index or game object id is invalid.",
				args = "(string saveName, string gameObjectId, number attributeIndex)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			loadValue =
			{
				type = "function",
				description = "Loads all values for the given game object id and the given save name. Returns false, if no save file could be found or game object id is invalid.",
				args = "(string saveName, string gameObjectId)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			getGlobalValue =
			{
				type = "function",
				description = "Gets the Variant global value from attribute name. Note: Global values are stored directly in GameProgressModule. Note: Global values are stored directly in GameProgressModule.They can be used for the whole game logic, like which boss has been defeated etc.",
				args = "(string attributeName)",
				returns = "(Variant)",
				valuetype = "Variant"
			},
			setGlobalValue =
			{
				type = "function",
				description = "Sets the bool value for the given attribute name and returns the global Variant. Note: Global values are stored directly in GameProgressModule. They can be used for the whole game logic, like which boss has been defeated etc.",
				args = "(string attributeName, boolean value)",
				returns = "(Variant)",
				valuetype = "Variant"
			},
			setGlobalValue =
			{
				type = "function",
				description = "Sets the number value for the given attribute name and returns the global Variant. Note: Global values are stored directly in GameProgressModule. They can be used for the whole game logic, like which boss has been defeated etc.",
				args = "(string attributeName, number value)",
				returns = "(Variant)",
				valuetype = "Variant"
			},
			setGlobalValue =
			{
				type = "function",
				description = "Sets the string value for the given attribute name and returns the global Variant. Note: Global values are stored directly in GameProgressModule. They can be used for the whole game logic, like which boss has been defeated etc.",
				args = "(string attributeName, string value)",
				returns = "(Variant)",
				valuetype = "Variant"
			},
			setGlobalValue =
			{
				type = "function",
				description = "Sets the Vector2 value for the given attribute name and returns the global Variant. Note: Global values are stored directly in GameProgressModule. They can be used for the whole game logic, like which boss has been defeated etc.",
				args = "(string attributeName, Vector2 value)",
				returns = "(Variant)",
				valuetype = "Variant"
			},
			setGlobalValue =
			{
				type = "function",
				description = "Sets the Vector3 value for the given attribute name and returns the global Variant. Note: Global values are stored directly in GameProgressModule. They can be used for the whole game logic, like which boss has been defeated etc.",
				args = "(string attributeName, Vector3 value)",
				returns = "(Variant)",
				valuetype = "Variant"
			},
			setGlobalValue =
			{
				type = "function",
				description = "Sets the Vector4 value for the given attribute name and returns the global Variant. Note: Global values are stored directly in GameProgressModule. They can be used for the whole game logic, like which boss has been defeated etc.",
				args = "(string attributeName, Vector4 value)",
				returns = "(Variant)",
				valuetype = "Variant"
			},
			changeWorld =
			{
				type = "method",
				description = "Changes the current world to the new given one.",
				args = "(string worldName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			changeWorldShowProgress =
			{
				type = "method",
				description = "Changes the current world to the new given one. Also shows the loading progress.",
				args = "(string worldName)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	GlobalId =
	{
		type = "class",
		description = "GlobalId class",
		childs = 
		{
			ALL_CATEGORIES_ID =
			{
				type = "value"
			},
			MAIN_GAMEOBJECT_ID =
			{
				type = "value"
			},
			MAIN_CAMERA_ID =
			{
				type = "value"
			},
			MAIN_LIGHT_ID =
			{
				type = "value"
			}
		}
	},
	GoalResult =
	{
		type = "class",
		description = "Class representing the result of a goals execution.",
		childs = 
		{
			GoalResult() =
			{
				type = "value"
			},
			setStatus =
			{
				type = "method",
				description = "Sets the status of the goal. Use one of the following states: ACTIVE, INACTIVE, COMPLETED, FAILED.",
				args = "(LuaGoalState status)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getStatus =
			{
				type = "function",
				description = "Gets the current status of the goal.",
				args = "()",
				returns = "(LuaGoalState)",
				valuetype = "LuaGoalState"
			},
			LuaGoalState =
			{
				type = "value"
			},
	},
	GpuParticlesComponent =
	{
		type = "class",
		description = "This component is a particle effect system, which runs on the gpu and is really performant.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets whether this component should be activated or not (Start the particle effect).",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this component is activated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setTemplateName =
			{
				type = "method",
				description = "Sets the particle template name. The name must be recognized by the resource system, else the particle effect cannot be played.",
				args = "(string particleTemplateName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTemplateName =
			{
				type = "function",
				description = "Gets currently used particle template name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setRepeat =
			{
				type = "method",
				description = "Sets whether the current particle effect should be repeated when finished.",
				args = "(boolean repeat)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRepeat =
			{
				type = "function",
				description = "Gets whether the current particle effect will be repeated when finished.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setPlayTimeMS =
			{
				type = "method",
				description = "Sets particle play time in milliseconds, how long the particle effect should be played.",
				args = "(number particlePlayTimeMS)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPlayTimeMS =
			{
				type = "function",
				description = "Gets particle play time in milliseconds.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setOffsetPosition =
			{
				type = "method",
				description = "Sets offset position of the particle effect at which it should be played away from the game object.",
				args = "(Vector3 offsetPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getOffsetPosition =
			{
				type = "function",
				description = "Gets offset position of the particle effect.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setOffsetOrientation =
			{
				type = "method",
				description = "Sets offset orientation (as vector3(degree, degree, degree)) of the particle effect at which it should be played away from the game object.",
				args = "(Vector3 orientation)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setScale =
			{
				type = "method",
				description = "Sets the scale (size) of the particle effect.",
				args = "(Vector3 scale)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getScale =
			{
				type = "function",
				description = "Gets scale of the particle effect.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setPlaySpeed =
			{
				type = "method",
				description = "Sets particle play speed. E.g. 2 will play the particle at double speed.",
				args = "(number playSpeed)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPlaySpeed =
			{
				type = "function",
				description = "Gets particle play play speed.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			isPlaying =
			{
				type = "function",
				description = "Gets whether the particle is playing. Note: This affects the value of @PlayTimeMS.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setGlobalPosition =
			{
				type = "method",
				description = "Sets a global play position for the particle.",
				args = "(Vector3 globalPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setGlobalOrientation =
			{
				type = "method",
				description = "Sets a global player orientation (as vector3(degree, degree, degree)) of the particle effect.",
				args = "(Vector3 globalOrientation)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	HdrEffectComponent =
	{
		type = "class",
		description = "Usage: Enables HDR with several predefined profiles and possibility to create own HDR calibration. Requirements: This component can only be set directly under a workspace component.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setEffectName =
			{
				type = "method",
				description = "Sets the hdr effect name. Possible values are: 'Bright, sunny day', 'Scary Night', 'Average, slightly hazy day', 'Heavy overcast day', 'Gibbous moon night', 'JJ Abrams style', 'Custom'Note: If any value is manipulated manually, the effect name will be set to 'Custom'.",
				args = "(string effectName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getEffectName =
			{
				type = "function",
				description = "Gets currently set effect name. Possible values are : 'Bright, sunny day', 'Scary Night', 'Average, slightly hazy day', 'Heavy overcast day', 'Gibbous moon night', 'JJ Abrams style', 'Custom'Note: If any value is manipulated manually, the effect name will be set to 'Custom'.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setSkyColor =
			{
				type = "method",
				description = "Sets sky color.",
				args = "(Vector3 skyColor)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSkyColor =
			{
				type = "function",
				description = "Gets the sky color.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setUpperHemisphere =
			{
				type = "method",
				description = "Sets the ambient color when the surface normal is close to hemisphereDir.",
				args = "(Vector3 upperHemisphere)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getUpperHemisphere =
			{
				type = "function",
				description = "Gets the upper hemisphere color.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setLowerHemisphere =
			{
				type = "method",
				description = "Sets the ambient color when the surface normal is pointing away from hemisphereDir.",
				args = "(Vector3 lowerHemisphere)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getLowerHemisphere =
			{
				type = "function",
				description = "Gets the sky color.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setSunPower =
			{
				type = "method",
				description = "Sets the sun power scale. Note: This is applied on the 'SunLight'.",
				args = "(number sunPower)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSunPower =
			{
				type = "function",
				description = "Gets the sun power scale.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setExposure =
			{
				type = "method",
				description = "Modifies the HDR Materials for the new exposure parameters. By default the HDR implementation will try to auto adjust the exposure based on the scene's average luminance. If left unbounded, even the darkest scenes can look well lit and the brigthest scenes appear too normal. These parameters are useful to prevent the auto exposure from jumping too much from one extreme to the otherand provide a consistent experience within the same lighting conditions. (e.g.you may want to change the params when going from indoors to outdoors)The smaller the gap between minAutoExposure & maxAutoExposure, the less the auto exposure tries to auto adjust to the scene's lighting conditions. The first value is exposure. Valid range is [-4.9; 4.9]. Low values will make the picture darker. Higher values will make the picture brighter.",
				args = "(number exposure)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getExposure =
			{
				type = "function",
				description = "Gets the exposure.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setMinAutoExposure =
			{
				type = "method",
				description = "Sets the min auto exposure. Valid range is [-4.9; 4.9]. Must be minAutoExposure <= maxAutoExposure Controls how much auto exposure darkens a bright scene. To prevent that looking at a very bright object makes the rest of the scene really dark, use higher values.",
				args = "(number minAutoExposure)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMinAutoExposure =
			{
				type = "function",
				description = "Gets the min auto exposure.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setMaxAutoExposure =
			{
				type = "method",
				description = "Sets max auto exposure. Valid range is [-4.9; 4.9]. Must be minAutoExposure <= maxAutoExposure Controls how much auto exposure brightens a dark scene. To prevent that looking at a very dark object makes the rest of the scene really bright, use lower values.",
				args = "(number maxAutoExposure)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMaxAutoExposure =
			{
				type = "function",
				description = "Gets max auto exposure.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setBloom =
			{
				type = "method",
				description = "Sets the bloom intensity. Scale is in lumens / 1024. Valid range is [0.01; 4.9].",
				args = "(number bloom)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getBloom =
			{
				type = "function",
				description = "Gets bloom intensity.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setEnvMapScale =
			{
				type = "method",
				description = "Sets enivornmental scale. Its a global scale that applies to all environment maps (for relevant Hlms implementations, like PBS). The value will be stored in upperHemisphere.a. Use 1.0 to disable.",
				args = "(number envMapScale)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getEnvMapScale =
			{
				type = "function",
				description = "Gets the environmental map scale.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	HelixPath =
	{
		type = "class",
		description = "Generates a procedural helix path."
	},
	HemisphereUVModifier =
	{
		type = "class",
		description = "Modifies UV on a procedural hemisphere."
	},
	IcoSphereGenerator =
	{
		type = "class",
		description = "Generates a procedural mesh ico sphere."
	},
	ImageBox =
	{
		type = "class",
		description = "MyGUI image box class.",
		childs = 
		{
			setImageTexture =
			{
				type = "method",
				description = "Sets the texture name for the image box. Note: The image texture must be known by the resource management of Ogre.",
				args = "(string textureName)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	InputDeviceCore =
	{
		type = "class",
		description = "Input device core functionality.",
		childs = 
		{
			getKeyboard =
			{
				type = "function",
				description = "Gets the keyboard for direct manipulation.",
				args = "()",
				returns = "(KeyBoard)",
				valuetype = "KeyBoard"
			},
			getMouse =
			{
				type = "function",
				description = "Gets the mouse for direct manipulation.",
				args = "()",
				returns = "(Mouse)",
				valuetype = "Mouse"
			}
		}
	},
	InputDeviceModule =
	{
		type = "class",
		description = "InputDeviceModule singleton for managing inputs like mouse, keyboard, joystick and mapping keys.",
		childs = 
		{
			getMappedKey =
			{
				type = "function",
				description = "Gets the OIS key that is mapped as action.",
				args = "(Action action)",
				returns = "(KeyCode)",
				valuetype = "KeyCode"
			},
			getStringFromMappedKey =
			{
				type = "function",
				description = "Gets the OIS key as string that is mapped as action.",
				args = "(Action action)",
				returns = "(string)",
				valuetype = "string"
			},
			getMappedKeyFromString =
			{
				type = "function",
				description = "Gets the OIS key from string key.",
				args = "(string key)",
				returns = "(KeyCode)",
				valuetype = "KeyCode"
			},
			getMappedButton =
			{
				type = "function",
				description = "Gets the OIS joystick button that is mapped as action.",
				args = "(Action action)",
				returns = "(JoyStickButton)",
				valuetype = "JoyStickButton"
			},
			getStringFromMappedButton =
			{
				type = "function",
				description = "Gets the OIS joystick button as string that is mapped as action.",
				args = "(Action action)",
				returns = "(string)",
				valuetype = "string"
			},
			setJoyStickDeadZone =
			{
				type = "method",
				description = "Sets the joystick dead zone.",
				args = "(number deadZone)",
				returns = "(nil)",
				valuetype = "nil"
			},
			hasActiveJoyStick =
			{
				type = "function",
				description = "Gets whether a joystick is plugged in and active.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			getLeftStickHorizontalMovingStrength =
			{
				type = "function",
				description = "Gets the strength of the left stick horizontal moving. If 0 horizontal stick is not moved. When moved right values are in range (0, 1]. When moved left values are in range (0, -1].",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getLeftStickVerticalMovingStrength =
			{
				type = "function",
				description = "Gets the strength of the left stick vertical moving. If 0 horizontal stick is not moved. When moved right values are in range (0, 1]. When moved left values are in range (0, -1].",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			isKeyDown =
			{
				type = "function",
				description = "Gets whether a specifig key is down.",
				args = "(KeyCode key)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			isButtonDown =
			{
				type = "function",
				description = "Gets whether a specifig joystick button is down.",
				args = "(JoyStickButton button)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			isActionDown =
			{
				type = "function",
				description = "Gets whether a specifig mapped action is down.",
				args = "(Action action)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			isActionDownAmount =
			{
				type = "function",
				description = "Gets whether a specifig mapped action is down, but only max for the specifig action duration. Default value is 0.2 seconds.",
				args = "(Action action, number dt, number actionDuration)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			isActionPressed =
			{
				type = "function",
				description = "Gets whether a specifig mapped action is pressed.",
				args = "(Action action, number dt, number durationBetweenTheAction)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			areButtonsDown2 =
			{
				type = "function",
				description = "Gets whether two specific joystick buttons are down at the same time.",
				args = "(JoyStickButton button1, JoyStickButton button2)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			areButtonsDown3 =
			{
				type = "function",
				description = "Gets whether three specific joystick buttons are down at the same time.",
				args = "(JoyStickButton button1, JoyStickButton button2, JoyStickButton button3)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			areButtonsDown3 =
			{
				type = "function",
				description = "Gets whether four specific joystick buttons are down at the same time.",
				args = "(JoyStickButton button1, JoyStickButton button2, JoyStickButton button3, JoyStickButton button4)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			getPressedButton =
			{
				type = "function",
				description = "Gets the currently pressed joystick button.",
				args = "()",
				returns = "(JoyStickButton)",
				valuetype = "JoyStickButton"
			},
			getPressedButtons =
			{
				type = "function",
				description = "Gets the currently simultanously pressed joystick buttons.",
				args = "()",
				returns = "(Table[number][JoyStickButton])",
				valuetype = "Table[number][JoyStickButton]"
			}
		}
	},
	InputMapping =
	{
		type = "class",
		description = "InputMapping class",
		childs = 
		{
			NOWA_K_UP =
			{
				type = "value"
			},
			NOWA_K_DOWN =
			{
				type = "value"
			},
			NOWA_K_LEFT =
			{
				type = "value"
			},
			NOWA_K_RIGHT =
			{
				type = "value"
			},
			NOWA_K_JUMP =
			{
				type = "value"
			},
			NOWA_K_RUN =
			{
				type = "value"
			},
			NOWA_K_DUCK =
			{
				type = "value"
			},
			NOWA_K_SNEAK =
			{
				type = "value"
			},
			NOWA_K_ACTION =
			{
				type = "value"
			},
			NOWA_K_ATTACK_1 =
			{
				type = "value"
			},
			NOWA_K_ATTACK_2 =
			{
				type = "value"
			},
			NOWA_K_RELOAD =
			{
				type = "value"
			},
			NOWA_K_INVENTORY =
			{
				type = "value"
			},
			NOWA_K_SAVE =
			{
				type = "value"
			},
			NOWA_K_LOAD =
			{
				type = "value"
			},
			NOWA_K_PAUSE =
			{
				type = "value"
			},
			NOWA_K_START =
			{
				type = "value"
			},
			NOWA_K_MAP =
			{
				type = "value"
			},
			NOWA_K_CAMERA_FORWARD =
			{
				type = "value"
			},
			NOWA_K_CAMERA_BACKWARD =
			{
				type = "value"
			},
			NOWA_K_CAMERA_RIGHT =
			{
				type = "value"
			},
			NOWA_K_CAMERA_UP =
			{
				type = "value"
			},
			NOWA_K_CAMERA_DOWN =
			{
				type = "value"
			},
			NOWA_K_CAMERA_LEFT =
			{
				type = "value"
			},
			NOWA_K_CONSOLE =
			{
				type = "value"
			},
			NOWA_K_WEAPON_CHANGE_FORWARD =
			{
				type = "value"
			},
			NOWA_K_WEAPON_CHANGE_BACKWARD =
			{
				type = "value"
			},
			NOWA_K_FLASH_LIGHT =
			{
				type = "value"
			},
			NOWA_K_SELECT =
			{
				type = "value"
			},
			NOWA_B_JUMP =
			{
				type = "value"
			},
			NOWA_B_RUN =
			{
				type = "value"
			},
			NOWA_B_ATTACK_1 =
			{
				type = "value"
			},
			NOWA_B_ACTION =
			{
				type = "value"
			},
			NOWA_B_RELOAD =
			{
				type = "value"
			},
			NOWA_B_INVENTORY =
			{
				type = "value"
			},
			NOWA_B_MAP =
			{
				type = "value"
			},
			NOWA_B_PAUSE =
			{
				type = "value"
			},
			NOWA_B_MENU =
			{
				type = "value"
			},
			NOWA_B_FLASH_LIGHT =
			{
				type = "value"
			},
			NOWA_A_UP =
			{
				type = "value"
			},
			NOWA_A_DOWN =
			{
				type = "value"
			},
			NOWA_A_LEFT =
			{
				type = "value"
			},
			NOWA_A_RIGHT =
			{
				type = "value"
			},
			NOWA_A_JUMP =
			{
				type = "value"
			},
			NOWA_A_RUN =
			{
				type = "value"
			},
			NOWA_A_COWER =
			{
				type = "value"
			},
			NOWA_A_DUCK =
			{
				type = "value"
			},
			NOWA_A_SNEAK =
			{
				type = "value"
			},
			NOWA_A_ACTION =
			{
				type = "value"
			},
			NOWA_A_ATTACK_1 =
			{
				type = "value"
			},
			NOWA_A_ATTACK_2 =
			{
				type = "value"
			},
			NOWA_A_RELOAD =
			{
				type = "value"
			},
			NOWA_A_INVENTORY =
			{
				type = "value"
			},
			NOWA_A_SAVE =
			{
				type = "value"
			},
			NOWA_A_LOAD =
			{
				type = "value"
			},
			NOWA_A_PAUSE =
			{
				type = "value"
			},
			NOWA_A_MENU =
			{
				type = "value"
			},
			NOWA_A_MAP =
			{
				type = "value"
			},
			NOWA_A_CAMERA_FORWARD =
			{
				type = "value"
			},
			NOWA_A_CAMERA_BACKWARD =
			{
				type = "value"
			},
			NOWA_A_CAMERA_RIGHT =
			{
				type = "value"
			},
			NOWA_A_CAMERA_UP =
			{
				type = "value"
			},
			NOWA_A_CAMERA_DOWN =
			{
				type = "value"
			},
			NOWA_A_CAMERA_LEFT =
			{
				type = "value"
			},
			NOWA_A_CONSOLE =
			{
				type = "value"
			},
			NOWA_A_WEAPON_CHANGE_FORWARD =
			{
				type = "value"
			},
			NOWA_A_WEAPON_CHANGE_BACKWARD =
			{
				type = "value"
			},
			NOWA_A_FLASH_LIGHT =
			{
				type = "value"
			},
			NOWA_A_SELECT =
			{
				type = "value"
			}
		}
	},
	Instrument =
	{
		type = "class",
		description = "Instrument class",
		childs = 
		{
			KICK_DRUM =
			{
				type = "value"
			},
			SNARE_DRUM =
			{
				type = "value"
			}
		}
	},
	IntCoord =
	{
		type = "class",
		description = "Absolute coordinates class.",
		childs = 
		{
			right =
			{
				type = "function",
				description = "The right absolute coordinate.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			bottom =
			{
				type = "function",
				description = "The bottom absolute coordinate.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			clear =
			{
				type = "method",
				description = "Clears the coordinate.",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			},
			set =
			{
				type = "method",
				description = "Sets the absolute coordinate.",
				args = "(number x1, number y1, number x2, number y2)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	IntSize =
	{
		type = "class",
		description = "Absolute size class.",
		childs = 
		{
			clear =
			{
				type = "method",
				description = "Clears the size.",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			},
			set =
			{
				type = "method",
				description = "Sets the width and height.",
				args = "(number width, number height)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	Interpolator =
	{
		type = "class",
		description = "Interpolator for utilities functions for mathematical operations and ray casting.",
		childs = 
		{
			Interpolator =
			{
				type = "value"
			},
			interpolate =
			{
				type = "function",
				description = "Linear interpolation between two values. The time t e.g. t = (time - animationStart) / duration",
				args = "(number v0, number v1, number t)",
				returns = "(number)",
				valuetype = "number"
			},
			interpolate =
			{
				type = "function",
				description = "Linear interpolation between two values. The time t e.g. t = (time - animationStart) / duration",
				args = "(Vector3 v0, Vector3 v1, number t)",
				returns = "(number)",
				valuetype = "number"
			},
			calculateBezierYT =
			{
				type = "function",
				description = "Calculates the bezier y at the given time t. The time t e.g. t = (time - animationStart) / duration",
				args = "(number t, boolean invert)",
				returns = "(number)",
				valuetype = "number"
			},
			calcBezierYTPoints =
			{
				type = "function",
				description = "Calculates the bezier y points at the given time t. The time t e.g. t = (time - animationStart) / duration",
				args = "(number t, boolean invert)",
				returns = "(number)",
				valuetype = "number"
			},
			easeInSine =
			{
				type = "function",
				description = "Slight acceleration from zero to full speed. The time t e.g. t = (time - animationStart) / duration",
				args = "(number t)",
				returns = "(number)",
				valuetype = "number"
			},
			easeOutSine =
			{
				type = "function",
				description = "Slight deceleration at the end. The time t e.g. t = (time - animationStart) / duration",
				args = "(number t)",
				returns = "(number)",
				valuetype = "number"
			},
			easeInOutSine =
			{
				type = "function",
				description = "Slight acceleration at beginning and slight deceleration at end. The time t e.g. t = (time - animationStart) / duration",
				args = "(number t)",
				returns = "(number)",
				valuetype = "number"
			},
			easeInQuad =
			{
				type = "function",
				description = "Accelerating from zero velocity. The time t e.g. t = (time - animationStart) / duration",
				args = "(number t)",
				returns = "(number)",
				valuetype = "number"
			},
			easeOutQuad =
			{
				type = "function",
				description = "Decelerating to zero velocity. The time t e.g. t = (time - animationStart) / duration",
				args = "(number t)",
				returns = "(number)",
				valuetype = "number"
			},
			easeInOutQuad =
			{
				type = "function",
				description = "Acceleration until halfway, then deceleration. The time t e.g. t = (time - animationStart) / duration",
				args = "(number t)",
				returns = "(number)",
				valuetype = "number"
			},
			easeInCubic =
			{
				type = "function",
				description = "Accelerating from zero velocity. The time t e.g. t = (time - animationStart) / duration",
				args = "(number t)",
				returns = "(number)",
				valuetype = "number"
			},
			easeOutCubic =
			{
				type = "function",
				description = "Decelerating to zero velocity. The time t e.g. t = (time - animationStart) / duration",
				args = "(number t)",
				returns = "(number)",
				valuetype = "number"
			},
			easeInOutCubic =
			{
				type = "function",
				description = "Acceleration until halfway, then deceleration. The time t e.g. t = (time - animationStart) / duration",
				args = "(number t)",
				returns = "(number)",
				valuetype = "number"
			},
			easeInQuart =
			{
				type = "function",
				description = "Accelerating from zero velocity. The time t e.g. t = (time - animationStart) / duration",
				args = "(number t)",
				returns = "(number)",
				valuetype = "number"
			},
			easeOutQuart =
			{
				type = "function",
				description = "Decelerating to zero velocity. The time t e.g. t = (time - animationStart) / duration",
				args = "(number t)",
				returns = "(number)",
				valuetype = "number"
			},
			easeInOutQuart =
			{
				type = "function",
				description = "Acceleration until halfway, then deceleration. The time t e.g. t = (time - animationStart) / duration",
				args = "(number t)",
				returns = "(number)",
				valuetype = "number"
			},
			easeInQuint =
			{
				type = "function",
				description = "Accelerating from zero velocity. The time t e.g. t = (time - animationStart) / duration",
				args = "(number t)",
				returns = "(number)",
				valuetype = "number"
			},
			easeOutQuint =
			{
				type = "function",
				description = "Decelerating to zero velocity. The time t e.g. t = (time - animationStart) / duration",
				args = "(number t)",
				returns = "(number)",
				valuetype = "number"
			},
			easeInOutQuint =
			{
				type = "function",
				description = "Acceleration until halfway, then deceleration. The time t e.g. t = (time - animationStart) / duration",
				args = "(number t)",
				returns = "(number)",
				valuetype = "number"
			},
			easeInExpo =
			{
				type = "function",
				description = "Accelerate exponentially until finish. The time t e.g. t = (time - animationStart) / duration",
				args = "(number t)",
				returns = "(number)",
				valuetype = "number"
			},
			easeOutExpo =
			{
				type = "function",
				description = "Initial exponential acceleration slowing to stop. The time t e.g. t = (time - animationStart) / duration",
				args = "(number t)",
				returns = "(number)",
				valuetype = "number"
			},
			easeInOutExpo =
			{
				type = "function",
				description = "Exponential acceleration and deceleration. The time t e.g. t = (time - animationStart) / duration",
				args = "(number t)",
				returns = "(number)",
				valuetype = "number"
			},
			easeInCirc =
			{
				type = "function",
				description = "Increasing velocity until stop. The time t e.g. t = (time - animationStart) / duration",
				args = "(number t)",
				returns = "(number)",
				valuetype = "number"
			},
			easeOutCirc =
			{
				type = "function",
				description = "Start fast, decreasing velocity until stop. The time t e.g. t = (time - animationStart) / duration",
				args = "(number t)",
				returns = "(number)",
				valuetype = "number"
			},
			easeInOutCirc =
			{
				type = "function",
				description = "Fast increase in velocity, fast decrease in velocity. The time t e.g. t = (time - animationStart) / duration",
				args = "(number t)",
				returns = "(number)",
				valuetype = "number"
			},
			easeInBack =
			{
				type = "function",
				description = "Slow movement backwards then fast snap to finish. The time t e.g. t = (time - animationStart) / duration. Info: Good value for magnitude is 1.70158",
				args = "(number t, number magnitude)",
				returns = "(number)",
				valuetype = "number"
			},
			easeOutBack =
			{
				type = "function",
				description = "Fast snap to backwards point then slow resolve to finish. The time t e.g. t = (time - animationStart) / duration. Info: Good value for magnitude is 1.70158",
				args = "(number t, number magnitude)",
				returns = "(number)",
				valuetype = "number"
			},
			easeInOutBack =
			{
				type = "function",
				description = "Slight acceleration from zero to full speed. The time t e.g. t = (time - animationStart) / duration. Info: Good value for magnitude is 1.70158",
				args = "(number t, number magnitude)",
				returns = "(number)",
				valuetype = "number"
			},
			easeInElastic =
			{
				type = "function",
				description = "Bounces slowly then quickly to finish. The time t e.g. t = (time - animationStart) / duration. Info: Good value for magnitude is 0.7",
				args = "(number t, number magnitude)",
				returns = "(number)",
				valuetype = "number"
			},
			easeOutElastic =
			{
				type = "function",
				description = "Fast acceleration, bounces to zero. The time t e.g. t = (time - animationStart) / duration. Info: Good value for magnitude is 0.7",
				args = "(number t, number magnitude)",
				returns = "(number)",
				valuetype = "number"
			},
			easeInOutElastic =
			{
				type = "function",
				description = "Slow start and end, two bounces sandwich a fast motion. The time t e.g. t = (time - animationStart) / duration. Info: Good value for magnitude is 0.65",
				args = "(number t, number magnitude)",
				returns = "(number)",
				valuetype = "number"
			},
			easeOutBounce =
			{
				type = "function",
				description = "Bounce to completion. The time t e.g. t = (time - animationStart) / duration",
				args = "(number t)",
				returns = "(number)",
				valuetype = "number"
			},
			easeInBounce =
			{
				type = "function",
				description = "Bounce increasing in velocity until completion. The time t e.g. t = (time - animationStart) / duration",
				args = "(number t)",
				returns = "(number)",
				valuetype = "number"
			},
			easeInOutBounce =
			{
				type = "function",
				description = "Bounce in and bounce out. The time t e.g. t = (time - animationStart) / duration",
				args = "(number t)",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	InventoryItemComponent =
	{
		type = "class",
		description = "Usage: Is used in conjunction with MyGuiItemBoxComponent in order build a connection between an item like energy and an inventory slot. ",
		inherits = "GameObjectComponent",
		childs = 
		{
			setResourceName =
			{
				type = "method",
				description = "Sets the used resource name.",
				args = "(string resourceName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getResourceName =
			{
				type = "function",
				description = "Gets the used resource name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			addQuantityToInventory =
			{
				type = "method",
				description = "Increases the quantity of this inventory item in the inventory game object. E.g. if inventory is used in MainGameObject, the following call is possible: 'inventoryItem:addQuantityToInventory(MAIN_GAMEOBJECT_ID, 1, true)'",
				args = "(string inventoryIdGameObject, number quantity, boolean once)",
				returns = "(nil)",
				valuetype = "nil"
			},
			removeQuantityFromInventory =
			{
				type = "method",
				description = "Decreases the quantity of this inventory item in the inventory game object. E.g. if inventory is used in MainGameObject, the following call is possible: 'inventoryItem:removeQuantityFromInventory(MAIN_GAMEOBJECT_ID, 1, true)'",
				args = "(string inventoryIdGameObject, number quantity, boolean once)",
				returns = "(nil)",
				valuetype = "nil"
			},
			addQuantityToInventory =
			{
				type = "method",
				description = "Increases the quantity of this inventory item in the inventory game object. E.g. if inventory is used in MainGameObject, the following call is possible: 'inventoryItem:addQuantityToInventory(MAIN_GAMEOBJECT_ID, 'Player1InventoryComponent', 1, true)'",
				args = "(string inventoryIdGameObject, string componentName, number quantity, boolean once)",
				returns = "(nil)",
				valuetype = "nil"
			},
			removeQuantityFromInventory =
			{
				type = "method",
				description = "Decreases the quantity of this inventory item in the inventory game object. E.g. if inventory is used in MainGameObject, the following call is possible: 'inventoryItem:removeQuantityFromInventory(MAIN_GAMEOBJECT_ID, 'Player1InventoryComponent', 1, true)'",
				args = "(string inventoryIdGameObject, string componentName, number quantity, boolean once)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	Joint6DofComponent =
	{
		type = "class",
		description = "Derived class from JointComponent. Joint with 6 degree of freedom.",
		inherits = "JointComponent",
		childs = 
		{
			getId =
			{
				type = "function",
				description = "Gets the id of this joint.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setAnchorPosition0 =
			{
				type = "method",
				description = "Sets the first anchor position where to place the joint. The anchor position is set relative to the global mesh origin.",
				args = "(Vector3 anchorPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAnchorPosition0 =
			{
				type = "function",
				description = "Gets the first joint anchor position.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setAnchorPosition1 =
			{
				type = "method",
				description = "Sets the second anchor position where to place the joint. The anchor position is set relative to the global mesh origin.",
				args = "(Vector3 anchorPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAnchorPosition1 =
			{
				type = "function",
				description = "Gets the second joint anchor position.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setMinMaxStopDistance =
			{
				type = "method",
				description = "Sets the min and max stop distance.",
				args = "(number minStopDistance, number maxStopDistance)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMinStopDistance =
			{
				type = "function",
				description = "Gets min stop distance.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getMaxStopDistance =
			{
				type = "function",
				description = "Gets max stop distance.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setMinMaxYawAngleLimit =
			{
				type = "method",
				description = "Sets the min max yaw angle limit in degree.",
				args = "(Degree minYawAngleLimit, Degree maxYawAngleLimit)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMinYawAngleLimit =
			{
				type = "function",
				description = "Gets the min yaw angle limit.",
				args = "()",
				returns = "(Degree)",
				valuetype = "Degree"
			},
			setMinMaxPitchAngleLimit =
			{
				type = "method",
				description = "Sets the min max pitch angle limit in degree.",
				args = "(Degree minPitchAngleLimit, Degree maxPitchAngleLimit)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMinPitchAngleLimit =
			{
				type = "function",
				description = "Gets the min pitch angle limit.",
				args = "()",
				returns = "(Degree)",
				valuetype = "Degree"
			},
			getMaxPitchAngleLimit =
			{
				type = "function",
				description = "Gets the max pitch angle limit.",
				args = "()",
				returns = "(Degree)",
				valuetype = "Degree"
			},
			setMinMaxRollAngleLimit =
			{
				type = "method",
				description = "Sets the min max roll angle limit in degree.",
				args = "(Degree minRollAngleLimit, Degree maxRollAngleLimit)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMinRollAngleLimit =
			{
				type = "function",
				description = "Gets the min roll angle limit.",
				args = "()",
				returns = "(Degree)",
				valuetype = "Degree"
			},
			getMaxRollAngleLimit =
			{
				type = "function",
				description = "Gets the max roll angle limit.",
				args = "()",
				returns = "(Degree)",
				valuetype = "Degree"
			}
		}
	},
	JointActiveSliderComponent =
	{
		type = "class",
		description = "Derived class from JointComponent. A child body attached via a slider joint can only slide up and down (move along) the axis it is attached to. It will move automatically from the min stop distance to the max stop distance by the specifed axis, or when direction change is activated from min- to max stop distance and vice versa.",
		inherits = "JointComponent",
		childs = 
		{
			getId =
			{
				type = "function",
				description = "Gets the id of this joint.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setAnchorPosition =
			{
				type = "method",
				description = "Sets anchor position where to place the joint. The anchor position is set relative to the global mesh origin.",
				args = "(Vector3 anchorPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAnchorPosition =
			{
				type = "function",
				description = "Gets joint anchor position.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setPin =
			{
				type = "method",
				description = "Sets pin axis for the joint, in order to slide in that direction.",
				args = "(Vector3 pin)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPin =
			{
				type = "function",
				description = "Gets joint pin axis.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setMinStopDistance =
			{
				type = "method",
				description = "Sets the min stop limit.Note: If 'setDirectionChange' is enabled, this is the min stop limit at which the motion direction will be changed.",
				args = "(number minStopDistance)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMinStopDistance =
			{
				type = "function",
				description = "Gets the min stop distance limit.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setMaxStopDistance =
			{
				type = "method",
				description = "Sets the max stop distance limit. Note: If 'setDirectionChange' is enabled, this is the max stop limit at which the motion direction will be changed.",
				args = "(number maxStopDistance)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMaxStopDistance =
			{
				type = "function",
				description = "Gets the max stop distance.",
				args = "()",
				returns = "(Degree)",
				valuetype = "Degree"
			},
			setMoveSpeed =
			{
				type = "method",
				description = "Sets the moving speed.",
				args = "(number moveSpeed)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMoveSpeed =
			{
				type = "function",
				description = "Gets moving speed.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setDirectionChange =
			{
				type = "method",
				description = "Sets whether the direction should be changed, when the motion reaches the min or max stop distance.",
				args = "(boolean directionChange)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDirectionChange =
			{
				type = "function",
				description = "Gets whether the direction is changed, when the motion reaches the min or max stop distance.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setRepeat =
			{
				type = "method",
				description = "Sets whether to repeat the direction changed motion. Note: If set to false the motion will only go from min stop distance and back to max stop distance once.",
				args = "(boolean repeat)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRepeat =
			{
				type = "function",
				description = "Gets whether the direction changed motion is repeated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			}
		}
	},
	JointAttractorComponent =
	{
		type = "class",
		description = "Derived class from JointComponent. With this joint its possible to influence other physics active game object (repeller) by magnetic strength. The to be attracted game object must belong to the specified repeller category.",
		inherits = "JointComponent",
		childs = 
		{
			getId =
			{
				type = "function",
				description = "Gets the id of this joint.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setMagneticStrength =
			{
				type = "method",
				description = "Sets the magnetic strength.",
				args = "(boolean magneticStrength)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMagneticStrength =
			{
				type = "function",
				description = "Gets the magnetic strength.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setAttractionDistance =
			{
				type = "method",
				description = "Sets attraction distance at which the object is influenced by the magnetic force.",
				args = "(number attractionDistance)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAttractionDistance =
			{
				type = "function",
				description = "Gets the attraction distance at which the object is influenced by the magnetic force.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setRepellerCategory =
			{
				type = "method",
				description = "Sets the repeller category. Note: All other game objects with a physics active component, that are belonging to this category, will be attracted.",
				args = "(string repellerCategory)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRepellerCategory =
			{
				type = "function",
				description = "Gets the repeller category.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			}
		}
	},
	JointBallAndSocketComponent =
	{
		type = "class",
		description = "Requirements: A kind of physics component must exist.",
		inherits = "JointComponent",
		childs = 
		{
			setAnchorPosition =
			{
				type = "method",
				description = "Sets anchor position where to place the joint. The anchor position is set relative to the global mesh origin.",
				args = "(Vector3 anchorPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAnchorPosition =
			{
				type = "function",
				description = "Gets joint anchor position.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setTwistLimitsEnabled =
			{
				type = "method",
				description = "Sets whether the twist of this joint should be limited by its rotation.",
				args = "(boolean enabled)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTwistLimitsEnabled =
			{
				type = "function",
				description = "Gets whether the twist of this joint is limited by its rotation.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setConeLimitsEnabled =
			{
				type = "method",
				description = "Sets whether the cone of this joint should be limited by its 'hanging'.",
				args = "(boolean enabled)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getConeLimitsEnabled =
			{
				type = "function",
				description = "Gets whether the cone of this joint is limited by its 'hanging'.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setMinMaxConeAngleLimit =
			{
				type = "method",
				description = "Sets the min and max angle and cone limit in degree. Note: This will only be applied if 'setTwistLimitsEnabled' and 'setConeLimitsEnabled' are set to 'true'.",
				args = "(Degree minAngleLimit, Degree minAngleLimit, Degree maxConeLimit)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMinAngleLimit =
			{
				type = "function",
				description = "Gets the min angle limit.",
				args = "()",
				returns = "(Degree)",
				valuetype = "Degree"
			},
			getMaxAngleLimit =
			{
				type = "function",
				description = "Gets the max angle limit.",
				args = "()",
				returns = "(Degree)",
				valuetype = "Degree"
			},
			getConeAngleLimit =
			{
				type = "function",
				description = "Gets the max cone limit.",
				args = "()",
				returns = "(Degree)",
				valuetype = "Degree"
			},
			setTwistFriction =
			{
				type = "method",
				description = "Sets the twist friction for this joint in order to prevent high peek values.",
				args = "(number friction)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTwistFriction =
			{
				type = "function",
				description = "Gets the twist friction of this joint.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setConeFriction =
			{
				type = "method",
				description = "Sets the cone friction for this joint in order to prevent high peek values.",
				args = "(number friction)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getConeFriction =
			{
				type = "function",
				description = "Gets the cone friction of this joint.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setAnchorPosition1 =
			{
				type = "method",
				description = "Sets anchor position 1 where to place the joint. The anchor position 1 is set relative to the global mesh origin.",
				args = "(Vector3 anchorPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAnchorPosition1 =
			{
				type = "function",
				description = "Gets joint anchor position 1.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setAnchorPosition2 =
			{
				type = "method",
				description = "Sets anchor position 2 where to attach to the joint. The anchor position 2 is set relative to the global mesh origin.",
				args = "(Vector3 anchorPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAnchorPosition2 =
			{
				type = "function",
				description = "Gets joint anchor position 2.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			}
		}
	},
	JointComponent =
	{
		type = "class",
		description = "Requirements: A kind of physics component must exist.",
		inherits = "GameObjectComponent",
		childs = 
		{
			getId =
			{
				type = "function",
				description = "Gets the id of this joint.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			getName =
			{
				type = "function",
				description = "Gets the name of this joint.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			getType =
			{
				type = "function",
				description = "Gets the type of this joint.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			getPredecessorJointComponent =
			{
				type = "function",
				description = "Gets the predecessor joint component.",
				args = "()",
				returns = "(JointComponent)",
				valuetype = "JointComponent"
			},
			getTargetJointComponent =
			{
				type = "function",
				description = "Gets the target joint component.",
				args = "()",
				returns = "(JointComponent)",
				valuetype = "JointComponent"
			},
			getJointPosition =
			{
				type = "function",
				description = "Gets the position of this joint.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			getUpdatedJointPosition =
			{
				type = "function",
				description = "Gets the updated joint position, which also may take a custom joint offset into account and does change, as the physics body does change its position.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setJointRecursiveCollisionEnabled =
			{
				type = "method",
				description = "Sets whether this joint should collide with its predecessors or not.",
				args = "(boolean enabled)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getJointRecursiveCollisionEnabled =
			{
				type = "function",
				description = "Gets whether this joint should collide with its predecessors or not.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			releaseJoint =
			{
				type = "method",
				description = "Releases this joint, so that it is no more connected with its predecessors and the joint is deleted. Note: This function is dangerous and may cause a crash, because an inconsistent behavior may happen.",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	JointCorkScrewComponent =
	{
		type = "class",
		description = "Derived class from JointComponent. This joint type is an enhanched version of a slider joint which allows the attached child body to not only slide up and down the axis, but also to rotate around this axis.",
		inherits = "JointComponent",
		childs = 
		{
			getId =
			{
				type = "function",
				description = "Gets the id of this joint.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setAnchorPosition =
			{
				type = "method",
				description = "Sets anchor position where to place the joint. The anchor position is set relative to the global mesh origin.",
				args = "(Vector3 anchorPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAnchorPosition =
			{
				type = "function",
				description = "Gets joint anchor position.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setLinearLimitsEnabled =
			{
				type = "method",
				description = "Sets whether the linear motion of this joint should be limited.",
				args = "(boolean enabled)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getLinearLimitsEnabled =
			{
				type = "function",
				description = "Gets whether the linear motion of this joint is limited.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setAngleLimitsEnabled =
			{
				type = "method",
				description = "Sets whether the angle rotation of this joint should be limited.",
				args = "(boolean enabled)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAngleLimitsEnabled =
			{
				type = "function",
				description = "Gets whether the angle rotation of this joint is limited.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setMinMaxStopDistance =
			{
				type = "method",
				description = "Sets the min and max distance limit. Note: This will only be applied if 'setLinearLimitsEnabled' is set to 'true'.",
				args = "(number minStopDistance, number maxStopDistance)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMinStopDistance =
			{
				type = "function",
				description = "Gets min stop distance.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getMaxStopDistance =
			{
				type = "function",
				description = "Gets max stop distance.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setMinMaxAngleLimit =
			{
				type = "method",
				description = "Sets the min and max angle limit. Note: This will only be applied if 'setAngleLimitsEnabled' is set to 'true'.",
				args = "(Degree minAngleLimit, number maxAngleLimit)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMinAngleLimit =
			{
				type = "function",
				description = "Gets min angle limit.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getMaxAngleLimit =
			{
				type = "function",
				description = "Gets max angle limit.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	JointDryRollingFrictionComponent =
	{
		type = "class",
		description = "Derived class from JointComponent. This joint is usefully to simulate the rolling friction of a rolling ball over a flat surface.  Normally this is not important for non spherical objects, but for games like poll, pinball, bolling, golf or any other where the movement of balls is the main objective the rolling friction is a real big problem. See: http://newtondynamics.com/forum/viewtopic.php?f=15&t=1187&p=8158&hilit=dry+rolling+friction",
		inherits = "JointComponent",
		childs = 
		{
			getId =
			{
				type = "function",
				description = "Gets the id of this joint.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setRadius =
			{
				type = "method",
				description = "Sets radius of the ball.",
				args = "(number radius)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRadius =
			{
				type = "function",
				description = "Gets the radius of the ball.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setRollingFrictionCoefficient =
			{
				type = "method",
				description = "Sets the rolling friction coefficient.",
				args = "(number rollingFrictionCoefficient)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRollingFrictionCoefficient =
			{
				type = "function",
				description = "Gets the rolling friction coefficient.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	JointFlexyPipeHandleComponent =
	{
		type = "class",
		description = "Requirements: A kind of physics component must exist.",
		inherits = "JointComponent",
		childs = 
		{
			setPin =
			{
				type = "method",
				description = "Sets the pin (direction) of this joint.",
				args = "(Vector3 pin)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPin =
			{
				type = "function",
				description = "Gets the pin (direction) of this component.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setVelocity =
			{
				type = "method",
				description = "Controls the motion of this joint by the given velocity vector and time step.",
				args = "(Vector3 velocity, number dt)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setOmega =
			{
				type = "method",
				description = "Controls the rotation of this joint by the given velocity vector and time step.",
				args = "(Vector3 omega, number dt)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	JointGearComponent =
	{
		type = "class",
		description = "Derived class from JointComponent. This joint is for use in conjunction with Hinge of other spherical joints is useful for establishing synchronization between the phase angle other the relative angular velocity of two spinning disks according to the law of gears velErro = -(W0 * r0 + W1 *  r1) where w0 and W1 are the angular velocity r0 and r1 are the radius of the spinning disk",
		inherits = "JointComponent",
		childs = 
		{
			getId =
			{
				type = "function",
				description = "Gets the id of this joint.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setGearRatio =
			{
				type = "method",
				description = "Sets the ratio between the first hinge rotation and the second hinge rotation.",
				args = "(number ratio)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getGearRatio =
			{
				type = "function",
				description = "Gets the ratio between the first hinge rotation and the second hinge rotation.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	JointHingeActuatorComponent =
	{
		type = "class",
		description = "Requirements: A kind of physics component must exist.",
		inherits = "JointComponent",
		childs = 
		{
			getId =
			{
				type = "function",
				description = "Gets the id of this joint.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setAnchorPosition =
			{
				type = "method",
				description = "Sets anchor position where to place the joint. The anchor position is set relative to the global mesh origin.",
				args = "(Vector3 anchorPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAnchorPosition =
			{
				type = "function",
				description = "Gets joint anchor position.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setPin =
			{
				type = "method",
				description = "Sets pin axis for the joint, to rotate around one dimension prependicular to the axis.",
				args = "(Vector3 pin)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPin =
			{
				type = "function",
				description = "Gets joint pin axis.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setAngleRate =
			{
				type = "method",
				description = "Sets the angle rate (rotation speed).",
				args = "(number angularRate)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAngleRate =
			{
				type = "function",
				description = "Gets the angle rate (rotation speed) for this joint.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setTargetAngle =
			{
				type = "method",
				description = "Sets target angle, the game object will be rotated to. Note: This does only work, if 'setDirectionChange' or 'repeat' is off.",
				args = "(Degree targetAngle)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTargetAngle =
			{
				type = "function",
				description = "Gets target angle.",
				args = "()",
				returns = "(Degree)",
				valuetype = "Degree"
			},
			setMinAngleLimit =
			{
				type = "method",
				description = "Sets the min angle limit in degree. Note: If 'setDirectionChange' is enabled, this is the angle at which the rotation direction will be changed.",
				args = "(Degree minAngleLimit)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMinAngleLimit =
			{
				type = "function",
				description = "Gets the min angle limit.",
				args = "()",
				returns = "(Degree)",
				valuetype = "Degree"
			},
			setMaxAngleLimit =
			{
				type = "method",
				description = "Sets the max angle limit in degree. Note: If 'setDirectionChange' is enabled, this is the angle at which the rotation direction will be changed.",
				args = "(Degree maxAngleLimit)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMaxAngleLimit =
			{
				type = "function",
				description = "Gets the max angle limit.",
				args = "()",
				returns = "(Degree)",
				valuetype = "Degree"
			},
			setMaxTorque =
			{
				type = "method",
				description = "Sets max torque during rotation. Note: This will affect the rotation rate. Note: A high value will cause that the rotation will start slow and will be faster and at the end slow again.",
				args = "(number maxTorque)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMaxTorque =
			{
				type = "function",
				description = "Gets max torque that is used during rotation.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setDirectionChange =
			{
				type = "method",
				description = "Sets whether the direction should be changed, when the rotation reaches the min or max angle limit.",
				args = "(boolean directionChange)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDirectionChange =
			{
				type = "function",
				description = "Gets whether the direction is changed, when the rotation reaches the min or max angle limit.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setRepeat =
			{
				type = "method",
				description = "Sets whether to repeat the direction changed rotation. Note: If set to false the rotation will only go from min angle limit and back to max angular limit once.",
				args = "(boolean repeat)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRepeat =
			{
				type = "function",
				description = "Gets whether the direction changed rotation is repeated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setSpring =
			{
				type = "method",
				description = "Activates the spring with currently set spring values.",
				args = "(boolean asSpringDamper, boolean massIndependent)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setSpring =
			{
				type = "method",
				description = "Sets the spring values for this joint. Note: When 'asSpringDamper' is activated the joint will use spring values for rotation.",
				args = "(boolean asSpringDamper, boolean massIndependent, number springDamperRelaxation, number springK, number springD)",
				returns = "(nil)",
				valuetype = "nil"
			},
			reactOnTargetAngleReached =
			{
				type = "method",
				description = "Sets whether to react at the moment when the game object has reached the target angle. Note: The angle parameter can be used to check if the min angle limit has been reached or the max angle limit.",
				args = "(func closure, Degree angle)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	JointHingeComponent =
	{
		type = "class",
		description = "Requirements: A kind of physics component must exist.",
		inherits = "JointComponent",
		childs = 
		{
			getId =
			{
				type = "function",
				description = "Gets the id of this joint.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setAnchorPosition =
			{
				type = "method",
				description = "Sets anchor position where to place the joint. The anchor position is set relative to the global mesh origin.",
				args = "(Vector3 anchorPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAnchorPosition =
			{
				type = "function",
				description = "Gets joint anchor position.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setPin =
			{
				type = "method",
				description = "Sets pin axis for the joint, to rotate around one dimension prependicular to the axis.",
				args = "(Vector3 pin)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPin =
			{
				type = "function",
				description = "Gets joint pin axis.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setLimitsEnabled =
			{
				type = "method",
				description = "Sets whether this joint should be limited by its rotation.",
				args = "(boolean enabled)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getLimitsEnabled =
			{
				type = "function",
				description = "Gets whether this joint is limited by its rotation.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setMinMaxAngleLimit =
			{
				type = "method",
				description = "Sets the min and max angle limit in degree. Note: This will only be applied if 'setLimitsEnabled' is set to 'true'.",
				args = "(Degree minAngleLimit, Degree minAngleLimit)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMinAngleLimit =
			{
				type = "function",
				description = "Gets the min angle limit.",
				args = "()",
				returns = "(Degree)",
				valuetype = "Degree"
			},
			getMaxAngleLimit =
			{
				type = "function",
				description = "Gets the max angle limit.",
				args = "()",
				returns = "(Degree)",
				valuetype = "Degree"
			},
			setTorgue =
			{
				type = "method",
				description = "Sets the torque rotation for this joint. Note: Without a friction the torque will not work. So get a balance between torque and friction (1, 20) e.g. The more friction, the more powerful the motor will become.",
				args = "(number torque)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setFriction =
			{
				type = "method",
				description = "Sets the friction (max. torque) for torque. Note: Without a friction the torque will not work. So get a balance between torque and friction (1, 20) e.g. The more friction, the more powerful the motor will become.",
				args = "(number maxTorque)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTorgue =
			{
				type = "function",
				description = "Gets the torque rotation for this joint.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setSpring =
			{
				type = "method",
				description = "Activates the spring with currently set spring values.",
				args = "(boolean asSpringDamper, boolean massIndependent)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setSpring =
			{
				type = "method",
				description = "Sets the spring values for this joint. Note: When 'asSpringDamper' is activated the joint will use spring values for rotation.",
				args = "(boolean asSpringDamper, boolean massIndependent, number springDamperRelaxation, number springK, number springD)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setBreakForce =
			{
				type = "method",
				description = "Sets the force, that is required to break this joint, so that the go will no longer use the hinge joint.",
				args = "(number breakForce)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getBreakForce =
			{
				type = "function",
				description = "Gets break force that is required to break this joint, so that the go will no longer use the hinge joint.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setBreakTorque =
			{
				type = "method",
				description = "Sets the torque, that is required to break this joint, so that the go will no longer use the hinge joint.",
				args = "(number breakForce)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getBreakTorque =
			{
				type = "function",
				description = "Gets break torque that is required to break this joint, so that the go will no longer use the hinge joint.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	JointKinematicComponent =
	{
		type = "class",
		description = "Requirements: A kind of physics component must exist.",
		inherits = "JointComponent",
		childs = 
		{
			getId =
			{
				type = "function",
				description = "Gets the id of this joint.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setActivated =
			{
				type = "method",
				description = "Sets whether to activate the kinematic motion.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether the kinematic motion is activated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setAnchorPosition =
			{
				type = "method",
				description = "Sets anchor position where to place the joint. The anchor position is set relative to the global mesh origin.",
				args = "(Vector3 anchorPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAnchorPosition =
			{
				type = "function",
				description = "Gets joint anchor position.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setPickingMode =
			{
				type = "method",
				description = "Sets the picking mode. Possible values are: Linear, Full 6 Degree Of Freedom, Linear And Twist, Linear And Cone, Linear Plus Angluar Friction",
				args = "(string picking mode)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPickingMode =
			{
				type = "function",
				description = "Gets the currently set picking mode. Possible values are: Linear, Full 6 Degree Of Freedom, Linear And Twist, Linear And Cone, Linear Plus Angluar Friction",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setMaxLinearAngleFriction =
			{
				type = "method",
				description = "Sets the max linear and angle friction.",
				args = "(number maxLinearFriction, number maxAngleFriction)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setMaxSpeed =
			{
				type = "method",
				description = "Sets the maximum speed in meters per seconds.",
				args = "(number maxSpeed)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMaxSpeed =
			{
				type = "function",
				description = "Gets the maximum speed in meters per seconds.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setMaxOmega =
			{
				type = "method",
				description = "Sets the maximum rotation speed in degrees per seconds.",
				args = "(number maxOmega)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMaxOmega =
			{
				type = "function",
				description = "Gets the maximum rotation speed in degrees per seconds.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setAngularViscousFrictionCoefficient =
			{
				type = "method",
				description = "Adds a viscous friction coefficient to the angular rotation (good for setting target position in water e.g.).",
				args = "(number coefficient)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAngularViscousFrictionCoefficient =
			{
				type = "function",
				description = "Gets the viscous friction coefficient of the angular rotation.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getMaxLinearAngleFriction =
			{
				type = "function",
				description = "Gets max linear and angle friction.",
				args = "()",
				returns = "(Table[number][number])",
				valuetype = "Table[number][number]"
			},
			setTargetPosition =
			{
				type = "method",
				description = "Sets the target position.",
				args = "(Vector3 targetPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTargetPosition =
			{
				type = "function",
				description = "Gets the target position.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setTargetRotation =
			{
				type = "method",
				description = "Sets the target rotation.",
				args = "(Quaternion targetRotation)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTargetRotation =
			{
				type = "function",
				description = "Gets the target rotation.",
				args = "()",
				returns = "(Quaternion)",
				valuetype = "Quaternion"
			},
			backToOrigin =
			{
				type = "method",
				description = "Moves the game object back to its origin.",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			},
			setShortTimeActivation =
			{
				type = "method",
				description = "If set to true, this component will be deactivated shortly after it has been activated. Which means, the kinematic joint will take its target transform and remain there.",
				args = "(boolean shortTimeActivation)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getShortTimeActivation =
			{
				type = "function",
				description = "Gets whether this component is deactivated shortly after it has been activated. Which means, the kinematic joint will take its target transform and remain there.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			reactOnTargetPositionReached =
			{
				type = "method",
				description = "Sets whether to react at the moment when the game object has reached the target position.",
				args = "(func closure, otherGameObject)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	JointMathSliderComponent =
	{
		type = "class",
		description = "Derived class from JointComponent. A child body attached via a slider joint can only slide by the given mathematical function.",
		inherits = "JointComponent",
		childs = 
		{
			getId =
			{
				type = "function",
				description = "Gets the id of this joint.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setMinMaxStopDistance =
			{
				type = "method",
				description = "Sets the min and max stop limit.Note: If 'setDirectionChange' is enabled, this is the min stop limit at which the motion direction will be changed.",
				args = "(number minStopDistance, number maxStopDistance)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMinStopDistance =
			{
				type = "function",
				description = "Gets the min stop distance limit.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getMaxStopDistance =
			{
				type = "function",
				description = "Gets the max stop distance.",
				args = "()",
				returns = "(Degree)",
				valuetype = "Degree"
			},
			setMoveSpeed =
			{
				type = "method",
				description = "Sets the moving speed.",
				args = "(number moveSpeed)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMoveSpeed =
			{
				type = "function",
				description = "Gets moving speed.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setDirectionChange =
			{
				type = "method",
				description = "Sets whether the direction should be changed, when the motion reaches the min or max stop distance.",
				args = "(boolean directionChange)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDirectionChange =
			{
				type = "function",
				description = "Gets whether the direction is changed, when the motion reaches the min or max stop distance.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setRepeat =
			{
				type = "method",
				description = "Sets whether to repeat the direction changed motion. Note: If set to false the motion will only go from min stop distance and back to max stop distance once.",
				args = "(boolean repeat)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRepeat =
			{
				type = "function",
				description = "Gets whether the direction changed motion is repeated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setFunctionX =
			{
				type = "method",
				description = "Sets the math function for x.",
				args = "(string mx)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getFunctionX =
			{
				type = "function",
				description = "Gets the math function for x.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setFunctionY =
			{
				type = "method",
				description = "Sets the math function for y.",
				args = "(string my)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getFunctionY =
			{
				type = "function",
				description = "Gets the math function for y.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setFunctionZ =
			{
				type = "method",
				description = "Sets the math function for z.",
				args = "(string mz)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getFunctionZ =
			{
				type = "function",
				description = "Gets the math function for z.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			}
		}
	},
	JointMotorComponent =
	{
		type = "class",
		description = "Derived class from JointComponent. A motor joint, which needs another joint component as reference (predecessor), which should be rotated. Its also possible to use a second joint component (target) and pin for a different rotation.",
		inherits = "JointComponent",
		childs = 
		{
			getId =
			{
				type = "function",
				description = "Gets the id of this joint.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setPin0 =
			{
				type = "method",
				description = "Sets pin0 axis for the first reference body, to rotate around one dimension prependicular to the axis.",
				args = "(Vector3 pin0)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPin0 =
			{
				type = "function",
				description = "Gets first reference body pin0 axis.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setPin1 =
			{
				type = "method",
				description = "Sets pin1 axis for the second reference body, to rotate around one dimension prependicular to the axis. Note: This is optional.",
				args = "(Vector3 pin1)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPin1 =
			{
				type = "function",
				description = "Gets second reference body pin1 axis.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setSpeed0 =
			{
				type = "method",
				description = "Sets the rotating speed 0.",
				args = "(number speed0)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSpeed0 =
			{
				type = "function",
				description = "Gets the rotating speed 0.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setSpeed1 =
			{
				type = "method",
				description = "Sets the rotating speed 1.",
				args = "(number speed1)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSpeed1 =
			{
				type = "function",
				description = "Gets the rotating speed 1.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setTorgue0 =
			{
				type = "method",
				description = "Sets the torque rotation 0 for this joint.",
				args = "(number torque0)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTorgue0 =
			{
				type = "function",
				description = "Gets the torque rotation 0 for this joint.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setTorgue1 =
			{
				type = "method",
				description = "Sets the torque rotation 1 for this joint. Note: This is optional.",
				args = "(number torque1)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTorgue1 =
			{
				type = "function",
				description = "Gets the torque rotation 1 for this joint.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	JointPassiveSliderComponent =
	{
		type = "class",
		description = "Derived class from JointComponent. A child body attached via a slider joint can only slide up and down (move along) the axis it is attached to.",
		inherits = "JointComponent",
		childs = 
		{
			getId =
			{
				type = "function",
				description = "Gets the id of this joint.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setAnchorPosition =
			{
				type = "method",
				description = "Sets anchor position where to place the joint. The anchor position is set relative to the global mesh origin.",
				args = "(Vector3 anchorPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAnchorPosition =
			{
				type = "function",
				description = "Gets joint anchor position.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setPin =
			{
				type = "method",
				description = "Sets pin axis for the joint, in order to slide in that direction.",
				args = "(Vector3 pin)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPin =
			{
				type = "function",
				description = "Gets joint pin axis.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setLimitsEnabled =
			{
				type = "method",
				description = "Sets whether the linear motion of this joint should be limited.",
				args = "(boolean enabled)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getLimitsEnabled =
			{
				type = "function",
				description = "Gets whether the linear motion of this joint is limited.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setMinMaxStopDistance =
			{
				type = "method",
				description = "Sets the min and max stop distance. Note: This will only be applied if 'setLimitsEnabled' is set to 'true'.",
				args = "(number minStopDistance, number maxStopDistance)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMinStopDistance =
			{
				type = "function",
				description = "Gets min stop distance.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getMaxStopDistance =
			{
				type = "function",
				description = "Gets max stop distance.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setSpring =
			{
				type = "method",
				description = "Activates the spring with currently set spring values",
				args = "(boolean asSpringDamper)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setSpring2 =
			{
				type = "method",
				description = "Sets the spring values for this joint. Note: When 'asSpringDamper' is activated the slider will use spring values for motion.",
				args = "(boolean asSpringDamper, number springDamperRelaxation, number springK, number springD)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	JointPinComponent =
	{
		type = "class",
		description = "Derived class from JointComponent. This class can be used to limit rotation to one axis of freedom.",
		inherits = "JointComponent",
		childs = 
		{
			getId =
			{
				type = "function",
				description = "Gets the id of this joint.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setPin =
			{
				type = "method",
				description = "Sets pin axis for the joint, to limit rotation to one axis of freedom.",
				args = "(Vector3 pin)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPin =
			{
				type = "function",
				description = "Gets joint pin axis.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			}
		}
	},
	JointPlaneComponent =
	{
		type = "class",
		description = "Derived class from JointComponent. This class can be used to limit position to two axis.",
		inherits = "JointComponent",
		childs = 
		{
			getId =
			{
				type = "function",
				description = "Gets the id of this joint.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setAnchorPosition =
			{
				type = "method",
				description = "Sets anchor position where to place the joint. The anchor position is set relative to the global mesh origin.",
				args = "(Vector3 anchorPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAnchorPosition =
			{
				type = "function",
				description = "Gets joint anchor position.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setNormal =
			{
				type = "method",
				description = "Sets normal axis for the joint, to limit position to two axis. Example: Setting normal to z, motion only at x and y is possible.",
				args = "(Vector3 normal)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getNormal =
			{
				type = "function",
				description = "Gets joint normal axis.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			}
		}
	},
	JointPulleyComponent =
	{
		type = "class",
		description = "Derived class from JointComponent. This joint is for use in conjunction with Slider of other linear joints it is useful for establishing synchronization between the relative position or relative linear velocity of two sliding bodies according to the law of pulley velErro = -(v0 + n * v1) where v0 and v1 are the linear velocity n is the number of turn on the pulley, in the case of this joint N could be a value with friction, as this joint is a generalization of the pulley idea.",
		inherits = "JointComponent",
		childs = 
		{
			getId =
			{
				type = "function",
				description = "Gets the id of this joint.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setPulleyRatio =
			{
				type = "method",
				description = "Sets the ratio between the first slider motion and the second slider motion.",
				args = "(number ratio)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPulleyRatio =
			{
				type = "function",
				description = "Gets the ratio between the first slider motion and the second slider motion.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	JointSliderActuatorComponent =
	{
		type = "class",
		description = "Derived class from JointComponent. A child body attached via a slider joint can only slide up and down (move along) the axis it is attached to. It will move automatically to the given target position, or when direction change is activated from min- to max stop distance and vice versa.",
		inherits = "JointComponent",
		childs = 
		{
			getId =
			{
				type = "function",
				description = "Gets the id of this joint.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setAnchorPosition =
			{
				type = "method",
				description = "Sets anchor position where to place the joint. The anchor position is set relative to the global mesh origin.",
				args = "(Vector3 anchorPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAnchorPosition =
			{
				type = "function",
				description = "Gets joint anchor position.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setPin =
			{
				type = "method",
				description = "Sets pin axis for the joint, in order to slide in that direction.",
				args = "(Vector3 pin)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPin =
			{
				type = "function",
				description = "Gets joint pin axis.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setTargetPosition =
			{
				type = "method",
				description = "Sets target position, the game object will move to. Note: This does only work, if 'setDirectionChange' is off.",
				args = "(number targetPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTargetPosition =
			{
				type = "function",
				description = "Gets target position.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setMinStopDistance =
			{
				type = "method",
				description = "Sets the min stop limit.Note: If 'setDirectionChange' is enabled, this is the min stop limit at which the motion direction will be changed.",
				args = "(number minStopDistance)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMinStopDistance =
			{
				type = "function",
				description = "Gets the min stop distance limit.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setMaxStopDistance =
			{
				type = "method",
				description = "Sets the max stop distance limit. Note: If 'setDirectionChange' is enabled, this is the max stop limit at which the motion direction will be changed.",
				args = "(number maxStopDistance)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMaxStopDistance =
			{
				type = "function",
				description = "Gets the max stop distance.",
				args = "()",
				returns = "(Degree)",
				valuetype = "Degree"
			},
			setMinForce =
			{
				type = "method",
				description = "Sets min force during motion. Note: This will affect the motion rate. Note: A high value will cause that the motion will start slow and will be faster and at the end slow again.",
				args = "(number minForce)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMinForce =
			{
				type = "function",
				description = "Gets min force that is used during motion.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setMaxForce =
			{
				type = "method",
				description = "Sets max force during motion. Note: This will affect the motion rate. Note: A high value will cause that the motion will start slow and will be faster and at the end slow again.",
				args = "(number maxForce)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMaxForce =
			{
				type = "function",
				description = "Gets max force that is used during motion.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setDirectionChange =
			{
				type = "method",
				description = "Sets whether the direction should be changed, when the motion reaches the min or max stop distance.",
				args = "(boolean directionChange)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDirectionChange =
			{
				type = "function",
				description = "Gets whether the direction is changed, when the motion reaches the min or max stop distance.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setRepeat =
			{
				type = "method",
				description = "Sets whether to repeat the direction changed motion. Note: If set to false the motion will only go from min stop distance and back to max stop distance once.",
				args = "(boolean repeat)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRepeat =
			{
				type = "function",
				description = "Gets whether the direction changed motion is repeated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			reactOnTargetPositionReached =
			{
				type = "method",
				description = "Sets whether to react at the moment when the game object has reached the target position. The position can be used to check if it equals the min stop distance or the max stop distance.",
				args = "(func closure, Vector3 position)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	JointSlidingContactComponent =
	{
		type = "class",
		description = "Derived class from JointComponent. A child body attached via a slider joint can only slide up and down (move along) the axis it is attached to and rotate around given pin-axis.",
		inherits = "JointComponent",
		childs = 
		{
			getId =
			{
				type = "function",
				description = "Gets the id of this joint.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setAnchorPosition =
			{
				type = "method",
				description = "Sets anchor position where to place the joint. The anchor position is set relative to the global mesh origin.",
				args = "(Vector3 anchorPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAnchorPosition =
			{
				type = "function",
				description = "Gets joint anchor position.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setPin =
			{
				type = "method",
				description = "Sets pin axis for the joint, in order to slide in that direction.",
				args = "(Vector3 pin)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPin =
			{
				type = "function",
				description = "Gets joint pin axis.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setLinearLimitsEnabled =
			{
				type = "method",
				description = "Sets whether the linear motion of this joint should be limited.",
				args = "(boolean enabled)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getLinearLimitsEnabled =
			{
				type = "function",
				description = "Gets whether the linear motion of this joint is limited.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setAngleLimitsEnabled =
			{
				type = "method",
				description = "Sets whether the angle rotation of this joint should be limited.",
				args = "(boolean enabled)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAngleLimitsEnabled =
			{
				type = "function",
				description = "Gets whether the angle rotation of this joint is limited.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setMinMaxStopDistance =
			{
				type = "method",
				description = "Sets the min and max stop distance. Note: This will only be applied if 'setLinearLimitsEnabled' is set to 'true'.",
				args = "(number minStopDistance, number maxStopDistance)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMinStopDistance =
			{
				type = "function",
				description = "Gets min stop distance.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getMaxStopDistance =
			{
				type = "function",
				description = "Gets max stop distance.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setMinMaxAngleLimit =
			{
				type = "method",
				description = "Sets the min and max angle limit. Note: This will only be applied if 'setAngleLimitsEnabled' is set to 'true'.",
				args = "(Degree minAngleLimit, number maxAngleLimit)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMinAngleLimit =
			{
				type = "function",
				description = "Gets min angle limit.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getMaxAngleLimit =
			{
				type = "function",
				description = "Gets max angle limit.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setSpring =
			{
				type = "method",
				description = "Activates the spring with currently set spring values",
				args = "(boolean asSpringDamper)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setSpring2 =
			{
				type = "method",
				description = "Sets the spring values for this joint. Note: When 'asSpringDamper' is activated the slider will use spring values for motion.",
				args = "(boolean asSpringDamper, number springDamperRelaxation, number springK, number springD)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	JointSpringComponent =
	{
		type = "class",
		description = "Derived class from JointComponent. With this joint its possible to connect two objects with a spring.",
		inherits = "JointComponent",
		childs = 
		{
			getId =
			{
				type = "function",
				description = "Gets the id of this joint.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setShowLine =
			{
				type = "method",
				description = "Sets whether to show the spring line.",
				args = "(boolean showLine)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getShowLine =
			{
				type = "function",
				description = "Gets whether the spring line is shown.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setAnchorOffsetPosition =
			{
				type = "method",
				description = "Sets anchor offset position, where the spring should start.",
				args = "(Vector3 anchorOffsetPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAnchorOffsetPosition =
			{
				type = "function",
				description = "Gets anchor offset position, where the spring starts.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setSpringOffsetPosition =
			{
				type = "method",
				description = "Sets spring offset position, where the spring should end.",
				args = "(Vector3 springOffsetPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSpringOffsetPosition =
			{
				type = "function",
				description = "Gets spring offset position, where the spring ends.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			releaseSpring =
			{
				type = "method",
				description = "Releases the spring, so that it is no more visible and no spring calculation takes places.",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	JointTargetTransformComponent =
	{
		type = "class",
		description = "Requirements: A kind of physics component must exist and a predecessor joint component. This joint will synchronize the transform of the predecessor joint.",
		inherits = "JointComponent",
		childs = 
		{
			getId =
			{
				type = "function",
				description = "Gets the id of this joint.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setAnchorPosition =
			{
				type = "method",
				description = "Sets anchor position where to place the joint. The anchor position is set relative to the global mesh origin.",
				args = "(Vector3 anchorPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAnchorPosition =
			{
				type = "function",
				description = "Gets joint anchor position.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setOffsetPosition =
			{
				type = "method",
				description = "Adds an offset position to the target predecessor position.",
				args = "(Vector3 offsetPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setOffsetOrientation =
			{
				type = "method",
				description = "Adds an offset orientation to the target predecessor orientation.",
				args = "(Quaternion offsetOrientation)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	JointUniversalActuatorComponent =
	{
		type = "class",
		description = "Derived class from JointComponent. An object attached to a universal actuator joint can rotate around two dimensions perpendicular to the axes it is attached to. It will rotate automatically to the given target angle, or when direction change is activated from min- to max angle limit and vice versa. It can be seen like a double hinge joint.",
		inherits = "JointComponent",
		childs = 
		{
			getId =
			{
				type = "function",
				description = "Gets the id of this joint.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setAnchorPosition =
			{
				type = "method",
				description = "Sets anchor position where to place the joint. The anchor position is set relative to the global mesh origin.",
				args = "(Vector3 anchorPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAnchorPosition =
			{
				type = "function",
				description = "Gets joint anchor position.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setAngleRate0 =
			{
				type = "method",
				description = "Sets the first angle rate (rotation speed).",
				args = "(number angularRate)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAngleRate0 =
			{
				type = "function",
				description = "Gets the first angle rate (rotation speed) for this joint.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setTargetAngle0 =
			{
				type = "method",
				description = "Sets first target angle, the game object will be rotated to. Note: This does only work, if 'setDirectionChange0' is off.",
				args = "(Degree targetAngle)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTargetAngle0 =
			{
				type = "function",
				description = "Gets first target angle.",
				args = "()",
				returns = "(Degree)",
				valuetype = "Degree"
			},
			setMinAngleLimit0 =
			{
				type = "method",
				description = "Sets the first min angle limit in degree. Note: If 'setDirectionChange0' is enabled, this is the angle at which the rotation direction will be changed.",
				args = "(Degree minAngleLimit)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMinAngleLimit0 =
			{
				type = "function",
				description = "Gets the first min angle limit.",
				args = "()",
				returns = "(Degree)",
				valuetype = "Degree"
			},
			setMaxAngleLimit0 =
			{
				type = "method",
				description = "Sets the first max angle limit in degree. Note: If 'setDirectionChange0' is enabled, this is the angle at which the rotation direction will be changed.",
				args = "(Degree maxAngleLimit)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMaxAngleLimit0 =
			{
				type = "function",
				description = "Gets the first max angle limit.",
				args = "()",
				returns = "(Degree)",
				valuetype = "Degree"
			},
			setMaxTorque0 =
			{
				type = "method",
				description = "Sets the first max torque during rotation. Note: This will affect the rotation rate. Note: A high value will cause that the rotation will start slow and will be faster and at the end slow again.",
				args = "(number maxTorque)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMaxTorque0 =
			{
				type = "function",
				description = "Gets the first max torque that is used during rotation.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setDirectionChange0 =
			{
				type = "method",
				description = "Sets whether the direction should be changed, when the first rotation reaches the min or max angle limit.",
				args = "(boolean directionChange)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDirectionChange0 =
			{
				type = "function",
				description = "Gets whether the direction is changed, when the first rotation reaches the min or max angle limit.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setRepeat0 =
			{
				type = "method",
				description = "Sets whether to repeat the first direction changed rotation. Note: If set to false the rotation will only go from min angle limit and back to max angular limit once.",
				args = "(boolean repeat)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRepeat0 =
			{
				type = "function",
				description = "Gets whether the second direction changed rotation is repeated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setAngleRate1 =
			{
				type = "method",
				description = "Sets the second angle rate (rotation speed).",
				args = "(number angularRate)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAngleRate1 =
			{
				type = "function",
				description = "Gets the second angle rate (rotation speed) for this joint.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setTargetAngle1 =
			{
				type = "method",
				description = "Sets second target angle, the game object will be rotated to. Note: This does only work, if 'setDirectionChange0' is off.",
				args = "(Degree targetAngle)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTargetAngle1 =
			{
				type = "function",
				description = "Gets second target angle.",
				args = "()",
				returns = "(Degree)",
				valuetype = "Degree"
			},
			setMinAngleLimit1 =
			{
				type = "method",
				description = "Sets the second min angle limit in degree. Note: If 'setDirectionChange1' is enabled, this is the angle at which the rotation direction will be changed.",
				args = "(Degree minAngleLimit)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMinAngleLimit1 =
			{
				type = "function",
				description = "Gets the second min angle limit.",
				args = "()",
				returns = "(Degree)",
				valuetype = "Degree"
			},
			setMaxAngleLimit1 =
			{
				type = "method",
				description = "Sets the second max angle limit in degree. Note: If 'setDirectionChange1' is enabled, this is the angle at which the rotation direction will be changed.",
				args = "(Degree maxAngleLimit)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMaxAngleLimit1 =
			{
				type = "function",
				description = "Gets the second max angle limit.",
				args = "()",
				returns = "(Degree)",
				valuetype = "Degree"
			},
			setMaxTorque1 =
			{
				type = "method",
				description = "Sets the second max torque during rotation. Note: This will affect the rotation rate. Note: A high value will cause that the rotation will start slow and will be faster and at the end slow again.",
				args = "(number maxTorque)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMaxTorque1 =
			{
				type = "function",
				description = "Gets the second max torque that is used during rotation.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setDirectionChange1 =
			{
				type = "method",
				description = "Sets whether the direction should be changed, when the second rotation reaches the min or max angle limit.",
				args = "(boolean directionChange)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDirectionChange1 =
			{
				type = "function",
				description = "Gets whether the direction is changed, when the second rotation reaches the min or max angle limit.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setRepeat1 =
			{
				type = "method",
				description = "Sets whether to repeat the second direction changed rotation. Note: If set to false the rotation will only go from min angle limit and back to max angular limit once.",
				args = "(boolean repeat)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRepeat1 =
			{
				type = "function",
				description = "Gets whether the second direction changed rotation is repeated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			}
		}
	},
	JointUniversalComponent =
	{
		type = "class",
		description = "Derived class from JointComponent. An object attached to a universal joint can rotate around two dimensions perpendicular to the axes it is attached to. It is to be seen like a double hinge joint.",
		inherits = "JointComponent",
		childs = 
		{
			getId =
			{
				type = "function",
				description = "Gets the id of this joint.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setAnchorPosition =
			{
				type = "method",
				description = "Sets anchor position where to place the joint. The anchor position is set relative to the global mesh origin.",
				args = "(Vector3 anchorPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAnchorPosition =
			{
				type = "function",
				description = "Gets joint anchor position.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setPin =
			{
				type = "method",
				description = "Sets the pin for the axis of the joint, to rotate around one dimension prependicular to the axis.",
				args = "(Vector3 pin)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPin =
			{
				type = "function",
				description = "Gets the joint pin for axis.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setLimits0Enabled =
			{
				type = "method",
				description = "Sets whether this joint should be limited by its first rotation.",
				args = "(boolean enabled)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getLimits0Enabled =
			{
				type = "function",
				description = "Gets whether this joint is limited by its first rotation.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setMinMaxAngleLimit0 =
			{
				type = "method",
				description = "Sets the first min and max angle limit in degree. Note: This will only be applied if 'setLimitsEnabled' is set to 'true'.",
				args = "(Degree minAngleLimit, Degree minAngleLimit)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMinAngleLimit0 =
			{
				type = "function",
				description = "Gets the first min angle limit.",
				args = "()",
				returns = "(Degree)",
				valuetype = "Degree"
			},
			getMaxAngleLimit0 =
			{
				type = "function",
				description = "Gets the first max angle limit.",
				args = "()",
				returns = "(Degree)",
				valuetype = "Degree"
			},
			setMotor0Enabled =
			{
				type = "method",
				description = "Sets whether to enable the first motor for automatical rotation.",
				args = "(boolean enabled)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMotor0Enabled =
			{
				type = "function",
				description = "Gets whether the first motor for automatical rotation is enabled.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setMotorSpeed0 =
			{
				type = "method",
				description = "Sets the first motor rotation speed. Note: Without a friction the torque will not work. So get a balance between torque and friction (1, 20) e.g. The more friction, the more powerful the motor will become.",
				args = "(number speed)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMotorSpeed0 =
			{
				type = "function",
				description = "Gets the first motor rotation speed.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setFriction0 =
			{
				type = "method",
				description = "Sets the friction (max. torque) for torque. Note: Without a friction the torque will not work. So get a balance between torque and friction (1, 20) e.g. The more friction, the more powerful the motor will become.",
				args = "(number friction)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getFriction0 =
			{
				type = "function",
				description = "Gets the first motor rotation friction.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setLimits1Enabled =
			{
				type = "method",
				description = "Sets whether this joint should be limited by its second rotation.",
				args = "(boolean enabled)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getLimits1Enabled =
			{
				type = "function",
				description = "Gets whether this joint is limited by its second rotation.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setMinMaxAngleLimit1 =
			{
				type = "method",
				description = "Sets the second min and max angle limit in degree. Note: This will only be applied if 'setLimitsEnabled' is set to 'true'.",
				args = "(Degree minAngleLimit, Degree minAngleLimit)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMinAngleLimit1 =
			{
				type = "function",
				description = "Gets the second min angle limit.",
				args = "()",
				returns = "(Degree)",
				valuetype = "Degree"
			},
			getMaxAngleLimit1 =
			{
				type = "function",
				description = "Gets the second max angle limit.",
				args = "()",
				returns = "(Degree)",
				valuetype = "Degree"
			},
			setMotor1Enabled =
			{
				type = "method",
				description = "Sets whether to enable the second motor for automatical rotation.",
				args = "(boolean enabled)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMotor1Enabled =
			{
				type = "function",
				description = "Gets whether the second motor for automatical rotation is enabled.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setMotorSpeed1 =
			{
				type = "method",
				description = "Sets the second motor rotation speed. Note: Without a friction the torque will not work. So get a balance between torque and friction (1, 20) e.g. The more friction, the more powerful the motor will become.",
				args = "(number speed)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMotorSpeed1 =
			{
				type = "function",
				description = "Gets the second motor rotation speed.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setSpring0 =
			{
				type = "method",
				description = "Activates the first spring with currently set spring values",
				args = "(boolean asSpringDamper)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setSpring0 =
			{
				type = "method",
				description = "Sets the first spring values for this joint. Note: When 'asSpringDamper' is activated the joint will use spring values for rotation.",
				args = "(boolean asSpringDamper, number springDamperRelaxation, number springK, number springD)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setSpring1 =
			{
				type = "method",
				description = "Activates the second spring with currently set spring values",
				args = "(boolean asSpringDamper)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setSpring1 =
			{
				type = "method",
				description = "Sets the second spring values for this joint. Note: When 'asSpringDamper' is activated the joint will use spring values for rotation.",
				args = "(boolean asSpringDamper, number springDamperRelaxation, number springK, number springD)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setFriction1 =
			{
				type = "method",
				description = "Sets the friction (max. torque) for torque. Note: Without a friction the torque will not work. So get a balance between torque and friction (1, 20) e.g. The more friction, the more powerful the motor will become.",
				args = "(number friction)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getFriction1 =
			{
				type = "function",
				description = "Gets the second motor rotation friction.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	JointWheelComponent =
	{
		type = "class",
		description = "Derived class from JointComponent. An object attached to a wheel joint can only rotate around one dimension perpendicular to the axis it is attached to and also steer.",
		inherits = "JointComponent",
		childs = 
		{
			getId =
			{
				type = "function",
				description = "Gets the id of this joint.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setAnchorPosition =
			{
				type = "method",
				description = "Sets anchor position where to place the joint. The anchor position is set relative to the global mesh origin.",
				args = "(Vector3 anchorPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAnchorPosition =
			{
				type = "function",
				description = "Gets joint anchor position.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setPin =
			{
				type = "method",
				description = "Sets pin axis for the joint, to rotate around one dimension prependicular to the axis.",
				args = "(Vector3 pin)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPin =
			{
				type = "function",
				description = "Gets joint pin axis.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			getSteeringAngle =
			{
				type = "function",
				description = "Gets current steering angle in degree.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setTargetSteeringAngle =
			{
				type = "method",
				description = "Sets the target steering angle in degree.",
				args = "(number targetSteeringAngle)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTargetSteeringAngle =
			{
				type = "function",
				description = "Gets the target steering angle in degree.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setSteeringRate =
			{
				type = "method",
				description = "Sets the steering rate.",
				args = "(number steeringRate)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSteeringRate =
			{
				type = "function",
				description = "Gets the steering rate.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getStartMoveDirection =
			{
				type = "function",
				description = "Gets the starting move direction of the game object on the path. It can be used e.g. in order to add a force in that direction, in order to start movement the game object along the path.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			getCurrentMoveDirection =
			{
				type = "function",
				description = "Gets the current move direction of the game object on the path. It can be used e.g. in order to add a force in that direction, in order to continue movement the game object along the path.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			getPathLength =
			{
				type = "function",
				description = "Gets the path length in meters.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getPathProgress =
			{
				type = "function",
				description = "Gets the path progress in percent.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	JointWormGearComponent =
	{
		type = "class",
		description = "Derived class from JointComponent. This joint is for use in conjunction with Hinge of other slider joints is useful for establishing synchronization between the phase angle and a slider motion.",
		inherits = "JointComponent",
		childs = 
		{
			getId =
			{
				type = "function",
				description = "Gets the id of this joint.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setGearRatio =
			{
				type = "method",
				description = "Sets the ratio between the first hinge rotation and the second hinge rotation.",
				args = "(number ratio)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getGearRatio =
			{
				type = "function",
				description = "Gets the ratio between the first hinge rotation and the second hinge rotation.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setSlideRatio =
			{
				type = "method",
				description = "Sets the ratio between the hinge rotation and the slider motion.",
				args = "(number ratio)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSlideRatio =
			{
				type = "function",
				description = "Gets the ratio between the hinge rotation and the slider motion.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	JoystickRemapComponent =
	{
		type = "class",
		description = "Usage: This component can be used as building block in order to have joystick remap functionality. It can be placed as root via the position or using a parent id to be placed as a child in a parent MyGUI window. Note: It can only be added under a MainGameObject.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets whether this component should be activated or not.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this component is activated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			}
		}
	},
	KeyEvent =
	{
		type = "class",
		description = "OIS Keyboard key event class.",
		childs = 
		{
			KC_1 =
			{
				type = "value"
			},
			KC_2 =
			{
				type = "value"
			},
			KC_3 =
			{
				type = "value"
			},
			KC_4 =
			{
				type = "value"
			},
			KC_5 =
			{
				type = "value"
			},
			KC_6 =
			{
				type = "value"
			},
			KC_7 =
			{
				type = "value"
			},
			KC_8 =
			{
				type = "value"
			},
			KC_9 =
			{
				type = "value"
			},
			KC_0 =
			{
				type = "value"
			},
			KC_MINUS =
			{
				type = "value"
			},
			KC_EQUALS =
			{
				type = "value"
			},
			KC_BACK =
			{
				type = "value"
			},
			KC_TAB =
			{
				type = "value"
			},
			KC_Q =
			{
				type = "value"
			},
			KC_W =
			{
				type = "value"
			},
			KC_E =
			{
				type = "value"
			},
			KC_R =
			{
				type = "value"
			},
			KC_T =
			{
				type = "value"
			},
			KC_Y =
			{
				type = "value"
			},
			KC_U =
			{
				type = "value"
			},
			KC_I =
			{
				type = "value"
			},
			KC_O =
			{
				type = "value"
			},
			KC_P =
			{
				type = "value"
			},
			KC_LBRACKET =
			{
				type = "value"
			},
			KC_RBRACKET =
			{
				type = "value"
			},
			KC_RETURN =
			{
				type = "value"
			},
			KC_LCONTROL =
			{
				type = "value"
			},
			KC_A =
			{
				type = "value"
			},
			KC_S =
			{
				type = "value"
			},
			KC_D =
			{
				type = "value"
			},
			KC_F =
			{
				type = "value"
			},
			KC_G =
			{
				type = "value"
			},
			KC_H =
			{
				type = "value"
			},
			KC_J =
			{
				type = "value"
			},
			KC_K =
			{
				type = "value"
			},
			KC_L =
			{
				type = "value"
			},
			KC_SEMICOLON =
			{
				type = "value"
			},
			KC_APOSTROPHE =
			{
				type = "value"
			},
			KC_GRAVE =
			{
				type = "value"
			},
			KC_LSHIFT =
			{
				type = "value"
			},
			KC_BACKSLASH =
			{
				type = "value"
			},
			KC_Z =
			{
				type = "value"
			},
			KC_X =
			{
				type = "value"
			},
			KC_C =
			{
				type = "value"
			},
			KC_V =
			{
				type = "value"
			},
			KC_B =
			{
				type = "value"
			},
			KC_N =
			{
				type = "value"
			},
			KC_M =
			{
				type = "value"
			},
			KC_COMMA =
			{
				type = "value"
			},
			KC_PERIOD =
			{
				type = "value"
			},
			KC_SLASH =
			{
				type = "value"
			},
			KC_RSHIFT =
			{
				type = "value"
			},
			KC_MULTIPLY =
			{
				type = "value"
			},
			KC_LMENU =
			{
				type = "value"
			},
			KC_SPACE =
			{
				type = "value"
			},
			KC_CAPITAL =
			{
				type = "value"
			},
			KC_F1 =
			{
				type = "value"
			},
			KC_F2 =
			{
				type = "value"
			},
			KC_F3 =
			{
				type = "value"
			},
			KC_F4 =
			{
				type = "value"
			},
			KC_F5 =
			{
				type = "value"
			},
			KC_F6 =
			{
				type = "value"
			},
			KC_F7 =
			{
				type = "value"
			},
			KC_F8 =
			{
				type = "value"
			},
			KC_F9 =
			{
				type = "value"
			},
			KC_F10 =
			{
				type = "value"
			},
			KC_NUMLOCK =
			{
				type = "value"
			},
			KC_SCROLL =
			{
				type = "value"
			},
			KC_NUMPAD7 =
			{
				type = "value"
			},
			KC_NUMPAD8 =
			{
				type = "value"
			},
			KC_NUMPAD9 =
			{
				type = "value"
			},
			KC_SUBTRACT =
			{
				type = "value"
			},
			KC_NUMPAD4 =
			{
				type = "value"
			},
			KC_NUMPAD5 =
			{
				type = "value"
			},
			KC_NUMPAD6 =
			{
				type = "value"
			},
			KC_ADD =
			{
				type = "value"
			},
			KC_NUMPAD1 =
			{
				type = "value"
			},
			KC_NUMPAD2 =
			{
				type = "value"
			},
			KC_NUMPAD3 =
			{
				type = "value"
			},
			KC_NUMPAD0 =
			{
				type = "value"
			},
			KC_DECIMAL =
			{
				type = "value"
			},
			KC_OEM_102 =
			{
				type = "value"
			},
			KC_F11 =
			{
				type = "value"
			},
			KC_F12 =
			{
				type = "value"
			},
			KC_F13 =
			{
				type = "value"
			},
			KC_F14 =
			{
				type = "value"
			},
			KC_F15 =
			{
				type = "value"
			},
			KC_KANA =
			{
				type = "value"
			},
			KC_ABNT_C1 =
			{
				type = "value"
			},
			KC_CONVERT =
			{
				type = "value"
			},
			KC_NOCONVERT =
			{
				type = "value"
			},
			KC_YEN =
			{
				type = "value"
			},
			KC_ABNT_C2 =
			{
				type = "value"
			},
			KC_NUMPADEQUALS =
			{
				type = "value"
			},
			KC_PREVTRACK =
			{
				type = "value"
			},
			KC_AT =
			{
				type = "value"
			},
			KC_COLON =
			{
				type = "value"
			},
			KC_UNDERLINE =
			{
				type = "value"
			},
			KC_KANJI =
			{
				type = "value"
			},
			KC_STOP =
			{
				type = "value"
			},
			KC_AX =
			{
				type = "value"
			},
			KC_UNLABELED =
			{
				type = "value"
			},
			KC_NEXTTRACK =
			{
				type = "value"
			},
			KC_NUMPADENTER =
			{
				type = "value"
			},
			KC_RCONTROL =
			{
				type = "value"
			},
			KC_MUTE =
			{
				type = "value"
			},
			KC_CALCULATOR =
			{
				type = "value"
			},
			KC_PLAYPAUSE =
			{
				type = "value"
			},
			KC_MEDIASTOP =
			{
				type = "value"
			},
			KC_VOLUMEDOWN =
			{
				type = "value"
			},
			KC_VOLUMEUP =
			{
				type = "value"
			},
			KC_WEBHOME =
			{
				type = "value"
			},
			KC_NUMPADCOMMA =
			{
				type = "value"
			},
			KC_DIVIDE =
			{
				type = "value"
			},
			KC_SYSRQ =
			{
				type = "value"
			},
			KC_RMENU =
			{
				type = "value"
			},
			KC_PAUSE =
			{
				type = "value"
			},
			KC_HOME =
			{
				type = "value"
			},
			KC_UP =
			{
				type = "value"
			},
			KC_PGUP =
			{
				type = "value"
			},
			KC_LEFT =
			{
				type = "value"
			},
			KC_RIGHT =
			{
				type = "value"
			},
			KC_END =
			{
				type = "value"
			},
			KC_DOWN =
			{
				type = "value"
			},
			KC_PGDOWN =
			{
				type = "value"
			},
			KC_INSERT =
			{
				type = "value"
			},
			KC_DELETE =
			{
				type = "value"
			},
			KC_LWIN =
			{
				type = "value"
			},
			KC_RWIN =
			{
				type = "value"
			},
			KC_APPS =
			{
				type = "value"
			},
			KC_POWER =
			{
				type = "value"
			},
			KC_SLEEP =
			{
				type = "value"
			},
			KC_WAKE =
			{
				type = "value"
			},
			KC_WEBSEARCH =
			{
				type = "value"
			},
			KC_WEBFAVORITES =
			{
				type = "value"
			},
			KC_WEBREFRESH =
			{
				type = "value"
			},
			KC_WEBSTOP =
			{
				type = "value"
			},
			KC_WEBFORWARD =
			{
				type = "value"
			},
			KC_WEBBACK =
			{
				type = "value"
			},
			KC_MYCOMPUTER =
			{
				type = "value"
			},
			KC_MAIL =
			{
				type = "value"
			},
			KC_MEDIASELECT =
			{
				type = "value"
			}
		}
	},
	Keyboard =
	{
		type = "class",
		description = "OIS Keyboard class.",
		childs = 
		{
			isKeyDown =
			{
				type = "function",
				description = "Gets whether a specifig key is down.",
				args = "(KeyCode key)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			isModifierDown =
			{
				type = "function",
				description = "Gets whether a specifig modifier is down.",
				args = "(Modifier mod)",
				returns = "(boolean)",
				valuetype = "boolean"
			}
		}
	},
	KeyboardRemapComponent =
	{
		type = "class",
		description = "Usage: This component can be used as building block in order to have keyboard remap functionality. It can be placed as root via the position or using a parent id to be placed as a child in a parent MyGUI window. Note: It can only be added under a MainGameObject.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets whether this component should be activated or not.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this component is activated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			}
		}
	},
	KeyholeEffectComponent =
	{
		type = "class",
		description = "Usage: Creates a keyhole effect which is applied to the screen. Also another custom shape can be used.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets whether this component should be activated or not.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this component is activated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			}
		}
	},
	KochanekBartelsSpline2 =
	{
		type = "class",
		description = "Generates a procedural kochanek bartels spline2."
	},
	Lathe =
	{
		type = "class",
		description = "Performs a lathe operation on a procedural mesh."
	},
	LightAreaComponent =
	{
		type = "class",
		description = "An Area Light is defined by a rectangle in space. Light is emitted in all directions uniformly across their surface area, but only from one side of the rectangle.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets whether this light type is visible.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this light type is visible.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setDiffuseColor =
			{
				type = "method",
				description = "Sets the diffuse color (r, g, b) of the light.",
				args = "(Vector3 diffuseColor)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDiffuseColor =
			{
				type = "function",
				description = "Gets the diffuse color (r, g, b) of the light.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setSpecularColor =
			{
				type = "method",
				description = "Sets the specular color (r, g, b) of the light.",
				args = "(Vector3 specularColor)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSpecularColor =
			{
				type = "function",
				description = "Gets the specular color (r, g, b) of the light.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setPowerScale =
			{
				type = "method",
				description = "Sets the power of the light. E.g. a value of 2 will make the light two times stronger. Default value is: PI",
				args = "(number powerScale)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPowerScale =
			{
				type = "function",
				description = "Gets strength of the light.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setCastShadows =
			{
				type = "method",
				description = "Sets whether the game objects, that are affected by this light will cast shadows.",
				args = "(boolean castShadows)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCastShadows =
			{
				type = "function",
				description = "Gets whether the game objects, that are affected by this light are casting shadows.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setAttenuationRadius =
			{
				type = "method",
				description = "Sets the attenuation radius. Default value is 10.",
				args = "(number attenuationRadius)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAttenuationRadius =
			{
				type = "function",
				description = "Gets the attenuation radius.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setRectSize =
			{
				type = "method",
				description = "Sets the size (w, h) of the area light rectangle. Default value is: Vector2(1, 1).",
				args = "(Vector2 size)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRectSize =
			{
				type = "function",
				description = "Gets the size (w, h) of the area light rectangle.",
				args = "()",
				returns = "(Vector2)",
				valuetype = "Vector2"
			},
			setTextureName =
			{
				type = "method",
				description = "Sets the texture name for the area light for the light shining pattern. Note: If no texture is set, no texture mask is used and the light will emit from complete plane. Default value is: 'lightAreaPattern1.png'.",
				args = "(string textureName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTextureName =
			{
				type = "function",
				description = "Gets the used texture name for the light shining pattern.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setDoubleSided =
			{
				type = "method",
				description = "Specifies whether the light lits in both directions (positive & negative sides of the plane) or if only towards one.",
				args = "(boolean doubleSided)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDoubleSided =
			{
				type = "function",
				description = "Gets whether the light lits in both directions (positive & negative sides of the plane) or if only towards one.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			}
		}
	},
	LightDirectionalComponent =
	{
		type = "class",
		description = "This Light simulates a huge source that is very far away - like daylight. Light hits the entire scene at the same angle everywhere.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets whether this light type is visible.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this light type is visible.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setDiffuseColor =
			{
				type = "method",
				description = "Sets the diffuse color (r, g, b) of the light.",
				args = "(Vector3 diffuseColor)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDiffuseColor =
			{
				type = "function",
				description = "Gets the diffuse color (r, g, b) of the light.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setSpecularColor =
			{
				type = "method",
				description = "Sets the specular color (r, g, b) of the light.",
				args = "(Vector3 specularColor)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSpecularColor =
			{
				type = "function",
				description = "Gets the specular color (r, g, b) of the light.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setPowerScale =
			{
				type = "method",
				description = "Sets the power of the light. E.g. a value of 2 will make the light two times stronger. Default value is: PI",
				args = "(number powerScale)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPowerScale =
			{
				type = "function",
				description = "Gets strength of the light.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setDirection =
			{
				type = "method",
				description = "Sets the direction of the light.",
				args = "(Vector3 direction)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDirection =
			{
				type = "function",
				description = "Gets the direction of the light.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setCastShadows =
			{
				type = "method",
				description = "Sets whether the game objects, that are affected by this light will cast shadows.",
				args = "(boolean castShadows)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCastShadows =
			{
				type = "function",
				description = "Gets whether the game objects, that are affected by this light are casting shadows.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setAttenuationRadius =
			{
				type = "method",
				description = "Sets the attenuation radius. Default value is 10.",
				args = "(number attenuationRadius)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAttenuationRadius =
			{
				type = "function",
				description = "Gets the attenuation radius.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setAttenuationLumThreshold =
			{
				type = "method",
				description = "Sets the attenuation lumen threshold. Default value is 0.00192.",
				args = "(number attenuationLumThreshold)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAttenuationLumThreshold =
			{
				type = "function",
				description = "Gets the attenuation lumen threshold.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setShowDummyEntity =
			{
				type = "method",
				description = "Sets whether to show a dummy entity, because normally a light is not visible and its hard to find out, where it actually is, so this function might help for analysis purposes.",
				args = "(boolean showDummyEntity)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getShowDummyEntity =
			{
				type = "function",
				description = "Gets whether dummy entity is shown.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			}
		}
	},
	LightPointComponent =
	{
		type = "class",
		description = "This Light speads out equally in all directions from a point.",
		inherits = "GameObjectComponent inherits GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets whether this light type is visible.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this light type is visible.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setDiffuseColor =
			{
				type = "method",
				description = "Sets the diffuse color (r, g, b) of the light.",
				args = "(Vector3 diffuseColor)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDiffuseColor =
			{
				type = "function",
				description = "Gets the diffuse color (r, g, b) of the light.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setSpecularColor =
			{
				type = "method",
				description = "Sets the specular color (r, g, b) of the light.",
				args = "(Vector3 specularColor)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSpecularColor =
			{
				type = "function",
				description = "Gets the specular color (r, g, b) of the light.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setPowerScale =
			{
				type = "method",
				description = "Sets the power of the light. E.g. a value of 2 will make the light two times stronger. Default value is: PI",
				args = "(number powerScale)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPowerScale =
			{
				type = "function",
				description = "Gets strength of the light.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setCastShadows =
			{
				type = "method",
				description = "Sets whether the game objects, that are affected by this light will cast shadows.",
				args = "(boolean castShadows)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCastShadows =
			{
				type = "function",
				description = "Gets whether the game objects, that are affected by this light are casting shadows.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setAttenuationMode =
			{
				type = "method",
				description = "Sets attenuation mode. Possible values are: 'Range', 'Radius'",
				args = "(string mode)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAttenuationMode =
			{
				type = "function",
				description = "Gets the attenuation mode.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setAttenuationRange =
			{
				type = "method",
				description = "Sets attenuation range. Default value is: 23. Note: This will only be applied, if @setAttenuationMode is set to 'Range'.",
				args = "(number range)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAttenuationRange =
			{
				type = "function",
				description = "Gets the attenuation range.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setAttenuationConstant =
			{
				type = "method",
				description = "Sets attenuation constant. Default value is 5. Note: This will only be applied, if @setAttenuationMode is set to 'Range'.",
				args = "(number constant)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAttenuationConstant =
			{
				type = "function",
				description = "Gets the attenuation constant.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setAttenuationLinear =
			{
				type = "method",
				description = "Sets linear attenuation. Default value is 0. Note: This will only be applied, if @setAttenuationMode is set to 'Range'.",
				args = "(number linear)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAttenuationLinear =
			{
				type = "function",
				description = "Gets the linear attenuation constant.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setAttenuationQuadratic =
			{
				type = "method",
				description = "Sets quadric attenuation. Default value is 0.5. Note: This will only be applied, if @setAttenuationMode is set to 'Range'.",
				args = "(number quadric)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAttenuationQuadratic =
			{
				type = "function",
				description = "Gets the quadric attenuation.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setAttenuationRadius =
			{
				type = "method",
				description = "Sets the attenuation radius. Default value is 10. Note: This will only be applied, if @setAttenuationMode is set to 'Radius'.",
				args = "(number attenuationRadius)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAttenuationRadius =
			{
				type = "function",
				description = "Gets the attenuation radius.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setAttenuationLumThreshold =
			{
				type = "method",
				description = "Sets the attenuation lumen threshold. Default value is 0.00192.",
				args = "(number attenuationLumThreshold)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAttenuationLumThreshold =
			{
				type = "function",
				description = "Gets the attenuation lumen threshold.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setShowDummyEntity =
			{
				type = "method",
				description = "Sets whether to show a dummy entity, because normally a light is not visible and its hard to find out, where it actually is, so this function might help for analysis purposes.",
				args = "(boolean showDummyEntity)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getShowDummyEntity =
			{
				type = "function",
				description = "Gets whether dummy entity is shown.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			}
		}
	},
	LightSpotComponent =
	{
		type = "class",
		description = "This Light works like a flashlight. It produces a solid cylinder of light that is brighter at the center and fades off.",
		inherits = "GameObjectComponent"
	},
	LineComponent =
	{
		type = "class",
		description = "Usage: Creates a visible line between two game objects.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setTargetId =
			{
				type = "method",
				description = "Sets target id, to get the target game object to attach the lines end point at. The target id is optional.",
				args = "(string targetId)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTargetId =
			{
				type = "function",
				description = "Gets target id of the target game object.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setColor =
			{
				type = "method",
				description = "Sets color (r, g, b) of the line.",
				args = "(Vector3 color)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getColor =
			{
				type = "function",
				description = "Gets color (r, g, b) of the line.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setSourceOffsetPosition =
			{
				type = "method",
				description = "Sets the source offset position the line start point is attached at this source game object.",
				args = "(Vector3 sourceOffsetPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSourceOffsetPosition =
			{
				type = "function",
				description = "Gets the source offset position the line start point.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setTargetOffsetPosition =
			{
				type = "method",
				description = "Sets the target offset position the line end point is attached at this target game object. If the target game object does not exist, the target offset position is at global space.",
				args = "(Vector3 targetOffsetPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTargetOffsetPosition =
			{
				type = "function",
				description = "Gets the target offset position the line end point.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			}
		}
	},
	LinePath =
	{
		type = "class",
		description = "Generates a procedural line path."
	},
	LinesComponent =
	{
		type = "class",
		description = "Usage: Creates one or several lines. It can be used in lua script e.g. for an audio spectrum visualization",
		inherits = "GameObjectComponent",
		childs = 
		{
			setConnected =
			{
				type = "method",
				description = "Sets whether the lines should be connected together forming a more complex object.",
				args = "(boolean connected)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getConnected =
			{
				type = "function",
				description = "Gets whether the lines are be connected together.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setOperationType =
			{
				type = "method",
				description = "Sets the operation type. Possible values are: A list of points, 1 vertex per point OT_POINT_LIST = 1, A list of lines, 2 vertices per line OT_LINE_LIST = 2, A strip of connected lines, 1 vertex per line plus 1 start vertex  OT_LINE_STRIP = 3, A list of triangles, 3 vertices per triangle OT_TRIANGLE_LIST = 4, A strip of triangles, 3 vertices for the first triangle,  and 1 per triangle after that OT_TRIANGLE_STRIP = 5",
				args = "(string operationType)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getOperationType =
			{
				type = "function",
				description = "Gets used operation type.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setLinesCount =
			{
				type = "method",
				description = "Sets the lines count.",
				args = "(number linesCount)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getLinesCount =
			{
				type = "function",
				description = "Gets the lines count.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setColor =
			{
				type = "method",
				description = "Sets color for the line (r, g, b).",
				args = "(number index, Vector3 color)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getColor =
			{
				type = "function",
				description = "Gets line color.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setStartPosition =
			{
				type = "method",
				description = "Sets the start position for the line.",
				args = "(number index, Vector3 startPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getStartPosition =
			{
				type = "function",
				description = "Gets the start position of the line.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setEndPosition =
			{
				type = "method",
				description = "Sets the end position for the line.",
				args = "(number index, Vector3 endPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getEndPosition =
			{
				type = "function",
				description = "Gets the end position of the line.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			}
		}
	},
	ListBox =
	{
		type = "class",
		description = "MyGUI list box class.",
		childs = 
		{
			getItemCount =
			{
				type = "function",
				description = "Gets the count of entries of the list box",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			insertItemAt =
			{
				type = "method",
				description = "Inserts an item at the given index.",
				args = "(?)",
				returns = "(nil)",
				valuetype = "nil"
			},
			addItem =
			{
				type = "method",
				description = "Adds an item.",
				args = "(string name)",
				returns = "(nil)",
				valuetype = "nil"
			},
			removeItemAt =
			{
				type = "method",
				description = "Removes an item at the given index.",
				args = "(number index)",
				returns = "(nil)",
				valuetype = "nil"
			},
			removeAllItems =
			{
				type = "method",
				description = "Removes all items.",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			},
			swapItemsAt =
			{
				type = "method",
				description = "Swaps two items at the given index.",
				args = "(number index1, number index2)",
				returns = "(nil)",
				valuetype = "nil"
			},
			findItemIndexWith =
			{
				type = "function",
				description = "Finds an item with the given index.",
				args = "(string name)",
				returns = "(number)",
				valuetype = "number"
			},
			getIndexSelected =
			{
				type = "function",
				description = "Gets the selected index.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setItemNameAt =
			{
				type = "method",
				description = "Sets the name at the given index.",
				args = "(number index, string name)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getItemNameAt =
			{
				type = "function",
				description = "Gets the name by the given index.",
				args = "(number index)",
				returns = "(string)",
				valuetype = "string"
			}
		}
	},
	LuaGoal =
	{
		type = "class",
		description = "A leaf atomic goal, which cannot have children anymore.",
		childs = 
		{
			isComplete =
			{
				type = "function",
				description = "Gets whether this goal is completed.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			isActive =
			{
				type = "function",
				description = "Gets whether this goal is still active.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			isInactive =
			{
				type = "function",
				description = "Gets whether this goal is inactive.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			hasFailed =
			{
				type = "function",
				description = "Gets whether this goal has failed.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			}
		}
	},
	LuaGoalComposite =
	{
		type = "class",
		description = "Composite goal class used for managing hierarchical goal structures.",
		inherits = "LuaGoal",
		childs = 
		{
			isComplete =
			{
				type = "function",
				description = "Gets whether this goal is completed.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			isActive =
			{
				type = "function",
				description = "Gets whether this goal is still active.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			isInactive =
			{
				type = "function",
				description = "Gets whether this goal is inactive.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			hasFailed =
			{
				type = "function",
				description = "Gets whether this goal has failed.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			addSubGoal =
			{
				type = "method",
				description = "Adds a new atomic or composite sub-goal to this composite goal.",
				args = "(Table luaGoalTable)",
				returns = "(nil)",
				valuetype = "nil"
			},
			removeAllSubGoals =
			{
				type = "method",
				description = "Removes and terminates all sub-goals managed by this composite goal.",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			},
			activate =
			{
				type = "method",
				description = "Activates this composite goal by calling its 'activate' function in Lua.",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			},
			terminate =
			{
				type = "method",
				description = "Terminates this composite goal and calls its 'terminate' function in Lua.",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			},
			process =
			{
				type = "function",
				description = "Processes the composite goal and its sub-goals, calling their 'process' functions in Lua. Returns the result of the goal's processing.",
				args = "(Ogre::Real dt)",
				returns = "(GoalResult)",
				valuetype = "GoalResult"
			}
		}
	},
	LuaScriptComponent =
	{
		type = "class",
		description = "Usage: Creates a lua script in the project folder for this game object for powerful script manipulation, game mechanics etc.Several game object may use the same script, if the script name for the lua script components is the same. This is usefull if they should behave the same. This also increases the performance",
		inherits = "GameObjectComponent",
		childs = 
		{
			callDelayedMethod =
			{
				type = "method",
				description = "Calls a lua closure function in lua script after a delay in seconds. Note: The game object is optional and may be nil.",
				args = "(func closureFunction, Ogre::Real delaySec)",
				returns = "(nil)",
				valuetype = "nil"
			},
			connect =
			{
				type = "method",
				description = "Calls connect for the given game object.",
				args = "(GameObject gameObject)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	ManualObjectComponent =
	{
		type = "class",
		description = "Usage: Creates manual objects. It can be used in lua script e.g. for an audio spectrum visualizationIt is much more performant as LinesComponent, because only one manual object with x start- and end-positions is created.The difference to LinesComponent is, that each end - position is also the next start position.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setConnected =
			{
				type = "method",
				description = "Sets whether the manual objects should be connected together forming a more complex object.",
				args = "(boolean connected)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getConnected =
			{
				type = "function",
				description = "Gets whether the manual objects are be connected together.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setOperationType =
			{
				type = "method",
				description = "Sets the operation type. Possible values are: A list of points, 1 vertex per point OT_POINT_LIST = 1, A list of lines, 2 vertices per line OT_LINE_LIST = 2, A strip of connected lines, 1 vertex per line plus 1 start vertex  OT_LINE_STRIP = 3, A list of triangles, 3 vertices per triangle OT_TRIANGLE_LIST = 4, A strip of triangles, 3 vertices for the first triangle,  and 1 per triangle after that OT_TRIANGLE_STRIP = 5",
				args = "(string operationType)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getOperationType =
			{
				type = "function",
				description = "Gets used operation type.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setLinesCount =
			{
				type = "method",
				description = "Sets the manual object count.",
				args = "(number linesCount)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getLinesCount =
			{
				type = "function",
				description = "Gets the lines count.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setColor =
			{
				type = "method",
				description = "Sets color for the manual object (r, g, b).",
				args = "(number index, Vector3 color)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getColor =
			{
				type = "function",
				description = "Gets the manual object color.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setStartPosition =
			{
				type = "method",
				description = "Sets the start position for the manual object.",
				args = "(number index, Vector3 startPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getStartPosition =
			{
				type = "function",
				description = "Gets the start position of the manual object.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setEndPosition =
			{
				type = "method",
				description = "Sets the end position for the manual object.",
				args = "(number index, Vector3 endPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getEndPosition =
			{
				type = "function",
				description = "Gets the end position of the manual object.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			}
		}
	},
	MathHelper =
	{
		type = "class",
		description = "MathHelper for utilities functions for mathematical operations and ray casting.",
		childs = 
		{
			MathHelper =
			{
				type = "value"
			},
			calibratedFromScreenSpaceToWorldSpace =
			{
				type = "function",
				description = "Maps an 3D-object to the graphics scene from 2D-mouse coordinates.",
				args = "(SceneNode node, Camera camera, Vector2 mousePosition, number offset)",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			getCalibratedWindowCoordinates =
			{
				type = "function",
				description = "Gets the relative position to the window size. For example mapping an crosshair overlay object and conrolling it via mouse or Wii controller..",
				args = "(number x, number y, number width, number height, Viewport viewport)",
				returns = "(Vector4)",
				valuetype = "Vector4"
			},
			lowPassFilter =
			{
				type = "function",
				description = "A low pass filter in order to smooth chaotic values. This is for example useful for the position determination of infrared camera of the Wii controller or smooth movement at all.",
				args = "(number value, number oldValue, number scale)",
				returns = "(number)",
				valuetype = "number"
			},
			lowPassFilter =
			{
				type = "function",
				description = "A low pass filter for vector3 in order to smooth chaotic values. This is for example useful for the position determination of infrared camera of the Wii controller or smooth movement at all.",
				args = "(Vector3 value, Vector3 oldValue, number scale)",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			clamp =
			{
				type = "function",
				description = "Clamps the value between min and max. The time t e.g. t = (time - animationStart) / duration",
				args = "(number value, number min, number max)",
				returns = "(number)",
				valuetype = "number"
			},
			round =
			{
				type = "function",
				description = "Rounds a number according to the number of places. E.g. 0.5 will be 1.0 and 4.443 will be 4.44 when number of places is set to 2.",
				args = "(number number, number places)",
				returns = "(number)",
				valuetype = "number"
			},
			round =
			{
				type = "function",
				description = "Rounds a number.",
				args = "(number number)",
				returns = "(number)",
				valuetype = "number"
			},
			round =
			{
				type = "function",
				description = "Rounds a vector according to the number of places.",
				args = "(Vector2 vec, number places)",
				returns = "(Vector2)",
				valuetype = "Vector2"
			},
			round =
			{
				type = "function",
				description = "Rounds a vector according to the number of places.",
				args = "(Vector3 vec, number places)",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			round =
			{
				type = "function",
				description = "Rounds a vector according to the number of places.",
				args = "(Vector4 vec, number places)",
				returns = "(Vector4)",
				valuetype = "Vector4"
			},
			diffPitch =
			{
				type = "function",
				description = "Gets the difference pitch of two quaternions.",
				args = "(Quaternion dest, Quaternion src)",
				returns = "(Quaternion)",
				valuetype = "Quaternion"
			},
			diffYaw =
			{
				type = "function",
				description = "Gets the difference yaw of two quaternions.",
				args = "(Quaternion dest, Quaternion src)",
				returns = "(Quaternion)",
				valuetype = "Quaternion"
			},
			diffRoll =
			{
				type = "function",
				description = "Gets the difference roll of two quaternions.",
				args = "(Quaternion dest, Quaternion src)",
				returns = "(Quaternion)",
				valuetype = "Quaternion"
			},
			diffDegree =
			{
				type = "function",
				description = "Gets the difference of two quaternions. The mode, 1: pitch quaternion, 2: yaw quaternion, 3: roll quaternion will be calculated.",
				args = "(Quaternion dest, Quaternion, src, number mode)",
				returns = "(Quaternion)",
				valuetype = "Quaternion"
			},
			extractEuler =
			{
				type = "function",
				description = "Extracts the yaw (heading), pitch and roll out of the quaternion. Returns the Vector3 in the form: Vector3(heading, pitch, roll)	The Vector3 holding pitch angle in radians for x, the heading angle in radians for y and the roll angle in radians for z.",
				args = "(Quaternion quat)",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			getOrientationFromHeadingPitch =
			{
				type = "function",
				description = "Gets the new orientation out from the current orientation and its steering angle in radians and pitch angle in radians, taking the default model direction into account.",
				args = "(Quaternion orientation, number steeringAngle, number pitchAngle, Vector3 defaultModelDirection)",
				returns = "(Quaternion)",
				valuetype = "Quaternion"
			},
			faceTarget =
			{
				type = "function",
				description = "Gets the orientation in order to face a target, this orientation can be set directly.",
				args = "(SceneNode source, SceneNode dest)",
				returns = "(Quaternion)",
				valuetype = "Quaternion"
			},
			faceTarget =
			{
				type = "function",
				description = "Gets the orientation in order to face a target, this orientation can be set directly, taking the default direction of the source game object into account.",
				args = "(SceneNode source, SceneNode dest, Vector3 defaultDirection)",
				returns = "(Quaternion)",
				valuetype = "Quaternion"
			},
			getAngle =
			{
				type = "function",
				description = "Gets angle between two vectors. The normal of the two vectors is used in conjunction with the signed angle parameter, to determine whether the angle between the two vectors is negative or positive. This function can be use e.g. when using a gizmo or a grabber to rotate objects via mouse in the correct direction.",
				args = "(Vector3 dir1, Vector3 dir2, Vector3 normal, boolean signedAngle)",
				returns = "(Radian)",
				valuetype = "Radian"
			},
			lookAt =
			{
				type = "function",
				description = "Gets the orientation in order to look a the target direction with a fixed axis. Note: The fixed axis y is used so that the result quaternion will not pitch or roll.",
				args = "(Vector3 unnormalizedDirection)",
				returns = "(Quaternion)",
				valuetype = "Quaternion"
			},
			faceDirectionSlerp =
			{
				type = "function",
				description = "Gets the orientation slerp steps in order to face towards a direction vector and also uses the default direction of the source.",
				args = "(Quaternion sourceOrientation, Vector3 direction, Vector3 defaultDirection, number dt, number rotationSpeed)",
				returns = "(Quaternion)",
				valuetype = "Quaternion"
			},
			normalizeDegreeAngle =
			{
				type = "function",
				description = "Normalizes the degree angle, e.g. a value bigger 180 would normally be set to -180, so after normalization even 181 degree are possible.",
				args = "(number degree)",
				returns = "(number)",
				valuetype = "number"
			},
			normalizeRadianAngle =
			{
				type = "function",
				description = "Normalizes the radian angle, e.g. a value bigger pi would normally be set to -pi, so after normalization even pi + 0.03 degree are possible.",
				args = "(number radian)",
				returns = "(number)",
				valuetype = "number"
			},
			degreeAngleEquals =
			{
				type = "function",
				description = "Checks whether the given degree angles are equal. Internally the angles are normalized.",
				args = "(number degree0, number degree1)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			radianAngleEquals =
			{
				type = "function",
				description = "Checks whether the given radian angles are equal. Internally the radian are normalized.",
				args = "(number radian0, number radian1)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			numberEquals =
			{
				type = "function",
				description = "Compares two real objects, using tolerance for inaccuracies.",
				args = "(number first, Vector2 float, number tolerance)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			vector2Equals =
			{
				type = "function",
				description = "Compares two vector2 objects, using tolerance for inaccuracies.",
				args = "(Vector2 first, Vector2 second, number tolerance)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			vector3Equals =
			{
				type = "function",
				description = "Compares two vector3 objects, using tolerance for inaccuracies.",
				args = "(Vector3 first, Vector3 second, number tolerance)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			vector4Equals =
			{
				type = "function",
				description = "Compares two vector4 objects, using tolerance for inaccuracies.",
				args = "(Vector4 first, Vector4 second, number tolerance)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			quaternionEquals =
			{
				type = "function",
				description = "Compares two quaternion objects, using tolerance for inaccuracies.",
				args = "(Quaternion first, Quaternion second, number tolerance)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			degreeAngleToRadian =
			{
				type = "function",
				description = "Converts degree angle to radian.",
				args = "(number degree)",
				returns = "(number)",
				valuetype = "number"
			},
			radianAngleToDegree =
			{
				type = "function",
				description = "Converts radian angle to degree.",
				args = "(number radian)",
				returns = "(number)",
				valuetype = "number"
			},
			quatToDegreeAngleAxis =
			{
				type = "function",
				description = "Builds the orientation in the form of: degree, x-axis, y-axis, z-axis for better usability.",
				args = "(Quaternion quat)",
				returns = "(Vector4)",
				valuetype = "Vector4"
			},
			quatToDegrees =
			{
				type = "function",
				description = "Builds the orientation in the form of: degreeX, degreeY, degreeZ for better usability.",
				args = "(Quaternion quat)",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			quatToDegreesRounded =
			{
				type = "function",
				description = "Builds the rounded orientation in the form of: degreeX, degreeY, degreeZ for better usability.",
				args = "(Quaternion quat)",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			degreesToQuat =
			{
				type = "function",
				description = "Builds the	quaternion from degreeX, degreeY, degreeZ.",
				args = "(Vector3 degrees)",
				returns = "(Quaternion)",
				valuetype = "Quaternion"
			},
			calculateRotationGridValue =
			{
				type = "function",
				description = "Calculates the rotation grid value.",
				args = "(number step, number angle)",
				returns = "(number)",
				valuetype = "number"
			},
			calculateRotationGridValue =
			{
				type = "function",
				description = "Calculates the rotation grid value.",
				args = "(Quaternion orientation, number step, number angle)",
				returns = "(number)",
				valuetype = "number"
			},
			getRandomNumberInt =
			{
				type = "function",
				description = "Gets a random number between the given min and max.",
				args = "(number min, number max)",
				returns = "(number)",
				valuetype = "number"
			},
			getRandomNumberLong =
			{
				type = "function",
				description = "Gets a random number between the given min and max.",
				args = "(number min, number max)",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	MathWindows =
	{
		type = "class",
		description = "Math windows class for audio spectrum analysis, for more harmonic visualization."
	},
	Matrix3 =
	{
		type = "class",
		description = "Matrix3 class."
	},
	MeshLinearTransform =
	{
		type = "class",
		description = "Performs a linear transform on a procedural mesh."
	},
	MeshUVTransform =
	{
		type = "class",
		description = "Transforms UV on a procedural mesh."
	},
	MessageBoxStyle =
	{
		type = "class",
		description = "MessageBox style class for MyGUI Message box reaction, which button has been pressed.",
		childs = 
		{
			Yes =
			{
				type = "value"
			},
			No =
			{
				type = "value"
			},
			Abort =
			{
				type = "value"
			},
			Retry =
			{
				type = "value"
			},
			Ignore =
			{
				type = "value"
			},
			Cancel =
			{
				type = "value"
			},
			Try =
			{
				type = "value"
			},
			Continue =
			{
				type = "value"
			}
		}
	},
	MinimapComponent =
	{
		type = "class",
		description = "Usage: Can be used for painting a minimap. Also fog of war is possible and the given game object id is tracked. Requirements: This component must be placed under a separate game object with a camera component, which is not the MainCamera. Note: The minimap can only be used with default direction of -z, which is also the default graphics engine mesh direction. Note: If terra shall be rendered properly on minimap, this minimap camera game object id, must be set in the TerraComponent as camera id.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets whether this component should be activated or not.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this component is activated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setVisibilityRadius =
			{
				type = "method",
				description = "If fog of war is used, sets the visibilty radius which discovers the fog of war.",
				args = "(number visibilityRadius)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getVisibilityRadius =
			{
				type = "function",
				description = "Gets the visibilty radius which discovers the fog of war.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			clearDiscoveryState =
			{
				type = "method",
				description = "Clears the area which has been already discovered and saves the state.",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			},
			saveDiscoveryState =
			{
				type = "method",
				description = "Saves the area which has been already discovered.",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			},
			loadDiscoveryState =
			{
				type = "method",
				description = "Loads the area which has been already discovered.",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			},
			setCameraHeight =
			{
				type = "method",
				description = "If whole scene visible is set to false, sets the camera height, which is added at the top of the y position of the target game object.",
				args = "(number cameraHeight)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCameraHeight =
			{
				type = "function",
				description = "If whole scene visible is set to false, gets the camera height, which is added at the top of the y position of the target game object.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			}
		}
	},
	Modifier =
	{
		type = "class",
		description = "Modifier class",
		childs = 
		{
			Alt =
			{
				type = "value"
			},
			Shift =
			{
				type = "value"
			},
			Ctrl =
			{
				type = "value"
			},
			CapsLock =
			{
				type = "value"
			},
			NumLock =
			{
				type = "value"
			}
		}
	},
	Mouse =
	{
		type = "class",
		description = "OIS mouse class.",
		childs = 
		{
			getMouseState =
			{
				type = "function",
				description = "Gets the mouse button state.",
				args = "()",
				returns = "(MouseState)",
				valuetype = "MouseState"
			}
		}
	},
	MouseButtonID =
	{
		type = "class",
		description = "MouseButtonID class",
		childs = 
		{
			MB_LEFT =
			{
				type = "value"
			},
			MB_MIDDLE =
			{
				type = "value"
			},
			MB_BUTTON_3 =
			{
				type = "value"
			},
			MB_BUTTON_4 =
			{
				type = "value"
			},
			MB_BUTTON_5 =
			{
				type = "value"
			},
			MB_BUTTON_6 =
			{
				type = "value"
			},
			MB_BUTTON_7 =
			{
				type = "value"
			}
		}
	},
	MouseState =
	{
		type = "class",
		description = "MouseState of OIS mouse.",
		childs = 
		{
			width =
			{
				type = "value"
			},
			height =
			{
				type = "value"
			},
			X =
			{
				type = "value"
			},
			Y =
			{
				type = "value"
			},
			Z =
			{
				type = "value"
			}
	},	},
	MovableObject =
	{
		type = "class",
		description = "Base class for Ogre objects.",
		childs = 
		{
			getParentSceneNode =
			{
				type = "function",
				description = "Gets the parent scene node, at which this movable object is attached to.",
				args = "()",
				returns = "(SceneNode)",
				valuetype = "SceneNode"
			},
			setVisible =
			{
				type = "method",
				description = "Switches the visibility of the movable object.",
				args = "(boolean visible)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getVisible =
			{
				type = "function",
				description = "Gets whether this movable object is visible.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			}
		}
	},
	MoveMathFunctionComponent =
	{
		type = "class",
		description = "Usage: Creates a mathematical function for physics active component movement. Example: XFunction0 = 20 - cos(t) * 20 YFunction0 = 0 ZFunction0 = sin(t) * 20 MinLength0 = 0 MaxLength0 = 2 * PI Speed0 = 5 XFunction1 = -20 - cos(t) * -20 YFunction1 = 0 ZFunction1 = sin(t) * 10 MinLength1 = 0 MaxLength1 = 2 * PI Speed1 = 2  Will move via functions0 the game object in a circle in x, z of radius 20 and when finished will move via functions1 the other direction an ellipse, forming a strange infinite curve. Note: 20 - ... * 20 should be used to offset the origin, so that the game object will not warp, but remains as origin. Other useful functions: XFunction0 = 10 - cos(t) * 10; YFunction0 = 1; ZFunction0 = 10 + sin(t) * 10 (screw function)Note: Functions behave different when using a kind of PhysicsActiveComponent, because not the absolute position can be taken, but the relative velocity or omega force must be used.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets whether move math function can start or not.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this move math is activated or not.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			reactOnFunctionFinished =
			{
				type = "method",
				description = "Sets wheather to react the game object function movement has finished.",
				args = "(func closureFunction)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	MovingBehavior =
	{
		type = "class",
		description = "Moving behavior controls one or several game objects (agents) in a artificial intelligent manner. Note: Its possible to combine behaviors.",
		childs = 
		{
			getPath =
			{
				type = "function",
				description = "Gets path for waypoints manipulation.",
				args = "()",
				returns = "(Path)",
				valuetype = "Path"
			},
			setRotationSpeed =
			{
				type = "method",
				description = "Sets the agent rotation speed. Default value is 0.1.",
				args = "(number speed)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isSwitchOn =
			{
				type = "function",
				description = "Gets whether a specifig behavior is switched on.",
				args = "(BehaviorType behaviorType)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			getTargetAgent =
			{
				type = "function",
				description = "Gets the target agent or nil, if does not exist.",
				args = "()",
				returns = "(GameObject)",
				valuetype = "GameObject"
			},
			getTargetAgent2 =
			{
				type = "function",
				description = "Gets the target agent 2 or nil, if does not exist.",
				args = "()",
				returns = "(GameObject)",
				valuetype = "GameObject"
			},
			setTargetAgentId =
			{
				type = "method",
				description = "Sets or changes the target agent id.",
				args = "(string id)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setTargetAgentId2 =
			{
				type = "method",
				description = "Sets or changes the target agent id 2 for interpose behavior.",
				args = "(string id2)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setWanderJitter =
			{
				type = "method",
				description = "Sets the wander jitter. Default value is 1.",
				args = "(number jitter)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setWanderRadius =
			{
				type = "method",
				description = "Sets the wander radius. Default value is 1.2.",
				args = "(number radius)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setWanderDistance =
			{
				type = "method",
				description = "Sets the wander distance. Default value is 20.",
				args = "(number distance)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setObstacleHideData =
			{
				type = "method",
				description = "Sets the categories of game objects which do count as obstacle and a range radius, at which the agent will hide, if 'HIDE' behavior is set. Note: Combined categories can be used like 'ALL-Player' etc.",
				args = "(string obstaclesHideCategories, number obstacleRangeRadius)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setObstacleAvoidanceData =
			{
				type = "method",
				description = "Sets the categories of game objects which do count as obstacle and a range radius, at which the agent will start turning from the obstacle. Note: Combined categories can be used like 'ALL-Player' etc.",
				args = "(string obstaclesHideCategories, number obstacleRangeRadius)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setGoalRadius =
			{
				type = "method",
				description = "Sets the tolerance radius, at which the agent is considered to be at the goal. Note: If the radius is to small, It may happen, that the agent never reaches the goal. Default value is 0.2.",
				args = "(number radius)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getGoalRadius =
			{
				type = "function",
				description = "Gets the tolerance radius, at which the agent is considered to be at the goal.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setActualizePathDelaySec =
			{
				type = "method",
				description = "Sets the interval in seconds in which a changed path will be re-created.",
				args = "(number delay)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getActualizePathDelaySec =
			{
				type = "function",
				description = "Gets the interval in seconds in which a changed path will be re-created.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setPathFindData =
			{
				type = "method",
				description = "Sets the path find data, if recast navigation is enabled. Note: Since its possible to have several paths running concurrently each path search must have an unique slot. The pathSlot identifies the target the path leads to. The targetSlot specifies the slot in which the found path is to be stored. The drawPath variable specifies whether to draw the path or not for debug purposes. The border values for slots are: [0, 128].",
				args = "(number pathSlot, number targetSlot, boolean drawPath)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPathSlot =
			{
				type = "function",
				description = "Gets the used path slot. Note: Recast navigation must be enabled.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getPathTargetSlot =
			{
				type = "function",
				description = "Gets the used path target slot. Note: Recast navigation must be enabled.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			findPath =
			{
				type = "method",
				description = "Tries to find an optimal path. It takes a start point and an end point and, if possible, generates a list of lines in a path. It might fail if the start or end points aren't near any navigation mesh polygons, or if the path is too long, or it can't make a path, or various other reasons.So far I've not had problems though. Note: Recast navigation must be enabled.",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			},
			setFlyMode =
			{
				type = "method",
				description = "Sets whether to use fly mode (taking y-axis into account). Note: this can be used e.g. for birds flying around.",
				args = "(boolean flyMode)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isInFlyMode =
			{
				type = "function",
				description = "Gets whether the agent is in flying mode (taking y-axis into account).",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setNeighborDistance =
			{
				type = "method",
				description = "Sets the neighbor agent distance in a flocking scenario. Note: 'FLOCKING' behavior must be switched on. Default value is 1.6 meters.",
				args = "(number distance)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getNeighborDistance =
			{
				type = "function",
				description = "Gets the neighbor agent distance in a flocking scenario.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setBehavior =
			{
				type = "method",
				description = "Clears all behaviors and set the given behavior.",
				args = "(BehaviorType behaviorType)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setBehavior2 =
			{
				type = "method",
				description = "Clears all behaviors and set the given behavior by string.",
				args = "(Strimg behaviorType)",
				returns = "(nil)",
				valuetype = "nil"
			},
			addBehavior =
			{
				type = "method",
				description = "Adds the given behavior to already existing behaviors.",
				args = "(BehaviorType behaviorType)",
				returns = "(nil)",
				valuetype = "nil"
			},
			addBehavior2 =
			{
				type = "method",
				description = "Adds the given behavior to already existing behaviors by string.",
				args = "(Strimg behaviorType)",
				returns = "(nil)",
				valuetype = "nil"
			},
			removeBehavior =
			{
				type = "method",
				description = "Removes the given behavior from already existing behaviors.",
				args = "(BehaviorType behaviorType)",
				returns = "(nil)",
				valuetype = "nil"
			},
			removeBehavior2 =
			{
				type = "method",
				description = "Removes the given behavior from already existing behaviors by string.",
				args = "(Strimg behaviorType)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCurrentBehavior =
			{
				type = "function",
				description = "Gets the current behavior constellation as composed string in the form 'SEEK ARRIVE' etc.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			reset =
			{
				type = "method",
				description = "Resets all behaviors, clears paths etc. and sets the default behavior 'NONE'.",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAgentId =
			{
				type = "function",
				description = "Gets the agent (game object) id.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setWeightSeparation =
			{
				type = "method",
				description = "Sets the weight for separation rule in a flocking scenario. Note: 'FLOCKING' behavior must be switched on. Default value is 1.",
				args = "(number weight)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWeightSeparation =
			{
				type = "function",
				description = "Gets the weight for separation rule in a flocking scenario.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setWeightCohesion =
			{
				type = "method",
				description = "Sets the weight for cohesion rule in a flocking scenario. Note: 'FLOCKING' behavior must be switched on. Default value is 1.",
				args = "(number weight)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWeightCohesion =
			{
				type = "function",
				description = "Gets the weight for cohesion rule in a flocking scenario.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setWeightAlignment =
			{
				type = "method",
				description = "Sets the weight for alignment rule in a flocking scenario. Note: 'FLOCKING' behavior must be switched on. Default value is 1.",
				args = "(number weight)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWeightAlignment =
			{
				type = "function",
				description = "Gets the weight for alignment rule in a flocking scenario.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setWeightWander =
			{
				type = "method",
				description = "Sets the weight for wander behavior. Default value is 1.",
				args = "(number weight)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWeightWander =
			{
				type = "function",
				description = "Gets the weight for wander behavior.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setWeightObstacleAvoidance =
			{
				type = "method",
				description = "Sets the weight for obstacle avoidance behavior. Default value is 1.",
				args = "(number weight)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWeightObstacleAvoidance =
			{
				type = "function",
				description = "Gets the weight for obstacle avoidance behavior. Default value is 1.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setWeightSeek =
			{
				type = "method",
				description = "Sets the weight for seek behavior. Default value is 1.",
				args = "(number weight)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWeightSeek =
			{
				type = "function",
				description = "Gets the weight for seek behavior.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setWeightFlee =
			{
				type = "method",
				description = "Sets the weight for flee behavior. Default value is 1.",
				args = "(number weight)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWeightFlee =
			{
				type = "function",
				description = "Gets the weight for flee behavior.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setWeightArrive =
			{
				type = "method",
				description = "Sets the weight for arrive behavior. Default value is 1.",
				args = "(number weight)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWeightArrive =
			{
				type = "function",
				description = "Gets the weight for arrive behavior.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setWeightPursuit =
			{
				type = "method",
				description = "Sets the weight for pursuit behavior. Default value is 1.",
				args = "(number weight)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWeightPursuit =
			{
				type = "function",
				description = "Sets the weight for pursuit behavior. Default value is 1.",
				args = "(number weight)",
				returns = "(number)",
				valuetype = "number"
			},
			setWeightOffsetPursuit =
			{
				type = "method",
				description = "Sets the weight for offset pursuit behavior. Default value is 1.",
				args = "(number weight)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWeightOffsetPursuit =
			{
				type = "function",
				description = "Sets the weight for offset pursuit behavior. Default value is 1.",
				args = "(number weight)",
				returns = "(number)",
				valuetype = "number"
			},
			setWeightHide =
			{
				type = "method",
				description = "Sets the weight for hide behavior. Default value is 1.",
				args = "(number weight)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWeightHide =
			{
				type = "function",
				description = "Gets the weight for hide behavior.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setWeightEvade =
			{
				type = "method",
				description = "Sets the weight for evade behavior. Default value is 1.",
				args = "(number weight)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWeightEvade =
			{
				type = "function",
				description = "Gets the weight for evade behavior.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setWeightFollowPath =
			{
				type = "method",
				description = "Sets the weight for follow path behavior. Default value is 1.",
				args = "(number weight)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWeightFollowPath =
			{
				type = "function",
				description = "Gets the weight for follow path behavior.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setWeightInterpose =
			{
				type = "method",
				description = "Sets the weight for interpose behavior. Default value is 1.",
				args = "(number weight)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWeightInterpose =
			{
				type = "function",
				description = "Gets the weight for interpose behavior.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getIsStuck =
			{
				type = "function",
				description = "Gets whether the agent is stuck (can not reach the goal).",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setStuckCheckTime =
			{
				type = "method",
				description = "The time in seconds the agent is stuck, until the current behavior will be removed. 0 disables the stuck check.",
				args = "(number timeSec)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMotionDistanceChange =
			{
				type = "function",
				description = "Gets motion distance change of the agent. The value is clamped from 0 to 1. 0 means the agent is not moving and 1 the agent is moving at current speed.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setAutoOrientation =
			{
				type = "method",
				description = "Sets whether the agent should be auto orientated during ai movement.",
				args = "(boolean autoOrientation)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setAutoAnimation =
			{
				type = "method",
				description = "Sets whether to use auto animation during ai movement. That is, the animation speed is adapted dynamically depending the velocity, which will create a much more realistic effect. Note: The game object must have a proper configured animation component.",
				args = "(boolean autoAnimation)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setOffsetPosition =
			{
				type = "method",
				description = "Sets the offset position for offset pursuit behavior.",
				args = "(Vector3 offsetPosition)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	MultiShape =
	{
		type = "class",
		description = "Combines procedural shapes."
	},
	MyGUIButtonComponent =
	{
		type = "class",
		description = "Usage: A MyGUI UI button component.",
		inherits = "MyGUIComponent",
		childs = 
		{
			setCaption =
			{
				type = "method",
				description = "Sets the caption for this widget.",
				args = "(string caption)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCaption =
			{
				type = "function",
				description = "Gets the caption of this widget.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setFontHeight =
			{
				type = "method",
				description = "Sets the font height for this widget. Note: Minimum value is 10 and maximum value is 60. Odd font sizes will be rounded to the next even font size. E.g. 23 will become 24.",
				args = "(number fontHeight)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getFontHeight =
			{
				type = "function",
				description = "Gets the font height of this widget.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setTextAlign =
			{
				type = "method",
				description = "Sets the widget text alignment. Possible values are: 'HCenter', 'Center', 'Left', 'Right', 'HStretch', 'Top', 'Bottom', 'VStretch', 'Stretch', 'Default' (LEFT|TOP).",
				args = "(string align)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTextAlign =
			{
				type = "function",
				description = "Gets the widget text alignment.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setTextColor =
			{
				type = "method",
				description = "Sets the widget's text color (r, g, b, a).",
				args = "(Vector4 color)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTextColor =
			{
				type = "function",
				description = "Gets the widget's text color (r, g, b, a).",
				args = "()",
				returns = "(Vector4)",
				valuetype = "Vector4"
			}
		}
	},
	MyGUICheckBoxComponent =
	{
		type = "class",
		description = "Usage: A MyGUI UI check box component.",
		inherits = "MyGUIComponent",
		childs = 
		{
			setCaption =
			{
				type = "method",
				description = "Sets the caption for this widget.",
				args = "(string caption)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCaption =
			{
				type = "function",
				description = "Gets the caption of this widget.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setFontHeight =
			{
				type = "method",
				description = "Sets the font height for this widget. Note: Minimum value is 10 and maximum value is 60. Odd font sizes will be rounded to the next even font size. E.g. 23 will become 24.",
				args = "(number fontHeight)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getFontHeight =
			{
				type = "function",
				description = "Gets the font height of this widget.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setTextAlign =
			{
				type = "method",
				description = "Sets the widget text alignment. Possible values are: 'HCenter', 'Center', 'Left', 'Right', 'HStretch', 'Top', 'Bottom', 'VStretch', 'Stretch', 'Default' (LEFT|TOP).",
				args = "(string align)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTextAlign =
			{
				type = "function",
				description = "Gets the widget text alignment.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setTextColor =
			{
				type = "method",
				description = "Sets the widget's text color (r, g, b, a).",
				args = "(Vector4 color)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTextColor =
			{
				type = "function",
				description = "Gets the widget's text color (r, g, b, a).",
				args = "()",
				returns = "(Vector4)",
				valuetype = "Vector4"
			}
		}
	},
	MyGUIComboBoxComponent =
	{
		type = "class",
		description = "Usage: A MyGUI UI combo box component.",
		inherits = "MyGUIComponent",
		childs = 
		{
			setItemCount =
			{
				type = "method",
				description = "Sets items count.",
				args = "(number itemCount)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getItemCount =
			{
				type = "function",
				description = "Gets the items count.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setItemText =
			{
				type = "method",
				description = "Sets the item text at the given index.",
				args = "(number index, string itemText)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getItemText =
			{
				type = "function",
				description = "Gets the the item at the given index.",
				args = "(number index)",
				returns = "(string)",
				valuetype = "string"
			},
			addItem =
			{
				type = "method",
				description = "Adds an item.",
				args = "(string item)",
				returns = "(nil)",
				valuetype = "nil"
			},
			insertItem =
			{
				type = "method",
				description = "Inserts an item at the given index.",
				args = "(number index, string item)",
				returns = "(nil)",
				valuetype = "nil"
			},
			removeItem =
			{
				type = "method",
				description = "Removes the item at the given index.",
				args = "(number index)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSelectedIndex =
			{
				type = "function",
				description = "Gets the selected index, or -1 if nothing is selected. Note: Always check against -1!",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			reactOnSelected =
			{
				type = "method",
				description = "Sets whether to react if a list item has been selected. The clicked list item index will be received.",
				args = "(func closureFunction, number index)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	MyGUIComponent =
	{
		type = "class",
		description = "MyGUIComponent class",
		childs = 
		{
			reactOnSpawn =
			{
				type = "method",
				description = "Sets whether to react at the moment when a game object is spawned.",
				args = "(func closure, spawnedGameObject, originGameObject)",
				returns = "(nil)",
				valuetype = "nil"
			},
			reactOnVanish =
			{
				type = "method",
				description = "Sets whether to react at the moment when a game object is vanished.",
				args = "(func closure, spawnedGameObject, originGameObject)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setActivated =
			{
				type = "method",
				description = "Controls the visibility of the widget.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this widget is visible or not.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setRealPosition =
			{
				type = "method",
				description = "Sets the relative position of the widget. Note: Relative means: 0,0 is left top corner and 1,1 is right bottom corner of the whole screen.",
				args = "(Vector2 position)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRealPosition =
			{
				type = "function",
				description = "Gets the relative position of the widget.",
				args = "()",
				returns = "(Vector2)",
				valuetype = "Vector2"
			},
			setRealSize =
			{
				type = "method",
				description = "Sets the relative size of the widget. Note: Relative means, a size of 1,1 the widget would fill the whole screen.",
				args = "(Vector2 size)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRealSize =
			{
				type = "function",
				description = "Gets the relative size of the widget.",
				args = "()",
				returns = "(Vector2)",
				valuetype = "Vector2"
			},
			setAlign =
			{
				type = "method",
				description = "Sets the widget alignment. Possible values are: 'HCenter', 'Center', 'Left', 'Right', 'HStretch', 'Top', 'Bottom', 'VStretch', 'Stretch', 'Default' (LEFT|TOP).",
				args = "(string align)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAlign =
			{
				type = "function",
				description = "Gets the widget alignment.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setLayer =
			{
				type = "method",
				description = "Sets the widget's layer (z-order). Possible values are: 'Wallpaper', 'ToolTip', 'Info', 'FadeMiddle', 'Popup', 'Main', 'Modal', 'Middle', 'Overlapped', 'Back', 'DragAndDrop', 'FadeBusy', 'Pointer', 'Fade', 'Statistic'. Default value is 'POPUP'",
				args = "(string layer)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getLayer =
			{
				type = "function",
				description = "Gets the widget's layer (z-order).",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setColor =
			{
				type = "method",
				description = "Sets the widget's background color (r, g, b, a).",
				args = "(Vector4 color)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getColor =
			{
				type = "function",
				description = "Gets the widget's background color (r, g, b, a).",
				args = "()",
				returns = "(Vector4)",
				valuetype = "Vector4"
			},
			setEnabled =
			{
				type = "method",
				description = "Sets whether the widget is enabled.",
				args = "(boolean loop)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getEnabled =
			{
				type = "function",
				description = "Gets whether whether the widget is enabled.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			getId =
			{
				type = "function",
				description = "Gets the id of this widget. Note: Widgets can be attached as child of other widgets and be transformed relatively to their parent widget. Hence each widget has an id and its parent id can be filled with the id of another widget component in the same game object..",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setParentId =
			{
				type = "method",
				description = "Sets the parent widget id. Note: Widgets can be attached as child of other widgets and be transformed relatively to their parent widget. Hence each widget has an id and its parent id can be filled with the id of another widget component in the same game object..",
				args = "(string parentId)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getParentId =
			{
				type = "function",
				description = "Gets the parent id of this widget.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			reactOnMouseButtonClick =
			{
				type = "method",
				description = "Sets whether to react if a mouse button has been clicked on the given widget.",
				args = "(func closureFunction)",
				returns = "(nil)",
				valuetype = "nil"
			},
			reactOnMouseEnter =
			{
				type = "method",
				description = "Sets whether to react if mouse has entered the given widget (hover start).",
				args = "(func closureFunction)",
				returns = "(nil)",
				valuetype = "nil"
			},
			reactOnMouseButtonClick =
			{
				type = "method",
				description = "Sets whether to react if a mouse button has been clicked on the inventory. The clicked resource name will be received and the clicked mouse button id.",
				args = "(func closureFunction, string resourceName, number buttonId)",
				returns = "(nil)",
				valuetype = "nil"
			},
			reactOnMouseButtonClick =
			{
				type = "method",
				description = "Sets whether to react if a mouse button has been clicked on the minimap. The clicked map tile index will be received.",
				args = "(func closureFunction, number mapTileIndex)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	MyGUIControllerComponent =
	{
		type = "class",
		description = "Usage: A base MyGUI UI controller component.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Activates the controller for a target widget component.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether the controller for a target widget component is activated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setSourceId =
			{
				type = "method",
				description = "Sets the source MyGUI UI widget component id, at which this controller should be attached to.",
				args = "(string sourceId)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSourceId =
			{
				type = "function",
				description = "Gets the source MyGUI UI widget component id, at which this controller is attached.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			}
		}
	},
	MyGUIEdgeHideControllerComponent =
	{
		type = "class",
		description = "Usage: This controller is used for hiding widgets near screen edges. Widget will start hiding(move out of screen) if it's near border and it and it's childrens don't have any focus. Hiding till only small part of widget be visible. Widget will move inside screen if it have any focus.",
		inherits = "MyGUIControllerComponent",
		childs = 
		{
			setTimeSec =
			{
				type = "method",
				description = "Sets the time in seconds in which widget will be hidden or shown.",
				args = "(number timeSec)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTimeSec =
			{
				type = "function",
				description = "Gets the time in seconds in which widget will be hidden or shown.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setRemainPixels =
			{
				type = "method",
				description = "Sets how many pixels are visible after full hide.",
				args = "(number pixels)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRemainPixels =
			{
				type = "function",
				description = "Gets how many pixels are visible after full hide.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setShadowSize =
			{
				type = "method",
				description = "Sets the shadow size, which will be added to 'remain pixels' when hiding left or top (for example used for windows with shadows).",
				args = "(number shadowSize)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getShadowSize =
			{
				type = "function",
				description = "Gets the shadow size, which will be added to 'remain pixels' when hiding left or top.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	MyGUIFadeAlphaControllerComponent =
	{
		type = "class",
		description = "Usage: A MyGUI UI fade alpha controller component.",
		inherits = "MyGUIControllerComponent",
		childs = 
		{
			setAlpha =
			{
				type = "method",
				description = "Sets the alpha that will be as result of changing.",
				args = "(number alpha)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAlpha =
			{
				type = "function",
				description = "Gets the target alpha.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setCoefficient =
			{
				type = "method",
				description = "Sets the coefficient of alpha changing speed. Note: 1 means that alpha will change from 0 to 1 at 1 second.",
				args = "(number coefficient)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCoefficient =
			{
				type = "function",
				description = "Gets the coefficient of alpha changing speed.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	MyGUIImageBoxComponent =
	{
		type = "class",
		description = "Usage: A MyGUI UI image box component.",
		inherits = "MyGUIComponent",
		childs = 
		{
			setImageFileName =
			{
				type = "method",
				description = "Sets the image file name. Note: Must exist in a resource group.",
				args = "(string fileName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getImageFileName =
			{
				type = "function",
				description = "Gets the used image file name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setCenter =
			{
				type = "method",
				description = "Sets the center position of the image. Note: Since an image can be rotated, the center position is imporant. Default is 0.5, 0.5.",
				args = "(Vector2 center)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCenter =
			{
				type = "function",
				description = "Gets the center position of the image.",
				args = "()",
				returns = "(Vector2)",
				valuetype = "Vector2"
			},
			setAngle =
			{
				type = "method",
				description = "Sets the image angle in degrees. Note: @setRotationSpeed must be 0, else the image will be rotating and overwriting the current angle.",
				args = "(number angleDegrees)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAngle =
			{
				type = "function",
				description = "Gets the image angle in degrees. Note: If @setRotationSpeed is set different from 0, the current angle continues to change.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setRotationSpeed =
			{
				type = "method",
				description = "Sets the image rotation speed, if its desired, that the image should rotate.",
				args = "(number rotationSpeed)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRotationSpeed =
			{
				type = "function",
				description = "Gets the image rotation speed.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	MyGUIItemBoxComponent =
	{
		type = "class",
		description = "Usage: A MyGUI UI item box component, for inventory usage in conjunction with InventoryItemComponent's.",
		inherits = "MyGUIWindowComponent",
		childs = 
		{
			getItemCount =
			{
				type = "function",
				description = "Gets the max inventory item count.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getResourceName =
			{
				type = "function",
				description = "Gets resource name for the given index. E.g. if 'energy' resource is placed in inventory at index 1, getResourceName(1) would deliver the energy resource. Note: This function can be used in a loop in conjunction with @getItemCount to iterate over all items in inventory and dump all resources.",
				args = "(number index)",
				returns = "(string)",
				valuetype = "string"
			},
			setQuantity2 =
			{
				type = "method",
				description = "Sets the quantity of the resource for the given index.",
				args = "(number index, number quantity)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setQuantity =
			{
				type = "method",
				description = "Sets the quantity of the resource.",
				args = "(number index, string resourceName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getQuantity2 =
			{
				type = "function",
				description = "Gets the quantity of the resource for the given index.",
				args = "(number index)",
				returns = "(number)",
				valuetype = "number"
			},
			getQuantity =
			{
				type = "function",
				description = "Gets the quantity of the resource.",
				args = "(string resourceName)",
				returns = "(number)",
				valuetype = "number"
			},
			increaseQuantity =
			{
				type = "method",
				description = "Gets the current quantity and increases by the given quantity value.",
				args = "(string resourceName, number quantity)",
				returns = "(nil)",
				valuetype = "nil"
			},
			decreaseQuantity =
			{
				type = "method",
				description = "Gets the current quantity and decreases by the given quantity value.",
				args = "(string resourceName, number quantity)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSellValue2 =
			{
				type = "function",
				description = "Gets the sell value of the resource for the given index.",
				args = "(number index)",
				returns = "(number)",
				valuetype = "number"
			},
			getSellValue =
			{
				type = "function",
				description = "Gets the sell value of the resource.",
				args = "(string resourceName)",
				returns = "(number)",
				valuetype = "number"
			},
			getBuyValue2 =
			{
				type = "function",
				description = "Gets the buy value of the resource for the given index.",
				args = "(number index)",
				returns = "(number)",
				valuetype = "number"
			},
			getBuyValue =
			{
				type = "function",
				description = "Gets the buy value of the resource.",
				args = "(string resourceName)",
				returns = "(number)",
				valuetype = "number"
			},
			addQuantity =
			{
				type = "method",
				description = "Adds the quantity of the resource, if does not exist, creates a new slot in inventory.",
				args = "(string resourceName, number quantity)",
				returns = "(nil)",
				valuetype = "nil"
			},
			removeQuantity =
			{
				type = "method",
				description = "Removes the quantity from the resource.",
				args = "(string resourceName, number quantity)",
				returns = "(nil)",
				valuetype = "nil"
			},
			clearItems =
			{
				type = "method",
				description = "Cleares the whole inventory.",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			},
			reactOnDropItemRequest =
			{
				type = "method",
				description = "Sets whether to react if an item is requested to be drag and dropped to another inventory. A return value also can be set to prohibit the operation. E.g. getMyGUIItemBoxComponent():reactOnDropItemRequest(function(dragDropData) ... end",
				args = "(func closureFunction, DragDropData dragDropData)",
				returns = "(nil)",
				valuetype = "nil"
			},
			reactOnDropItemAccepted =
			{
				type = "method",
				description = "Sets whether to react if an item drop has been accepted to another inventory. E.g. getMyGUIItemBoxComponent():reactOnDropItemAccepted(function(dragDropData) ... end",
				args = "(func closureFunction, DragDropData dragDropData)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	MyGUIListBoxComponent =
	{
		type = "class",
		description = "Usage: A MyGUI UI list box component.",
		inherits = "MyGUIComponent",
		childs = 
		{
			setItemCount =
			{
				type = "method",
				description = "Sets items count.",
				args = "(number itemCount)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getItemCount =
			{
				type = "function",
				description = "Gets the items count.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setItemText =
			{
				type = "method",
				description = "Sets the item text at the given index.",
				args = "(number index, string itemText)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getItemText =
			{
				type = "function",
				description = "Gets the the item at the given index.",
				args = "(number index)",
				returns = "(string)",
				valuetype = "string"
			},
			addItem =
			{
				type = "method",
				description = "Adds an item.",
				args = "(string item)",
				returns = "(nil)",
				valuetype = "nil"
			},
			insertItem =
			{
				type = "method",
				description = "Inserts an item at the given index.",
				args = "(number index, string item)",
				returns = "(nil)",
				valuetype = "nil"
			},
			removeItem =
			{
				type = "method",
				description = "Removes the item at the given index.",
				args = "(number index)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSelectedIndex =
			{
				type = "function",
				description = "Gets the selected index, or -1 if nothing is selected. Note: Always check against -1!",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			reactOnSelected =
			{
				type = "method",
				description = "Sets whether to react if a list item has been selected. The clicked list item index will be received.",
				args = "(func closureFunction, number index)",
				returns = "(nil)",
				valuetype = "nil"
			},
			reactOnAccept =
			{
				type = "method",
				description = "Sets whether to react if a list item change has been accepted. The accepted list item index will be received.",
				args = "(func closureFunction, number index)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	MyGUIMessageBoxComponent =
	{
		type = "class",
		description = "Usage: A MyGUI UI combo box component.",
		inherits = "MyGUIComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Activates this component, so that the message box will be shown.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether the message box is activated (shown).",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setTitle =
			{
				type = "method",
				description = "Sets the title. Note: Using #{Identifier}, the text can be presented in a prior specified language.",
				args = "(string title)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTitle =
			{
				type = "function",
				description = "Gets the title.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setMessage =
			{
				type = "method",
				description = "Sets the message. Note: Using #{Identifier}, the text can be presented in a prior specified language.",
				args = "(string message)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMessage =
			{
				type = "function",
				description = "Gets the Message.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setStylesCount =
			{
				type = "method",
				description = "Sets the message box styles count.",
				args = "(number stylesCount)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getStylesCount =
			{
				type = "function",
				description = "Gets the styles count.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setStyle =
			{
				type = "method",
				description = "Sets the style at the given index. Possible values are: 'Ok', 'Yes', 'No', 'Abord', 'Retry', 'Ignore', 'Cancel', 'Try', 'Continue', 'IconInfo', 'IconQuest', 'IconError', 'IconWarning'. Note: Several styles can be combined.",
				args = "(number index, string style)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getStyle =
			{
				type = "function",
				description = "Gets the the style at the given index.",
				args = "(number index)",
				returns = "(string)",
				valuetype = "string"
			}
		}
	},
	MyGUIMiniMapComponent =
	{
		type = "class",
		description = "Usage: A MyGUI UI minimap component. This component will search for all exit components in all scenes of the current project and build a minimap of all scenes with a target game object painted as dot.Note: The minimap component can use the 'mouseClickEventName' in order to specify a function name for being executed in lua script, if the corresponding minimap tile has been clicked.",
		inherits = "MyGUIWindowComponent",
		childs = 
		{
			showMiniMap =
			{
				type = "method",
				description = "Shows the minimap. Internally generates the minimap from the scratch and the trackables.",
				args = "(boolean show)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isMiniMapShown =
			{
				type = "function",
				description = "Gets whether the minimap is shown.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setStartPosition =
			{
				type = "method",
				description = "Sets the relative start position, at which the whole minimap will be generated. Default value is 0,0 (left, top corner). E.g. setting to 0.5, 0.5 would start in the middle of the screen.",
				args = "(Vector2 startPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getStartPosition =
			{
				type = "function",
				description = "Gets the start position, at which the whole minimap will be generated.",
				args = "()",
				returns = "(Vector2)",
				valuetype = "Vector2"
			},
			setUseVisitation =
			{
				type = "method",
				description = "If activated, only minimap tiles are visible, which are marked as visited, via @setVisited(...) functionality.",
				args = "(boolean useVisitation)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getUseVisitation =
			{
				type = "function",
				description = "Gets whether only minimap tiles are visible, which are marked as visited, via @setVisited(...) functionality.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			getMiniMapTilesCount =
			{
				type = "function",
				description = "Gets the generated minimap tiles count.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setMiniMapTileColor =
			{
				type = "method",
				description = "Sets the color for the given minimap tile index.",
				args = "(number index, Vector3 color)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMiniMapTileColor =
			{
				type = "function",
				description = "Gets the color for the given minimap tile index.",
				args = "(number index)",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setToolTipDescription =
			{
				type = "method",
				description = "Sets the tooltip description for the given minimap tile index.",
				args = "(number index, string description)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getToolTipDescription =
			{
				type = "function",
				description = "Gets the tooltip description for the given minimap tile index.",
				args = "(number index)",
				returns = "(string)",
				valuetype = "string"
			},
			setTrackableCount =
			{
				type = "method",
				description = "Sets the count of trackables. Trackables are important points on a map, like items or save room etc.",
				args = "(number count)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTrackableCount =
			{
				type = "function",
				description = "Gets the count of trackables.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setTrackableId =
			{
				type = "method",
				description = "Sets the trackable id for the given trackable index, that is shown on the mini map. Note: If the trackable id is in another scene, the scene name must be specified. For example 'scene3:2341435213'. Will search in scene3 for the game object with the id 2341435213 and in conjunction with the image attribute, the image will be placed correctly on the minimap.If the scene name is missing, its assumed, that the id is an global one (like the player which is available for each scene) and has the same id for each scene.",
				args = "(number index, string id)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTrackableId =
			{
				type = "function",
				description = "Gets the count of trackables.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setTrackableImage =
			{
				type = "method",
				description = "Sets the image name for the given trackable index.",
				args = "(number index, string imageName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTrackableImage =
			{
				type = "function",
				description = "Gets the image name for the given trackable index.",
				args = "(number index)",
				returns = "(string)",
				valuetype = "string"
			},
			setTrackableImageTileSize =
			{
				type = "method",
				description = "Sets the image tile size for the given trackable index. Details: Image may be of size: 32x64, but tile size 32x32, so that sprite animation is done automatically switching the image tiles from 0 to 32 and 32 to 64 automatically.",
				args = "(number index, Vector2 tileSize)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTrackableImageTileSize =
			{
				type = "function",
				description = "Gets the image name for the given trackable index.",
				args = "(number index)",
				returns = "(Vector2)",
				valuetype = "Vector2"
			},
			generateMiniMap =
			{
				type = "method",
				description = "Generates the minimap from the scratch manually.",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			},
			generateTrackables =
			{
				type = "method",
				description = "Generates the trackables from the scratch manually.",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			},
			setSceneVisited =
			{
				type = "method",
				description = "Sets whether the given level (scene) index has been visited and shall be visible on the minimap.",
				args = "(number index, boolean visited)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getIsSceneVisited =
			{
				type = "function",
				description = "Gets whether the given level (scene) index has been visited and is visible on the minimap.",
				args = "(number index)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setSceneVisited =
			{
				type = "method",
				description = "Sets whether the given level (scene) name has been visited and shall be visible on the minimap.",
				args = "(string index, boolean visited)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	MyGUIPositionControllerComponent =
	{
		type = "class",
		description = "Usage: A MyGUI UI position controller component.",
		inherits = "MyGUIControllerComponent",
		childs = 
		{
			setCoordinate =
			{
				type = "method",
				description = "Sets the target transform coordinate at which the widget will be moved and resized. Note: x,y is the new relative position and z,w is the new size. If z,w is set to 0, the widget will keep its current size.",
				args = "(Vector4 coordinate)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCoordinate =
			{
				type = "function",
				description = "Gets the target transform coordinate.",
				args = "()",
				returns = "(Vector4)",
				valuetype = "Vector4"
			},
			setFunction =
			{
				type = "method",
				description = "Sets the movement function. Possible values are: 'Inertional', 'Accelerated', 'Slowed', 'Jump'.",
				args = "(string movementFunction)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getFunction =
			{
				type = "function",
				description = "Gets the used movement function.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setDurationSec =
			{
				type = "method",
				description = "Sets the duration in seconds after which the widget should be at the target coordinate.",
				args = "(number durationSec)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDurationSec =
			{
				type = "function",
				description = "Gets the duration in seconds.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	MyGUIProgressBarComponent =
	{
		type = "class",
		description = "Usage: A MyGUI UI progress bar component.",
		inherits = "MyGUIComponent",
		childs = 
		{
			setValue =
			{
				type = "method",
				description = "Sets the progress bar current value.",
				args = "(number value)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getValue =
			{
				type = "function",
				description = "Gets the progress bar current value.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setRange =
			{
				type = "method",
				description = "Sets the range (max value) for the progress bar.",
				args = "(number value)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRange =
			{
				type = "function",
				description = "Gets the range (max value) for the progress bar.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setFlowDirection =
			{
				type = "method",
				description = "Sets flow direction for the progress bar.",
				args = "(string flowDirection)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getFlowDirection =
			{
				type = "function",
				description = "Gets the flow direction for the progress bar. Possible values are: 'LeftToRight', 'RightToLeft', 'TopToBottom', 'BottomToTop'",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			}
		}
	},
	MyGUIRepeatClickControllerComponent =
	{
		type = "class",
		description = "Usage: A MyGUI UI repeat click controller component.",
		inherits = "MyGUIControllerComponent",
		childs = 
		{
			setTimeLeftSec =
			{
				type = "method",
				description = "Sets the delay in second before the first event will be triggered.",
				args = "(number timeLeftSec)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTimeLeftSec =
			{
				type = "function",
				description = "Gets the delay in second before the first event will be triggered.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setStep =
			{
				type = "method",
				description = "Sets the delay after each event before the next event is triggered.",
				args = "(number step)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getStep =
			{
				type = "function",
				description = "Gets the delay after each event before the next event is triggered.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	MyGUIScrollingMessageControllerComponent =
	{
		type = "class",
		description = "Usage: A MyGUI UI scrolling message controller component.",
		inherits = "MyGUIControllerComponent",
		childs = 
		{
			setMessageCount =
			{
				type = "method",
				description = "Sets scrolling message line count. Note: Each message occupies one line.",
				args = "(number count)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMessageCount =
			{
				type = "function",
				description = "Gets the scrolling message line count.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setMessage =
			{
				type = "method",
				description = "Sets scrolling message at the given index.",
				args = "(number index, string message)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMessage =
			{
				type = "function",
				description = "Gets the scrolling message from the given index.",
				args = "(number index)",
				returns = "(string)",
				valuetype = "string"
			}
		}
	},
	MyGUITextComponent =
	{
		type = "class",
		description = "Usage: A MyGUI UI text component.",
		inherits = "MyGUIComponent",
		childs = 
		{
			setCaption =
			{
				type = "method",
				description = "Sets the caption for this widget. Note: If multi line is enabled, backslash 'n' can be used for a line break.",
				args = "(string caption)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCaption =
			{
				type = "function",
				description = "Gets the caption of this widget.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setFontHeight =
			{
				type = "method",
				description = "Sets the font height for this widget. Note: Minimum value is 10 and maximum value is 60. Odd font sizes will be rounded to the next even font size. E.g. 23 will become 24.",
				args = "(number fontHeight)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getFontHeight =
			{
				type = "function",
				description = "Gets the font height of this widget.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setTextAlign =
			{
				type = "method",
				description = "Sets the widget text alignment. Possible values are: 'HCenter', 'Center', 'Left', 'Right', 'HStretch', 'Top', 'Bottom', 'VStretch', 'Stretch', 'Default' (LEFT|TOP).",
				args = "(string align)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTextAlign =
			{
				type = "function",
				description = "Gets the widget text alignment.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setTextColor =
			{
				type = "method",
				description = "Sets the widget's text color (r, g, b, a).",
				args = "(Vector4 color)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTextColor =
			{
				type = "function",
				description = "Gets the widget's text color (r, g, b, a).",
				args = "()",
				returns = "(Vector4)",
				valuetype = "Vector4"
			},
			setReadOnly =
			{
				type = "method",
				description = "Sets whether the widget is read only (Text cannot be manipulated).",
				args = "(boolean readonly)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getReadOnly =
			{
				type = "function",
				description = "Gets whether the widget is read only (Text cannot be manipulated).",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			}
		}
	},
	MyGUIWindowComponent =
	{
		type = "class",
		description = "Usage: A MyGUI UI window component.",
		inherits = "MyGUIComponent",
		childs = 
		{
			setSkin =
			{
				type = "method",
				description = "Sets the widgets skin. Possible values are: 'WindowCSX', 'Window', 'WindowC', 'WindowCX', 'WindowCS', 'WindowCX2_Dark', 'WindowCXS2_Dark', 'Back1Skin_Dark', 'PanelSkin'.",
				args = "(string skin)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSkin =
			{
				type = "function",
				description = "Gets the widgets skin.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setMovable =
			{
				type = "method",
				description = "Sets whether this widget can be moved manually. Default value is false.",
				args = "(boolean movable)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMovable =
			{
				type = "function",
				description = "Gets whether this widget can be moved manually.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setWindowCaption =
			{
				type = "method",
				description = "Sets the title caption for this widget.",
				args = "(string caption)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWindowCaption =
			{
				type = "function",
				description = "Gets the title caption of this widget.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			}
		}
	},
	MyGuiSpriteComponent =
	{
		type = "class",
		description = "Usage: An image with sprite tiles can be loaded and manipulated, so that a sprite movement is possible.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets whether this component should be activated and the sprite animation take place.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this component is activated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setCurrentRow =
			{
				type = "method",
				description = "Sets manually the current to be displayed tile row, if animation shall take place in lua script.",
				args = "(number currentRow)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCurrentRow =
			{
				type = "function",
				description = "Gets the current row.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setCurrentCol =
			{
				type = "method",
				description = "Sets manually the current to be displayed tile column, if animation shall take place in lua script.",
				args = "(number currentCol)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCurrentCol =
			{
				type = "function",
				description = "Gets the current column.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	Node =
	{
		type = "class",
		description = "Base class for SceneNodes."
	},
	NodeTrackComponent =
	{
		type = "class",
		description = "Usage: This Component is used for camera tracking in conjunction with several NodeComponents acting as waypoints.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets whether this node track is activated or not.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this node track is activated or not.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setNodeTrackCount =
			{
				type = "method",
				description = "Sets the node track count (how many nodes are used for the tracking).",
				args = "(number nodeTrackCount)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getNodeTrackCount =
			{
				type = "function",
				description = "Gets the node track count.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setNodeTrackId =
			{
				type = "method",
				description = "Sets the node track id for the given index in the node track list with @nodeTrackCount elements. Note: The order is controlled by the index, from which node to which node this game object will be tracked.",
				args = "(number index, string id)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getNodeTrackId =
			{
				type = "function",
				description = "Gets node track id from the given node track index from list.",
				args = "(number index)",
				returns = "(string)",
				valuetype = "string"
			},
			setTimePosition =
			{
				type = "method",
				description = "Sets time position in milliseconds after which this game object should be tracked at the node from the given index.",
				args = "(number index, number timePosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTimePosition =
			{
				type = "function",
				description = "Gets time position in milliseconds for the node with the given index.",
				args = "(number index)",
				returns = "(number)",
				valuetype = "number"
			},
			setInterpolationMode =
			{
				type = "method",
				description = "Sets the curve interpolation mode how the game object will be moved. Possible values are: 'Spline', 'Linear'",
				args = "(string interpolationMode)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getInterpolationMode =
			{
				type = "function",
				description = "Gets the curve interpolation mode how the game object is moved. Possible values are: 'Spline', 'Linear'",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setRotationMode =
			{
				type = "method",
				description = "Sets the rotation mode how the game object will be rotated during movement. Possible values are: 'Linear', 'Spherical'",
				args = "(string rotationMode)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRotationMode =
			{
				type = "function",
				description = "Gets the rotation mode how the game object is rotated during movement. Possible values are: 'Linear', 'Spherical'",
				args = "(void)",
				returns = "(string)",
				valuetype = "string"
			}
		}
	},
	OgreALModule =
	{
		type = "class",
		description = "OgreALModule for some OgreAL utilities operations.",
		childs = 
		{
			setContinue =
			{
				type = "method",
				description = "Sets whether the sound manager continues with the current scene manager. Note: This can be used when sounds are used in an AppState, but another AppState is pushed on the top, yet still the sounds should continue playing from the prior AppState. E.g. start music in MenuState and continue playing when pushing LoadMenuState and going back again to MenuState.",
				args = "(boolean bContinue)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	OgreNewtModule =
	{
		type = "class",
		description = "OgreNewtModule for some physics utilities operations.",
		childs = 
		{
			showOgreNewtCollisionLines =
			{
				type = "method",
				description = "Shows the debug physics collision lines.",
				args = "(boolean show)",
				returns = "(nil)",
				valuetype = "nil"
			},
			debugDrawNavMesh =
			{
				type = "method",
				description = "Draws debug navigation mesh lines.",
				args = "(boolean draw)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	OgreRecastModule =
	{
		type = "class",
		description = "OgreRecastModule for navigation mesh operations."
	},
	OldBone =
	{
		type = "class",
		description = "Class for managing a v1 skeleton bone.",
		childs = 
		{
			setManuallyControlled =
			{
				type = "method",
				description = "Sets whether this v1 old bone should be controlled manually. Note: Animation will no longer transform this v1 old bone.",
				args = "(boolean manuallyControlled)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setInheritOrientation =
			{
				type = "method",
				description = "Tells the OldNode whether it should inherit orientation from it's parent OldNode.",
				args = "(boolean inherit)",
				returns = "(nil)",
				valuetype = "nil"
			},
			convertLocalToWorldPosition =
			{
				type = "function",
				description = "Gets the world position of a point in the OldNode local space useful for simple transforms that don't require a child OldNode.",
				args = "(Vector3 worldPosition)",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			convertLocalToWorldPosition =
			{
				type = "function",
				description = "Gets the world position of a point in the OldNode local space useful for simple transforms that don't require a child OldNode.",
				args = "(Vector3 localPosition)",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			convertWorldToLocalOrientation =
			{
				type = "function",
				description = "Gets the local orientation, relative to this OldNode, of the given world-space orientation.",
				args = "(Quaternion worldOrientation)",
				returns = "(Quaternion)",
				valuetype = "Quaternion"
			},
			convertLocalToWorldOrientation =
			{
				type = "function",
				description = "Gets the world orientation of an orientation in the OldNode local space useful for simple transforms that don't require a child OldNode.",
				args = "(Quaternion localOrientation)",
				returns = "(Quaternion)",
				valuetype = "Quaternion"
			},
			getParent =
			{
				type = "function",
				description = "Gets this OldNode's parent (nil if this is the root).",
				args = "()",
				returns = "(OldNode)",
				valuetype = "OldNode"
			}
		}
	},
	OldNode =
	{
		type = "class",
		description = "Class for managing a v1 skeleton node.",
		childs = 
		{
			getPosition =
			{
				type = "function",
				description = "Gets the position of this v1 skeleton node.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			getOrientation =
			{
				type = "function",
				description = "Gets the Orientation of this v1 skeleton node.",
				args = "()",
				returns = "(Quaternion)",
				valuetype = "Quaternion"
			},
			setDirection =
			{
				type = "method",
				description = "Sets the direction of this v1 skeleton node.",
				args = "(Vector3 vector, Vector3 localDirectionVector)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPositionDerived =
			{
				type = "function",
				description = "Gets the derived position of this v1 skeleton node.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			getOrientationDerived =
			{
				type = "function",
				description = "Gets the derived Orientation of this v1 skeleton node.",
				args = "()",
				returns = "(Quaternion)",
				valuetype = "Quaternion"
			},
			setPosition =
			{
				type = "method",
				description = "Sets the position of this v1 skeleton node.",
				args = "(Vector3 vector)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setPosition2 =
			{
				type = "method",
				description = "Sets the position of this v1 skeleton node.",
				args = "(number x, number y, number z)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setDerivedPosition =
			{
				type = "method",
				description = "Sets the derived position of this v1 skeleton node.",
				args = "(Vector3 vector)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setDerivedOrientation =
			{
				type = "method",
				description = "Sets the derived orientation of this v1 skeleton node.",
				args = "(Quaternion quaternion)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setOrientation =
			{
				type = "method",
				description = "Sets the absolute orientation of this v1 skeleton node.",
				args = "(Quaternion quaternion)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setOrientation =
			{
				type = "method",
				description = "Sets the absolute orientation of this v1 skeleton node.",
				args = "(number w, number x, number y, number z)",
				returns = "(nil)",
				valuetype = "nil"
			},
			roll =
			{
				type = "method",
				description = "Rolls this v1 skeleton node relative.",
				args = "(Vector vector)",
				returns = "(nil)",
				valuetype = "nil"
			},
			pitch =
			{
				type = "method",
				description = "Pitches this v1 skeleton node relative.",
				args = "(Vector vector)",
				returns = "(nil)",
				valuetype = "nil"
			},
			yaw =
			{
				type = "method",
				description = "Yaws this v1 skeleton node relative.",
				args = "(Vector vector)",
				returns = "(nil)",
				valuetype = "nil"
			},
			translate =
			{
				type = "method",
				description = "Translates the v1 skeleton node by the given vector from its current position.",
				args = "(Vector3 vector)",
				returns = "(nil)",
				valuetype = "nil"
			},
			translate2 =
			{
				type = "method",
				description = "Translates the v1 skeleton node by the given vector from its current position.",
				args = "(number x, number y, number z)",
				returns = "(nil)",
				valuetype = "nil"
			},
			rotate =
			{
				type = "method",
				description = "Rotates the v1 skeleton node relative.",
				args = "(Quaternion quaternion)",
				returns = "(nil)",
				valuetype = "nil"
			},
			rotate2 =
			{
				type = "method",
				description = "Rotates the v1 skeleton node relative.",
				args = "(number w, number x, number y, number z)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	ParticleFxComponent =
	{
		type = "class",
		description = "Usage: My usage text.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets whether this component should be activated or not.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this component is activated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			}
		}
	},
	ParticleUniverseComponent =
	{
		type = "class",
		description = "Usage: This Component is in order for playing nice particle effects using the ParticleUniverse-Engine.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets whether this component should be activated or not (Start the particle effect).",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this component is activated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setTemplateName =
			{
				type = "method",
				description = "Sets the particle template name. The name must be recognized by the resource system, else the particle effect cannot be played.",
				args = "(string particleTemplateName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTemplateName =
			{
				type = "function",
				description = "Gets currently used particle template name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setRepeat =
			{
				type = "method",
				description = "Sets whether the current particle effect should be repeated when finished.",
				args = "(boolean repeat)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRepeat =
			{
				type = "function",
				description = "Gets whether the current particle effect will be repeated when finished.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setPlayTimeMS =
			{
				type = "method",
				description = "Sets particle play time in milliseconds, how long the particle effect should be played.",
				args = "(number particlePlayTimeMS)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPlayTimeMS =
			{
				type = "function",
				description = "Gets particle play time in milliseconds.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setOffsetPosition =
			{
				type = "method",
				description = "Sets offset position of the particle effect at which it should be played away from the game object.",
				args = "(Vector3 offsetPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getOffsetPosition =
			{
				type = "function",
				description = "Gets offset position of the particle effect.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setOffsetOrientation =
			{
				type = "method",
				description = "Sets offset orientation (as vector3(degree, degree, degree)) of the particle effect at which it should be played away from the game object.",
				args = "(Vector3 orientation)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setScale =
			{
				type = "method",
				description = "Sets the scale (size) of the particle effect.",
				args = "(Vector3 scale)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getScale =
			{
				type = "function",
				description = "Gets scale of the particle effect.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setPlaySpeed =
			{
				type = "method",
				description = "Sets particle play speed. E.g. 2 will play the particle at double speed.",
				args = "(number playSpeed)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPlaySpeed =
			{
				type = "function",
				description = "Gets particle play play speed.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			isPlaying =
			{
				type = "function",
				description = "Gets whether the particle is playing. Note: This affects the value of @PlayTimeMS.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setGlobalPosition =
			{
				type = "method",
				description = "Sets a global play position for the particle.",
				args = "(Vector3 globalPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setGlobalOrientation =
			{
				type = "method",
				description = "Sets a global player orientation (as vector3(degree, degree, degree)) of the particle effect.",
				args = "(Vector3 globalOrientation)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	Path =
	{
		type = "class",
		description = "Creates a path for ai path follow component.",
		childs = 
		{
			isFinished =
			{
				type = "function",
				description = "Gets whether the end of the path list has been reached by the agent.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			getCurrentWayPoint =
			{
				type = "function",
				description = "Gets the current way point position.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setRepeat =
			{
				type = "method",
				description = "Sets whether the path traversal should be repeated, if the path is finished.",
				args = "(boolean repeat)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRepeat =
			{
				type = "function",
				description = "Gets whether the path traversal should be repeated, if the path is finished.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setDirectionChange =
			{
				type = "method",
				description = "Sets whether the traversal direction should be changed, if path is finished.",
				args = "(boolean directionChange)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDirectionChange =
			{
				type = "function",
				description = "Gets whether the traversal direction should be changed, if path is finished.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			addWayPoint =
			{
				type = "method",
				description = "Adds a new way point at the end of the list.",
				args = "(Vector3 newPoint)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setWayPoint =
			{
				type = "method",
				description = "Clears the list and sets a new way point.",
				args = "(Vector3 newPoint)",
				returns = "(nil)",
				valuetype = "nil"
			},
			clear =
			{
				type = "method",
				description = "Clears the way point list.",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWayPoints =
			{
				type = "function",
				description = "Gets the list of all waypoints.",
				args = "()",
				returns = "(Table[Vector3])",
				valuetype = "Table[Vector3]"
			}
		}
	},
	PhysicsActiveComponent =
	{
		type = "class",
		description = "Derived class of PhysicsComponent. It can be used for physically active game objects (movable).",
		inherits = "PhysicsComponent",
		childs = 
		{
			setLinearDamping =
			{
				type = "method",
				description = "Sets the linear damping. Range: [0, 1]",
				args = "(number linearDamping)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getLinearDamping =
			{
				type = "function",
				description = "Gets the linear damping.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setAngularDamping =
			{
				type = "method",
				description = "Sets the angular damping. Range: [0, 1]",
				args = "(Vector3 angularDamping)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAngularDamping =
			{
				type = "function",
				description = "Gets the angular damping.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			translate =
			{
				type = "method",
				description = "Sets relative position of the physics component.",
				args = "(Vector3 relativePosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			addImpulse =
			{
				type = "method",
				description = "Adds an impulse at the given offset position away from the origin of the physics body.",
				args = "(Vector3 deltaVector, Vector3 offsetPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setVelocity =
			{
				type = "method",
				description = "Sets the global linear velocity on the physics body. Note: This should only be used for initzialisation. Use @applyRequiredForceForVelocity in simualtion instead. Or it may be called if its a physics active kinematic body.",
				args = "(Vector3 velocity)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setDirectionVelocity =
			{
				type = "method",
				description = "Sets the velocity speed for the current direction. Note: The default direction axis is applied. See: @GameObject::getDefaultDirection(). Note: This should only be set for initialization and not during simulation, as It could break physics calculation, or it may be called if its a physics active kinematic body.",
				args = "(number speed)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getVelocity =
			{
				type = "function",
				description = "Gets currently acting velocity on the body.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setMass =
			{
				type = "method",
				description = "Sets mass for the body. Note: Internally the inertial values are adapted.",
				args = "(number mass)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMass =
			{
				type = "function",
				description = "Gets the mass of the body.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setMassOrigin =
			{
				type = "method",
				description = "Sets mass origin for the body. Note: The mass origin is calculated automatically, but there are cases, in which the user wants to specify its own origin, e.g. balancing bird.",
				args = "(Vector3 massOrigin)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMassOrigin =
			{
				type = "function",
				description = "Gets the manually set or calculated mass origin of the body.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setSpeed =
			{
				type = "method",
				description = "Sets the speed for this game object. Note: Speed is used in conjunction with other components like the AiMoveComponent etc.",
				args = "(number speed)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSpeed =
			{
				type = "function",
				description = "Gets the speed.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setMaxSpeed =
			{
				type = "method",
				description = "Sets the max speed for this game object. Note: Max speed is used in conjunction with other components like the AiMoveComponent etc.",
				args = "(number maxSpeed)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMaxSpeed =
			{
				type = "function",
				description = "Gets the max speed.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getMinSpeed =
			{
				type = "function",
				description = "Gets the min speed.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			applyOmegaForce =
			{
				type = "method",
				description = "Applies omega force vector of the physics body. Note: This should be used during simulation instead of @setOmegaVelocity.",
				args = "(Vector3 omegaForce)",
				returns = "(nil)",
				valuetype = "nil"
			},
			applyOmegaForceRotateTo =
			{
				type = "method",
				description = "Applies omega force in order to rotate the game object to the given orientation. The axes at which the rotation should occur (Vector3::UNIT_Y for y, Vector3::UNIT_SCALE for all axes, or just Vector3(1, 1, 0) for x,y axis etc.). The strength at which the rotation should occur. Note: This should be used during simulation instead of @setOmegaVelocity.",
				args = "(Quaternion resultOrientation, Vector3 axes, Ogre::Real strength)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setOmegaVelocity =
			{
				type = "function",
				description = "Set the omega velocity (rotation). Note: This should only be set for initialization and not during simulation, as It could break physics calculation. Use @applyAngularVelocity instead. Or it may be called if its a physics active kinematic body.",
				args = "(Vector3 rotation)",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			getOmega =
			{
				type = "function",
				description = "Gets the global angular velocity vector to the physics body. Note: This should be used in order to rotate a game object savily.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			applyForce =
			{
				type = "method",
				description = "Sets the force to the body.",
				args = "(Vector3 force)",
				returns = "(nil)",
				valuetype = "nil"
			},
			applyRequiredForceForVelocity =
			{
				type = "method",
				description = "Applies the required force for the given velocity. Note: This should be used instead of setVelocity in simulation, for realistic physics behavior.",
				args = "(Vector3 velocity)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getForce =
			{
				type = "function",
				description = "Gets the current force acting on the game object.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setGravity =
			{
				type = "method",
				description = "Sets the acting gravity for the physics body. Default value is Vector3(0, -19.8, 0).",
				args = "(Vector3 gravity)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getGravity =
			{
				type = "function",
				description = "Gets the acting gravity for the physics body.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setGyroscopicTorqueEnabled =
			{
				type = "method",
				description = "Sets whether to enable gyroscopic precision for torque. See: https://www.youtube.com/watch?v=BCVQFoPO5qQ, https://www.youtube.com/watch?v=UlErvZoU7Q0.",
				args = "(boolean enable)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setContactSolvingEnabled =
			{
				type = "method",
				description = "Enables contact solving, so that in a lua script onContactSolving(otherGameObject) will be called, when two bodies are colliding.",
				args = "(boolean enable)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setCollisionDirection =
			{
				type = "method",
				description = "Sets an offset collision direction for the collision hull. The collision hull will be rotated according to the given direction. Default is Vector3(0, 0, 0). Note: For convex hull, this cannot be used.",
				args = "(Vector3 collisionOffsetDirection)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCollisionDirection =
			{
				type = "function",
				description = "Gets the offset collision direction for the collision hull.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setCollisionDirection =
			{
				type = "method",
				description = "Sets an offset to the collision position of the collision hull. Note: This may be necessary when e.g. capsule is used and the origin point of the mesh is not in the middle of geometry. Default is Vector3(0, 0, 0). Note: For convex hull, this cannot be used.",
				args = "(Vector3 collisionOffsetPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCollisionPosition =
			{
				type = "function",
				description = "Gets the offset to the collision position of the collision hull.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setCollisionSize =
			{
				type = "method",
				description = "Sets collision size for a hull. Note: If convex hull is used, this cannot be used.",
				args = "(Vector3 collisionSize)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCollisionSize =
			{
				type = "function",
				description = "Gets collision size.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setConstraintAxis =
			{
				type = "method",
				description = "Sets the constraint axis. The axis in the vector, that is set to '1' will be no more available for movement. E.g. Vector3(0, 0, 1), the physics body will only move on x, y-axis. Note: This can be used e.g. for a Jump 'n' Run game.",
				args = "(Vector3 constraintAxis)",
				returns = "(nil)",
				valuetype = "nil"
			},
			releaseConstraintAxis =
			{
				type = "method",
				description = "Releases the constraint axis, so that the game object is no more clipped by a plane.",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			},
			releaseConstraintAxisPin =
			{
				type = "method",
				description = "Releases the pin of the constraint axis, so that the game object is still clipped on a plane but can rotate freely.",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			},
			getConstraintAxis =
			{
				type = "function",
				description = "Gets the constraint axis.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			applyConstraintDirection =
			{
				type = "method",
				description = "Sets the constraint direction. The physics body can only be rotated on that axis. E.g. Vector3(0, 1, 0) the physics body will always stand up. Note: This is useful when creating a character, that should move and not fall.",
				args = "(Vector3 constraintDirection)",
				returns = "(nil)",
				valuetype = "nil"
			},
			releaseConstraintDirection =
			{
				type = "method",
				description = "Releases the set constraint direction. @See setConstraintDirection(...) for further details.",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			},
			setGravitySourceCategory =
			{
				type = "method",
				description = "Sets the gravity source category. If there is a gravity source game object, gravity in that direction of the source will be calculated, but only work with the nearest object, else the force will mess up. Note: This is good for movement on a planet.",
				args = "(string gravitySourceCategory)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getGravitySourceCategory =
			{
				type = "function",
				description = "Gets the gravity source category.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setContinuousCollision =
			{
				type = "method",
				description = "Sets whether to use continuous collision, if set to true, the collision hull will be slightly resized, so that a fast moving physics body will still collide with other bodies. Note: This comes with a performance impact.",
				args = "(boolean continuousCollision)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getContinuousCollision =
			{
				type = "function",
				description = "Gets whether continuous collision mode is used.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			getContactAhead =
			{
				type = "function",
				description = "Gets a contact data if there was a physics ray contact a head of the physics body. The direction is for the ray direction to shoot and the offset, at which offset position away from the physics component. The length of the ray in meters. If 'drawLine' is set to true, a debug line is shown. In order to remove the line call this function once with this value set to false.",
				args = "(number index,  Vector3 offset, number length, boolean forceDrawLine, string categoryIds)",
				returns = "(ContactData)",
				valuetype = "ContactData"
			},
			getContactAhead2 =
			{
				type = "function",
				description = "Gets a contact data if there was a physics ray contact a head of the physics body. The direction is for the ray direction to shoot and the offset, at which offset position away from the physics component. The length of the ray in meters. If 'drawLine' is set to true, a debug line is shown. In order to remove the line call this function once with this value set to false. Note: This method is slower, as the category names must first be translated in to the int category id.",
				args = "(number index,  Vector3 offset, number length, boolean forceDrawLine, string categoryNames)",
				returns = "(ContactData)",
				valuetype = "ContactData"
			},
			getContactToDirection =
			{
				type = "function",
				description = "Gets a contact data if there was a physics ray contact with a physics body. The direction is for the ray direction to shoot and the offset, at which offset position away from the physics component. The value 'from' is from which distance to shoot the ray and the value 'to' is the length of the ray in meters. If 'forceDrawLine' is set to true, a debug line is shown. In order to remove the line call this function once with this value set to false.",
				args = "(number index, const Vector3 direction, Vector3 offset, number from, number to, boolean forceDrawLine, string categoryIds)",
				returns = "(ContactData)",
				valuetype = "ContactData"
			},
			getContactToDirection2 =
			{
				type = "function",
				description = "Gets a contact data if there was a physics ray contact with a physics body. The direction is for the ray direction to shoot and the offset, at which offset position away from the physics component. The value 'from' is from which distance to shoot the ray and the value 'to' is the length of the ray in meters. If 'forceDrawLine' is set to true, a debug line is shown. In order to remove the line call this function once with this value set to false. Note: This method is slower, as the category names must first be translated in to the int category id.",
				args = "(number index, const Vector3 direction, Vector3 offset, number from, number to, boolean forceDrawLine, string categoryNames)",
				returns = "(ContactData)",
				valuetype = "ContactData"
			},
			getContactBelow =
			{
				type = "function",
				description = "Gets a contact data below the physics body. The offset is used to set the offset position away from the physics component. If 'forceDrawLine' is set to true, a debug line is shown. In order to remove the line call this function once with this value set to false.",
				args = "(number index, Vector3 offset, boolean forceDrawLine, string categoryIds)",
				returns = "(ContactData)",
				valuetype = "ContactData"
			},
			getContactBelow2 =
			{
				type = "function",
				description = "Gets a contact data below the physics body. The offset is used to set the offset position away from the physics component. If 'forceDrawLine' is set to true, a debug line is shown. In order to remove the line call this function once with this value set to false. Note: This method is slower, as the category names must first be translated in to the int category id.",
				args = "(number index, Vector3 offset, boolean forceDrawLine, string categoryNames)",
				returns = "(ContactData)",
				valuetype = "ContactData"
			},
			getContactAbove =
			{
				type = "function",
				description = "Gets a contact data above the physics body. The offset is used to set the offset position away from the physics component. If 'forceDrawLine' is set to true, a debug line is shown. In order to remove the line call this function once with this value set to false.",
				args = "(number index, Vector3 offset, boolean forceDrawLine, string categoryIds)",
				returns = "(ContactData)",
				valuetype = "ContactData"
			},
			getContactAbove2 =
			{
				type = "function",
				description = "Gets a contact data above the physics body. The offset is used to set the offset position away from the physics component. If 'forceDrawLine' is set to true, a debug line is shown. In order to remove the line call this function once with this value set to false. Note: This method is slower, as the category names must first be translated in to the int category id.",
				args = "(number index, Vector3 offset, boolean forceDrawLine, string categoryNames)",
				returns = "(ContactData)",
				valuetype = "ContactData"
			},
			setBounds =
			{
				type = "method",
				description = "Sets the min max bounds, until which this body can be moved.",
				args = "(Vector3 minBounds, Vector3 maxBounds)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setKinematicContactSolvingEnabled =
			{
				type = "method",
				description = "Enables kinematic contact solving, to react at the moment when another game object collided with this kinematic game object. It can also be used with (setCollidable(false)), so that ghost collision can be detected. E.g. onKinematicContact(otherGameObject).",
				args = "(boolean enable)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	PhysicsActiveCompoundComponent =
	{
		type = "class",
		description = "Derived class of PhysicsActiveComponent. It can be used for more complex compound collision hulls for physically active game objects (movable).",
		inherits = "PhysicsActiveComponent",
		childs = 
		{
			setMeshCompoundConfigFile =
			{
				type = "method",
				description = "Sets the collision hull XML file name, which describes the sub-collision hull, that are forming the compound. E.g. torus.xml. Note: Normally only simple collision hull creation is only possible like a cylinder, etc. But when e.g. a torus should be created, its a complex collision hull, because even a convex hull cannot describe the torus, because there would be no hole anymore. But a torus can be formed of several curved pipes. Even a chain could be created that way.",
				args = "(string collisionHullFileXML)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	PhysicsActiveDestructableComponent =
	{
		type = "class",
		description = "Derived class of PhysicsActiveComponent. It can be used to created several random pieces for a collision hull and tied together for desctruction. Note: When this component is used, several peaces of the mesh will be created automatically. This should not be used for complex collisions, as it may crash, or no pieces can be created.",
		inherits = "PhysicsActiveComponent",
		childs = 
		{
			setBreakForce =
			{
				type = "method",
				description = "Sets the break force, that is required to tear this physics body into peaces. Default value is 85.",
				args = "(number breakForce)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setBreakTorque =
			{
				type = "method",
				description = "Sets the break torque, that is required to tear this physics body into peaces. Default value is 85.",
				args = "(number breakTorque)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setDensity =
			{
				type = "method",
				description = "Sets the density of physics body, for more stability. Default value is 60.",
				args = "(number density)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setMotionLess =
			{
				type = "method",
				description = "If the physics body is configured as motionless. It will behave like a normal artifact physics body and cannot be moved.",
				args = "(boolean motionless)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setVelocity =
			{
				type = "method",
				description = "Sets the global linear velocity on the physics body. Note: This should only be used for initzialisation. Use @applyRequiredForceForVelocity in simualtion instead. Or it may be called if its a physics active kinematic body.",
				args = "(Vector3 velocity)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getVelocity =
			{
				type = "function",
				description = "Gets currently acting velocity on the body.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			}
		}
	},
	PhysicsActiveKinematicComponent =
	{
		type = "class",
		description = "Derived class of PhysicsActiveComponent. It is a special kind of physics component. It does not react on forces but just on velocity and collision system.",
		inherits = "PhysicsActiveComponent",
		childs = 
		{
			setCollidable =
			{
				type = "method",
				description = "Sets whether this body is collidable with other physics bodies.",
				args = "(boolean collidable)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCollidable =
			{
				type = "function",
				description = "Gets whether this body is collidable with other phyisics bodies.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setOmegaVelocityRotateTo =
			{
				type = "method",
				description = "Sets the omega velocity to rotate the game object to the given result orientation. The axes at which the rotation should occur (Vector3::UNIT_Y for y, Vector3::UNIT_SCALE for all axes, or just Vector3(1, 1, 0) for x,y axis etc.). The strength at which the rotation should occur.",
				args = "(Quaternion resultOrientation, Vector3 axes, Ogre::Real strength)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	PhysicsActiveVehicleComponent =
	{
		type = "class",
		description = "Derived class of PhysicsActiveVehicleComponent. It can be used to control a vehicle.",
		inherits = "PhysicsActiveComponent",
		childs = 
		{
			getVehicleForce =
			{
				type = "function",
				description = "Gets current vehicle force.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			applyWheelie =
			{
				type = "method",
				description = "Applies a wheelie stunt by putting up the front tires at the given strength.",
				args = "(number strength)",
				returns = "(nil)",
				valuetype = "nil"
			},
			applyDrift =
			{
				type = "method",
				description = "Applies a drift at the given strength (jump) and if left side, at the given steering strength and vice versa. Note: A high force strength is required to put the vehicle in the air, e.g. 50000NM.",
				args = "(boolean left, number strength, number steeringStrength)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	PhysicsArtifactComponent =
	{
		type = "class",
		description = "Derived class of PhysicsActiveComponent. Usage: This Component is used for a non movable maybe complex collision hull, like an whole world, or terra.",
		inherits = "PhysicsComponent",
		childs = 
		{
			setCollisionFaceId =
			{
				type = "method",
				description = "Changes the user defined collision attribute stored with faces of the collision mesh. This function is used to obtain the user data stored in faces of the collision geometry. The application can use this user data to achieve per polygon material behavior in large static collision meshes. By changing the value of this user data the application can achieve modifiable surface behavior with the collision geometry. For example, in a driving game, the surface of a polygon that represents the street can changed from pavement to oily or wet after some collision event occurs.",
				args = "(number id)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	PhysicsBuoyancyComponent =
	{
		type = "class",
		description = "Usage: This Component is used for fluid simulation of physics active components. Requirements: A PhysicsArtifactComponent, which acts as the fluid container.",
		inherits = "PhysicsComponent",
		childs = 
		{
			setWaterToSolidVolumeRatio =
			{
				type = "method",
				description = "Sets the water to solid volume ratio (default is 0.9).",
				args = "(number waterToSolidVolumeRatio)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWaterToSolidVolumeRatio =
			{
				type = "function",
				description = "Gets the water to solid volume ratio (default is 0.9).",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setViscosity =
			{
				type = "method",
				description = "Sets the viscosity for the water (default is 0.995).",
				args = "(number viscosity)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getViscosity =
			{
				type = "function",
				description = "Gets the viscosity for the water (default is 0.995).",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setBuoyancyGravity =
			{
				type = "method",
				description = "Sets the gravity in this area (default value is Vector3(0, -15, 0)).",
				args = "(number speed)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getBuoyancyGravity =
			{
				type = "function",
				description = "Gets the gravity in this area (default value is Vector3(0, -15, 0)).",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setOffsetHeight =
			{
				type = "method",
				description = "Sets the offset height, at which the area begins (default value is 0).",
				args = "(number offsetHeight)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getOffsetHeight =
			{
				type = "function",
				description = "Gets the offset height, at which the area begins (default value is 0).",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			reactOnEnter =
			{
				type = "method",
				description = "Lua closure function gets called in order to react when a game object enters the trigger area.",
				args = "(func closure, visitorGameObject)",
				returns = "(nil)",
				valuetype = "nil"
			},
			reactOnInside =
			{
				type = "method",
				description = "Lua closure function gets called in order to react when a game object is inside the trigger area. Note: This function should only be used if really necessary, because this function gets triggered permanently.",
				args = "(func closure, visitorGameObject)",
				returns = "(nil)",
				valuetype = "nil"
			},
			reactOnVanish =
			{
				type = "method",
				description = "Lua closure function gets called in order to react when a game object leaves the trigger area.",
				args = "(func closure, visitorGameObject)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	PhysicsComponent =
	{
		type = "class",
		description = "Base class for some kind of physics components.",
		inherits = "GameObjectComponent",
		childs = 
		{
			isMovable =
			{
				type = "function",
				description = "Gets whether this physics component is movable.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setDirection =
			{
				type = "method",
				description = "Sets the direction vector of the physics component. Note: local direction vector is NEGATIVE_UNIT_Z by default. Attention: Never ever use this function in an update function for physics, as it will mess up parts of physics like ragdolls etc. Only use it for initialization!",
				args = "(Vector3 direction, Vector3 localDirectionVector)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setOrientation =
			{
				type = "method",
				description = "Sets the orientation of the physics component. Attention: Never ever use this function in an update function for physics, as it will mess up parts of physics like ragdolls etc. Only use it for initialization!",
				args = "(Quaternion orientation)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setOrientation =
			{
				type = "function",
				description = "Gets the orientation of the physics component.",
				args = "()",
				returns = "(Quaternion)",
				valuetype = "Quaternion"
			},
			setPosition =
			{
				type = "method",
				description = "Sets the position of the physics component. Attention: Never ever use this function in an update function for physics, as it will mess up parts of physics like ragdolls etc. Only use it for initialization!",
				args = "(Vector3 position)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setPosition2 =
			{
				type = "method",
				description = "Sets the position of the physics component. Attention: Never ever use this function in an update function for physics, as it will mess up parts of physics like ragdolls etc. Only use it for initialization!",
				args = "(number x, float, y, number z)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPosition =
			{
				type = "function",
				description = "Gets the position of the physics component.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setScale =
			{
				type = "method",
				description = "Sets the scale of the physics component. Note: When scale has changed, the collision hull will be re-created.",
				args = "(Vector3 scale)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getScale =
			{
				type = "function",
				description = "Gets the scale of the physics component.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			translate =
			{
				type = "method",
				description = "Sets relative position of the physics component. Attention: Never ever use this function in an update function for physics, as it will mess up parts of physics like ragdolls etc. Only use it for initialization!",
				args = "(Vector3 relativePosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			rotate =
			{
				type = "method",
				description = "Sets the relative orientation of the physics component. Attention: Never ever use this function in an update function for physics, as it will mess up parts of physics like ragdolls etc. Only use it for initialization!",
				args = "(Quaternion relativeOrientation)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setMass =
			{
				type = "method",
				description = "Sets the mass of the physics component. Default value is: 10. Note: Mass only makes sense for physics active movable components.",
				args = "(Vector3 mass)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setVolume =
			{
				type = "method",
				description = "Sets the volume of the physics component. Note: This can be used for game object, that are influenced by buoyancy.",
				args = "(number volume)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getInitialPosition =
			{
				type = "function",
				description = "Gets the initial position of the physics component (position at the time, the component has been created).",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			getInitialScale =
			{
				type = "function",
				description = "Gets the initial scale of the physics component (scale at the time, the component has been created).",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			getInitialOrientation =
			{
				type = "function",
				description = "Gets the initial orientation of the physics component (orientation at the time, the component has been created).",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setCollidable =
			{
				type = "method",
				description = "Sets whether this physics component is collidable.",
				args = "(boolean collidable)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCollidable =
			{
				type = "function",
				description = "Gets whether this physics component is collidable.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			}
		}
	},
	PhysicsExplosionComponent =
	{
		type = "class",
		description = "Usage: With this component a nice physically explosion effect is created. That is all physics active components, which are located in the critical radius will be thrown away.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Activates the components behaviour, so that explosion will be controller by the explosion timer.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether explosion has been started.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setAffectedCategories =
			{
				type = "method",
				description = "Sets affected categories. Note: This function can be used e.g. to exclude some game object, even if there were at range when the detonation occurred.",
				args = "(string categories)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAffectedCategories =
			{
				type = "function",
				description = "Gets the affected categories.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setExplosionCountDownSec =
			{
				type = "method",
				description = "Sets the explosion timer in seconds. The timer starts to count down, when the component is activated.",
				args = "(number countDown)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getExplosionCountDownSec =
			{
				type = "function",
				description = "Gets the spawn interval in seconds.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setExplosionRadius =
			{
				type = "method",
				description = "Sets the explosion radius in meters.",
				args = "(number radius)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getExplosionRadius =
			{
				type = "function",
				description = "Gets the explosion radius in meters.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setExplosionStrengthN =
			{
				type = "method",
				description = "Sets the explosion strength in newton. Note: The given explosion strength is a maximal value. The far away an affected game object is away from the explosion center the weaker the detonation.",
				args = "(number strengthN)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getExplosionStrengthN =
			{
				type = "function",
				description = "Gets the explosion strength in newton.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	PhysicsMaterialComponent =
	{
		type = "class",
		description = "Usage: With this component physics effects like friction, possibility on reacting when two physically active game objects have collided etc. It glues two game object categories together. Note: Several of this components can be created. Attention: Physics material collision do only work for the following constellations: Category1						Category2							Result PhysicsActiveComponent			PhysicsActiveComponent				Working PhysicsArtifactComponent		PhysicsActiveComponent				Working PhysicsActiveComponent			PhysicsKinematicComponent			Working PhysicsArtifactComponent		PhysicsKinematicComponent			Not Working PhysicsKinematicComponent		PhysicsKinematicComponent			Not Working The two last constellations do not work, because no force is taking place, use PhysicsTriggerComponent, in order to detect collisions of this ones!",
		inherits = "GameObjectComponent",
		childs = 
		{
			setFriction =
			{
				type = "method",
				description = "Overrides the friction for this two material categories. Note: Friction data x value is the static friction and y the kinetic friction. Note: static friction and kinetic friction must be positive values. Kinetic friction must be lower than static friction. It is recommended that static friction and kinetic friction be set to a value lower or equal to 1.0, however because some synthetic materials can have higher than one coefficient of friction, Newton allows for the coefficient of friction to be as high as 2.0.",
				args = "(Vector2 frictionData)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getFriction =
			{
				type = "function",
				description = "Gets the friction for this two material categories. Note: Friction data x value is the static friction and y the kinetic friction.",
				args = "()",
				returns = "(Vector2)",
				valuetype = "Vector2"
			},
			setSoftness =
			{
				type = "method",
				description = "Overrides the softness for this two material categories. Details: A typical value for softness coefficient is 0.15, default value is 0.1. Note: Must be a positive value.",
				args = "(number softness)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSoftness =
			{
				type = "function",
				description = "Gets the softness for this two material categories.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setElasticity =
			{
				type = "method",
				description = "Overrides the default coefficients of restitution (elasticity) for the material interaction between two physics material categories. Note: Must be a positive value. It is recommended that elastic coefficient be set to a value lower or equal to 1.0.",
				args = "(number elasticity)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getElasticity =
			{
				type = "function",
				description = "Gets the elasticity this two material categories.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setSurfaceThickness =
			{
				type = "method",
				description = "Overrides the surface thickness for this two material categories. Details: Set an imaginary thickness between the collision geometry of two colliding bodies. Note: Surfaces thickness can improve the behaviors of rolling objects on flat surfaces.",
				args = "(number surfaceThickness)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSurfaceThickness =
			{
				type = "function",
				description = "Gets the surface thickness for this two material categories.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setCollideable =
			{
				type = "method",
				description = "Overrides the collidable state for this two material categories. Note: False means that those two material groups will not collide against each other.",
				args = "(boolean collideable)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCollideable =
			{
				type = "function",
				description = "Gets the softness for this material pair.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setContactSpeed =
			{
				type = "method",
				description = "Sets the contact speed for a specified contact behavior.",
				args = "(number speed)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getContactSpeed =
			{
				type = "function",
				description = "Gets the contact speed for a specified contact behavior.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setContactDirection =
			{
				type = "method",
				description = "Sets the contact direction for a specified contact behavior.",
				args = "(Vector3 direction)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getContactDirection =
			{
				type = "function",
				description = "Gets the contact direction for a specified contact behavior.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			}
		}
	},
	PhysicsPlayerControllerComponent =
	{
		type = "class",
		description = "Derived class of PhysicsActiveComponent. It can be used to control a player with jump/stairs up going functionality.",
		inherits = "PhysicsActiveComponent",
		childs = 
		{
			move =
			{
				type = "method",
				description = "Moves the the player according to the forward, sidespeed and heading angle in radian",
				args = "(number forwardSpeed, number sideSpeed, Radian headingAngleRad)",
				returns = "(nil)",
				valuetype = "nil"
			},
			toggleCrouch =
			{
				type = "method",
				description = "Toggles whether the player is crouching or not. Note: When crouching, the corresponding animation should be changed.",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			},
			jump =
			{
				type = "method",
				description = "Applies a player jump according to the given Jump speed. Note: developer should take care whether player can jump by using @isOnFloor function.",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			},
			getFrame =
			{
				type = "function",
				description = "Gets the current local orientation.",
				args = "()",
				returns = "(Quaternion)",
				valuetype = "Quaternion"
			},
			setRadius =
			{
				type = "method",
				description = "Sets the player collision hull radius.",
				args = "(number radius)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRadius =
			{
				type = "function",
				description = "Gets the player collision hull radius.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setHeight =
			{
				type = "method",
				description = "Sets the player collision hull height.",
				args = "(number height)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getHeight =
			{
				type = "function",
				description = "Gets the player collision hull height.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setStepHeight =
			{
				type = "method",
				description = "Sets the max. step height, the player is able to walk up.",
				args = "(number stepHeight)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getStepHeight =
			{
				type = "function",
				description = "Gets the max. step height, the player is able to walk up.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getupDirection =
			{
				type = "function",
				description = "Gets the up direction of the player.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			isInFreeFall =
			{
				type = "function",
				description = "Gets whether the player is in free fall.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			isOnFloor =
			{
				type = "function",
				description = "Gets whether the player is on floor.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			isCrouching =
			{
				type = "function",
				description = "Gets whether the player is crouching.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setJumpSpeed =
			{
				type = "method",
				description = "Sets the player jump speed. Default value is 20.",
				args = "(number jumpSpeed)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getJumpSpeed =
			{
				type = "function",
				description = "Gets the player collision jump speed.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getForwardSpeed =
			{
				type = "function",
				description = "Gets the player current forward speed when moving.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getSideSpeed =
			{
				type = "function",
				description = "Gets the player current side speed when moving.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getHeading =
			{
				type = "function",
				description = "Gets the player current heading in degrees value.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	PhysicsRagDollComponent =
	{
		type = "class",
		description = "Usage: This component is used to create physics rag doll. The ragdoll details are specified in a XML file. This component has also several states, so that it can also behave like an usual physics active component. Even a partial rag doll can be created e.g. just using the right arm of the player as rag doll.Requirements: A game object with mesh and animation (*.skeleton) file. Note: This component will only work with Ogre Entity, as Ogre Item has not been created for such complex animation things.",
		inherits = "PhysicsActiveComponent",
		childs = 
		{
			setVelocity =
			{
				type = "method",
				description = "Sets the global linear velocity on the physics body. Note: This should only be used for initzialisation. Use @applyRequiredForceForVelocity in simualtion instead. Or it may be called if its a physics active kinematic body.",
				args = "(Vector3 velocity)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getVelocity =
			{
				type = "function",
				description = "Gets currently acting velocity on the body.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			getPosition =
			{
				type = "function",
				description = "Gets the position of the physics component.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setOrientation =
			{
				type = "method",
				description = "Sets the orientation of the physics component. Attention: Never ever use this function in an update function for physics, as it will mess up parts of physics like ragdolls etc. Only use it for initialization!",
				args = "(Quaternion orientation)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getOrientation =
			{
				type = "function",
				description = "Gets the orientation of the physics component.",
				args = "()",
				returns = "(Quaternion)",
				valuetype = "Quaternion"
			},
			setInitialState =
			{
				type = "method",
				description = "If in ragdoll state, resets all bones ot its initial position and orientation.",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			},
			setAnimationEnabled =
			{
				type = "method",
				description = "Enables animation for the ragdoll. That is, the bones are no more controlled manually, but transform comes from animation state.",
				args = "(boolean enabled)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isAnimationEnabled =
			{
				type = "function",
				description = "Gets whether the ragdoll is being animated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setBoneConfigFile =
			{
				type = "method",
				description = "Sets the bone configuration file. Which describes in XML, how the ragdoll is configure. The file must be placed in the same folder as the mesh and skeleton file. Note: The file can be exchanged at runtime, if a different ragdoll configuration is desire.",
				args = "(string boneConfigFile)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getBoneConfigFile =
			{
				type = "function",
				description = "Gets the currently applied bone config file.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			getRagDataList =
			{
				type = "function",
				description = "Gets List of all configured rag bones.",
				args = "()",
				returns = "(Table[RagBone])",
				valuetype = "Table[RagBone]"
			},
			getRagBone =
			{
				type = "function",
				description = "Gets RagBone from the given name or nil, if it does not exist.",
				args = "(string ragboneName)",
				returns = "(RagBone)",
				valuetype = "RagBone"
			},
			setBoneRotation =
			{
				type = "method",
				description = "Rotates the given RagBone around the given axis by degree amount.",
				args = "(string ragboneName, Vector3 axis, number degree)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	PhysicsTerrainComponent =
	{
		type = "class",
		description = "Derived class of PhysicsActiveComponent. Usage: This Component is used for a non movable maybe complex collision hull, like an whole world, or terra.",
		inherits = "PhysicsComponent",
		childs = 
		{
			setCollisionFaceId =
			{
				type = "method",
				description = "Changes the user defined collision attribute stored with faces of the collision mesh. This function is used to obtain the user data stored in faces of the collision geometry. The application can use this user data to achieve per polygon material behavior in large static collision meshes. By changing the value of this user data the application can achieve modifiable surface behavior with the collision geometry. For example, in a driving game, the surface of a polygon that represents the street can changed from pavement to oily or wet after some collision event occurs.",
				args = "(number id)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	PhysicsTriggerComponent =
	{
		type = "class",
		description = "Usage: This component is used to specify an area, which is triggered when another game object enters the erea. Requirements: A kind of physics component.",
		inherits = "PhysicsComponent",
		childs = 
		{
			reactOnEnter =
			{
				type = "method",
				description = "Lua closure function gets called in order to react when a game object enters the trigger area.",
				args = "(func closure, visitorGameObject)",
				returns = "(nil)",
				valuetype = "nil"
			},
			reactOnInside =
			{
				type = "method",
				description = "Lua closure function gets called in order to react when a game object is inside the trigger area. Note: This function should only be used if really necessary, because this function gets triggered permanently.",
				args = "(func closure, visitorGameObject)",
				returns = "(nil)",
				valuetype = "nil"
			},
			reactOnVanish =
			{
				type = "method",
				description = "Lua closure function gets called in order to react when a game object leaves the trigger area.",
				args = "(func closure, visitorGameObject)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	PickerComponent =
	{
		type = "class",
		description = "Usage: This component can be used to pick a game objects with physics component via mouse and manipulate the transform in a spring manner.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets whether this component should be activated or not.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this component is activated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setTargetId =
			{
				type = "method",
				description = "Sets the target game object id to pick.",
				args = "(string targetId)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTargetId =
			{
				type = "function",
				description = "Gets the target game object id which is currently picked.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setTargetJointId =
			{
				type = "method",
				description = "Sets instead a joint id to get the physics body for dragging. Note: Useful if a ragdoll with joint rag bones is involved.",
				args = "(string targetJointId)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setTargetBody =
			{
				type = "method",
				description = "Sets the physics body pointer for dragging. Note: Useful if a ragdoll with joint rag bones is involved.",
				args = "(Body body)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setOffsetPosition =
			{
				type = "method",
				description = "Sets an offset position at which the source game object should be picked.",
				args = "(Vector3 offsetPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getOffsetPosition =
			{
				type = "function",
				description = "Gets the offset position at which the source game object is picked.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setSpringStrength =
			{
				type = "method",
				description = "Sets the pick strength (spring strength).",
				args = "(number springStrength)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSpringStrength =
			{
				type = "function",
				description = "Gets the pick strength (spring strength).",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setDragAffectDistance =
			{
				type = "method",
				description = "Sets the drag affect distance in meters at which the target game object is affected by the picker.",
				args = "(number dragAffectDistance)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDragAffectDistance =
			{
				type = "function",
				description = "Gets the drag affect distance in meters at which the target game object is affected by the picker.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setDrawLine =
			{
				type = "method",
				description = "Sets whether to draw the spring line.",
				args = "(boolean drawLine)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDrawLine =
			{
				type = "function",
				description = "Gets whether a spring line is drawn.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setMouseButtonPickId =
			{
				type = "method",
				description = "Sets the mouse button, which shall active the mouse picking.",
				args = "(string mouseButtonPickId)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMouseButtonPickId =
			{
				type = "function",
				description = "Gets the mouse button, which shall active the mouse picking.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setJoystickButtonPickId =
			{
				type = "method",
				description = "Sets the joystick button, which shall active the joystick picking.",
				args = "(string joystickButtonPickId)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getJoystickButtonPickId =
			{
				type = "function",
				description = "Gets the joystick button, which shall active the joystick picking.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			reactOnDraggingStart =
			{
				type = "method",
				description = "Sets whether to react at the moment when the dragging has started.",
				args = "(func closure)",
				returns = "(nil)",
				valuetype = "nil"
			},
			reactOnDraggingEnd =
			{
				type = "method",
				description = "Sets whether to react at the moment when the dragging has ended.",
				args = "(func closure)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	PlaneComponent =
	{
		type = "class",
		description = "Usage: This component is used to create a plane.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setWidth =
			{
				type = "method",
				description = "Sets the width of the plane. Note: Plane will be reconstructed, if there is a @PhysicsArtifactComponent, the collision hull will also be re-generated.",
				args = "(number width)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWidth =
			{
				type = "function",
				description = "Gets the width of the plane.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setHeight =
			{
				type = "method",
				description = "Sets the height of the plane. Note: Plane will be reconstructed, if there is a @PhysicsArtifactComponent, the collision hull will also be re-generated.",
				args = "(number height)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getHeight =
			{
				type = "function",
				description = "Gets the height of the plane.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setXSegments =
			{
				type = "method",
				description = "Sets number of x-segments of the plane. Note: Plane will be reconstructed, if there is a @PhysicsArtifactComponent, the collision hull will also be re-generated.",
				args = "(number xSegments)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getXSegments =
			{
				type = "function",
				description = "Gets number of x-segments of the plane.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setYSegments =
			{
				type = "method",
				description = "Sets number of y-segments of the plane. Note: Plane will be reconstructed, if there is a @PhysicsArtifactComponent, the collision hull will also be re-generated.",
				args = "(number ySegments)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getYSegments =
			{
				type = "function",
				description = "Gets number of y-segments of the plane.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setNumTexCoordSets =
			{
				type = "method",
				description = "Sets number of texture coordinate sets of the plane. Note: Plane will be reconstructed, if there is a @PhysicsArtifactComponent, the collision hull will also be re-generated.",
				args = "(number numTexCoordSets)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getNumTexCoordSets =
			{
				type = "function",
				description = "Gets number of texture coordinate sets of the plane.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setUTile =
			{
				type = "method",
				description = "Sets number of u-tiles of the plane. Note: Plane will be reconstructed, if there is a @PhysicsArtifactComponent, the collision hull will also be re-generated.",
				args = "(number uTile)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getUTile =
			{
				type = "function",
				description = "Gets number of u-tiles of the plane.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setUTile =
			{
				type = "method",
				description = "Sets number of v-tiles of the plane. Note: Plane will be reconstructed, if there is a @PhysicsArtifactComponent, the collision hull will also be re-generated.",
				args = "(number vTile)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	PlaneGenerator =
	{
		type = "class",
		description = "Generates a procedural mesh plane."
	},
	PlaneUVModifier =
	{
		type = "class",
		description = "Modifies UV on a procedural plane."
	},
	PlayerContact =
	{
		type = "class",
		description = "PlayerContact class",
		childs = 
		{
			PlayerContact =
			{
				type = "value"
			},
			getPosition =
			{
				type = "function",
				description = "Gets the collision position.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			getNormal =
			{
				type = "function",
				description = "Gets the collision normal.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			getPenetration =
			{
				type = "function",
				description = "Gets the penetration between the two collided game objects.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setResultFriction =
			{
				type = "method",
				description = "Sets the result friction. With that its possible to control how much friction the player will get on the ground.",
				args = "(number resultFriction)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	PlayerControllerClickToPointComponent =
	{
		type = "class",
		description = "Usage: A player controller helper for click to point player movement. Requirements: A kind of physics component must exist.",
		inherits = "PlayerControllerComponent",
		childs = 
		{
			setCategories =
			{
				type = "method",
				description = "Sets categories (may be composed: e.g. ALL or ALL-House+Floor a new click point can be placed via mouse.",
				args = "(string categories)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCategories =
			{
				type = "function",
				description = "Gets categories a click point can be placed via mouse.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			getCategoryIds =
			{
				type = "function",
				description = "Gets category ids a click point can be placed via mouse.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setRange =
			{
				type = "method",
				description = "Sets the maximum range a point can be placed away from the player.",
				args = "(number range)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRange =
			{
				type = "function",
				description = "Gets the maximum range a point can be placed away from the player.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setPathSlot =
			{
				type = "method",
				description = "Sets path slot, a path should be generated in.",
				args = "(number pathSlot)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPathSlot =
			{
				type = "function",
				description = "Gets path slot a path is generated.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getMovingBehavior =
			{
				type = "function",
				description = "Gets moving behavior for direct ai manipulation.",
				args = "()",
				returns = "(MovingBehavior)",
				valuetype = "MovingBehavior"
			}
		}
	},
	PlayerControllerComponent =
	{
		type = "class",
		description = "Requirements: A kind of physics component must exist.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setDefaultDirection =
			{
				type = "method",
				description = "Sets the direction the player is modelled.",
				args = "(Vector3 direction)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAnimationBlender =
			{
				type = "function",
				description = "Gets the used animation blender so that the animations may be manipulated manually.",
				args = "()",
				returns = "(AnimationBlender)",
				valuetype = "AnimationBlender"
			},
			setRotationSpeed =
			{
				type = "method",
				description = "Sets the rotation speed for the player. Valid values are: [5, 15]. Default is 10.",
				args = "(number rotationSpeed)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRotationSpeed =
			{
				type = "function",
				description = "Gets the player rotation speed.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setAnimationSpeed =
			{
				type = "method",
				description = "Sets the animation speed for the player.",
				args = "(number animationSpeed)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAnimationSpeed =
			{
				type = "function",
				description = "Gets the player animation speed.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getNormal =
			{
				type = "function",
				description = "Gets the player ground normal vector.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			getHeight =
			{
				type = "function",
				description = "Gets the player height from the ground. Note: If the player is in air and there is no ground, the height is always 500. This value can be used for checking.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getSlope =
			{
				type = "function",
				description = "Gets the player slope between the player and the ground.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setMoveWeight =
			{
				type = "method",
				description = "Sets the move weight for the player.",
				args = "(number moveWeight)",
				returns = "(nil)",
				valuetype = "nil"
			},
			requestMoveWeight =
			{
				type = "method",
				description = "Requests a move weight for the given owner name, so that the lowest move weight is always present. E.g. when performing a pickup animation, the move weight is set to 0, and only the owner can release the move weight again!",
				args = "(string ownerName, number moveWeight)",
				returns = "(nil)",
				valuetype = "nil"
			},
			releaseMoveWeight =
			{
				type = "method",
				description = "Release the move weight for the given owner name, so that the next higher move weight is present again.",
				args = "(string ownerName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setJumpWeight =
			{
				type = "method",
				description = "Sets the jump weight for the player.",
				args = "(number jumpWeight)",
				returns = "(nil)",
				valuetype = "nil"
			},
			requestJumpWeight =
			{
				type = "method",
				description = "Requests a jump weight for the given owner name, so that the lowest jump weight is always present. E.g. when performing a pickup animation, the jump weight is set to 0, and only the owner can release the jump weight again!",
				args = "(string ownerName, number jumpWeight)",
				returns = "(nil)",
				valuetype = "nil"
			},
			releaseJumpWeight =
			{
				type = "method",
				description = "Release the jump weight for the given owner name, so that the next higher jump weight is present again.",
				args = "(string ownerName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPhysicsComponent =
			{
				type = "function",
				description = "Gets physics active component for direct manipulation.",
				args = "()",
				returns = "(PhysicsActiveComponent)",
				valuetype = "PhysicsActiveComponent"
			},
			getPhysicsRagDollComponent =
			{
				type = "function",
				description = "Gets physics ragdoll component for direct manipulation. Note: Only use this function if the game object has a ragdoll created.",
				args = "()",
				returns = "(PhysicsRagDollComponent)",
				valuetype = "PhysicsRagDollComponent"
			},
			getHitGameObjectBelow =
			{
				type = "function",
				description = "Gets the game object, that has been hit below the player. Note: Always check against nil.",
				args = "()",
				returns = "(GameObject)",
				valuetype = "GameObject"
			},
			getHitGameObjectFront =
			{
				type = "function",
				description = "Gets the game object, that has been hit in front of the player. Note: Always check against nil.",
				args = "()",
				returns = "(GameObject)",
				valuetype = "GameObject"
			},
			reactOnAnimationFinished =
			{
				type = "method",
				description = "Sets whether to react when the given animation has finished.",
				args = "(func closureFunction, boolean oneTime)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	PlayerControllerJumpNRunComponent =
	{
		type = "class",
		description = "Usage: A player controller helper for Jump 'n' Run player movement. Requirements: A kind of physics component must exist.",
		inherits = "PlayerControllerComponent",
		childs = 
		{
			setJumpForce =
			{
				type = "method",
				description = "Sets the jump force for the player.",
				args = "(number jumpForce)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getJumpForce =
			{
				type = "function",
				description = "Gets the jump force.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	PlayerControllerJumpNRunLuaComponent =
	{
		type = "class",
		description = "Usage: A player controller helper for Jump 'n' Run player movement in a lua script. Requirements: A kind of physics component must exist.",
		inherits = "PlayerControllerComponent",
		childs = 
		{
			setStartStateName =
			{
				type = "method",
				description = "Sets start state name in lua script, that should be executed.",
				args = "(string startName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getStartStateName =
			{
				type = "function",
				description = "Gets start state name in lua script, that should be executed.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			getStateMachine =
			{
				type = "function",
				description = "Gets the state machine to switch between states etc. in lua script.",
				args = "()",
				returns = "(LuaStateMachine)",
				valuetype = "LuaStateMachine"
			}
		}
	},
	PointerManager =
	{
		type = "class",
		description = "MyGUI pointer manager singleton class.",
		childs = 
		{
			showMouse =
			{
				type = "method",
				description = "Show the MyGUI mouse cursor or hides the mouse cursor.",
				args = "(boolean show)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	PrismGenerator =
	{
		type = "class",
		description = "Generates a procedural mesh prism."
	},
	ProceduralMultiPath =
	{
		type = "class",
		description = "Generates a procedural multi path."
	},
	ProceduralPath =
	{
		type = "class",
		description = "Generates a procedural path."
	},
	PurePursuitComponent =
	{
		type = "class",
		description = "Usage: To calculate the steering angle of a car moving along waypoints, we can use the Pure Pursuit algorithm. This algorithm is a simple yet effective method for path tracking in autonomous vehicles. The Pure Pursuit algorithm calculates the necessary steering angle to drive the vehicle towards a lookahead point on the path. Requirements: A LuaScriptComponent, because calculateSteeringAngle and/or calculatePitchAngle shall be called in script to get the steering/pitch results.Note: Check the orientation and global mesh direction of the checkpoints, in order to get correct results, if the manually controlled vehicle is driving in the correct direction. If the RaceGoalComponent is used.Since there are a lot of waypoints necessary and setting than manually may be disturbing, hence this function will help. You can put this peace of code in the lua console, which can be opened via ^-key. Adapt the id and set proper string quotes.local thisGameObject = AppStateManager:getGameObjectController():getGameObjectFromId('528669575'); local purePursuitComp = thisGameObject:getPurePursuitComponent(); local nodeGameObjects = AppStateManager:getGameObjectController():getGameObjectsFromComponent('NodeComponent'); purePursuitComp:setWaypointsCount(#nodeGameObjects + 1); for i = 0, #nodeGameObjects do    local gameObject = nodeGameObjects[i];    purePursuitComp:setWaypointId(i, gameObject:getId()); end purePursuitComp:reorderWaypoints(); ",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets whether this component should be activated or not for the pure pursuit calculation.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this component is activated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setManuallyControlled =
			{
				type = "method",
				description = "Sets whether the game object is manually controlled by the player, or moves along the waypoints automatically. This can also be switched at runtime.",
				args = "(boolean manuallyControlled)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getIsManuallyControlled =
			{
				type = "function",
				description = "Gets Sets whether the game object is manually controlled by the player, or moves along the waypoints automatically. This can also be switched at runtime.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setWaypointsCount =
			{
				type = "method",
				description = "Sets the way points count.",
				args = "(number count)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWaypointsCount =
			{
				type = "function",
				description = "Gets the way points count.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setWaypointId =
			{
				type = "method",
				description = "Sets the id of the GameObject with the NodeComponent for the given waypoint index.",
				args = "(number index, string id)",
				returns = "(nil)",
				valuetype = "nil"
			},
			addWaypointId =
			{
				type = "method",
				description = "Adds the id of the GameObject with the NodeComponent.",
				args = "(string id)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWaypointId =
			{
				type = "function",
				description = "Gets the way point id from the given index.",
				args = "(number index)",
				returns = "(string)",
				valuetype = "string"
			},
			setRepeat =
			{
				type = "method",
				description = "Sets whether to repeat the path, when the game object reached the last way point.",
				args = "(boolean repeat)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRepeat =
			{
				type = "function",
				description = "Gets whether to repeat the path, when the game object reached the last way point.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setLookaheadDistance =
			{
				type = "method",
				description = "The lookahead distance parameter determines how far ahead the car should look in meters on the path to calculate the steering angle. Default value is 20 meters.",
				args = "(number lookaheadDistance)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getLookaheadDistance =
			{
				type = "function",
				description = "Gets the distance how far ahead the car is looking in meters on the path to calculate the steering angle. Default value is 20 meters.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setCurvatureThreshold =
			{
				type = "method",
				description = "Sets the threashold at which the motorforce shall be adapted. Default value is 0.1.",
				args = "(number curvatureThreshold)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCurvatureThreshold =
			{
				type = "function",
				description = "Gets the threashold at which the motorforce shall be adapted. Default value is 0.1.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setMaxMotorForce =
			{
				type = "method",
				description = "Sets the Force applied by the motor. Default value is 5000N.",
				args = "(number maxMotorForce)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMaxMotorForce =
			{
				type = "function",
				description = "Gets the Force applied by the motor. Default value is 5000N.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setMotorForceVariance =
			{
				type = "method",
				description = "Sets some random motor force variance (-motorForceVariance, +motorForceVariance) which is added to the max motor force.",
				args = "(number motorForceVariance)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMotorForceVariance =
			{
				type = "function",
				description = "Gets the motor force variance (-motorForceVariance, +motorForceVariance) which is added to the max motor force.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setOvertakingMotorForce =
			{
				type = "method",
				description = "Sets the force if a game object comes to close to another one and shall overtaking it. If set to 0, this behavior is not added. E.g. the player itsself shall not have this behavior.",
				args = "(number overtakingMotorForce)",
				returns = "(nil)",
				valuetype = "nil"
			},
			sgetOvertakingMotorForce =
			{
				type = "function",
				description = "Gets the force if a game object comes to close to another one and shall overtaking it. If 0, this behavior is not added. E.g. the player itsself shall not have this behavior.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setMinMaxSteerAngle =
			{
				type = "method",
				description = "Sets the minimum and maximum steering angle in degree. Valid values are from -80 to 80. Default is -45 to 45.",
				args = "(Vector2 minMaxSteerAngle)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMinMaxSteerAngle =
			{
				type = "function",
				description = "Gets the minimum and maximum steering angle in degree. Valid values are from -80 to 80. Default is -45 to 45.",
				args = "()",
				returns = "(Vector2)",
				valuetype = "Vector2"
			},
			setCheckWaypointY =
			{
				type = "method",
				description = "Sets whether to check also the y coordinate of the game object when approaching to a waypoint. This could be necessary,  if e.g. using a looping, so that the car targets against the waypoint , but may be bad, if e.g. a car jumps via a ramp and the waypoint is below, hence he did not hit the waypoint and will travel back.  This flag can be switched on the fly, e.g. reaching a special waypoint using the getCurrentWaypointIndex function.",
				args = "(boolean checkWaypointY)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCheckWaypointY =
			{
				type = "function",
				description = "Gets whether to check also the y coordinate of the game object when approaching to a waypoint. This could be necessary,  if e.g. using a looping, so that the car targets against the waypoint , but may be bad, if e.g. a car jumps via a ramp and the waypoint is below, hence he did not hit the waypoint and will travel back.  This flag can be switched on the fly, e.g. reaching a special waypoint using the getCurrentWaypointIndex function.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setWaypointVariance =
			{
				type = "method",
				description = "Sets some random waypoint variance radius. This ensures, that if several game objects are moved, there is more intuitive moving chaos involved.",
				args = "(number waypointVariance)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWaypointVariance =
			{
				type = "function",
				description = "Gets the waypoint variance radius. This ensures, that if several game objects are moved, there is more intuitive moving chaos involved.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setVarianceIndividual =
			{
				type = "method",
				description = "Sets whether the variance from attribute shall take place for each waypoint, or for all waypoints the same random variance.",
				args = "(boolean varianceIndividual)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getVarianceIndividual =
			{
				type = "function",
				description = "Gets whether the variance from attribute is taking place for each waypoint, or for all waypoints the same random variance.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setObstacleCategory =
			{
				type = "method",
				description = "Sets one or several categories which may belong to obstacle game objects, which a game object will try to overcome. If empty, not obstacle detection takes place.",
				args = "(string obstacleCategory)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getObstacleCategory =
			{
				type = "function",
				description = "Gets one or several categories which may belong to obstacle game objects, which a game object will try to overcome. If empty, not obstacle detection takes place.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setRammingBehavior =
			{
				type = "method",
				description = "Sets whether a ramming behavior is desired if another game objects comes to close to this one. Note: If ramming and overtaking behavior are both switched on, there will be a random decision, which one is chosed each time.",
				args = "(boolean rammingBehavior)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setRammingBehavior =
			{
				type = "function",
				description = "Gets whether a ramming behavior is desired if another game objects comes to close to this one. Note: If ramming and overtaking behavior are both switched on, there will be a random decision, which one is chosed each time.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setOvertakingBehavior =
			{
				type = "method",
				description = "Sets whether a overtaking behavior is desired if another game objects comes to close to this one. Note: If ramming and overtaking behavior are both switched on, there will be a random decision, which one is chosed each time.",
				args = "(boolean overtakingBehavior)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setOvertakingBehavior =
			{
				type = "function",
				description = "Gets whether a overtaking behavior is desired if another game objects comes to close to this one. Note: If ramming and overtaking behavior are both switched on, there will be a random decision, which one is chosed each time.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			getMotorForce =
			{
				type = "function",
				description = "Gets the calculated motor force, which can be applied.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getSteerAmount =
			{
				type = "function",
				description = "Gets the calculated steer angle amount in degrees, which can be applied.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getPitchAmount =
			{
				type = "function",
				description = "Gets the calculated pitch angle amount in degrees, which can be applied.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getCurrentWaypointIndex =
			{
				type = "function",
				description = "Gets the current waypoint index the game object is approaching, or -1 if there are no waypoints.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getCurrentWaypoint =
			{
				type = "function",
				description = "Gets the current waypoint vector position the game object is approaching, or Vector3(0, 0, 0) if there are no waypoints.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			reorderWaypoints =
			{
				type = "method",
				description = "Reorders the waypoints according to the node game object names. This is necessary, since lua may mess up with keys and the first node may be the last one. But order is important, hence call this function, after you add waypoints in lua!",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	Quaternion =
	{
		type = "class",
		description = "Quaternion class.",
		childs = 
		{
			x =
			{
				type = "value"
			},
			y =
			{
				type = "value"
			},
			z =
			{
				type = "value"
			},
			w =
			{
				type = "value"
			},
			fromRotationMatrix =
			{
				type = "method",
				description = "Constructs a quaternion from rotation matrix3.",
				args = "(Matrix3 matrix)",
				returns = "(nil)",
				valuetype = "nil"
			},
			ToRotationMatrix =
			{
				type = "method",
				description = "Constructs a rotation matrix3 from quaternion.",
				args = "(Matrix3 matrix)",
				returns = "(nil)",
				valuetype = "nil"
			},
			Vector3] toAngleAxisFromRadian =
			{
				type = "function",
				description = "Constructs angle (radian) and axis from quaternion.",
				args = "()",
				returns = "(Table[Radian,)",
				valuetype = "Table[Radian,"
			},
			Vector3] toAngleAxisFromDegree =
			{
				type = "function",
				description = "Constructs angle (degree) and axis from quaternion.",
				args = "()",
				returns = "(Table[Degree,)",
				valuetype = "Table[Degree,"
			},
			fromAngleAxis =
			{
				type = "method",
				description = "Constructs a quaternion from angle axis.",
				args = "(Radian rfAngle, Vector3 rkAxis)",
				returns = "(nil)",
				valuetype = "nil"
			},
			xAxis =
			{
				type = "function",
				description = "Returns the X orthonormal axis defining the quaternion. Same as doing xAxis = Vector3::UNIT_X * this. Also called the local X-axis",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			yAxis =
			{
				type = "function",
				description = "Returns the Y orthonormal axis defining the quaternion. Same as doing yAxis = Vector3::UNIT_Y * this. Also called the local Y-axis",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			zAxis =
			{
				type = "function",
				description = "Returns the Z orthonormal axis defining the quaternion. Same as doing zAxis = Vector3::UNIT_Z * this. Also called the local Z-axis",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			dot =
			{
				type = "function",
				description = "Returns the dot product of the quaternion.",
				args = "(Quaternion other)",
				returns = "(Real)",
				valuetype = "Real"
			},
			norm =
			{
				type = "function",
				description = "Returns the norm value of the quaternion. Note: internally w*w+x*x+y*y+z*z; is done.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			normalise =
			{
				type = "function",
				description = "Normalises this quaternion, and returns the previous length.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			inverse =
			{
				type = "function",
				description = "Returns the inverse quaternion for angle subtract operations. Note: Apply to non-zero quaternion.",
				args = "()",
				returns = "(Quaternion)",
				valuetype = "Quaternion"
			},
			unitInverse =
			{
				type = "function",
				description = "Returns the inverse quaternion for angle subtract operations. Note: Apply to unit-length quaternion. That means quaternion must be normalized! After that this operation is cheaper.",
				args = "()",
				returns = "(Quaternion)",
				valuetype = "Quaternion"
			},
			exp =
			{
				type = "function",
				description = "Returns the exponential value of the quaternion.",
				args = "()",
				returns = "(Quaternion)",
				valuetype = "Quaternion"
			},
			log =
			{
				type = "function",
				description = "Returns the logarithmic value of the quaternion.",
				args = "()",
				returns = "(Quaternion)",
				valuetype = "Quaternion"
			},
			getRoll =
			{
				type = "function",
				description = "Calculate the local roll element of this quaternion. Note: reprojectAxis By default the method returns the 'intuitive' result that is, if you projected the local Y of the quaternion onto the X and Y axes, the angle between them is returned. If set to false though, the result is the actual yaw that will be used to implement the quaternion, which is the shortest possible path to get to the same orientation and  may involve less axial rotation. The co-domain of the returned value is  from -180 to 180 degrees.",
				args = "(boolean reprojectAxis)",
				returns = "(Radian)",
				valuetype = "Radian"
			},
			getPitch =
			{
				type = "function",
				description = "Calculate the local pitch element of this quaternion. Note: reprojectAxis By default the method returns the 'intuitive' result that is, if you projected the local Z of the quaternion onto the X and Y axes, the angle between them is returned. If set to true though, the result is the actual yaw that will be used to implement the quaternion, which is the shortest possible path to get to the same orientation and may involve less axial rotation. The co-domain of the returned value is from -180 to 180 degrees.",
				args = "(boolean reprojectAxis)",
				returns = "(Radian)",
				valuetype = "Radian"
			},
			getYaw =
			{
				type = "function",
				description = "Calculate the local yaw element of this quaternion. Note: reprojectAxis By default the method returns the 'intuitive' result that is, if you projected the local Y of the quaternion onto the X and Z axes, the angle between them is returned. If set to true though, the result is the actual yaw that will be used to implement the quaternion, which is the shortest possible path to get to the same orientation and may involve less axial rotation. The co-domain of the returned value is from -180 to 180 degrees.",
				args = "(boolean reprojectAxis)",
				returns = "(Radian)",
				valuetype = "Radian"
			},
			equals =
			{
				type = "function",
				description = "Equality with tolerance (tolerance is max angle difference).",
				args = "(Quaternion other, Radian tolerance)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			orientationEquals =
			{
				type = "function",
				description = "Compare two quaternions which are assumed to be used as orientations. Note:Both equals() and orientationEquals() measure the exact same thing. One measures the difference by angle, the other by a different, non-linear metric.Attention: slerp (0.75f, A, B) != slerp (0.25f, B, A); therefore be careful if your code relies in the order of the operands. This is specially important in IK animation.Note: Default tolerance is 1e-3",
				args = "(Quaternion other, number tolerance)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			slerp =
			{
				type = "function",
				description = "Performs Spherical linear interpolation between two quaternions, and returns the result.Slerp ( 0.0f, A, B ) = A; Slerp ( 1.0f, A, B ) = B Slerp has the proprieties of performing the interpolation at constant velocity, and being torque-minimal (unless shortestPath=false). However, it's NOT commutative, which means Slerp ( 0.75f, A, B ) != Slerp ( 0.25f, B, A ); therefore be careful if your code relies in the order of the operands. This is specially important in IK animation.",
				args = "()",
				returns = "(Quaternion)",
				valuetype = "Quaternion"
			},
			slerpExtraSpins  =
			{
				type = "function",
				description = "@see Slerp. It adds extra 'spins' (i.e. rotates several times) specified by parameter 'iExtraSpins' while interpolating before arriving to the final values",
				args = "(number fT, Quaternion rkP, Quaternion rkQ, number iExtraSpins)",
				returns = "(Quaternion)",
				valuetype = "Quaternion"
			},
			Quaternion] intermediate  =
			{
				type = "function",
				description = "Setup for spherical quadratic interpolation.",
				args = "(Quaternion rkQ0, Quaternion rkQ1, Quaternion rkQ2)",
				returns = "(Table[Quaternion,)",
				valuetype = "Table[Quaternion,"
			},
			squad =
			{
				type = "function",
				description = "Spherical quadratic interpolation. Note: shortest path is by default false",
				args = "(number fT, Quaternion rkP, Quaternion rkA, Quaternion rkB, Quaternion rkQ, boolean shortestPath)",
				returns = "(Quaternion)",
				valuetype = "Quaternion"
			},
			nlerp =
			{
				type = "function",
				description = "Performs Normalised linear interpolation between two quaternions, and returns the result. nlerp (0.0f, A, B) = A; nlerp (1.0f, A, B) = B; Nlerp is faster than Slerp. Nlerp has the proprieties of being commutative (@see Slerp; commutativity is desired in certain places, like IK animation), and being torque-minimal (unless shortestPath=false). However, it's performing the interpolation at non-constant velocity; sometimes this is desired, sometimes it is not. Having a non-constant velocity can produce a more natural rotation feeling without the need of tweaking the weights; however if your scene relies on the timing of the rotation or assumes it will point at a specific angle at a specific weight value, Slerp is a better choice. Note: shortest path is by default false",
				args = "()",
				returns = "(Quaternion)",
				valuetype = "Quaternion"
			},
			IDENTITY =
			{
				type = "value"
			}
		}
	},
	RaceGoalComponent =
	{
		type = "class",
		description = "Usage: Can be used for a player controlled vehicle against cheating :). That is create several checkpoints with the given race direction, which the player must pass in the correct direction and also counts the laps. Requirements: Checkpoints must possess a PhysicsActiveKinematicComponent. The Checkpoint should be a wall, which is big enough, so that the vehicle will pass the wall in any case (even flying over a ramp :)) Note: Check the global mesh direction carefully for the vehicle and the checkpoints and also the orientation of the checkpoints is important!, if e.g. If the track turns 180 degrees, the checkpoint should also be orientated 180degree.Note: Also check the orientation and global mesh direction of the checkpoints, in order to get correct results, if the manually controlled vehicle is driving in the correct direction.Note: The PurePuresuitComponent should also be used, if current racing positions shall be determined. For that also set the lookahead distance big enough, so that the player also reaches all waypoints, for correct racing position determination.",
		inherits = "GameObjectComponent",
		childs = 
		{
			getCheckpointsCount =
			{
				type = "function",
				description = "Gets the count of set checkpoints on the race track.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getSpeedInKmh =
			{
				type = "function",
				description = "Gets the current speed in kilometers per hour.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getLapsCount =
			{
				type = "function",
				description = "Gets specified laps count.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getRacingPosition =
			{
				type = "function",
				description = "Gets current racing position. If not valid, -1 will be delivered.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getDistanceTraveled =
			{
				type = "function",
				description = "Gets the forward distance traveled on the race. Note: If driving the wrong direction, the distance will be decreased.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getCurrentWaypointIndex =
			{
				type = "function",
				description = "Gets current waypoint index, which can be use to get the whole progress of the lap. Note: if -1, waypoint index is invalid.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getCanDrive =
			{
				type = "function",
				description = "Gets whether the vehicle can drive. E.g. is countdown is used, only if the countdown is over, the vehicle shall drive.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			}
		}
	},
	Radian =
	{
		type = "class",
		description = "Radian class.",
		childs = 
		{
			valueDegrees =
			{
				type = "function",
				description = "Gets the degrees as float from radian.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			valueRadians =
			{
				type = "function",
				description = "Gets the radians as float from radian.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			valueAngleUnits =
			{
				type = "function",
				description = "Gets the angle units as float from radian.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	RagBone =
	{
		type = "class",
		description = "The inner class RagBone represents one physically controlled rag bone.",
		childs = 
		{
			getName =
			{
				type = "function",
				description = "Gets name of this bone, that has been specified in the bone config file.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			getPosition =
			{
				type = "function",
				description = "Gets the position of this rag bone in global space.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setOrientation =
			{
				type = "method",
				description = "Sets the orientation of this rag bone in global space. Attention: Never ever use this function in an update function for physics, as it will mess up parts of physics like ragdolls etc. Only use it for initialization!",
				args = "(Quaternion orientation)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getOrientation =
			{
				type = "function",
				description = "Gets the orientation of this rag bone in global space.",
				args = "()",
				returns = "(Quaternion)",
				valuetype = "Quaternion"
			},
			setInitialState =
			{
				type = "method",
				description = "If in ragdoll state, resets this rag bone ot its initial position and orientation.",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			},
			getOgreBone =
			{
				type = "function",
				description = "Gets the Ogre v1 old bone.",
				args = "()",
				returns = "(OldBone)",
				valuetype = "OldBone"
			},
			getParentRagBone =
			{
				type = "function",
				description = "Gets the parent rag bone or nil, if its the root.",
				args = "()",
				returns = "(RagBone)",
				valuetype = "RagBone"
			},
			getInitialBonePosition =
			{
				type = "function",
				description = "Gets the initial position of this rag bone in global space.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			getInitialBoneOrientation =
			{
				type = "function",
				description = "Gets the initial orientation of this rag bone in global space.",
				args = "()",
				returns = "(Quaternion)",
				valuetype = "Quaternion"
			},
			getPhysicsRagDollComponent =
			{
				type = "function",
				description = "Gets PhysicsRagDollComponent outer class object from this rag bone.",
				args = "()",
				returns = "(PhysicsRagDollComponent)",
				valuetype = "PhysicsRagDollComponent"
			},
			getPose =
			{
				type = "function",
				description = "Gets the pose of this rag bone. Note: This is a vector if not ZERO, that is used to constraint a specific axis, so that the rag bone can not be moved along that axis.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			applyPose =
			{
				type = "method",
				description = "Applies a the pose to this rag bone. Note: This is a vector if not ZERO, that is used to constraint a specific axis, so that the rag bone can not be moved along that axis.",
				args = "(Vector3 pose)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getJointComponent =
			{
				type = "function",
				description = "Gets the base joint component that connects this rag bone with another one for rag doll constraints or nil, if it does not exist.",
				args = "()",
				returns = "(JointComponent)",
				valuetype = "JointComponent"
			},
			getJointHingeComponent =
			{
				type = "function",
				description = "Gets the joint hinge component that connects this rag bone with another one for rag doll constraints or nil, if it does not exist.",
				args = "()",
				returns = "(JointHingeComponent)",
				valuetype = "JointHingeComponent"
			},
			getJointUniversalComponent =
			{
				type = "function",
				description = "Gets the joint universal (double hinge) component that connects this rag bone with another one for rag doll constraints or nil, if it does not exist.",
				args = "()",
				returns = "(JointComponent)",
				valuetype = "JointComponent"
			},
			getJointBallAndSocketComponent =
			{
				type = "function",
				description = "Gets the joint ball and socket component that connects this rag bone with another one for rag doll constraints or nil, if it does not exist.",
				args = "()",
				returns = "(JointComponent)",
				valuetype = "JointComponent"
			},
			getJointHingeActuatorComponent =
			{
				type = "function",
				description = "Gets the joint hinge actuator component that connects this rag bone with another one for rag doll constraints or nil, if it does not exist.",
				args = "()",
				returns = "(JointComponent)",
				valuetype = "JointComponent"
			},
			getJointUniversalActuatorComponent =
			{
				type = "function",
				description = "Gets the joint universal actuator (double hinge actuator) component that connects this rag bone with another one for rag doll constraints or nil, if it does not exist.",
				args = "()",
				returns = "(JointComponent)",
				valuetype = "JointComponent"
			},
			applyRequiredForceForVelocity =
			{
				type = "method",
				description = "Applies internally as many force, to satisfy the given velocity and move the bone by that force.",
				args = "(Vector3 velocity)",
				returns = "(nil)",
				valuetype = "nil"
			},
			applyOmegaForce =
			{
				type = "method",
				description = "Applies omega force in order to rotate the rag bone.",
				args = "(Vector3 omega)",
				returns = "(nil)",
				valuetype = "nil"
			},
			applyOmegaForceRotateTo =
			{
				type = "method",
				description = "Applies omega force in order to rotate the game object to the given orientation. The axes at which the rotation should occur (Vector3::UNIT_Y for y, Vector3::UNIT_SCALE for all axes, or just Vector3(1, 1, 0) for x,y axis etc.). The strength at which the rotation should occur. Note: This should be used during simulation instead of @setOmegaVelocity.",
				args = "(Quaternion resultOrientation, Vector3 axes, Ogre::Real strength)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSize =
			{
				type = "function",
				description = "Gets the size of the bone.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			getJointId =
			{
				type = "function",
				description = "Gets the joint id of the given rag bone for direct attachment.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			getBody =
			{
				type = "function",
				description = "Gets the OgreNewt physics body for direct attachment.",
				args = "()",
				returns = "(Body)",
				valuetype = "Body"
			}
		}
	},
	RandomImageShuffler =
	{
		type = "class",
		description = "Usage:The random image shuffler shuffles through images randomly and allows the user to stop and display the current image. The start and stop function shall be called in lua script or C++.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets whether this component should be activated or not.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this component is activated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			}
		}
	},
	Rect2DComponent =
	{
		type = "class",
		description = "Usage: My usage text.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets whether this component should be activated or not.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this component is activated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			}
		}
	},
	RectangleComponent =
	{
		type = "class",
		description = "Usage: Creates one or several rectangles. It can be used in lua script e.g. for an audio spectrum visualizationIt is much more performant as LinesComponent, because only one manual object with x start- and width/height is created.The difference to LinesComponent is, that each end - position is also the next start position.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setTwoSided =
			{
				type = "method",
				description = "Sets whether to render the rectangle on two sides.",
				args = "(boolean two sided)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTwoSided =
			{
				type = "function",
				description = "Gets whether the rectangle is rendered on two sides.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setRectanglesCount =
			{
				type = "method",
				description = "Sets the rectangles count.",
				args = "(number rectanglesCount)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRectanglesCount =
			{
				type = "function",
				description = "Gets the rectangles count.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setColor1 =
			{
				type = "method",
				description = "Sets color 1 for the rectangle (r, g, b).",
				args = "(number index, Vector3 color)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getColor2 =
			{
				type = "function",
				description = "Gets rectangle color 2.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setColor2 =
			{
				type = "method",
				description = "Sets color 2 for the rectangle (r, g, b).",
				args = "(number index, Vector3 color)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getColor1 =
			{
				type = "function",
				description = "Gets rectangle color 1.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setStartPosition =
			{
				type = "method",
				description = "Sets the start position for the rectangle.",
				args = "(number index, Vector3 startPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getStartPosition =
			{
				type = "function",
				description = "Gets the start position of the rectangle.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setStartOrientation =
			{
				type = "method",
				description = "Sets the start orientation vector in degrees for the rectangle. Note: Orientation is set in the form: (degreeX, degreeY, degreeZ).",
				args = "(number index, Vector3 startOrientation)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getStartOrientation =
			{
				type = "function",
				description = "Gets the start orientation vector in degrees of the rectangle. Orientation is in the form: (degreeX, degreeY, degreeZ)",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setWidth =
			{
				type = "method",
				description = "Sets the width of the rectangle.",
				args = "(number index, number width)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWidth =
			{
				type = "function",
				description = "Gets the width of the rectangle.",
				args = "(number index)",
				returns = "(number)",
				valuetype = "number"
			},
			setHeight =
			{
				type = "method",
				description = "Sets the height of the rectangle.",
				args = "(number index, number height)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getHeight =
			{
				type = "function",
				description = "Gets the height of the rectangle.",
				args = "(number index)",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	RectangleShape =
	{
		type = "class",
		description = "Generates a procedural rectangle shape mesh."
	},
	ReferenceComponent =
	{
		type = "class",
		description = "Usage: This component can be used to give more specific information about the game objects equipment(s). That is, several things can be equipped. Hence give each component a proper name.For example, if the player has a sword, which is an own game object, add for the player an ReferenceComponent with the name 'RightHandSword' and set the target id to the id of the sword game object.In lua e.g.call player : getReferenceComponentFromName('RightHandSword') : getTargetId() to get the currently equipped sword. You can also use another ReferenceComponent e.g. 'Shoes' to store the currently equipped shoes.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setTargetId =
			{
				type = "method",
				description = "Sets the target referenced game object id. If set to 0, nothing is referenced.",
				args = "(string targetId)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTargetId =
			{
				type = "function",
				description = "Gets the target referenced game object id. If set to 0, nothing is referenced.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			getTargetGameObject =
			{
				type = "function",
				description = "Gets the target referenced game object. If it does not exist, nil is delivered.",
				args = "()",
				returns = "(GameObject)",
				valuetype = "GameObject"
			}
		}
	},
	RoundedBoxGenerator =
	{
		type = "class",
		description = "Generates a procedural mesh rounded box."
	},
	RoundedCornerSpline2 =
	{
		type = "class",
		description = "Generates a procedural rounded corner spline2."
	},
	RoundedCornerSpline3 =
	{
		type = "class",
		description = "Generates a procedural rounded corner spline3."
	},
	SceneManager =
	{
		type = "class",
		description = "Main class for all Ogre specifig functions."
	},
	SceneNode =
	{
		type = "class",
		description = "Class for managing scene graph hierarchies.",
		childs = 
		{
			createChildSceneNode =
			{
				type = "function",
				description = "Creates a child scene node of this node.",
				args = "()",
				returns = "(SceneNode)",
				valuetype = "SceneNode"
			},
			attachObject =
			{
				type = "method",
				description = "Attaches a movable object to this node. Object will be rendered.",
				args = "(MovableObject movableObject)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setPosition =
			{
				type = "method",
				description = "Sets the position of this node. Note: When a PhysicsComponent is used, this function has no effect. The function must then be called for the PhysicsComponent.",
				args = "(Vector3 vector)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setPosition2 =
			{
				type = "method",
				description = "Sets the position of this node. Note: When a PhysicsComponent is used, this function has no effect. The function must then be called for the PhysicsComponent.",
				args = "(number x, number y, number z)",
				returns = "(nil)",
				valuetype = "nil"
			},
			translate =
			{
				type = "method",
				description = "Translates the node by the given vector from its current position. Note: When a PhysicsComponent is used, this function has no effect. The function must then be called for the PhysicsComponent.",
				args = "(Vector3 vector)",
				returns = "(nil)",
				valuetype = "nil"
			},
			translate2 =
			{
				type = "method",
				description = "Translates the node by the given vector from its current position. Note: When a PhysicsComponent is used, this function has no effect. The function must then be called for the PhysicsComponent.",
				args = "(number x, number y, number z)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setDirection =
			{
				type = "method",
				description = "Sets the direction of this node. Note: When a PhysicsComponent is used, this function has no effect. The function must then be called for the PhysicsComponent.",
				args = "(Vector3 vector, TransformSpace space)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setDirection2 =
			{
				type = "method",
				description = "Sets the direction of this node. Note: When a PhysicsComponent is used, this function has no effect. The function must then be called for the PhysicsComponent.",
				args = "(number x, number y, number z, TransformSpace space)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setScale =
			{
				type = "method",
				description = "Scales the scene node absolute and all its attached objects.",
				args = "(Vector3 vector)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setScale2 =
			{
				type = "method",
				description = "Scales the scene node absolute and all its attached objects.",
				args = "(number x, number y, number z)",
				returns = "(nil)",
				valuetype = "nil"
			},
			scale =
			{
				type = "method",
				description = "Scales the scene node relative and all its attached objects.",
				args = "(Vector3 vector)",
				returns = "(nil)",
				valuetype = "nil"
			},
			scale2 =
			{
				type = "method",
				description = "Scales the scene node relative and all its attached objects.",
				args = "(number x, number y, number z)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setOrientation =
			{
				type = "method",
				description = "Sets the absolute orientation of this node. Note: When a PhysicsComponent is used, this function has no effect. The function must then be called for the PhysicsComponent.",
				args = "(Quaternion quaternion)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setOrientation2 =
			{
				type = "method",
				description = "Sets the absolute orientation of this node. Note: When a PhysicsComponent is used, this function has no effect. The function must then be called for the PhysicsComponent.",
				args = "(number w, number x, number y, number z)",
				returns = "(nil)",
				valuetype = "nil"
			},
			rotate =
			{
				type = "method",
				description = "Rotates the node relative. Note: When a PhysicsComponent is used, this function has no effect. The function must then be called for the PhysicsComponent.",
				args = "(Quaternion quaternion)",
				returns = "(nil)",
				valuetype = "nil"
			},
			rotate2 =
			{
				type = "method",
				description = "Rotates the node relative. Note: When a PhysicsComponent is used, this function has no effect. The function must then be called for the PhysicsComponent.",
				args = "(number w, number x, number y, number z)",
				returns = "(nil)",
				valuetype = "nil"
			},
			roll =
			{
				type = "method",
				description = "Rolls this node relative. Note: When a PhysicsComponent is used, this function has no effect. The function must then be called for the PhysicsComponent.",
				args = "(Vector vector)",
				returns = "(nil)",
				valuetype = "nil"
			},
			pitch =
			{
				type = "method",
				description = "Pitches this node relative. Note: When a PhysicsComponent is used, this function has no effect. The function must then be called for the PhysicsComponent.",
				args = "(Vector vector)",
				returns = "(nil)",
				valuetype = "nil"
			},
			yaw =
			{
				type = "method",
				description = "Yaws this node relative. Note: When a PhysicsComponent is used, this function has no effect. The function must then be called for the PhysicsComponent.",
				args = "(Vector vector)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPosition =
			{
				type = "function",
				description = "Gets the position of this node.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			getScale =
			{
				type = "function",
				description = "Gets the scale of this node.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			getOrientation =
			{
				type = "function",
				description = "Gets the Orientation of this node.",
				args = "()",
				returns = "(Quaternion)",
				valuetype = "Quaternion"
			},
			getParentSceneNode =
			{
				type = "function",
				description = "Gets the parent scene node. If it does not exist, nil will be delivered.",
				args = "()",
				returns = "(SceneNode)",
				valuetype = "SceneNode"
			},
			convertLocalToWorldPosition =
			{
				type = "function",
				description = "Gets the world position of a point in the Node local space useful for simple transforms that don't require a child Node.",
				args = "(Vector3 localPosition)",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			convertLocalToWorldOrientation =
			{
				type = "function",
				description = "Gets the world orientation of an orientation in the Node local space useful for simple transforms that don't require a child Node.",
				args = "(Quaternion localOrientation)",
				returns = "(Quaternion)",
				valuetype = "Quaternion"
			},
			convertWorldToLocalPosition =
			{
				type = "function",
				description = "Gets the local position of a point in the Node world space.",
				args = "(Vector3 worldPosition)",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			convertWorldToLocalOrientation =
			{
				type = "function",
				description = "Gets the local orientation of a point in the Node world space.",
				args = "(Quaternion worldOrientation)",
				returns = "(Quaternion)",
				valuetype = "Quaternion"
			},
			lookAt =
			{
				type = "method",
				description = "Looks at the given vector. Note: When a PhysicsComponent is used, this function has no effect. The function must then be called for the PhysicsComponent.",
				args = "(Vector3 vector)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	ScriptEventManager =
	{
		type = "class",
		description = "ScriptEventManager can be used to define events in lua. Its possible to register an event and subscribe for that event in any other lua script file. Note: Its also possible to react in C++ on a lua event.",
		childs = 
		{
			registerEvent =
			{
				type = "method",
				description = "Registers a new event name. Note: registered events must be known by all other lua scripts. Hence its a good idea to use the init.lua script for event registration. For example: 'AppStateManager:getScriptEventManager():registerEvent(/DestroyStone/);'",
				args = "(string eventName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			registerEventListener =
			{
				type = "method",
				description = "Registers an event listener, that wants to react on an event. When the events gets trigger somewhere, the callback function in lua is called. For example: 'ScriptEventManager:registerEventListener(EventType.DestroyStone, stone_felsite6_0[/onDestroyStone/]);'",
				args = "(EventType eventType, luabind::object callbackFunction)",
				returns = "(nil)",
				valuetype = "nil"
			},
			queueEvent =
			{
				type = "function",
				description = "Queues an event, that is send out as soon as possible.",
				args = "(EventType eventType, luabind::object eventData)",
				returns = "(boolean)",
				valuetype = "boolean"
			}
		}
	},
	SelectGameObjectsComponent =
	{
		type = "class",
		description = "Usage: This component can be used for more complex game object selection behavior, like selection rectangle, multi selection etc. It must be placed under a main game object.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets whether this component should be activated or not.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this component is activated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setCategories =
			{
				type = "method",
				description = "Sets the categories which shall be selectable. Default is 'All', its also possible to mix categories like: 'Player+Enemy'.",
				args = "(string categories)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCategories =
			{
				type = "function",
				description = "Gets selectable categories.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setUseMultiSelection =
			{
				type = "method",
				description = "Sets whether multiple game objects can be selected.",
				args = "(boolean useMultiSelection)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getUseMultiSelection =
			{
				type = "function",
				description = "Gets whether multiple game objects can be selected.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setUseSelectionRectangle =
			{
				type = "method",
				description = "Sets whether use a selection rectangle for multiple game objects selection.",
				args = "(boolean useSelectionRectangle)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getUseSelectionRectangle =
			{
				type = "function",
				description = "Gets whether use a selection rectangle for multiple game objects selection.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			reactOnGameObjectsSelected =
			{
				type = "method",
				description = "Sets whether to react if one or more game objects are selected.",
				args = "(func closureFunction, table[GameObject])",
				returns = "(nil)",
				valuetype = "nil"
			},
			select =
			{
				type = "method",
				description = "Selects or un-selects the given game object by id.",
				args = "(string gameObjectId, boolean bSelect)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	Shape =
	{
		type = "class",
		description = "Generates a custom procedural shape mesh."
	},
	SimpleSoundComponent =
	{
		type = "class",
		description = "With this component a dolby surround is created and calibrated. It can be used for music, sound and effects like spectrum analysis, doppler effect etc.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setSoundName =
			{
				type = "method",
				description = "Sets the sound name to be played. Note: The sound name should be set without path. It will be searched automatically in the Audio Resource group.",
				args = "(string soundName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSoundName =
			{
				type = "function",
				description = "Gets the sound name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setLoop =
			{
				type = "method",
				description = "Sets whether to loop the sound, if its played completely.",
				args = "(boolean loop)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isLooped =
			{
				type = "function",
				description = "Gets whether this sound is played in loop.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setVolume =
			{
				type = "method",
				description = "Sets the volume (gain) of the sound. Note: 0 means extremely quiet and 100 is maximum.",
				args = "(number volume)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getVolume =
			{
				type = "function",
				description = "Gets the volume (gain) of the sound.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setStream =
			{
				type = "method",
				description = "Sets whether this sound should be streamed instead of fully loaded to buffer. Note: The sound gets streamed during runtime. If set to false, the music gets loaded into a buffer offline.Buffering the music file offline, is more stable for big files. Streaming is useful for small files and it is more performant. But using streaming for bigger files can cause interrupting the music playback and stops it",
				args = "(boolean stream)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isStreamed =
			{
				type = "function",
				description = "Gets whether is sound is streamed instead of fully loaded to buffer.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setRelativeToListener =
			{
				type = "method",
				description = "Sets whether this should should be played relative to the listener. Note: When set to true, the sound may come out of another dolby surround box depending where the camera is located creating nice and maybe creapy sound effects. When set to false, it does not matter where the camera is, the sound will always have the same values.",
				args = "(boolean relativeToListener)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isRelativeToListener =
			{
				type = "function",
				description = "Gets whether is sound is player relative to the listener.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setActivated =
			{
				type = "method",
				description = "Activates the sound. Note: If set to true, the will be played, else the sound stops.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this sound is activated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setFadeInOutTime =
			{
				type = "method",
				description = "Setting fadeInFadeOutTime.x, the sound starts playing while fading in. Setting fadeInFadeOutTime.y fades out, but keeps playing at volume 0, so it can be faded in again.",
				args = "(Vector2 fadeInFadeOutTime)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setInnerOuterConeAngle =
			{
				type = "method",
				description = "Setting innerOuterConeAngle.x sets the inner angle of the sound cone for a directional sound. Valid values are [0 - 360]. Setting innerOuterConeAngle.y sets the outer angle of the sound cone for a directional sound. Valid values are [0 - 360]. Note: Each directional source has three zones: The inner zone as defined by the innerOuterConeAngle.x where the gain is constant and is set by 'setVolume'. The outer zone which is set by innerOuterConeAngle.y and the gain is a linear. Transition between the gain and the outerConeGain, outside of the sound cone. The gain in this zone is set by 'setOuterConeGain'.",
				args = "(Vector2 innerOuterConeAngle)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setOuterConeGain =
			{
				type = "method",
				description = "Sets the gain outside the sound cone of a directional sound. Note: Each directional source has three zones:<ol><li>The inner zone as defined by the innerOuterConeAngle.x where the gain is constant and is set by 'setVolume'. The outer zone which is set by innerOuterConeAngle.y and the gain is a linear transition between the gain and the outerConeGain, outside of the sound cone. The gain in this zone is set by setOuterConeGain. Valid range is [0.0 - 1.0] all other values will be ignored",
				args = "(number outerConeGain)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getOuterConeGain =
			{
				type = "function",
				description = "Gets the outer cone gain.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setPitch =
			{
				type = "method",
				description = "Sets the pitch multiplier. Note: Pitch must always be positive non-zero, all other values will be ignored.",
				args = "(number pitch)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPitch =
			{
				type = "function",
				description = "Gets the pitch multiplier.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setDistanceValues =
			{
				type = "method",
				description = "Sets the variables used in the distance attenuation calculation. Setting distanceValues.x, sets the max distance used in the Inverse Clamped Distance Model. Setting distanceValues.y, sets the rolloff rate for the source. Setting distanceValues.z, sets the reference distance used in attenuation calculations.",
				args = "(Vector3 distanceValues)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setSecondOffset =
			{
				type = "method",
				description = "Sets the offset within the audio stream in seconds. Note: Negative values will be ignored.",
				args = "(number secondOffset)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSecondOffset =
			{
				type = "function",
				description = "Gets the current offset within the audio stream in seconds.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setVelocity =
			{
				type = "method",
				description = "Sets the velocity of the sound.",
				args = "(Vector3 velocity)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getVelocity =
			{
				type = "function",
				description = "Gets the velocity of the sound.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setDirection =
			{
				type = "method",
				description = "Sets the direction of the sound. Note: In the case that this sound is attached to a SceneNode this directions becomes relative to the parent's direction. Default value is NEGATIVE_UNIT_Z.",
				args = "(Vector3 direction)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDirection =
			{
				type = "function",
				description = "Gets the direction of the sound.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			enableSpectrumAnalysis =
			{
				type = "method",
				description = "Enables spectrum analysis.Sets the spectrum processing size. Note: addSoundSpectrumHandler must be used and stream must be set to true and spectrumProcessingSize must always be higher as mSpectrumNumberOfBands and divisible by 2. Default value is 1024. It must not go below 1024Sets the spectrum number of bands. Note: addSoundSpectrumHandler must be used and stream must be set to true. Default value is 20Sets the math window type for more harmonic visualization: rectangle, blackman harris, blackman, hanning, hamming, tukeySets the spectrum preparation type: raw (for own visualization), linear, logarithmicSets the spectrum motion smooth factor (default 0 disabled, max 1)",
				args = "(boolean enable, number processingSize, number numberOfBands, WindowType windowType, SpectrumPreparationType spectrumPreparationType, number smoothFactor)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getVUPointsData =
			{
				type = "function",
				description = "Gets the spectrum VU points data list.",
				args = "()",
				returns = "(Table[number])",
				valuetype = "Table[number]"
			},
			getAmplitudeData =
			{
				type = "function",
				description = "Gets the amplitude data list.",
				args = "()",
				returns = "(Table[number])",
				valuetype = "Table[number]"
			},
			getLevelData =
			{
				type = "function",
				description = "Gets the level data list.",
				args = "()",
				returns = "(Table[number])",
				valuetype = "Table[number]"
			},
			getFrequencyData =
			{
				type = "function",
				description = "Gets the frequency data list.",
				args = "()",
				returns = "(Table[number])",
				valuetype = "Table[number]"
			},
			getPhaseData =
			{
				type = "function",
				description = "Gets the phase list (Which phase in degrees is present at the spectrum processing point in time.",
				args = "()",
				returns = "(Table[number])",
				valuetype = "Table[number]"
			},
			getActualSpectrumSize =
			{
				type = "function",
				description = "Gets the actual spectrum size.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getCurrentSpectrumTimePosSec =
			{
				type = "function",
				description = "Gets the current spectrum time position in seconds when the sound is being played.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getFrequency =
			{
				type = "function",
				description = "Gets the frequency.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			isSpectrumArea =
			{
				type = "function",
				description = "During spectrum analysis, gets whether a specific spectrum area has been recognized when being played.",
				args = "(SpectrumArea spectrumArea)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			isInstrument =
			{
				type = "function",
				description = "During spectrum analysis, gets whether a specific instrument has been recognized when being played.",
				args = "(Instrument instrument)",
				returns = "(boolean)",
				valuetype = "boolean"
			}
		}
	},
	Sound =
	{
		type = "class",
		description = "OgreAL 3D dolby surround sound."
	},
	SpawnComponent =
	{
		type = "class",
		description = "Usage: With this component other game objects are spawned. E.g. creating a cannon that throws gold coins :). Note: If this game object contains a @LuaScriptComponent, its possible to react in script at the moment when a game object is spawned: function onSpawn(spawnedGameObject, originGameObject) or function onVanish(spawnedGameObject, originGameObject)",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Activates the components behaviour, so that the spawning will begin.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether the component behaviour is activated or not.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setIntervalMS =
			{
				type = "method",
				description = "Sets the spawn interval in milliseconds. Details: E.g. an interval of 10000 would spawn a game object each 10 seconds.",
				args = "(number intervalMS)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getIntervalMS =
			{
				type = "function",
				description = "Gets the spawn interval in milliseconds.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setCount =
			{
				type = "method",
				description = "Sets the spawn count. Details: E.g. a count of 100 would spawn 100 game objects of this type. When the count is set to 0, the spawning will never stop.",
				args = "(number count)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCount =
			{
				type = "function",
				description = "Gets the Gets the spawn count.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setLifeTimeMS =
			{
				type = "method",
				description = "Sets the spawned game object life time in milliseconds. When the life time is set to 0, the spawned gameobject will live forever. Details: E.g. a life time of 10000 would let live the spawned game object for 10 seconds.",
				args = "(number lifeTimeMS)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getLifeTimeMS =
			{
				type = "function",
				description = "Gets the spawned game object life time in milliseconds.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setOffsetPosition =
			{
				type = "method",
				description = "Sets a new initial position, at which game objects should be spawned.",
				args = "(Vector3 offsetPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getOffsetPosition =
			{
				type = "function",
				description = "Gets the offset position of the to be spawned game objects.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setOffsetOrientation =
			{
				type = "method",
				description = "Sets an offset orientation at which the spawned game objects will occur relative to the origin game object. Note: The offset orientation to set (degreeX, degreeY, degreeZ).",
				args = "(Quaternion offsetOrientation)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getOffsetOrientation =
			{
				type = "function",
				description = "Gets an offset orientation at which the spawned game objects will occur relative to the origin game object.",
				args = "()",
				returns = "(Quaternion)",
				valuetype = "Quaternion"
			},
			setSpawnAtOrigin =
			{
				type = "method",
				description = "Sets whether the spawn new game objects always at the init position or the current position of the original game object. Note: If spawn at origin is set to false. New game objects will be spawned at the current position of the original game object. Thus complex scenarios are possible! E.g. pin the original active physical game object at a rotating block. When spawning a new game object implement the spawn callback and add for the spawned game object an impulse at the current direction. Spawned game objects will be catapultated in all directions while the original game object is being rotated!",
				args = "(boolean spawnAtOrigin)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isSpawnedAtOrigin =
			{
				type = "function",
				description = "Gets whether the game objects are spawned at initial position and orientation of the original game object.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setSpawnTargetId =
			{
				type = "method",
				description = "Sets spawn target game object id. Note: Its possible to specify another game object which should be spawned  at the position of this game object. E.g. a rotating case shall spawn coins, so the case has the spawn component and as spawn target the coin scene node name.",
				args = "(string spawnTargetId)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSpawnTargetId =
			{
				type = "function",
				description = "Gets the spawn target game object id.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setKeepAliveSpawnedGameObjects =
			{
				type = "method",
				description = "Sets whether keep alive spawned game objects, when the component has ben de-activated and re-activated again.",
				args = "(boolean keepAlive)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	SpectrumArea =
	{
		type = "class",
		description = "SpectrumArea class",
		childs = 
		{
			DEEP_BASS =
			{
				type = "value"
			},
			LOW_BASS =
			{
				type = "value"
			},
			MID_BASS =
			{
				type = "value"
			},
			UPPER_BASS =
			{
				type = "value"
			},
			LOWER_MIDRANGE =
			{
				type = "value"
			},
			MIDDLE_MIDRANGE =
			{
				type = "value"
			},
			UPPER_MIDRANGE =
			{
				type = "value"
			},
			PRESENCE_RANGE =
			{
				type = "value"
			},
			HIGH_END =
			{
				type = "value"
			},
			EXTREMELY_HIGH_END =
			{
				type = "value"
			}
		}
	},
	SpectrumPreparationType =
	{
		type = "class",
		description = "SpectrumPreparationType class",
		childs = 
		{
			RAW =
			{
				type = "value"
			},
			LINEAR =
			{
				type = "value"
			},
			LOGARITHMIC =
			{
				type = "value"
			}
		}
	},
	SpeechBubbleComponent =
	{
		type = "class",
		description = "This component can be used to draw an speech bubble at the game object with an offset. A GameObjectTitle is required and optionally for sound, a SimpleSoundComponent is required.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets whether this component should be activated or not.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this component is activated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setTextColor =
			{
				type = "method",
				description = "Sets the color for the text.",
				args = "(Vector3 color)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTextColor =
			{
				type = "function",
				description = "Gets the text color.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setRunSpeech =
			{
				type = "method",
				description = "Sets whether the speech text shall appear char by char running.",
				args = "(boolean runSpeech)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRunSpeech =
			{
				type = "function",
				description = "Gets whether the speech text shall appear char by char running.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setRunSpeed =
			{
				type = "method",
				description = "Sets the speed of the speech run.",
				args = "(number runSpeed)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRunSpeed =
			{
				type = "function",
				description = "Gets the speed of the speech run.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setCaption =
			{
				type = "method",
				description = "Sets the caption text to be displayed.",
				args = "(string caption)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCaption =
			{
				type = "function",
				description = "Gets the caption text.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setSpeechDuration =
			{
				type = "method",
				description = "Sets the speed duration. That is how long the bubble shall remain in seconds.",
				args = "(number speechDuration)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSpeechDuration =
			{
				type = "function",
				description = "Gets the speed duration. That is how long the bubble shall remain in seconds.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setRunSpeechSound =
			{
				type = "method",
				description = "Sets whether to use a sound if the speech is running char by char.",
				args = "(boolean runSpeechSound)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getRunSpeechSound =
			{
				type = "function",
				description = "Gets whether to use a sound if the speech is running char by char.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			reactOnSpeechDone =
			{
				type = "method",
				description = "Sets whether to react if a speech is done.",
				args = "(func closureFunction)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	SphereGenerator =
	{
		type = "class",
		description = "Generates a procedural mesh sphere."
	},
	SphereUVModifier =
	{
		type = "class",
		description = "Modifies UV on a procedural sphere."
	},
	SpherifyModifier =
	{
		type = "class",
		description = "Modifies a sphere on a procedural mesh."
	},
	SplitScreenComponent =
	{
		type = "class",
		description = "Usage: This component can be placed under the MainGameObject in order to active split screening. For that, it is necessary, to create several camera with a workspace and setting the viewport rect appropriate. E.g. in order to split vertically, set the first rect to (0, 0, 0.5, 1) and de second rect to (0.5, 0, 0.5, 1).",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets whether this component should be activated or not.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this component is activated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			}
		}
	},
	SpotLightComponent =
	{
		type = "class",
		description = "SpotLightComponent class",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets whether this light type is visible.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this light type is visible.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setDiffuseColor =
			{
				type = "method",
				description = "Sets the diffuse color (r, g, b) of the light.",
				args = "(Vector3 diffuseColor)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDiffuseColor =
			{
				type = "function",
				description = "Gets the diffuse color (r, g, b) of the light.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setSpecularColor =
			{
				type = "method",
				description = "Sets the specular color (r, g, b) of the light.",
				args = "(Vector3 specularColor)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSpecularColor =
			{
				type = "function",
				description = "Gets the specular color (r, g, b) of the light.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setPowerScale =
			{
				type = "method",
				description = "Sets the power of the light. E.g. a value of 2 will make the light two times stronger. Default value is: PI",
				args = "(number powerScale)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPowerScale =
			{
				type = "function",
				description = "Gets strength of the light.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setSize =
			{
				type = "method",
				description = "Sets the size (x, y, z) of the spot light. Default value is: Vector3(30, 40, 1)",
				args = "(Vector3 size)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSize =
			{
				type = "function",
				description = "Gets the size (x, y, z) of the spotlight.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setNearClipDistance =
			{
				type = "method",
				description = "Sets near clip distance. Default value is: 0.1",
				args = "(number nearClipDistance)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getNearClipDistance =
			{
				type = "function",
				description = "Gets the near clip distance.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setCastShadows =
			{
				type = "method",
				description = "Sets whether the game objects, that are affected by this light will cast shadows.",
				args = "(boolean castShadows)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCastShadows =
			{
				type = "function",
				description = "Gets whether the game objects, that are affected by this light are casting shadows.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setAttenuationMode =
			{
				type = "method",
				description = "Sets attenuation mode. Possible values are: 'Range', 'Radius'",
				args = "(string mode)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAttenuationMode =
			{
				type = "function",
				description = "Gets the attenuation mode.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setAttenuationRange =
			{
				type = "method",
				description = "Sets attenuation range. Default value is: 23. Note: This will only be applied, if @setAttenuationMode is set to 'Range'.",
				args = "(number range)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAttenuationRange =
			{
				type = "function",
				description = "Gets the attenuation range.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setAttenuationConstant =
			{
				type = "method",
				description = "Sets attenuation constant. Default value is 5. Note: This will only be applied, if @setAttenuationMode is set to 'Range'.",
				args = "(number constant)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAttenuationConstant =
			{
				type = "function",
				description = "Gets the attenuation constant.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setAttenuationLinear =
			{
				type = "method",
				description = "Sets linear attenuation. Default value is 0. Note: This will only be applied, if @setAttenuationMode is set to 'Range'.",
				args = "(number linear)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAttenuationLinear =
			{
				type = "function",
				description = "Gets the linear attenuation constant.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setAttenuationQuadratic =
			{
				type = "method",
				description = "Sets quadric attenuation. Default value is 0.5. Note: This will only be applied, if @setAttenuationMode is set to 'Range'.",
				args = "(number quadric)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAttenuationQuadratic =
			{
				type = "function",
				description = "Gets the quadric attenuation.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setAttenuationRadius =
			{
				type = "method",
				description = "Sets the attenuation radius. Default value is 10. Note: This will only be applied, if @setAttenuationMode is set to 'Radius'.",
				args = "(number attenuationRadius)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAttenuationRadius =
			{
				type = "function",
				description = "Gets the attenuation radius.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setAttenuationLumThreshold =
			{
				type = "method",
				description = "Sets the attenuation lumen threshold. Default value is 0.00192.",
				args = "(number attenuationLumThreshold)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAttenuationLumThreshold =
			{
				type = "function",
				description = "Gets the attenuation lumen threshold.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setShowDummyEntity =
			{
				type = "method",
				description = "Sets whether to show a dummy entity, because normally a light is not visible and its hard to find out, where it actually is, so this function might help for analysis purposes.",
				args = "(boolean showDummyEntity)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getShowDummyEntity =
			{
				type = "function",
				description = "Gets whether dummy entity is shown.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			}
		}
	},
	SpringGenerator =
	{
		type = "class",
		description = "Generates a procedural mesh spring."
	},
	SvgLoader =
	{
		type = "class",
		description = "Generates a procedural from svg file."
	},
	TagChildNodeComponent =
	{
		type = "class",
		description = "This component can be used to add the scene node of another source game object as a child of the scene node of this game object. When this game object is transformed, all its children will also be transformed relative to this parent. A hierarchy can be formed. Info: Several tag child node components can be added to one game object Example: Adding a light as a child to ball. When the ball is moved and rotated the light will also rotateand move relative to the parents transform.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setSourceId =
			{
				type = "method",
				description = "Sets source id for the game object that should be added as a child to this components game object.",
				args = "(string sourceId)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSourceId =
			{
				type = "function",
				description = "Gets the source id for the game object that has been added as a child of this components game object.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			}
		}
	},
	TagPointComponent =
	{
		type = "class",
		description = "This component can be used to attach another source game object to the local tag point (bone). Info: Several tag point components can be added to one game object. Example : A weapon(source game object) could be attached to the right hand of the player. Requirements : Tags a game object to a bone.Requirements: Entity must have animations.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setTagPointName =
			{
				type = "method",
				description = "Sets the tag point name the source game object should be attached to.",
				args = "(string tagName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTagPointName =
			{
				type = "function",
				description = "Gets the current active tag point name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setSourceId =
			{
				type = "method",
				description = "Sets source id for the game object that should be attached to this tag point.",
				args = "(string sourceId)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSourceId =
			{
				type = "function",
				description = "Gets the source id for the game object that is attached to this tag point.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setOffsetPosition =
			{
				type = "method",
				description = "Sets an offset position at which the source game object should be attached.",
				args = "(Vector3 offsetPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getOffsetPosition =
			{
				type = "function",
				description = "Gets the offset position at which the source game object is attached.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setOffsetOrientation =
			{
				type = "method",
				description = "Sets an offset orientation at which the source game object should be attached.",
				args = "(Vector3 offsetPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getOffsetOrientation =
			{
				type = "function",
				description = "Gets the offset orientation at which the source game object is attached.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			}
		}
	},
	TerraComponent =
	{
		type = "class",
		description = "Usage: Enables more detailed rendering customization for the mesh according physically based shading.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setBasePixelDimension =
			{
				type = "method",
				description = "Sets the base pixel dimension. That is: Lower values makes LOD very aggressive. Higher values less aggressive. Must be power of 2.",
				args = "(number index)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getBasePixelDimension =
			{
				type = "function",
				description = "Gets the base pixel dimension. That is: Lower values makes LOD very aggressive. Higher values less aggressive..",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setStrength =
			{
				type = "method",
				description = "Sets the terrain modify strength. Also negative values are possible in order to lower the terrain. Range: [-500; 500].",
				args = "(number strength)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getStrength =
			{
				type = "function",
				description = "Gets the terrain modify strength. Also negative values are possible in order to lower the terrain.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setBrushName =
			{
				type = "method",
				description = "Sets the brush name for modify or paint the terrain, depending which function is called e.g. ",
				args = "(string brushName)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getBrushName =
			{
				type = "function",
				description = "Gets used brush name.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			getAllBrushNames =
			{
				type = "function",
				description = "Gets List of all available brush names for modifying the terrain.",
				args = "()",
				returns = "(table[string])",
				valuetype = "table[string]"
			},
			setBrushSize =
			{
				type = "method",
				description = "Sets brush size. Range: [1; 5000].",
				args = "(number brushSize)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getBrushSize =
			{
				type = "function",
				description = "Gets the brush size.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setBrushIntensity =
			{
				type = "method",
				description = "Sets the brush intensity. Range: [1; 256].",
				args = "(number brushIntensity)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getBrushIntensity =
			{
				type = "function",
				description = "Gets the brush intensity.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setImageLayer =
			{
				type = "method",
				description = "Sets the image layer string for painting.",
				args = "(string imageLayer)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getImageLayer =
			{
				type = "function",
				description = "Gets the used image layer string.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setImageLayerId =
			{
				type = "method",
				description = "Sets the image layer id for painting.",
				args = "(number imageLayerId)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getImageLayerId =
			{
				type = "function",
				description = "Gets the used image layer id.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getAllImageLayer =
			{
				type = "function",
				description = "Gets List of all available image layer names for painting.",
				args = "()",
				returns = "(Table[string])",
				valuetype = "Table[string]"
			},
			modifyTerrainStart =
			{
				type = "method",
				description = "Starts modifying the terrain at the given position and strength. Must be always called once before modifyTerrain is called frequently in order to satisfy undo/redo feature.",
				args = "(Vector3 position, number strength)",
				returns = "(nil)",
				valuetype = "nil"
			},
			smoothTerrainStart =
			{
				type = "method",
				description = "Starts smoothing the terrain at the given position and strength. Must be always called once before msmootTerrain is called frequently in order to satisfy undo/redo feature.",
				args = "(Vector3 position, number strength)",
				returns = "(nil)",
				valuetype = "nil"
			},
			paintTerrainStart =
			{
				type = "method",
				description = "Starts painting the terrain at the given position with the given intensity and the image layer. Must be always called once before paintTerrain is called frequently in order to satisfy undo/redo feature.",
				args = "(Vector3 position, number intensity, number imageLayer)",
				returns = "(nil)",
				valuetype = "nil"
			},
			modifyTerrain =
			{
				type = "method",
				description = "Modifies the terrain at the given position and strength. Note: modifyTerrainStart must be called first one time in order to satisfy undo/redo feature.",
				args = "(Vector3 position, number strength)",
				returns = "(nil)",
				valuetype = "nil"
			},
			smoothTerrain =
			{
				type = "method",
				description = "Smoothes the terrain at the given position and strength. Note: modifyTerrainStart must be called first one time in order to satisfy undo/redo feature.",
				args = "(Vector3 position, number strength)",
				returns = "(nil)",
				valuetype = "nil"
			},
			paintTerrain =
			{
				type = "method",
				description = "Paints the terrain at the given position with the given intensity and the image layer. Note: modifyTerrainStart must be called first one time in order to satisfy undo/redo feature.",
				args = "(Vector3 position, number intensity, number imageLayer)",
				returns = "(nil)",
				valuetype = "nil"
			},
			modifyTerrainEnd =
			{
				type = "method",
				description = "Ends modifying the terrain at the given position and strength. Must be called at last in order to satisfy undo/redo feature.",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			},
			smoothTerrainEnd =
			{
				type = "method",
				description = "Ends smoothing the terrain at the given position and strength.Must be called at last in order to satisfy undo/redo feature.",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			},
			paintTerrainEnd =
			{
				type = "method",
				description = "Ends painting the terrain at the given position with the given intensity and the image layer. Must be called at last in order to satisfy undo/redo feature.",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			},
			modifyTerrainLoop =
			{
				type = "method",
				description = "Abreviation for modifying the terrain at the given position and strength and loop of times. It calles all necessary other methods internally in order to satisfy undo/redo feature",
				args = "(Vector3 position, number strength, number loopCount)",
				returns = "(nil)",
				valuetype = "nil"
			},
			smoothTerrainLoop =
			{
				type = "method",
				description = "Abreviation for smoothing the terrain at the given position and strength and loop of times. It calles all necessary other methods internally in order to satisfy undo/redo feature",
				args = "(Vector3 position, number strength, number loopCount)",
				returns = "(nil)",
				valuetype = "nil"
			},
			paintTerrainLoop =
			{
				type = "method",
				description = "Abreviation for painting the terrain at the given position and strength and loop of times. It calles all necessary other methods internally in order to satisfy undo/redo feature",
				args = "(Vector3 position, number intensity, number imageLayer, number loopCount)",
				returns = "(nil)",
				valuetype = "nil"
			}
		}
	},
	TextureBuffer =
	{
		type = "class",
		description = "Textures a procedural mesh."
	},
	TimeLineComponent =
	{
		type = "class",
		description = "Usage: This component can be used in the main game object to add id's of other game objects, that are activated at a specifig time point and deactivated after a duration of time. The time line propagation starts, as soon as this game object has been connected.Or if a lua script component does exist an its possible to react on an event at the given time point.Example: Creating a film sequence or a space game, in which enemy space ships are spawned at specific time points. Its also possible set a current time and automatically the nearest time point is determined.Requirements: MainGameObject ",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets whether time line can start or not.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this time line is activated or not.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setCurrentTimeSec =
			{
				type = "function",
				description = "Sets the current time in seconds. Note: The next time point is determined, and the corresponding game object or lua function (if existing) called. Note: If the given time exceeds the overwhole time line duration, false is returned.",
				args = "(number timeSec)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			getCurrentTimeSec =
			{
				type = "function",
				description = "Gets the current time in seconds, since this component is running.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getMaxTimeLineDuration =
			{
				type = "function",
				description = "Calculates and gets the maximum time line duration in seconds. Note: Do not call this to often, because the max time is calculated each time from the scratch, in order to be as flexible as possible. E.g. a time point has been removed during runtime.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	TimeTriggerComponent =
	{
		type = "class",
		description = "This component can be used to activate a prior component of this game object after a time and for an duration. Info: Several tag child node components can be added to one game object.Each time trigger component should have prior activate  - able component, or one time trigger component will activate all prior coming components. Example : Activating a movement of physics slider for a specific amount of timeand starting an animation of the slider. Used components : PhysicsActiveComponent->ActiveSliderJointComponent->TimeTriggerComponent->AnimationComponent->TimeTriggerComponent Requirements : A prior component that is activate - able.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets whether time trigger can start or not.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this time trigger is activated or not.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			}
		}
	},
	TorusGenerator =
	{
		type = "class",
		description = "Generates a procedural mesh torus."
	},
	TorusKnotGenerator =
	{
		type = "class",
		description = "Generates a procedural mesh torus knot."
	},
	TransformEaseComponent =
	{
		type = "class",
		description = "Usage: My usage text.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets whether this component should be activated or not.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this component is activated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			}
		}
	},
	TransformHistoryComponent =
	{
		type = "class",
		description = "Usage: This component can be used to linear interpolate a target game object from to the past transform of this game object. The schema is as follows:()Go3<--target--()Go2<--target--()Go1. The game object1 is the root and has the game object2 as target id, the game object 2 can also have this component and have game object 3 as target id, and so one, so Go3 will follow Go2 and Go2 will follow Go1.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setActivated =
			{
				type = "method",
				description = "Sets whether this component should be activated or not.",
				args = "(boolean activated)",
				returns = "(nil)",
				valuetype = "nil"
			},
			isActivated =
			{
				type = "function",
				description = "Gets whether this component is activated.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setTargetId =
			{
				type = "method",
				description = "Sets target id for the game object, which shall be linear interpolated.",
				args = "(string targetId)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTargetId =
			{
				type = "function",
				description = "Gets the target id for the game object, which is linear interpolated.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setHistoryLength =
			{
				type = "method",
				description = "Controls how long the history is and how often a value is saved, setHistoryLength(10) means that a value is saved every 100 ms and therefore 10														values in one second. Calculated history size, this must be large enough even in the case of high latency and thus a high past value, have enough saved values available for interpolation.",
				args = "(number historyLength)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getHistoryLength =
			{
				type = "function",
				description = "Gets the history length.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setPastTime =
			{
				type = "method",
				description = "Sets how much the target game object is transformed in the past of the given game object. Default value is 1000ms.",
				args = "(number pastTimeMS)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getPastTime =
			{
				type = "function",
				description = "Gets how much the target game object is transformed in the past of the given game object. Default value is 1000ms.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setOrientate =
			{
				type = "method",
				description = "Sets whether the target game object also shall be orientated besides movement.",
				args = "(boolean orientate)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getOrientate =
			{
				type = "function",
				description = "Gets whether the target game object also shall be orientated besides movement.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setUseDelay =
			{
				type = "method",
				description = "Sets whether to use delay on transform. That means, the target game object will start transform and replay the path to the source game object, if the source game object has stopped.",
				args = "(boolean useDelay)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getUseDelay =
			{
				type = "function",
				description = "Gets whether delay on transform is used. That means, the target game object will start transform and replay the path to the source game object, if the source game object has stopped.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setStopImmediately =
			{
				type = "method",
				description = "Sets whether the target game object shall stop its interpolation immediately, if this game object stops transform.",
				args = "(boolean stopImmediately)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getStopImmediately =
			{
				type = "function",
				description = "Gets whether the target game object stops its interpolation immediately, if this game object stops transform.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			getTargetGameObject =
			{
				type = "function",
				description = "Gets the target referenced game object. If it does not exist, nil is delivered.",
				args = "()",
				returns = "(GameObject)",
				valuetype = "GameObject"
			}
		}
	},
	TransformSpace =
	{
		type = "class",
		description = "TransformSpace class",
		childs = 
		{
			TS_LOCAL =
			{
				type = "value"
			},
			TS_PARENT =
			{
				type = "value"
			},
			TS_WORLD =
			{
				type = "value"
			}
		}
	},
	TriangleBuffer =
	{
		type = "class",
		description = "Triangulates a procedural mesh."
	},
	TriangleShape =
	{
		type = "class",
		description = "Generates a procedural triangle shape mesh."
	},
	Triangulator =
	{
		type = "class",
		description = "Triangulator manages trianulgation operations on a procedural mesh."
	},
	TubeGenerator =
	{
		type = "class",
		description = "Generates a procedural mesh tube."
	},
	UnweldVerticesModifier =
	{
		type = "class",
		description = "Unwelds vertices on a procedural mesh."
	},
	ValueBarComponent =
	{
		type = "class",
		description = "This component can be used to draw an value bar at the game object with an offset. The current value level can also be manipulated.",
		inherits = "GameObjectComponent",
		childs = 
		{
			setTwoSided =
			{
				type = "method",
				description = "Sets whether to render the value bar on two sides.",
				args = "(boolean two sided)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getTwoSided =
			{
				type = "function",
				description = "Gets whether the value bar is rendered on two sides.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setInnerColor =
			{
				type = "method",
				description = "Sets the inner color for the value bar (r, g, b).",
				args = "(Vector3 color)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getInnerColor =
			{
				type = "function",
				description = "Gets the inner color for the value bar (r, g, b).",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setOuterColor =
			{
				type = "method",
				description = "Sets the outer color for the value bar (r, g, b).",
				args = "(Vector3 color)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getOuterColor =
			{
				type = "function",
				description = "Gets the outer color for the value bar (r, g, b).",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setBorderSize =
			{
				type = "method",
				description = "Sets border size of the outer rectangle of the value bar.",
				args = "(number borderSize)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getBorderSize =
			{
				type = "function",
				description = "Gets border size of the outer rectangle of the value bar.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setOffsetPosition =
			{
				type = "method",
				description = "Sets the offset position for the value bar. Note: Normally the value bar is placed at the same position as its carrying game object. Hence setting an offset is necessary.",
				args = "(Vector3 offsetPosition)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getOffsetPosition =
			{
				type = "function",
				description = "Gets the offset position of the value bar.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setOffsetOrientation =
			{
				type = "method",
				description = "Sets the offset orientation vector in degrees for the value bar. Note: Orientation is set in the form: (degreeX, degreeY, degreeZ).",
				args = "(Vector3 offsetOrientation)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getOffsetOrientation =
			{
				type = "function",
				description = "Gets the offset orientation vector in degrees of the value bar. Orientation is in the form: (degreeX, degreeY, degreeZ)",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			setWidth =
			{
				type = "method",
				description = "Sets the width of the rectangle.",
				args = "(number width)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWidth =
			{
				type = "function",
				description = "Gets the width of the rectangle.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setHeight =
			{
				type = "method",
				description = "Sets the height of the rectangle.",
				args = "(number height)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getHeight =
			{
				type = "function",
				description = "Gets the height of the rectangle.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setMaxValue =
			{
				type = "method",
				description = "Sets the maximum value for the bar.",
				args = "(number maxValue)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMaxValue =
			{
				type = "function",
				description = "Gets the maximum value for the bar.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setCurrentValue =
			{
				type = "method",
				description = "Sets the current absolute value for the bar. Note: The value is clamped if it exceeds the maximum value or is < 0.",
				args = "(number currentValue)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getCurrentValue =
			{
				type = "function",
				description = "Gets the current value for the bar.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	Variant =
	{
		type = "class",
		description = "The class Variant is a common datatype for any attribute key and value.",
		childs = 
		{
			assign =
			{
				type = "method",
				description = "Assings another value to this Variant.",
				args = "(string other)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getType =
			{
				type = "function",
				description = "Gets the type of this variant.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getName =
			{
				type = "function",
				description = "Gets the name of this variant.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			setValueNumber =
			{
				type = "method",
				description = "Sets the number value.",
				args = "(number value)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setValueBool =
			{
				type = "method",
				description = "Sets the bool value.",
				args = "(boolean value)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setValueString =
			{
				type = "method",
				description = "Sets the String value.",
				args = "(string value)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setValueId =
			{
				type = "method",
				description = "Sets the Game Object ID value.",
				args = "(string id)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setValueVector2 =
			{
				type = "method",
				description = "Sets the Vector2 value.",
				args = "(Vector2 value)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setValueVector3 =
			{
				type = "method",
				description = "Sets the Vector3 value.",
				args = "(Vector3 value)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setValueVector4 =
			{
				type = "method",
				description = "Sets the Vector4 value.",
				args = "(Vector4 value)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getValueNumber =
			{
				type = "function",
				description = "Gets the number value.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getValueString =
			{
				type = "function",
				description = "Gets the string value.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			getValueId =
			{
				type = "function",
				description = "Gets the id value.",
				args = "()",
				returns = "(string)",
				valuetype = "string"
			},
			getValueBool =
			{
				type = "function",
				description = "Gets the bool value.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			getValueVector2 =
			{
				type = "function",
				description = "Gets the Vector2 value.",
				args = "()",
				returns = "(Vector2)",
				valuetype = "Vector2"
			},
			getValueVector3 =
			{
				type = "function",
				description = "Gets the Vector3 value.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			getValueVector4 =
			{
				type = "function",
				description = "Gets the Vector4 value.",
				args = "()",
				returns = "(Vector4)",
				valuetype = "Vector4"
			},
			VAR_NULL =
			{
				type = "value"
			},
			VAR_BOOL =
			{
				type = "value"
			},
			VAR_REAL =
			{
				type = "value"
			},
			VAR_INT =
			{
				type = "value"
			},
			VAR_VEC2 =
			{
				type = "value"
			},
			VAR_VEC3 =
			{
				type = "value"
			},
			VAR_VEC4 =
			{
				type = "value"
			},
			VAR_QUAT =
			{
				type = "value"
			},
			VAR_MAT3 =
			{
				type = "value"
			},
			VAR_MAT4 =
			{
				type = "value"
			},
			VAR_STRING =
			{
				type = "value"
			}
		}
	},
	Vector2 =
	{
		type = "class",
		description = "Vector2 class.",
		childs = 
		{
			x =
			{
				type = "value"
			},
			y =
			{
				type = "value"
			},
			crossProduct =
			{
				type = "function",
				description = "Gets the cross product between two vectors.",
				args = "(Vector2 other)",
				returns = "(Vector2)",
				valuetype = "Vector2"
			},
			distance =
			{
				type = "function",
				description = "Gets the distance between two vectors.",
				args = "(Vector2 other)",
				returns = "(number)",
				valuetype = "number"
			},
			dotProduct =
			{
				type = "function",
				description = "Gets the dot product between two vectors.",
				args = "(Vector2 other)",
				returns = "(number)",
				valuetype = "number"
			},
			isZeroLength =
			{
				type = "function",
				description = "Checks whether the length to the other vector is zero.",
				args = "(Vector2 other)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			length =
			{
				type = "function",
				description = "Gets the length between of a vector.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			makeCeil =
			{
				type = "function",
				description = "Rounds up or down the vector.",
				args = "()",
				returns = "(Vector2)",
				valuetype = "Vector2"
			},
			makeFloor =
			{
				type = "function",
				description = "Rounds up the vector.",
				args = "()",
				returns = "(Vector2)",
				valuetype = "Vector2"
			},
			midPoint =
			{
				type = "function",
				description = "Gets the mid point between two vectors.",
				args = "(Vector2 other)",
				returns = "(Vector2)",
				valuetype = "Vector2"
			},
			normalise =
			{
				type = "function",
				description = "Normalizes the vector.",
				args = "()",
				returns = "(Vector2)",
				valuetype = "Vector2"
			},
			normalisedCopy =
			{
				type = "function",
				description = "Normalizes the vector and return as copy.",
				args = "()",
				returns = "(Vector2)",
				valuetype = "Vector2"
			},
			perpendicular =
			{
				type = "function",
				description = "Generates a vector perpendicular to this vector (eg an 'up' vector).",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			randomDeviant =
			{
				type = "function",
				description = "Generates a new random vector which deviates from this vector by a given angle in a random direction.",
				args = "(Radian angle)",
				returns = "(Vector2)",
				valuetype = "Vector2"
			},
			reflect =
			{
				type = "function",
				description = "Calculates a reflection vector to the plane with the given normal.",
				args = "(Vector2 normal)",
				returns = "(Vector2)",
				valuetype = "Vector2"
			},
			squaredDistance =
			{
				type = "function",
				description = "Returns the square of the distance to another vector.",
				args = "(Vector2 other)",
				returns = "(number)",
				valuetype = "number"
			},
			squaredLength =
			{
				type = "function",
				description = "Returns the square of the length(magnitude) of the vector.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			isNaN =
			{
				type = "function",
				description = "Check whether this vector contains valid values.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			angleBetween =
			{
				type = "function",
				description = "Gets the angle between 2 vectors.",
				args = "(Vector2 other)",
				returns = "(Radian)",
				valuetype = "Radian"
			},
			angleTo =
			{
				type = "function",
				description = "Gets the oriented angle between 2 vectors. Note: Vectors do not have to be unit-length but must represent directions. The angle is comprised between 0 and 2 PI.",
				args = "(Vector2 other)",
				returns = "(Radian)",
				valuetype = "Radian"
			},
			ZERO =
			{
				type = "value"
			},
			UNIT_X =
			{
				type = "value"
			},
			UNIT_Y =
			{
				type = "value"
			},
			NEGATIVE_UNIT_X =
			{
				type = "value"
			},
			NEGATIVE_UNIT_Y =
			{
				type = "value"
			},
			UNIT_SCALE =
			{
				type = "value"
			}
		}
	},
	Vector3 =
	{
		type = "class",
		description = "Vector3 class.",
		childs = 
		{
			x =
			{
				type = "value"
			},
			y =
			{
				type = "value"
			},
			z =
			{
				type = "value"
			},
			crossProduct =
			{
				type = "function",
				description = "Gets the cross product between two vectors.",
				args = "(Vector3 other)",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			distance =
			{
				type = "function",
				description = "Gets the distance between two vectors.",
				args = "(Vector3 other)",
				returns = "(number)",
				valuetype = "number"
			},
			dotProduct =
			{
				type = "function",
				description = "Gets the dot product between two vectors.",
				args = "(Vector3 other)",
				returns = "(number)",
				valuetype = "number"
			},
			isZeroLength =
			{
				type = "function",
				description = "Checks whether the length to the other vector is zero.",
				args = "(Vector3 other)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			length =
			{
				type = "function",
				description = "Gets the length between of a vector.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			makeCeil =
			{
				type = "function",
				description = "Rounds up or down the vector.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			makeFloor =
			{
				type = "function",
				description = "Rounds up the vector.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			midPoint =
			{
				type = "function",
				description = "Gets the mid point between two vectors.",
				args = "(Vector3 other)",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			normalise =
			{
				type = "function",
				description = "Normalizes the vector.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			normalisedCopy =
			{
				type = "function",
				description = "Normalizes the vector and return as copy.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			perpendicular =
			{
				type = "function",
				description = "Generates a vector perpendicular to this vector (eg an 'up' vector).",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			randomDeviant =
			{
				type = "function",
				description = "Generates a new random vector which deviates from this vector by a given angle in a random direction.",
				args = "(Radian angle)",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			reflect =
			{
				type = "function",
				description = "Calculates a reflection vector to the plane with the given normal.",
				args = "(Vector3 normal)",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			squaredDistance =
			{
				type = "function",
				description = "Returns the square of the distance to another vector.",
				args = "(Vector3 other)",
				returns = "(number)",
				valuetype = "number"
			},
			squaredLength =
			{
				type = "function",
				description = "Returns the square of the length(magnitude) of the vector.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			isNaN =
			{
				type = "function",
				description = "Check whether this vector contains valid values.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			angleBetween =
			{
				type = "function",
				description = "Gets the angle between 2 vectors.",
				args = "(Vector3 other)",
				returns = "(Radian)",
				valuetype = "Radian"
			},
			getRotationTo =
			{
				type = "function",
				description = "Gets the shortest arc quaternion to rotate this vector to the destination vector.Note: If you call this with a dest vector that is close to the inverse of this vector, we will rotate 180 degrees around the 'fallbackAxis' (if specified, default is ZERO, or a generated axis if not) since in this case ANY axis of rotation is valid.",
				args = "(Vector3 other, Vector3 fallbackAxis)",
				returns = "(Radian)",
				valuetype = "Radian"
			},
			positionCloses =
			{
				type = "function",
				description = "Returns whether this vector is within a positional tolerance of another vector, also take scale of the vectors into account. Note: Default tolerance is 1e-03f",
				args = "(Vector3 other, number tolerance)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			positionEquals =
			{
				type = "function",
				description = "Returns whether this vector is within a positional tolerance of another vector. Note: Default tolerance is 1e-03f",
				args = "(Vector3 other, number tolerance)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			absDotProduct =
			{
				type = "function",
				description = "Calculates the absolute dot (scalar) product of this vector with another.",
				args = "(Vector3 other)",
				returns = "(number)",
				valuetype = "number"
			},
			makeAbs =
			{
				type = "method",
				description = "Causes negative members to become positive.",
				args = "()",
				returns = "(nil)",
				valuetype = "nil"
			},
			directionEquals =
			{
				type = "function",
				description = "Returns whether this vector is within a directional tolerance of another vector. Note: Both vectors should be normalised.",
				args = "(Vector3 other, const Radian tolerance)",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			primaryAxis =
			{
				type = "function",
				description = "Extract the primary (dominant) axis from this direction vector.",
				args = "()",
				returns = "(Vector3)",
				valuetype = "Vector3"
			},
			ZERO =
			{
				type = "value"
			},
			UNIT_X =
			{
				type = "value"
			},
			UNIT_Y =
			{
				type = "value"
			},
			UNIT_Z =
			{
				type = "value"
			},
			NEGATIVE_UNIT_X =
			{
				type = "value"
			},
			NEGATIVE_UNIT_Y =
			{
				type = "value"
			},
			NEGATIVE_UNIT_Z =
			{
				type = "value"
			},
			UNIT_SCALE =
			{
				type = "value"
			}
		}
	},
	Vector4 =
	{
		type = "class",
		description = "Vector4 class.",
		childs = 
		{
			x =
			{
				type = "value"
			},
			y =
			{
				type = "value"
			},
			z =
			{
				type = "value"
			},
			w =
			{
				type = "value"
			},
			dotProduct =
			{
				type = "function",
				description = "Gets the dot product between two vectors.",
				args = "(Vector4 other)",
				returns = "(number)",
				valuetype = "number"
			},
			isNaN =
			{
				type = "function",
				description = "Check whether this vector contains valid values.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			ZERO =
			{
				type = "value"
			}
		}
	},
	VehicleComponent =
	{
		type = "class",
		description = "Usage: With this component, a physically vehicle is created.",
		inherits = "GameObjectComponent"
	},
	VehicleDrivingManipulation =
	{
		type = "class",
		description = "VehicleDrivingManipulation class",
		childs = 
		{
			VehicleDrivingManipulation =
			{
				type = "value"
			},
			setSteerAngle =
			{
				type = "method",
				description = "Sets the steering angle e.g. via input device.",
				args = "(number steerAngle)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getSteerAngle =
			{
				type = "function",
				description = "Gets the steering angle.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setMotorForce =
			{
				type = "method",
				description = "Sets the motor force e.g. via input device.",
				args = "(number motorForce)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMotorForce =
			{
				type = "function",
				description = "Gets the motor force.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setHandBrake =
			{
				type = "method",
				description = "Sets hand brake force e.g. via input device.",
				args = "(number handBrake)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getHandBrake =
			{
				type = "function",
				description = "Gets hand brake force.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setBrake =
			{
				type = "method",
				description = "Sets brake force e.g. via input device.",
				args = "(number brake)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getBrake =
			{
				type = "function",
				description = "Gets brake force.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	WeldVerticesModifier =
	{
		type = "class",
		description = "Welds vertices on a procedural mesh."
	},
	Widget =
	{
		type = "class",
		description = "MyGUI widget class.",
		childs = 
		{
			setPosition =
			{
				type = "method",
				description = "Sets the absolute position of the widget.",
				args = "(number x, number y)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setSize =
			{
				type = "method",
				description = "Sets the absolute size of the widget.",
				args = "(number width, number height)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setCoord =
			{
				type = "method",
				description = "Sets the absolute position and size of the widget.",
				args = "(number x1, number y1, number x2, number y2)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setRealPosition =
			{
				type = "method",
				description = "Sets the relative position of the widget.",
				args = "(number x, number y)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setRealSize =
			{
				type = "method",
				description = "Sets the relative size of the widget.",
				args = "(number width, number height)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setRealCoord =
			{
				type = "method",
				description = "Sets the relative position and size of the widget.",
				args = "(number x1, number y1, number x2, number y2)",
				returns = "(nil)",
				valuetype = "nil"
			},
			setVisible =
			{
				type = "method",
				description = "Sets whether to show the widget or not.",
				args = "(boolean visible)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getVisible =
			{
				type = "function",
				description = "Gets whether the widget is visible or not.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setDepth =
			{
				type = "method",
				description = "Sets the rendering depth (ordering).",
				args = "(number depth)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getDepth =
			{
				type = "function",
				description = "Gets the rendering depth (ordering).",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			setAlign =
			{
				type = "method",
				description = "Sets the align of the widget.",
				args = "(Align align)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getAlign =
			{
				type = "function",
				description = "Gets the align of the widget.",
				args = "()",
				returns = "(Align)",
				valuetype = "Align"
			},
			setColor =
			{
				type = "method",
				description = "Sets the color of the widget.",
				args = "(Vector4 color)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getParent =
			{
				type = "function",
				description = "Gets parent widget of this widget. Note: If this is the root widget and there is no parent, nil will be delivered.",
				args = "()",
				returns = "(Widget)",
				valuetype = "Widget"
			},
			getParentSize =
			{
				type = "function",
				description = "Gets the parent widget size of this widget.",
				args = "()",
				returns = "(IntSize)",
				valuetype = "IntSize"
			},
			getChildCount =
			{
				type = "function",
				description = "Gets the count of children of this widget.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			},
			getChildAt =
			{
				type = "function",
				description = "Gets the child widget of this widget by the given index.",
				args = "(number index)",
				returns = "(Widget)",
				valuetype = "Widget"
			},
			findWidget =
			{
				type = "function",
				description = "Finds a widget by the given name.",
				args = "(string Name)",
				returns = "(Widget)",
				valuetype = "Widget"
			},
			setEnabled =
			{
				type = "method",
				description = "Sets whether to enable the widget or not.",
				args = "(boolean enable)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getEnabled =
			{
				type = "function",
				description = "Gets whether the widget is enabled or not.",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			},
			setWidgetStyle =
			{
				type = "method",
				description = "Sets the widget style.",
				args = "(WidgetStyle align)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getWidgetStyle =
			{
				type = "function",
				description = "Gets the widget style.",
				args = "()",
				returns = "(WidgetStyle)",
				valuetype = "WidgetStyle"
			},
			setProperty =
			{
				type = "method",
				description = "Sets a user property for this widget.",
				args = "(string key, Stirng value)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getItemIndex =
			{
				type = "function",
				description = "Gets the index of the item.",
				args = "()",
				returns = "(number)",
				valuetype = "number"
			}
		}
	},
	WidgetStyle =
	{
		type = "class",
		description = "WidgetStyle class.",
		childs = 
		{
			CHILD =
			{
				type = "value"
			},
			POPUP =
			{
				type = "value"
			},
			OVERLAPPED =
			{
				type = "value"
			}
		}
	},
	Window =
	{
		type = "class",
		description = "MyGUI window class.",
		childs = 
		{
			setMovable =
			{
				type = "method",
				description = "Sets whether the window can be moved.",
				args = "(boolean movable)",
				returns = "(nil)",
				valuetype = "nil"
			},
			getMovable =
			{
				type = "function",
				description = "Gets whether the window can be moved",
				args = "()",
				returns = "(boolean)",
				valuetype = "boolean"
			}
		}
	},
	WindowType =
	{
		type = "class",
		description = "WindowType class",
		childs = 
		{
			RECTANGULAR =
			{
				type = "value"
			},
			BLACKMAN =
			{
				type = "value"
			},
			BLACKMAN_HARRIS =
			{
				type = "value"
			},
			TUKEY =
			{
				type = "value"
			},
			HANNING =
			{
				type = "value"
			},
			HAMMING =
			{
				type = "value"
			},
			BARLETT =
			{
				type = "value"
			}
		}
	},
	log =
	{
		type = "method",
		description = "Logs information.",
		args = "(string message)",
		returns = "(nil)",
		valuetype = "nil"
	}

}