#ifndef CONNECTION_H
#define CONNECTION_H
#include <functional>
#include <chrono>
#include "socket.hpp"

typedef unsigned int PacketIndex;

enum UDPHeaderFlag : uint8_t {
	None = 0,
	ACK = 1,
	REQ = 1 << 1,
	SYN = 1 << 2,
	FIN = 1 << 3
};
inline constexpr UDPHeaderFlag operator | (const UDPHeaderFlag& left, const UDPHeaderFlag& right) {
	return static_cast<UDPHeaderFlag>(static_cast<uint8_t>(left) | static_cast<uint8_t>(right));
}

inline constexpr UDPHeaderFlag operator & (const UDPHeaderFlag& left, const UDPHeaderFlag& right) {
	return static_cast<UDPHeaderFlag>(static_cast<uint8_t>(left) & static_cast<uint8_t>(right));
}

#pragma pack(push, 1)
struct UDPHeader {
	PacketIndex index;
	PacketIndex ackIndex;
	UDPHeaderFlag flag;

	static std::optional<UDPHeader> Deserialize(std::span<const char> buffer);
	static void Serialize(const UDPHeader& header, std::span<char> buffer);
};
#pragma pack(pop)

struct UDPPacket {
	std::vector<char> payload;
	sockaddr_in address;

	UDPPacket();
	UDPHeader Header() const;
	void SetHeader(const UDPHeader& header);
	size_t DataSize() const;
	void ResizeData(size_t size);
	void Append(std::span<const char> data);
	std::span<char> Data();
};

struct UDPAwaitResponse {
	std::function<void(const UDPPacket&)> responseHandle;
	std::chrono::time_point<std::chrono::high_resolution_clock> ackWaitBegin;
};

class UDPConnection {
private:
	UDPSocket socket;
	sockaddr_in peer;
	PacketIndex currentIndex;
	std::mutex lock;
	std::list<std::pair<PacketIndex, UDPAwaitResponse>> awaitResponse;
	std::function<void(const sockaddr_in&)> onDisconnect;
public:
	UDPConnection(size_t packetBufferSize, std::function<void(const sockaddr_in&)> onDisconnect = {});
	UDPConnection(UDPConnection&& other) = delete;
	UDPConnection(const UDPConnection& other) = delete;
	UDPConnection& operator = (UDPConnection&& other) = delete;
	UDPConnection& operator = (const UDPConnection& other) = delete;

	std::optional<sockaddr_in> ConnectedPeer();
	void Listen();
	void Disconnect();
	void ConnectPeer(const sockaddr_in& peer);

	template<typename T>
	requires std::same_as<std::decay_t<T>, UDPPacket>
	std::optional<UDPPacket> Send(T&& packet) {
		// No peer connected, do nothing
		if (peer.sin_family == AF_UNSPEC) {
			std::cout << "Attempting to send data via unconnected connection" << std::endl;
			return {};
		}

		UDPHeader header = packet.Header();
		header.index = currentIndex++;
		packet.SetHeader(header);
		socket.SendPacket(reinterpret_cast<const Packet&>(packet));
		if (!(packet.Header().flag & UDPHeaderFlag::REQ)) {
			return {};
		}
		auto response = socket.Wait(
			[&](const Packet& packet) {
				auto responseHeader = reinterpret_cast<const UDPPacket&>(packet).Header();
				return responseHeader.flag | UDPHeaderFlag::ACK && responseHeader.ackIndex == header.index;
			},
			std::chrono::milliseconds(3000)
		);
		if (response) return reinterpret_cast<UDPPacket&>(response);
		return {};
	}

	std::optional<UDPPacket> Receive();
	const sockaddr_in& Address() const;
};
#endif