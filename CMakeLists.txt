cmake_minimum_required(VERSION 3.22)
project(input)

set(CMAKE_CXX_STANDARD 17)

add_library(input SHARED library.cpp input.hpp input.cpp windows.hpp common.hpp windows-raw-input.hpp test/test.cpp)

target_compile_definitions(input PRIVATE -DINPUT_WINDOWS)

find_package(GTest REQUIRED)
if (GTest_FOUND)
    enable_testing()
    add_executable(input-test test/test.cpp)
    target_link_libraries(input-test PRIVATE input GTest::gtest GTest::gtest_main)
    target_include_directories(input-test PRIVATE ${CMAKE_SOURCE_DIR})
    target_include_directories(input-test PRIVATE "C:\\MinGW-Library\\include")
    gtest_discover_tests(input-test)
endif(GTest_FOUND)