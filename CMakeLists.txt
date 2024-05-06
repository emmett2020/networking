cmake_minimum_required(VERSION 3.22)

# Set compilation related commands.
set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Set project properties.
project(networking VERSION "0.0.1" LANGUAGES CXX)
message(STATUS "building ${PROJECT_NAME} ${PROJECT_VERSION}......" )

# Set some variables to conveniently use the subdirectory path.
set(NET_INCLUDE_PATH        ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(NET_SOURCE_PATH         ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(NET_THIRDPARTIES_PATH   ${CMAKE_CURRENT_SOURCE_DIR}/thirdparties)
set(NET_TESTS_PATH          ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set(NET_EXAMPLES_PATH       ${CMAKE_CURRENT_SOURCE_DIR}/examples)
set(NET_CMAKE_PATH          ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Include necessary directoires.
include_directories(${NET_INCLUDE_PATH})

# WARN: make sure you have commented the catch2 in stdexec since this is conflict with networking now.
set(BUILD_TESTING OFF)
set(STDEXEC_BUILD_EXAMPLES OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
add_subdirectory(${NET_THIRDPARTIES_PATH}/stdexec)

# TODO: rework senders-io's cmakelist.txt
set(stdexec_DIR ${CMAKE_SOURCE_DIR}/build/thirdparties/stdexec/)

add_subdirectory(${NET_THIRDPARTIES_PATH}/senders-io)
add_subdirectory(${NET_THIRDPARTIES_PATH}/fmt)
add_subdirectory(${NET_THIRDPARTIES_PATH}/nlohmann-json)
add_subdirectory(${NET_THIRDPARTIES_PATH}/spdlog)
add_subdirectory(${NET_THIRDPARTIES_PATH}/magic_enum)
add_subdirectory(${NET_THIRDPARTIES_PATH}/expected)
add_subdirectory(${NET_THIRDPARTIES_PATH}/range-v3)

# find source files
file (GLOB_RECURSE SOURCE_LIST ${NET_SOURCE_PATH}/utils/*.cpp)
add_executable(${PROJECT_NAME} ${NET_SOURCE_LIST} main.cpp)

target_link_libraries(${PROJECT_NAME} PUBLIC STDEXEC::stdexec
                                             sio::sio
                                             fmt::fmt
                                             nlohmann_json::nlohmann_json
                                             spdlog::spdlog
                                             magic_enum::magic_enum
                                             range-v3
                                             tl::expected)

option(BUILD_NET_TEST "build networking test" OFF)
if (BUILD_NET_TEST)
    message(STATUS "building networing unittest...")
    # !stdexec also use Catch2
    add_subdirectory(${NET_THIRDPARTIES_PATH}/catch2)
    add_subdirectory(${NET_TESTS_PATH})
endif()


