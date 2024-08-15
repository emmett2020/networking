add_compile_options(-fno-access-control)

# Add a directory to save the generated executable file.
# If the directory is not specified, it is stored with the intermediate
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin/tests/)

set (LIBS Catch2::Catch2WithMain fmt::fmt magic_enum tl::expected STDEXEC::stdexec sio::sio)

add_executable(test_string_compare test_string_compare.cpp ${NET_SOURCE_LIST})
target_include_directories(test_string_compare PRIVATE ${NET_INCLUDE_PATH})
target_link_libraries(test_string_compare PRIVATE ${LIBS})

add_executable(test_http1_message_parser test_http1_message_parser.cpp ${NET_SOURCE_LIST})
target_include_directories(test_http1_message_parser PRIVATE ${NET_INCLUDE_PATH})
target_link_libraries(test_http1_message_parser PRIVATE ${LIBS})

add_executable(test_http_server test_http_server.cpp)
target_include_directories(test_http_server PRIVATE ${NET_INCLUDE_PATH})
target_link_libraries(test_http_server PRIVATE ${LIBS})





