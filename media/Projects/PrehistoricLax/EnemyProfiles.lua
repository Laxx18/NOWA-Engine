-- Combat values of every enemy type, keyed by the enemy's tag name (GameObject TagName).
--
-- Used by PrehistoricLax.lua:
--
--     local EnemyProfiles = require("EnemyProfiles");
--     local profile = EnemyProfiles.get(enemyGameObject:getTagName());
--
-- No module() and no global name on purpose: require() hands back the table this file returns, and
-- the caller keeps it in a local.
--
-- Fields:
--   contactDamage         energy the player loses per landed attack
--   impactDelay           seconds from the attack start to the moment it hurts (0 = on contact)
--   attackDuration        length of the enemy's ANIM_ATTACK_1 clip, then it goes back to locomotion
--   attackCooldown        minimum time between two attacks, measured from the attack start
--   reach, reachVertical  how close the player must be for the attack to land (world units)
--   knockbackHorizontal   knockback velocity given to the player, sideways
--   knockbackUp           knockback velocity given to the player, upwards
--   staggerTime           how long the player's input stays locked after the hit
--   locomotionAnimation   optional, AnimationBlender constant name to go back to after the attack
--                         (default "ANIM_WALK_NORTH")

local EnemyProfiles = {};

local profiles = {
    Rhino = {
        contactDamage = 15, impactDelay = 0.3, attackDuration = 0.9, attackCooldown = 1.6,
        reach = 1.6, reachVertical = 1.5, knockbackHorizontal = 4.0, knockbackUp = 3.0, staggerTime = 0.35
    },

    -- A bit stronger and faster than the rhino. Its attack is the rolling charge (Roll_InPlace, 1.33 s),
    -- and it runs its path, so it goes back to running after the attack.
    Coyote = {
        contactDamage = 20, impactDelay = 0.25, attackDuration = 1.33, attackCooldown = 1.3,
        reach = 1.7, reachVertical = 1.5, knockbackHorizontal = 5.0, knockbackUp = 3.5, staggerTime = 0.4,
        locomotionAnimation = "ANIM_RUN"
    }
};

-- Used for every enemy whose tag name has no entry above.
local defaultProfile = {
    contactDamage = 10, impactDelay = 0.3, attackDuration = 0.9, attackCooldown = 1.5,
    reach = 1.5, reachVertical = 1.5, knockbackHorizontal = 3.0, knockbackUp = 2.5, staggerTime = 0.3
};

function EnemyProfiles.get(tagName)
    local profile = profiles[tagName];
    if (profile == nil) then
        return defaultProfile;
    end
    return profile;
end

return EnemyProfiles;
