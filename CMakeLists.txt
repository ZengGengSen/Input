cmake_minimum_required(VERSION 3.22)
project(input)

set(CMAKE_CXX_STANDARD 17)

add_library(input SHARED library.cpp input.hpp input.cpp common.hpp mapping.hpp mapping.cpp)

if(WIN32)
    target_compile_definitions(input PRIVATE -DINPUT_WINDOWS)
    target_sources(input windows.hpp windows-raw-input.hpp)
elseif(UNIX)
    target_compile_definitions(input PRIVATE -DINPUT_UDEV)
    target_link_libraries(input PUBLIC udev)
    target_sources(input PRIVATE
            udev.hpp
            joypad/udev.hpp
            mouse/xlib.hpp
            keyboard/xlib.hpp)
endif(WIN32)

find_package(GTest REQUIRED)
find_package(glog REQUIRED)
if (GTest_FOUND)
    enable_testing()
    add_executable(input-test test/test.cpp)
    target_compile_definitions(input-test PRIVATE -DINPUT_UDEV)
    target_link_libraries(input-test PRIVATE input GTest::gtest)
    target_link_libraries(input-test PRIVATE input glog::glog)
    target_include_directories(input-test PRIVATE ${CMAKE_SOURCE_DIR})
    gtest_discover_tests(input-test)
endif(GTest_FOUND)