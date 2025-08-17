#ifndef MULTIPLAYER_GAME_H
#define MULTIPLAYER_GAME_H
#include "infrastructure/coroutine.hpp"
#include "infrastructure/object.hpp"
#include "multiplayer/game_packet.hpp"
#include "infrastructure/clock.hpp"
#include "networking/lite_conn.hpp"

struct SlicableAwaitResult {
	uint32_t index = 0;
	bool isBomb = false;
	LiteConnResponse results[2];
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

	uint64_t contextIndex = 0;
	PlayerContext contexts[2] = {};
	std::shared_ptr<LiteConnConnection> players[2];

	std::list<SlicableAwaitResult> pendingSlicables;

	float spawnTimer = 0;
	uint64_t spawnIndex = 0;

	ObjectManager objManager = {};
	LiteConnManager connectionManager;
	CoroutineManager coroutineManager;
	Clock gameClock;

	GameState state = GameState::Wait;

	void Step();
	void StartCoroutine(Coroutine&& coroutine);

	Coroutine WaitForSeconds(float time);

	void SendCommand(ServerPacket::ServerCommand cmd);
	void SendCommand(ServerPacket::ServerCommand cmd, std::shared_ptr<LiteConnConnection>& player);
	void CheckPlayerReadiness(const std::vector<PlayerInputState>(&inputs)[2]);
	void ProcessMousePositions(const std::vector<PlayerInputState> (&inputs)[2]);
	void SpawnFruit();
public:
	MultiplayerGame(int tickRate, USHORT port);
	void AdvanceGameState();
	void ProcessInput();
	void SendUpdate();
};
#endif