#include <luajit-2.1/lua.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <libgen.h>
#include <dirent.h>
#include <limits.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>

#include <luajit-2.1/luajit.h>
#include <luajit-2.1/lualib.h>
#include <luajit-2.1/lauxlib.h>

#include "lua_fs.h"



#if defined(__MINGW64__) || defined (__MINGW32__)
inline static char*
realpath(const char* szInputPath, char* szOutputPath) {
    return _fullpath(szOutputPath, szInputPath, PATH_MAX);
}
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

static int
lua_stat(lua_State* L);

static int
lua_stat_getDeviceID(lua_State* L);

static int
lua_stat_getFileID(lua_State* L);

static int
lua_stat_getOwnerID(lua_State* L);

static int
lua_stat_getGroupID(lua_State* L);

static int
lua_stat_getOwnerName(lua_State* L);

static int
lua_stat_getGroupName(lua_State* L);

static int
lua_stat_getLastAccessTime(lua_State* L);

static int
lua_stat_getLastModificationTime(lua_State* L);

static int
lua_stat_getLastStatusChangeTime(lua_State* L);

static int
lua_stat_getFileSize(lua_State* L);

static int
lua_stat_getFileType(lua_State* L);

static int
lua_stat_getOwnerPermissions(lua_State* L);

static int
lua_stat_getGroupPermissions(lua_State* L);

static int
lua_stat_getOtherPermissions(lua_State* L);

static char
    szPathBuffer[PATH_MAX + 1];

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
            { "readdir",    lua_readdir   },
            { "rewinddir",  lua_rewinddir },
            { "seekdir",    lua_seekdir   },
            { "telldir",    lua_telldir   },
            { "__gc",       lua_closedir  },
            { NULL,         NULL          }
        };
    luaL_setfuncs(L, lpDirectoryFuncs, 0);
    lua_settop(L, -2);

    luaL_newmetatable(L, "FileStats");
    lua_pushnil(L);
    lua_copy(L, -2, -1);
    lua_setfield(L, -2, "__index");
    luaL_Reg
        lpFileStatsFuncs[]  = {
            { "getDeviceID",                lua_stat_getDeviceID             },
            { "getFileID",                  lua_stat_getFileID               },
            { "getOwnerID",                 lua_stat_getOwnerID              },
            { "getGroupID",                 lua_stat_getGroupID              },
            { "getOwnerName",               lua_stat_getOwnerName            },
            { "getGroupName",               lua_stat_getGroupName            },
            { "getLastAccesTime",           lua_stat_getLastAccessTime       },
            { "getLastModificationTime",    lua_stat_getLastModificationTime },
            { "getLastStatusChangeTime",    lua_stat_getLastStatusChangeTime },
            { "getFileSize",                lua_stat_getFileSize             },
            { "getFileType",                lua_stat_getFileType             },
            { "getOwnerPermissions",        lua_stat_getOwnerPermissions     },
            { "getGroupPermissions",        lua_stat_getGroupPermissions     },
            { "getOtherPermissions",        lua_stat_getOtherPermissions     },
            { NULL,                         NULL                             }
        };
    luaL_setfuncs(L, lpFileStatsFuncs, 0);
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
            { "stat",       lua_stat     },
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

static int
lua_stat(lua_State* L) {
    assert(L);

    const char*
        szPath  = luaL_checkstring(L, 1);
    
    struct stat
        st_stat = {};
    
    if (stat(szPath, &st_stat) == 0) {
        struct stat*
            lpStat  = lua_newuserdata(L, sizeof(struct stat));
        
        *lpStat = st_stat;

        luaL_getmetatable(L, "FileStats");
        lua_setmetatable(L, -2);
    }
    else
        lua_pushnil(L);

    return 1;
}

static int
lua_stat_getDeviceID(lua_State* L) {
    assert(L);

    struct stat*
        lpStat  = luaL_checkudata(L, 1, "FileStats");

    lua_pushinteger(L, (lua_Integer)lpStat->st_dev);
    return 1;
}

static int
lua_stat_getFileID(lua_State* L) {
    assert(L);

    struct stat*
        lpStat  = luaL_checkudata(L, 1, "FileStats");

    lua_pushinteger(L, (lua_Integer)lpStat->st_ino);
    return 1;
}

static int
lua_stat_getOwnerID(lua_State* L) {
    assert(L);

    struct stat*
        lpStat  = luaL_checkudata(L, 1, "FileStats");

    lua_pushinteger(L, (lua_Integer)lpStat->st_uid);
    return 1;
}

static int
lua_stat_getGroupID(lua_State* L) {
    assert(L);

    struct stat*
        lpStat  = luaL_checkudata(L, 1, "FileStats");

    lua_pushinteger(L, (lua_Integer)lpStat->st_gid);
    return 1;
}

static int
lua_stat_getOwnerName(lua_State* L) {
    assert(L);

    struct stat*
        lpStat  = luaL_checkudata(L, 1, "FileStats");
    struct passwd*
        lpUser  = getpwuid(lpStat->st_uid);
    
    if (lpUser)
        lua_pushstring(L, lpUser->pw_name);
    else
        lua_pushnil(L);

    return 1;
}

static int
lua_stat_getGroupName(lua_State* L) {
    assert(L);

    struct stat*
        lpStat  = luaL_checkudata(L, 1, "FileStats");
    struct group*
        lpGroup = getgrgid(lpStat->st_gid);

    if (lpGroup)
        lua_pushstring(L, lpGroup->gr_name);
    else
        lua_pushnil(L);

    return 1;
}

static int
lua_stat_getLastAccessTime(lua_State* L) {
    assert(L);

    struct stat*
        lpStat  = luaL_checkudata(L, 1, "FileStats");

    lua_pushinteger(L, lpStat->st_atim.tv_sec);
    return 1;
}

static int
lua_stat_getLastModificationTime(lua_State* L) {
    assert(L);

    struct stat*
        lpStat  = luaL_checkudata(L, 1, "FileStats");

    lua_pushinteger(L, lpStat->st_mtim.tv_sec);
    return 1;
}

static int
lua_stat_getLastStatusChangeTime(lua_State* L) {
    assert(L);

    struct stat*
        lpStat  = luaL_checkudata(L, 1, "FileStats");

    lua_pushinteger(L, lpStat->st_ctim.tv_sec);
    return 1;
}

static int
lua_stat_getFileSize(lua_State* L) {
    assert(L);

    struct stat*
        lpStat  = luaL_checkudata(L, 1, "FileStats");

    lua_pushinteger(L, lpStat->st_size);
    return 1;
}

static int
lua_stat_getFileType(lua_State* L) {
    assert(L);

    struct stat*
        lpStat  = luaL_checkudata(L, 1, "FileStats");

    const char*
        szType  = "unknown";
    
    if (S_ISREG(lpStat->st_mode))
        szType  = "file";
    else if (S_ISDIR(lpStat->st_mode))
        szType  = "directory";
    else if (S_ISFIFO(lpStat->st_mode))
        szType  = "pipe";
    else if (S_ISLNK(lpStat->st_mode))
        szType  = "symbolic link";
    else if (S_ISSOCK(lpStat->st_mode))
        szType  = "socket";
    else if (S_ISBLK(lpStat->st_mode))
        szType  = "block special";
    else if (S_ISCHR(lpStat->st_mode))
        szType  = "char special";

    lua_pushstring(L, szType);
    return 1;
}

static int
lua_stat_getOwnerPermissions(lua_State* L) {
    assert(L);

    struct stat*
        lpStat  = luaL_checkudata(L, 1, "FileStats");

    lua_pushboolean(L, S_IRUSR && lpStat->st_mode);
    lua_pushboolean(L, S_IWUSR && lpStat->st_mode);
    lua_pushboolean(L, S_IXUSR && lpStat->st_mode);

    return 3;
}

static int
lua_stat_getGroupPermissions(lua_State* L) {
    assert(L);

    struct stat*
        lpStat  = luaL_checkudata(L, 1, "FileStats");

    lua_pushboolean(L, S_IRGRP && lpStat->st_mode);
    lua_pushboolean(L, S_IWGRP && lpStat->st_mode);
    lua_pushboolean(L, S_IXGRP && lpStat->st_mode);

    return 3;
}

static int
lua_stat_getOtherPermissions(lua_State* L) {
    assert(L);

    struct stat*
        lpStat  = luaL_checkudata(L, 1, "FileStats");

    lua_pushboolean(L, S_IROTH && lpStat->st_mode);
    lua_pushboolean(L, S_IWOTH && lpStat->st_mode);
    lua_pushboolean(L, S_IXOTH && lpStat->st_mode);

    return 3;
}
