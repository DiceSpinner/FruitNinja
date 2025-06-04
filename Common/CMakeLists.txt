add_library(common STATIC "rendering/shader.cpp" "libraries/stb_image.cpp" "rendering/mesh.cpp" "rendering/model.cpp" "infrastructure/object.cpp"  "infrastructure/ui.cpp" "infrastructure/transform.cpp"  "physics/rigidbody.cpp"  "rendering/font.cpp"      "audio/audiosource.cpp" "audio/audiolistener.cpp" "rendering/camera.cpp" "rendering/renderer.cpp" "audio/audio_context.cpp" "audio/audio_clip.cpp"   "rendering/particle_system.cpp"  "audio/audiosource_pool.cpp" "infrastructure/state_machine.cpp" "networking/networking.cpp" "infrastructure/coroutine.cpp"  "networking/socket.cpp"  "networking/connection.cpp" "rendering/render_context.cpp" "infrastructure/clock.cpp")

find_package(glm CONFIG REQUIRED)
find_package(freetype CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(OpenAL CONFIG REQUIRED)
find_package(AudioFile CONFIG REQUIRED)
find_package(unofficial-enet CONFIG REQUIRED)

target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(common PUBLIC ws2_32 glm::glm glad::glad glfw assimp::assimp Freetype::Freetype OpenAL::OpenAL AudioFile unofficial::enet::enet)