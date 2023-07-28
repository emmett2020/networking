cmake_minimum_required(VERSION 3.22)

# Project properties.
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 1)
project(networking VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}" LANGUAGES CXX)
message(STATUS "building networking version: " ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

# Cpp standard.
set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Set path for source files.
set(INCLUDE_PATH        ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(THIRDPARTIES_PATH   ${CMAKE_CURRENT_SOURCE_DIR}/thirdparties)
set(TESTS_PATH          ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set(EXAMPLES_PATH       ${CMAKE_CURRENT_SOURCE_DIR}/examples)
message(STATUS "include      path: "  ${INCLUDE_PATH})
message(STATUS "thirdparties path: "  ${THIRDPARTIES_PATH})
message(STATUS "unittests    path: "  ${TESTS_PATH})
message(STATUS "examples     path: "  ${EXAMPLES_PATH})

# Include necessary directoires.
include_directories(${INCLUDE_PATH}/)
include_directories(${THIRDPARTY_PATH}/stdexec/include/)
include_directories(${THIRDPARTY_PATH}/sources/include/)

# Autodetect platform
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	message(STATUS "Configuring for Linux")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	message(STATUS "Configuring for macOS")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	message(STATUS "Configuring for Windows")
else()
	message(STATUS "Configuring on/for ${CMAKE_SYSTEM_NAME}")
endif()

# Autodetect complier
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    message(STATUS "using Clang compiler")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    message(STATUS "using AppleClang compiler")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    message(STATUS "using GCC compiler")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    message(STATUS "using MSVC compiler")
else()
    message(STATUS "!unknow compiler. ${CMAKE_CXX_COMPILER_ID}")
endif()

add_subdirectory(thirdparties/stdexec)

# TODO: rework senders-io's cmakelists
set(stdexec_DIR ${CMAKE_SOURCE_DIR}/build/thirdparties/stdexec/)

add_subdirectory(thirdparties/senders-io)
add_subdirectory(thirdparties/fmt)

add_executable(networking main.cpp)
target_link_libraries(networking PUBLIC STDEXEC::stdexec sio::sio fmt::fmt)


