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
	std::thread listener;
	std::list<Packet> packetQueue;
	size_t queueCapacity;
	std::atomic<bool> closed;
	std::mutex queueLock;
	std::condition_variable cv;
	void Listener();
public:
	std::atomic<bool> blocked;

	UDPSocket(size_t capacity, DWORD maxPacketSize = DEFAULT_UDP_BUFFER_SIZE);
	UDPSocket(USHORT port, size_t capacity, DWORD maxPacketSize);
	UDPSocket(const UDPSocket& other) = delete;
	UDPSocket(UDPSocket&& other) = delete;
	UDPSocket& operator = (const UDPSocket& other) = delete;
	UDPSocket& operator = (UDPSocket&& other) = delete;
	~UDPSocket();

	void SendPacket(const Packet& packet) const;
	void SendPacket(std::span<const char> payload, const sockaddr_in& target) const;
	const DWORD MaxPacketSize() const;
	const USHORT Port() const;
	const size_t QueueCapacity() const;
	std::optional<Packet> ReadFront();
	std::optional<Packet> ReadBack();

	/// <summary>
	/// Block the thread until a packet satisfying the condition is present in the buffer
	/// </summary>
	/// <typeparam name="Predicate"> A callable that takes in packet reference and return bool </typeparam>
	/// <param name="predicate">The predicate used to evaluate if the packet satisfies the condition</param>
	template<typename Predicate>
		requires requires (Predicate p, const Packet& packet) { { p(packet) } -> std::same_as<bool>; }
	std::optional<Packet> Wait(Predicate predicate, std::chrono::milliseconds DEFAULT_TIMEOUT) {
		std::unique_lock<std::mutex> lock(queueLock);

		for (auto i = packetQueue.begin(); i != packetQueue.end();i++) {
			if (predicate(*i)) {
				Packet packet = std::move(*i);
				packetQueue.erase(i);
				return packet;
			}
		}

		auto lambda = [&]() {
			return std::any_of(packetQueue.begin(), packetQueue.end(), predicate);
			};

		// Assume lock is already acquired, the lambda will pass the packet received to the predicate
		if (!cv.wait_for(lock, DEFAULT_TIMEOUT, lambda)) return {};
		for (auto i = packetQueue.begin(); i != packetQueue.end(); i++) {
			if (predicate(*i)) {
				Packet packet = std::move(*i);
				packetQueue.erase(i);
				return packet;
			}
		}

		// Should never happen
		std::cout << "Socket received a matching packet, but it went missing. This is a bug!" << std::endl;
		return {};
	}

	void Close();
	bool IsClosed() const;
};
#endif