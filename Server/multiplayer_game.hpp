#ifndef MULTIPLAYER_GAME_H
#define MULTIPLAYER_GAME_H
#include "infrastructure/object.hpp"
#include "multiplayer/object_data.hpp"
#include "multiplayer/game_packet.hpp"
#include "multiplayer/player_input.hpp"

struct PlayerContext {
	bool isReady = false;
	bool bombHit = false;
	int maxMisses = 3;
	int numMisses = 0;
	int score = 0;
	int energy = 0;
	int bombThrowCost = 10;
	std::optional<std::pair<glm::vec2, glm::vec2>> slice;
};

class Game {
public:
	enum class GameState {
		Wait,
		Game,
		Score
	};
private:
	PlayerContext context1 = {};
	PlayerContext context2 = {};
	ObjectManager objManager = {};

	GameState state = GameState::Wait;
public:
	Game();
	void Step(const Clock& clock, std::shared_ptr<UDPConnection>& player1, std::shared_ptr<UDPConnection>& player2);
	void ProcessInput(std::shared_ptr<UDPConnection>& player1, std::shared_ptr<UDPConnection>& player2);
	void Reset();
};
#endif