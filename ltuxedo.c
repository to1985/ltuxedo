/*
**	lua tuxedo extension
*/

#include "ltuxedo.h"

#define PARA_MAX_NUM 64

static char errmsg[256];

static inline void set_errmsg_tp(void)
{
	strncpy(errmsg,tpstrerror(tperrno),sizeof(errmsg)-1);
}

static inline void set_errmsg_fml(void)
{
	strncpy(errmsg,Fstrerror32(Ferror32),sizeof(errmsg)-1);
}

static inline size_t get_value_len(LTUXEDO_BUFFER *p_buf)
{
	if (p_buf->ptr == NULL) return 0;
	if (strcmp(p_buf->type,"STRING") == 0) return strlen(p_buf->ptr);
	if (strcmp(p_buf->type,"FML32") == 0) return Fused32((FBFR32 *)(p_buf->ptr));
	return p_buf->raw_len;
}
/* ----------------------BUFFER API-----------------------*/

static int buffer_tostring(lua_State *L)
{
	LTUXEDO_BUFFER *p_buf=(LTUXEDO_BUFFER *)luaL_checkudata(L,1,LUAMETA_BUFFER);
	if (p_buf->ptr)
	{
		lua_pushfstring(L,"TUXEDO %s buffer, size:%d, len:%d",p_buf->type,p_buf->size,get_value_len(p_buf));
	}
	else
	{
		lua_pushfstring(L,"TUXEDO %s buffer, freed",p_buf->type);
	}
	return 1;
}

static int buffer_len(lua_State *L)
{
	LTUXEDO_BUFFER *p_buf=(LTUXEDO_BUFFER *)luaL_checkudata(L,1,LUAMETA_BUFFER);
	lua_pushinteger(L,get_value_len(p_buf));
	return 1;
}

static int buffer_gc(lua_State *L)
{
	LTUXEDO_BUFFER *p_buf=(LTUXEDO_BUFFER *)luaL_checkudata(L,1,LUAMETA_BUFFER);
	if (p_buf->ptr)
	{
		tpfree(p_buf->ptr);
		p_buf->ptr=NULL;
	}
	p_buf->raw_len=0;
	return 0;
}

/* realloc tuxedo buffer 
          buffer,err = buffer:realloc(size)
            IN:
              size : tuxedo buffer realloc length

           OUT:
            buffer : tuxedo buffer , nil for err
               err : error msg
*/
static int buffer_realloc(lua_State *L)
{
	LTUXEDO_BUFFER *p_buf=(LTUXEDO_BUFFER *)luaL_checkudata(L,1,LUAMETA_BUFFER);
	long size=lua_tointeger(L,2);
	char *result;

	if ((result=tprealloc(p_buf->ptr,size)) == NULL)
	{
		set_errmsg_tp();
		lua_pushnil(L);
		lua_pushstring(L,errmsg);
		return 2;
	}
	p_buf->ptr=result;
	if ((p_buf->size=tptypes(p_buf->ptr,NULL,NULL)) == -1) 
	{
		userlog("tptypes fails after tprealloc %d %s",tperrno,tpstrerror(tperrno));
		p_buf->size=size;
	}
	if (p_buf->raw_len > p_buf->size)
	{
		p_buf->raw_len=p_buf->size;
	}

	lua_pushvalue(L,1);
	return 1;
}

/* free tuxedo buffer 
            buffer:free()
*/
static int buffer_free(lua_State *L)
{
	return buffer_gc(L);
}

/* get tuxedo buffer size
              size = buffer:size()
           OUT:
              size : tuxedo buffer size length
*/
static int buffer_sizeof(lua_State *L)
{
	LTUXEDO_BUFFER *p_buf=(LTUXEDO_BUFFER *)luaL_checkudata(L,1,LUAMETA_BUFFER);
	size_t len=0;
	if (p_buf->ptr)
	{
		len=p_buf->size;
	}
	lua_pushinteger(L,len);
	return 1;
}

/* get tuxedo buffer raw data
        string,err = buffer:get_raw()
           OUT:
            string : tuxedo buffer raw data, nil for err
               err : error msg
*/
static int buffer_get_raw(lua_State *L)
{
	LTUXEDO_BUFFER *p_buf=(LTUXEDO_BUFFER *)luaL_checkudata(L,1,LUAMETA_BUFFER);
	size_t len=get_value_len(p_buf);
	if (len == -1)
	{
		set_errmsg_fml();
		lua_pushnil(L);
		lua_pushstring(L,errmsg);
		return 2;
	}
	lua_pushlstring(L,p_buf->ptr,len);
	return 1;
}

/* set tuxedo buffer raw data
        buffer,err = buffer:set_raw(string)
            IN:
            string : tuxedo buffer raw data

           OUT:
            buffer : tuxedo buffer , nil for err
               err : error msg
*/
static int buffer_set_raw(lua_State *L)
{
	LTUXEDO_BUFFER *p_buf=(LTUXEDO_BUFFER *)luaL_checkudata(L,1,LUAMETA_BUFFER);
	size_t len=0;
	char *data=(char *)lua_tolstring(L,2,&len);

	if (len > p_buf->size)
	{
		lua_pushnil(L);
		lua_pushliteral(L,"buffer size exceed");
		return 2;
	}
	p_buf->raw_len=len;
	memset(p_buf->ptr,0,p_buf->size);
	memcpy(p_buf->ptr,data,len);
	lua_pushvalue(L,1);
	return 1;
}

/* create tuxedo buffer 
         buffer ,err = new_buffer(type [,subtype] [,size])
            IN:
              type : tuxedo buffer type string, - "CARRAY" "STRING" "FML32" "XML"
           subtype : tuxedo buffer sub type string, none for null
              size : tuxedo buffer length, none for default size

           OUT:
            buffer : tuxedo buffer , nil for err
               err : error msg
*/
static int ltuxedo_new_buffer(lua_State *L)
{
	long size=0,top=2;
	char *buffer_type=(char *)luaL_checkstring(L,1);
	char *buffer_subtype=NULL;
	LTUXEDO_BUFFER *p_buf=NULL;

	if (lua_type(L,top) == LUA_TSTRING)
	{
		buffer_subtype=(char *)lua_tostring(L,top);
		top++;
	}
	if (lua_type(L,top) == LUA_TNUMBER)
	{
		size=lua_tointeger(L,top);
		top++;
	}

	p_buf=(LTUXEDO_BUFFER *)lua_newuserdata(L,sizeof(LTUXEDO_BUFFER));
	memset(p_buf,0,sizeof(LTUXEDO_BUFFER));
	p_buf->ptr=NULL;
	luaL_getmetatable(L,LUAMETA_BUFFER);
	lua_setmetatable(L,-2);

	strncpy(p_buf->type,buffer_type,sizeof(p_buf->type)-1);
	if (buffer_subtype) strncpy(p_buf->sub_type,buffer_subtype,sizeof(p_buf->sub_type)-1);
	p_buf->ptr=tpalloc(buffer_type,buffer_subtype,size);
	p_buf->raw_len=0;

	if (p_buf->ptr == NULL)
	{
		set_errmsg_tp();
		lua_pop(L,1);
		lua_pushnil(L);
		lua_pushstring(L,errmsg);
		return 2;
	}
	if ((p_buf->size=tptypes(p_buf->ptr,NULL,NULL)) == -1)
	{
		set_errmsg_tp();
		lua_pop(L,1);
		lua_pushnil(L);
		lua_pushfstring(L,"tptypes fail after tpalloc %s",errmsg);
		return 2;
	}
	memset(p_buf->ptr,0,p_buf->size);

	return 1;
}

/* ----------------------FML32 API-----------------------*/

#define CHECK_FML32_TYPE(buf) \
	if (strcmp(buf->type,"FML32") != 0) \
	{ \
		lua_pushnil(L); \
		lua_pushliteral(L,"not a FML32 buffer"); \
		return 2; \
	} \

/* return ferror
      errno,errmsg = ferror()
           OUT:
             errno : ferror 
            errmsg : Fstrerror32()
*/
static int ltuxedo_ferror(lua_State *L)
{
	lua_pushinteger(L,Ferror32);
	lua_pushstring(L,Fstrerror32(Ferror32));
	return 2;
}

/* Fneeded32
          size,err = fneeded(occ,len)
            IN:
               occ : number of fields
               len : length of fields

           OUT:
              size : needed fml32 buffer size, nil for err
               err : error msg
*/
static int ltuxedo_fml_needed(lua_State *L)
{
	long occ=lua_tointeger(L,1);
	long len=lua_tointeger(L,2);
	long size=0;

	size=Fneeded32(occ,len);
	if (size == -1)
	{
		set_errmsg_fml();
		lua_pushnil(L);
		lua_pushstring(L,errmsg);
		return 2;
	}

	lua_pushinteger(L,size);
	return 1;
}

/* Fldid32
             id,err=fld_id(field_name)
            IN:
        field_name : field name

           OUT:
                id : field id , nil for err
               err : error msg
*/
static int ltuxedo_fml_id(lua_State *L)
{
	char *name=(char *)lua_tostring(L,1);
	FLDID32 id=0;

	if ((id=Fldid32(name)) == BADFLDID)
	{
		set_errmsg_fml();
		lua_pushnil(L);
		lua_pushstring(L,errmsg);
		return 2;
	}

	lua_pushinteger(L,id);
	return 1;
}

/* Fname32
           name,err=fld_name(field_id)
            IN:
          field_id : fml32 field id

           OUT:
              name : field name string
               err : error msg
*/
static int ltuxedo_fml_name(lua_State *L)
{
	FLDID32 id=lua_tointeger(L,1);
	char *name;

	if ((name=Fname32(id)) == NULL)
	{
		set_errmsg_fml();
		lua_pushnil(L);
		lua_pushstring(L,errmsg);
		return 2;
	}

	lua_pushstring(L,name);
	return 1;
}

/* Ftype32
           type,err=fld_type(field_id)
            IN:
          field_id : fml32 field id

           OUT:
              type : field type string
               err : error msg
*/
static int ltuxedo_fml_type(lua_State *L)
{
	FLDID32 id=lua_tointeger(L,1);
	char *type;

	if ((type=Ftype32(id)) == NULL)
	{
		set_errmsg_fml();
		lua_pushnil(L);
		lua_pushstring(L,errmsg);
		return 2;
	}

	lua_pushstring(L,type);
	return 1;
}

/* Fldno32
              fldno=fld_no(field_id)
            IN:
          field_id : fml32 field id

           OUT:
             fldno : field number
*/
static int ltuxedo_fml_no(lua_State *L)
{
	FLDID32 id=lua_tointeger(L,1);
	lua_pushinteger(L,Fldno32(id));
	return 1;
}

/* Fmkfldid32
             id,err=mkfld_id(type,fld_num)
            IN:
              type : field type def  -  ltuxedo.FLD_STRING etc ...
           fld_num : field number

           OUT:
                id : field id , nil for err
               err : error msg
*/
static int ltuxedo_fml_mkfldid(lua_State *L)
{
	int type=lua_tointeger(L,1);
	FLDID32 num=lua_tointeger(L,2);
	FLDID32 id;

	if ((id=Fmkfldid32(type,num)) == BADFLDID)
	{
		set_errmsg_fml();
		lua_pushnil(L);
		lua_pushstring(L,errmsg);
		return 2;
	}

	lua_pushinteger(L,id);
	return 1;
}

/* Finit32
         buffer,err=buffer:finit()
            IN:

           OUT:
            buffer : fml32 buffer, nil for err
               err : error msg
*/
static int buffer_fml_init(lua_State *L)
{
	LTUXEDO_BUFFER *p_buf=(LTUXEDO_BUFFER *)luaL_checkudata(L,1,LUAMETA_BUFFER);

	CHECK_FML32_TYPE(p_buf);

	if (Finit32((FBFR32 *)(p_buf->ptr),p_buf->size) == -1)
	{
		set_errmsg_fml();
		lua_pushnil(L);
		lua_pushstring(L,errmsg);
		return 2;
	}

	lua_pushvalue(L,1);
	return 1;
}

/* Fadd32
         buffer,err=buffer:fadd(fieldid,str)
            IN:
           fieldid : field id
               str : field value

           OUT:
            buffer : fml32 buffer, nil for err
               err : error msg
*/
static int buffer_fml_add(lua_State *L)
{
	LTUXEDO_BUFFER *p_buf=(LTUXEDO_BUFFER *)luaL_checkudata(L,1,LUAMETA_BUFFER);
	FLDID32 id=lua_tointeger(L,2);
	char *value=NULL;
	size_t len=0;

	CHECK_FML32_TYPE(p_buf);

	value=(char *)lua_tolstring(L,3,&len);
	if (CFadd32((FBFR32 *)(p_buf->ptr),id,value,len,FLD_CARRAY) == -1)
	{
		set_errmsg_fml();
		lua_pushnil(L);
		lua_pushstring(L,errmsg);
		return 2;
	}

	lua_pushvalue(L,1);
	return 1;
}

/* Fchg32
         buffer,err=buffer:fchg(fieldid,occ,str)
            IN:
           fieldid : field id
               occ : field occurrence number
               str : field value

           OUT:
            buffer : fml32 buffer, nil for err
               err : error msg
*/
static int buffer_fml_chg(lua_State *L)
{
	LTUXEDO_BUFFER *p_buf=(LTUXEDO_BUFFER *)luaL_checkudata(L,1,LUAMETA_BUFFER);
	FLDID32 id=lua_tointeger(L,2);
	FLDOCC32 oc=lua_tointeger(L,3);
	char *value=NULL;
	size_t len=0;

	CHECK_FML32_TYPE(p_buf);

	value=(char *)lua_tolstring(L,4,&len);
	if (CFchg32((FBFR32 *)(p_buf->ptr),id,oc,value,len,FLD_CARRAY) == -1)
	{
		set_errmsg_fml();
		lua_pushnil(L);
		lua_pushstring(L,errmsg);
		return 2;
	}

	lua_pushvalue(L,1);
	return 1;
}

/* Fget32
         buffer,err=buffer:fget(fieldid [,occ])
            IN:
           fieldid : field id
               occ : field occurrence number , default 0

           OUT:
            buffer : fml32 buffer, nil for err
               err : error msg
*/
static int buffer_fml_get(lua_State *L)
{
	LTUXEDO_BUFFER *p_buf=(LTUXEDO_BUFFER *)luaL_checkudata(L,1,LUAMETA_BUFFER);
	FLDID32 id=lua_tointeger(L,2);
	FLDOCC32 oc=luaL_optinteger(L,3,0);
	char *value=NULL;
	FLDLEN32 len=0;

	CHECK_FML32_TYPE(p_buf);

	if ((value=CFgetalloc32((FBFR32 *)(p_buf->ptr),id,oc,FLD_CARRAY,&len)) == NULL)
	{
		set_errmsg_fml();
		lua_pushnil(L);
		lua_pushstring(L,errmsg);
		return 2;
	}

	lua_pushlstring(L,value,len);
	free(value);
	return 1;
}

/* Fdel32
         buffer,err=buffer:fdel(fieldid [,occ])
            IN:
           fieldid : field id
               occ : field occurrence number , default 0

           OUT:
            buffer : fml32 buffer, nil for err
               err : error msg
*/
static int buffer_fml_del(lua_State *L)
{
	LTUXEDO_BUFFER *p_buf=(LTUXEDO_BUFFER *)luaL_checkudata(L,1,LUAMETA_BUFFER);
	FLDID32 id=lua_tointeger(L,2);
	FLDOCC32 oc=luaL_optinteger(L,3,0);

	CHECK_FML32_TYPE(p_buf);

	if (Fdel32((FBFR32 *)(p_buf->ptr),id,oc) == -1)
	{
		set_errmsg_fml();
		lua_pushnil(L);
		lua_pushstring(L,errmsg);
		return 2;
	}

	lua_pushvalue(L,1);
	return 1;
}

/* Fdelall32
         buffer,err=buffer:fdel_all(fieldid)
            IN:
           fieldid : field id

           OUT:
            buffer : fml32 buffer, nil for err
               err : error msg
*/
static int buffer_fml_del_all(lua_State *L)
{
	LTUXEDO_BUFFER *p_buf=(LTUXEDO_BUFFER *)luaL_checkudata(L,1,LUAMETA_BUFFER);
	FLDID32 id=lua_tointeger(L,2);

	CHECK_FML32_TYPE(p_buf);

	if (Fdelall32((FBFR32 *)(p_buf->ptr),id) == -1)
	{
		set_errmsg_fml();
		lua_pushnil(L);
		lua_pushstring(L,errmsg);
		return 2;
	}

	lua_pushvalue(L,1);
	return 1;
}

/* Fcpy32
    dest_buffer,err=dest_buffer:fcopy_from(src_buffer)
            IN:
        src_buffer : fml32 buffer

           OUT:
       dest_buffer : fml32 buffer, nil for err
               err : error msg
*/
static int buffer_fml_copy_from(lua_State *L)
{
	LTUXEDO_BUFFER *dest_buf=(LTUXEDO_BUFFER *)luaL_checkudata(L,1,LUAMETA_BUFFER);
	LTUXEDO_BUFFER *src_buf=(LTUXEDO_BUFFER *)luaL_checkudata(L,2,LUAMETA_BUFFER);

	CHECK_FML32_TYPE(src_buf);
	CHECK_FML32_TYPE(dest_buf);

	if (Fcpy32((FBFR32 *)(dest_buf->ptr),(FBFR32 *)(src_buf->ptr)) == -1)
	{
		set_errmsg_fml();
		lua_pushnil(L);
		lua_pushstring(L,errmsg);
		return 2;
	}

	lua_pushvalue(L,1);
	return 1;
}

/* Fjoin32
    dest_buffer,err=dest_buffer:fjoin_from(src_buffer)
            IN:
        src_buffer : fml32 buffer

           OUT:
       dest_buffer : fml32 buffer, nil for err
               err : error msg
*/
static int buffer_fml_join_from(lua_State *L)
{
	LTUXEDO_BUFFER *dest_buf=(LTUXEDO_BUFFER *)luaL_checkudata(L,1,LUAMETA_BUFFER);
	LTUXEDO_BUFFER *src_buf=(LTUXEDO_BUFFER *)luaL_checkudata(L,2,LUAMETA_BUFFER);

	CHECK_FML32_TYPE(src_buf);
	CHECK_FML32_TYPE(dest_buf);

	if (Fjoin32((FBFR32 *)(dest_buf->ptr),(FBFR32 *)(src_buf->ptr)) == -1)
	{
		set_errmsg_fml();
		lua_pushnil(L);
		lua_pushstring(L,errmsg);
		return 2;
	}

	lua_pushvalue(L,1);
	return 1;
}

/* Foccur32
            occ,err=buffer:foccur(fieldid)
            IN:
           fieldid : field id

           OUT:
               occ : field number of occurrences, nil for err
               err : error msg
*/
static int buffer_fml_occur(lua_State *L)
{
	LTUXEDO_BUFFER *p_buf=(LTUXEDO_BUFFER *)luaL_checkudata(L,1,LUAMETA_BUFFER);
	FLDID32 id=lua_tointeger(L,2);
	FLDOCC32 occ=0;

	CHECK_FML32_TYPE(p_buf);

	if ((occ=Foccur32((FBFR32 *)(p_buf->ptr),id)) == -1)
	{
		set_errmsg_fml();
		lua_pushnil(L);
		lua_pushstring(L,errmsg);
		return 2;
	}

	lua_pushinteger(L,occ);
	return 1;
}

/* Ffindocc32
            occ,err=buffer:ffindocc(fieldid,pattern_str)
            IN:
           fieldid : field id
       pattern_str : the user-specified field value

           OUT:
               occ : the occurrence number of the first field occurrence that matches pattern, nil for err
               err : error msg
*/
static int buffer_fml_findocc(lua_State *L)
{
	LTUXEDO_BUFFER *p_buf=(LTUXEDO_BUFFER *)luaL_checkudata(L,1,LUAMETA_BUFFER);
	FLDID32 id=lua_tointeger(L,2);
	size_t len;
	char *pattern=(char *)lua_tolstring(L,3,&len);
	FLDOCC32 occ=0;

	CHECK_FML32_TYPE(p_buf);

	if ((occ=Ffindocc32((FBFR32 *)(p_buf->ptr),id,pattern,len)) == -1)
	{
		set_errmsg_fml();
		lua_pushnil(L);
		lua_pushstring(L,errmsg);
		return 2;
	}

	lua_pushinteger(L,occ);
	return 1;
}

int buffer_fml_field_iter(lua_State *L)
{
	int ret;
	LTUXEDO_BUFFER *p_buf=(LTUXEDO_BUFFER *)luaL_checkudata(L,1,LUAMETA_BUFFER);
	FLDID32 id=lua_tointeger(L,2);
	FLDOCC32 occ=lua_tointeger(L,lua_upvalueindex(1));

	ret=Fnext32((FBFR32 *)(p_buf->ptr),&id,&occ,NULL,NULL);
	if (ret <= 0)
	{
		if (ret == -1)
		{
			set_errmsg_fml();
			userlog(errmsg);
		}
		lua_pushnil(L);
		return 1;
	}

	lua_pushinteger(L,id);
	lua_pushinteger(L,occ);
	lua_pushinteger(L,occ);
	lua_replace(L,lua_upvalueindex(1));

	return 2;
}

/* Fnext32 warpper
          for fldid,occ in buffer:ffield_iterator([fieldid[,occ]])
            IN:
           fieldid : field id
               occ : field occurrence number

           OUT:
           fieldid : all field id
               occ : all field occurrence number
*/
static int buffer_fml_get_field_iter(lua_State *L)
{
	LTUXEDO_BUFFER *p_buf=(LTUXEDO_BUFFER *)luaL_checkudata(L,1,LUAMETA_BUFFER);
	FLDID32 id=luaL_optinteger(L,2,FIRSTFLDID);
	FLDOCC32 occ=luaL_optinteger(L,3,0);

	CHECK_FML32_TYPE(p_buf);

	lua_pushinteger(L,occ);
	lua_pushcclosure(L,buffer_fml_field_iter,1);
	lua_pushvalue(L,1);
	lua_pushinteger(L,id);
	return 3;
}

/* -----------------------SVC API-----------------------*/

/* call TPCALL
     result_buffer,err = tpcall(svc ,send_buffer ,result_buffer [,flag])
            IN:
               svc : tuxedo service name
       send_buffer : tuxedo buffer to send data , or nil
     result_buffer : tuxedo buffer to receive data
              flag : TPCALL flag - TPNOTRAN TPNOCHANGE TPNOBLOCK TPNOTIME TPSIGRSTRT

           OUT:
     result_buffer : tuxedo buffer to receive data , nil for err
               err : error msg
*/
static int ltuxedo_tpcall(lua_State *L)
{
	int ret=-1;
	char *svc=(char *)luaL_checkstring(L,1);
	LTUXEDO_BUFFER *send=NULL;
	if (!lua_isnil(L,2)) send=(LTUXEDO_BUFFER *)luaL_checkudata(L,2,LUAMETA_BUFFER);
	LTUXEDO_BUFFER *rcv=(LTUXEDO_BUFFER *)luaL_checkudata(L,3,LUAMETA_BUFFER);
	long flags=luaL_optinteger(L,4,0);

	do
	{
		long ilen=0,olen=0;
		char *idata=NULL;
		if (send)
		{
			ilen=send->raw_len;
			idata=send->ptr;
		}
		if ((ret=tpcall(svc,idata,ilen,&(rcv->ptr),&olen,flags)) == -1)
		{
			set_errmsg_tp();
			break;
		}
		if (olen != 0)
		{
			rcv->size=tptypes(rcv->ptr,rcv->type,rcv->sub_type);	/* rcv buffer may change */
			if (rcv->size == -1)
			{
				set_errmsg_tp();
				ret=-1;
				break;
			}
			rcv->raw_len=olen;
		}
		ret=0;
	}
	while(0);

	if (ret != 0)
	{
		lua_pushnil(L);
		lua_pushstring(L,errmsg);
		return 2;
	}

	lua_pushvalue(L,3);
	return 1;
}

/* call TPACALL
   call_descriptor ,err = tpacall(svc ,send_buffer [,flag])
            IN:
               svc : tuxedo service name
       send_buffer : tuxedo buffer to send data , or nil
              flag : TPACALL flag - TPNOTRAN TPNOREPLY TPNOBLOCK TPNOTIME TPSIGRSTRT

           OUT:
   call_descriptor : descriptor that used to receive the reply of the request sent , nil for err
               err : error msg
*/
static int ltuxedo_tpacall(lua_State *L)
{
	int ret=-1;
	char *svc=(char *)luaL_checkstring(L,1);
	LTUXEDO_BUFFER *send=NULL;
	if (!lua_isnil(L,2)) send=(LTUXEDO_BUFFER *)luaL_checkudata(L,2,LUAMETA_BUFFER);
	long flags=luaL_optinteger(L,3,0);

	long ilen=0;
	char *idata=NULL;
	if (send)
	{
		ilen=send->raw_len;
		idata=send->ptr;
	}
	if ((ret=tpacall(svc,idata,ilen,flags)) == -1)
	{
		set_errmsg_tp();
		lua_pushnil(L);
		lua_pushstring(L,errmsg);
		return 2;
	}

	lua_pushinteger(L,ret);
	return 1;
}

/* call TPGETRPLY
     result_buffer , err|real_descriptor = tpgetreply(call_descriptor ,result_buffer [,flag])
            IN:
   call_descriptor : descriptor that return by tpacall , ignore for TPGETANY flag
     result_buffer : tuxedo buffer to receive data
              flag : TPGETRPLY flag - TPGETANY TPNOCHANGE TPNOBLOCK TPNOTIME TPSIGRSTRT

           OUT:
     result_buffer : tuxedo buffer to receive data , nil for err
               err : error msg 
                     or return the call descriptor for the reply returned when TPGETANY flag set
*/
static int ltuxedo_tpgetreply(lua_State *L)
{
	int ret=-1;
	int cd=luaL_checkinteger(L,1);
	LTUXEDO_BUFFER *rcv=(LTUXEDO_BUFFER *)luaL_checkudata(L,2,LUAMETA_BUFFER);
	long flags=luaL_optinteger(L,3,0);

	do
	{
		long olen=0;
		if ((ret=tpgetrply(&cd,&(rcv->ptr),&olen,flags)) == -1)
		{
			set_errmsg_tp();
			break;
		}
		if (olen != 0)
		{
			rcv->size=tptypes(rcv->ptr,rcv->type,rcv->sub_type);	/* rcv buffer may change */
			if (rcv->size == -1)
			{
				set_errmsg_tp();
				ret=-1;
				break;
			}
			rcv->raw_len=olen;
		}
		ret=0;
	}
	while(0);

	if (ret != 0)
	{
		lua_pushnil(L);
		lua_pushstring(L,errmsg);
		return 2;
	}

	lua_pushvalue(L,2);
	lua_pushinteger(L,cd);
	return 2;
}

/* call TPCANCEL
       result, err = tpcancel(call_descriptor)
            IN:
   call_descriptor : descriptor that return by tpacall

           OUT:
            result : nil for err
               err : error msg
*/
static int ltuxedo_tpcancel(lua_State *L)
{
	int cd=luaL_checkinteger(L,1);

	if (tpcancel(cd) == -1)
	{
		set_errmsg_tp();
		lua_pushnil(L);
		lua_pushstring(L,errmsg);
		return 2;
	}

	lua_pushboolean(L,1);
	return 1;
}

/* call tpconnect
   conn_descriptor,err = tpconnect(svc ,send_buffer [,flag])
            IN:
               svc : tuxedo service name
       send_buffer : tuxedo buffer to send data , or nil
              flag : tpconnect flag , default TPSENDONLY
			               - TPNOTRAN TPSENDONLY TPRECVONLY TPNOBLOCK TPNOTIME TPSIGRSTRT

           OUT:
   conn_descriptor : a descriptor that is used to refer to the connection in subsequent calls , nil for error
               err : error msg
*/
static int ltuxedo_tpconnect(lua_State *L)
{
	int ret=-1;
	char *svc=(char *)luaL_checkstring(L,1);
	LTUXEDO_BUFFER *send=NULL;
	if (!lua_isnil(L,2)) send=(LTUXEDO_BUFFER *)luaL_checkudata(L,2,LUAMETA_BUFFER);
	long flags=luaL_optinteger(L,3,TPSENDONLY);

	long ilen=0;
	char *idata=NULL;

	if (send)
	{
		ilen=send->raw_len;
		idata=send->ptr;
	}
	if ((ret=tpconnect(svc,idata,ilen,flags)) == -1)
	{
		set_errmsg_tp();
		lua_pushnil(L);
		lua_pushstring(L,errmsg);
		return 2;
	}
	lua_pushinteger(L,ret);
	return 1;
}

/* call tpsend
  return_event,err = tpsend(conn_descriptor ,send_buffer [,flag])
            IN:
   conn_descriptor : a descriptor created by tpconnect
       send_buffer : tuxedo buffer to send data , or nil
              flag : tpsend flag - TPRECVONLY TPNOBLOCK TPNOTIME TPSIGRSTRT

           OUT:
      return_event : return event type , nil for err , 0 for no event
	  					- TPEV_DISCONIMM TPEV_SVCERR TPEV_SVCFAIL
               err : error msg
*/
static int ltuxedo_tpsend(lua_State *L)
{
	int cd=luaL_checkinteger(L,1);
	LTUXEDO_BUFFER *send=NULL;
	if (!lua_isnil(L,2)) send=(LTUXEDO_BUFFER *)luaL_checkudata(L,2,LUAMETA_BUFFER);
	long flags=luaL_optinteger(L,3,0);

	long ilen=0,revent=0;
	char *idata=NULL;

	if (send)
	{
		ilen=send->raw_len;
		idata=send->ptr;
	}
	if ((tpsend(cd,idata,ilen,flags,&revent) == -1)&&(tperrno != TPEEVENT))
	{
		set_errmsg_tp();
		lua_pushnil(L);
		lua_pushstring(L,errmsg);
		return 2;
	}
	lua_pushinteger(L,revent);
	return 1;
}

/* call tprecv
  return_event,err = tprecv(conn_descriptor ,rcv_buffer [,flag])
            IN:
   conn_descriptor : a descriptor created by tpconnect
        rcv_buffer : tuxedo buffer to receive data
              flag : tprecv flag - TPNOCHANGE TPNOBLOCK TPNOTIME TPSIGRSTRT

           OUT:
      return_event : return event type , nil for err , 0 for no event
	  					- TPEV_DISCONIMM TPEV_SENDONLY TPEV_SVCERR TPEV_SVCFAIL TPEV_SVCSUCC 
               err : error msg
*/
static int ltuxedo_tprecv(lua_State *L)
{
	int cd=luaL_checkinteger(L,1);
	LTUXEDO_BUFFER *rcv=(LTUXEDO_BUFFER *)luaL_checkudata(L,2,LUAMETA_BUFFER);
	long flags=luaL_optinteger(L,3,0);

	long olen=0,revent=0;

	if ((tprecv(cd,&(rcv->ptr),&olen,flags,&revent) == -1)&&(tperrno != TPEEVENT))
	{
		set_errmsg_tp();
		lua_pushnil(L);
		lua_pushstring(L,errmsg);
		return 2;
	}
	if (olen != 0)
	{
		rcv->size=tptypes(rcv->ptr,rcv->type,rcv->sub_type);	/* rcv buffer may change */
		if (rcv->size == -1)
		{
			set_errmsg_tp();
			lua_pushnil(L);
			lua_pushstring(L,errmsg);
			return 2;
		}
		rcv->raw_len=olen;
	}

	lua_pushinteger(L,revent);
	return 1;
}

/* call tpdiscon
        result,err = tpdiscon(conn_descriptor)
            IN:
   conn_descriptor : a descriptor created by tpconnect

           OUT:
            result : true for succ , false for err
               err : error msg
*/
static int ltuxedo_tpdiscon(lua_State *L)
{
	int cd=luaL_checkinteger(L,1);

	if (tpdiscon(cd) == -1)
	{
		set_errmsg_tp();
		lua_pushboolean(L,0);
		lua_pushstring(L,errmsg);
		return 2;
	}
	lua_pushboolean(L,1);
	return 1;
}

/* call tpbegin
        result,err = tpbegin([timeout])
            IN:
           timeout : transaction should be allowed at least timeout seconds before timing out, default 0

           OUT:
            result : true for succ , false for err
               err : error msg
*/
static int ltuxedo_tpbegin(lua_State *L)
{
	int tmout=luaL_optinteger(L,1,0);

	if (tpbegin(tmout,0) == -1)
	{
		set_errmsg_tp();
		lua_pushboolean(L,0);
		lua_pushstring(L,errmsg);
		return 2;
	}
	lua_pushboolean(L,1);
	return 1;
}

/* call tpcommit
        result,err = tpcommit()
           OUT:
            result : true for succ , false for err
               err : error msg
*/
static int ltuxedo_tpcommit(lua_State *L)
{
	if (tpcommit(0) == -1)
	{
		set_errmsg_tp();
		lua_pushboolean(L,0);
		lua_pushstring(L,errmsg);
		return 2;
	}
	lua_pushboolean(L,1);
	return 1;
}

/* call tpabort
        result,err = tpabort()
           OUT:
            result : true for succ , false for err
               err : error msg
*/
static int ltuxedo_tpabort(lua_State *L)
{
	if (tpabort(0) == -1)
	{
		set_errmsg_tp();
		lua_pushboolean(L,0);
		lua_pushstring(L,errmsg);
		return 2;
	}
	lua_pushboolean(L,1);
	return 1;
}

/* call tpsuspend
        tranid,err = tpsuspend()
           OUT:
            tranid : the transaction identifier table , nil for err
               err : error msg
*/
static int ltuxedo_tpsuspend(lua_State *L)
{
	int i;
	TPTRANID tranid;

	memset(&tranid,0,sizeof(tranid));

	if (tpsuspend(&tranid,0) == -1)
	{
		set_errmsg_tp();
		lua_pushnil(L);
		lua_pushstring(L,errmsg);
		return 2;
	}
	lua_createtable(L,6,0);
	for(i=1;i<=6;i++)
	{
		lua_pushinteger(L,tranid.info[i-1]);
		lua_rawseti(L,-2,i);
	}
	return 1;
}

/* call tpresume
        result,err = tpresume(tranid)
           IN:
            tranid : the transaction identifier table returned from tpsuspend

           OUT:
            result : true for succ , false for err
               err : error msg
*/
static int ltuxedo_tpresume(lua_State *L)
{
	int i;
	TPTRANID tranid;

	memset(&tranid,0,sizeof(tranid));

	if (lua_type(L,1) != LUA_TTABLE) luaL_error(L,"transaction identifier isn't a table");
	for(i=1;i<=6;i++)
	{
		lua_rawgeti(L,1,i);
		tranid.info[i-1]=lua_tointeger(L,-1);
		lua_pop(L,1);
	}
	if (tpresume(&tranid,0) == -1)
	{
		set_errmsg_tp();
		lua_pushboolean(L,0);
		lua_pushstring(L,errmsg);
		return 2;
	}
	lua_pushboolean(L,1);
	return 1;
}

/* call tpgetlev
         level,err = tpgetlev()
           OUT:
             level : 0 - no transaction  1 - transaction is in progress  nil for err
               err : error msg
*/
static int ltuxedo_tpgetlev(lua_State *L)
{
	int ret;

	if ((ret=tpgetlev()) == -1)
	{
		set_errmsg_tp();
		lua_pushnil(L);
		lua_pushstring(L,errmsg);
		return 2;
	}
	lua_pushinteger(L,ret);
	return 1;
}

/* call tpscmt
     prev_ctrl,err = tpscmt(ctrl)
            IN:
              ctrl : TP_COMMIT_CONTROL value - TP_CMT_LOGGED TP_CMT_COMPLETE 

           OUT:
         prev_ctrl : previous TP_COMMIT_CONTROL value - TP_CMT_LOGGED TP_CMT_COMPLETE , nil for err
               err : error msg
*/
static int ltuxedo_tpscmt(lua_State *L)
{
	int ret;
	int ctrl=lua_tointeger(L,1);

	if ((ret=tpscmt(ctrl)) == -1)
	{
		set_errmsg_tp();
		lua_pushnil(L);
		lua_pushstring(L,errmsg);
		return 2;
	}
	lua_pushinteger(L,ret);
	return 1;
}

static int get_tpqctl(lua_State *L,int tab_idx,TPQCTL *ctl)
{
	static struct {
		char *name;
		char type;
		size_t offset;
		size_t length;
	} flds[] = {
		{ "flags",'L',offsetof(TPQCTL,flags),0 },
		{ "deq_time",'L',offsetof(TPQCTL,deq_time),0 },
		{ "priority",'L',offsetof(TPQCTL,priority),0 },
		{ "exp_time",'L',offsetof(TPQCTL,exp_time),0 },
		{ "delivery_qos",'L',offsetof(TPQCTL,delivery_qos),0 },
		{ "urcode",'L',offsetof(TPQCTL,urcode),0 },
		{ "msgid",'S',offsetof(TPQCTL,msgid),sizeof(ctl->msgid) },
		{ "corrid",'S',offsetof(TPQCTL,corrid),sizeof(ctl->corrid) },
		{ "replyqueue",'S',offsetof(TPQCTL,replyqueue),sizeof(ctl->replyqueue)},
		{ "failurequeue",'S',offsetof(TPQCTL,failurequeue),sizeof(ctl->failurequeue)},
		{ NULL,0,0,0 }
	};
	int i;
	int i_val;
	long l_val;
	double d_val;
	char *p=(char *)ctl;

	for(i=0;flds[i].name;i++)
	{
		lua_pushstring(L,flds[i].name);
		if (lua_rawget(L,tab_idx) != LUA_TNIL)
		{
			switch(flds[i].type)
			{
				case 'I':
					i_val=lua_tointeger(L,-1);
					memcpy(p+flds[i].offset,&i_val,sizeof(int));
					break;
				case 'L':
					l_val=lua_tointeger(L,-1);
					memcpy(p+flds[i].offset,&l_val,sizeof(long));
					break;
				case 'D':
					d_val=lua_tonumber(L,-1);
					memcpy(p+flds[i].offset,&d_val,sizeof(double));
					break;
				case 'S':
					strncpy(p+flds[i].offset,lua_tostring(L,-1),flds[i].length-1);
					break;
				default:
					luaL_error(L,"TPQCTL structure def error");
			}
		}
		lua_pop(L,1);
	}

	return 0;
}

/* call tpenqueue
    ctrl_table,err,diagnostic = tpenqueue(qspace ,qname ,message_buffer [,ctrl_table] [,flag])
            IN:
            qspace : queue space name
             qname : queue name
    message_buffer : enqueue data in tuxedo buffer
        ctrl_table : TPQCTL structure key/value table
              flag : tpenqueue flag - TPNOTRAN TPNOBLOCK TPNOTIME TPSIGRSTRT

           OUT:
        ctrl_table : TPQCTL structure key/value table , nil for err
               err : error msg
        diagnostic : diagnostic value in TPEDIAGNOSTIC err
*/
static int ltuxedo_tpenqueue(lua_State *L)
{
	char *qspace=(char *)lua_tostring(L,1);
	char *qname=(char *)lua_tostring(L,2);
	LTUXEDO_BUFFER *message=(LTUXEDO_BUFFER *)luaL_checkudata(L,3,LUAMETA_BUFFER);
	TPQCTL ctl;
	long top=4,flags;

	memset(&ctl,0,sizeof(ctl));

	if (lua_istable(L,top))
	{
		get_tpqctl(L,top,&ctl);
		top++;
	}
	flags=luaL_optinteger(L,top,0);

	if (tpenqueue(qspace,qname,&ctl,message->ptr,message->raw_len,flags) == -1)
	{
		set_errmsg_tp();
		lua_pushnil(L);
		lua_pushstring(L,errmsg);
		if (tperrno == TPEDIAGNOSTIC)
		{
			lua_pushinteger(L,ctl.diagnostic);
			return 3;
		}
		return 2;
	}

	lua_createtable(L,0,4);
	lua_pushliteral(L,"flags");
	lua_pushinteger(L,ctl.flags);
	lua_rawset(L,-3);
	lua_pushliteral(L,"msgid");
	lua_pushstring(L,ctl.msgid);
	lua_rawset(L,-3);
	lua_pushliteral(L,"diagnostic");
	lua_pushinteger(L,ctl.diagnostic);
	lua_rawset(L,-3);
	return 1;
}

/* call tpdequeue
    ctrl_table,err,diagnostic = tpdequeue(qspace ,qname ,message_buffer [,ctrl_table] [,flag])
            IN:
            qspace : queue space name
             qname : queue name
    message_buffer : dequeue data in tuxedo buffer
        ctrl_table : TPQCTL structure key/value table
              flag : tpenqueue flag - TPNOTRAN TPNOBLOCK TPNOTIME TPNOCHANGE TPSIGRSTRT

           OUT:
        ctrl_table : TPQCTL structure key/value table , nil for err
               err : error msg
       diagnosticr : diagnostic value in TPEDIAGNOSTIC err
*/
static int ltuxedo_tpdequeue(lua_State *L)
{
	char *qspace=(char *)lua_tostring(L,1);
	char *qname=(char *)lua_tostring(L,2);
	LTUXEDO_BUFFER *message=(LTUXEDO_BUFFER *)luaL_checkudata(L,3,LUAMETA_BUFFER);
	TPQCTL ctl;
	long olen=0,top=4,flags;

	memset(&ctl,0,sizeof(ctl));

	if (lua_istable(L,top))
	{
		get_tpqctl(L,top,&ctl);
		top++;
	}
	flags=luaL_optinteger(L,top,0);

	if (tpdequeue(qspace,qname,&ctl,&(message->ptr),&olen,flags) == -1)
	{
		set_errmsg_tp();
		lua_pushnil(L);
		lua_pushstring(L,errmsg);
		if (tperrno == TPEDIAGNOSTIC)
		{
			lua_pushinteger(L,ctl.diagnostic);
			return 3;
		}
		return 2;
	}
	if (olen != 0)
	{
		message->size=tptypes(message->ptr,message->type,message->sub_type);	/* message buffer may change */
		if (message->size == -1)
		{
			set_errmsg_tp();
			lua_pushnil(L);
			lua_pushstring(L,errmsg);
			return 2;
		}
		message->raw_len=olen;
	}

	lua_createtable(L,0,12);
	lua_pushliteral(L,"flags");
	lua_pushinteger(L,ctl.flags);
	lua_rawset(L,-3);
	lua_pushliteral(L,"priority");
	lua_pushinteger(L,ctl.priority);
	lua_rawset(L,-3);
	lua_pushliteral(L,"msgid");
	lua_pushstring(L,ctl.msgid);
	lua_rawset(L,-3);
	lua_pushliteral(L,"corrid");
	lua_pushstring(L,ctl.corrid);
	lua_rawset(L,-3);
	lua_pushliteral(L,"delivery_qos");
	lua_pushinteger(L,ctl.delivery_qos);
	lua_rawset(L,-3);
	lua_pushliteral(L,"reply_qos");
	lua_pushinteger(L,ctl.reply_qos);
	lua_rawset(L,-3);
	lua_pushliteral(L,"replyqueue");
	lua_pushstring(L,ctl.replyqueue);
	lua_rawset(L,-3);
	lua_pushliteral(L,"failurequeue");
	lua_pushstring(L,ctl.failurequeue);
	lua_rawset(L,-3);
	lua_pushliteral(L,"diagnostic");
	lua_pushinteger(L,ctl.diagnostic);
	lua_rawset(L,-3);
	lua_pushliteral(L,"appkey");
	lua_pushinteger(L,ctl.appkey);
	lua_rawset(L,-3);
	lua_pushliteral(L,"urcode");
	lua_pushinteger(L,ctl.urcode);
	lua_rawset(L,-3);
	lua_pushliteral(L,"cltid");
	lua_createtable(L,4,0);
	lua_pushinteger(L,ctl.cltid.clientdata[0]);
	lua_rawseti(L,-2,1);
	lua_pushinteger(L,ctl.cltid.clientdata[1]);
	lua_rawseti(L,-2,2);
	lua_pushinteger(L,ctl.cltid.clientdata[2]);
	lua_rawseti(L,-2,3);
	lua_pushinteger(L,ctl.cltid.clientdata[3]);
	lua_rawseti(L,-2,4);
	lua_rawset(L,-3);

	return 1;
}

/* return tpurcode
       app_ret_code = tpurcode()
           OUT:
      app_ret_code : tuxedo service application defined value that was sent as part of tpreturn()
*/
static int ltuxedo_tpurcode(lua_State *L)
{
	lua_pushinteger(L,tpurcode);
	return 1;
}

/* return tperrno
      errno,errmsg = tperrno()
           OUT:
             errno : tperrno 
            errmsg : tpstrsrror()
*/
static int ltuxedo_tperrno(lua_State *L)
{
	lua_pushinteger(L,tperrno);
	lua_pushstring(L,tpstrerror(tperrno));
	return 2;
}

/* call tuxputenv
        result,err = tpsetenv(env_string)
            IN:
        env_string : string of the form "name=value"

           OUT:
            result : true for success , false for err
               err : error msg
*/
static int ltuxedo_tpsetenv(lua_State *L)
{
	char *env=(char *)lua_tostring(L,1);

	if (tuxputenv(env) != 0)
	{
		lua_pushboolean(L,0);
		lua_pushliteral(L,"cannot obtain enough space");
		return 2;
	}

	lua_pushboolean(L,1);
	return 1;
}

#ifndef SERVER

/* call TPINIT with auth param
       result ,err = tpinit(usrname,cltname,passwd,grpname,usr_data,flag)
            IN:
           usrname : tuxedo INIT param
           cltname : tuxedo INIT param
            passwd : tuxedo INIT param
           grpname : tuxedo INIT param
          usr_data : tuxedo INIT param
              flag : TPINIT flag - TPU_SIG TPU_DIP TPU_IGN TPSA_FASTPATH TPSA_PROTECTED

           OUT:
            result : true for success , false for err
               err : error msg
*/
static int ltuxedo_init_auth(lua_State *L)
{
	int usr_data_len;
	TPINIT *tpinitbuf=NULL;
	char *usrname=(char *)luaL_optstring(L,1,"");
	char *cltname=(char *)luaL_optstring(L,2,"");
	char *passwd=(char *)luaL_optstring(L,3,"");
	char *grpname=(char *)luaL_optstring(L,4,"");
	char *usr_data=(char *)luaL_optstring(L,5,"");

	tpterm();

	usr_data_len=strlen(usr_data);
	tpinitbuf=(TPINIT *)tpalloc("TPINIT",NULL,TPINITNEED(usr_data_len+1));
	if (tpinitbuf == (TPINIT *)NULL)
	{
		set_errmsg_tp();
		lua_pushboolean(L,0);
		lua_pushstring(L,errmsg);
		return 2;
	}
	strncpy(tpinitbuf->usrname,usrname,sizeof(tpinitbuf->usrname)-1);
	strncpy(tpinitbuf->cltname,cltname,sizeof(tpinitbuf->cltname)-1);
	strncpy(tpinitbuf->passwd,passwd,sizeof(tpinitbuf->passwd)-1);
	strncpy(tpinitbuf->grpname,grpname,sizeof(tpinitbuf->grpname)-1);
	strncpy((char *)&(tpinitbuf->data),usr_data,usr_data_len);
	tpinitbuf->datalen=usr_data_len+1;
	if (tpinit(tpinitbuf) == -1)
	{
		set_errmsg_tp();
		lua_pushboolean(L,0);
		lua_pushstring(L,errmsg);
		tpfree((char *)tpinitbuf);
		return 2;
	}

	lua_pushboolean(L,1);
	return 1;
}

#endif

#ifdef SERVER

/* -----------------------SERVER API-----------------------*/

typedef struct SvcList {
	struct SvcList *next;
	int svc_func_ref;
	char svc_name[XATMI_SERVICE_NAME_LENGTH];
}SvcList,*SvcListPtr;
static SvcListPtr G_SvcHead;
static int G_ServerRunning;
static lua_State *G_L;

static int G_Forward;
static char G_ForwardService[XATMI_SERVICE_NAME_LENGTH];

void tuxedo_svc_dispatch(TPSVCINFO *rqst)
{
	int ret;
	long out_len,rcode;
	char *svc_name,*out_data=NULL;
	SvcListPtr curr;
	lua_State *L;
	LTUXEDO_BUFFER in_buf,*lua_in_buf,*lua_out_buf;

	memset(&in_buf,0,sizeof(in_buf));

	svc_name=rqst->name;
	for(curr=G_SvcHead;curr;curr=curr->next)
	{
		if (strcmp(curr->svc_name,svc_name) == 0) break;
	}
	if (!curr)
	{
		userlog("LTUXEDO err:can't find svc in list - %s",svc_name);
		tpreturn(TPFAIL,0,NULL,0L,0);
	}
	in_buf.ptr=rqst->data;
	if (in_buf.ptr != NULL)
	{
		if ((in_buf.size=tptypes(in_buf.ptr,in_buf.type,in_buf.sub_type)) == -1)
		{
			userlog("LTUXEDO err:tptypes fail - %s",tpstrerror(tperrno));
			tpreturn(TPFAIL,0,NULL,0L,0);
		}
		in_buf.raw_len=rqst->len;
	}

	G_Forward=0;
	memset(G_ForwardService,0,sizeof(G_ForwardService));
	L=G_L;
	lua_rawgeti(L,LUA_REGISTRYINDEX,curr->svc_func_ref);
	if (in_buf.ptr != NULL)
	{
		lua_in_buf=(LTUXEDO_BUFFER *)lua_newuserdata(L,sizeof(LTUXEDO_BUFFER));
		luaL_getmetatable(L,LUAMETA_BUFFER);
		lua_setmetatable(L,-2);
		memcpy(lua_in_buf,&in_buf,sizeof(LTUXEDO_BUFFER));
	}
	else
	{
		lua_pushnil(L);
	}
	lua_pushinteger(L,rqst->cd);
	ret=lua_pcall(L,2,2,0);
	lua_gc(L,LUA_GCCOLLECT,0);
	if (ret != LUA_OK)
	{
		userlog("LTUXEDO err:lua function execute fail - %d %s",ret,lua_tostring(L,-1));
		lua_pop(L,1);
		tpreturn(TPFAIL,0,NULL,0L,0);
	}

	rcode=luaL_optinteger(L,-1,0);
	if (lua_type(L,-2) == LUA_TNIL)
	{
		lua_pop(L,2);
		tpreturn(TPFAIL,rcode,NULL,0L,0);
	}
	else if ((lua_type(L,-2) == LUA_TUSERDATA)&&((lua_out_buf=luaL_testudata(L,-2,LUAMETA_BUFFER)) != NULL))
	{
		out_len=lua_out_buf->raw_len;
		out_data=lua_out_buf->ptr;
		lua_out_buf->ptr=NULL;
	}
	else
	{
		char *lua_out;
		lua_out=(char *)lua_tolstring(L,-2,(unsigned long *)&out_len);
		if (out_len == 0)
		{
			out_data=NULL;
		}
		else
		{
			if ((out_data=tpalloc("CARRAY",NULL,out_len)) == NULL)
			{
				lua_pop(L,2);
				userlog("LTUXEDO err:tpalloc fail - %s",tpstrerror(tperrno));
				tpreturn(TPFAIL,0,NULL,0L,0);
			}
			memcpy(out_data,lua_out,out_len);
		}
	}
	lua_pop(L,2);
	lua_gc(L,LUA_GCCOLLECT,0);

	if (G_Forward)
	{
		tpforward(G_ForwardService,out_data,out_len,0);
	}

	tpreturn(TPSUCCESS,rcode,out_data,out_len,0);
}

static inline int add_service_list_node(lua_State *L,char *svc_name,int func_ref)
{
	int ref=LUA_NOREF;
	SvcListPtr *p_curr=&G_SvcHead;

	lua_pushvalue(L,func_ref);
	ref=luaL_ref(L,LUA_REGISTRYINDEX);
	if ((ref == LUA_NOREF)||(ref == LUA_REFNIL))
	{
		strcpy(errmsg,"Reference lua func object fail");
		return -1;
	}
	for(;*p_curr;p_curr=&((*p_curr)->next));
	if ((*p_curr=malloc(sizeof(SvcList))) == NULL)
	{
		strcpy(errmsg,"Svc list node malloc fail");
		return -1;
	}
	(*p_curr)->next=NULL;
	(*p_curr)->svc_func_ref=ref;
	strncpy((*p_curr)->svc_name,svc_name,sizeof((*p_curr)->svc_name)-1);

	return 0;
}

static inline void del_service_list_node(lua_State *L,char *svc_name)
{
	SvcListPtr *p_curr=&G_SvcHead,t;

	for(;*p_curr;p_curr=&((*p_curr)->next))
	{
		if (strncmp(svc_name,(*p_curr)->svc_name,sizeof((*p_curr)->svc_name)-1) == 0) break;
	}
	if (*p_curr == NULL)
	{
		userlog("LTUXEDO fail:%s service not in list");
		return;
	}
	luaL_unref(L,LUA_REGISTRYINDEX,(*p_curr)->svc_func_ref);
	t=*p_curr;
	*p_curr=(*p_curr)->next;
	free(t);

	return;
}

/* call TPADVERTISE to advertise servce
       result ,err = tpadvertise(svcname,func)
            IN:
           svcname : service name
              func : lua function
			  			func=function(tuxedo_buffer,conversation descriptor)
							...
							return tuxedo_buffer|string|nil [,rcode]
						end
                     return string = '' means no return data
                     return nil means error, return TPFAIL
           OUT:
            result : true for success , false for err
               err : error msg
*/
static int ltuxedo_tpadvertise(lua_State *L)
{
	int ret=-1;
	char *svc_name=(char *)lua_tostring(L,1);

	if (G_ServerRunning == 0)
	{
		luaL_error(L,"Use tpadvertise() after mainloop()");
	}
	if (lua_type(L,2) != LUA_TFUNCTION)
	{
		luaL_error(L,"Paramater 2 isn't a function");
	}
	if (L != G_L)
	{
		luaL_error(L,"Can't set service function in coroutine");
	}
	do
	{
        if (add_service_list_node(L,svc_name,2) != 0) break;
		if (tpadvertise(svc_name,tuxedo_svc_dispatch) == -1)
		{
			set_errmsg_tp();
			break;
		}
		ret=0;
	}
	while(0);
	if (ret != 0)
	{
		del_service_list_node(L,svc_name);
		lua_pushboolean(L,0);
		lua_pushstring(L,errmsg);
		return 2;
	}

	lua_pushboolean(L,1);
	return 1;
}

/* call TPUNADVERTISE to unadvertise servce
       result ,err = tpunadvertise(svcname)
            IN:
           svcname : service name
           OUT:
            result : true for success , false for err
               err : error msg
*/
static int ltuxedo_tpunadvertise(lua_State *L)
{
	char *svc_name=(char *)lua_tostring(L,1);

	if (G_ServerRunning == 0)
	{
		luaL_error(L,"Use tpadvertise() after mainloop()");
	}
	if (tpunadvertise(svc_name) == -1)
	{
		set_errmsg_tp();
		lua_pushboolean(L,0);
		lua_pushstring(L,errmsg);
		return 2;
	}
	del_service_list_node(L,svc_name);

	lua_pushboolean(L,1);
	return 1;
}

/* set tpforward service after self return
                 tpforward(svcname)
            IN:
           svcname : service name
*/
static int ltuxedo_tpforward(lua_State *L)
{
	char *svc_name=(char *)lua_tostring(L,1);

	if (G_ServerRunning == 0)
	{
		luaL_error(L,"Use tpforward() in tuxedo service function");
	}
	G_Forward=1;
	strncpy(G_ForwardService,svc_name,sizeof(G_ForwardService)-1);

	return 0;
}

/* add initial service before mainloop()
       result ,err = init_service(svcname,func)
            IN:
           svcname : service name
              func : lua function
			  			func=function(tuxedo_buffer,conversation descriptor)
							...
							return tuxedo_buffer|string|nil [,rcode]
						end
                     return string = '' means no return data
                     return nil means error, return TPFAIL
           OUT:
            result : true for success , false for err
               err : error msg
*/
static int init_service(lua_State *L)
{
	char *svc_name=(char *)lua_tostring(L,1);

	if (G_ServerRunning == 1)
	{
		luaL_error(L,"Use init_service() before mainloop()");
	}
	if (lua_type(L,2) != LUA_TFUNCTION)
	{
		luaL_error(L,"Paramater 2 isn't a function");
	}
	if (L != G_L)
	{
		luaL_error(L,"Can't set service function in coroutine");
	}
	if (add_service_list_node(L,svc_name,2) != 0)
	{
		lua_pushboolean(L,0);
		lua_pushstring(L,errmsg);
		return 2;
	}

	lua_pushboolean(L,1);
	return 1;
}

int tpsvrinit(int argc, char *argv[])
{
	userlog("LTUXEDO Server start , begin advertise initial service ...");

	SvcListPtr curr=G_SvcHead;

	for(;curr;curr=curr->next)
	{
		if (tpadvertise(curr->svc_name,tuxedo_svc_dispatch) == -1)
		{
			set_errmsg_tp();
			userlog("LTUXEDO err:service %s advertise fail - %s",curr->svc_name,errmsg);
		}
	}
	G_ServerRunning=1;

	userlog("LTUXDEO Server start finished");

	tpopen();

	return 0;
}

void tpsvrdone(void)
{
	userlog("LTUXEDO Server stop ...");
	G_ServerRunning=0;

	tpclose();

	return;
}

#ifndef _TMDLLIMPORT
#define _TMDLLIMPORT
#endif
_TMDLLIMPORT extern struct xa_switch_t tmnull_switch;
static struct xa_switch_t *xa_switch=&tmnull_switch;

static struct tmdsptchtbl_t _tmdsptchtbl[] = {
	{ "","tuxedo_svc_dispatch",(void (*) _((TPSVCINFO *)))tuxedo_svc_dispatch,0,0 },  
	{ NULL,NULL,NULL,0,0 }
};
extern int _tmrunserver _((int));

static struct tmsvrargs_t tmsvrargs = {
	NULL,
	&_tmdsptchtbl[0],
	0,
	tpsvrinit,
	tpsvrdone,
	_tmrunserver,	/* PRIVATE  */
	NULL,			/* RESERVED */
	NULL,			/* RESERVED */
	NULL,			/* RESERVED */
	NULL,			/* RESERVED */
	tpsvrthrinit,
	tpsvrthrdone
};

/* set xa switch 
          init_xa_switch(xa_switch_lib_file,xa_switch_name)
            IN:
      xa_switch_lib_file : library name that provide xa functions   E.G '/usr/informix/lib/esql/libifxa.so'
          xa_switch_name : xa switch structure name                 E.G 'infx_xa_switch'
*/
static int ltuxedo_init_xa_switch(lua_State *L)
{
	void *lib_handle=NULL;
	char *err=NULL;
	char *fn=(char *)luaL_optstring(L,1,"");
	char *xa_name=(char *)lua_tostring(L,2);

	lib_handle=dlopen(fn,RTLD_NOW|RTLD_GLOBAL);
	if (lib_handle == NULL)
	{
		lua_pushnil(L);
		lua_pushfstring(L,"dlopen err %s\n",dlerror());
		return 2;
	}
	xa_switch=(struct xa_switch_t *)dlsym(lib_handle,xa_name);
	if ((err=dlerror()) != NULL)
	{
		lua_pushnil(L);
		lua_pushfstring(L,"dlsym err %s\n",err);
		dlclose(lib_handle);
		return 2;
	}

	lua_pushboolean(L,1);
	return 1;
}

/* start tuxedo control process
                  mainloop([argument])
            IN:
           argument : argument line send to tuxedo start func
		               E.G '-i svc_no -g grp_no -u pmid'
*/
static int ltuxedo_mainloop(lua_State *L)
{
	int ret,i;
	static int argc=1;
	static char *argv[PARA_MAX_NUM],para_line[1024];

	memset(para_line,0,sizeof(para_line));

	argv[0]="ltuxedo";
	for(i=1;i<PARA_MAX_NUM;i++) argv[i]=NULL;

	if (lua_gettop(L) > 0)
	{
		char *p;
		strncpy(para_line,lua_tostring(L,1),sizeof(para_line)-1);
		p=strtok(para_line," ");
		while ((p != NULL)&&(argc < PARA_MAX_NUM-1))
		{
			argv[argc]=p;
			argc++;
			p=strtok(NULL," ");
		}
		argv[argc]=NULL;
	}

	tmsvrargs.xa_switch=xa_switch;
	if ((ret=_tmstartserver(argc,&argv[0],&tmsvrargs)) != 0)
	{
		userlog("LTUXEDO err:start server fail - %d",ret);
		set_errmsg_tp();
		lua_pushboolean(L,0);
		lua_pushstring(L,errmsg);
		return 2;
	}
	return 0;
}

#endif


/* -----------------------MISC/TEST FUNC-----------------------*/

static int buffer_export(lua_State *L)
{
	LTUXEDO_BUFFER *p_buf=(LTUXEDO_BUFFER *)luaL_checkudata(L,1,LUAMETA_BUFFER);
	long flag=luaL_optinteger(L,2,TPEX_STRING);
	char *value=NULL;
	long len=p_buf->size*4;

	if ((value=malloc(len)) == NULL)
	{
		lua_pushnil(L);
		lua_pushliteral(L,"alloc fail");
		return 2;
	}

	if (tpexport(p_buf->ptr,p_buf->raw_len,value,&len,flag) == -1)
	{
		set_errmsg_tp();
		lua_pushnil(L);
		lua_pushstring(L,errmsg);
		return 2;
	}

	lua_pushlstring(L,value,len);
	free(value);
	return 1;
}

static int buffer_import(lua_State *L)
{
	LTUXEDO_BUFFER *p_buf=(LTUXEDO_BUFFER *)luaL_checkudata(L,1,LUAMETA_BUFFER);
	size_t len;
	char *str=(char *)lua_tolstring(L,2,&len);
	long flag=luaL_optinteger(L,3,TPEX_STRING);

	if (tpimport(str,len,&p_buf->ptr,&p_buf->raw_len,flag) == -1)
	{
		set_errmsg_tp();
		lua_pushnil(L);
		lua_pushstring(L,errmsg);
		return 2;
	}

	lua_pushvalue(L,1);
	return 1;
}

/* -----------------------INITIATION-----------------------*/

static void set_tuxedo_constant(lua_State *L)
{
	struct {
		char *const_name;
		int const_int;
	} static const_elem[] = {

		{ "TPEABORT",1 },
		{ "TPEBADDESC",2 },
		{ "TPEBLOCK",3 },
		{ "TPEINVAL",4 },
		{ "TPELIMIT",5 },
		{ "TPENOENT",6 },
		{ "TPEOS",7 },
		{ "TPEPERM",8 },
		{ "TPEPROTO",9 },
		{ "TPESVCERR",10 },
		{ "TPESVCFAIL",11 },
		{ "TPESYSTEM",12 },
		{ "TPETIME",13 },
		{ "TPETRAN",14 },
		{ "TPGOTSIG",15 },
		{ "TPERMERR",16 },
		{ "TPEITYPE",17 },
		{ "TPEOTYPE",18 },
		{ "TPERELEASE",19 },
		{ "TPEHAZARD",20 },
		{ "TPEHEURISTIC",21 },
		{ "TPEEVENT",22 },
		{ "TPEMATCH",23 },
		{ "TPEDIAGNOSTIC",24 },
		{ "TPEMIB",25 },

		{ "TPNOBLOCK",0x00000001 },
		{ "TPSIGRSTRT",0x00000002 },
		{ "TPNOREPLY",0x00000004 },
		{ "TPNOTRAN",0x00000008 },
		{ "TPTRAN",0x00000010 },
		{ "TPNOTIME",0x00000020 },
		{ "TPABSOLUTE",0x00000040 },
		{ "TPGETANY",0x00000080 },
		{ "TPNOCHANGE",0x00000100 },
		{ "TPCONV",0x00000400 },
		{ "TPSENDONLY",0x00000800 },
		{ "TPRECVONLY",0x00001000 },

		{ "TPU_SIG",0x00000001 },
		{ "TPU_DIP",0x00000002 },
		{ "TPU_IGN",0x00000004 },
		{ "TPSA_FASTPATH",0x00000008 },
		{ "TPSA_PROTECTED",0x00000010 },
		{ "TPMULTICONTEXTS",0x00000020 },
		{ "TPU_THREAD",0x00000040 },

		{ "TPEV_NOEVENT",0x0000 },
		{ "TPEV_DISCONIMM",0x0001 },
		{ "TPEV_SVCERR",0x0002 },
		{ "TPEV_SVCFAIL",0x0004 },
		{ "TPEV_SVCSUCC",0x0008 },
		{ "TPEV_SENDONLY",0x0020 },

		{ "TPEVSERVICE",0x00000001 },
		{ "TPEVQUEUE",0x00000002 },
		{ "TPEVTRAN",0x00000004 },
		{ "TPEVPERSIST",0x00000008 },

		{ "TPNOFLAGS",0x00000 },
		{ "TPQCORRID",0x00001 },
		{ "TPQFAILUREQ",0x00002 },
		{ "TPQBEFOREMSGID",0x00004 },
		{ "TPQGETBYMSGIDOLD",0x00008 },
		{ "TPQMSGID",0x00010 },
		{ "TPQPRIORITY",0x00020 },
		{ "TPQTOP",0x00040 },
		{ "TPQWAIT",0x00080 },
		{ "TPQREPLYQ",0x00100 },
		{ "TPQTIME_ABS",0x00200 },
		{ "TPQTIME_REL",0x00400 },
		{ "TPQGETBYCORRIDOLD",0x00800 },
		{ "TPQPEEK",0x01000 },
		{ "TPQDELIVERYQOS",0x02000 },
		{ "TPQREPLYQOS",0x04000 },
		{ "TPQEXPTIME_ABS",0x08000 },
		{ "TPQEXPTIME_REL",0x10000 },
		{ "TPQEXPTIME_NONE",0x20000 },
		{ "TPQGETBYMSGID",0x40008 },
		{ "TPQGETBYCORRID",0x80800 },

		{ "TPQQOSDEFAULTPERSIST",0x00001 },
		{ "TPQQOSPERSISTENT",0x00002 },
		{ "TPQQOSNONPERSISTENT",0x00004 },

		{ "QMEINVAL",-1 },
		{ "QMEBADRMID",-2 },
		{ "QMENOTOPEN",-3 },
		{ "QMETRAN",-4 },
		{ "QMEBADMSGID",-5 },
		{ "QMESYSTEM",-6 },
		{ "QMEOS",-7 },
		{ "QMEABORTED",-8 },
		{ "QMENOTA",-8 },
		{ "QMEPROTO",-9 },
		{ "QMEBADQUEUE",-10 },
		{ "QMENOMSG",-11 },
		{ "QMEINUSE",-12 },
		{ "QMENOSPACE",-13 },
		{ "QMERELEASE",-14 },
		{ "QMEINVHANDLE",-15 },
		{ "QMESHARE",-16 },
 
		{ "TP_CMT_LOGGED",0x01 },
		{ "TP_CMT_COMPLETE",0x02 },

		{ "FLD_SHORT",0 },
		{ "FLD_LONG",1 },
		{ "FLD_CHAR",2 },
		{ "FLD_FLOAT",3 },
		{ "FLD_DOUBLE",4 },
		{ "FLD_STRING",5 },
		{ "FLD_CARRAY",6 },
		{ "FLD_INT",7 },
		{ "FLD_DECIMAL",8 },
		{ "FLD_PTR",9 },
		{ "FLD_FML32",10 },
		{ "FLD_VIEW32",11 },
		{ "FLD_MBSTRING",12 },
		{ "FLD_FML",13 },

		{ NULL,0 }
	};
	int i;

	for(i=0;const_elem[i].const_name;i++)
	{
		lua_pushstring(L,const_elem[i].const_name);
		lua_pushinteger(L,const_elem[i].const_int);
		lua_rawset(L,-3);
	}
}

/* initialize library */
#ifdef CLI_WORKSTATION
	int luaopen_ltuxedo_ws(lua_State *L)
#else
	#ifdef CLI_NATIVE
		int luaopen_ltuxedo_nc(lua_State *L)
	#else
		#ifdef SERVER
			int luaopen_ltuxedo_sv(lua_State *L)
		#else
			int luaopen_ltuxedo(lua_State *L)
		#endif
	#endif
#endif
{
	struct luaL_Reg ltuxedo_methods[]=
	{
		{"new_buffer",ltuxedo_new_buffer},
		{"fneeded",ltuxedo_fml_needed},
		{"fld_id",ltuxedo_fml_id},
		{"fld_type",ltuxedo_fml_type},
		{"fld_name",ltuxedo_fml_name},
		{"fld_no",ltuxedo_fml_no},
		{"mkfld_id",ltuxedo_fml_mkfldid},
		{"tpcall",ltuxedo_tpcall},
		{"tpacall",ltuxedo_tpacall},
		{"tpgetreply",ltuxedo_tpgetreply},
		{"tpcancel",ltuxedo_tpcancel},
		{"tpconnect",ltuxedo_tpconnect},
		{"tpsend",ltuxedo_tpsend},
		{"tprecv",ltuxedo_tprecv},
		{"tpbegin",ltuxedo_tpbegin},
		{"tpcommit",ltuxedo_tpcommit},
		{"tpabort",ltuxedo_tpabort},
		{"tpsuspend",ltuxedo_tpsuspend},
		{"tpresume",ltuxedo_tpresume},
		{"tpgetlev",ltuxedo_tpgetlev},
		{"tpscmt",ltuxedo_tpscmt},
		{"tpenqueue",ltuxedo_tpenqueue},
		{"tpdequeue",ltuxedo_tpdequeue},
		{"tpdiscon",ltuxedo_tpdiscon},
		{"tpurcode",ltuxedo_tpurcode},
		{"tperrno",ltuxedo_tperrno},
		{"ferror",ltuxedo_ferror},
		{"tpsetenv",ltuxedo_tpsetenv},
#ifndef SERVER
		{"tpinit",ltuxedo_init_auth},
#endif
#ifdef SERVER
		/*------SERVER func------*/
		{"tpadvertise",ltuxedo_tpadvertise},
		{"tpunadvertise",ltuxedo_tpunadvertise},
		{"tpforward",ltuxedo_tpforward},
		{"init_service",init_service},
		{"init_xa_switch",ltuxedo_init_xa_switch},
		{"mainloop",ltuxedo_mainloop},
#endif
		{NULL, NULL},
	};

	struct luaL_Reg lbuffer_methods[]=
	{
		{"__tostring",buffer_tostring},
		{"__len",buffer_len},
		{"__gc",buffer_gc},
		{"realloc",buffer_realloc},
		{"free",buffer_gc},
		{"get_raw",buffer_get_raw},
		{"set_raw",buffer_set_raw},
		{"size",buffer_sizeof},
		/*------FML32 func------*/
		{"finit",buffer_fml_init},
		{"fadd",buffer_fml_add},
		{"fchg",buffer_fml_chg},
		{"fget",buffer_fml_get},
		{"fdel",buffer_fml_del},
		{"fdel_all",buffer_fml_del_all},
		{"fcopy_from",buffer_fml_copy_from},
		{"fjoin_from",buffer_fml_join_from},
		{"foccur",buffer_fml_occur},
		{"ffindocc",buffer_fml_findocc},
		{"ffield_iterator",buffer_fml_get_field_iter},
		/*--------misc\test\-------*/
		{"buff_export",buffer_export},
		{"buff_import",buffer_import},
		{NULL, NULL},
	};

#ifndef SERVER
#ifdef CLI_WORKSTATION
	/* in workstation mode ,if WSNADDR setted , call tpinit automatically , or use tpsetenv+tpinit manually */
	if (tuxgetenv("WSNADDR"))
	{
#endif
		/* if no authentication is required , call tpinit automatically , or use tpinit manually */
		if (tpchkauth() == TPNOAUTH)
		{
			if (tpinit(NULL) == -1)
			{
				set_errmsg_tp();
				return luaL_error(L,errmsg);
			}
		}
#ifdef CLI_WORKSTATION
	}
#endif
#endif

#ifdef SERVER
	G_SvcHead=NULL;
	G_ServerRunning=0;
	G_L=L;
#endif

	lua_createtable(L,0,200);
	luaL_setfuncs(L,ltuxedo_methods,0);
	lua_pushliteral(L,"_COPYRIGHT");
	lua_pushliteral(L,"Copyright (C) 2016.7");
	lua_rawset(L,-3);
	lua_pushliteral(L,"_DESCRIPTION");
	lua_pushliteral(L,"Lua Tuxedo Simple Extension");
	lua_rawset(L,-3);

	set_tuxedo_constant(L);

	/* set buffer user data metatable */
	luaL_newmetatable(L,LUAMETA_BUFFER);
	luaL_setfuncs(L,lbuffer_methods,0);
	lua_pushliteral(L,"__index");
	lua_pushvalue(L,-2);
	lua_rawset(L,-3);
	lua_pushliteral(L,"__metatable");
	lua_pushliteral(L,"you're not allowed to get this metatable");
	lua_rawset(L, -3);
	lua_pop(L,1);

	return 1;
}
