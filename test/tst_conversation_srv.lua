lt=require'ltuxedo_sv'

send_file=function(cd,path,fn)
	local buf=assert(lt.new_buffer('CARRAY',4096))
	local file=assert(io.open(path..fn,'rb'))
	for s in file:lines(4096) do
		buf:set_raw(s)
		event,err=assert(lt.tpsend(cd,buf))
		if event == lt.TPEV_DISCONIMM then
			file:close()
			return 'disconnected'
		end
	end
	file:close()
	return 'success'
end

rcv_file=function(cd,path,fn)
	local buf=assert(lt.new_buffer('CARRAY',4096))
	local file=assert(io.open(path..fn,'wb'))
	repeat
		event,err=assert(lt.tprecv(cd,buf))
		if event == lt.TPEV_DISCONIMM then
			file:close()
			return 'disconnected'
		end
		if event == lt.TPEV_SENDONLY then break end
		file:write(buf:get_raw())
	until false
	file:close()
	return 'success'
end

fileserver=function(req,cd)
	local mode,path,filename=string.match(req:get_raw(),'(%g+)\n+(%g+)\n+(%g+)')
	local result=''
	if mode == 'GET' then r,result=pcall(send_file,cd,path,filename)
	elseif mode == 'PUT' then r,result=pcall(rcv_file,cd,path,filename)
	else result='incorrect func'
	end
	return result
end

assert(lt.init_service('FILE_SRV',fileserver))
assert(lt.mainloop('-g 1 -i 18780'))
