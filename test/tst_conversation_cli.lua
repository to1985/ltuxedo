lt=require'ltuxedo_ws'

put_file=function(path,filename)
	local cmd=assert(lt.new_buffer('STRING'))
	local s='PUT\n'..path..'\n'..filename
	cmd:set_raw(s)
	local cd=assert(lt.tpconnect('FILE_SRV',cmd))

	local buf=assert(lt.new_buffer('CARRAY',4096))
	file=assert(io.open(filename,'rb'))
	for s in file:lines(4096) do
		buf:set_raw(s)
		event,err=assert(lt.tpsend(cd,buf))
		if event == lt.TPEV_DISCONIMM then
			file:close()
			lt.tpdiscon(cd)
			return 'fail - server disconnected'
		end
	end
	file:close()
	event,err=assert(lt.tpsend(cd,nil,lt.TPRECVONLY))
	event,err=assert(lt.tprecv(cd,cmd))
	lt.tpdiscon(cd)
	if event ~= lt.TPEV_SVCSUCC then return 'fail - server event:'..event end
	return 'success'
end

get_file=function(path,filename)
	local cmd=assert(lt.new_buffer('STRING'))
	local s='GET\n'..path..'\n'..filename
	cmd:set_raw(s)
	local cd=assert(lt.tpconnect('FILE_SRV',cmd,lt.TPRECVONLY))

	buf=assert(lt.new_buffer('CARRAY',4096))
	file=assert(io.open(filename,'wb'))
	repeat
		event,err=assert(lt.tprecv(cd,buf))
		if event == lt.TPEV_SVCSUCC then break end
		if event == lt.TPEV_SVCERR then
			file:close()
			lt.tpdiscon(cd)
			return 'fail - service err'
		end
		file:write(buf:get_raw())
	until false
	file:close()
	lt.tpdiscon(cd)
	return buf:get_raw()
end

local func=string.upper(string.sub(arg[1] or '',1,1))
local path=arg[2] or ''
local filename=arg[3] or ''
local wsn=arg[4] or ''
if func == '' or path == '' or filename == '' then
	print('usage: '..arg[0]..' GET|PUT remote_path filename_without_path [WSNADDR] ')
	os.exit()
end
if wsn ~= '' then
	assert(lt.tpsetenv('WSNADDR='..wsn))
	assert(lt.tpinit())
end
if string.sub(path,-1) ~= '/' then path=path..'/' end
if func == 'G' then
	r,result=pcall(get_file,path,filename)
elseif func == 'P' then
	r,result=pcall(put_file,path,filename)
else
	error(arg[1]..'not correct')
	os.exit()
end
print(result)
