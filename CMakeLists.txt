cmake_minimum_required(VERSION 3.16)

project(classyc-dev LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(WARNINGS_FLAGS "-Wall" "-Wextra" "-Wconversion" "-Werror")

set(ASSETS_DIR  "${CMAKE_SOURCE_DIR}/tests/assets")
set(ASSETS_LINK "${CMAKE_BINARY_DIR}/assets")
file (CREATE_LINK ${ASSETS_DIR} ${ASSETS_LINK} SYMBOLIC)

add_executable(classyc
    "source/main.cpp")
target_include_directories(classyc
    PRIVATE
        "include/")
target_compile_options(classyc
    PRIVATE
        ${WARNINGS_FLAGS})

add_executable(classyc-test-patterns
    "tests/TestPatterns.cpp")
target_include_directories(classyc-test-patterns
    PRIVATE
        "include/"
        "tests/include")
target_compile_options(classyc-test-patterns
    PRIVATE
        ${WARNINGS_FLAGS})

add_executable(classyc-test-text-io
    "tests/TestTextIO.cpp")
target_include_directories(classyc-test-text-io
    PRIVATE
        "include/"
        "tests/include")
target_compile_options(classyc-test-text-io
    PRIVATE
        ${WARNINGS_FLAGS})

add_executable(classyc-proto-preproc
    "tests/ProtoPreproc.cpp")
target_include_directories(classyc-proto-preproc
    PRIVATE
        "include/"
        "tests/include")
target_compile_options(classyc-proto-preproc
    PRIVATE
        ${WARNINGS_FLAGS})

include(CTest)

add_test(
    NAME    classyc-test-text-io
    COMMAND classyc-test-text-io)
add_test(
    NAME    classyc-test-patterns
    COMMAND classyc-test-patterns)
add_test(
    NAME    classyc-proto-preproc
    COMMAND classyc-proto-preproc)