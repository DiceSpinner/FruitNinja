#include <chrono>
#include "time.hpp"

using namespace std;

static chrono::time_point<chrono::high_resolution_clock> timeAnchor;
static float currTime = 0;
static float cdeltaTime = 0;
static float usDeltaTime = 0;
static bool init = true;
static float fixedUpdateDelta = 0;
static float fixedUpdateAccumulator = 0;
static bool shouldPhysicsUpdate = false;

namespace Time {
	float timeScale = 1;

	void initTime() {
		currTime = chrono::duration<float>(chrono::steady_clock::now() - timeAnchor).count();
	}

	void updateTime() {
		float currentTime = chrono::duration<float>(chrono::steady_clock::now() - timeAnchor).count();
		cdeltaTime = currentTime - currTime;
		usDeltaTime = cdeltaTime;
		cdeltaTime *= timeScale;
		currTime = currentTime;
		fixedUpdateAccumulator += cdeltaTime;

		// Has upadted physics last frame, reset fixedUpdateDelta
		if (shouldPhysicsUpdate) {
			fixedUpdateDelta = 0;
		}
		fixedUpdateDelta += cdeltaTime;
		
		if (fixedUpdateAccumulator >= 1.0f / PHYSICS_FPS) {
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