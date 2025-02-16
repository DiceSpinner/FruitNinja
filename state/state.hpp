#ifndef STATE_H
#define STATE_H
enum State {
	START,
	GAME,
	EXPLOSION,
	SCORE
};

namespace Game {
	extern State state;
	extern int score;
	extern int misses;
	extern int recovery;
	extern bool bombHit;
	extern bool recentlyRecovered;
}
#endif