#include <iostream>
#include "socket.hpp"

UDPSocket::UDPSocket(size_t capacity, DWORD maxPacketSize)
	: queueCapacity(capacity), listener(), port(),
	sock(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)), packetQueue(), 
	closed(true), blocked(false), queueLock(), cv()
{
	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = 0;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int optLen = sizeof(bufferSize);
	getsockopt(sock, SOL_SOCKET, SO_MAX_MSG_SIZE, reinterpret_cast<char*>(&bufferSize), &optLen);
	bufferSize = min(bufferSize, maxPacketSize);

	if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
		std::cout << "[Error] Failed to bind socket\n";
	}
	else {
		closed = false;
		int addrLen = sizeof(addr);
		getsockname(sock, reinterpret_cast<sockaddr*>(&addr), &addrLen);
		port = addr.sin_port;
		listener = std::thread(&UDPSocket::Listener, this);
	}
}

UDPSocket::UDPSocket(USHORT port, size_t capacity, DWORD maxPacketSize)
	: queueCapacity(capacity), listener(), port(htons(port)),
	sock(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)), packetQueue(),
	closed(true), blocked(false), queueLock(), cv()
{
	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int optLen = sizeof(bufferSize);
	getsockopt(sock, SOL_SOCKET, SO_MAX_MSG_SIZE, reinterpret_cast<char*>(&bufferSize), &optLen);

	bufferSize = min(bufferSize, maxPacketSize);

	if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
		std::cout << "[Error] Failed to bind socket\n";
	}
	else {
		closed = false;
		listener = std::thread(&UDPSocket::Listener, this);
	}
}

UDPSocket::~UDPSocket() {
	if (!closed) {
		closed = true;
		closesocket(sock);
		if(listener.joinable()) listener.join();
	}
}

void UDPSocket::Listener() {
	if (closed) return;

	sockaddr_in otherAddr = {};
	int addrSize = sizeof(otherAddr);
	std::vector<char> buf(bufferSize);
	while (!closed) {
		addrSize = sizeof(otherAddr);
		int byteRead = recvfrom(sock, buf.data(), bufferSize, 0, (sockaddr*)&otherAddr, &addrSize);
		std::lock_guard<std::mutex> lock(queueLock);

		if (byteRead == SOCKET_ERROR) {
			auto error = WSAGetLastError();
			if (error == WSAECONNRESET) std::cout << "[Msg] A packet was sent to invalid address previously!" << std::endl;
			else if (error == WSAEMSGSIZE) std::cout << "[Warning] An oversized packet was dropped" << std::endl;
			else if (error != WSAESHUTDOWN && error != WSAEINTR && error != WSAENOTSOCK) std::cout << "[Error] Packet read failure: " << error << "\n";
		}
		else if (!blocked && packetQueue.size() < queueCapacity)
		{
			packetQueue.emplace_back(
				Packet {
					.address = otherAddr,
					.timeReceived = std::chrono::steady_clock::now(),
					.payload = {buf.data(), buf.data() + byteRead},
				}
			);
			cv.notify_all();
		}
	}
}

void UDPSocket::SendPacket(const Packet& packet) const {
	if (closed) {
		std::cout << "[Error] Attempting to write to a closed socket!\n"; 
		return;
	}

	if (sendto(sock, packet.payload.data(), packet.payload.size(), 0, reinterpret_cast<const sockaddr*>(&packet.address), sizeof(packet.address)) == SOCKET_ERROR) {
		std::cout << "[Error] Failed to send packet due to error " << WSAGetLastError() << std::endl;
	}
}

void UDPSocket::SendPacket(std::span<const char> payload, const sockaddr_in& target) const {
	if (closed) {
		std::cout << "[Error] Attempting to write to a closed socket!\n";
		return;
	}

	sendto(sock, payload.data(), payload.size(), 0, reinterpret_cast<const sockaddr*>(&target), sizeof(target));
}

std::optional<Packet> UDPSocket::ReadFront() {
	if (closed) {
		return {};
	}

	std::lock_guard<std::mutex> lock(queueLock);
	if (packetQueue.empty()) {
		return {};
	}
	Packet packet = std::move(packetQueue.front());
	packetQueue.pop_front();
	return packet;
}

std::optional<Packet> UDPSocket::ReadBack() {
	if (closed) {
		return {};
	}

	std::lock_guard<std::mutex> lock(queueLock);
	if (packetQueue.empty()) {
		return {};
	}
	Packet packet = std::move(packetQueue.back());
	packetQueue.pop_back();
	return packet;
}

const DWORD UDPSocket::MaxPacketSize() const { return bufferSize; }

const USHORT UDPSocket::Port() const {
	return port;
}

const size_t UDPSocket::QueueCapacity() const { return queueCapacity; }

void UDPSocket::Close() {
	closed = true;
	closesocket(sock);
	listener.join();

	// No lock is needed as the listener thread is no longer running
	packetQueue.clear();
}

bool UDPSocket::IsClosed() const { return closed; }