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
			Debug::Log("Player 1 Ready ", context.isReady);
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

void MultiplayerGame::Step() {
	objManager.Tick(gameClock);

	if (state == GameState::Game) {
		// Check fruit slice result for both players
		for (auto i = slicableGroup1.begin(); i != slicableGroup1.end();) {
			if (i->result.WaitForResponse(std::chrono::seconds())) {
				auto response = i->result.GetResponse();
				if (!response) {
					Debug::LogError("Error: The client 1 refused to respond to fruit slice result");
					i = slicableGroup1.erase(i);
					continue;
				}
				
				if (response->data.size() == 0) {
					Debug::LogError("Error: The client 1 responded with empty message for the fruit slice result");
					i = slicableGroup1.erase(i);
					continue;
				}
				bool isHit = response->data[0];
				if (isHit) {
					context1.score += i->score;
				}
				else {
					context1.numMisses++;
				}
				i = slicableGroup1.erase(i);
			}
			else {
				++i;
			}
		}
		for (auto i = slicableGroup2.begin(); i != slicableGroup2.end();) {
			if (i->result.WaitForResponse(std::chrono::seconds())) {
				auto response = i->result.GetResponse();
				if (!response) {
					Debug::LogError("Error: The client 2 refused to respond to fruit slice result");
					i = slicableGroup2.erase(i);
					continue;
				}

				if (response->data.size() == 0) {
					Debug::LogError("Error: The client 2 responded with empty message for the fruit slice result");
					i = slicableGroup2.erase(i);
					continue;
				}
				bool isHit = response->data[0];
				if (isHit) {
					context2.score += i->score;
				}
				else {
					context2.numMisses++;
				}
				i = slicableGroup2.erase(i);
			}
			else {
				++i;
			}
		}

		if (context1.numMisses < context1.maxMisses || context2.numMisses < context2.maxMisses) {
			state = GameState::Wait();
			NotifyGameEnd();
		}

		// Spawn fruits
		float currTime = gameClock.Time();
		if (currTime >= spawnTimer) {
			spawnTimer = currTime + randFloat(MultiplayerSetting.spawnCooldownMin, MultiplayerSetting.spawnCooldownMax);
			spawnIndex++;
			SpawnFruit(player1, player2, slicableGroup1, spawnIndex);
			SpawnFruit(player2, player1, slicableGroup2, spawnIndex);
		}
	}
	SendUpdate();
}

void MultiplayerGame::SendUpdate() {
	context1.index++;
	context2.index++;
	context1.isConnected = player1 && player1->IsConnected();
	context2.isConnected = player2 && player2->IsConnected();

	if (context1.isConnected) {
		player1->SendData(ServerPacket::SerializeGameState(context1, context2));
	}
	if (context2.isConnected) {
		player2->SendData(ServerPacket::SerializeGameState(context2, context1));
	}
}

void MultiplayerGame::ProcessInput() {
	// Populate player input array and sort by index
	std::vector<PlayerInputState> input1;
	std::vector<PlayerInputState> input2;

	if (!player1 || player1->IsDisconnected()) {
		context1.isConnected = true;
		player1 = connectionManager.Accept(timeout, std::chrono::seconds()); 
	}
	else if (player1->IsConnected()) {
		context1.isConnected = false;
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
		context2.isConnected = false;
		player2 = connectionManager.Accept(timeout, std::chrono::seconds()); 
	}
	else if (player2->IsConnected()) {
		context2.isConnected = true;
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
		if (!player1->IsConnected() || !player2->IsConnected()) {
			objManager.UnregisterAll();
			context1 = {};
			context2 = {};
			slicableGroup1.clear();
			slicableGroup2.clear();
			state = GameState::Wait;
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
			NotifyGameStart();
		}
	}
	
	gameClock.Tick();
}

void MultiplayerGame::NotifyGameStart() {
}

void MultiplayerGame::NotifyGameEnd() {
	
}

void MultiplayerGame::SpawnFruit(
	const std::shared_ptr<LiteConnConnection>& player, 
	const std::shared_ptr<LiteConnConnection>& other,
	std::list<SlicableAwaitResult>& slicables,
	uint64_t& spawnIndex
) 
{
	int numFruits = randInt(MultiplayerSetting.spawnAmountMin, MultiplayerSetting.spawnAmountMax);

	for (auto i = 0; i < numFruits;++i) {
		SlicableType fruitType = static_cast<SlicableType>(randInt(0, SlicableType::Count - 1));
		float upForce = randFloat(MultiplayerSetting.fruitUpMin, MultiplayerSetting.fruitUpMax);
		float horizontalForce = randFloat(MultiplayerSetting.fruitHorizontalMin, MultiplayerSetting.fruitHorizontalMax);
		glm::vec3 velocity = glm::vec3(horizontalForce, upForce, 0);

		float startX = randFloat(MultiplayerSetting.fruitSpawnCenter - MultiplayerSetting.fruitSpawnWidth / 2, MultiplayerSetting.fruitSpawnCenter + MultiplayerSetting.fruitSpawnWidth / 2);
		glm::vec3 position(startX, MultiplayerSetting.fruitSpawnHeight, 0);

		auto index = spawnIndex++;

		
	}
}