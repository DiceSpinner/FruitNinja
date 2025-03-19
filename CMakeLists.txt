cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(FruitNinja)

add_compile_definitions(
	TEXTURE_DIR="${CMAKE_SOURCE_DIR}/images/"
	MODEL_DIR="${CMAKE_SOURCE_DIR}/models/"
	SHADER_DIR="${CMAKE_SOURCE_DIR}/shaders/"
	AUDIO_DIR="${CMAKE_SOURCE_DIR}/sounds/"
	FONT_DIR="${CMAKE_SOURCE_DIR}/fonts/"
)

add_subdirectory("Common")
add_subdirectory("Client")