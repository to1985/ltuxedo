require 'baselib'
lt=require'ltuxedo_sv'

ping=function(req)
	req:set_raw('ECHO:'..req:get_raw())
	os.execute('sleep 2')
	return req,100
end

checkpwd=function(req)
	if req:get_raw() ~= 'PASSWORD' then return nil
	else return '',0
	end
end

logger=BranchNative.Base.logger("test_ltuxedo.log")
assert(lt.init_service('PING',ping))
assert(lt.init_service('CHKPWD',checkpwd))
assert(lt.mainloop('-g 1 -i 700'))
