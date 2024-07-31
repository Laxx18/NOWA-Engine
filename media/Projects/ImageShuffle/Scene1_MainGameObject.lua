module("Scene1_MainGameObject", package.seeall);

randomImageShufflerComponent = nil;
Scene1_MainGameObject = {}

-- Scene: Scene1

require("init");

scene1_MainGameObject = nil

Scene1_MainGameObject["connect"] = function(gameObject)
	randomImageShufflerComponent = gameObject:getRandomImageShuffler();
end

Scene1_MainGameObject["disconnect"] = function()

end

Scene1_MainGameObject["update"] = function(dt)
	if InputDeviceModule:isActionDown(NOWA_A_ACTION) then
        randomImageShufflerComponent:start();
    elseif InputDeviceModule:isActionDown(NOWA_A_JUMP) then
        randomImageShufflerComponent:stop();
    end
end

Scene1_MainGameObject["onImageChosen"] = function(imageName, imageIndex)
    log("Image: " .. imageName .. " index: " .. imageIndex);
end