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

constexpr struct {
	float sizeWatermelon = 1.5f;
	float sizePineapple = 1.0f;
	float sizeApple = 1.0f;
	float sizeCoconut = 1.2f;
	float sizeBomb = 0.8f;
	float fruitSpawnCenter = 0;
	float fruitSpawnWidth = 15;
	float fruitSpawnHeight = -13;
	float fruitKillHeight = -15;
	float bombKillHeight = -40;
	float fruitUpMax = 32;
	float fruitUpMin = 25;
	float fruitHorizontalMin = -7;
	float fruitHorizontalMax = 7;
	float fruitSliceForce = 10;

	int spawnAmountMin = 1;
	int spawnAmountMax = 5;
	float spawnCooldownMin = 0.5;
	float spawnCooldownMax = 3;
	glm::vec3 spawnMinRotation = { 30, 30, 30 };
	glm::vec3 spawnMaxRotation = { 90, 90, 90 };
	unsigned int missTolerence = 3;
} MultiplayerSetting;
#endif