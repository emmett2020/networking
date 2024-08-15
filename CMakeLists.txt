cmake_minimum_required(VERSION 3.22)
project(networking VERSION "0.0.1"
                   DESCRIPTION "A networking framework based on senders."
                   LANGUAGES CXX)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
    set(CMAKE_POLICY_DEFAULT_CMP0135 NEW)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

set(NET_ROOT_PATH      ${PROJECT_SOURCE_DIR})
set(NET_INCLUDE_PATH   ${PROJECT_SOURCE_DIR}/include)
set(NET_SOURCE_PATH    ${PROJECT_SOURCE_DIR}/src)
set(NET_TESTS_PATH     ${PROJECT_SOURCE_DIR}/tests)
set(NET_EXAMPLES_PATH  ${PROJECT_SOURCE_DIR}/examples)
set(NET_CMAKE_PATH     ${PROJECT_SOURCE_DIR}/cmake)

list(APPEND CMAKE_MODULE_PATH "${NET_CMAKE_PATH}")
include(FetchSIO)
include(FetchFmt)
include(FetchNlohmannJson)
include(FetchSpdlog)
include(FetchMagicEnum)
include(FetchExpected)
include(FetchRangeV3)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-subobject-linkage")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-shift-count-overflow")
file(GLOB_RECURSE NET_SOURCE_LIST ${NET_SOURCE_PATH}/utils/*.cpp)
add_executable(networking ${NET_SOURCE_LIST} ${NET_ROOT_PATH}/main.cpp)

target_include_directories(networking PRIVATE ${NET_INCLUDE_PATH})
target_link_libraries(networking PRIVATE
                      STDEXEC::stdexec
                      sio::sio
                      fmt::fmt
                      nlohmann_json::nlohmann_json
                      spdlog::spdlog
                      magic_enum::magic_enum
                      range-v3
                      tl::expected)

# If uses this as top level project, build tests default.
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
   set(NET_BUILD_TESTS_DEFAULT ON)
else()
   set(NET_BUILD_TESTS_DEFAULT OFF)
endif()

option(NET_BUILD_TESTS "Build networking tests" ${NET_BUILD_TESTS_DEFAULT})
option(BUILD_TESTING "A universal option to open/close testing" ${NET_BUILD_TESTS})
if (BUILD_TESTING)
   include(CTest)
   message(STATUS "Building networing unittest...")
   add_subdirectory(${NET_TESTS_PATH})
endif()


