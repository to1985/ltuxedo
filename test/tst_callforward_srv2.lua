require 'baselib'
lt=require'ltuxedo_sv'

forward_test=function(req)
	r,e=lt.tpcall('CHKPWD',req,req)
	if r then
		return 'INFO:42'
	else
		logger.error(e)
		lt.tpforward('PING')
	end
	return req
end

logger=BranchNative.Base.logger("test_ltuxedo.log")
assert(lt.init_service('CHK_PING',forward_test))
assert(lt.mainloop('-g 1 -i 701'))
