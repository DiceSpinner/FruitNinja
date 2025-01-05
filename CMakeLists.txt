cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(FruitNinja) 
find_package(glm CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)

include_directories("libraries")

add_executable(${CMAKE_PROJECT_NAME} "FruitNinja.cpp" "rendering/shader.cpp" "libraries/stb_image.cpp" "rendering/mesh.cpp" "rendering/model.cpp" "core/object.cpp"  "state/time.cpp" "core/ui.cpp" "core/transform.cpp" "core/component.cpp" "components/rigidbody.cpp" "components/fruit.cpp" "state/score.cpp" "state/cursor.cpp" "core/ui3D.cpp" "state/camera.cpp"  "state/new_objects.cpp" "components/fruitslice.cpp" "scripts/game.cpp") 
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE glm::glm glad::glad glfw assimp::assimp)