#ifndef MULTIPLAYER_SETTING
#define MULTIPLAYER_SETTING
#include <glm/glm.hpp>
#include <winsock2.h>
#include "networking/lite_conn.hpp"

static TimeoutSetting ConnectionTimeOut {
	.connectionTimeout = std::chrono::seconds(20),
	.connectionRetryInterval = std::chrono::milliseconds(100),
	.impRetryInterval = std::chrono::milliseconds(50),
	.replyKeepDuration = std::chrono::seconds(5)
};

namespace MTP_Setting {
	constexpr float fruitPlaneZ = 0;
	constexpr float bombPlaneZ = 5;

	constexpr float sizeWatermelon = 1.5f;
	constexpr float sizePineapple = 1.0f;
	constexpr float sizeApple = 1.0f;
	constexpr float sizeCoconut = 1.2f;
	constexpr float sizeBomb = 0.8f;

	constexpr float slicableSizes[5] = { sizeApple, sizePineapple, sizeWatermelon, sizeCoconut, sizeBomb };

	constexpr float fruitSpawnCenter = 0;
	constexpr float fruitSpawnWidth = 15;
	constexpr float fruitSpawnHeight = -13;
	constexpr float fruitKillHeight = -15;
	constexpr float bombKillHeight = -40;
	constexpr float fruitUpMax = 32;
	constexpr float fruitUpMin = 25;
	constexpr float fruitHorizontalMin = -7;
	constexpr float fruitHorizontalMax = 7;
	constexpr float fruitSliceForce = 10;

	constexpr int spawnAmountMin = 1;
	constexpr int spawnAmountMax = 5;
	constexpr float spawnCooldownMin = 0.5;
	constexpr float spawnCooldownMax = 3;
	constexpr glm::vec3 spawnMinRotation = { 30, 30, 30 };
	constexpr glm::vec3 spawnMaxRotation = { 90, 90, 90 };
	constexpr uint32_t missTolerence = 4;
	constexpr uint32_t bombCost = 10;
}
#endif