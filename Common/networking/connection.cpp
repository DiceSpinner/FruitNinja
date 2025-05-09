#include "connection.hpp"

UDPConnection::UDPConnection(
	std::shared_ptr<UDPSocket> socket, size_t packetQueueCapacity, 
	sockaddr_in peerAddr, uint32_t sessionID, std::chrono::steady_clock::duration timeout
)
	: peerAddr(peerAddr), queueCapacity(packetQueueCapacity), socket(std::move(socket)), 
	lastReceived(std::chrono::steady_clock::now()),
	currentIndex(0), timeout(timeout), sessionID(sessionID), 
	status(ConnectionStatus::Disconnected)
{

}

void UDPConnection::ParsePacket(const UDPHeader& header, UDPPacket&& packet) {
	bool goodPacket = false;
	std::lock_guard<std::mutex> guard(lock);
	switch (status) {
		case ConnectionStatus::Connected:
			{
				// Handle response to requests from peer
				if (header.flag & UDPHeaderFlag::ACK) {
					for (auto i = awaitResponse.begin(); i != awaitResponse.end();++i) {
						// Packet's ownership will be transferred to the response handler if matches
						if (i->second.index == header.ackIndex) {
							i->second.responseHandle(std::move(packet));
							goodPacket = true;
							break;
						}
					}
					break;
				}
				// Regular data packets will be stored in a queue
				if (packetQueue.size() < queueCapacity) packetQueue.emplace_back(std::move(packet));
				goodPacket = true;
				break;
			}
			
		case ConnectionStatus::Connecting:
			if (header.flag & UDPHeaderFlag::SYN && header.flag & UDPHeaderFlag::ACK) {
				sessionID = header.index;
				status = ConnectionStatus::Connected;
				UDPPacket reply;
				reply.address = packet.address;
				UDPHeader replyHeader = {
					.sessionID = sessionID,
					.flag = UDPHeaderFlag::ACK,
				};
				std::cout << "Client acknowledge session id " << sessionID << std::endl;
				reply.SetHeader(replyHeader);
				socket->SendPacket(reinterpret_cast<Packet&>(reply));
				goodPacket = true;
			}
			break;
		case ConnectionStatus::Pending:
			#pragma warning(push)
			#pragma warning(disable: 26813)
			
			if (header.flag == UDPHeaderFlag::ACK && !packet.DataSize()) // Check if ACK is the only flag toggled and empty packet
			{
				std::cout << "Server connected " << std::endl;
				status = ConnectionStatus::Connected;
				goodPacket = true;
			}
			break;
			#pragma warning(pop)
	}
	if (goodPacket) { 
		peerAddr = packet.address;
		lastReceived = std::chrono::steady_clock::now(); 
	}
}

bool UDPConnection::Connected() { 
	std::lock_guard<std::mutex> guard(lock);
	return status == ConnectionStatus::Connected; 
}

bool UDPConnection::Closed() {
	std::lock_guard<std::mutex> guard(lock);
	return status == ConnectionStatus::Disconnected;
}

sockaddr_in UDPConnection::PeerAddr() { 
	std::lock_guard<std::mutex> guard(lock); return peerAddr; 
}

std::optional<UDPPacket> UDPConnection::Receive() {
	// No peer connected, do nothing
	std::lock_guard<std::mutex> guard(lock);
	if (status == ConnectionStatus::Disconnected) {
		return {};
	}
	if (packetQueue.empty()) {
		return {};
	}
	UDPPacket pkt = packetQueue.front();
	packetQueue.pop_front();
	return pkt;
}

void UDPConnection::RemoveTimedOutAwaits() {
	std::lock_guard<std::mutex> guard(lock);
	for (auto i = awaitResponse.begin(); i != awaitResponse.end();) {
		if (std::chrono::steady_clock::now() - i->first >= timeout.load()) {
			i = awaitResponse.erase(i);
		}
		else {
			++i;
		}
	}
}

void UDPConnection::TimeoutDisconnect() {
	std::lock_guard<std::mutex> guard(lock);
	status = ConnectionStatus::Disconnected;
	awaitResponse.clear();
	packetQueue.clear();
}

void UDPConnection::Disconnect() {
	std::lock_guard<std::mutex> guard(lock);
	if (status == ConnectionStatus::Disconnected) {
		return;
	}
	status = ConnectionStatus::Disconnected;
	awaitResponse.clear();
	packetQueue.clear();

	UDPPacket packet;
	packet.address = peerAddr;

	UDPHeader header = {
		.index = currentIndex,
		.flag = UDPHeaderFlag::FIN
	};
	packet.SetHeader(header);

	UDPPacket response;
	socket->SendPacket(std::span<const char>{packet.payload.data(), packet.payload.size()}, peerAddr);
}