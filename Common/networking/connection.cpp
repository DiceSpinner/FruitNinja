#include "connection.hpp"
#include "debug/log.hpp"
#include <cassert>

RequestHandle::RequestHandle(
	std::weak_ptr<UDPConnection>&& connection, 
	const uint64_t& id, 
	std::future<std::optional<Message>>&& value
) : connection(std::move(connection)), requestID(id), response(std::move(value))
{ }

RequestHandle::~RequestHandle() {
	if (response.valid()) {
		auto ptr = connection.lock();
		if (ptr) {
			ptr->CloseRequest(requestID);
		}
	}
}

void RequestHandle::Cancel() {
	if (response.valid()) {
		auto ptr = connection.lock();
		if (ptr) {
			ptr->CloseRequest(requestID);
		}
		response = {};
	}
}

std::optional<Message> RequestHandle::GetResponse() {
	if (!response.valid()) return {};
	return response.get();
}

ResponseHandle::ResponseHandle(
	std::weak_ptr<UDPConnection>&& connection, 
	const uint64_t& id
) : connection(std::move(connection)), requestID(id), isValid(true)
{

}

ResponseHandle::~ResponseHandle() {
	if (!std::exchange(isValid, false)) return;
	auto ptr = connection.lock();
	if (ptr) {
		ptr->RejectRequest(requestID);
	}
}

bool ResponseHandle::IsValid() const {
	return isValid;
}

ResponseHandle::operator bool() const {
	return isValid;
}

void ResponseHandle::Reject() {
	if (!std::exchange(isValid, false)) return;
	auto ptr = connection.lock();
	if (ptr) {
		ptr->RejectRequest(requestID);
	}
}

void ResponseHandle::Respond(const std::span<const char> response) {
	if (!std::exchange(isValid, false)) return;
	auto ptr = connection.lock();
	if (ptr) {
		ptr->Respond(requestID, response);
	}
}

std::optional<RequestHandle> ResponseHandle::Converse(const std::span<const char> data) {
	if (!std::exchange(isValid, false)) return {};
	auto ptr = connection.lock();
	if (ptr) {
		return ptr->Converse(requestID, data);
	}
	return {};
}


UDPConnection::UDPConnection(
	std::shared_ptr<UDPSocket> socket, size_t packetQueueCapacity, 
	sockaddr_in peerAddr, uint32_t sessionID, TimeoutSetting setting
)
	: peerAddr(peerAddr), queueCapacity(packetQueueCapacity), socket(std::move(socket)), 
	lastReceived(std::chrono::steady_clock::now()), 
	pktIndex(0), timeout(setting), sessionID(sessionID), latestReceivedIndex(0),
	status(ConnectionStatus::Disconnected), heartBeatTime(std::chrono::steady_clock::now() + setting.connectionRetryInterval)
{

}

void UDPConnection::UpdateAddress(const UDPHeader& header, const sockaddr_in& address) {
	if (int32_t(header.index) - int32_t(latestReceivedIndex) >= 0) {
		peerAddr = address;
		latestReceivedIndex = header.index;
	}
	lastReceived = std::chrono::steady_clock::now();
	heartBeatTime = lastReceived.load() + timeout.connectionRetryInterval;
}

void UDPConnection::AckReceival(const UDPHeader& header) {
	UDPHeader replyHeader = {
		.sessionID = sessionID,
		.index = pktIndex++,
		.flag = UDPHeaderFlag::ACK,
		.id32 = header.id32,
	};
	auto reply = UDPHeader::Serialize(replyHeader);
	socket->SendPacket(reply, peerAddr);
	autoAcks.emplace(
		header.id32,
		std::chrono::steady_clock::now() + timeout.replyKeepDuration
	);
}

void UDPConnection::SendPacket(UDPHeader& header, const std::span<const char> data) {
	header.sessionID = sessionID;
	header.index = pktIndex++;
	auto payload = UDPHeader::Serialize(header);
	payload.insert(payload.end(), data.begin(), data.end());
	socket->SendPacket(payload, peerAddr);
}

void UDPConnection::SendPacketReliable(UDPHeader& header, const std::span<const char> data) {
	header.sessionID = sessionID;
	header.index = pktIndex++;
	header.id32 = impIndex++;
	auto payload = UDPHeader::Serialize(header);
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

void UDPConnection::CloseRequest(uint64_t index) {
	std::lock_guard<std::mutex> guard(lock);
	auto requestEntry = requestHandles.find(index);
	if (requestEntry == requestHandles.end()) return;
	autoResendEntries.erase(index);
	requestEntry->second.set_value(std::nullopt);
	requestHandles.erase(requestEntry);

	UDPHeader replyHeader = {
		.sessionID = sessionID,
		.index = pktIndex++,
		.flag = UDPHeaderFlag::CXL,
		.id64 = index,
	};
	SendPacketReliable(replyHeader, {});
}

void UDPConnection::RejectRequest(uint64_t index) {
	std::lock_guard<std::mutex> guard(lock);
	if (!responseHandles.erase(index)) return;
	
	UDPHeader header = {
		.sessionID = sessionID,
		.index = pktIndex++,
		.flag = UDPHeaderFlag::ACK | UDPHeaderFlag::CXL,
		.id64 = index,
	};

	SendPacketReliable(header, {});
}

void UDPConnection::Respond(uint64_t index, const std::span<const char>& data) {
	std::lock_guard<std::mutex> guard(lock);
	if (!responseHandles.erase(index)) return;

	UDPHeader replyHeader = {
		.flag = UDPHeaderFlag::DATA | UDPHeaderFlag::ACK,
		.id64 = index,
	};
	SendPacketReliable(replyHeader, data);
}

std::optional<RequestHandle> UDPConnection::Converse(uint64_t index, const std::span<const char>& data){
	std::lock_guard<std::mutex> guard(lock);
	if (!responseHandles.erase(index)) return {};

	UDPHeader replyHeader = {
		.flag = UDPHeaderFlag::DATA | UDPHeaderFlag::ACK | UDPHeaderFlag::REQ,
		.id64 = index,
	};
	std::vector<char> payload(sizeof(uint64_t) + data.size());
	uint64_t requestID = reqIndex++;
	auto ptr = payload.data();
	auto serializedID = htonll(requestID);
	memcpy(std::exchange(ptr, ptr + sizeof(serializedID)), &serializedID, sizeof(uint64_t));
	memcpy(ptr, data.data(), data.size());

	SendPacketReliable(replyHeader, payload);
	auto entry = requestHandles.emplace(requestID, std::promise<std::optional<Message>>{});
	assert(entry.second);
	auto& kwp = entry.first;
	return RequestHandle{ weak_from_this(), kwp->first, kwp->second.get_future() };
}

#pragma warning(push)
#pragma warning(disable: 26813)
bool UDPConnection::TryHandleMissedHandshake(const UDPHeader& header, const std::vector<char>& data) {
	if (header.flag == UDPHeaderFlag::SYN) {
		UDPHeader replyHeader = {
			.sessionID = sessionID,
			.flag = UDPHeaderFlag::ACK,
		};
		Debug::Log("Client retransmit syn-ack message");
		auto reply = UDPHeader::Serialize(replyHeader);
		socket->SendPacket(reply, peerAddr);
		return true;
	}
	return false;
}

bool UDPConnection::TryHandleDisconnect(const UDPHeader& header) {
	if (header.flag & UDPHeaderFlag::FIN) {
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

bool UDPConnection::TryHandleDuplicates(const UDPHeader& header) {
	if (header.flag & (UDPHeaderFlag::CXL | UDPHeaderFlag::IMP | UDPHeaderFlag::REQ)) {
		auto ackEntry = autoAcks.find(header.id32);
		if (ackEntry != autoAcks.end()) {
			ackEntry->second = std::chrono::steady_clock::now() + timeout.replyKeepDuration;
			UDPHeader replyHeader = {
				.sessionID = sessionID,
				.index = pktIndex++,
				.flag = UDPHeaderFlag::ACK,
				.id32 = header.id32
			};
			SendPacket(replyHeader, {});
			return true;
		}
	}
	return false;
}

bool UDPConnection::TryHandleAcknowledgement(const UDPHeader& header, std::vector<char>& data) {
	if (header.flag & UDPHeaderFlag::ACK) {
		if (!(header.flag & UDPHeaderFlag::DATA)) { 
			autoResendEntries.erase(header.id32);
			return true; 
		}

		auto result = requestHandles.find(header.id64);
		if (result == requestHandles.end()) {
			// Tells the peer the REQ has expired or not exist
			UDPHeader reply = {
				.flag = UDPHeaderFlag::CXL,
				.id64 = header.id64
			};

			SendPacketReliable(reply, {});
			return true;
		}
		if (header.flag & UDPHeaderFlag::REQ) {
			if (data.size() < sizeof(uint64_t)) {
				Debug::LogError("Error: The received REQ does not have its request id in the payload");
				return true;
			}
			uint64_t id;
			memcpy(&id, data.data(), sizeof(uint64_t));
			id = ntohll(id);

			AckReceival(header);
			responseHandles.emplace(id);
			data.erase(data.begin(), data.begin() + sizeof(uint64_t));
			result->second.set_value(Message{ std::move(data), ResponseHandle{weak_from_this(), id}, header.index } );
		}
		else {
			result->second.set_value(Message{ std::move(data), std::nullopt,  header.index });
		}
		requestHandles.erase(result);
		return true;
	}
	return false;
}

bool UDPConnection::TryHandleData(const UDPHeader& header, std::vector<char>& data) {
	if (header.flag & UDPHeaderFlag::DATA) {
		if (packetQueue.size() < queueCapacity) {
			if (header.flag & UDPHeaderFlag::IMP) {
				AckReceival(header);
				packetQueue.emplace_back(std::move(data), std::nullopt, header.index);
			}
			else if (header.flag & UDPHeaderFlag::REQ) {
				AckReceival(header);
				responseHandles.emplace(header.id64);
				packetQueue.emplace_back(std::move(data), ResponseHandle{ weak_from_this(), header.id64 }, header.index);
			}
			else {
				packetQueue.emplace_back(std::move(data), std::nullopt, header.index);
			}
			cv.notify_one();
		}
		return true;
	}
	return false;
}

bool UDPConnection::TryHandleHeartBeat(const UDPHeader& header, const std::vector<char>& data) {
	// Acknowledge heartbeat packets
	if (header.flag == UDPHeaderFlag::HBT) {
		UDPHeader replyHeader = {
			.sessionID = sessionID,
			.index = pktIndex++,
			.flag = UDPHeaderFlag::ACK | UDPHeaderFlag::HBT
		};
		auto reply = UDPHeader::Serialize(replyHeader);
		socket->SendPacket(reply, peerAddr);
		return true;
	}
	return false;
}

bool UDPConnection::TryHandleRequestCancellation(const UDPHeader& header, const std::vector<char>& data){
	if (header.flag & UDPHeaderFlag::CXL) {
		if (header.flag & UDPHeaderFlag::ACK) { // Peer cancels request that was sent
			auto entry = requestHandles.find(header.id64);
			if (entry != requestHandles.end()) {
				entry->second.set_value(std::nullopt);
				requestHandles.erase(entry);
			}
		}
		else { // Peer cancels their own request
			responseHandles.erase(header.id64);
		}
		AckReceival(header);
		return true;
	}
	return false;
}

void UDPConnection::HandleServerAcknowledgement(const UDPHeader& header, const std::vector<char>& data) {
	if (header.flag == (UDPHeaderFlag::SYN | UDPHeaderFlag::ACK)) {
		sessionID = header.id32;
		status = ConnectionStatus::Connected;
		UDPHeader replyHeader = {
			.sessionID = sessionID,
			.flag = UDPHeaderFlag::ACK,
		};
		Debug::Log("Client acknowledge session id ", sessionID);
		auto reply = UDPHeader::Serialize(replyHeader);
		socket->SendPacket(reply, peerAddr);

		cv.notify_all();
	}
}

bool UDPConnection::TryHandleMissedAcknowledgement(const UDPHeader& header, const std::vector<char>& data) {
	// Client did not receive the packet
	if (header.flag == UDPHeaderFlag::SYN && header.sessionID == 0) { 
		UDPHeader syncHeader = {
			.sessionID = clientChecksum,
			.flag = UDPHeaderFlag::SYN | UDPHeaderFlag::ACK,
			.id32 = sessionID,
		};
		auto synMessage = UDPHeader::Serialize(syncHeader);
		socket->SendPacket(synMessage, peerAddr);
		return true;
	}
	return false;
}

void UDPConnection::HandleClientAcknowledgement(const UDPHeader& header, const std::vector<char>& data) {
	// Received client ack, connection established
	if (header.flag == UDPHeaderFlag::ACK && data.size() == 0)
	{
		Debug::Log("Server connected");
		status = ConnectionStatus::Connected;
		cv.notify_all();
		return;
	}
}

void UDPConnection::ParsePacket(const UDPHeader& header, std::vector<char>&& data, const sockaddr_in& address) {
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

bool UDPConnection::IsConnected() { 
	std::lock_guard<std::mutex> guard(lock);
	return status == ConnectionStatus::Connected; 
}

bool UDPConnection::IsDisconnected() {
	std::lock_guard<std::mutex> guard(lock);
	return status == ConnectionStatus::Disconnected;
}

sockaddr_in UDPConnection::PeerAddr() { 
	std::lock_guard<std::mutex> guard(lock); return peerAddr; 
}

std::optional<Message> UDPConnection::Receive() {
	// No peer connected, do nothing
	std::lock_guard<std::mutex> guard(lock);
	if (status == ConnectionStatus::Disconnected) {
		return {};
	}
	if (packetQueue.empty()) {
		return {};
	}
	Message msg = std::move(packetQueue.front());
	packetQueue.pop_front();
	return msg;
}

bool UDPConnection::UpdateTimeout() {
	std::lock_guard<std::mutex> guard(lock);
	if (std::chrono::steady_clock::now() - lastReceived.load() > timeout.connectionTimeout) {
		Debug::Log("Connection timed out!");
		status = ConnectionStatus::Disconnected;
		for (auto& promise : requestHandles) {
			promise.second.set_value(std::nullopt);
		}
		responseHandles.clear();
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
				auto header = UDPHeader::Deserialize(entry.packet);
				assert(header);
				auto& hd = header.value();
				assert(hd.id32 == i->first);
				hd.index = pktIndex++;
				UDPHeader::Serialize(hd, entry.packet);
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
			UDPHeader hbtHeader = {
				.sessionID = sessionID,
				.index = pktIndex++,
				.flag = UDPHeaderFlag::HBT
			};
			auto heartbeat = UDPHeader::Serialize(hbtHeader);
			heartBeatTime = std::chrono::steady_clock::now() + timeout.connectionRetryInterval;
			socket->SendPacket(heartbeat, peerAddr);
		}
	}
	else if (status == ConnectionStatus::Pending) {
		if (std::chrono::steady_clock::now() > heartBeatTime.load()) {
			// Case 1: Client received the packet but the ack packet is lost
			UDPHeader resyncHeader = {
				.sessionID = sessionID,
				.flag = UDPHeaderFlag::SYN
			};
			auto resync = UDPHeader::Serialize(resyncHeader);
			heartBeatTime = std::chrono::steady_clock::now() + timeout.connectionRetryInterval;
			socket->SendPacket(resync, peerAddr);

			// Case 2: Client did not receive the packet, so retransmit the original message
			UDPHeader resyncHeader2 = {
				.sessionID = clientChecksum,
				.flag = UDPHeaderFlag::SYN | UDPHeaderFlag::ACK,
				.id32 = sessionID,
			};
			auto resync2 = UDPHeader::Serialize(resyncHeader2);
			heartBeatTime = std::chrono::steady_clock::now() + timeout.connectionRetryInterval;
			socket->SendPacket(resync2, peerAddr);
		}
	}
	else if (status == ConnectionStatus::Connecting) {
		if (std::chrono::steady_clock::now() > heartBeatTime.load()) {
			UDPHeader resyncHeader = {
				.sessionID = 0,
				.flag = UDPHeaderFlag::SYN,
				.id32 = sessionID,
			};
			auto resync = UDPHeader::Serialize(resyncHeader);
			heartBeatTime = std::chrono::steady_clock::now() + timeout.connectionRetryInterval;
			socket->SendPacket(resync, peerAddr);
		}
	}
	return true;
}

void UDPConnection::SendData(const std::span<const char> data) {
	// No peer connected, do nothing
	std::lock_guard<std::mutex> guard(lock);
	if (status != ConnectionStatus::Connected) {
		Debug::Log("Attempting to send data via unconnected connection");
		return;
	}

	UDPHeader header = {
		.sessionID = sessionID,
		.index = pktIndex++,
		.flag = UDPHeaderFlag::DATA,
		.id32 = 0,
	};
	auto payload = UDPHeader::Serialize(header);
	payload.insert(payload.end(), data.begin(), data.end());
	socket->SendPacket(std::span<const char> {payload.data(), payload.size()}, peerAddr);
}

void UDPConnection::SendReliableData(const std::span<const char> data) {
	// No peer connected, do nothing
	std::lock_guard<std::mutex> guard(lock);
	if (status != ConnectionStatus::Connected) {
		Debug::LogError("Attempting to send data via unconnected connection");
		return;
	}

	UDPHeader header = {
		.flag = UDPHeaderFlag::IMP | UDPHeaderFlag::DATA
	};
	
	SendPacketReliable(header, data);
}

std::optional<RequestHandle> UDPConnection::SendRequest(const std::span<const char> data) {
	// No peer connected, do nothing
	std::lock_guard<std::mutex> guard(lock);
	if (status != ConnectionStatus::Connected) {
		Debug::LogError("Attempting to send data via unconnected connection");
		return {};
	}

	UDPHeader header = {
		.flag = UDPHeaderFlag::REQ | UDPHeaderFlag::DATA,
		.id64 = reqIndex++,
	};

	SendPacketReliable(header, data);
	auto entry = requestHandles.emplace(header.id64, std::promise<std::optional<Message>>{});
	assert(entry.second);
	auto& kwp = entry.first;
	return RequestHandle{ weak_from_this(), kwp->first, kwp->second.get_future() };
}

void UDPConnection::Disconnect() {
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

	UDPHeader header = {
		.sessionID = sessionID,
		.index = pktIndex,
		.flag = UDPHeaderFlag::FIN
	};

	auto packet = UDPHeader::Serialize(header);

	socket->SendPacket(packet, peerAddr);
	cv.notify_all();
}

bool UDPConnection::WaitForConnectionComplete() {
	std::unique_lock<std::mutex> guard(lock);
	if (status == ConnectionStatus::Connected) return true;
	if (status == ConnectionStatus::Disconnected) return false;

	auto predicate = [&]() { return status == ConnectionStatus::Connected || status == ConnectionStatus::Disconnected; };

	cv.wait(guard, predicate); 
	return status == ConnectionStatus::Connected;
}

bool UDPConnection::WaitForDataPacket() {
	std::unique_lock<std::mutex> guard(lock);
	if (status != ConnectionStatus::Connected) return false;

	auto predicate = [&]() { return status == ConnectionStatus::Disconnected || !packetQueue.empty(); };
	cv.wait(guard, predicate);
	return status == ConnectionStatus::Connected && !packetQueue.empty();
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
				auto& payload = packet.value().payload;
				auto address = packet.value().address;
				auto hd = UDPHeader::Deserialize(payload);

				if (hd) {
					const UDPHeader& header = hd.value();
					bool routed = false;
					Debug::Log("Host received packet with sessionID ", header.sessionID);

					for (auto i = 0; i < numConnections; i++) {
						if (tempConnections[i] && !tempConnections[i]->IsDisconnected() && header.sessionID == tempConnections[i]->sessionID) {
							Debug::Log("Routed to connection with sessionID ", tempConnections[i]->sessionID);
							routed = true;
							payload.erase(payload.begin(), payload.begin() + UDPHeader::Size);
							tempConnections[i]->ParsePacket(header, std::move(payload), address);
							break;
						}
					}

					// Packet was not delivered to any opened connections, could be a request to open new connection
					if (!routed && isListening) {
						if (header.sessionID == 0 && header.flag & UDPHeaderFlag::SYN) {
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
				if (tempConnections[i] &&
					(tempConnections[i]->IsDisconnected() || !tempConnections[i]->UpdateTimeout())
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
	auto result = std::make_shared<UDPConnection>(socket, queueCapacity, request.address, checksum, timeout);
	result->status = UDPConnection::ConnectionStatus::Pending;
	result->clientChecksum = request.checksum;
	connections[index] = result;
	guard.unlock();

	Debug::Log("Received client number ", request.checksum);
	Debug::Log("Server using number ", checksum);
	UDPHeader header = {
		.sessionID = request.checksum,
		.flag = UDPHeaderFlag::SYN | UDPHeaderFlag::ACK,
		.id32 = checksum,
	};
	auto synMessage = UDPHeader::Serialize(header);
	socket->SendPacket(synMessage, request.address);

	return result;
}

bool UDPConnectionManager::Good() const { return !socket->IsClosed(); }

size_t UDPConnectionManager::Count() {
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

std::shared_ptr<UDPConnection> UDPConnectionManager::ConnectPeer(sockaddr_in peerAddr, TimeoutSetting timeout) {
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
	UDPHeader header = {
		.sessionID = 0,
		.flag = UDPHeaderFlag::SYN,
		.id32 = checksum,
	};
	auto packet = UDPHeader::Serialize(header);

	auto result = std::make_shared<UDPConnection>(socket, queueCapacity, peerAddr, checksum, timeout);
	result->status = UDPConnection::ConnectionStatus::Connecting;
	connections[index] = result;

	socket->SendPacket(packet, peerAddr);

	return result;
}