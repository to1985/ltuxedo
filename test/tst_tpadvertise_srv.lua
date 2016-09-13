require 'baselib'
lt=require'ltuxedo_sv'
switch=0

ping=function(req)
	req:set_raw('ECHO:'..req:get_raw())
	return req
end

switch_service=function(req)
	local s=string.upper(req:get_raw())
	local r,e
	if s == 'ON' and switch == 0 then
		r,e=lt.tpadvertise('PING',ping)
		if r then switch=1 end
	elseif s == 'OFF' and switch == 1 then
		r,e=lt.tpunadvertise('PING')
		if r then switch=0 end
	else
		return 'already in '..s..' mode'
	end
	if not r then return 'fail : '..e end
	return 'success'
end

assert(lt.init_service('SWITCH',switch_service))
assert(lt.mainloop('-g 1 -i 710'))
