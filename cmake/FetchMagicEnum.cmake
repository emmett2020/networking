include(FetchContent)

FetchContent_Declare(
  magic_enum
  GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
  GIT_TAG v0.9.6
)

message(STATUS "Downloading magic_enum...")
FetchContent_MakeAvailable(magic_enum)

