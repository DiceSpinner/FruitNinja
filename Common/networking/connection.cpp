#include "connection.hpp"

void UDPHeader::Serialize(const UDPHeader& header, std::span<char> buffer) {
	if (buffer.size() < sizeof(UDPHeader)) { 
		std::cout << "Size of buffer too small for UDPHeader" << std::endl;
		return; 
	}
	auto index = htonl(header.index);
	auto ackIndex = htonl(header.ackIndex);
	std::memcpy(buffer.data(), &index, sizeof(PacketIndex));
	std::memcpy(buffer.data() + offsetof(UDPHeader, ackIndex), &ackIndex, sizeof(PacketIndex));
	std::memcpy(buffer.data() + offsetof(UDPHeader, flag), &header.flag, sizeof(flag));
}

std::optional<UDPHeader> UDPHeader::Deserialize(std::span<const char> buffer) {
	if (buffer.size() < sizeof(UDPHeader)) return {};
	u_long index;
	memcpy(&index, buffer.data(), sizeof(u_long));
	u_long ackIndex;
	memcpy(&ackIndex, buffer.data() + offsetof(UDPHeader, ackIndex), sizeof(u_long));
	uint8_t flag;
	memcpy(&flag, buffer.data() + offsetof(UDPHeader, flag), sizeof(flag));

	return UDPHeader{
		.index = ntohl(index),
		.ackIndex = ntohl(ackIndex),
		.flag = static_cast<UDPHeaderFlag>(flag)
	};
}

UDPPacket::UDPPacket() : payload(), address() {
	payload.reserve(UDP_PACKET_DATA_SIZE + sizeof(UDPHeader));
	payload.resize(sizeof(UDPHeader));
}

UDPHeader UDPPacket::Header() const {
	return UDPHeader::Deserialize({ payload.data(), sizeof(UDPHeader) }).value();
}

void UDPPacket::SetHeader(const UDPHeader& header) {
	UDPHeader::Serialize(header, { payload.data(), sizeof(UDPHeader) });
}

void UDPPacket::Append(std::span<const char> data) {
	payload.insert(payload.end(), data.begin(), data.end());
}

std::span<char> UDPPacket::Data() {
	return { payload.data() + sizeof(UDPHeader), payload.size() - sizeof(UDPHeader) };
}

void UDPPacket::ResizeData(size_t size) {
	payload.resize(size + sizeof(UDPHeader));
}

size_t UDPPacket::DataSize() const { return payload.size() - sizeof(UDPPacket); }
    

UDPConnection::UDPConnection(size_t packetBufferSize, std::function<void(const sockaddr_in&)> onDisconnect)
	: socket(packetBufferSize), peer(), currentIndex(0), onDisconnect(onDisconnect)
{

}

const sockaddr_in& UDPConnection::Address() const { return socket.Address(); }

void UDPConnection::Listen() {
	if (ConnectedPeer()) return;
	socket.SetMatchAddress({});
	socket.blocked = false;	
	currentIndex = 0;
	
	while (true) {
		auto packet = socket.ReadFront();
		if (packet) {
			UDPPacket data = reinterpret_cast<UDPPacket&>(packet.value());
			if (data.Header().flag | UDPHeaderFlag::SYN) {
				std::cout << "Received syn request" << std::endl;
				socket.SetMatchAddress(data.address);

				UDPPacket response;
				response.address = data.address;

				UDPHeader header {};
				header.flag = UDPHeaderFlag::SYN;
				header.ackIndex = data.Header().index;
				header.index = currentIndex;
				response.SetHeader(header);
				socket.SendPacket(reinterpret_cast<Packet&>(response));
				auto reply = socket.Wait(
					[&](const Packet& packet) {
						const UDPPacket& data = reinterpret_cast<const UDPPacket&>(packet);
						return data.Header().flag & UDPHeaderFlag::ACK && data.Header().ackIndex == currentIndex;
					}
					, std::chrono::milliseconds(5000));
				if (!reply) { 
					socket.SetMatchAddress({});
					std::cout << "Waiting for ACK timed out" << std::endl;
					continue;
				}
				peer = data.address;
				currentIndex++;
				std::cout << "Accepted client" << std::endl;
				return;
			}
		}
	}
}

std::optional<UDPPacket> UDPConnection::Receive() {
	// No peer connected, do nothing
	if (peer.sin_family == AF_UNSPEC) {
		return {};
	}

	std::optional<Packet> packet = socket.ReadFront();
	if (packet) {
		return reinterpret_cast<UDPPacket&>(packet.value());
	}
	return {};
}

std::optional<sockaddr_in> UDPConnection::ConnectedPeer() {
	if (peer.sin_family == AF_UNSPEC) return {};
	return peer;
}

void UDPConnection::ConnectPeer(const sockaddr_in& address) {
	if (SockAddrInEqual(address, socket.Address())) {
		return;
	}
	if (peer.sin_family != AF_UNSPEC) {
		Disconnect();
	}
	
	currentIndex = 0;
	UDPPacket packet;
	packet.address = address;
	UDPHeader header = {
		.index = 1,
		.ackIndex = 0,
		.flag = UDPHeaderFlag::SYN
	};
	packet.SetHeader(header);

	socket.blocked = false;
	socket.SetMatchAddress(address);
	socket.SendPacket(reinterpret_cast<Packet&>(packet));

	auto syncPacket = socket.Wait(
		[&](const Packet& packet) {
			auto& data = reinterpret_cast<const UDPPacket&>(packet);
			auto header = data.Header();
			return header.flag & UDPHeaderFlag::SYN && header.ackIndex == 1;
		}
		, std::chrono::milliseconds(3000)
	);

	if(!syncPacket) {
		// Timeout or no correct response from target
		socket.SetMatchAddress({});
		socket.blocked = true;
		return;
	}
	auto syncDataHeader = reinterpret_cast<UDPPacket&>(syncPacket.value()).Header();
	// Connected, acknowledging the connection
	UDPPacket replyPacket;
	replyPacket.address = address;
	UDPHeader replyHeader = {
		.index = 2,
		.ackIndex = syncDataHeader.index,
		.flag = (UDPHeaderFlag::ACK | UDPHeaderFlag::SYN)
	};
	currentIndex = 3;
	replyPacket.SetHeader(replyHeader);
	socket.SendPacket(reinterpret_cast<Packet&>(replyPacket));
	peer = address;
	std::cout << "Connected to server" << "\n";
}

void UDPConnection::Disconnect() {
	if (peer.sin_family == AF_UNSPEC) {
		return;
	}
	UDPPacket packet;
	packet.address = peer;

	UDPHeader header = {
		.index = currentIndex,
		.flag = UDPHeaderFlag::FIN
	};
	packet.SetHeader(header);

	UDPPacket response;
	socket.SendPacket(std::span<const char>{packet.payload.data(), packet.payload.size()}, peer);
	socket.Wait(
		[&](const Packet& packet) {
			const UDPPacket& item = reinterpret_cast<const UDPPacket&>(packet);
			if (item.Header().flag | UDPHeaderFlag::FIN) {
				return true;
			}
			return false;
		}, 
		std::chrono::milliseconds(3000)
	);

	socket.blocked = true;
	if (onDisconnect) {
		onDisconnect(peer);
	}
	peer = {};
}