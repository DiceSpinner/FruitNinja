#include "state.hpp"

namespace Game {
	State state = State::START;
	int score = 0;
	int misses = 0;
	int recovery = 0;
	bool bombHit = false;
	bool recentlyRecovered = false;
	glm::vec3 explosionPosition(0, 0, 0);
	float explosionTimer = 0;
}
