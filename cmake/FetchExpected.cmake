include(FetchContent)

FetchContent_Declare(
  expected
  GIT_REPOSITORY https://github.com/TartanLlama/expected.git
  GIT_TAG v1.1.0
)

set(EXPECTED_BUILD_TESTS OFF CACHE BOOL "close expected tests")
message(STATUS "Downloading TartanLlama/expected...")
FetchContent_MakeAvailable(expected)

