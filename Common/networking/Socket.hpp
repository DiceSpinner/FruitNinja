#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <list>
#include <vector>
#include <optional>
#include <atomic>
#include <mutex>

constexpr auto UDP_BUFFER_SIZE = 1500;

struct UDPPacket {	
	std::vector<char> payload;
};

class UDPSocket {
private:
	SOCKET sock;
	sockaddr_in address;
	std::thread listener;
	std::list<UDPPacket> packetQueue;
	size_t queueCapacity;
	std::atomic<bool> isRunning;
	std::mutex queueLock;
	void Listen();
public:
	UDPSocket(size_t capacity);
	UDPSocket(const UDPSocket& other) = delete;
	UDPSocket(UDPSocket&& other) = delete;
	UDPSocket& operator = (const UDPSocket& other) = delete;
	UDPSocket& operator = (UDPSocket&& other) = delete;

	~UDPSocket();
	void SendPacket(const UDPPacket& packet, const sockaddr_in& target) const;
	const sockaddr_in& Address() const;
	std::optional<UDPPacket> ReadPacket();
};