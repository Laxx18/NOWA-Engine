-- A better random seed. This was taken from http://lua-users.org/wiki/MathLibraryTutorialmath.randomseed(tonumber(tostring(os.time()):reverse():sub(1,6)))

function dump(o)
	if type(o) == 'table' then
		local s = '{ '		for k,v in pairs(o) do
			if type(k) ~= 'number' then k = '"'..k..'"' end
			s = s .. '['..k..'] = ' .. dump(v) .. ','
		end
		return s .. '} '
	else
		return tostring(o)
	end
end

-- Register your events here:
-- Example: ScriptEventManager:registerEvent("DestroyStone");
