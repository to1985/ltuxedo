local lt=require'ltuxedo_nc'
local buffer=lt.new_buffer('FML32')

local printtable=function(name,table) print('table '..name) for k,v in pairs(table) do print(k,v) end end

repeat
	local ctrl_table,err,diagnostic = lt.tpdequeue('TEST_QSPACE','TEST_QUEUE1',buffer,{flags=lt.TPQWAIT})
	if ctrl_table ~= nil then
		opr=buffer:fget(167778172)
		sts=buffer:fget(167778175)
		if opr == 'INFO' then
			print('INFO: ',sts)
--			printtable('ctrl table',ctrl_table)
--			printtable('cltid table',ctrl_table.cltid)
		end
	else
		if diagnostic ~= lt.QMENOMSG then
			print('enqueue fail',err,diagnostic)
			break
		end
	end
until opr == 'CMD' and sts == 'STOP'
print('stop')
os.exit(1,1)
