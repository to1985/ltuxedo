lt=require'ltuxedo_ws'

input=arg[1] or ''
s=lt.new_buffer('STRING',512)

s:set_raw(input)
r,e=lt.tpcall('PING',s,s)
print (r,e)
if r then print(r:get_raw()) end

s:set_raw(input)
r,e=lt.tpcall('CHKPWD',s,s)
print (r,e)
if r then print(r:get_raw()) end

s:set_raw(input)
r,e=lt.tpcall('CHK_PING',s,s)
print (r,e)
if r then print(r:get_raw()) end

s:set_raw(input)
cd=assert(lt.tpacall('PING',s))
times=1
repeat 
	r,e=lt.tpgetreply(cd,s,lt.TPNOBLOCK)
	if r == nil and lt.tperrno()~= lt.TPEBLOCK then error(e) end
	print('reply',times)
	os.execute('sleep 1')
	times=times+1
until r
print(r:get_raw())
