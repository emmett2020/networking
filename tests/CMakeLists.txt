

add_compile_options(-fno-access-control)

# Add a directory to save the generated executable file. 
# If the directory is not specified, it is stored with the intermediate
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin/tests/)

# TODO: update stdexec catch2 version, so we can use the latest Catch2WithMain
set (LIBS Catch2::Catch2WithMain fmt::fmt magic_enum)

add_executable(test_http1_message_parser test_http1_message_parser.cpp)
target_link_libraries(test_http1_message_parser PRIVATE ${LIBS})

add_executable(test_http_server test_http_server.cpp)
target_link_libraries(test_http_server PRIVATE ${LIBS} STDEXEC::stdexec sio::sio)

add_executable(test_then_value test_then_value.cpp)
target_link_libraries(test_then_value PRIVATE ${LIBS} STDEXEC::stdexec sio::sio)



