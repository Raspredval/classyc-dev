#include <luajit-2.1/luajit.h>
#include <luajit-2.1/lualib.h>
#include <luajit-2.1/lauxlib.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

#include "lua_bytecode.h"
#include "lua_fs.h"

typedef struct {
    const char*
        szModuleName;
    const void*
        lpvBytecode;
    size_t
        uSize;
} ModuleInfo;

static int
PreloadModules(lua_State* L, const ModuleInfo lpModules[]);

static void
PassBuildInfo(lua_State* L);

static void
PassConsoleArgs(lua_State* L, int argc, const char* argv[]);

static void
CleanupAtExit();

static lua_State*
    g_hLuaState = NULL;

extern int
main(int argc, const char* argv[]) {
    atexit(CleanupAtExit);
    
    g_hLuaState     = luaL_newstate();
    if (!g_hLuaState) {
        fprintf(stderr, "Internal error > failed to create Lua context\n");
        return EXIT_FAILURE;
    }

    luaL_openlibs(g_hLuaState);
    PassBuildInfo(g_hLuaState);
    PassConsoleArgs(g_hLuaState, argc, argv);
    PreloadFilesystemAPI(g_hLuaState);

    ModuleInfo
        lpModules[] = {
            { "CommonPatterns",             luaJIT_BC_CommonPatterns,               luaJIT_BC_CommonPatterns_SIZE               },
            { "FileInfo",                   luaJIT_BC_FileInfo,                     luaJIT_BC_FileInfo_SIZE                     },
            { "log",                        luaJIT_BC_log,                          luaJIT_BC_log_SIZE                          },
            { "main",                       luaJIT_BC_main,                         luaJIT_BC_main_SIZE                         },
            { "oop",                        luaJIT_BC_oop,                          luaJIT_BC_oop_SIZE                          },
            { "Preprocessor",               luaJIT_BC_Preprocessor,                 luaJIT_BC_Preprocessor_SIZE                 },
            { "Preprocessor.IMacro",        luaJIT_BC_Preprocessor_IMacro,          luaJIT_BC_Preprocessor_IMacro_SIZE          },
            { "Preprocessor.MacroBuiltin",  luaJIT_BC_Preprocessor_MacroBuiltin,    luaJIT_BC_Preprocessor_MacroBuiltin_SIZE    },
            { "Preprocessor.MacroDefined",  luaJIT_BC_Preprocessor_MacroDefined,    luaJIT_BC_Preprocessor_MacroDefined_SIZE    },
            { "Preprocessor.MacroExpansion",luaJIT_BC_Preprocessor_MacroExpansion,  luaJIT_BC_Preprocessor_MacroExpansion_SIZE  },
            { NULL,                         NULL,                                   0                                           }
        };
    
    if (PreloadModules  (g_hLuaState, lpModules) ||
        luaL_loadstring (g_hLuaState, "require \"main\"") ||
        lua_pcall       (g_hLuaState, 0, 0, 0))
    {
        fprintf(stderr, "%s\n",
            lua_tostring(g_hLuaState, -1));

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static void
CleanupAtExit() {
    if (g_hLuaState)
        lua_close(g_hLuaState);
}

static void
PassConsoleArgs(lua_State* L, int argc, const char* argv[])  {
    assert(L && argv);

    lua_getglobal(L, "io");
    lua_newtable(L);

    for (int i = 1; i < argc; ++i) {
        lua_pushinteger(L, i);
        lua_pushstring(L, argv[i]);
        lua_settable(L, -3);
    }

    lua_setfield(L, -2, "cargs");
    lua_settop(L, 0);
}

static void
PassBuildInfo(lua_State* L) {
    assert(L);

    lua_newtable(L);

    lua_pushliteral(L, CLC_VERSION);
    lua_setfield(L, -2, "VERSION");

    lua_pushliteral(L, CC_VERSION);
    lua_setfield(L, -2, "CC_VERSION");

    lua_pushliteral(L, BUILD_TYPE);
    lua_setfield(L, -2, "BUILD_TYPE");

    lua_setglobal(L, "CLASSYC");
}

static int
PreloadModules(lua_State* L, const ModuleInfo* lpModules)  {
    assert(L && lpModules);

    lua_getglobal(L, "package");
    lua_getfield(L, -1, "preload");

    const ModuleInfo*
        itCurModule = lpModules;
    while (itCurModule->szModuleName != NULL) {
        int iLoadBuffer =
            luaL_loadbuffer(L,
                itCurModule->lpvBytecode,
                itCurModule->uSize,
                itCurModule->szModuleName);
        if (iLoadBuffer != LUA_OK)
            return iLoadBuffer;
        lua_setfield(L, -2, itCurModule->szModuleName);
        itCurModule += 1;
    }

    lua_settop(L, -3);
    return LUA_OK;
}
