#ifndef WINDOW_H
#define WINDOW_H
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

extern unsigned int SCR_WIDTH;
extern unsigned int SCR_HEIGHT;
extern GLFWwindow* window;
extern glm::mat4 perspective;
extern glm::mat4 ortho;
#endif