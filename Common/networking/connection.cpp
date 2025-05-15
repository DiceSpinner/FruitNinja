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

#pragma warning(push)
#pragma warning(disable: 26813)
void UDPConnection::ParsePacket(const UDPHeader& header, UDPPacket&& packet) {
	bool goodPacket = false;
	std::lock_guard<std::mutex> guard(connectionLock);

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
					for (auto i = awaitResponse.begin(); i != awaitResponse.end();) {
						i->promise.set_value({});
						i = awaitResponse.erase(i);
					}
					awaitAck.clear();
					packetQueue.clear();
					return;
				}

				// Sync packet sent to the server is lost, retransmit
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
					goodPacket = true;
					break;
				}

				// Handle acknoledgements from peer
				if (header.flag & UDPHeaderFlag::ACK) {
					// Do nothing if it is heart beat acknowledgement
					if (header.flag & UDPHeaderFlag::HBT) {
						goodPacket = true;
					}
					else if(header.flag & UDPHeaderFlag::IMP){
						// Check if acknoledgement matches any important messages
						if (packetQueue.size() < queueCapacity) {
							for (auto i = awaitAck.begin(); i != awaitAck.end(); ++i) {
								if (i->index == header.ackIndex) {
									goodPacket = true;
									awaitAck.erase(i);
									break;
								}
							}
						}
					}
					else {
						// Check if acknoledgement matches any requests
						for (auto i = awaitResponse.begin(); i != awaitResponse.end(); ++i) {
							// Packet's ownership will be transferred to the response handler if matches
							if (i->index == header.ackIndex) {
								try {
									i->promise.set_value(std::move(packet));
								}
								catch (const std::future_error& e) {
									if (e.code() == std::make_error_code(std::future_errc::broken_promise)) {
										// Expected: client abandoned waiting
									}
									else {
										throw;
									}
								}
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
						.sessionID = sessionID,
						.flag = UDPHeaderFlag::ACK | UDPHeaderFlag::HBT
					};
					reply.SetHeader(replyHeader);
					socket->SendPacket(reinterpret_cast<const Packet&>(reply));
					break;
				}

				if (header.flag & UDPHeaderFlag::IMP) {
					auto sendAck = false;

					// Prevents double processing by checking if the packet has already been acknoledged
					for (auto i = ackedIndices.rbegin(); i != ackedIndices.rend();++i) {
						if (*i == header.index) {
							sendAck = true;
							break;
						}
					}
					if (!sendAck) {
						if (packetQueue.size() < queueCapacity) {
							sendAck = true;
							packetQueue.emplace_back(std::move(packet));
							ackedIndices.push_back(header.index);
							if (ackedIndices.size() > queueCapacity) ackedIndices.pop_front();
						}
					}

					if (sendAck) {
						UDPPacket reply;
						reply.address = peerAddr;
						UDPHeader replyHeader = {
							.ackIndex = header.index,
							.sessionID = sessionID,
							.flag = UDPHeaderFlag::ACK | UDPHeaderFlag::IMP
						};
						reply.SetHeader(replyHeader);
						socket->SendPacket(reinterpret_cast<Packet&>(reply));
						std::cout << "Acking imp\n";
					}
					
					goodPacket = true;
					break;
				}

				// Regular data packets will be stored in a queue
				if (packetQueue.size() < queueCapacity) {
					packetQueue.emplace_back(std::move(packet));
				}
				goodPacket = true;
				break;
			}
			
		case ConnectionStatus::Connecting:
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
				return;
			}
			break;
		case ConnectionStatus::Pending:
			if (header.flag == UDPHeaderFlag::ACK && !packet.DataSize()) // Received client ack, connection established
			{
				std::cout << "Server connected " << std::endl;
				status = ConnectionStatus::Connected;

				// Manually update during initialization
				peerAddr = packet.address;
				lastReceived = std::chrono::steady_clock::now();
				heartBeatTime = lastReceived.load() + timeout.connectionRetryInterval;
				return;
			}
			else if (header.flag == UDPHeaderFlag::SYN && header.sessionID == 0) { // Client did not receive the packet
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
			}
			break;
			
	}

	// Difference between indices greater than 2^31 is considered wrapped around
	if (goodPacket) {
		if (int32_t(header.index) - int32_t(latestReceivedIndex) > 0) {
			peerAddr = packet.address;
			latestReceivedIndex = header.index;
		}
		lastReceived = std::chrono::steady_clock::now();
		heartBeatTime = lastReceived.load() + timeout.connectionRetryInterval;
	}
}
#pragma warning(pop)

bool UDPConnection::Connected() { 
	std::lock_guard<std::mutex> guard(connectionLock);
	return status == ConnectionStatus::Connected; 
}

bool UDPConnection::Closed() {
	std::lock_guard<std::mutex> guard(connectionLock);
	return status == ConnectionStatus::Disconnected;
}

sockaddr_in UDPConnection::PeerAddr() { 
	std::lock_guard<std::mutex> guard(connectionLock); return peerAddr; 
}

std::optional<UDPPacket> UDPConnection::Receive() {
	// No peer connected, do nothing
	std::lock_guard<std::mutex> guard(connectionLock);
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
	std::lock_guard<std::mutex> guard(connectionLock);
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
				if (std::chrono::steady_clock::now() > i->resend) {
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
}

std::future<std::optional<UDPPacket>> UDPConnection::Send(UDPPacket& packet) {
	// No peer connected, do nothing
	std::lock_guard<std::mutex> guard(connectionLock);
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
			.resend = std::chrono::steady_clock::now() + timeout.requestRetryInterval
		}
	);

	return asyncResult;
}

void UDPConnection::TimeoutDisconnect() {
	std::lock_guard<std::mutex> guard(connectionLock);
	std::cout << "Connection timed out!" << std::endl;
	status = ConnectionStatus::Disconnected;
	for (auto i = awaitResponse.begin(); i != awaitResponse.end();) {
		i->promise.set_value({});
		i = awaitResponse.erase(i);
	}
	packetQueue.clear();
	awaitAck.clear();
}

void UDPConnection::Disconnect() {
	std::lock_guard<std::mutex> guard(connectionLock);
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
}