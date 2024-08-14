include(FetchContent)

FetchContent_Declare(
  ranges-v3
  GIT_REPOSITORY https://github.com/ericniebler/range-v3.git
  # 2024-5-11
  GIT_TAG 53c40dd628450c977ee1558285ff43e0613fa7a9
)

message(STATUS "Downloading ranges-v3...")
FetchContent_MakeAvailable(ranges-v3)

