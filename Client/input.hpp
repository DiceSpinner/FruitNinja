#ifndef INPUT_H
#define INPUT_H
#include <glfw/glfw3.h>
#include <unordered_map>
#include <functional>
#include <typeindex>
#include "infrastructure/clock.hpp"

namespace Input {
	extern bool lockedCamera;
	extern std::unordered_map<std::type_index, std::function<void(int, int)>> keyCallbacks;
	void onKeyPressed(GLFWwindow* window, int key, int scancode, int action, int mods);
	void cursorAim(GLFWwindow* window, double xpos, double ypos);
	void processInput(GLFWwindow* window);
	void initInput(GLFWwindow* window, Clock& gameClock);
}

#endif