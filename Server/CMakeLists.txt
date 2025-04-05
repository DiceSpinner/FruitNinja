add_executable(Server "fruit_ninja_server.cpp" "server.cpp"  "default_state.cpp" "connection_state.cpp")
target_include_directories(Server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Server PRIVATE common)