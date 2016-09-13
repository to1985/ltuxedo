local lt=require'ltuxedo_nc'
local buffer=assert(lt.new_buffer('FML32')):finit()

local printtable=function(name,table) print('table '..name) for k,v in pairs(table) do print(k,v) end end

for ln in io.lines() do
	if ln == '' then break end
	assert(buffer:fchg(167778172,0,'INFO'))
	assert(buffer:fchg(167778175,0,ln))
	local ctrl_table,err,diagnostic = lt.tpenqueue('TEST_QSPACE','TEST_QUEUE1',buffer,{flags=lt.TPQMSGID})
	if ctrl_table == nil then
		print('enqueue fail',err,diagnostic)
		break
	end
--	printtable('ctrl table',ctrl_table)
	print('msg '..ctrl_table.msgid..' sent')
end
buffer:fchg(167778172,0,'CMD')
buffer:fchg(167778175,0,'STOP')
lt.tpenqueue('TEST_QSPACE','TEST_QUEUE1',buffer)
os.exit(1,1)
