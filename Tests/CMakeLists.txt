add_executable(UnitTests "test_udp_socket.cpp") 

target_include_directories(UnitTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(UnitTests
    PRIVATE
        Catch2::Catch2WithMain
        common           # Link your networking code here
)

include(CTest)
include(Catch)
catch_discover_tests(UnitTests)