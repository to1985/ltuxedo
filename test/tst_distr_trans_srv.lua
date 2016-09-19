lt=require'ltuxedo_sv'
ls=require'luasql.informix'
env=assert(ls.informix())
conn=assert(env:connect('worktsdb'))

withdraw=function(req)
	local draw=function(req)
		local amt=tonumber(req:get_raw())
		local bal=tonumber(assert(assert(conn:execute('select amt from test_dis_trans where acct="from"')):fetch()))
		if amt > bal then error'available balance insufficient' end
		assert(conn:execute('update test_dis_trans set amt='..tostring(bal-amt)..' where acct="from"'))
		return 'success'
	end
	local _,msg=pcall(draw,req)
	return msg
end

deposit=function(req)
	local depo=function(req)
		local amt=tonumber(req:get_raw())
		local bal=tonumber(assert(assert(conn:execute('select amt from test_dis_trans where acct="to"')):fetch()))
		if -amt > bal then error'available balance insufficient' end
		assert(conn:execute('update test_dis_trans set amt='..tostring(bal+amt)..' where acct="to"'))
		return 'success'
	end
	local _,msg=pcall(depo,req)
	return msg
end

assert(lt.init_service('WITHDRAW',withdraw))
assert(lt.init_service('DEPOSIT',deposit))
assert(lt.init_xa_switch('/home/db/informix/lib/esql/libifxa.so','infx_xa_switch'))
assert(lt.mainloop('-g 106 -i 107 '))
