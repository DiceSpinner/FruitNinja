#include "connection.hpp"

UDPConnection::UDPConnection(
	std::shared_ptr<UDPSocket> socket, size_t packetQueueCapacity, 
	sockaddr_in peerAddr, uint32_t sessionID, TimeoutSetting setting
)
	: peerAddr(peerAddr), queueCapacity(packetQueueCapacity), socket(std::move(socket)), 
	lastReceived(std::chrono::steady_clock::now()),
	currentIndex(0), timeout(setting), sessionID(sessionID), latestReceivedIndex(0),
	status(ConnectionStatus::Disconnected)
{

}

void UDPConnection::ParsePacket(const UDPHeader& header, UDPPacket&& packet) {
	bool goodPacket = false;
	std::lock_guard<std::mutex> guard(lock);

	switch (status) {
		case ConnectionStatus::Disconnected:
			std::cout << "[Error] A packet is routed to a closed UDPConnection, this is a bug!" << std::endl;
			return;

		case ConnectionStatus::Connected:
			{
				// Close connection immediately if received FIN signal from peer
				if(header.flag & UDPHeaderFlag::FIN){
					std::cout << "Received FIN signal from peer, closing connection" << "\n";
					status = ConnectionStatus::Disconnected;
					return;
				}

				// Handle response to requests from peer
				if (header.flag & UDPHeaderFlag::ACK) {
					// Do nothing if it is heart beat acknowledgement
					if (header.flag & UDPHeaderFlag::HBT) {
						goodPacket = true;
					}
					else {
						for (auto i = awaitResponse.begin(); i != awaitResponse.end();++i) {
							// Packet's ownership will be transferred to the response handler if matches
							if (i->index == header.ackIndex) {
								i->promise.set_value(std::move(packet));
								goodPacket = true;
								awaitResponse.erase(i);
								break;
							}
						}
					}
					break;
				}

				// Acknowledge heartbeat packets
				if (header.flag & UDPHeaderFlag::HBT) {
					goodPacket = true;
					UDPPacket reply;
					reply.address = packet.address;
					UDPHeader replyHeader = {
						.index = currentIndex++,
						.sessionID = sessionID,
						.flag = UDPHeaderFlag::ACK | UDPHeaderFlag::HBT
					};
					reply.SetHeader(replyHeader);
					socket->SendPacket(reinterpret_cast<const Packet&>(reply));
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

				// Manually update during initialization
				peerAddr = packet.address;
				lastReceived = std::chrono::steady_clock::now();
				heartBeatTime = std::chrono::steady_clock::now() + timeout.connectionTimeout / timeout.Divisor();
				return;
			}
			break;
		case ConnectionStatus::Pending:
			#pragma warning(push)
			#pragma warning(disable: 26813)
			
			if (header.flag == UDPHeaderFlag::ACK && !packet.DataSize()) // Check if ACK is the only flag toggled and empty packet
			{
				std::cout << "Server connected " << std::endl;
				status = ConnectionStatus::Connected;

				// Manually update during initialization
				peerAddr = packet.address;
				lastReceived = std::chrono::steady_clock::now();
				heartBeatTime = lastReceived.load() + timeout.connectionTimeout / timeout.Divisor();
				return;
			}
			break;
			#pragma warning(pop)
	}

	// Difference between indices greater than 2^31 is considered wrapped around
	if (goodPacket) {
		if (int32_t(header.index) - int32_t(latestReceivedIndex) > 0) {
			peerAddr = packet.address;
			latestReceivedIndex = header.index;
		}
		lastReceived = std::chrono::steady_clock::now();
		heartBeatTime = lastReceived.load() + timeout.connectionTimeout / timeout.Divisor();
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

void UDPConnection::UpdateAwaits() {
	std::lock_guard<std::mutex> guard(lock);
	for (auto i = awaitResponse.begin(); i != awaitResponse.end();) {
		if (std::chrono::steady_clock::now() > i->timeout) {
			i->promise.set_value({});
			i = awaitResponse.erase(i);
		}
		else {
			if (std::chrono::steady_clock::now() > i->resend) { 
				socket->SendPacket(i->packet); 
				i->resend = std::chrono::steady_clock::now() + timeout.connectionTimeout / timeout.Divisor();
			}
			++i;
		}
	}
}

void UDPConnection::TimeoutDisconnect() {
	std::lock_guard<std::mutex> guard(lock);
	std::cout << "Connection timed out!" << std::endl;
	status = ConnectionStatus::Disconnected;
	for (auto i = awaitResponse.begin(); i != awaitResponse.end();) {
		i->promise.set_value({});
		i = awaitResponse.erase(i);
	}
	packetQueue.clear();
}

void UDPConnection::Disconnect() {
	std::lock_guard<std::mutex> guard(lock);
	if (status == ConnectionStatus::Disconnected) {
		return;
	}
	std::cout << "Closing connection." << std::endl;
	status = ConnectionStatus::Disconnected;
	for (auto i = awaitResponse.begin(); i != awaitResponse.end();) {
		i->promise.set_value({});
		i = awaitResponse.erase(i);
	}
	packetQueue.clear();

	UDPPacket packet;
	packet.address = peerAddr;

	UDPHeader header = {
		.index = currentIndex,
		.sessionID = sessionID,
		.flag = UDPHeaderFlag::FIN
	};
	packet.SetHeader(header);

	UDPPacket response;
	socket->SendPacket(std::span<const char>{packet.payload.data(), packet.payload.size()}, peerAddr);
}