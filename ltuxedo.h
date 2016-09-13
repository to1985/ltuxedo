#ifndef _LUA_TUXEDO_
#define _LUA_TUXEDO_

#include "lua.h"
#include "lauxlib.h"

#if !defined LUA_VERSION_NUM || LUA_VERSION_NUM<=501
#error LUA Version less than 5.2
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <atmi.h>
#include <xa.h>
#include <fml32.h>
#include <e_engine.h>
#include <userlog.h>
#include <dlfcn.h>

#define LUAMETA_BUFFER "LUA_TUXEDO_BUFFER"
typedef struct{
	char type[ED_TYPELEN+1];
	char sub_type[ED_STYPELEN+1];
	long size;						/* tptypes return value */
	long raw_len;					/* buffer:raw_set() value length / length from receive data functions */
	char *ptr;
}LTUXEDO_BUFFER;

#endif
