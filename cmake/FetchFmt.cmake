include(FetchContent)

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 11.0.2
)

message(STATUS "Downloading fmt...")
FetchContent_MakeAvailable(fmt)

