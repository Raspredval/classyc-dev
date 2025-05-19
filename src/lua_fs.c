#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>
#include <assert.h>

#include <luajit.h>
#include <lualib.h>
#include <lauxlib.h>

#include "lua_fs.h"


static int
lua_getcwd(lua_State* L);

static int
lua_chdir(lua_State* L);

static int
lua_realpath(lua_State* L);

static int
lua_basename(lua_State* L);

static int
lua_dirname(lua_State* L);

static int
lua_access(lua_State* L);

static int
lua_load_fs(lua_State* L);

static char
    szPathBuffer[PATH_MAX + 1] = "";

extern void
PreloadFilesystemAPI(lua_State* L) {
    assert(L);

    lua_getglobal(L, "package");
    lua_getfield(L, -1, "preload");
    
    lua_pushcfunction(L, lua_load_fs);
    lua_setfield(L, -2, "fs");

    lua_settop(L, -3);
}

static int
lua_load_fs(lua_State* L) {
    assert(L);

    luaL_Reg
        lpLibFuncs[] = {
            { "getcwd",     lua_getcwd   },
            { "chdir",      lua_chdir    },
            { "realpath",   lua_realpath },
            { "basename",   lua_basename },
            { "dirname",    lua_dirname  },
            { "access",     lua_access   },
            { NULL,         NULL         }
        };

    luaL_newlib(L, lpLibFuncs);
    return 1;
}

static int
lua_access(lua_State* L) {
    assert(L);

    int iFlags      = F_OK;
    const char*
        szPath      = luaL_checkstring(L, 1);

    if (lua_isstring(L, 2)) {
        const char*
            szMode      = lua_tostring(L, 2);
        size_t
            uModeLen    = strnlen(szMode, 4);
        for (size_t i = 0; i != uModeLen; ++i) {
            switch(szMode[i]) {
            case 'f':
            case 'F':
                iFlags |= F_OK;
                break;
            
            case 'x':
            case 'X':
                iFlags |= X_OK;
                break;
                
            case 'w':
            case 'W':
                iFlags |= W_OK;
                break;
            
            case 'r':
            case 'R':
                iFlags |= R_OK;
                break;

            default:
                return luaL_error(L,
                    "invalid mode character: '%c', must be one of [FfXxWwRr]",
                    szMode[i]);
            }
        }
    }

    lua_pushboolean(L, access(szPath, iFlags) == 0);
    return 1;
}

static int
lua_dirname(lua_State* L) {
    assert(L);

    const char*
        szPath      = luaL_checkstring(L, 1);
    strncpy(szPathBuffer, szPath, PATH_MAX);
    const char*
        szDirname   = dirname(szPathBuffer);

    if (szDirname)
        lua_pushstring(L, szDirname);
    else
        lua_pushnil(L);
    return 1;
}

static int
lua_basename(lua_State* L) {
    assert(L);

    const char*
        szPath      = luaL_checkstring(L, 1);
    strncpy(szPathBuffer, szPath, PATH_MAX);
    const char*
        szBasename  = basename(szPathBuffer);

    if (szBasename)
        lua_pushstring(L, szBasename);
    else
        lua_pushnil(L);
    return 1;
}

static int
lua_realpath(lua_State* L) {
    assert(L);

    const char*
        szPath  = luaL_checkstring(L, 1);

    if(realpath(szPath, szPathBuffer))
        lua_pushstring(L, szPathBuffer);
    else
        lua_pushnil(L);
    return 1;
}

static int
lua_chdir(lua_State* L) {
    assert(L);

    const char*
        szPath  = luaL_checkstring(L, 1);

    lua_pushboolean(L, chdir(szPath) == 0);
    return 1;
}

static int
lua_getcwd(lua_State* L) {
    assert(L);

    if (getcwd(szPathBuffer, PATH_MAX))
        lua_pushstring(L, szPathBuffer);
    else
        lua_pushnil(L);
    return 1;
}
