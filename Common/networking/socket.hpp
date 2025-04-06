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

constexpr auto UDP_BUFFER_SIZE = 1500;
constexpr auto UDP_PACKET_DATA_SIZE = 100;

inline bool SockAddrInEqual(sockaddr_in address1, sockaddr_in address2) {
	return address1.sin_family == address2.sin_family &&
		address1.sin_addr.s_addr == address2.sin_addr.s_addr &&
		address1.sin_port == address2.sin_port;
}

struct Packet {
	std::vector<char> payload;
	sockaddr_in address;
};

class UDPSocket {
private:
	SOCKET sock;
	sockaddr_in address;
	std::thread listener;
	std::list<Packet> packetQueue;
	size_t queueCapacity;
	std::atomic<bool> closed;
	std::mutex queueLock;
	std::condition_variable cv;
	sockaddr_in matchAddr;
	void Listener();
public:
	std::atomic<bool> blocked;

	UDPSocket(size_t capacity);
	UDPSocket(const UDPSocket& other) = delete;
	UDPSocket(UDPSocket&& other) = delete;
	UDPSocket& operator = (const UDPSocket& other) = delete;
	UDPSocket& operator = (UDPSocket&& other) = delete;
	~UDPSocket();

	void SendPacket(const Packet& packet) const;
	void SendPacket(std::span<const char> payload, sockaddr_in target) const;
	const sockaddr_in& Address() const;
	std::optional<Packet> ReadFront();
	std::optional<Packet> ReadBack();

	/// <summary>
	/// Block the thread until a packet satisfying the condition is present in the buffer
	/// </summary>
	/// <typeparam name="Predicate"> A callable that takes in packet reference and return bool </typeparam>
	/// <param name="predicate">The predicate used to evaluate if the packet satisfies the condition</param>
	template<typename Predicate>
		requires requires (Predicate p, const Packet& packet) { { p(packet) } -> std::same_as<bool>; }
	std::optional<Packet> Wait(Predicate predicate, std::chrono::milliseconds timeout) {
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
		if (cv.wait_for(lock, timeout) == std::cv_status::timeout) return {};
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

	void SetMatchAddress(const sockaddr_in& match);
	void Close();
	bool IsClosed() const;
};
#endif