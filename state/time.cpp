#include "GLFW/glfw3.h"
#include "time.hpp"
static float ctime = 0;
static float cdeltaTime = 0;

void updateTime() {
	float currentTime = glfwGetTime();
	cdeltaTime = currentTime - ctime;
	ctime = currentTime;
}

float time() {
	return ctime;
}

float deltaTime() {
	return cdeltaTime;
}