#ifndef STATE_H
#define STATE_H
#include <glm/glm.hpp>

enum State {
	START,
	GAME,
	EXPLOSION,
	SCORE
};

constexpr float EXPLOSION_DURATION = 2;

namespace Game {
	extern State state;
	extern int score;
	extern int misses;
	extern int recovery;
	extern bool bombHit;
	extern bool recentlyRecovered;
	extern glm::vec3 explosionPosition;
	extern float explosionTimer;
}
#endif