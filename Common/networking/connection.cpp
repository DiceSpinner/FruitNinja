#include "connection.hpp"

UDPConnection::UDPConnection(
	std::shared_ptr<UDPSocket> socket, size_t packetQueueCapacity, 
	sockaddr_in peerAddr, uint32_t sessionID, TimeoutSetting setting
)
	: peerAddr(peerAddr), queueCapacity(packetQueueCapacity), socket(std::move(socket)), 
	lastReceived(std::chrono::steady_clock::now()), 
	currentIndex(0), timeout(setting), sessionID(sessionID), latestReceivedIndex(0),
	status(ConnectionStatus::Disconnected), heartBeatTime(std::chrono::steady_clock::now() + setting.connectionRetryInterval)
{

}

void UDPConnection::UpdateAddress(const UDPHeader& header, const UDPPacket& packet) {
	if (int32_t(header.index) - int32_t(latestReceivedIndex) > 0) {
		peerAddr = packet.address;
		latestReceivedIndex = header.index;
	}
	lastReceived = std::chrono::steady_clock::now();
	heartBeatTime = lastReceived.load() + timeout.connectionRetryInterval;
}

#pragma warning(push)
#pragma warning(disable: 26813)
bool UDPConnection::TryHandleMissedHandshake(const UDPHeader& header, const UDPPacket& packet) {
	if (header.flag == UDPHeaderFlag::SYN) {
		UDPPacket reply;
		reply.address = packet.address;
		UDPHeader replyHeader = {
			.sessionID = sessionID,
			.flag = UDPHeaderFlag::ACK,
		};
		std::cout << "Client retransmit syn-ack message" << std::endl;
		reply.SetHeader(replyHeader);
		socket->SendPacket(reinterpret_cast<Packet&>(reply));

		if (int32_t(header.index) - int32_t(latestReceivedIndex) > 0) {
			peerAddr = packet.address;
			latestReceivedIndex = header.index;
		}
		lastReceived = std::chrono::steady_clock::now();
		heartBeatTime = lastReceived.load() + timeout.connectionRetryInterval;
		return true;
	}
	return false;
}

bool UDPConnection::TryHandleDisconnect(const UDPHeader& header) {
	if (header.flag & UDPHeaderFlag::FIN) {
		std::cout << "Received FIN signal from peer, closing connection" << "\n";
		status = ConnectionStatus::Disconnected;
		for (auto i = awaitResponse.begin(); i != awaitResponse.end();) {
			i->promise.set_value({});
			i = awaitResponse.erase(i);
		}
		awaitAck.clear();
		packetQueue.clear();
		return true;
	}
	return false;
}

bool UDPConnection::TryHandleAcknowledgement(const UDPHeader& header, UDPPacket&& packet) {
	if (header.flag & UDPHeaderFlag::ACK) {
		// Do nothing if it is heart beat acknowledgement
		if (header.flag & UDPHeaderFlag::HBT) {
			UpdateAddress(header, packet);
		}
		else if (header.flag & UDPHeaderFlag::IMP) {
			// Check if acknoledgement matches any important messages
			if (header.flag & UDPHeaderFlag::REQ) {
				for (auto i = awaitResponse.begin(); i != awaitResponse.end(); ++i) {
					if (i->index == header.ackIndex) {
						UpdateAddress(header, packet);
						i->acked = true;
						return true;
					}
				}
			}
			else {
				for (auto i = awaitAck.begin(); i != awaitAck.end(); ++i) {
					if (i->index == header.ackIndex) {
						UpdateAddress(header, packet);
						awaitAck.erase(i);
						return true;
					}
				}
			}
		}
		else {
			// Check if acknoledgement matches any requests
			for (auto i = awaitResponse.begin(); i != awaitResponse.end(); ++i) {
				if (i->index == header.ackIndex) {
					UpdateAddress(header, packet);
					try {
						i->promise.set_value(std::move(packet));
					}
					catch (const std::future_error& e) { 
						// Application logic decided not to track response value
					}
					awaitResponse.erase(i);
					return true;
				}
			}
		}
		return true;
	}
	return false;
}

bool UDPConnection::TryHandleReliableData(const UDPHeader& header, UDPPacket&& packet) {
	if (header.flag & UDPHeaderFlag::IMP || header.flag & UDPHeaderFlag::REQ) {
		UpdateAddress(header, packet);

		bool processed = false;

		// Prevents double processing by checking if the packet has already been acknoledged
		for (auto i = ackedIndices.rbegin(); i != ackedIndices.rend(); ++i) {
			if (*i == header.index) {
				processed = true;
				break;
			}
		}

		// Do nothing if the packet is new and dropped due to capacity limit
		if (!processed && packetQueue.size() >= queueCapacity) {
			std::cout << "A message demands reliability is received but dropped due to capacity limit" << std::endl;
			return true;
		}

		// Process the packet if not already and send back acknoledgement
		if (!processed && packetQueue.size() < queueCapacity) {
			packetQueue.emplace_back(std::move(packet));
			ackedIndices.push_back(header.index);
			if (ackedIndices.size() > queueCapacity) ackedIndices.pop_front();
		}
		UDPPacket reply;
		reply.address = peerAddr;
		UDPHeaderFlag ackFlag = (header.flag & UDPHeaderFlag::IMP) ? UDPHeaderFlag::IMP : (UDPHeaderFlag::IMP | UDPHeaderFlag::REQ);
		UDPHeader replyHeader = {
			.ackIndex = header.index,
			.sessionID = sessionID,
			.flag = UDPHeaderFlag::ACK | ackFlag
		};
		reply.SetHeader(replyHeader);
		socket->SendPacket(reinterpret_cast<Packet&>(reply));
		std::cout << "Acking imp\n";
		return true;
	}
	return false;
}

void UDPConnection::HandleUnreliableData(const UDPHeader& header, UDPPacket&& packet) {
	if (packetQueue.size() < queueCapacity) {
		packetQueue.emplace_back(std::move(packet));
	}
}

bool UDPConnection::TryHandleHeartBeat(const UDPHeader& header, const UDPPacket& packet) {
	// Acknowledge heartbeat packets
	if (header.flag == UDPHeaderFlag::HBT) {
		UpdateAddress(header, packet);
		UDPPacket reply;
		reply.address = packet.address;
		UDPHeader replyHeader = {
			.sessionID = sessionID,
			.flag = UDPHeaderFlag::ACK | UDPHeaderFlag::HBT
		};
		reply.SetHeader(replyHeader);
		socket->SendPacket(reinterpret_cast<const Packet&>(reply));
		return true;
	}
	return false;
}

void UDPConnection::HandleServerAcknowledgement(const UDPHeader& header, const UDPPacket& packet) {
	if (header.flag == (UDPHeaderFlag::SYN | UDPHeaderFlag::ACK)) {
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
		heartBeatTime = std::chrono::steady_clock::now() + timeout.connectionRetryInterval;

		cv.notify_all();
	}
}

bool UDPConnection::TryHandleMissedAcknowledgement(const UDPHeader& header, const UDPPacket& packet) {
	// Client did not receive the packet
	if (header.flag == UDPHeaderFlag::SYN && header.sessionID == 0) { 
		UDPPacket synMessage;
		synMessage.address = packet.address;
		UDPHeader syncHeader = {
			.index = sessionID,
			.sessionID = clientChecksum,
			.flag = UDPHeaderFlag::SYN | UDPHeaderFlag::ACK,
		};
		synMessage.SetHeader(syncHeader);
		socket->SendPacket(reinterpret_cast<Packet&>(synMessage));

		peerAddr = packet.address;
		lastReceived = std::chrono::steady_clock::now();
		heartBeatTime = lastReceived.load() + timeout.connectionRetryInterval;
		return true;
	}
	return false;
}

void UDPConnection::HandleClientAcknowledgement(const UDPHeader& header, const UDPPacket& packet) {
	// Received client ack, connection established
	if (header.flag == UDPHeaderFlag::ACK && !packet.DataSize())
	{
		std::cout << "Server connected " << std::endl;
		status = ConnectionStatus::Connected;

		// Manually update during initialization
		peerAddr = packet.address;
		lastReceived = std::chrono::steady_clock::now();
		heartBeatTime = lastReceived.load() + timeout.connectionRetryInterval;
		cv.notify_all();
		return;
	}
}

void UDPConnection::ParsePacket(const UDPHeader& header, UDPPacket&& packet) {
	bool goodPacket = false;
	std::lock_guard<std::mutex> guard(lock);

	switch (status) {
		case ConnectionStatus::Disconnected:
			std::cout << "[Error] A packet is routed to a closed UDPConnection, this is a bug!" << std::endl;
			return;

		case ConnectionStatus::Connected:
			// Close connection immediately if received FIN signal from peer
			if (TryHandleDisconnect(header)) return;

			// Sync packet sent to the server is lost, retransmit
			if (TryHandleMissedHandshake(header, packet)) return;
			if (TryHandleHeartBeat(header, packet)) return;

			// Handle acknoledgements from peer
			if (TryHandleAcknowledgement(header, std::move(packet))) return;
			if (TryHandleReliableData(header, std::move(packet))) return;	
			HandleUnreliableData(header, std::move(packet));

			return;
			
		case ConnectionStatus::Connecting:
			HandleServerAcknowledgement(header, packet);
			return;
		case ConnectionStatus::Pending:
			if (TryHandleMissedAcknowledgement(header, packet)) return;
			
			HandleClientAcknowledgement(header, packet);
			return;
	}
}
#pragma warning(pop)

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

bool UDPConnection::UpdateTimeout() {
	std::lock_guard<std::mutex> guard(lock);
	if (std::chrono::steady_clock::now() - lastReceived.load() > timeout.connectionTimeout) {
		std::cout << "Connection timed out!" << std::endl;
		status = ConnectionStatus::Disconnected;
		for (auto i = awaitResponse.begin(); i != awaitResponse.end();) {
			i->promise.set_value({});
			i = awaitResponse.erase(i);
		}
		packetQueue.clear();
		awaitAck.clear();
		cv.notify_all();
		return false;
	}

	if (status == ConnectionStatus::Connected) {
		for (auto i = awaitAck.begin(); i != awaitAck.end(); ++i) {
			if (std::chrono::steady_clock::now() > i->resend) {
				socket->SendPacket(i->packet);
				i->resend = std::chrono::steady_clock::now() + timeout.impRetryInterval;
			}
		}

		for (auto i = awaitResponse.begin(); i != awaitResponse.end();) {
			if (std::chrono::steady_clock::now() > i->timeout) {
				i->promise.set_value({});
				i = awaitResponse.erase(i);
			}
			else {
				if (!i->acked && std::chrono::steady_clock::now() > i->resend) {
					socket->SendPacket(i->packet);
					i->resend = std::chrono::steady_clock::now() + timeout.requestRetryInterval;
				}
				++i;
			}
		}

		if (std::chrono::steady_clock::now() > heartBeatTime.load()) {
			UDPPacket heartbeat;
			heartbeat.address = peerAddr;
			UDPHeader hbtHeader = {
				.index = currentIndex++,
				.sessionID = sessionID,
				.flag = UDPHeaderFlag::HBT
			};
			heartbeat.SetHeader(hbtHeader);
			heartBeatTime = std::chrono::steady_clock::now() + timeout.connectionRetryInterval;
			socket->SendPacket(reinterpret_cast<Packet&>(heartbeat));
		}
	}
	else if (status == ConnectionStatus::Pending) {
		if (std::chrono::steady_clock::now() > heartBeatTime.load()) {
			// Case 1: Client received the packet but the ack packet is lost
			UDPPacket resync;
			resync.address = peerAddr;
			UDPHeader resyncHeader = {
				.sessionID = sessionID,
				.flag = UDPHeaderFlag::SYN
			};
			resync.SetHeader(resyncHeader);
			heartBeatTime = std::chrono::steady_clock::now() + timeout.connectionRetryInterval;
			socket->SendPacket(reinterpret_cast<Packet&>(resync));

			// Case 2: Client did not receive the packet, so retransmit the original message
			UDPPacket resync2;
			resync2.address = peerAddr;
			UDPHeader resyncHeader2 = {
				.index = sessionID,
				.sessionID = clientChecksum,
				.flag = UDPHeaderFlag::SYN | UDPHeaderFlag::ACK
			};
			resync2.SetHeader(resyncHeader2);
			heartBeatTime = std::chrono::steady_clock::now() + timeout.connectionRetryInterval;
			socket->SendPacket(reinterpret_cast<Packet&>(resync2));
		}
	}
	else if (status == ConnectionStatus::Connecting) {
		if (std::chrono::steady_clock::now() > heartBeatTime.load()) {
			UDPPacket resync;
			resync.address = peerAddr;
			UDPHeader resyncHeader = {
				.index = sessionID,
				.sessionID = 0,
				.flag = UDPHeaderFlag::SYN
			};
			resync.SetHeader(resyncHeader);
			heartBeatTime = std::chrono::steady_clock::now() + timeout.connectionRetryInterval;
			socket->SendPacket(reinterpret_cast<Packet&>(resync));
		}
	}
	return true;
}

std::future<std::optional<UDPPacket>> UDPConnection::Send(UDPPacket& packet) {
	// No peer connected, do nothing
	std::lock_guard<std::mutex> guard(lock);
	if (status != ConnectionStatus::Connected) {
		std::cout << "Attempting to send data via unconnected connection" << std::endl;
		return {};
	}

	UDPHeader header = packet.Header();
	if (header.flag & UDPHeaderFlag::ACK && header.flag & UDPHeaderFlag::IMP) {
		std::cout << "ACK cannot be used together with IMP, see doc for more information." << std::endl;
		return {};
	}

	header.sessionID = sessionID;
	header.index = currentIndex++;
	packet.SetHeader(header);
	packet.address = peerAddr;

	socket->SendPacket(reinterpret_cast<const Packet&>(packet));
	if (header.flag & UDPHeaderFlag::IMP) {
		awaitAck.push_back(
			AwaitAck{
				.index = header.index,
				.packet = reinterpret_cast<const Packet&>(packet),
				.resend = std::chrono::steady_clock::now() + timeout.impRetryInterval
			}
		);
		return {};
	}

	if (!(header.flag & UDPHeaderFlag::REQ)) {
		return {};
	}

	std::promise<std::optional<UDPPacket>> promise;
	std::future<std::optional<UDPPacket>> asyncResult = promise.get_future();
	// If indicated to handle request response asynchronously, the response handler will be used to process the reponse
	awaitResponse.push_back(
		AwaitResponse{
			.index = header.index,
			.promise = std::move(promise),
			.packet = reinterpret_cast<const Packet&>(packet),
			.timeout = std::chrono::steady_clock::now() + timeout.requestTimeout,
			.resend = std::chrono::steady_clock::now() + timeout.requestRetryInterval,
			.acked = false
		}
	);

	return asyncResult;
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
	awaitAck.clear();
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
	cv.notify_all();
}

bool UDPConnection::Wait(std::optional<std::chrono::steady_clock::duration> waitTime) {
	std::unique_lock<std::mutex> guard(lock);
	if (status == ConnectionStatus::Connected || status == ConnectionStatus::Disconnected) return status == ConnectionStatus::Connected;

	auto predicate = [&]() { return status == ConnectionStatus::Connected || status == ConnectionStatus::Disconnected; };

	if (waitTime)
	{
		cv.wait_for(guard, waitTime.value(), predicate);
	}
	else { 
		cv.wait(guard, predicate); 
	}
	return status == ConnectionStatus::Connected;
}

uint32_t UDPConnectionManager::GenerateChecksum() {
	checksumGenerator.seed(std::random_device{}());
	return std::uniform_int_distribution<uint32_t>{1, UINT32_MAX}(checksumGenerator);
}

UDPConnectionManager::UDPConnectionManager(USHORT port, size_t numConnections, size_t packetQueueCapacity, DWORD maxPacketSize, std::chrono::steady_clock::duration updateInterval)
	: socket(std::make_shared<UDPSocket>(port, maxPacketSize)), numConnections(numConnections), connections(numConnections), updateInterval(updateInterval), queueCapacity(packetQueueCapacity),
	routeThread(&UDPConnectionManager::RouteAndTimeout, this)
{

}

UDPConnectionManager::UDPConnectionManager(size_t packetQueueCapacity, size_t numConnections, DWORD maxPacketSize, std::chrono::steady_clock::duration updateInterval)
	: socket(std::make_shared<UDPSocket>(maxPacketSize)), numConnections(numConnections), updateInterval(updateInterval), queueCapacity(packetQueueCapacity),
	routeThread(&UDPConnectionManager::RouteAndTimeout, this)
{

}

UDPConnectionManager::~UDPConnectionManager() {
	{
		std::lock_guard<std::mutex> guard(lock);
		std::cout << "Closing all connections" << std::endl;
		for (auto i = 0; i < numConnections; i++) { // Close all connections after the socket is closed
			auto ptr = connections[i].lock();
			if (ptr) {
				ptr->Disconnect();
			}
		}
		socket->Close();
	}

	if (routeThread.joinable()) routeThread.join();
}

void UDPConnectionManager::RouteAndTimeout() {
	std::vector<std::shared_ptr<UDPConnection>> tempConnections(numConnections);
	while (true) { 
		{
			std::lock_guard<std::mutex> guard(lock);
			if (socket->IsClosed()) break;

			for (auto i = 0; i < numConnections; i++) {
				tempConnections[i] = connections[i].lock();
			}

			auto packet = socket->Read();
			while (packet) {
				UDPPacket& udpPacket = reinterpret_cast<UDPPacket&>(packet.value());
				if (udpPacket.Good()) {
					UDPHeader header = udpPacket.Header();
					bool routed = false;
					std::cout << "Host received packet with sessionID " << header.sessionID << std::endl;
					for (auto i = 0; i < numConnections; i++) {
						if (tempConnections[i] && !tempConnections[i]->Closed() && header.sessionID == tempConnections[i]->sessionID) {
							std::cout << "Connection with sessionID " << tempConnections[i]->sessionID << std::endl;
							routed = true;
							tempConnections[i]->ParsePacket(header, std::move(udpPacket));
							break;
						}
					}

					// Packet was not delivered to any opened connections, could be a request to open new connection
					if (!routed && isListening) {
						if (header.sessionID == 0 && header.flag & UDPHeaderFlag::SYN) {
							connectionRequests.emplace_back(
								ConnectionRequest{
									.address = udpPacket.address,
									.checksum = header.index
								}
							);
							cv.notify_one();
						}
					}
				}

				packet = socket->Read();
			}

			// Ask active connections to process timeouts and remove closed connections
			for (auto i = 0; i < numConnections; i++) {
				if (tempConnections[i] &&
					(tempConnections[i]->Closed() || !tempConnections[i]->UpdateTimeout())
					)
				{
					connections[i].reset();
				}
			}
		}
		std::this_thread::sleep_for(updateInterval);
	}
}

std::shared_ptr<UDPConnection> UDPConnectionManager::Accept(TimeoutSetting timeout, std::optional<std::chrono::steady_clock::duration> waitTime) {
	if (socket->IsClosed()) return {};

	// Only accept connection when there's spot available
	std::unique_lock<std::mutex> guard(lock);
	if (!isListening && connectionRequests.empty()) return {};

	size_t index = numConnections;

	for (size_t i = 0; i < numConnections; i++) {
		auto ptr = connections[i].lock();
		if (!ptr || ptr->Closed()) {
			index = i;
			break;
		}
	}
	if (index == numConnections) return {};

	auto predicate = [&]() { return !connectionRequests.empty(); };
	if (waitTime) {
		if (!cv.wait_for(guard, waitTime.value(), predicate)) return {};
	}
	else {
		cv.wait(guard, predicate);
	}

	ConnectionRequest request = connectionRequests.front();
	connectionRequests.pop_front();

	uint32_t checksum = GenerateChecksum();
	auto result = std::make_shared<UDPConnection>(socket, queueCapacity, request.address, checksum, timeout);
	result->status = UDPConnection::ConnectionStatus::Pending;
	result->clientChecksum = request.checksum;
	connections[index] = result;
	guard.unlock();

	UDPPacket synMessage;
	synMessage.address = request.address;
	std::cout << "Received client number " << request.checksum << std::endl;
	std::cout << "Server using number " << checksum << std::endl;
	UDPHeader header = {
		.index = checksum,
		.sessionID = request.checksum,
		.flag = UDPHeaderFlag::SYN | UDPHeaderFlag::ACK,
	};
	synMessage.SetHeader(header);
	socket->SendPacket(reinterpret_cast<const Packet&>(synMessage));

	return result;
}

bool UDPConnectionManager::Good() const { return !socket->IsClosed(); }

size_t UDPConnectionManager::Count() {
	std::lock_guard<std::mutex> guard(lock);
	size_t count = 0;
	for (size_t i = 0; i < numConnections; i++) {
		auto ptr = connections[i].lock();
		if (ptr) {
			if (ptr->Closed()) connections[i].reset();
			else { ++count; }
		}
	}
	return count;
}

std::shared_ptr<UDPConnection> UDPConnectionManager::ConnectPeer(sockaddr_in peerAddr, TimeoutSetting timeout) {
	// Do nothing if socket is closed
	if (socket->IsClosed()) return {};

	std::lock_guard<std::mutex> guard(lock);
	size_t index = numConnections;
	for (size_t i = 0; i < numConnections; i++) {
		auto ptr = connections[i].lock();
		if (!ptr || ptr->Closed()) {
			index = i;
			break;
		}
	}
	// No available spots left for new connections
	if (index == numConnections) return {};

	uint32_t checksum = GenerateChecksum();
	std::cout << "Client using number " << checksum << std::endl;
	UDPPacket packet;
	packet.address = peerAddr;
	UDPHeader header = {
		.index = checksum,
		.sessionID = 0,
		.flag = UDPHeaderFlag::SYN
	};
	packet.SetHeader(header);

	auto result = std::make_shared<UDPConnection>(socket, queueCapacity, peerAddr, checksum, timeout);
	result->status = UDPConnection::ConnectionStatus::Connecting;
	connections[index] = result;

	socket->SendPacket(reinterpret_cast<Packet&>(packet));

	return result;
}