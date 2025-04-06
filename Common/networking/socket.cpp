#include <iostream>
#include "socket.hpp"

UDPSocket::UDPSocket(size_t capacity) 
	: queueCapacity(capacity), listener(), address(), matchAddr(),
	sock(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)), packetQueue(), 
	closed(true), blocked(true), queueLock(), cv()
{
	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = 0; 
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
		std::cout << "Failed to bind socket\n";
	}
	else {
		closed = false;
		int addrLen = sizeof(address);
		getsockname(sock, reinterpret_cast<sockaddr*>(&address), &addrLen);
		listener = std::thread(&UDPSocket::Listener, this);
	}
}

UDPSocket::~UDPSocket() {
	if (!closed) {
		closed = true;
		closesocket(sock);
		listener.join();
	}
}

void UDPSocket::SetMatchAddress(const sockaddr_in& match) {
	std::lock_guard<std::mutex> guard(queueLock);
	if (SockAddrInEqual(match, matchAddr)) {
		return;
	}
	matchAddr = match;
	packetQueue.clear();
}

void UDPSocket::Listener() {
	if (closed) return;

	sockaddr_in otherAddr = {};
	int addrSize = sizeof(otherAddr);
	char buf[UDP_BUFFER_SIZE];
	while (!closed) {
		addrSize = sizeof(otherAddr);
		int byteRead = recvfrom(sock, buf, sizeof(buf), 0, (sockaddr*)&otherAddr, &addrSize);
		std::lock_guard<std::mutex> lock(queueLock);

		if (byteRead == SOCKET_ERROR) {
			auto error = WSAGetLastError();
			if(error != WSAESHUTDOWN && error != WSAEINTR) std::cout << "Packet read failure: " << error << "\n";
		}
		else if (
			!blocked && packetQueue.size() < queueCapacity && 
			(matchAddr.sin_family == AF_UNSPEC || SockAddrInEqual(matchAddr, otherAddr)))
		{
			packetQueue.emplace_back(
				Packet {
					.payload = {buf, buf + byteRead},
					.address = otherAddr
				}
			);
			cv.notify_all();
		}
	}
}

void UDPSocket::SendPacket(const Packet& packet) const {
	if (closed) {
		std::cout << "Attempting to write to a closed socket!\n"; 
		return;
	}

	if (sendto(sock, packet.payload.data(), packet.payload.size(), 0, reinterpret_cast<const sockaddr*>(&packet.address), sizeof(packet.address)) == SOCKET_ERROR) {
		std::cout << "Failed to send packet due to error " << WSAGetLastError() << std::endl;
	}
}

void UDPSocket::SendPacket(std::span<const char> payload, sockaddr_in target) const {
	if (closed) {
		std::cout << "Attempting to write to a closed socket!\n";
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

const sockaddr_in& UDPSocket::Address() const {
	return address;
}

void UDPSocket::Close() {
	closed = true;
	closesocket(sock);
	listener.join();

	// No lock is needed as the listener thread is no longer running
	packetQueue.clear();
}

bool UDPSocket::IsClosed() const { return closed; }