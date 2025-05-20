#ifndef SOCKET_H
#define SOCKET_H
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <list>
#include <vector>
#include <optional>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <condition_variable>
#include <span>
#include <functional>
#include <iostream>
#include <chrono>

constexpr auto DEFAULT_UDP_BUFFER_SIZE = 1500;

inline bool SockAddrInEqual(sockaddr_in address1, sockaddr_in address2) {
	return address1.sin_family == address2.sin_family &&
		address1.sin_addr.s_addr == address2.sin_addr.s_addr &&
		address1.sin_port == address2.sin_port;
}

struct Packet {
	sockaddr_in address;
	std::chrono::steady_clock::time_point timeReceived;
	std::vector<char> payload;
};

class UDPSocket {
private:
	SOCKET sock;
	USHORT port;
	DWORD bufferSize = 0;
	std::atomic<bool> closed;
	std::atomic<bool> reconfiguring;
	std::mutex lock;
	// Called when IP failure detected, assume lock is already acquired
	bool Rebind();
public:
	std::atomic<bool> blocked;

	UDPSocket(DWORD maxPacketSize = DEFAULT_UDP_BUFFER_SIZE);
	UDPSocket(USHORT port, DWORD maxPacketSize);
	UDPSocket(const UDPSocket& other) = delete;
	UDPSocket(UDPSocket&& other) = delete;
	UDPSocket& operator = (const UDPSocket& other) = delete;
	UDPSocket& operator = (UDPSocket&& other) = delete;
	~UDPSocket();

	void SendPacket(const Packet& packet);
	void SendPacket(std::span<const char> payload, const sockaddr_in& target);
	const DWORD MaxPacketSize() const;
	const USHORT Port() const;
	std::optional<Packet> Read();

	void Close();
	bool IsClosed() const;
};
#endif