cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(FruitNinja) 
find_package(glm CONFIG REQUIRED)
find_package(freetype CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(OpenAL CONFIG REQUIRED)
find_package(AudioFile CONFIG REQUIRED)

include_directories("libraries", ${CMAKE_SOURCE_DIR})

add_executable(${CMAKE_PROJECT_NAME} "FruitNinja.cpp" "rendering/shader.cpp" "libraries/stb_image.cpp" "rendering/mesh.cpp" "rendering/model.cpp" "infrastructure/object.cpp"  "state/time.cpp" "infrastructure/ui.cpp" "infrastructure/transform.cpp" "infrastructure/component.cpp" "physics/rigidbody.cpp" "game/fruit.cpp"  "state/cursor.cpp"  "game/fruitslice.cpp" "game/game.cpp" "rendering/font.cpp" "state/window.cpp"   "infrastructure/context.cpp" "game/input.cpp"   "audio/audiosource.cpp" "audio/audiolistener.cpp" "rendering/camera.cpp" "rendering/renderer.cpp" "audio/audio_context.cpp" "audio/audio_clip.cpp"   "rendering/particle_system.cpp"  "audio/audiosource_pool.cpp" "game/bomb.cpp" "game/util.cpp" "game/vfx.cpp" "game/modes/classic.cpp" "infrastructure/state_machine.cpp" "game/modes/classic_networking.cpp" "networking/networking.cpp" "game/state_selection.cpp" "infrastructure/coroutine.cpp") 
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ws2_32 glm::glm glad::glad glfw assimp::assimp Freetype::Freetype OpenAL::OpenAL AudioFile)