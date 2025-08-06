#include "lite_conn.hpp"
#include "debug/log.hpp"
#include <cassert>

LiteConnResponse::LiteConnResponse(){}

LiteConnResponse::LiteConnResponse(
	std::weak_ptr<LiteConnConnection>&& connection, 
	const uint64_t& id, 
	std::future<std::optional<LiteConnMessage>>&& value
) : connection(std::move(connection)), requestID(id), response(std::move(value))
{ }

LiteConnResponse::operator bool() {
	return response.valid();
}

LiteConnResponse::~LiteConnResponse() {
	if (response.valid()) {
		auto ptr = connection.lock();
		if (ptr) {
			ptr->CloseRequest(requestID);
		}
	}
}

void LiteConnResponse::Cancel() {
	if (response.valid()) {
		auto ptr = connection.lock();
		if (ptr) {
			ptr->CloseRequest(requestID);
		}
		response = {};
	}
}

std::optional<LiteConnMessage> LiteConnResponse::GetResponse() {
	if (!response.valid()) return {};
	return response.get();
}

LiteConnRequest::LiteConnRequest(
	std::weak_ptr<LiteConnConnection>&& connection, 
	const uint64_t& id
) : connection(std::move(connection)), requestID(id), isValid(true)
{

}

LiteConnRequest::~LiteConnRequest() {
	if (!std::exchange(isValid, false)) return;
	auto ptr = connection.lock();
	if (ptr) {
		ptr->RejectRequest(requestID);
	}
}

bool LiteConnRequest::IsValid() const {
	return isValid;
}

LiteConnRequest::operator bool() const {
	return isValid;
}

void LiteConnRequest::Reject() {
	if (!std::exchange(isValid, false)) return;
	auto ptr = connection.lock();
	if (ptr) {
		ptr->RejectRequest(requestID);
	}
}

void LiteConnRequest::Respond(const std::span<const char> response) {
	if (!std::exchange(isValid, false)) return;
	auto ptr = connection.lock();
	if (ptr) {
		ptr->Respond(requestID, response);
	}
}

std::optional<LiteConnResponse> LiteConnRequest::Converse(const std::span<const char> data) {
	if (!std::exchange(isValid, false)) return {};
	auto ptr = connection.lock();
	if (ptr) {
		return ptr->Converse(requestID, data);
	}
	return {};
}


LiteConnConnection::LiteConnConnection(
	std::shared_ptr<UDPSocket> socket, size_t packetQueueCapacity, 
	sockaddr_in peerAddr, uint32_t sessionID, TimeoutSetting setting
)
	: peerAddr(peerAddr), queueCapacity(packetQueueCapacity), socket(std::move(socket)), 
	lastReceived(std::chrono::steady_clock::now()), 
	pktIndex(0), timeout(setting), sessionID(sessionID), latestReceivedIndex(0),
	status(ConnectionStatus::Disconnected), heartBeatTime(std::chrono::steady_clock::now() + setting.connectionRetryInterval)
{

}

void LiteConnConnection::UpdateAddress(const LiteConnHeader& header, const sockaddr_in& address) {
	if (int32_t(header.index) - int32_t(latestReceivedIndex) >= 0) {
		peerAddr = address;
		latestReceivedIndex = header.index;
	}
	lastReceived = std::chrono::steady_clock::now();
	heartBeatTime = lastReceived.load() + timeout.connectionRetryInterval;
}

void LiteConnConnection::AckReceival(const LiteConnHeader& header) {
	LiteConnHeader replyHeader = {
		.sessionID = sessionID,
		.index = pktIndex++,
		.flag = LiteConnHeaderFlag::ACK,
		.id32 = header.id32,
	};
	auto reply = LiteConnHeader::Serialize(replyHeader);
	socket->SendPacket(reply, peerAddr);
	autoAcks.emplace(
		header.id32,
		std::chrono::steady_clock::now() + timeout.replyKeepDuration
	);
}

void LiteConnConnection::SendPacket(LiteConnHeader& header, const std::span<const char> data) {
	header.sessionID = sessionID;
	header.index = pktIndex++;
	auto payload = LiteConnHeader::Serialize(header);
	payload.insert(payload.end(), data.begin(), data.end());
	socket->SendPacket(payload, peerAddr);
}

void LiteConnConnection::SendPacketReliable(LiteConnHeader& header, const std::span<const char> data) {
	header.sessionID = sessionID;
	header.index = pktIndex++;
	header.id32 = impIndex++;
	auto payload = LiteConnHeader::Serialize(header);
	payload.insert(payload.end(), data.begin(), data.end());
	socket->SendPacket(payload, peerAddr);
	auto pair = autoResendEntries.emplace(
		header.id32,
		AutoResendEntry{
			.packet = std::move(payload),
			.resend = std::chrono::steady_clock::now() + timeout.impRetryInterval
		}
	);
}

void LiteConnConnection::CloseRequest(uint64_t index) {
	std::lock_guard<std::mutex> guard(lock);
	auto requestEntry = requestHandles.find(index);
	if (requestEntry == requestHandles.end()) return;
	autoResendEntries.erase(index);
	requestEntry->second.set_value(std::nullopt);
	requestHandles.erase(requestEntry);

	LiteConnHeader replyHeader = {
		.sessionID = sessionID,
		.index = pktIndex++,
		.flag = LiteConnHeaderFlag::CXL,
		.id64 = index,
	};
	SendPacketReliable(replyHeader, {});
}

void LiteConnConnection::RejectRequest(uint64_t index) {
	std::lock_guard<std::mutex> guard(lock);
	if (!LiteConnResponses.erase(index)) return;
	
	LiteConnHeader header = {
		.sessionID = sessionID,
		.index = pktIndex++,
		.flag = LiteConnHeaderFlag::ACK | LiteConnHeaderFlag::CXL,
		.id64 = index,
	};

	SendPacketReliable(header, {});
}

void LiteConnConnection::Respond(uint64_t index, const std::span<const char>& data) {
	std::lock_guard<std::mutex> guard(lock);
	if (!LiteConnResponses.erase(index)) return;

	LiteConnHeader replyHeader = {
		.flag = LiteConnHeaderFlag::DATA | LiteConnHeaderFlag::ACK,
		.id64 = index,
	};
	SendPacketReliable(replyHeader, data);
}

std::optional<LiteConnResponse> LiteConnConnection::Converse(uint64_t index, const std::span<const char>& data){
	std::lock_guard<std::mutex> guard(lock);
	if (!LiteConnResponses.erase(index)) return {};

	LiteConnHeader replyHeader = {
		.flag = LiteConnHeaderFlag::DATA | LiteConnHeaderFlag::ACK | LiteConnHeaderFlag::REQ,
		.id64 = index,
	};
	std::vector<char> payload(sizeof(uint64_t) + data.size());
	uint64_t requestID = reqIndex++;
	auto ptr = payload.data();
	auto serializedID = htonll(requestID);
	memcpy(std::exchange(ptr, ptr + sizeof(serializedID)), &serializedID, sizeof(uint64_t));
	memcpy(ptr, data.data(), data.size());

	SendPacketReliable(replyHeader, payload);
	auto entry = requestHandles.emplace(requestID, std::promise<std::optional<LiteConnMessage>>{});
	assert(entry.second);
	auto& kwp = entry.first;
	return LiteConnResponse{ weak_from_this(), kwp->first, kwp->second.get_future() };
}

#pragma warning(push)
#pragma warning(disable: 26813)
bool LiteConnConnection::TryHandleMissedHandshake(const LiteConnHeader& header, const std::vector<char>& data) {
	if (header.flag == LiteConnHeaderFlag::SYN) {
		LiteConnHeader replyHeader = {
			.sessionID = sessionID,
			.flag = LiteConnHeaderFlag::ACK,
		};
		Debug::Log("Client retransmit syn-ack message");
		auto reply = LiteConnHeader::Serialize(replyHeader);
		socket->SendPacket(reply, peerAddr);
		return true;
	}
	return false;
}

bool LiteConnConnection::TryHandleDisconnect(const LiteConnHeader& header) {
	if (header.flag & LiteConnHeaderFlag::FIN) {
		Debug::Log("Received FIN signal from peer, closing connection");
		status = ConnectionStatus::Disconnected;
		for (auto& promise : requestHandles) {
			promise.second.set_value(std::nullopt);
		}
		requestHandles.clear();
		autoResendEntries.clear();
		packetQueue.clear();
		return true;
	}
	return false;
}

bool LiteConnConnection::TryHandleDuplicates(const LiteConnHeader& header) {
	if (header.flag & (LiteConnHeaderFlag::CXL | LiteConnHeaderFlag::IMP | LiteConnHeaderFlag::REQ)) {
		auto ackEntry = autoAcks.find(header.id32);
		if (ackEntry != autoAcks.end()) {
			ackEntry->second = std::chrono::steady_clock::now() + timeout.replyKeepDuration;
			LiteConnHeader replyHeader = {
				.sessionID = sessionID,
				.index = pktIndex++,
				.flag = LiteConnHeaderFlag::ACK,
				.id32 = header.id32
			};
			SendPacket(replyHeader, {});
			return true;
		}
	}
	return false;
}

bool LiteConnConnection::TryHandleAcknowledgement(const LiteConnHeader& header, std::vector<char>& data) {
	if (header.flag & LiteConnHeaderFlag::ACK) {
		if (!(header.flag & LiteConnHeaderFlag::DATA)) { 
			autoResendEntries.erase(header.id32);
			return true; 
		}

		auto result = requestHandles.find(header.id64);
		if (result == requestHandles.end()) {
			// Tells the peer the REQ has expired or not exist
			LiteConnHeader reply = {
				.flag = LiteConnHeaderFlag::CXL,
				.id64 = header.id64
			};

			SendPacketReliable(reply, {});
			return true;
		}
		if (header.flag & LiteConnHeaderFlag::REQ) {
			if (data.size() < sizeof(uint64_t)) {
				Debug::LogError("Error: The received REQ does not have its request id in the payload");
				return true;
			}
			uint64_t id;
			memcpy(&id, data.data(), sizeof(uint64_t));
			id = ntohll(id);

			AckReceival(header);
			LiteConnResponses.emplace(id);
			data.erase(data.begin(), data.begin() + sizeof(uint64_t));
			result->second.set_value(LiteConnMessage{ std::move(data), LiteConnRequest{weak_from_this(), id} } );
		}
		else {
			result->second.set_value(LiteConnMessage{ std::move(data), std::nullopt });
		}
		requestHandles.erase(result);
		return true;
	}
	return false;
}

bool LiteConnConnection::TryHandleData(const LiteConnHeader& header, std::vector<char>& data) {
	if (header.flag & LiteConnHeaderFlag::DATA) {
		if (packetQueue.size() < queueCapacity) {
			if (header.flag & LiteConnHeaderFlag::IMP) {
				AckReceival(header);
				packetQueue.emplace_back(std::move(data), std::nullopt);
			}
			else if (header.flag & LiteConnHeaderFlag::REQ) {
				AckReceival(header);
				LiteConnResponses.emplace(header.id64);
				packetQueue.emplace_back(std::move(data), LiteConnRequest{ weak_from_this(), header.id64 });
			}
			else {
				packetQueue.emplace_back(std::move(data), std::nullopt);
			}
			cv.notify_one();
		}
		return true;
	}
	return false;
}

bool LiteConnConnection::TryHandleHeartBeat(const LiteConnHeader& header, const std::vector<char>& data) {
	// Acknowledge heartbeat packets
	if (header.flag == LiteConnHeaderFlag::HBT) {
		LiteConnHeader replyHeader = {
			.sessionID = sessionID,
			.index = pktIndex++,
			.flag = LiteConnHeaderFlag::ACK | LiteConnHeaderFlag::HBT
		};
		auto reply = LiteConnHeader::Serialize(replyHeader);
		socket->SendPacket(reply, peerAddr);
		return true;
	}
	return false;
}

bool LiteConnConnection::TryHandleRequestCancellation(const LiteConnHeader& header, const std::vector<char>& data){
	if (header.flag & LiteConnHeaderFlag::CXL) {
		if (header.flag & LiteConnHeaderFlag::ACK) { // Peer cancels request that was sent
			auto entry = requestHandles.find(header.id64);
			if (entry != requestHandles.end()) {
				entry->second.set_value(std::nullopt);
				requestHandles.erase(entry);
			}
		}
		else { // Peer cancels their own request
			LiteConnResponses.erase(header.id64);
		}
		AckReceival(header);
		return true;
	}
	return false;
}

void LiteConnConnection::HandleServerAcknowledgement(const LiteConnHeader& header, const std::vector<char>& data) {
	if (header.flag == (LiteConnHeaderFlag::SYN | LiteConnHeaderFlag::ACK)) {
		sessionID = header.id32;
		status = ConnectionStatus::Connected;
		LiteConnHeader replyHeader = {
			.sessionID = sessionID,
			.flag = LiteConnHeaderFlag::ACK,
		};
		Debug::Log("Client acknowledge session id ", sessionID);
		auto reply = LiteConnHeader::Serialize(replyHeader);
		socket->SendPacket(reply, peerAddr);

		cv.notify_all();
	}
}

bool LiteConnConnection::TryHandleMissedAcknowledgement(const LiteConnHeader& header, const std::vector<char>& data) {
	// Client did not receive the packet
	if (header.flag == LiteConnHeaderFlag::SYN && header.sessionID == 0) { 
		LiteConnHeader syncHeader = {
			.sessionID = clientChecksum,
			.flag = LiteConnHeaderFlag::SYN | LiteConnHeaderFlag::ACK,
			.id32 = sessionID,
		};
		auto synMessage = LiteConnHeader::Serialize(syncHeader);
		socket->SendPacket(synMessage, peerAddr);
		return true;
	}
	return false;
}

void LiteConnConnection::HandleClientAcknowledgement(const LiteConnHeader& header, const std::vector<char>& data) {
	// Received client ack, connection established
	if (header.flag == LiteConnHeaderFlag::ACK && data.size() == 0)
	{
		Debug::Log("Server connected");
		status = ConnectionStatus::Connected;
		cv.notify_all();
		return;
	}
}

void LiteConnConnection::ParsePacket(const LiteConnHeader& header, std::vector<char>&& data, const sockaddr_in& address) {
	std::lock_guard<std::mutex> guard(lock);

	if (status == ConnectionStatus::Disconnected) {
		Debug::Log("A packet is routed to a closed UDPConnection");
		return;
	}

	UpdateAddress(header, address);

	if (TryHandleDisconnect(header)) return;

	switch (status) {
	case ConnectionStatus::Pending:
		if (TryHandleMissedAcknowledgement(header, data)) return;
		HandleClientAcknowledgement(header, data);
		return;

	case ConnectionStatus::Connecting:
		HandleServerAcknowledgement(header, data);
		return;

	case ConnectionStatus::Connected:
		if (TryHandleMissedHandshake(header, data)) return;
		if (TryHandleHeartBeat(header, data)) return;
		if (TryHandleDuplicates(header)) return;
		if (TryHandleRequestCancellation(header, data)) return;
		if (TryHandleAcknowledgement(header, data)) return;
		if (TryHandleData(header, data)) return;
		return;
	}
}

#pragma warning(pop)

bool LiteConnConnection::IsConnected() { 
	std::lock_guard<std::mutex> guard(lock);
	return status == ConnectionStatus::Connected; 
}

bool LiteConnConnection::IsDisconnected() {
	std::lock_guard<std::mutex> guard(lock);
	return status == ConnectionStatus::Disconnected;
}

sockaddr_in LiteConnConnection::PeerAddr() { 
	std::lock_guard<std::mutex> guard(lock); return peerAddr; 
}

std::optional<LiteConnMessage> LiteConnConnection::Receive() {
	// No peer connected, do nothing
	std::lock_guard<std::mutex> guard(lock);
	if (status == ConnectionStatus::Disconnected) {
		return {};
	}
	if (packetQueue.empty()) {
		return {};
	}
	LiteConnMessage msg = std::move(packetQueue.front());
	packetQueue.pop_front();
	return msg;
}

bool LiteConnConnection::UpdateTimeout() {
	std::lock_guard<std::mutex> guard(lock);
	if (std::chrono::steady_clock::now() - lastReceived.load() > timeout.connectionTimeout) {
		Debug::Log("Connection timed out!");
		status = ConnectionStatus::Disconnected;
		for (auto& promise : requestHandles) {
			promise.second.set_value(std::nullopt);
		}
		LiteConnResponses.clear();
		requestHandles.clear();
		autoResendEntries.clear();
		packetQueue.clear();
		cv.notify_all();
		return false;
	}

	if (status == ConnectionStatus::Connected) {
		for (auto i = autoResendEntries.begin(); i != autoResendEntries.end(); ++i) {
			auto& entry = i->second;
			if (std::chrono::steady_clock::now() > entry.resend) {
				auto header = LiteConnHeader::Deserialize(entry.packet);
				assert(header);
				auto& hd = header.value();
				assert(hd.id32 == i->first);
				hd.index = pktIndex++;
				LiteConnHeader::Serialize(hd, entry.packet);
				socket->SendPacket(entry.packet, peerAddr);
				entry.resend = std::chrono::steady_clock::now() + timeout.impRetryInterval;
			}
		}
		for (auto i = autoAcks.begin(); i != autoAcks.end();) {
			if (std::chrono::steady_clock::now() > i->second) {
				i = autoAcks.erase(i);
			}
			else {
				++i;
			}
		}

		if (std::chrono::steady_clock::now() > heartBeatTime.load()) {
			LiteConnHeader hbtHeader = {
				.sessionID = sessionID,
				.index = pktIndex++,
				.flag = LiteConnHeaderFlag::HBT
			};
			auto heartbeat = LiteConnHeader::Serialize(hbtHeader);
			heartBeatTime = std::chrono::steady_clock::now() + timeout.connectionRetryInterval;
			socket->SendPacket(heartbeat, peerAddr);
		}
	}
	else if (status == ConnectionStatus::Pending) {
		if (std::chrono::steady_clock::now() > heartBeatTime.load()) {
			// Case 1: Client received the packet but the ack packet is lost
			LiteConnHeader resyncHeader = {
				.sessionID = sessionID,
				.flag = LiteConnHeaderFlag::SYN
			};
			auto resync = LiteConnHeader::Serialize(resyncHeader);
			heartBeatTime = std::chrono::steady_clock::now() + timeout.connectionRetryInterval;
			socket->SendPacket(resync, peerAddr);

			// Case 2: Client did not receive the packet, so retransmit the original message
			LiteConnHeader resyncHeader2 = {
				.sessionID = clientChecksum,
				.flag = LiteConnHeaderFlag::SYN | LiteConnHeaderFlag::ACK,
				.id32 = sessionID,
			};
			auto resync2 = LiteConnHeader::Serialize(resyncHeader2);
			heartBeatTime = std::chrono::steady_clock::now() + timeout.connectionRetryInterval;
			socket->SendPacket(resync2, peerAddr);
		}
	}
	else if (status == ConnectionStatus::Connecting) {
		if (std::chrono::steady_clock::now() > heartBeatTime.load()) {
			LiteConnHeader resyncHeader = {
				.sessionID = 0,
				.flag = LiteConnHeaderFlag::SYN,
				.id32 = sessionID,
			};
			auto resync = LiteConnHeader::Serialize(resyncHeader);
			heartBeatTime = std::chrono::steady_clock::now() + timeout.connectionRetryInterval;
			socket->SendPacket(resync, peerAddr);
		}
	}
	return true;
}

void LiteConnConnection::SendData(const std::span<const char> data) {
	// No peer connected, do nothing
	std::lock_guard<std::mutex> guard(lock);
	if (status != ConnectionStatus::Connected) {
		Debug::Log("Attempting to send data via unconnected connection");
		return;
	}

	LiteConnHeader header = {
		.sessionID = sessionID,
		.index = pktIndex++,
		.flag = LiteConnHeaderFlag::DATA,
		.id32 = 0,
	};
	auto payload = LiteConnHeader::Serialize(header);
	payload.insert(payload.end(), data.begin(), data.end());
	socket->SendPacket(std::span<const char> {payload.data(), payload.size()}, peerAddr);
}

void LiteConnConnection::SendReliableData(const std::span<const char> data) {
	// No peer connected, do nothing
	std::lock_guard<std::mutex> guard(lock);
	if (status != ConnectionStatus::Connected) {
		Debug::LogError("Attempting to send data via unconnected connection");
		return;
	}

	LiteConnHeader header = {
		.flag = LiteConnHeaderFlag::IMP | LiteConnHeaderFlag::DATA
	};
	
	SendPacketReliable(header, data);
}

std::optional<LiteConnResponse> LiteConnConnection::SendRequest(const std::span<const char> data) {
	// No peer connected, do nothing
	std::lock_guard<std::mutex> guard(lock);
	if (status != ConnectionStatus::Connected) {
		Debug::LogError("Attempting to send data via unconnected connection");
		return {};
	}

	LiteConnHeader header = {
		.flag = LiteConnHeaderFlag::REQ | LiteConnHeaderFlag::DATA,
		.id64 = reqIndex++,
	};

	SendPacketReliable(header, data);
	auto entry = requestHandles.emplace(header.id64, std::promise<std::optional<LiteConnMessage>>{});
	assert(entry.second);
	auto& kwp = entry.first;
	return LiteConnResponse{ weak_from_this(), kwp->first, kwp->second.get_future() };
}

void LiteConnConnection::Disconnect() {
	std::lock_guard<std::mutex> guard(lock);
	if (status == ConnectionStatus::Disconnected) {
		return;
	}
	Debug::Log("Closing connection.");
	status = ConnectionStatus::Disconnected;
	for (auto i = requestHandles.begin(); i != requestHandles.end();) {
		i->second.set_value(std::nullopt);
		i = requestHandles.erase(i);
	}
	packetQueue.clear();
	autoAcks.clear();

	LiteConnHeader header = {
		.sessionID = sessionID,
		.index = pktIndex,
		.flag = LiteConnHeaderFlag::FIN
	};

	auto packet = LiteConnHeader::Serialize(header);

	socket->SendPacket(packet, peerAddr);
	cv.notify_all();
}

bool LiteConnConnection::WaitForConnectionComplete() {
	std::unique_lock<std::mutex> guard(lock);
	if (status == ConnectionStatus::Connected) return true;
	if (status == ConnectionStatus::Disconnected) return false;

	auto predicate = [&]() { return status == ConnectionStatus::Connected || status == ConnectionStatus::Disconnected; };

	cv.wait(guard, predicate); 
	return status == ConnectionStatus::Connected;
}

bool LiteConnConnection::WaitForDataPacket() {
	std::unique_lock<std::mutex> guard(lock);
	if (status != ConnectionStatus::Connected) return false;

	auto predicate = [&]() { return status == ConnectionStatus::Disconnected || !packetQueue.empty(); };
	cv.wait(guard, predicate);
	return status == ConnectionStatus::Connected && !packetQueue.empty();
}

LiteConnConnection::~LiteConnConnection() {
	if (status == ConnectionStatus::Disconnected) {
		return;
	}
	Debug::Log("Closing connection.");
	status = ConnectionStatus::Disconnected;
	for (auto i = requestHandles.begin(); i != requestHandles.end();) {
		i->second.set_value(std::nullopt);
		i = requestHandles.erase(i);
	}
	packetQueue.clear();
	autoAcks.clear();

	LiteConnHeader header = {
		.sessionID = sessionID,
		.index = pktIndex,
		.flag = LiteConnHeaderFlag::FIN
	};

	auto packet = LiteConnHeader::Serialize(header);

	socket->SendPacket(packet, peerAddr);
	cv.notify_all();
}

uint32_t LiteConnManager::GenerateChecksum() {
	checksumGenerator.seed(std::random_device{}());
	return std::uniform_int_distribution<uint32_t>{1, UINT32_MAX}(checksumGenerator);
}

LiteConnManager::LiteConnManager(USHORT port, size_t numConnections, size_t packetQueueCapacity, DWORD maxPacketSize, std::chrono::steady_clock::duration updateInterval)
	: socket(std::make_shared<UDPSocket>(port, maxPacketSize)), numConnections(numConnections), connections(numConnections), updateInterval(updateInterval), queueCapacity(packetQueueCapacity),
	routeThread(&LiteConnManager::RouteAndTimeout, this)
{

}

LiteConnManager::LiteConnManager(size_t packetQueueCapacity, size_t numConnections, DWORD maxPacketSize, std::chrono::steady_clock::duration updateInterval)
	: socket(std::make_shared<UDPSocket>(maxPacketSize)), numConnections(numConnections), connections(numConnections), updateInterval(updateInterval), queueCapacity(packetQueueCapacity),
	routeThread(&LiteConnManager::RouteAndTimeout, this)
{

}

LiteConnManager::~LiteConnManager() {
	{
		std::lock_guard<std::mutex> guard(lock);
		Debug::Log("Closing all connections");
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

void LiteConnManager::RouteAndTimeout() {
	while (true) { 
		auto currentTime = std::chrono::steady_clock::now();
		{
			std::lock_guard<std::mutex> guard(lock);
			
			if (socket->IsClosed()) break;

			auto packet = socket->Read();
			while (packet) {
				auto& payload = packet.value().payload;
				auto address = packet.value().address;
				auto hd = LiteConnHeader::Deserialize(payload);

				if (hd) {
					const LiteConnHeader& header = hd.value();
					bool routed = false;
					// Debug::Log("Host received packet with sessionID ", header.sessionID);

					for (auto i = 0; i < numConnections; i++) {
						auto temp = connections[i].lock();
						if (temp && !temp->IsDisconnected() && header.sessionID == temp->sessionID) {
							// Debug::Log("Routed to connection with sessionID ", tempConnections[i]->sessionID);
							routed = true;
							payload.erase(payload.begin(), payload.begin() + LiteConnHeader::Size);
							temp->ParsePacket(header, std::move(payload), address);
							break;
						}
					}

					// Packet was not delivered to any opened connections, could be a request to open new connection
					if (!routed && isListening) {
						if (header.sessionID == 0 && header.flag & LiteConnHeaderFlag::SYN) {
							connectionRequests.emplace_back(
								ConnectionRequest {
									.address = address,
									.checksum = header.id32
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
				auto temp = connections[i].lock();
				if (temp &&
					(temp->IsDisconnected() || !temp->UpdateTimeout())
					)
				{
					connections[i].reset();
				}
			}
		}
		std::this_thread::sleep_until(currentTime + updateInterval);
	}
}

std::shared_ptr<LiteConnConnection> LiteConnManager::Accept(TimeoutSetting timeout, std::optional<std::chrono::steady_clock::duration> waitTime) {
	if (socket->IsClosed()) return {};

	// Only accept connection when there's spot available
	std::unique_lock<std::mutex> guard(lock);
	if (!isListening && connectionRequests.empty()) return {};

	size_t index = numConnections;

	for (size_t i = 0; i < numConnections; i++) {
		auto ptr = connections[i].lock();
		if (!ptr || ptr->IsDisconnected()) {
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
	auto result = std::make_shared<LiteConnConnection>(socket, queueCapacity, request.address, checksum, timeout);
	result->status = LiteConnConnection::ConnectionStatus::Pending;
	result->clientChecksum = request.checksum;
	connections[index] = result;
	guard.unlock();

	Debug::Log("Received client number ", request.checksum);
	Debug::Log("Server using number ", checksum);
	LiteConnHeader header = {
		.sessionID = request.checksum,
		.flag = LiteConnHeaderFlag::SYN | LiteConnHeaderFlag::ACK,
		.id32 = checksum,
	};
	auto synMessage = LiteConnHeader::Serialize(header);
	socket->SendPacket(synMessage, request.address);

	return result;
}

bool LiteConnManager::Good() const { return !socket->IsClosed(); }

size_t LiteConnManager::Count() {
	std::lock_guard<std::mutex> guard(lock);
	size_t count = 0;
	for (size_t i = 0; i < numConnections; i++) {
		auto ptr = connections[i].lock();
		if (ptr) {
			if (ptr->IsDisconnected()) connections[i].reset();
			else { ++count; }
		}
	}
	return count;
}

std::shared_ptr<LiteConnConnection> LiteConnManager::ConnectPeer(sockaddr_in peerAddr, TimeoutSetting timeout) {
	// Do nothing if socket is closed
	if (socket->IsClosed()) return {};

	std::lock_guard<std::mutex> guard(lock);
	size_t index = numConnections;
	for (size_t i = 0; i < numConnections; i++) {
		auto ptr = connections[i].lock();
		if (!ptr || ptr->IsDisconnected()) {
			index = i;
			break;
		}
	}
	// No available spots left for new connections
	if (index == numConnections) return {};

	uint32_t checksum = GenerateChecksum();
	Debug::Log("Client using number ", checksum);
	LiteConnHeader header = {
		.sessionID = 0,
		.flag = LiteConnHeaderFlag::SYN,
		.id32 = checksum,
	};
	auto packet = LiteConnHeader::Serialize(header);

	auto result = std::make_shared<LiteConnConnection>(socket, queueCapacity, peerAddr, checksum, timeout);
	result->status = LiteConnConnection::ConnectionStatus::Connecting;
	connections[index] = result;

	socket->SendPacket(packet, peerAddr);

	return result;
}