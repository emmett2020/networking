cmake_minimum_required(VERSION 3.22)

# Project properties.
project(networking VERSION "0.1" LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
message(STATUS "building networking......" )

# Set path for source files.
set(SOURCE_PATH         ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(THIRDPARTIES_PATH   ${CMAKE_CURRENT_SOURCE_DIR}/thirdparties)
set(TESTS_PATH          ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set(EXAMPLES_PATH       ${CMAKE_CURRENT_SOURCE_DIR}/examples)

# Include necessary directoires.
include_directories(${SOURCE_PATH}/)

add_subdirectory(${THIRDPARTIES_PATH}/stdexec)
# TODO: rework senders-io's cmakelist.txt
set(stdexec_DIR ${CMAKE_SOURCE_DIR}/build/thirdparties/stdexec/)
add_subdirectory(${THIRDPARTIES_PATH}/senders-io)
add_subdirectory(${THIRDPARTIES_PATH}/fmt)
add_subdirectory(${THIRDPARTIES_PATH}/nlohmann-json)
add_subdirectory(${THIRDPARTIES_PATH}/spdlog)
add_subdirectory(${THIRDPARTIES_PATH}/magic_enum)

# find source files
file (GLOB_RECURSE SOURCE_LIST ${SOURCE_PATH}/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCE_LIST} main.cpp)

target_link_libraries(${PROJECT_NAME} PUBLIC STDEXEC::stdexec 
                                             sio::sio 
                                             fmt::fmt
                                             nlohmann_json::nlohmann_json
                                             spdlog::spdlog
                                             magic_enum::magic_enum)

option(BUILD_NETWORKING_TEST "build networking test" ON)
if (BUILD_NETWORKING_TEST)
    message(STATUS "building networing unittest...")
    # !stdexec also use Catch2
    add_subdirectory(${THIRDPARTIES_PATH}/catch2)
    add_subdirectory(${TESTS_PATH})
endif()


