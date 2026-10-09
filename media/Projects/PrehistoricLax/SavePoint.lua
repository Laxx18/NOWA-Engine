module("Savepoint", package.seeall);
-- Scene: Level5

require("init");

local savepoint = nil;
local mainGameObject = nil;

-- Without a pause the trigger would save again and again while the player stands in it or jitters on
-- its border. It also starts non zero on purpose: after a death the save game is loaded and the player
-- stands INSIDE this trigger again, that must not write the save a second time right away.
local cooldown = 0;
local SAVE_COOLDOWN = 3.0;

Savepoint = {}

Savepoint["connect"] = function(gameObject)
	savepoint = AppStateManager:getGameObjectController():castGameObject(gameObject);
    mainGameObject = AppStateManager:getGameObjectController():getGameObjectFromId(MAIN_GAMEOBJECT_ID);
    cooldown = SAVE_COOLDOWN;

    local physicsTriggerComponent = savepoint:getPhysicsTriggerComponent();
    
    physicsTriggerComponent:reactOnEnter(function(visitorGameObject)
        -- Never write a save game while testing in the editor.
        if (false == Core:isGame()) then
            do return end;
        end
	
          visitorGameObject = AppStateManager:getGameObjectController():castGameObject(visitorGameObject);
		  
        -- Enemies, eggs and everything else that flies through the trigger must not save.
        if (visitorGameObject:getName() ~= PLAYER_NAME) then
            do return end;
        end

        if (cooldown > 0) then
            do return end;
        end
        cooldown = SAVE_COOLDOWN;

        -- Attributes (energy, strength, experience, level, coins, killed enemies) go from the working copy into
        -- the global values, together with everything that is a world flag already (bosses, items, levers,
        -- abilities). The name of THIS game object tells the save game where the player comes back, it has to
        -- be listed in SAVEPOINTS in init.lua.
        saveGame(mainGameObject:getAttributesComponent(), savepoint:getName());

        -- Optional: add a sound component named "Save" to the save point game object.
        local saveSound = savepoint:getSimpleSoundComponentFromName("Save");
        if (nil ~= saveSound) then
            saveSound:setActivated(true);
		end
    end);
end

Savepoint["update"] = function(dt)
    if (cooldown > 0) then
        cooldown = cooldown - dt;
    end
end

Savepoint["disconnect"] = function()
    savepoint = nil;
    mainGameObject = nil;
end
