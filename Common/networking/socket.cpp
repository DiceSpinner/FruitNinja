#include <iostream>
#include "socket.hpp"
#include "debug/log.hpp"

UDPSocket::UDPSocket(DWORD maxPacketSize)
	: port(), sock(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)),
	closed(true), blocked(false), reconfiguring(false), lock()
{
	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = 0;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int optLen = sizeof(bufferSize);
	getsockopt(sock, SOL_SOCKET, SO_MAX_MSG_SIZE, reinterpret_cast<char*>(&bufferSize), &optLen);
	bufferSize = min(bufferSize, maxPacketSize);

	if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
		Debug::LogError("[Error] Failed to bind socket due to error ", WSAGetLastError());
	}
	else {
		closed = false;
		int addrLen = sizeof(addr);
		getsockname(sock, reinterpret_cast<sockaddr*>(&addr), &addrLen);
		port = addr.sin_port;
	}
}

UDPSocket::UDPSocket(USHORT port,DWORD maxPacketSize)
	: port(htons(port)), sock(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)),
	closed(true), blocked(false), reconfiguring(false), lock()
{
	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int optLen = sizeof(bufferSize);
	getsockopt(sock, SOL_SOCKET, SO_MAX_MSG_SIZE, reinterpret_cast<char*>(&bufferSize), &optLen);

	bufferSize = min(bufferSize, maxPacketSize);

	if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
		Debug::LogError("[Error] Failed to bind socket due to error ", WSAGetLastError());
	}
	else {
		closed = false;
	}
}

UDPSocket::~UDPSocket() {
	if (!closed) {
		closed = true;
		closesocket(sock);
	}
}

bool UDPSocket::Rebind() {
	Debug::Log("[Warning] The ip bound to the socket has gone down, attempting to reconfigure");
	closesocket(sock);
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = port;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
		Debug::LogError("[Error] Failed to rebind socket due to error ", WSAGetLastError());
		closed = true;
		return false;
	}
	return true;
}

void UDPSocket::SendPacket(const Packet& packet) {
	if (closed) {
		Debug::LogError("[Error] Attempting to write to a closed socket!"); 
		return;
	}

	std::lock_guard<std::mutex> guard(lock);
	while (sendto(sock, packet.payload.data(), packet.payload.size(), 0, reinterpret_cast<const sockaddr*>(&packet.address), sizeof(packet.address)) == SOCKET_ERROR) {
		auto error = WSAGetLastError();
		if (error == WSAENETDOWN) {
			if (!Rebind()) { Debug::LogError("[Error] Failed to send packet since all network interfaces have gone down"); return; }
			continue;
		}
		Debug::LogError("[Error] Failed to send packet due to error ", error);
		return;
	}
}

void UDPSocket::SendPacket(const std::span<const char> payload, const sockaddr_in& target) {
	if (closed) {
		Debug::LogError("[Error] Attempting to write to a closed socket!");
		return;
	}

	std::lock_guard<std::mutex> guard(lock);
	while (sendto(sock, payload.data(), payload.size(), 0, reinterpret_cast<const sockaddr*>(&target), sizeof(target)) == SOCKET_ERROR) {
		auto error = WSAGetLastError();
		if (error == WSAENETDOWN) {
			if (!Rebind()) { Debug::LogError("[Error] Failed to send packet since all network interfaces have gone down"); return; }
			continue;
		}
		Debug::LogError("[Error] Failed to send packet due to error ", error);
		return;
	}
}

std::optional<Packet> UDPSocket::Read() {
	if (closed) {
		return {};
	}
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	TIMEVAL timeout = {
		.tv_sec = 0,
		.tv_usec = 0
	};
	sockaddr_in otherAddr = {};
	int addrSize = sizeof(otherAddr);

	std::lock_guard<std::mutex> guard(lock);
	std::vector<char> buffer(bufferSize);
	int error;
	while ((error = select(0, &readfds, nullptr, nullptr, &timeout)) > 0) {
		int byteRead = recvfrom(sock, buffer.data(), bufferSize, 0, (sockaddr*)&otherAddr, &addrSize);

		if (byteRead == SOCKET_ERROR) {
			auto error = WSAGetLastError();
			if (error == WSAECONNRESET) {
				std::cout << "[Warning] One message could not reach the remote port" << std::endl;
				continue;
			}
			if (error == WSAEMSGSIZE) {
				std::cout << "[Warning] An oversized packet was dropped" << std::endl;
				continue;
			}
			if (error == WSAENETUNREACH || error == WSAEADDRNOTAVAIL) {
				if (!Rebind()) { return {}; }
				continue;
			}
			std::cout << "[Error] Packet read failure: " << error << "\n";
			continue;
		}
		if (!blocked)
		{
			return Packet{
					.address = otherAddr,
					.timeReceived = std::chrono::steady_clock::now(),
					.payload = {buffer.data(), buffer.data() + byteRead},
			};
		}
	}
	if (error == SOCKET_ERROR) {
		Debug::LogError("[Error] select() returned with error ", WSAGetLastError());
	}
	return {};
}

const DWORD UDPSocket::MaxPacketSize() const { return bufferSize; }

const USHORT UDPSocket::Port() const {
	return port;
}

void UDPSocket::Close() {
	std::lock_guard<std::mutex> guard(lock);
	closed = true;
	closesocket(sock);
}

bool UDPSocket::IsClosed() const { return closed; }