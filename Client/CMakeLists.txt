add_executable(Client "FruitNinja.cpp" "game/game.cpp" "game/classic_mode/classic.cpp"  "game/vfx.cpp" "game/bomb.cpp" "game/fruit.cpp" "game/fruitslice.cpp" "game/state_selection.cpp" "game/util.cpp" "input.cpp" "cursor.cpp" "game/mtp_mode/mtp_classic.cpp") 
target_include_directories(Client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

find_package(boost_program_options CONFIG REQUIRED)
target_link_libraries(Client PRIVATE common Boost::program_options)
