function myprint(...)
	local str
	str = ''
 
	for i = 1, arg.n do
		if str ~= '' then str = str .. '\t' end
		str = str .. tostring( arg[i] )
	end
 
	str = str .. '\n'
 
	interpreterOutput( str )
end
 
oldprint = print
print = myprint

-- This redirects the output of any scripts that use print to the console output. You can use oldprint to still print to stdout if you need to.
-- Remember interpreterOutput is the static method the interpreter class registers, and note it does not add a newline to the end of the string you provide.
-- While this works, I found better results with binding the 'print' method of the console class to Lua and calling that.
-- This way, any intermediate output from a script is displayed almost immediately, rather than once the script is finished. 
