module("EnemyBehavior1", package.seeall);

enemy = nil;
aiLuaComponent = nil;
aiPathFollowComponent2D = nil;
animationComponent = nil;
animationBlender = nil;
movingBehavior = nil;
physicsActiveComponent = nil;
canJump = false;

EnemyBehavior1 = {};


function jumpHandling()
    local centerOffset = enemy:getCenterOffset();
	local contactData = physicsActiveComponent:getContactAhead(13, centerOffset, 2, false, ALL_CATEGORIES_ID);
	local hitGameObject = contactData:getHitGameObject();
	if (hitGameObject ~= nil) then
        --log("[Lua]: --->getCategory: " .. hitGameObject:getCategory());
		if ((hitGameObject:getCategory() == "Default" or hitGameObject:getCategory() == "Platform") and canJump == true) then
			physicsActiveComponent:addImpulse(Vector3(0, 3, 0));
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
end

function checkForSeek()
   -- show ray
	local centerOffset = enemy:getCenterOffset();
	local contactData = physicsActiveComponent:getContactAhead2(12, centerOffset, 10, true, "Character");
	-- hide ray again
	local hitGameObject = contactData:getHitGameObject();
	if (hitGameObject ~= nil and hitGameObject:getTagName() == "Player") then
        log("[Lua]: Goto Seekstate");
		aiLuaComponent:getMovingBehavior():setTargetAgentId(hitGameObject:getId());
		aiLuaComponent:changeState(SeekState);
	end 
end

EnemyBehavior1["connect"] = function(gameObject)
    enemy = gameObject;
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
    animationBlender:registerAnimation(AnimationBlender.ANIM_DEAD_1, "death-01");
    
	
	animationBlender:init1(AnimationBlender.ANIM_RUN, true);
    
    if (EventType.EnemyIdleEvent ~= nil) then
		AppStateManager:getScriptEventManager():registerEventListener(EventType.EnemyIdleEvent, EnemyBehavior1["onEnemyIdle"]);
	end
    if (EventType.EnemyDeadEvent ~= nil) then
		AppStateManager:getScriptEventManager():registerEventListener(EventType.EnemyDeadEvent, EnemyBehavior1["onEnemyDead"]);
	end
end

EnemyBehavior1["onEnemyIdle"] = function(eventData)
	-- Change to idle state, so that enemy will not attack dead player
    animationComponent:setSpeed(0.5);
	aiLuaComponent:changeState(IdleState);
end

EnemyBehavior1["onEnemyDead"] = function(eventData)
	-- Change to idle state, so that enemy will not attack dead player
    animationComponent:setSpeed(0.5);
    
    local id = eventData["enemyId"];
    if (id == enemy:getId()) then
        --log("###onEnemyDead: " .. id);
        aiLuaComponent:changeState(DeadState);
    end
end

IdleState = { };

IdleState["enter"] = function(gameObject)
    --log("[Lua]: IdleState Current Behavior: " .. movingBehavior:getCurrentBehavior() .. " GO: " .. gameObject:getName());
    animationComponent:setSpeed(0.5);
	movingBehavior:setBehavior(MovingBehavior.STOP);
	animationBlender:blend5(AnimationBlender.ANIM_IDLE_3, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);
end

PatrolState = { };

PatrolState["enter"] = function(gameObject)
    animationComponent:setSpeed(0.5);
    physicsActiveComponent:setSpeed(3);
	movingBehavior:setBehavior(MovingBehavior.FOLLOW_PATH_2D);
    animationBlender:blend5(AnimationBlender.ANIM_RUN, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);
end

PatrolState["execute"] = function(gameObject, dt)
	--log("[Lua]: PatrolState Current Behavior: " .. movingBehavior:getCurrentBehavior() .. " GO: " .. gameObject:getName());
	
	checkForSeek();
    
    jumpHandling();

end

PatrolState["exit"] = function(gameObject)
	
end

DeadState = { };

DeadState["enter"] = function(gameObject)
    animationComponent:setSpeed(0.5);
	movingBehavior:setBehavior(MovingBehavior.STOP);
	animationBlender:blend5(AnimationBlender.ANIM_DEAD_1, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);
    
    animationComponent:reactOnAnimationFinished(function()
            log("------>Delete enemy: " .. enemy:getId());
            AppStateManager:getGameObjectController():deleteGameObject(enemy:getId());
        end, true);
end

DeadState["execute"] = function(gameObject, dt)
    
end

DeadState["exit"] = function(gameObject)
	
end

----------------------------------------------------------------------------------------------------------------

SeekState = { }

SeekState["enter"] = function(gameObject)
    log("[Lua]: SeekState Current Behavior: " .. movingBehavior:getCurrentBehavior() .. " GO: " .. gameObject:getName());
    animationComponent:setSpeed(0.5);
    physicsActiveComponent:setSpeed(3);
	movingBehavior:setBehavior(MovingBehavior.PURSUIT_2D);
    --movingBehavior:setBehavior(MovingBehavior.FOLLOW_TRACE_2D);
    animationBlender:blend5(AnimationBlender.ANIM_RUN, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);
	canJump = true;
end

SeekState["execute"] = function(gameObject, dt)
	--log("[Lua]: SeekState Current Behavior: " .. movingBehavior:getCurrentBehavior() .. " GO: " .. gameObject:getName() .. " can jump: " .. (canJump and 'true' or 'false'));
	
	-- Here do not work with victims, because else the path is calculated for another victim and the position is checked for another one, so that its maybe more far away and MoveRandomlyState is set
	local distance = gameObject:getPosition():squaredDistance(aiLuaComponent:getMovingBehavior():getTargetAgent():getPosition());
	--log("Distance: " .. distance .. " Victim: " .. aiLuaComponent:getMovingBehavior():getTargetAgent():getName());
	if distance < 2 * 2 then
		aiLuaComponent:changeState(AttackState);
	end
    
    -- Checks if the player is above the enemy and if the player is on a platform (ray shoot above is not the player)
    -- If this is the case enemy loses desire on the player
    --if (aiLuaComponent:getMovingBehavior():getTargetAgent():getPosition().y > gameObject:getPosition().y) then
    --    local centerOffset = gameObject:getCenterOffset();
    --    local contactData = physicsActiveComponent:getContactAbove(0, Vector3.ZERO, false, ALL_CATEGORIES_ID);
        -- hide ray again
    --    local hitGameObject = contactData:getHitGameObject();
    --    if (hitGameObject ~= nil and hitGameObject:getTagName() ~= "Player") then
    --        aiLuaComponent:changeState(WanderState);
    --    end
    --end
	
	--if (movingBehavior:getIsStuck() == true) then
	--	aiLuaComponent:changeState(IdleState);
	--end

	
    --log("Seek " .. gameObject:getName() .. " pos: " .. toString(gameObject:getPosition()) .. " ori: " .. toString(gameObject:getOrientation()));
    
    jumpHandling();
end

SeekState["exit"] = function(gameObject)
    
end

---------------------------------------------------------------------------------------------------------------

WanderState = { }

WanderState["enter"] = function(gameObject)
    animationComponent:setSpeed(0.5);
    physicsActiveComponent:setSpeed(3);
	movingBehavior:setBehavior(MovingBehavior.WANDER_2D);
	movingBehavior:setWanderJitter(100);
    
    animationBlender:blend5(AnimationBlender.ANIM_RUN, AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true);
end

WanderState["execute"] = function(gameObject, dt)
	checkForSeek();
    
    jumpHandling();
end

WanderState["exit"] = function(gameObject)
    
end

---------------------------------------------------------------------------------------------------------------

AttackState = { }

AttackState["enter"] = function(gameObject)
    log("[Lua]: AttackState Current Behavior: " .. movingBehavior:getCurrentBehavior() .. " GO: " .. gameObject:getName());
    animationComponent:setSpeed(0.2);
    physicsActiveComponent:setSpeed(1);
    
    -- Difference between NONE and STOP. 
    -- If none is on, add no force, so that other behaviors can still move the agent! Only stop adds force, even if its null
	movingBehavior:setBehavior(MovingBehavior.SEEK_2D);

    --animationBlender:blendExclusive1(AnimationBlender.ANIM_ATTACK_1, AnimationBlender.BLEND_WHILE_ANIMATING);
end

AttackState["execute"] = function(gameObject, dt)
        --log("[Lua]: Current Behavior: " .. movingBehavior:getCurrentBehavior());
	local distance = gameObject:getPosition():squaredDistance(aiLuaComponent:getMovingBehavior():getTargetAgent():getPosition());
	--log("Distance: " .. distance .. " Victim: " .. aiLuaComponent:getMovingBehavior():getTargetAgent():getName());
	if distance > 1.7 * 1.7 and animationBlender:isComplete() then
		aiLuaComponent:changeState(SeekState);
	end

    if (animationBlender:isAnimationActive(AnimationBlender.ANIM_ATTACK_1) == false and animationBlender:isComplete()) then
        animationBlender:blend5(AnimationBlender.ANIM_ATTACK_1, AnimationBlender.BLEND_WHILE_ANIMATING, 0.5, true);
    end
    
    jumpHandling();
end

AttackState["exit"] = function(gameObject)
    
end