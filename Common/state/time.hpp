#ifndef TIME_H
#define TIME_H
namespace Time {
	constexpr int PHYSICS_FPS = 60;
	extern float timeScale;
	void updateTime();
	float deltaTime();
	float unscaledDeltaTime();
	float fixedDeltaTime();
	float time();
	bool checkShouldPhysicsUpdate();
}
#endif