#ifndef GAME_PACKET_TYPE_H
#define GAME_PACKET_TYPE_H
#include <cstdint>
#include <span>
#include <vector>
#include <optional>
#include <variant>
#include <bit>
#include <glm/glm.hpp>
#include <array>
#include <WinSock2.h>
#include "debug/log.hpp"


struct PlayerContext {
	static constexpr size_t MinSize = sizeof(uint8_t) + sizeof(uint32_t) * 4 + sizeof(uint64_t);
	static constexpr size_t POS_SIZE = sizeof(uint32_t) * 4;
	static constexpr uint8_t READY_BIT = 1;
	static constexpr uint8_t BOMB_HIT_BIT = 1 << 1;

	bool isReady = false;
	bool bombHit = false;
	uint32_t maxMisses = 3;
	uint32_t numMisses = 0;
	uint32_t energy = 0;
	uint32_t bombThrowCost = 10;
	uint64_t score = 0;
	std::vector<std::array<glm::vec2, 2>> slices;

	static std::optional<PlayerContext> Deserialize(std::span<const char> buffer) {
		if (buffer.size() < MinSize) return {};
		auto base = buffer.data();
		uint8_t bitfield;
		uint32_t maxMisses, numMisses, energy, bombThrowCost;
		uint64_t score;

		memcpy(&bitfield, std::exchange(base, base + sizeof(uint8_t)), sizeof(uint8_t));
		memcpy(&maxMisses, std::exchange(base, base + sizeof(uint32_t)), sizeof(uint32_t));
		memcpy(&numMisses, std::exchange(base, base + sizeof(uint32_t)), sizeof(uint32_t));
		memcpy(&energy, std::exchange(base, base + sizeof(uint32_t)), sizeof(uint32_t));
		memcpy(&bombThrowCost, std::exchange(base, base + sizeof(uint32_t)), sizeof(uint32_t));
		memcpy(&score, std::exchange(base, base + sizeof(uint64_t)), sizeof(uint64_t));

		PlayerContext context = {
			.isReady = bool(bitfield | READY_BIT),
			.bombHit = bool(bitfield | BOMB_HIT_BIT),
			.maxMisses = ntohl(maxMisses),
			.numMisses = ntohl(numMisses),
			.energy = ntohl(energy),
			.bombThrowCost = ntohl(bombThrowCost),
			.score = ntohll(score)
		};
		auto remainder = buffer.size() - MinSize;
		while (remainder > POS_SIZE) {
			remainder -= POS_SIZE;
			uint32_t x1, y1, x2, y2;
			memcpy(&x1, std::exchange(base, base + sizeof(uint32_t)), sizeof(uint32_t));
			memcpy(&y1, std::exchange(base, base + sizeof(uint32_t)), sizeof(uint32_t));
			memcpy(&x2, std::exchange(base, base + sizeof(uint32_t)), sizeof(uint32_t));
			memcpy(&y2, std::exchange(base, base + sizeof(uint32_t)), sizeof(uint32_t));
			context.slices.push_back({ glm::vec2{ ntohf(x1), ntohf(y1) }, glm::vec2{ ntohf(x2), ntohf(y2) } });
		}
		return context;
	}

	static std::vector<char> Serialize(const PlayerContext& context) {
		std::vector<char> buffer;
		buffer.resize(MinSize + POS_SIZE * context.slices.size());
	}

	static void AppendSerialize(const PlayerContext& context, std::vector<char> buffer) {

	}
};

enum SlicableType : uint8_t {
	Apple,
	Pineapple,
	Watermelon,
	Coconut,
	Bomb,
	Count
};

struct SpawnRequest {
	static constexpr size_t Size =
		sizeof(uint64_t) + // index
		sizeof(uint32_t) * 6 + // position and velocity
		sizeof(uint8_t); // model
		
	uint64_t index;
	glm::vec3 pos;
	glm::vec3 vel;
	uint8_t fruitType;

	static void AppendSerialize(const SpawnRequest& slicable, std::vector<char>& buffer) {
		buffer.resize(buffer.size() + Size);
		auto base = buffer.data() + buffer.size() - Size;
		uint64_t index = htonll(slicable.index);
		memcpy(std::exchange(base, base + sizeof(uint64_t)), &index, sizeof(uint64_t));

		uint32_t posX = htonf(slicable.pos.x);
		memcpy(std::exchange(base, base + sizeof(uint32_t)), &posX, sizeof(uint32_t));

		uint32_t posY = htonf(slicable.pos.y);
		memcpy(std::exchange(base, base + sizeof(uint32_t)), &posY, sizeof(uint32_t));

		uint32_t posZ = htonf(slicable.pos.z);
		memcpy(std::exchange(base, base + sizeof(uint32_t)), &posZ, sizeof(uint32_t));

		uint32_t vX = htonf(slicable.vel.x);
		memcpy(std::exchange(base, base + sizeof(uint32_t)), &vX, sizeof(uint32_t));

		uint32_t vY = htonf(slicable.vel.y);
		memcpy(std::exchange(base, base + sizeof(uint32_t)), &vY, sizeof(uint32_t));

		uint32_t vZ = htonf(slicable.vel.z);
		memcpy(std::exchange(base, base + sizeof(uint32_t)), &vZ, sizeof(uint32_t));

		memcpy(std::exchange(base, base + 1), &slicable.fruitType, 1);
	}

	static std::optional<SpawnRequest> Deserialize(const std::span<const char> buffer) {
		if (buffer.size() < Size) return {};
		auto base = const_cast<char*>(buffer.data());

		uint64_t index;
		memcpy(&index, std::exchange(base, base + sizeof(uint64_t)), sizeof(uint64_t));

		uint32_t posX;
		memcpy(&posX, std::exchange(base, base + sizeof(uint32_t)), sizeof(uint32_t));

		uint32_t posY;
		memcpy(&posY, std::exchange(base, base + sizeof(uint32_t)), sizeof(uint32_t));

		uint32_t posZ;
		memcpy(&posZ, std::exchange(base, base + sizeof(uint32_t)), sizeof(uint32_t));

		uint32_t vX;
		memcpy(&vX, std::exchange(base, base + sizeof(uint32_t)), sizeof(uint32_t));

		uint32_t vY;
		memcpy(&vY, std::exchange(base, base + sizeof(uint32_t)), sizeof(uint32_t));

		uint32_t vZ;
		memcpy(&vZ, std::exchange(base, base + sizeof(uint32_t)), sizeof(uint32_t));

		uint8_t fruitType;
		memcpy(&fruitType, std::exchange(base, base + 1), 1);

		return SpawnRequest{
			.index = ntohll(index),
			.pos = {ntohf(posX), ntohf(posY), ntohf(posZ)},
			.vel = {ntohf(vX), ntohf(vY), ntohf(vZ)},
			.fruitType = fruitType
 		};
	}
};

enum ServerPacketType : uint8_t {
	ForwardPlayerInput,
	ForwardSlicableUpdate,
	ForwardSpawnSlicable,
	ForwardContextUpdate,
	SpawnSlicable,
	ContextUpdate,  // Update player context
	StartGame, // Notify game start
	GameEnd,   // Notify game end
	SetPlayerID, // When first connected with player
	PlayerConnected, // When connected with a player
	PlayerDisconnected,	 // When a player is disconnected
};

namespace ServerPacket {
	static std::vector<char> PlayerDisconnected() {
		return { static_cast<char>(ServerPacketType::PlayerDisconnected) };
	}
	static std::vector<char> PlayerConnected() {
		return { static_cast<char>(ServerPacketType::PlayerConnected) };
	}

	static std::vector<char> GameStart() {
		return { static_cast<char>(ServerPacketType::StartGame) };
	}
	static std::vector<char> GameEnd(const PlayerContext& context1, const PlayerContext& context2) {
		std::vector<char> payload = { static_cast<char>(ServerPacketType::GameEnd) };
		PlayerContext::AppendSerialize(context1, payload);
		PlayerContext::AppendSerialize(context2, payload);
		return payload;
	}

	static std::vector<char> SpawnSlicable(SlicableType type, glm::vec3 pos, glm::vec3 vel, uint64_t index) {
		std::vector<char> payload = { static_cast<char>(ServerPacketType::SpawnSlicable) };
		payload.resize(1 + 6 * sizeof(uint32_t) + sizeof(uint64_t));
		SpawnRequest::AppendSerialize(
			SpawnRequest{
				.index = index,
				.pos = pos,
				.vel = vel,
				.fruitType = type
			}, payload
		);
		return payload;
	}

	static std::vector<char> ForwardSpawnSlicable(SlicableType type, glm::vec3 pos, glm::vec3 vel, uint64_t index) {
		std::vector<char> payload = { static_cast<char>(ServerPacketType::ForwardSpawnSlicable) };
		payload.resize(1 + 6 * sizeof(uint32_t) + sizeof(uint64_t));
		SpawnRequest::AppendSerialize(
			SpawnRequest{
				.index = index,
				.pos = pos,
				.vel = vel,
				.fruitType = type
			}, payload
		);
		return payload;
	}
};

class PlayerKeyPressed {
public:
	enum Key : uint8_t {
		None = 0,
		Space = 1,
		MouseLeft = 1 << 1
	} value;
	PlayerKeyPressed(Key value) : value(value) {}
	PlayerKeyPressed(uint8_t value) : value(static_cast<Key>(value)) {}

	constexpr operator uint8_t() const {
		return value;
	}
};

struct PlayerInputState {
	static constexpr size_t Size = sizeof(uint64_t) + sizeof(PlayerKeyPressed) + sizeof(float) * 2;

	uint64_t index;
	PlayerKeyPressed keys;
	float mouseX;
	float mouseY;

	static std::optional<PlayerInputState> Deserialize(const std::span<const char> buffer) {
		if (buffer.size() < Size) return {};
		auto base = buffer.data();

		uint64_t index;
		uint8_t keys;
		uint32_t posX, posY;
		memcpy(&index, std::exchange(base, base + sizeof(uint64_t)), sizeof(uint64_t));
		memcpy(&keys, std::exchange(base, base + sizeof(PlayerKeyPressed)), sizeof(PlayerKeyPressed));
		memcpy(&posX, std::exchange(base, base + sizeof(posX)), sizeof(posX));
		memcpy(&posY, std::exchange(base, base + sizeof(posY)), sizeof(posY));

		return PlayerInputState{
			.index = ntohll(index),
			.keys = keys,
			.mouseX = ntohf(posX),
			.mouseY = ntohf(posY),
		};
	}
	static void Serialize(const PlayerInputState& state, std::span<char> buffer) {
		if (buffer.size() < Size) {
			std::cout << "Cannot serialize player input, buffer size too small" << std::endl;
			return;
		}
		auto base = buffer.data();
		auto nIndex = htonll(state.index);
		memcpy(std::exchange(base, base + sizeof(uint64_t)), &nIndex, sizeof(uint64_t));
		memcpy(std::exchange(base, base + sizeof(PlayerKeyPressed)), &state.keys, sizeof(PlayerKeyPressed));
		uint32_t posX, posY;
		memcpy(&posX, &state.mouseX, sizeof(posX));
		memcpy(&posY, &state.mouseY, sizeof(posY));
		posX = htonl(posX);
		posY = htonl(posY);
		memcpy(std::exchange(base, base + sizeof(float)), &posX, sizeof(posX));
		memcpy(std::exchange(base, base + sizeof(float)), &posY, sizeof(posY));
	}

	static std::vector<char> Serialize(const PlayerInputState& state) {
		std::vector<char> buffer;
		buffer.resize(Size);

		auto base = buffer.data();
		auto nIndex = htonll(state.index);
		memcpy(std::exchange(base, base + sizeof(uint64_t)), &nIndex, sizeof(uint64_t));
		memcpy(std::exchange(base, base + sizeof(PlayerKeyPressed)), &state.keys, sizeof(PlayerKeyPressed));
		uint32_t posX, posY;
		memcpy(&posX, &state.mouseX, sizeof(posX));
		memcpy(&posY, &state.mouseY, sizeof(posY));
		posX = htonl(posX);
		posY = htonl(posY);
		memcpy(std::exchange(base, base + sizeof(float)), &posX, sizeof(posX));
		memcpy(std::exchange(base, base + sizeof(float)), &posY, sizeof(posY));
	}
};

namespace ClientPacket {
	enum ClientPacketType : uint8_t {
		Input,
		SliceResult
	};

	static std::vector<char> Serialize(const PlayerInputState& inputState) {
		std::vector<char> buffer;
		buffer.resize(sizeof(ClientPacketType) + PlayerInputState::Size);
		auto base = buffer.data();
		memset(std::exchange(base, base + sizeof(ClientPacketType)), Input, sizeof(ClientPacketType));
		PlayerInputState::Serialize(inputState, { base, PlayerInputState::Size });
		return buffer;
	}

	static std::vector<char> Serialize(const std::pair<uint64_t, bool>& sliceResult) {
		std::vector<char> buffer;
		buffer.resize(2 + sizeof(uint64_t));
		auto base = buffer.data();
		memset(std::exchange(base, base + sizeof(ClientPacketType)), SliceResult, sizeof(ClientPacketType));
		auto id = htonll(sliceResult.first);
		memset(std::exchange(base, base + sizeof(uint64_t)), sliceResult.first, sizeof(uint64_t));
		memset(std::exchange(base, base + 1), sliceResult.second, 1);
		return buffer;
	}

	static std::variant<PlayerInputState, std::pair<uint64_t, bool>, std::monostate> Deserialize(std::span<const char> buffer) {
		if (buffer.size() < 1) return  std::monostate{};
		ClientPacketType type;

		auto base = buffer.data();
		memcpy(&type, std::exchange(base, base + sizeof(ClientPacketType)), sizeof(ClientPacketType));
		if (type == Input) {
			auto input = PlayerInputState::Deserialize({ base, buffer.size() - 1 });
			if (input) {
				return input.value();
			}
			return std::monostate{};
		}
		if (type == SliceResult) {
			if (buffer.size() - 1 < sizeof(uint64_t) + 1) {
				return  std::monostate{};
			}
			uint64_t id;
			bool isHit;
			memcpy(&id, std::exchange(base, base + sizeof(uint64_t)), sizeof(uint64_t));
			memcpy(&isHit, std::exchange(base, base + 1), 1);

			return std::pair<uint64_t, bool>(ntohll(id) , isHit);
		}
		return  std::monostate{};
	}
};
#endif