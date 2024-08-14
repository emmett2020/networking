
include(FetchContent)

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.14.1
)

message(STATUS "Downloading spdlog...")
FetchContent_MakeAvailable(spdlog)

