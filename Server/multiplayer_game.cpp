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
	std::uniform_int_distribution<int> dist(0, max - 1);

	return dist(gen);
}

MultiplayerGame::MultiplayerGame(int FPS, USHORT port) 
	: gameClock(FPS), connectionManager(port, 2, packetQueueCapacity, maxPacketSize, std::chrono::seconds(1) / FPS) 
{
	connectionManager.isListening = true;
}

void MultiplayerGame::CheckPlayerReadiness(const std::vector<PlayerInputState>& inputs, PlayerContext& context) {
	for (auto& input : inputs) {
		if (input.keys & PlayerKeyPressed::Space) {
			context.isReady = !context.isReady;
		}	
	}
}

void MultiplayerGame::ProcessMousePositions(const std::vector<PlayerInputState>& inputs, PlayerContext& context) {
	context.slices.clear();
	std::optional<glm::vec2> lastPressed = {};
	for (auto& input : inputs) {
		if (input.keys & PlayerKeyPressed::MouseLeft) {
			if (lastPressed) {
				context.slices.push_back({ *lastPressed, {input.mouseX, input.mouseY} });
				lastPressed = glm::vec2 { input.mouseX, input.mouseY };
			}
			lastPressed = glm::vec2{ input.mouseX, input.mouseY };
		}
		else {
			lastPressed = {};
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
			if (i->result1.WaitForResponse(std::chrono::seconds())) {
				auto response = i->result1.GetResponse();
				if (!response) {
					Debug::LogError("Error: The client 1 refused to respond to fruit slice result");
					i = pendingSlicables.erase(i);
					continue;
				}
				
				if (response->data.size() == 0) {
					Debug::LogError("Error: The client 1 responded with empty message for the fruit slice result");
					i = pendingSlicables.erase(i);
					continue;
				}
				bool isHit = response->data[0];
				if (isHit) {
					context1.score ++;
				}
				else {
					context1.numMisses++;
				}
				i = pendingSlicables.erase(i);
			}
			else {
				++i;
			}
		}

		if (context1.numMisses >= MultiplayerSetting.missTolerence) {
			state = GameState::Wait;
			Debug::Log("Game Over: Player 1 missed too many fruits!");
			SendCommand(ServerPacket::GameLost, player1);
			SendCommand(ServerPacket::GameWon, player2);
			return;
		}
		if (context2.numMisses >= MultiplayerSetting.missTolerence) {
			state = GameState::Wait;
			Debug::Log("Game Over: Player 2 missed too many fruits!");
			SendCommand(ServerPacket::GameLost, player2);
			SendCommand(ServerPacket::GameWon, player1);
			return;
		}

		// Spawn fruits
		float currTime = gameClock.Time();
		if (currTime >= spawnTimer) {
			spawnTimer = currTime + randFloat(MultiplayerSetting.spawnCooldownMin, MultiplayerSetting.spawnCooldownMax);
			spawnIndex++;
			SpawnFruit();
		}
	}
}

void MultiplayerGame::SendUpdate() {
	contextIndex++;
	context1.isConnected = player1 && player1->IsConnected();
	context2.isConnected = player2 && player2->IsConnected();

	if (context1.isConnected) {
		player1->SendData(ServerPacket::SerializeGameState(contextIndex, context1, context2));
	}
	if (context2.isConnected) {
		player2->SendData(ServerPacket::SerializeGameState(contextIndex, context2, context1));
	}
}

void MultiplayerGame::ProcessInput() {
	// Populate player input array and sort by index
	std::vector<PlayerInputState> input1;
	std::vector<PlayerInputState> input2;

	bool disconnect = false;
	if (!player1 || player1->IsDisconnected()) {
		disconnect = true;
		player1 = connectionManager.Accept(timeout, std::chrono::seconds()); 
		context1 = {};
	}
	else if (player1->IsConnected()) {
		for (auto pkt = player1->Receive(); pkt.has_value(); pkt = player1->Receive()) {
			auto clientData = ClientPacket::Deserialize(pkt->data);

			std::visit(
				overload{
					[&](std::monostate) {},
					[&](PlayerInputState input) { input1.push_back(input); },
					[&](std::pair<uint64_t, bool>){  }
				}, clientData
			);
		}
	}

	if (!player2 || player2->IsDisconnected()) {
		disconnect = true;
		player2 = connectionManager.Accept(timeout, std::chrono::seconds()); 
		context2 = {};
	}
	else if (player2->IsConnected()) {
		for (auto pkt = player2->Receive(); pkt.has_value(); pkt = player2->Receive()) {
			auto clientData = ClientPacket::Deserialize(pkt->data);
			std::visit(
				overload{
					[&](std::monostate) {},
					[&](PlayerInputState input) { input2.push_back(input); },
					[&](std::pair<uint64_t, bool>) {}
				}, clientData
			);
		}
	}

	auto comparator = [](const PlayerInputState& a, const PlayerInputState& b) {
		return a.index < b.index;
	};

	std::sort(input1.begin(), input1.end(), comparator);
	std::sort(input2.begin(), input2.end(), comparator);

	if (state == GameState::Game) {
		if (disconnect) {
			objManager.UnregisterAll();
			pendingSlicables.clear();
			state = GameState::Wait;
			Debug::Log("Player Disconnected!");
			SendCommand(ServerPacket::Disconnect);
		}
		else {
			ProcessMousePositions(input1, context1);
			ProcessMousePositions(input2, context2);
		}
	}
	else if (state == GameState::Wait) {
		CheckPlayerReadiness(input1, context1);
		CheckPlayerReadiness(input2, context2);
		if (context1.isReady && context2.isReady) {
			state = GameState::Game;
			SendCommand(ServerPacket::StartGame);
			context1 = {};
			context2 = {};
			Debug::Log("Game Start");
			StartCoroutine(WaitForSeconds(3));
		}
	}
	
	gameClock.Tick();
}

void MultiplayerGame::SendCommand(ServerPacket::ServerCommand cmd) {
	auto signal = ServerPacket::SerializeCommand(cmd);
	if (player1 && player1->IsConnected()) {
		player1->SendReliableData(signal);
	}
	if (player2 && player2->IsConnected()) {
		player2->SendReliableData(signal);
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
	int numFruits = randInt(MultiplayerSetting.spawnAmountMin, MultiplayerSetting.spawnAmountMax);
	for (auto i = 0; i < numFruits;++i) {
		SlicableType fruitType = static_cast<SlicableType>(randInt(0, SlicableType::Count - 1));
		float upForce = randFloat(MultiplayerSetting.fruitUpMin, MultiplayerSetting.fruitUpMax);
		float horizontalForce = randFloat(MultiplayerSetting.fruitHorizontalMin, MultiplayerSetting.fruitHorizontalMax);
		glm::vec2 velocity = { horizontalForce, upForce };

		float startX = randFloat(MultiplayerSetting.fruitSpawnCenter - MultiplayerSetting.fruitSpawnWidth / 2, MultiplayerSetting.fruitSpawnCenter + MultiplayerSetting.fruitSpawnWidth / 2);
		glm::vec2 position = { startX, MultiplayerSetting.fruitSpawnHeight };

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

		if (player1) {
			auto result1 = player1->SendRequest(signal);
			if (result1) {
				wait.result1 = std::move(result1.value());
			}
			else {
				Debug::LogError("Failed to send fruit spawn request to player 1: ");
			}
		}

		if (player2) {
			auto result2 = player2->SendRequest(signal);
			if (result2) {
				wait.result2 = std::move(result2.value());
			}
			else {
				Debug::LogError("Failed to send fruit spawn request to player 2");
			}
		}

		if (wait.result1 || wait.result2) {
			pendingSlicables.push_back(std::move(wait));
		}
	}
}