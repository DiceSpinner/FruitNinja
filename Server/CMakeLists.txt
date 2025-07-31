add_executable(Server "fruit_ninja_server.cpp"  "multiplayer_game.cpp" "multiplayer_fruit.cpp")
target_include_directories(Server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

find_package(boost_program_options CONFIG REQUIRED)
target_link_libraries(Server PRIVATE common Boost::program_options)