#ifndef CLOCK_H
#define CLOCK_H
#include <chrono>

class Clock {
private:
	std::chrono::steady_clock::time_point lastTick;
	std::chrono::duration<float> delta = {};
	std::chrono::duration<float> unscaledDelta = {};

	uint32_t physicsFPS;
	std::chrono::duration<float> accumulator;
public:
	float timeScale = 1;

	Clock(uint32_t physicsFPS);
	void Tick();
	bool ShouldUpdatePhysics();
	uint32_t PhysicsFPS() const;
	float DeltaTime() const;
	float UnscaledDeltaTime() const;
	float FixedDeltaTime() const;
};

#endif