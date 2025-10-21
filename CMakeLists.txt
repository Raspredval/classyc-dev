cmake_minimum_required(VERSION 3.16)

project(classyc-dev LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(classyc
    "source/main.cpp")

target_include_directories(classyc
    PRIVATE
        "include/"
        "dependencies/")
target_compile_options(classyc
    PRIVATE
        "-Wall" "-Wextra" "-Wconversion" "-Werror")

add_executable(classyc-tests
    "tests/tests.cpp")
target_include_directories(classyc-tests
    PRIVATE
        "include/"
        "tests/include"
        "dependencies/")
target_compile_options(classyc-tests
    PRIVATE
        "-Wall" "-Wextra" "-Wconversion" "-Werror")

include(CTest)

add_test(
    NAME    classyc-test-patterns
    COMMAND classyc-tests "--patterns")
add_test(
    NAME    classyc-test-text-io
    COMMAND classyc-tests "--text-io")