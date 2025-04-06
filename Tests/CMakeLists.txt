add_executable(networking_test "test_udp_socket.cpp" "test_network.cpp") 

target_include_directories(networking_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(networking_test
    PRIVATE
        Catch2::Catch2
        common    
)

include(CTest)
include(Catch)
catch_discover_tests(networking_test)