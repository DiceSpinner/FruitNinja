#ifndef CONNECTION_H
#define CONNECTION_H
#include <random>
#include <chrono>
#include <array>
#include <future>
#include "socket.hpp"

typedef uint32_t PacketIndex;

/// <summary>
/// Options available for sending UDPPacket.
///
/// The following flags can be used by application logic:
/// - None: No flags.
/// - ACK: Acknowledgement of a received packet.
/// - REQ: This packet is a request expecting a response. It will be retransmitted 
///        until a matching response is received or a timeout occurs.
///        No ACK is required ¡ª the application can ignore the request, or respond with ACK flag to fufill the request.
///        Additionally, in the response REQ can also be used to make the message both an acknowledgement and request
/// - IMP: Transport-level reliability. The packet must be acknowledged by the receiver
///        via an ACK, or it will be retransmitted.
///
/// Notes:
/// - Do not use IMP and REQ together. REQ includes its own retry logic and assumes reliability.
/// - Modifying internal-use flags (SYN, FIN, HBT) will result in undefined behavior.
/// </summary>
class UDPHeaderFlag {
public:
	enum Flag : uint8_t {
		None = 0,
		ACK = 1,           // Acknowledgement
		REQ = 1 << 1,    // Indicates this packet is a question that expects response
		SYN = 1 << 2,    // Indicates this packet is used for setting up connection, should not be used directly
		IMP = 1 << 3,    // Indicates this packet must be acknowledged, will ignore REQ if this is used
		FIN = 1 << 4,    // Tells the peer the connection is closed, should not be used directly
		HBT = 1 << 5	   // Indicates a heartbeat packet, used to query the connection is still alive, should not be used directly
	} value;
	UDPHeaderFlag(Flag value) : value(value) {}
	UDPHeaderFlag(uint8_t value) : value(static_cast<Flag>(value)) {}

	constexpr operator uint8_t() const {
		return value;
	}
};

#pragma pack(push, 1)
struct UDPHeader {
	PacketIndex index;
	PacketIndex ackIndex;
	uint32_t sessionID;
	UDPHeaderFlag flag;

	static std::optional<UDPHeader> Deserialize(std::span<const char> buffer) {
		if (buffer.size() < sizeof(UDPHeader)) return {};
		u_long index;
		memcpy(&index, buffer.data(), sizeof(u_long));
		u_long ackIndex;
		memcpy(&ackIndex, buffer.data() + offsetof(UDPHeader, ackIndex), sizeof(u_long));
		u_long sessionID;
		memcpy(&sessionID, buffer.data() + offsetof(UDPHeader, sessionID), sizeof(u_long));
		uint8_t flag;
		memcpy(&flag, buffer.data() + offsetof(UDPHeader, flag), sizeof(flag));

		return UDPHeader{
			.index = ntohl(index),
			.ackIndex = ntohl(ackIndex),
			.sessionID = ntohl(sessionID),
			.flag = static_cast<UDPHeaderFlag>(flag)
		};
	}
	static void Serialize(const UDPHeader& header, std::span<char> buffer) {
		if (buffer.size() < sizeof(UDPHeader)) {
			std::cout << "Size of buffer too small for UDPHeader" << std::endl;
			return;
		}
		auto index = htonl(header.index);
		auto ackIndex = htonl(header.ackIndex);
		auto sessionID = htonl(header.sessionID);
		std::memcpy(buffer.data() + offsetof(UDPHeader, index), &index, sizeof(PacketIndex));
		std::memcpy(buffer.data() + offsetof(UDPHeader, ackIndex), &ackIndex, sizeof(PacketIndex));
		std::memcpy(buffer.data() + offsetof(UDPHeader, sessionID), &sessionID, sizeof(PacketIndex));
		std::memcpy(buffer.data() + offsetof(UDPHeader, flag), &header.flag, sizeof(flag));
	}
	bool operator == (const UDPHeader&) const = default;
};
#pragma pack(pop)

struct UDPPacket {
	sockaddr_in address;
	std::chrono::steady_clock::time_point timeReceived;
	std::vector<char> payload;

	UDPPacket() : payload(), address() {
		payload.reserve(sizeof(UDPHeader));
		payload.resize(sizeof(UDPHeader));
	}

	bool Good() const {
		return payload.size() >= sizeof(UDPHeader);
	}

	UDPHeader Header() const {
		return UDPHeader::Deserialize({ payload.data(), sizeof(UDPHeader) }).value();
	}
	void SetHeader(const UDPHeader& header) {
		UDPHeader::Serialize(header, { payload.data(), sizeof(UDPHeader) });
	}
	size_t DataSize() const { return payload.size() - sizeof(UDPHeader); }
	void ResizeData(size_t size) {
		payload.resize(size + sizeof(UDPHeader));
	}
	void Append(std::span<const char> data) {
		payload.insert(payload.end(), data.begin(), data.end());
	}
	void SetData(std::span<const char> data) {
		payload.resize(sizeof(UDPHeader));
		payload.insert(payload.end(), data.begin(), data.end());
	}

	std::span<char> Data() {
		return { payload.data() + sizeof(UDPHeader), payload.size() - sizeof(UDPHeader) };
	}
};

struct TimeoutSetting {
	std::chrono::steady_clock::duration connectionTimeout;
	std::chrono::steady_clock::duration connectionRetryInterval;

	std::chrono::steady_clock::duration requestTimeout;
	std::chrono::steady_clock::duration requestRetryInterval;

	std::chrono::steady_clock::duration impRetryInterval;
};

class UDPConnectionManager;
class UDPConnection {
	friend class UDPConnectionManager;
private:
	struct AwaitResponse {
		PacketIndex index;
		std::promise<std::optional<UDPPacket>> promise;
		Packet packet;
		std::chrono::steady_clock::time_point timeout;
		std::chrono::steady_clock::time_point resend;
		bool acked;
	};

	struct AwaitAck {
		PacketIndex index;
		Packet packet;
		std::chrono::steady_clock::time_point resend;
	};

public:
	enum class ConnectionStatus {
		Pending, // Server has sent back response to client request, waiting for client acknoledgement
		Connecting, // Client has sent initial connection request, waiting for server response
		Connected, // Connection established
		Disconnected // Connection closed
	};

private:
	std::shared_ptr<UDPSocket> socket;
	std::atomic<PacketIndex> currentIndex;
	uint32_t sessionID;
	uint32_t clientChecksum = 0;
	size_t queueCapacity;
	std::condition_variable cv;

	// Used by timeout
	std::atomic<std::chrono::steady_clock::time_point> lastReceived;
	std::atomic<std::chrono::steady_clock::time_point> heartBeatTime;
	uint32_t latestReceivedIndex;

	// The following fields are guarded by lock
	std::mutex lock;
	std::list<AwaitResponse> awaitResponse;
	std::list<UDPPacket> packetQueue;
	std::list<AwaitAck> awaitAck;
	std::list<PacketIndex> ackedIndices;
	sockaddr_in peerAddr;
	ConnectionStatus status;

	/// <summary>
	/// Called by UDPConnectionManager to remove await entries that have expired
	/// </summary>
	bool UpdateTimeout();
	void ParsePacket(const UDPHeader& header, UDPPacket&& packet);
	
	// Packet handlers when status is connected
	void UpdateAddress(const UDPHeader& header, const UDPPacket& packet);
	bool TryHandleDisconnect(const UDPHeader& header);
	bool TryHandleMissedHandshake(const UDPHeader& header, const UDPPacket& packet);
	bool TryHandleAcknowledgement(const UDPHeader& header, UDPPacket&& packet);
	void HandleUnreliableData(const UDPHeader& header, UDPPacket&& packet);
	bool TryHandleReliableData(const UDPHeader& header, UDPPacket&& packet);
	bool TryHandleHeartBeat(const UDPHeader& header, const UDPPacket& packet);
	
	// Packet handlers when status is connecting
	void HandleServerAcknowledgement(const UDPHeader& header, const UDPPacket& packet);

	// Packet handlers when status is pending
	bool TryHandleMissedAcknowledgement(const UDPHeader& header, const UDPPacket& packet);
	void HandleClientAcknowledgement(const UDPHeader& header, const UDPPacket& packet);

public:
	const TimeoutSetting timeout;

	UDPConnection(std::shared_ptr<UDPSocket> socket, size_t packetQueueCapacity, sockaddr_in peerAddr, uint32_t sessionID, TimeoutSetting setting);
	UDPConnection(UDPConnection&& other) = delete;
	UDPConnection(const UDPConnection& other) = delete;
	UDPConnection& operator = (UDPConnection&& other) = delete;
	UDPConnection& operator = (const UDPConnection& other) = delete;

	bool Wait(std::optional<std::chrono::steady_clock::duration> waitTime);
	bool Connected();
	bool Closed();
	sockaddr_in PeerAddr();
	void Disconnect();

	std::future<std::optional<UDPPacket>> Send(UDPPacket& packet);
	std::optional<UDPPacket> Receive();

#ifdef ENABLE_TEST_HOOKS
	void SimulateDisconnect() {
		socket->Close();
	}

	size_t NumImpMsg() {
		return awaitAck.size();
	}

	uint32_t SessionID() {
		return sessionID;
	}
#endif
};

class UDPConnectionManager {
private:
	const size_t numConnections;
	size_t queueCapacity;
	std::shared_ptr<UDPSocket> socket;
	std::chrono::steady_clock::duration updateInterval;
	std::mt19937 checksumGenerator = {};

	// The lock guards the following fields
	std::mutex lock = {};
	std::condition_variable cv = {};
	std::vector<std::weak_ptr<UDPConnection>> connections;

	struct ConnectionRequest {
		sockaddr_in address;
		uint32_t checksum;
	};
	std::list<ConnectionRequest> connectionRequests = {};

	std::thread routeThread = {};

	uint32_t GenerateChecksum();
public:
	std::atomic<bool> isListening = false;

	UDPConnectionManager(USHORT port, size_t numConnections, size_t packetQueueCapacity, DWORD maxPacketSize, std::chrono::steady_clock::duration updateInterval);

	UDPConnectionManager(size_t packetQueueCapacity, size_t numConnections, DWORD maxPacketSize, std::chrono::steady_clock::duration updateInterval);

	UDPConnectionManager(const UDPConnectionManager& other) = delete;
	UDPConnectionManager(UDPConnectionManager&& other) = delete;
	UDPConnectionManager& operator = (const UDPConnectionManager& other) = delete;
	UDPConnectionManager& operator = (UDPConnectionManager&& other) = delete;

	~UDPConnectionManager();

	void RouteAndTimeout();
	
	/// <summary>
	/// Attempts to establish a connection from the list of pending connection requests
	/// </summary>
	/// <param name="timeout"> The duration of the timeout </param>
	/// <returns> An half-established UDPConnection pointer, or nullptr when timed out or the connection limit is reached </returns>
	std::shared_ptr<UDPConnection> Accept(TimeoutSetting timeout, std::optional<std::chrono::steady_clock::duration> waitTime = {});

	/// <summary>
	/// Check if the underlying socket is opened, no connection can be established when the socket is closed.
	/// </summary>
	/// <returns>true if the underlying socket is opened and functional, false otherwise</returns>
	bool Good() const;

	size_t Count();

	/// <summary>
	/// Attempts to connect to a host that is actively listening
	/// </summary>
	/// <param name="address"> The address of the host </param>
	/// <returns> A pointer to a half established connection, or nullptr if timed out or socket is closed </returns>
	std::shared_ptr<UDPConnection> ConnectPeer(sockaddr_in peerAddr, TimeoutSetting timeout);
};
#endif