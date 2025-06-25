#ifndef SERVER_STATE_H
#define SERVER_STATE_H
#include <iostream>
#include <unordered_map>
#include <optional>
#include <typeindex>
#include <memory>
#include <concepts>
#include "infrastructure/clock.hpp"
#include "multiplayer/game_packet.hpp"
#include "multiplayer_game.hpp"
#include "multiplayer/player_input.hpp"

class Server {
private:
	TimeoutSetting timeout;
	Clock gameClock;

	std::unique_ptr<LiteConnManager> connectionManager;
	Game game;
	std::shared_ptr<LiteConnConnection> player1;
	std::shared_ptr<LiteConnConnection> player2;

	enum ServerState {
		OnHold,
		InGame,
		Score
	};

	std::atomic<ServerState> state;
public:
	std::atomic<bool> running;

	Server(TimeoutSetting timeout, uint32_t fps);
	Server(const Server& other) = delete;
	Server(Server&& other) = delete;
	Server& operator = (const Server& other) = delete;
	Server& operator = (Server&& other) = delete;

	void OnEnterState();
	void OnExitState();
	void ChangeState(ServerState state);
	void UpdateState();
	void Terminate();
	void ProcessInput();
	void Step();
	void DrawBackUI();
	void DrawObjects();
	void DrawFrontUI();
};

#endif