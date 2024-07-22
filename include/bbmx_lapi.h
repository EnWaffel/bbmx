#ifndef __BBMX_LAPI_H
#define __BBMX_LAPI_H

#include <lua/lua.h>
#include "bbmxs/bbmxs.h"

#define LAPI_SUCCESS 0
#define LAPI_ERR_UNKNOWN -1

int bbmx_lapi_load(lua_State* L, BBMXSinitargs* initargs);
void bbmx_lapi_loaded();

#endif // __BBMX_LAPI_H