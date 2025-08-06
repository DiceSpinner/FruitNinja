#include "game_packet.hpp"

namespace ClientPacket {
	std::vector<char> Serialize(const PlayerInputState& inputState) {
		std::vector<char> buffer;
		buffer.resize(sizeof(ClientPacketType) + PlayerInputState::Size);
		auto base = buffer.data();
		memset(std::exchange(base, base + sizeof(ClientPacketType)), Input, sizeof(ClientPacketType));
		PlayerInputState::Serialize(inputState, { base, PlayerInputState::Size });
		return buffer;
	}

	std::vector<char> Serialize(const std::pair<uint64_t, bool>& sliceResult) {
		std::vector<char> buffer;
		buffer.resize(2 + sizeof(uint64_t));
		auto base = buffer.data();
		memset(std::exchange(base, base + sizeof(ClientPacketType)), SliceResult, sizeof(ClientPacketType));
		auto id = htonll(sliceResult.first);
		memset(std::exchange(base, base + sizeof(uint64_t)), sliceResult.first, sizeof(uint64_t));
		memset(std::exchange(base, base + 1), sliceResult.second, 1);
		return buffer;
	}

	std::variant<PlayerInputState, std::pair<uint64_t, bool>, std::monostate> Deserialize(std::span<const char> buffer) {
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

			return std::pair<uint64_t, bool>(ntohll(id), isHit);
		}
		return  std::monostate{};
	}
}

namespace ServerPacket {
	std::variant<std::tuple<uint64_t, PlayerContext, PlayerContext>, ServerCommand, SpawnRequest, std::monostate> Deserialize(std::span<const char> buffer) {
		if (buffer.size() < 1) return std::monostate{};
		auto base = buffer.data();
		ServerPacketType packetType;
		memcpy(&packetType, std::exchange(base, base + 1), 1);

		if (packetType == ContextUpdate) {
			std::span<const char> sub = buffer.subspan(1);
			if (sub.size() < sizeof(uint64_t)) return std::monostate{};
			uint64_t rawIndex;
			memcpy(&rawIndex, sub.data(), sizeof(uint64_t));
			sub = sub.subspan(sizeof(uint64_t));

			auto context = PlayerContext::Deserialize(sub);
			if (!context) {
				Debug::LogError("Context 1 failed to deserialize");
				return std::monostate{};
			}
			auto context2 = PlayerContext::Deserialize(sub);
			if (!context2) {
				Debug::LogError("Context 2 failed to deserialize");
				return std::monostate{};
			}
			return std::tuple<uint64_t, PlayerContext, PlayerContext> { ntohll(rawIndex), std::move(context.value()), std::move(context2.value()) };
		}

		if (packetType == Command) {
			if (buffer.size() < 2) return std::monostate{};
			ServerCommand cmd;
			memcpy(&cmd, std::exchange(base, base + 1), 1);
			return cmd;
		}

		if (packetType == SpawnSlicable) {
			auto result = SpawnRequest::Deserialize(buffer.subspan(1));
			if (result) return result.value();
			return std::monostate{};
		}
		return std::monostate{};
	}

	std::vector<char> SerializeGameState(const uint64_t index, const PlayerContext& context1, const PlayerContext& context2) {
		std::vector<char> payload;
		payload.push_back(static_cast<char>(ServerPacketType::ContextUpdate));
		payload.resize(1 + sizeof(uint64_t));
		auto ptr = payload.data() + 1;
		uint64_t serializedIndex = htonll(index);
		memcpy(ptr, &serializedIndex, sizeof(uint64_t));
		PlayerContext::AppendSerialize(context1, payload);
		PlayerContext::AppendSerialize(context2, payload);
		return payload;
	}

	std::vector<char> SerializeCommand(const ServerCommand& cmd) {
		std::vector<char> data;
		data.push_back(static_cast<char>(ServerPacketType::Command));
		data.push_back(static_cast<char>(cmd));
		return data;
	}

	std::vector<char> SerializeSpawnRequest(const SpawnRequest& request) {
		std::vector<char> data;
		data.push_back(static_cast<char>(ServerPacketType::SpawnSlicable));
		SpawnRequest::AppendSerialize(request, data);
		return data;
	}
}