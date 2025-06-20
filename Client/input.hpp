#ifndef INPUT_H
#define INPUT_H
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include "infrastructure/clock.hpp"

namespace Input {
	extern bool lockedCamera;
	void onKeyPressed(GLFWwindow* window, int key, int scancode, int action, int mods);
	void cursorAim(GLFWwindow* window, double xpos, double ypos);
	void processInput(GLFWwindow* window);
	void initInput(GLFWwindow* window, Clock& gameClock);
}

#endif