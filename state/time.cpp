#include "GLFW/glfw3.h"
#include "time.hpp"

using namespace std;
static float currTime = 0;
static float cdeltaTime = 0;
static bool init = true;

void updateTime() {
	float currentTime = glfwGetTime();
	if (init) {
		init = false;
		cdeltaTime = 0;
	}
	else {
		cdeltaTime = currentTime - currTime;
	}
	currTime = currentTime;
}

float time() {
	return currTime;
}

float deltaTime() {
	return cdeltaTime;
}