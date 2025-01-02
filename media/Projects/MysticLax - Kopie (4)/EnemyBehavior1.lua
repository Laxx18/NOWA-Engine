module("EnemyBehavior1", package.seeall);


aiLuaComponent = nil;
aiPathFollowComponent2D = nil;
animationComponent = nil;
animationBlender = nil;
movingBehavior = nil;
physicsActiveComponent = nil;
canJump = false;

EnemyBehavior1 = {};

IdleState = { };

IdleState["enter"] = function(gameObject)
	movingBehavior:setBehavior(eBehaviorType.NONE);
	animationBlender:blend(AnimationBlender.ANIM_IDLE_1, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);
end

PatrolState = { };

PatrolState["enter"] = function(gameObject)
	aiLuaComponent = gameObject:getAiLuaComponent();
	-- Still use aiPathFollowComponent2D, because each enemy has its own waypoints, but this script should be generic for certain enemies
	aiPathFollowComponent2D = gameObject:getAiPathFollowComponent2D();
	animationComponent = gameObject:getAnimationComponent();
	animationBlender = animationComponent:getAnimationBlender();
	movingBehavior = aiLuaComponent:getMovingBehavior();
	physicsActiveComponent = gameObject:getPhysicsActiveComponent();
	canJump = true;
	
	animationBlender:registerAnimation(AnimationBlender.ANIM_IDLE_1, "idle-01");
	animationBlender:registerAnimation(AnimationBlender.ANIM_IDLE_2, "idle-02");
	animationBlender:registerAnimation(AnimationBlender.ANIM_IDLE_3, "joke");
	animationBlender:registerAnimation(AnimationBlender.ANIM_WALK_NORTH, "walk-01");
	animationBlender:registerAnimation(AnimationBlender.ANIM_WALK_SOUTH, "walk-01");
	animationBlender:registerAnimation(AnimationBlender.ANIM_WALK_WEST, "walk-01");
	animationBlender:registerAnimation(AnimationBlender.ANIM_WALK_EAST, "walk-01");
	animationBlender:registerAnimation(AnimationBlender.ANIM_JUMP_START, "jump-01");
	animationBlender:registerAnimation(AnimationBlender.ANIM_JUMP_WALK, "jump-0p");
	animationBlender:registerAnimation(AnimationBlender.ANIM_HIGH_JUMP_END, "jump-pose");
	animationBlender:registerAnimation(AnimationBlender.ANIM_JUMP_END, "jump-pose");
	animationBlender:registerAnimation(AnimationBlender.ANIM_FALL, "jump-pose");
	animationBlender:registerAnimation(AnimationBlender.ANIM_RUN, "run-01");
	--animationBlender:registerAnimation(AnimationBlender.ANIM_SNEAK, "Take_damage");
	--animationBlender:registerAnimation(AnimationBlender.ANIM_DUCK, "Land2");
	--animationBlender:registerAnimation(AnimationBlender.ANIM_HALT, "Halt");
	animationBlender:registerAnimation(AnimationBlender.ANIM_ATTACK_1, "attack-01");
	animationBlender:registerAnimation(AnimationBlender.ANIM_ATTACK_2, "attack-02");
	
	animationBlender:init(AnimationBlender.ANIM_RUN, true);
	
	movingBehavior:setBehavior(eBehaviorType.FOLLOW_PATH_2D);
end

PatrolState["execute"] = function(gameObject, dt)
	--log("[Lua]: PatrolState Current Behavior: " .. movingBehavior:getCurrentBehavior() .. " GO: " .. gameObject:getName());
	
	-- show ray
	local centerOffset = gameObject:getCenterOffset();
	local contactData = physicsActiveComponent:getContactAhead(0, centerOffset, 10, false, ALL_CATEGORIES_ID);
	-- hide ray again
	local hitGameObject = contactData:getHitGameObject();
	if (hitGameObject ~= nil and hitGameObject:getName() == "PrehistoricLax_0") then
		aiLuaComponent:getMovingBehavior():setTargetAgentId(hitGameObject:getId());
		aiLuaComponent:getStateMachine():changeState(SeekState);
	end
end

PatrolState["exit"] = function(gameObject)
	
end

----------------------------------------------------------------------------------------------------------------

SeekState = { }

SeekState["enter"] = function(gameObject)
	movingBehavior:setBehavior(eBehaviorType.SEEK_2D);
	canJump = true;
end

SeekState["execute"] = function(gameObject, dt)
	--log("[Lua]: SeekState Current Behavior: " .. movingBehavior:getCurrentBehavior() .. " GO: " .. gameObject:getName() .. " can jump: " .. (canJump and 'true' or 'false'));
	
	-- Here do not work with victims, because else the path is calculated for another victim and the position is checked for another one, so that its maybe more far away and MoveRandomlyState is set
	local distance = gameObject:getPosition():squaredDistance(aiLuaComponent:getMovingBehavior():getTargetAgent():getPosition());
	--log("Distance: " .. distance .. " Victim: " .. aiLuaComponent:getMovingBehavior():getTargetAgent():getName());
	if distance < 2 * 2 then
		--aiLuaComponent:getStateMachine():changeState(PatrolState);
		aiLuaComponent:getStateMachine():changeState(AttackState);
	end
	
	--if (movingBehavior:getIsStuck() == true) then
	--	aiLuaComponent:getStateMachine():changeState(IdleState);
	--end

	local centerOffset = gameObject:getCenterOffset();
	local contactData = physicsActiveComponent:getContactAhead(0, centerOffset, 2, false, ALL_CATEGORIES_ID);
	local hitGameObject = contactData:getHitGameObject();
	if (hitGameObject ~= nil) then
		if (hitGameObject:getName() ~= "PrehistoricLax_0" and canJump == true) then
			physicsActiveComponent:addImpulse(Vector3(0, 2, 0));
			canJump = false;
		else
			local contactData = physicsActiveComponent:getContactBelow(0, Vector3.ZERO, false, ALL_CATEGORIES_ID);
			local hitGameObject = contactData:getHitGameObject();
			
			-- Enemy landed again, may jump again
			if (hitGameObject ~= nil) then
				if (contactData:getHeight() < 0.4) then
					canJump = true;
				end
			end
		end
	end
    --log("Seek " .. gameObject:getName() .. " pos: " .. toString(gameObject:getPosition()) .. " ori: " .. toString(gameObject:getOrientation()));
end

SeekState["exit"] = function(gameObject)
    
end

---------------------------------------------------------------------------------------------------------------

WanderState = { }

WanderState["enter"] = function(gameObject)
	movingBehavior:setBehavior(eBehaviorType.WANDER_2D);
	movingBehavior:setWanderJitter(1000);
end

WanderState["execute"] = function(gameObject, dt)
	--log("[Lua]: Current Behavior: " .. movingBehavior:getCurrentBehavior());
	local direction = physicsActiveComponent:getOrientation() * gameObject:getDefaultDirection();
	local centerOffset = gameObject:getCenterOffset();
	local contactData = physicsActiveComponent:getContactToDirection(0, direction, centerOffset, 0, 10, false, ALL_CATEGORIES_ID);
	local hitGameObject = contactData:getHitGameObject();
	if (hitGameObject ~= nil and hitGameObject:getName() == "PrehistoricLax_0") then
		movingBehavior:setTargetAgentId(hitGameObject:getId());
		aiLuaComponent:getStateMachine():changeState(SeekState);
	end
end

WanderState["exit"] = function(gameObject)
    
end

---------------------------------------------------------------------------------------------------------------

AttackState = { }

AttackState["enter"] = function(gameObject)
	movingBehavior:setBehavior(eBehaviorType.NONE);
end

AttackState["execute"] = function(gameObject, dt)
	--log("[Lua]: Current Behavior: " .. movingBehavior:getCurrentBehavior());
	local distance = gameObject:getPosition():squaredDistance(aiLuaComponent:getMovingBehavior():getTargetAgent():getPosition());
	--log("Distance: " .. distance .. " Victim: " .. aiLuaComponent:getMovingBehavior():getTargetAgent():getName());
	if distance > 1 * 1 then
		aiLuaComponent:getStateMachine():changeState(SeekState);
	end
	
	animationBlender:blendAndContinue(AnimationBlender.ANIM_ATTACK_1);
end

AttackState["exit"] = function(gameObject)
    
end