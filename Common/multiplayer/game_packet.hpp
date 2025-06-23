#ifndef GAME_PACKET_TYPE_H
#define GAME_PACKET_TYPE_H
#include <cstdint>

enum GamePacketType : uint8_t {
	PlayerInput = 0,
	ObjectData = 1,
	Summary = 2,
	PlayerConnect = 3,
	PlayerDisconnect = 4,	
};
#endif