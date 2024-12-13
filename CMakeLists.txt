cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(ToyEngine)
find_package(glm CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

add_executable(${CMAKE_PROJECT_NAME} ToyEngine.cpp "window/window.cpp") 
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE glm::glm glad::glad glfw)