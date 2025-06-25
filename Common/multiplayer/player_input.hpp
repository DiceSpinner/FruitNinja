#ifndef PLAYER_INPUT_H
#define PLAYER_INPUT_H
#include <glm/glm.hpp>
#include <utility>
#include <memory>
#include <optional>
#include "networking/lite_conn.hpp"

class PlayerKeyPress {
public:
	enum Key : uint8_t {
		None = 0,
		Space = 1,
		Mouse = 1 << 1,
		R = 1 << 2
	} value;
	PlayerKeyPress(Key value) : value(value) {}
	PlayerKeyPress(uint8_t value) : value(static_cast<Key>(value)) {}

	constexpr operator uint8_t() const {
		return value;
	}
};

#pragma pack(push, 1)
struct PlayerInput {
	PlayerKeyPress keys;
	float mousePosX;
	float mousePosY;

	static std::optional<PlayerInput> Deserialize(const std::span<const char> buffer) {
		if (buffer.size() < sizeof(PlayerInput)) return {};
		auto base = buffer.data();
		float posX, posY;
		auto rawX = ntohl(*reinterpret_cast<const uint32_t*> (base + offsetof(PlayerInput, mousePosX)));
		auto rawY = ntohl(*reinterpret_cast<const uint32_t*> (base + offsetof(PlayerInput, mousePosY)));
		memcpy(&posX, &rawX, sizeof(posX));
		memcpy(&posY, &rawY, sizeof(posY));

		return PlayerInput {
			.keys = *reinterpret_cast<const PlayerKeyPress*>(base),
			.mousePosX = posX, 
			.mousePosY = posY,
		};
	}
	void Serialize(std::span<char> buffer) const {
		if (buffer.size() < sizeof(PlayerInput)) {
			std::cout << "Cannot serialize player input, buffer size too small" << std::endl;
			return;
		}
		auto base = buffer.data();
		memcpy(base, &keys, sizeof(PlayerKeyPress));
		uint32_t posX, posY;
		memcpy(&posX, &mousePosX, sizeof(posX));
		memcpy(&posY, &mousePosY, sizeof(posY));
		posX = htonl(posX);
		posY = htonl(posY);
		memcpy(base + offsetof(PlayerInput, mousePosX), &posX, sizeof(posX));
		memcpy(base + offsetof(PlayerInput, mousePosY), &posY, sizeof(posY));
	}
};
#pragma pack(pop)
#endif