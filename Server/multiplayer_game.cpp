#include <cmath>
#include "multiplayer_game.hpp"
#include "multiplayer/setting.hpp"

template<typename... T>
struct overload : T... {
	using T::operator()...;
};

static float randFloat(float min, float max) {
	return rand() / static_cast<float>(RAND_MAX) * (max - min) + min;
}

static int randInt(int min, int max) {
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dist(0, max);

	return dist(gen);
}

MultiplayerGame::MultiplayerGame(int FPS, USHORT port) 
	: gameClock(FPS), connectionManager(port, 2, packetQueueCapacity, maxPacketSize, std::chrono::seconds(1) / FPS) 
{
	connectionManager.isListening = true;
}

void MultiplayerGame::CheckPlayerReadiness(const std::vector<PlayerInputState>(&inputs)[2]) {
	for (auto i = 0; i < 2; i++) {
		for (auto& input : inputs[i]) {
			if (input.keys & PlayerKeyPressed::Space) {
				contexts[i].isReady = !contexts[i].isReady;
			}
		}
	}
}

void MultiplayerGame::ProcessMousePositions(const std::vector<PlayerInputState>(&inputs)[2]) {
	for (auto i = 0; i < 2; i++) {
		contexts[i].slices.clear();
		std::optional<glm::vec2> lastPressed = {};
		for (auto& input : inputs[i]) {
			if (input.keys & PlayerKeyPressed::MouseLeft) {
				if (lastPressed) {
					contexts[i].slices.push_back({*lastPressed, {input.mouseX, input.mouseY}});
					lastPressed = glm::vec2{ input.mouseX, input.mouseY };
				}
				lastPressed = glm::vec2{ input.mouseX, input.mouseY };
			}
			else {
				lastPressed = {};
			}
		}
	}
}

Coroutine MultiplayerGame::WaitForSeconds(float time) {
	co_yield Coroutine::YieldOption::Wait(time);
}

void MultiplayerGame::StartCoroutine(Coroutine&& coroutine) {
	coroutineManager.AddCoroutine(std::forward<Coroutine>(coroutine));
}

void MultiplayerGame::AdvanceGameState() {
	objManager.Tick(gameClock);
	if (!coroutineManager.Empty()) {
		coroutineManager.Run(gameClock);
	}
	else {
		Step();
	}
}

void MultiplayerGame::Step() {
	if (state == GameState::Game) {
		// Check fruit slice result for both players

		for (auto i = pendingSlicables.begin(); i != pendingSlicables.end();) {
			bool slicableFinished = true;
			for (auto playerID = 0; playerID < 2; playerID++) {
				if (!i->results[playerID]) continue;
				if (i->results[playerID].WaitForResponse(std::chrono::seconds())) {
					auto pkt = i->results[playerID].GetResponse();
					if (!pkt) {
						Debug::LogError("Error: The client ", (playerID + 1), " refused to respond to fruit slice result");
						continue;
					}
					auto response = SliceResult::Deserialize(pkt->data);
					if (!response) {
						Debug::LogError("Error: Failed to deserialize client ", (playerID + 1), " slice result");
						continue;
					}

					if (response->isSliced) {
						Debug::Log("Client ", (playerID + 1), " sliced slicable!");
						contexts[playerID].score++;
					}
					else {
						Debug::Log("Client ", (playerID + 1), " missed slicable");
						// contexts[playerID].numMisses++;
					}
				}
				else {
					slicableFinished = false;
				}
			}

			if (slicableFinished) {
				i = pendingSlicables.erase(i);
			}
			else {
				++i;
			}
		}

		if (contexts[0].numMisses >= MTP_Setting::missTolerence && contexts[1].numMisses >= MTP_Setting::missTolerence) {
			state = GameState::Wait;
			Debug::Log("Game Over: Player 1 missed too many fruits!");
			SendCommand(ServerPacket::GameDraw, players[0]);
			SendCommand(ServerPacket::GameDraw, players[1]);
			return;
		}

		if (contexts[0].numMisses >= MTP_Setting::missTolerence) {
			state = GameState::Wait;
			Debug::Log("Game Over: Player 1 missed too many fruits!");
			SendCommand(ServerPacket::GameLost, players[0]);
			SendCommand(ServerPacket::GameWon, players[1]);
			return;
		}
		if (contexts[1].numMisses >= MTP_Setting::missTolerence) {
			state = GameState::Wait;
			Debug::Log("Game Over: Player 2 missed too many fruits!");
			SendCommand(ServerPacket::GameLost, players[1]);
			SendCommand(ServerPacket::GameWon, players[0]);
			return;
		}

		// Spawn fruits
		float currTime = gameClock.Time();
		if (currTime >= spawnTimer) {
			spawnTimer = currTime + randFloat(MTP_Setting::spawnCooldownMin, MTP_Setting::spawnCooldownMax);
			spawnIndex++;
			SpawnFruit();
		}
	}
}

void MultiplayerGame::SendUpdate() {
	contextIndex++;
	contexts[0].isConnected = players[0] && players[0]->IsConnected();
	contexts[1].isConnected = players[1] && players[1]->IsConnected();

	if (contexts[0].isConnected) {
		players[0]->SendData(ServerPacket::SerializeGameState(contextIndex, contexts[0], contexts[1]));
	}
	if (contexts[1].isConnected) {
		players[1]->SendData(ServerPacket::SerializeGameState(contextIndex, contexts[1], contexts[0]));
	}
}

void MultiplayerGame::ProcessInput() {
	// Populate player input array and sort by index
	std::vector<PlayerInputState> inputs[2];

	bool disconnect = false;
	for (auto i = 0; i < 2; i++) {
		if (!players[i] || players[i]->IsDisconnected()) {
			disconnect = true;
			players[i] = connectionManager.Accept(timeout, std::chrono::seconds());
			contexts[i] = {};
		}
		else if (players[i]->IsConnected()) {
			for (auto pkt = players[i]->Receive(); pkt.has_value(); pkt = players[i]->Receive()) {
				auto clientData = ClientPacket::Deserialize(pkt->data);

				std::visit(
					overload{
						[&](std::monostate) {},
						[&](PlayerInputState input) { inputs[i].push_back(input); },
					}, clientData
				);
			}
		}
	}

	auto comparator = [](const PlayerInputState& a, const PlayerInputState& b) {
		return a.index < b.index;
	};

	std::sort(inputs[0].begin(), inputs[0].end(), comparator);
	std::sort(inputs[1].begin(), inputs[1].end(), comparator);

	if (state == GameState::Game) {
		if (disconnect) {
			objManager.UnregisterAll();
			pendingSlicables.clear();
			state = GameState::Wait;
			Debug::Log("Player Disconnected!");
			SendCommand(ServerPacket::Disconnect);
		}
		else {
			ProcessMousePositions(inputs);
		}
	}
	else if (state == GameState::Wait) {
		CheckPlayerReadiness(inputs);
		if (contexts[0].isReady && contexts[1].isReady) {
			state = GameState::Game;
			SendCommand(ServerPacket::StartGame);
			contexts[0] = {};
			contexts[1] = {};
			Debug::Log("Game Start");
			StartCoroutine(WaitForSeconds(3));
		}
	}
	
	gameClock.Tick();
}

void MultiplayerGame::SendCommand(ServerPacket::ServerCommand cmd) {
	auto signal = ServerPacket::SerializeCommand(cmd);
	if (players[0] && players[0]->IsConnected()) {
		players[0]->SendReliableData(signal);
	}
	if (players[1] && players[1]->IsConnected()) {
		players[1]->SendReliableData(signal);
	}
}

void MultiplayerGame::SendCommand(ServerPacket::ServerCommand cmd, std::shared_ptr<LiteConnConnection>& player) {
	auto signal = ServerPacket::SerializeCommand(cmd);
	if (player && player->IsConnected()) {
		player->SendReliableData(signal);
	}
}

void MultiplayerGame::SpawnFruit() 
{
	int numFruits = randInt(MTP_Setting::spawnAmountMin, MTP_Setting::spawnAmountMax);
	for (auto i = 0; i < numFruits;++i) {
		SlicableType fruitType = static_cast<SlicableType>(randInt(0, SlicableType::Count - 1));
		float upForce = randFloat(MTP_Setting::fruitUpMin, MTP_Setting::fruitUpMax);
		float horizontalForce = randFloat(MTP_Setting::fruitHorizontalMin, MTP_Setting::fruitHorizontalMax);
		glm::vec2 velocity = { horizontalForce, upForce };

		float startX = randFloat(MTP_Setting::fruitSpawnCenter - MTP_Setting::fruitSpawnWidth / 2, MTP_Setting::fruitSpawnCenter + MTP_Setting::fruitSpawnWidth / 2);
		glm::vec2 position = { startX, MTP_Setting::fruitSpawnHeight };

		auto index = spawnIndex++;

		auto signal = ServerPacket::SerializeSpawnRequest(
			SpawnRequest{
					.index = index,
					.pos = position,
					.vel = velocity,
					.fruitType = fruitType
			});
		SlicableAwaitResult wait;
		wait.isBomb = fruitType == SlicableType::Bomb;

		if (players[0]) {
			auto result1 = players[0]->SendRequest(signal);
			if (result1) {
				wait.results[0] = std::move(result1.value());
			}
			else {
				Debug::LogError("Failed to send fruit spawn request to player 1: ");
			}
		}

		if (players[1]) {
			auto result2 = players[1]->SendRequest(signal);
			if (result2) {
				wait.results[1] = std::move(result2.value());
			}
			else {
				Debug::LogError("Failed to send fruit spawn request to player 2");
			}
		}

		if (wait.results[0] || wait.results[1]) {
			pendingSlicables.push_back(std::move(wait));
		}
	}
}