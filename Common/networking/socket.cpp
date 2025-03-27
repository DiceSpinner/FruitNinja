#include <iostream>
#include "socket.hpp"

UDPSocket::UDPSocket(size_t capacity) 
	: queueCapacity(capacity), listener(), address(),
	sock(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)), packetQueue(), 
	isRunning(true), queueLock()
{
	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = 0; 
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
		std::cout << "Failed to bind socket\n";
	}
	else {
		int addrLen = sizeof(address);
		getsockname(sock, reinterpret_cast<sockaddr*>(&address), &addrLen);
		listener = std::thread(&UDPSocket::Listen, this);
	}
}

UDPSocket::~UDPSocket() {
	if (sock != INVALID_SOCKET) {
		isRunning = false;
		closesocket(sock);
		listener.join();
	}
}

void UDPSocket::Listen() {
	sockaddr_in otherAddr = {};
	int addrSize = sizeof(otherAddr);
	char buf[UDP_BUFFER_SIZE];
	while (isRunning) {
		addrSize = sizeof(otherAddr);
		int byteRead = recvfrom(sock, buf, sizeof(buf), 0, (sockaddr*)&otherAddr, &addrSize);

		std::lock_guard<std::mutex> lock(queueLock);
		if (byteRead == SOCKET_ERROR) {
			auto error = WSAGetLastError();
			if(error != WSAESHUTDOWN) std::cout << "Packet read failure: " << error << "\n";
		}
		else if(packetQueue.size() < queueCapacity){
			
			// Extract header from read bytes and put the rest into packet struct
			packetQueue.push_back(
				UDPPacket{
					.payload = {buf, buf + byteRead}
				}
			);
		}
	}
}

void UDPSocket::SendPacket(const UDPPacket& packet, const sockaddr_in& target) const {
	sendto(sock, packet.payload.data(), packet.payload.size(), 0, reinterpret_cast<const sockaddr*>(&target), sizeof(target));
}

std::optional<UDPPacket> UDPSocket::ReadPacket() {
	std::lock_guard<std::mutex> lock(queueLock);
	if (packetQueue.empty()) {
		return {};
	}
	UDPPacket packet = std::move(packetQueue.front());
	packetQueue.pop_front();
	return packet;
}

const sockaddr_in& UDPSocket::Address() const {
	return address;
}