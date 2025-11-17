cmake_minimum_required(VERSION 3.16)

project(classyc-dev LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(WARNINGS_FLAGS "-Wall" "-Wextra" "-Wconversion" "-Werror")

add_executable(classyc
    "source/main.cpp")
target_include_directories(classyc
    PRIVATE
        "include/"
        "dependencies/")
target_compile_options(classyc
    PRIVATE
        ${WARNINGS_FLAGS})

add_executable(classyc-test-patterns
    "tests/TestPatterns.cpp")
target_include_directories(classyc-test-patterns
    PRIVATE
        "include/"
        "tests/include"
        "dependencies/")
target_compile_options(classyc-test-patterns
    PRIVATE
        ${WARNINGS_FLAGS})

add_executable(classyc-test-text-io
    "tests/TestTextIO.cpp")
target_include_directories(classyc-test-text-io
    PRIVATE
        "include/"
        "tests/include"
        "dependencies/")
target_compile_options(classyc-test-text-io
    PRIVATE
        ${WARNINGS_FLAGS})

add_executable(classyc-proto-preproc
    "tests/ProtoPreproc.cpp")
target_include_directories(classyc-proto-preproc
    PRIVATE
        "include/"
        "tests/include"
        "dependencies/")
target_compile_options(classyc-proto-preproc
    PRIVATE
        ${WARNINGS_FLAGS})

set(ASSETS_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/tests/assets")
set(ASSETS_LINK "${CMAKE_CURRENT_BINARY_DIR}/assets")
add_custom_command(
    OUTPUT  ${ASSETS_LINK}
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/create_symlink.cmake ${ASSETS_DIR} ${ASSETS_LINK})
add_custom_target(create_assets_symlink
    DEPENDS ${ASSETS_LINK})

add_dependencies(classyc-test-patterns
    create_assets_symlink)
add_dependencies(classyc-test-text-io
    create_assets_symlink)
add_dependencies(classyc-proto-preproc
    create_assets_symlink)

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