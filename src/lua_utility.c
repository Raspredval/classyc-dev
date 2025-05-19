#include <assert.h>
#include <luajit.h>
#include <lualib.h>
#include <lauxlib.h>

#include "lua_utility.h"

static int
lua_optval(lua_State* L) {
    assert(L);

    luaL_argcheck(L,
        (!lua_isnoneornil(L, 1)),
        1, "shouldn't be nil");
    return 1;
}

extern void
PassUtilityFunctions(lua_State* L) {
    assert(L);

    lua_pushcfunction(L, lua_optval);
    lua_setglobal(L, "optval");
}