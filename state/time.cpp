#include "GLFW/glfw3.h"
#include "time.hpp"

using namespace std;
static float currTime = 0;
static float cdeltaTime = 0;
static float usDeltaTime = 0;
static bool init = true;
static float fixedUpdateDelta = 0;
static float lastFixedUpdate = 0;
static float fixedUpdateAccumulator = 0;
static bool shouldPhysicsUpdate = false;

namespace Time {
	float timeScale = 1;

	void updateTime() {
		float currentTime = glfwGetTime();
		if (init) {
			init = false;
			cdeltaTime = 0;
		}
		else {
			cdeltaTime = currentTime - currTime;
		}
		usDeltaTime = cdeltaTime;
		cdeltaTime *= timeScale;
		currTime = currentTime;
		fixedUpdateAccumulator += cdeltaTime;
		if (fixedUpdateAccumulator >= 1.0f / PHYSICS_FPS) {
			fixedUpdateDelta = currTime - lastFixedUpdate;
			lastFixedUpdate = currTime;
			fixedUpdateAccumulator -= 1.0f / PHYSICS_FPS;
			shouldPhysicsUpdate = true;
		}
		else {
			shouldPhysicsUpdate = false;
		}
	}

	float time() {
		return currTime;
	}

	float deltaTime() {
		return cdeltaTime;
	}

	float unscaledDeltaTime() {
		return usDeltaTime;
	}

	float fixedDeltaTime() {
		return fixedUpdateDelta;
	}

	bool checkShouldPhysicsUpdate() {
		return shouldPhysicsUpdate;
	}
}