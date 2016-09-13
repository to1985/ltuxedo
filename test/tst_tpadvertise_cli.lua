lt=require'ltuxedo_ws'

local input=arg[1] or ''
local s=lt.new_buffer('STRING',512)

if input== '' then
	s:set_raw('off')
	assert(lt.tpcall('SWITCH',s,s))
	print(s:get_raw())
	os.exit(1,1)
end

s:set_raw('on')
assert(lt.tpcall('SWITCH',s,s))
local r=s:get_raw()
print(r)
if string.sub(r,1,4) == 'fail' then
	os.exit(1,1)
end

s:set_raw(input)
assert(lt.tpcall('PING',s,s))
print(s:get_raw())
os.exit(1,1)
