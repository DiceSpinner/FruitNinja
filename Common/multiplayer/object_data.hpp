#ifndef OBJECT_DATA_H
#define OBJECT_DATA_H
#include <cstdint>
#include <span>
#include <iostream>
#include <optional>
#include <bit>
#include <WinSock2.h>

enum ModelIndex : uint8_t {
	Apple = 0,
	Pineapple = 1,
	Watermelon = 2,
	Coconut = 3,
	Bomb = 4
};

enum ObjectOpcode : uint8_t {
	None = 0,
	Create = 1,
	Destroy = 2
};

struct ObjectData {
	static constexpr size_t serializedSize =
		sizeof(uint8_t) + // opcode
		sizeof(uint8_t) + // model
		sizeof(uint32_t) + // index
		sizeof(uint32_t) * 3; // posX, posY, posZ

	uint8_t opcode;
	uint8_t model;
	uint32_t index;
	float posX;
	float posY;
	float posZ;

	static std::optional<ObjectData> Deserialize(std::span<const char> buffer) {
		if (buffer.size() < serializedSize) {
			std::cout << "Cannot deserialize object data, buffer size too small" << std::endl;
			return {};
		}

		auto base = buffer.data();
		uint8_t opcode, model;
		memcpy(&opcode, base, sizeof(uint8_t));
		base += sizeof(decltype(ObjectData().opcode));
		memcpy(&model, base, sizeof(uint8_t));
		base += sizeof(decltype(ObjectData().model));
		uint32_t index;
		memcpy(&index, base, sizeof(index));
		base += sizeof(decltype(ObjectData().index));

		uint32_t rawX, rawY, rawZ;
		memcpy(&rawX, base, sizeof(uint32_t));
		base += sizeof(decltype(ObjectData().posX));
		memcpy(&rawY, base, sizeof(uint32_t));
		base += sizeof(decltype(ObjectData().posY));
		memcpy(&rawZ, base, sizeof(uint32_t));

		return ObjectData {
			.opcode = opcode,
			.model = model, 
			.index = ntohl(index),
			.posX = std::bit_cast<float>(ntohl(rawX)),
			.posY = std::bit_cast<float>(ntohl(rawY)),
			.posZ = std::bit_cast<float>(ntohl(rawZ))
		};
	}

	void Serialize(std::span<char> buffer) const {
		if (buffer.size() < serializedSize) {
			std::cout << "Cannot serialize object data, buffer size too small" << std::endl;
			return;
		}

		auto base = buffer.data();
		memcpy(base, &opcode, sizeof(uint8_t));
		base += sizeof(opcode);
		memcpy(base, &model, sizeof(uint8_t));
		base += sizeof(model);
		auto nIndex = htonl(index);
		memcpy(base, &nIndex, sizeof(uint32_t));
		base += sizeof(index);
		auto rawX = htonl(std::bit_cast<uint32_t>(posX));
		memcpy(base, &rawX, sizeof(uint32_t));
		base += sizeof(posX);
		auto rawY = htonl(std::bit_cast<uint32_t>(posY));
		memcpy(base, &rawY, sizeof(uint32_t));
		base += sizeof(posY);
		auto rawZ = htonl(std::bit_cast<uint32_t>(posZ));
		memcpy(base, &rawZ, sizeof(uint32_t));
	}
};

#endif