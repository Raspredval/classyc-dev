cmake_minimum_required(VERSION 3.31)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(PROJECT_NAME "classyc")
project(${PROJECT_NAME} VERSION 0.1 LANGUAGES C)

add_executable(${PROJECT_NAME}
    "src/main.c"
    "src/lua_fs.c"

    "include/lua_bytecode.h"
    "include/lua_fs.h"
    
    "src/lua/CommonPatterns.lua"
    "src/lua/FileInfo.lua"
    "src/lua/log.lua"
    "src/lua/main.lua"
    "src/lua/oop.lua"
    "src/lua/Preprocessor.lua"
    "src/lua/Preprocessor/IMacro.lua"
    "src/lua/Preprocessor/MacroBuiltin.lua"
    "src/lua/Preprocessor/MacroDefined.lua"
    "src/lua/Preprocessor/MacroExpansion.lua")


if (NOT EXISTS "build/")
    file(MAKE_DIRECTORY "build/")
endif()

if (NOT EXISTS "bytecode/")
    file(MAKE_DIRECTORY "bytecode/")
    file(MAKE_DIRECTORY "bytecode/Preprocessor/")
endif()

if (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
    message("Host system: Windows")
    if (NOT DEFINED ${MSYS_PATH})
        set(MSYS_PATH "C:/msys64")
    endif()

    if (NOT EXISTS ${MSYS_PATH})
        message(FATAL_ERROR "${MSYS_PATH} doesn't exist, specify MSYS_PATH variable if MSYS2 is installed elsewhere")
    else()
        message("MSYS2 path: ${MSYS_PATH}")
    endif()

    set(CMAKE_SYSTEM_LIBRARY_PATH "${MSYS_PATH}/ucrt64/lib")
    set(CMAKE_SYSTEM_INCLUDE_PATH "${MSYS_PATH}/ucrt64/include")

    set(LUAJIT_CLI_PATH "${MSYS_PATH}/ucrt64/bin/luajit.exe")
else()
    message("Host system: Unix-like")
    set(CMAKE_SYSTEM_LIBRARY_PATH "/usr/lib")
    set(CMAKE_SYSTEM_INCLUDE_PATH "/usr/include")

    set(LUAJIT_CLI_PATH "/usr/bin/luajit")
endif()

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(LUAJIT_ARGS -b -g -t h)
else()
    set(LUAJIT_ARGS -b -s -t h)
endif()

target_include_directories(${PROJECT_NAME}
    PRIVATE
        "include/"
        "bytecode/")
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        luajit-5.1)

add_custom_target(CompileLua
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${LUAJIT_CLI_PATH} ${LUAJIT_ARGS} -n "CommonPatterns"
        "src/lua/CommonPatterns.lua" "bytecode/CommonPatterns.lua.h"
    COMMAND ${LUAJIT_CLI_PATH} ${LUAJIT_ARGS} -n "FileInfo"
        "src/lua/FileInfo.lua" "bytecode/FileInfo.lua.h"
    COMMAND ${LUAJIT_CLI_PATH} ${LUAJIT_ARGS} -n "log"
        "src/lua/log.lua" "bytecode/log.lua.h"
    COMMAND ${LUAJIT_CLI_PATH} ${LUAJIT_ARGS} -n "main"
        "src/lua/main.lua" "bytecode/main.lua.h"
    COMMAND ${LUAJIT_CLI_PATH} ${LUAJIT_ARGS} -n "oop"
        "src/lua/oop.lua" "bytecode/oop.lua.h"
    COMMAND ${LUAJIT_CLI_PATH} ${LUAJIT_ARGS} -n "Preprocessor"
        "src/lua/Preprocessor.lua" "bytecode/Preprocessor.lua.h"
    COMMAND ${LUAJIT_CLI_PATH} ${LUAJIT_ARGS} -n "Preprocessor.IMacro"
        "src/lua/Preprocessor/IMacro.lua" "bytecode/Preprocessor/IMacro.lua.h"
    COMMAND ${LUAJIT_CLI_PATH} ${LUAJIT_ARGS} -n "Preprocessor.MacroBuiltin"
        "src/lua/Preprocessor/MacroBuiltin.lua" "bytecode/Preprocessor/MacroBuiltin.lua.h"
    COMMAND ${LUAJIT_CLI_PATH} ${LUAJIT_ARGS} -n "Preprocessor.MacroDefined"
        "src/lua/Preprocessor/MacroDefined.lua" "bytecode/Preprocessor/MacroDefined.lua.h"
    COMMAND ${LUAJIT_CLI_PATH} ${LUAJIT_ARGS} -n "Preprocessor.MacroExpansion"
        "src/lua/Preprocessor/MacroExpansion.lua" "bytecode/Preprocessor/MacroExpansion.lua.h")
add_dependencies(${PROJECT_NAME}
    CompileLua)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        CLC_VERSION="ClassyC ${CMAKE_PROJECT_VERSION}"
        CC_VERSION="${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}"
        BUILD_TYPE="${CMAKE_BUILD_TYPE}")

if (${CMAKE_C_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_C_COMPILER_ID} STREQUAL "Clang")
    message("Using ${CMAKE_C_COMPILER_ID} c compiler")
    target_compile_options(${PROJECT_NAME}
        PRIVATE
            "-Wall"
            "-Wextra"
            "-Wconversion"
            "-Werror"
            "-Wno-unused-parameter")
else()
    message(FATAL_ERROR "Unsupported c compiler (supported: gcc, clang)")
endif()
