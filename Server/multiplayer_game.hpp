#ifndef MULTIPLAYER_GAME_H
#define MULTIPLAYER_GAME_H
#include "infrastructure/object.hpp"
#include "multiplayer/game_packet.hpp"
#include "infrastructure/clock.hpp"
#include "networking/lite_conn.hpp"

struct SlicableAwaitResult {
	bool isBomb;
	uint32_t score;
	LiteConnResponse result;
};

class MultiplayerGame {
public:
	enum class GameState {
		Wait,
		Game
	};
private:
	static constexpr TimeoutSetting timeout = {
		.connectionTimeout = std::chrono::seconds(5),
		.connectionRetryInterval = std::chrono::milliseconds(50),
		.impRetryInterval = std::chrono::milliseconds(100),
		.replyKeepDuration = std::chrono::seconds(5)
	};

	static constexpr int numConnections = 2;
	static constexpr int packetQueueCapacity = 100;
	static constexpr int maxPacketSize = 1500;

	PlayerContext context1 = {};
	PlayerContext context2 = {};
	std::shared_ptr<LiteConnConnection> player1;
	std::shared_ptr<LiteConnConnection> player2;
	
	std::list<SlicableAwaitResult> slicableGroup1 = {};
	std::list<SlicableAwaitResult> slicableGroup2 = {};

	float spawnTimer1 = 0;
	uint64_t spawnIndex1 = 0;
	
	float spawnTimer2 = 0;
	uint64_t spawnIndex2 = 0;

	ObjectManager objManager = {};
	LiteConnManager connectionManager;
	Clock gameClock;

	GameState state = GameState::Wait;

	void NotifyDisconnect();
	void NotifyGameEnd();
	void NotifyGameStart();
	void CheckPlayerReadiness(const std::vector<PlayerInputState>& inputs, PlayerContext& context);
	void ProcessMousePositions(const std::vector<PlayerInputState>& inputs, PlayerContext& context);
	void SpawnFruit(
		const std::shared_ptr<LiteConnConnection>& player,
		const std::shared_ptr<LiteConnConnection>& otherPlayer,
		std::list<SlicableAwaitResult>& playerSlicables,
		uint64_t& spawnIndex
	);
public:
	MultiplayerGame(int tickRate, USHORT port);
	void Step();
	void ProcessInput();
	void SendUpdate();
};
#endif