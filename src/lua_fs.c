#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <dirent.h>
#include <limits.h>
#include <assert.h>

#include <luajit-2.1/luajit.h>
#include <luajit-2.1/lualib.h>
#include <luajit-2.1/lauxlib.h>

#include "lua_fs.h"

#ifdef __MINGW64__
    #define realpath(N, R) _fullpath((R), (N), PATH_MAX)
#endif

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
lua_opendir(lua_State* L);

static int
lua_closedir(lua_State* L);

static int
lua_readdir(lua_State* L);

static int
lua_readdir_iterator(lua_State* L);

static int
lua_seekdir(lua_State* L);

static int
lua_telldir(lua_State* L);

static int
lua_rewinddir(lua_State* L);

static int
lua_rmdir(lua_State* L);

static int
lua_rmfile(lua_State* L);

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

    luaL_newmetatable(L, "Directory");
    lua_pushnil(L);
    lua_copy(L, -2, -1);
    lua_setfield(L, -2, "__index");
    luaL_Reg
        lpDirectoryFuncs[] = {
            { "readdir",    lua_readdir     },
            { "rewinddir",  lua_rewinddir   },
            { "seekdir",    lua_seekdir     },
            { "telldir",    lua_telldir     },
            { "__gc",       lua_closedir    },
            { NULL,         NULL            }
        };
    luaL_setfuncs(L, lpDirectoryFuncs, 0);
    lua_settop(L, -2);

    luaL_Reg
        lpLibFuncs[] = {
            { "getcwd",     lua_getcwd   },
            { "chdir",      lua_chdir    },
            { "realpath",   lua_realpath },
            { "basename",   lua_basename },
            { "dirname",    lua_dirname  },
            { "access",     lua_access   },
            { "opendir",    lua_opendir  },
            { "rmdir",      lua_rmdir    },
            { "rmfile",     lua_rmfile   },
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
        szPath      = luaL_checkstring(L, 1);
    const char*
        szRealpath  = realpath(szPath, szPathBuffer);

    if (szRealpath)
        lua_pushstring(L, szRealpath);
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

static int
lua_opendir(lua_State* L) {
    assert(L);

    const char*
        szPath      = luaL_checkstring(L, 1);
    DIR*
        lpDirStream = opendir(szPath);
    if (lpDirStream) {
        *(DIR**)lua_newuserdata(L, sizeof(DIR*)) =
            lpDirStream;

        luaL_getmetatable(L, "Directory");
        lua_setmetatable(L, -2);
    }
    else
        lua_pushnil(L);

    return 1;
}

static int
lua_closedir(lua_State* L) {
    assert(L);

    DIR*
        lpDirStream = *(DIR**)luaL_checkudata(L, 1, "Directory");
    assert(lpDirStream);

    closedir(lpDirStream);
    return 0;
}

static int
lua_readdir(lua_State* L) {
    assert(L);
    
    luaL_checkudata(L, 1, "Directory");
    
    lua_pushnil(L);
    lua_copy(L, 1, -1);

    lua_pushcclosure(L, lua_readdir_iterator, 1);
    
    return 1;
}

static int
lua_readdir_iterator(lua_State* L) {
    assert(L);

    DIR*
        lpDirStream = *(DIR**)luaL_checkudata(L, lua_upvalueindex(1), "Directory");
    assert(lpDirStream);

    struct dirent*
        lpDirEntry  = readdir(lpDirStream);
    if (lpDirEntry)
        lua_pushstring(L, lpDirEntry->d_name);
    else 
        lua_pushnil(L);

    return 1;
}

static int
lua_seekdir(lua_State* L) {
    assert(L);

    DIR*
        lpDirStream = *(DIR**)luaL_checkudata(L, 1, "Directory");
    assert(lpDirStream);

    long
        iOffset     = (long)luaL_checkinteger(L, 2);
    
    seekdir(lpDirStream, iOffset);

    return 0;
}

static int
lua_telldir(lua_State* L) {
    assert(L);

    DIR*
        lpDirStream = *(DIR**)luaL_checkudata(L, 1, "Directory");
    assert(lpDirStream);

    lua_Integer
        iOffset     = telldir(lpDirStream);

    lua_pushinteger(L, iOffset);
    return 1;
}

static int
lua_rewinddir(lua_State* L) {
    assert(L);

    DIR*
        lpDirStream = *(DIR**)luaL_checkudata(L, 1, "Directory");
    assert(lpDirStream);

    rewinddir(lpDirStream);
    return 0;
}

static int
lua_rmdir(lua_State* L) {
    assert(L);

    const char*
        szPath  = luaL_checkstring(L, 1);

    lua_pushboolean(L, rmdir(szPath) == 0);
    return 1;
}

static int
lua_rmfile(lua_State* L) {
    assert(L);

    const char*
        szPath  = luaL_checkstring(L, 1);

    lua_pushboolean(L, remove(szPath) == 0);
    return 1;
}