#include "clock.hpp"

Clock::Clock(uint32_t physicsFPS) : 
	physicsFPS(physicsFPS), lastTick(std::chrono::steady_clock::now()), 
	accumulator(1.0 / physicsFPS)
{

}

void Clock::Tick() {
	auto now = std::chrono::steady_clock::now();
	unscaledDelta = now - lastTick;
	delta = unscaledDelta * timeScale;
	lastTick = now;

	accumulator += delta;
}

bool Clock::ShouldUpdatePhysics() { 
	auto updateInterval = std::chrono::duration<float>(1.0 / physicsFPS);
	if (accumulator >= updateInterval) {
		accumulator -= updateInterval;
		return true;
	}
	return false; 
}
float Clock::DeltaTime() const { return delta.count(); }
float Clock::UnscaledDeltaTime() const { return unscaledDelta.count(); }
float Clock::FixedDeltaTime() const { return 1.0 / physicsFPS; }
uint32_t Clock::PhysicsFPS() const { return physicsFPS; }