cmake_minimum_required(VERSION 3.16)

project(classyc-dev LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ASSETS_DIR  "${CMAKE_SOURCE_DIR}/tests/assets")
set(ASSETS_LINK "${CMAKE_BINARY_DIR}/assets")
file (CREATE_LINK ${ASSETS_DIR} ${ASSETS_LINK} SYMBOLIC)

set(CXX_WARNINGS "-Wall" "-Wextra" "-Wconversion" "-Werror")

add_executable(classyc
    "source/main.cpp")
target_include_directories(classyc
    PRIVATE
        "include/")
target_compile_options(classyc
    PRIVATE
        ${CXX_WARNINGS})

add_executable(classyc-proto-preproc
    "tests/ProtoPreproc.cpp")
target_include_directories(classyc-proto-preproc
    PRIVATE
        "include/"
        "tests/include")
target_compile_options(classyc-proto-preproc
    PRIVATE
        ${CXX_WARNINGS})