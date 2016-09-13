require 'baselib'
logger=BranchNative.Base.logger("test_ltuxedo.log")
lt=require'ltuxedo_nc'
ls=require'luasql.informix'
env=assert(ls.informix())
conn=assert(env:connect('worktsdb'))

local from_amt=arg[1] or ''
local to_amt=arg[2] or ''
local transfer_amt=arg[3] or ''

if from_amt == '' or to_amt == '' or transfer_amt== '' then
	print('Uasge : '..arg[0]..' acct_from_bal  acct_to_bal transfer_amt ')
	os.exit()
end

conn:execute('drop table test_dis_trans')
assert(conn:execute([[
	create table test_dis_trans (
		acct char (32) not null,
		amt decimal (16,2) not null
	)lock mode row;
]]))
assert(conn:execute('insert into test_dis_trans values ("from",'..tostring(tonumber(from_amt))..')'))
assert(conn:execute('insert into test_dis_trans values ("to",'..tostring(tonumber(to_amt))..')'))

local snd_buf=assert(lt.new_buffer('STRING'))
local rcv_buf=assert(lt.new_buffer('STRING'))
assert(snd_buf:set_raw(tostring(tonumber(transfer_amt))))

assert(lt.tpbegin())
local result,err=lt.tpcall('WITHDRAW',snd_buf,rcv_buf)
if not result then
	logger.error('withdraw',err)
	assert(lt.tpabort())
	os.exit()
end
local rcv=assert(rcv_buf:get_raw())
if rcv ~= 'success' then
	logger.error('withdraw',rcv)
	assert(lt.tpabort())
	os.exit()
end
local result,err=lt.tpcall('DEPOSIT',snd_buf,rcv_buf)
if not result then
	logger.error('deposit',err)
	assert(lt.tpabort())
	os.exit()
end
local rcv=assert(rcv_buf:get_raw())
if rcv ~= 'success' then
	logger.error('deposit',rcv)
	assert(lt.tpabort())
	os.exit()
end
assert(lt.tpcommit())
print'success'
