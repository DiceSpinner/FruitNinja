#include "window.hpp"
// settings
unsigned int SCR_WIDTH = 1920;
unsigned int SCR_HEIGHT = 1080;
GLFWwindow* window = nullptr;
glm::mat4 ortho(1);
glm::mat4 perspective(1);