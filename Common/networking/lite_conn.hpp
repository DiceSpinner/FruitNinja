#ifndef LITE_CONN_H
#define LITE_CONN_H
#include <random>
#include <chrono>
#include <future>
#include <utility>
#include <unordered_set>
#include "socket.hpp"
#include "debug/log.hpp"

// Forward declarations
class LiteConnManager;
class LiteConnConnection;
class LiteConnResponse;

class LiteConnRequest {
private:
	std::weak_ptr<LiteConnConnection> connection;
	bool isValid;
	uint64_t requestID;
public:
	LiteConnRequest(std::weak_ptr<LiteConnConnection>&& connection, const uint64_t& id);
	LiteConnRequest(const LiteConnRequest&) noexcept = delete;
	LiteConnRequest& operator = (const LiteConnRequest&) noexcept = delete;
	LiteConnRequest(LiteConnRequest&&) noexcept = default;
	LiteConnRequest& operator = (LiteConnRequest&&) noexcept = default;
	~LiteConnRequest();

	operator bool() const;
	bool IsValid() const;
	void Reject();
	void Respond(const std::span<const char> response);
	std::optional<LiteConnResponse> Converse(const std::span<const char> data);
};

struct LiteConnMessage {
	std::vector<char> data;
	std::optional<LiteConnRequest> requestHandle;
};

class LiteConnResponse {
private:
	std::weak_ptr<LiteConnConnection> connection;
	uint64_t requestID;
	std::future<std::optional<LiteConnMessage>> response;
public:
	LiteConnResponse(std::weak_ptr<LiteConnConnection>&& connection, const uint64_t& id, std::future<std::optional<LiteConnMessage>>&& value);
	LiteConnResponse(const LiteConnResponse&) = delete;
	LiteConnResponse(LiteConnResponse&&) noexcept = default;
	LiteConnResponse& operator = (const LiteConnResponse&) noexcept = delete;
	LiteConnResponse& operator = (LiteConnResponse&&) noexcept = default;
	~LiteConnResponse();

	void Cancel();

	template<typename Rep, typename Period>
	bool WaitForResponse(std::chrono::duration<Rep, Period> timeout = {}) const {
		if (!response.valid()) return true;

		return response.wait_for(timeout) == std::future_status::ready;
	}
	std::optional<LiteConnMessage> GetResponse();
};

class LiteConnHeaderFlag {
public:
	enum Flag : uint8_t {
		DATA = 1,        // Indicate this packet contains data
		ACK = 1 << 1,     // Acknowledgement of receival
		REQ = 1 << 2,    // Indicates this packet is a question that expects response
		CXL = 1 << 3,    // Tells the peer the REQ is closed.
		IMP = 1 << 4,    // Indicates this packet must be acknowledged of receival
		SYN = 1 << 5,    // Indicates this packet is used for setting up connection
		FIN = 1 << 6,    // Tells the peer the connection is closed
		HBT = 1 << 7	   // Indicates a heartbeat packet, used to query the connection is still alive
	} value;
	LiteConnHeaderFlag(Flag value) : value(value) {}
	LiteConnHeaderFlag(uint8_t value) : value(static_cast<Flag>(value)) {}

	constexpr operator uint8_t() const {
		return value;
	}
};

struct LiteConnHeader {
	static constexpr size_t Size = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(LiteConnHeaderFlag);

	uint32_t sessionID;     // Connection/session identifier
	uint32_t index;         // Transport-level sequencing
	LiteConnHeaderFlag flag;     // Packet behavior (REQ, IMP, etc.)
	uint32_t id32;         // Used as reliability identifier during connection and storage for sessionID negatiation.
	uint64_t id64;       // Identifier for REQ

	static std::optional<LiteConnHeader> Deserialize(std::span<const char> buffer) {
		if (buffer.size() < Size) return {};
		auto base = buffer.data();

		uint32_t sessionID;
		memcpy(&sessionID, std::exchange(base, base + sizeof(uint32_t)), sizeof(uint32_t));
		uint32_t index;
		memcpy(&index, std::exchange(base, base + sizeof(uint32_t)), sizeof(uint32_t));
		uint8_t flag;
		memcpy(&flag, std::exchange(base, base + sizeof(uint8_t)), sizeof(uint8_t));
		uint32_t extra;
		memcpy(&extra, std::exchange(base, base + sizeof(uint32_t)), sizeof(uint32_t));
		uint64_t extra64; 
		memcpy(&extra64, base, sizeof(uint64_t));
		
		return LiteConnHeader{
			.sessionID = ntohl(sessionID),
			.index = ntohl(index),
			.flag = static_cast<LiteConnHeaderFlag>(flag),
			.id32 = ntohl(extra),
			.id64 = ntohll(extra64)
		};
	}
	static void Serialize(const LiteConnHeader& header, std::span<char> buffer) {
		if (buffer.size() < Size) {
			Debug::Log("Size of buffer too small for UDPHeader");
			return;
		}
		auto index = htonl(header.index);
		auto extra = htonl(header.id32);
		auto extra64 = htonll(header.id64);
		auto sessionID = htonl(header.sessionID);
		auto base = buffer.data();

		std::memcpy(std::exchange(base, base + sizeof(uint32_t)), &sessionID, sizeof(uint32_t));
		std::memcpy(std::exchange(base, base + sizeof(uint32_t)), &index, sizeof(uint32_t));
		std::memcpy(std::exchange(base, base + sizeof(flag)), &header.flag, sizeof(flag));
		std::memcpy(std::exchange(base, base + sizeof(uint32_t)), &extra, sizeof(uint32_t));
		std::memcpy(base, &extra64, sizeof(uint64_t));
	}
	static std::vector<char> Serialize(const LiteConnHeader& header) {
		std::vector<char> buffer;
		buffer.resize(Size);
		auto index = htonl(header.index);
		auto extra = htonl(header.id32);
		auto extra64 = htonll(header.id64);
		auto sessionID = htonl(header.sessionID);
		auto base = buffer.data();

		std::memcpy(std::exchange(base, base + sizeof(uint32_t)), &sessionID, sizeof(uint32_t));
		std::memcpy(std::exchange(base, base + sizeof(uint32_t)), &index, sizeof(uint32_t));
		std::memcpy(std::exchange(base, base + sizeof(flag)), &header.flag, sizeof(flag));
		std::memcpy(std::exchange(base, base + sizeof(uint32_t)), &extra, sizeof(uint32_t));
		std::memcpy(base, &extra64, sizeof(uint64_t));
		return buffer;
	}

	bool operator == (const LiteConnHeader&) const = default;
};

struct TimeoutSetting {
	std::chrono::steady_clock::duration connectionTimeout;
	std::chrono::steady_clock::duration connectionRetryInterval;

	std::chrono::steady_clock::duration impRetryInterval;
	std::chrono::steady_clock::duration replyKeepDuration;
};

class LiteConnConnection : public std::enable_shared_from_this<LiteConnConnection> {
	friend class LiteConnManager;
	friend class LiteConnResponse;
	friend class LiteConnRequest;
private:
	/// <summary>
	/// Represents a packet that needs to be acknowledged
	/// </summary>
	struct AutoResendEntry {
		std::vector<char> packet;
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
	std::atomic<uint32_t> pktIndex;
	std::atomic<uint32_t> impIndex;
	std::atomic<uint64_t> reqIndex;
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
	std::unordered_map<uint64_t, AutoResendEntry> autoResendEntries;
	std::unordered_map<uint32_t, std::chrono::steady_clock::time_point> autoAcks;
	std::unordered_map<uint64_t, std::promise<std::optional<LiteConnMessage>>> requestHandles;
	std::unordered_set<uint64_t> LiteConnResponses;
	std::list<LiteConnMessage> packetQueue;
	sockaddr_in peerAddr;
	ConnectionStatus status;

	// Called by request handles
	void CloseRequest(uint64_t index);
	void RejectRequest(uint64_t index);
	void Respond(uint64_t index, const std::span<const char>& data);
	std::optional<LiteConnResponse> Converse(uint64_t index, const std::span<const char>& data);

	/// <summary>
	/// Called by UDPConnectionManager to remove await entries that have expired
	/// </summary>
	bool UpdateTimeout();
	void ParsePacket(const LiteConnHeader& header, std::vector<char>&& data, const sockaddr_in& address);
	
	// Assumes lock is acquired
	void AckReceival(const LiteConnHeader& index);
	void SendPacket(LiteConnHeader& header, const std::span<const char> data);
	void SendPacketReliable(LiteConnHeader& header, const std::span<const char> data);

	// Packet handlers when status is connected
	void UpdateAddress(const LiteConnHeader& header, const sockaddr_in& address);
	bool TryHandleDisconnect(const LiteConnHeader& header);
	bool TryHandleMissedHandshake(const LiteConnHeader& header, const std::vector<char>& data);
	bool TryHandleDuplicates(const LiteConnHeader& header);
	bool TryHandleAcknowledgement(const LiteConnHeader& header, std::vector<char>& data);
	bool TryHandleRequestCancellation(const LiteConnHeader& header, const std::vector<char>& data);
	bool TryHandleData(const LiteConnHeader& header, std::vector<char>& data);
	bool TryHandleHeartBeat(const LiteConnHeader& header, const std::vector<char>& data);
	
	// Packet handlers when status is connecting
	void HandleServerAcknowledgement(const LiteConnHeader& header, const std::vector<char>& data);

	// Packet handlers when status is pending
	bool TryHandleMissedAcknowledgement(const LiteConnHeader& header, const std::vector<char>& data);
	void HandleClientAcknowledgement(const LiteConnHeader& header, const std::vector<char>& data);

public:
	const TimeoutSetting timeout;

	LiteConnConnection(std::shared_ptr<UDPSocket> socket, size_t packetQueueCapacity, sockaddr_in peerAddr, uint32_t sessionID, TimeoutSetting setting);
	LiteConnConnection(LiteConnConnection&& other) = delete;
	LiteConnConnection(const LiteConnConnection& other) = delete;
	LiteConnConnection& operator = (LiteConnConnection&& other) = delete;
	LiteConnConnection& operator = (const LiteConnConnection& other) = delete;
	~LiteConnConnection();

	template<typename Rep, typename Period>
	bool WaitForConnectionComplete(std::chrono::duration<Rep, Period> timeout) {
		std::unique_lock<std::mutex> guard(lock);
		auto pred = [&]() { return status == ConnectionStatus::Connected || status == ConnectionStatus::Disconnected; };
		cv.wait_for(guard, timeout, pred);
		return status == ConnectionStatus::Connected;
	}

	template<typename Rep, typename Period>
	bool WaitForDataPacket(std::chrono::duration<Rep, Period> timeout) {
		std::unique_lock<std::mutex> guard(lock);
		if (status != ConnectionStatus::Connected) return false;
		auto pred = [&]() { return status == ConnectionStatus::Disconnected || !packetQueue.empty(); };
		cv.wait_for(guard, timeout, pred);
		return status == ConnectionStatus::Connected && !packetQueue.empty();
	}

	bool WaitForConnectionComplete();
	bool WaitForDataPacket();
	bool IsConnected();
	bool IsDisconnected();
	sockaddr_in PeerAddr();
	void Disconnect();

	void SendData(const std::span<const char> data);
	void SendReliableData(const std::span<const char> data);
	std::optional<LiteConnResponse> SendRequest(const std::span<const char> data);
	std::optional<LiteConnMessage> Receive();

#ifdef ENABLE_TEST_HOOKS
	void SimulateDisconnect() {
		socket->Close();
	}

	size_t NumImpMsg() {
		return autoResendEntries.size();
	}

	uint32_t SessionID() {
		return sessionID;
	}
#endif
};

class LiteConnManager {
private:
	const size_t numConnections;
	size_t queueCapacity;
	std::shared_ptr<UDPSocket> socket;
	std::chrono::steady_clock::duration updateInterval;
	std::mt19937 checksumGenerator = {};

	// The lock guards the following fields
	std::mutex lock = {};
	std::condition_variable cv = {};
	std::vector<std::weak_ptr<LiteConnConnection>> connections;

	struct ConnectionRequest {
		sockaddr_in address;
		uint32_t checksum;
	};
	std::list<ConnectionRequest> connectionRequests = {};

	std::thread routeThread = {};

	uint32_t GenerateChecksum();
public:
	std::atomic<bool> isListening = false;

	LiteConnManager(USHORT port, size_t numConnections, size_t packetQueueCapacity, DWORD maxPacketSize, std::chrono::steady_clock::duration updateInterval);

	LiteConnManager(size_t packetQueueCapacity, size_t numConnections, DWORD maxPacketSize, std::chrono::steady_clock::duration updateInterval);

	LiteConnManager(const LiteConnManager& other) = delete;
	LiteConnManager(LiteConnManager&& other) = delete;
	LiteConnManager& operator = (const LiteConnManager& other) = delete;
	LiteConnManager& operator = (LiteConnManager&& other) = delete;

	~LiteConnManager();

	void RouteAndTimeout();
	
	/// <summary>
	/// Attempts to establish a connection from the list of pending connection requests
	/// </summary>
	/// <param name="timeout"> The duration of the timeout </param>
	/// <returns> An half-established UDPConnection pointer, or nullptr when timed out or the connection limit is reached </returns>
	std::shared_ptr<LiteConnConnection> Accept(TimeoutSetting timeout, std::optional<std::chrono::steady_clock::duration> waitTime = {});

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
	std::shared_ptr<LiteConnConnection> ConnectPeer(sockaddr_in peerAddr, TimeoutSetting timeout);
};
#endif