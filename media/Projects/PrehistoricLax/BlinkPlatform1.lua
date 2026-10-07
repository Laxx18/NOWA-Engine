module("BlinkPlatform1", package.seeall);
-- Scene: Level5

require("init");

local blinkPlatform1 = nil;
local datablockPbsComponent = nil;
local physicsArtifactComponent = nil;

-- Cycle: Delay -> Solid -> FadeOut -> Gone -> FadeIn -> Solid -> ...
-- The platform stays solid during FadeOut on purpose: the fading is the warning for the player,
-- and he can still stand on it. It only becomes passable when it is fully gone.
local phase = "";
local phaseTimer = 0;

BlinkPlatform1 = {}

local function enterPhase(newPhase)
    phase = newPhase;
    phaseTimer = 0;

    if "Solid" == phase then
        datablockPbsComponent:setTransparency(1);
        physicsArtifactComponent:setActivated(true);
    elseif "FadeOut" == phase then
        datablockPbsComponent:setTransparency(1);
    elseif "Gone" == phase then
        datablockPbsComponent:setTransparency(0);
        physicsArtifactComponent:setActivated(false);
    elseif "FadeIn" == phase then
        datablockPbsComponent:setTransparency(0);
    end
end

BlinkPlatform1["connect"] = function(gameObject)
    blinkPlatform1 = AppStateManager:getGameObjectController():castGameObject(gameObject);
    datablockPbsComponent = blinkPlatform1:getDatablockPbsComponent();
    physicsArtifactComponent = blinkPlatform1:getPhysicsArtifactComponent();

    -- startDelay shifts the whole cycle, so several platforms blink out of phase
    phase = "Delay";
    phaseTimer = 0;

    datablockPbsComponent:setTransparency(1);
    physicsArtifactComponent:setActivated(true);
end

BlinkPlatform1["disconnect"] = function()
    -- Leave the platform in its normal state, so it is visible and solid again when the simulation stops
    datablockPbsComponent:setTransparency(1);
    physicsArtifactComponent:setActivated(true);
end

BlinkPlatform1["update"] = function(dt)
    phaseTimer = phaseTimer + dt;

    if "Delay" == phase then
        if phaseTimer >= BLINK_PLATFORM_1.startDelay then
            enterPhase("Solid");
        end
    elseif "Solid" == phase then
        if phaseTimer >= BLINK_PLATFORM_1.solidTime then
            enterPhase("FadeOut");
        end
    elseif "FadeOut" == phase then
        local progress = phaseTimer / BLINK_PLATFORM_1.fadeOutTime;
        if progress > 1 then
            progress = 1;
        end
        datablockPbsComponent:setTransparency(1 - progress);

        if phaseTimer >= BLINK_PLATFORM_1.fadeOutTime then
            enterPhase("Gone");
        end
    elseif "Gone" == phase then
        if phaseTimer >= BLINK_PLATFORM_1.goneTime then
            enterPhase("FadeIn");
        end
    elseif "FadeIn" == phase then
        local progress = phaseTimer / BLINK_PLATFORM_1.fadeInTime;
        if progress > 1 then
            progress = 1;
        end
        datablockPbsComponent:setTransparency(progress);

        if phaseTimer >= BLINK_PLATFORM_1.fadeInTime then
            enterPhase("Solid");
        end
    end
end