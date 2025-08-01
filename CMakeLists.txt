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
    
    "lua/CommonPatterns.lua"
    "lua/FileInfo.lua"
    "lua/log.lua"
    "lua/main.lua"
    "lua/oop.lua"
    "lua/Preprocessor.lua"
    "lua/Preprocessor/IMacro.lua"
    "lua/Preprocessor/MacroBuiltin.lua"
    "lua/Preprocessor/MacroDefined.lua")

if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
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

set(BYTECODE_DIR "${CMAKE_CURRENT_BINARY_DIR}/lua_bytecode")

if (NOT EXISTS "${BYTECODE_DIR}")
    file(MAKE_DIRECTORY "${BYTECODE_DIR}")
    file(MAKE_DIRECTORY "${BYTECODE_DIR}/Preprocessor/")
endif()

target_include_directories(${PROJECT_NAME}
    PRIVATE
        "include/"
        "${BYTECODE_DIR}/")
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        luajit-5.1)

add_custom_target(CompileLua
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${LUAJIT_CLI_PATH} ${LUAJIT_ARGS} -n "CommonPatterns"
        "lua/CommonPatterns.lua" "${BYTECODE_DIR}/CommonPatterns.lua.h"
    COMMAND ${LUAJIT_CLI_PATH} ${LUAJIT_ARGS} -n "FileInfo"
        "lua/FileInfo.lua" "${BYTECODE_DIR}/FileInfo.lua.h"
    COMMAND ${LUAJIT_CLI_PATH} ${LUAJIT_ARGS} -n "log"
        "lua/log.lua" "${BYTECODE_DIR}/log.lua.h"
    COMMAND ${LUAJIT_CLI_PATH} ${LUAJIT_ARGS} -n "main"
        "lua/main.lua" "${BYTECODE_DIR}/main.lua.h"
    COMMAND ${LUAJIT_CLI_PATH} ${LUAJIT_ARGS} -n "oop"
        "lua/oop.lua" "${BYTECODE_DIR}/oop.lua.h"
    COMMAND ${LUAJIT_CLI_PATH} ${LUAJIT_ARGS} -n "Preprocessor"
        "lua/Preprocessor.lua" "${BYTECODE_DIR}/Preprocessor.lua.h"
    COMMAND ${LUAJIT_CLI_PATH} ${LUAJIT_ARGS} -n "Preprocessor.IMacro"
        "lua/Preprocessor/IMacro.lua" "${BYTECODE_DIR}/Preprocessor/IMacro.lua.h"
    COMMAND ${LUAJIT_CLI_PATH} ${LUAJIT_ARGS} -n "Preprocessor.MacroBuiltin"
        "lua/Preprocessor/MacroBuiltin.lua" "${BYTECODE_DIR}/Preprocessor/MacroBuiltin.lua.h"
    COMMAND ${LUAJIT_CLI_PATH} ${LUAJIT_ARGS} -n "Preprocessor.MacroDefined"
        "lua/Preprocessor/MacroDefined.lua" "${BYTECODE_DIR}/Preprocessor/MacroDefined.lua.h")
add_dependencies(${PROJECT_NAME}
    CompileLua)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        CLC_VERSION="ClassyCC ${CMAKE_PROJECT_VERSION}"
        CC_VERSION="${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}"
        BUILD_TYPE="${CMAKE_BUILD_TYPE}")

if (${CMAKE_C_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_C_COMPILER_ID} STREQUAL "Clang")
    message("Using ${CMAKE_C_COMPILER_ID} C compiler")
    target_compile_options(${PROJECT_NAME}
        PRIVATE
            "-Wall"
            "-Wextra"
            "-Wconversion"
            "-Werror"
            "-Wno-unused-parameter")
else()
    message(FATAL_ERROR "Unsupported C compiler (supported: gcc, clang)")
endif()
