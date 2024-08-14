include(FetchContent)

FetchContent_Declare(
  senders-io
  # TODO: use http address
  # GIT_REPOSITORY https://github.com/Runner-2019/senders-io.git
  GIT_REPOSITORY git@github.com:Runner-2019/senders-io.git
  GIT_TAG networking
)

message(STATUS "Downloading senders-io...")
FetchContent_MakeAvailable(senders-io)

