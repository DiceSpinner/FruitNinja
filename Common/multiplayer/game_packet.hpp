#ifndef GAME_PACKET_TYPE_H
#define GAME_PACKET_TYPE_H
#include <cstdint>
#include <span>
#include <vector>
#include <optional>
#include <variant>
#include <tuple>
#include <bit>
#include <glm/glm.hpp>
#include <array>
#include <WinSock2.h>
#include "debug/log.hpp"


struct PlayerContext {
	static constexpr size_t MinSize = sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t) * 2 + sizeof(uint64_t) * 1;
	static constexpr size_t POS_SIZE = sizeof(uint32_t) * 4;
	static constexpr uint8_t CONNECT_BIT = 1;
	static constexpr uint8_t READY_BIT = 1 << 1;
	static constexpr uint8_t BOMB_HIT_BIT = 1 << 2;

	bool isConnected = false;
	bool isReady = false;
	bool bombHit = false;
	uint32_t numMisses = 0;
	uint32_t energy = 0;
	uint64_t score = 0;
	std::vector<std::pair<glm::vec2, glm::vec2>> slices;

	static std::optional<PlayerContext> Deserialize(std::span<const char>& buffer) {
		if (buffer.size() < MinSize) {
			Debug::LogError("Cannot deserialize PlayerContext: Not enough space in the buffer!");
			return {}; 
		}
		auto base = buffer.data();
		uint8_t bitfield;
		uint32_t numMisses, energy, bombThrowCost;
		uint64_t score;

		memcpy(&bitfield, std::exchange(base, base + sizeof(uint8_t)), sizeof(uint8_t));
		memcpy(&numMisses, std::exchange(base, base + sizeof(uint32_t)), sizeof(uint32_t));
		memcpy(&energy, std::exchange(base, base + sizeof(uint32_t)), sizeof(uint32_t));
		memcpy(&score, std::exchange(base, base + sizeof(uint64_t)), sizeof(uint64_t));

		PlayerContext context = {
			.isConnected = bool(bitfield & CONNECT_BIT),
			.isReady = bool(bitfield & READY_BIT),
			.bombHit = bool(bitfield & BOMB_HIT_BIT),
			.numMisses = ntohl(numMisses),
			.energy = ntohl(energy),
			.score = ntohll(score)
		};

		uint16_t numSlices;
		memcpy(&numSlices, std::exchange(base, base + sizeof(uint16_t)), sizeof(uint16_t));
		numSlices = ntohs(numSlices);

		auto remainder = buffer.size() - MinSize;
		if (numSlices * POS_SIZE > remainder) {
			Debug::LogError("Cannot deserialize player context: Buffer does not contain enough mouse positions as advertised.");
			return {};
		}

		while (numSlices-- != 0) {
			remainder -= POS_SIZE;
			uint32_t x1, y1, x2, y2;
			memcpy(&x1, std::exchange(base, base + sizeof(uint32_t)), sizeof(uint32_t));
			memcpy(&y1, std::exchange(base, base + sizeof(uint32_t)), sizeof(uint32_t));
			memcpy(&x2, std::exchange(base, base + sizeof(uint32_t)), sizeof(uint32_t));
			memcpy(&y2, std::exchange(base, base + sizeof(uint32_t)), sizeof(uint32_t));
			context.slices.push_back({ glm::vec2{ ntohf(x1), ntohf(y1) }, glm::vec2{ ntohf(x2), ntohf(y2) } });
		}
		buffer = buffer.subspan(base -  buffer.data());
		return context;
	}

	static void AppendSerialize(const PlayerContext& context, std::vector<char>& buffer) {
		auto originalSize = buffer.size();
		buffer.resize(originalSize + MinSize + context.slices.size() * POS_SIZE);
		auto base = buffer.data() + originalSize;

		uint8_t flags = 0;
		if (context.isConnected) {
			flags |= CONNECT_BIT;
		}
		if (context.isReady) {
			flags |= READY_BIT;
		}
		if (context.bombHit) {
			flags |= BOMB_HIT_BIT;
		}
		memcpy(std::exchange(base, base + 1), &flags, 1);

		uint32_t numMisses = htonl(context.numMisses);
		memcpy(std::exchange(base, base + sizeof(uint32_t)), &numMisses, sizeof(uint32_t));

		uint32_t energy = htonl(context.energy);
		memcpy(std::exchange(base, base + sizeof(uint32_t)), &energy, sizeof(uint32_t));

		uint64_t score = htonll(context.score);
		memcpy(std::exchange(base, base + sizeof(uint64_t)), &score, sizeof(uint64_t));

		uint16_t numSlices = htons(context.slices.size());
		memcpy(std::exchange(base, base + sizeof(uint16_t)), &numSlices, sizeof(uint16_t));

		for (auto i = context.slices.begin(); i != context.slices.end();++i) {
			uint32_t x1 = htonf(i->first.x);
			uint32_t y1 = htonf(i->first.y);
			uint32_t x2 = htonf(i->second.x);
			uint32_t y2 = htonf(i->second.y);

			memcpy(std::exchange(base, base + sizeof(uint32_t)), &x1, sizeof(uint32_t));
			memcpy(std::exchange(base, base + sizeof(uint32_t)), &y1, sizeof(uint32_t));
			memcpy(std::exchange(base, base + sizeof(uint32_t)), &x2, sizeof(uint32_t));
			memcpy(std::exchange(base, base + sizeof(uint32_t)), &y2, sizeof(uint32_t));
		}
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
		sizeof(uint32_t) * 4 + // position and velocity
		sizeof(uint8_t); // model
		
	uint64_t index;
	glm::vec2 pos;
	glm::vec2 vel;
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

		uint32_t vX = htonf(slicable.vel.x);
		memcpy(std::exchange(base, base + sizeof(uint32_t)), &vX, sizeof(uint32_t));

		uint32_t vY = htonf(slicable.vel.y);
		memcpy(std::exchange(base, base + sizeof(uint32_t)), &vY, sizeof(uint32_t));

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

		uint32_t vX;
		memcpy(&vX, std::exchange(base, base + sizeof(uint32_t)), sizeof(uint32_t));

		uint32_t vY;
		memcpy(&vY, std::exchange(base, base + sizeof(uint32_t)), sizeof(uint32_t));

		uint8_t fruitType;
		memcpy(&fruitType, std::exchange(base, base + 1), 1);

		return SpawnRequest{
			.index = ntohll(index),
			.pos = {ntohf(posX), ntohf(posY)},
			.vel = {ntohf(vX), ntohf(vY)},
			.fruitType = fruitType
 		};
	}
};

namespace ServerPacket {
	enum ServerPacketType : uint8_t {
		SlicableUpdate,
		SpawnSlicable,
		ContextUpdate,  // Update player context
		Command,
	};

	enum ServerCommand : uint8_t {
		StartGame,
		Disconnect,
		GameDraw,
		GameWon,
		GameLost
	};

	std::variant<std::tuple<uint64_t, PlayerContext, PlayerContext>, ServerCommand, SpawnRequest, std::monostate> Deserialize(std::span<const char> buffer);

	std::vector<char> SerializeGameState(const uint64_t index, const PlayerContext& context1, const PlayerContext& context2);

	std::vector<char> SerializeCommand(const ServerCommand& cmd);

	std::vector<char> SerializeSpawnRequest(const SpawnRequest& request);
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

	std::vector<char> Serialize(const PlayerInputState& inputState);

	std::vector<char> Serialize(const std::pair<uint64_t, bool>& sliceResult);

	std::variant<PlayerInputState, std::pair<uint64_t, bool>, std::monostate> Deserialize(std::span<const char> buffer);
};
#endif