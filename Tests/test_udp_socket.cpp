#include <catch2/catch_test_macros.hpp>
#include "networking/socket.hpp"

TEST_CASE("UDPSocket send and receive msg", "[UDPSocket]") {
    UDPSocket socket1(5);
    UDPSocket socket2(5);
    const std::string msg = "Hello from socket 1";

    sockaddr_in addr1 = {};
    addr1.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr1.sin_family = AF_INET;
    addr1.sin_port = socket1.Port();

    sockaddr_in addr2 = {};
    addr2.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr2.sin_family = AF_INET;
    addr2.sin_port = socket2.Port();

    Packet packet{ 
        .address = addr2,
        .payload = {msg.data(), msg.data() + msg.length()},
    };
    socket1.SendPacket(packet);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto result = socket2.ReadBack();
    REQUIRE(result);
    auto& recvPacket = result.value();
    REQUIRE(std::string(recvPacket.payload.begin(), recvPacket.payload.end()) == msg);

    const std::string reply = "Hello from socket 2";
    Packet replyPacket{
        .address = recvPacket.address,
        .payload = {reply.data(), reply.data() + reply.length()}
    };
    socket2.SendPacket(replyPacket);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto result2 = socket1.ReadBack();
    REQUIRE(result2);
    auto& recvPacket2 = result2.value();
    REQUIRE(std::string(recvPacket2.payload.begin(), recvPacket2.payload.end()) == reply);
}

TEST_CASE("UDPSocket send and receive msg on selected port", "[UDPSocket]") {
    UDPSocket socket1(30000, 5, DEFAULT_UDP_BUFFER_SIZE);
    UDPSocket socket2(40000, 5, DEFAULT_UDP_BUFFER_SIZE);
    const std::string msg = "Hello from socket 1";

    sockaddr_in addr2 = {};
    addr2.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr2.sin_family = AF_INET;
    addr2.sin_port = socket2.Port();

    Packet packet{
        .address = addr2,
        .payload = {msg.data(), msg.data() + msg.length()},
    };
    socket1.SendPacket(packet);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto result = socket2.ReadBack();
    REQUIRE(result);
    auto& recvPacket = result.value();
    REQUIRE(std::string(recvPacket.payload.begin(), recvPacket.payload.end()) == msg);

    const std::string reply = "Hello from socket 2";
    Packet replyPacket{
        .address = recvPacket.address,
        .payload = {reply.data(), reply.data() + reply.length()}
    };
    socket2.SendPacket(replyPacket);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto result2 = socket1.ReadBack();
    REQUIRE(result2);
    auto& recvPacket2 = result2.value();
    REQUIRE(std::string(recvPacket2.payload.begin(), recvPacket2.payload.end()) == reply);
}

TEST_CASE("UDPSocket respect queue capacity and blocking", "[UDPSocket]") {
    UDPSocket sender(1);
    UDPSocket receiver(2);

    sockaddr_in receiverAddr = {};
    receiverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    receiverAddr.sin_family = AF_INET;
    receiverAddr.sin_port = receiver.Port();

    Packet p1 = {
        .address = receiverAddr,
        .payload = {},
    };
    sender.SendPacket(p1);
    sender.SendPacket(p1);
    sender.SendPacket(p1);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(receiver.ReadBack());
    REQUIRE(receiver.ReadBack());
    REQUIRE(!receiver.ReadBack());

    receiver.blocked = true;
    sender.SendPacket(p1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(!receiver.ReadBack());
    receiver.blocked = false;
    sender.SendPacket(p1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(receiver.ReadBack());
    receiver.Close();
    REQUIRE(receiver.IsClosed());
    sender.SendPacket(p1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(!receiver.ReadBack());
}

TEST_CASE("UDPSocket wait for required packet", "[UDPSocket]") {
    UDPSocket sender(1);
    UDPSocket receiver(2);
    receiver.blocked = false;

    sockaddr_in receiverAddr = {};
    receiverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    receiverAddr.sin_family = AF_INET;
    receiverAddr.sin_port = receiver.Port();

    const std::string msg1 = "Message 1!";
    const std::string msg2 = "Message 2!";
    Packet p1 = {
        .address = receiverAddr,
        .payload = {msg1.data(), msg1.data() + msg1.length()},
    };
    Packet p2 = {
        .address = receiverAddr,
        .payload = {msg2.data(), msg2.data() + msg2.length()},
    };
    
    sender.SendPacket(p1);
    sender.SendPacket(p1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    sender.SendPacket(p2);

    auto isMsg1 = [&](const Packet& packet) { return std::string(packet.payload.begin(), packet.payload.end()) == msg1; };
    auto isMsg2 = [&](const Packet& packet) { return std::string(packet.payload.begin(), packet.payload.end()) == msg2; };

    REQUIRE(!receiver.Wait(isMsg2, std::chrono::milliseconds(200)));
    REQUIRE(receiver.Wait(isMsg1, std::chrono::milliseconds(10)));
    REQUIRE(receiver.Wait(isMsg1, std::chrono::milliseconds(10)));

    sender.SendPacket(p2);
    REQUIRE(!receiver.Wait(isMsg1, std::chrono::milliseconds(100)));
    sender.SendPacket(p1);
    REQUIRE(receiver.Wait(isMsg1, std::chrono::milliseconds(100)));
    REQUIRE(receiver.Wait(isMsg2, std::chrono::milliseconds(10)));
}

TEST_CASE("UDPSocket drop oversized packet but keep packet <= bufferSize", "[UDPSocket]") {
    UDPSocket socket1(5, 1800);
    socket1.blocked = false;
    UDPSocket socket2(5, 1500);
    socket2.blocked = false;
    const std::string msg(socket2.MaxPacketSize() + 100, 'x');
    const std::string msg2(socket2.MaxPacketSize(), 'x');

    sockaddr_in addr1 = {};
    addr1.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr1.sin_family = AF_INET;
    addr1.sin_port = socket1.Port();

    sockaddr_in addr2 = {};
    addr2.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr2.sin_family = AF_INET;
    addr2.sin_port = socket2.Port();

    Packet packet{
        .address = addr2,
        .payload = {msg.data(), msg.data() + msg.length()},
    };
    Packet packet2{
        .address = addr2,
        .payload = {msg2.data(), msg2.data() + msg2.length()},
    };

    socket1.SendPacket(packet);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto result = socket2.ReadBack();
    REQUIRE(!result);

    packet.address = addr1;
    socket2.SendPacket(packet);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    result = socket1.ReadBack();
    REQUIRE(result);

    socket1.SendPacket(packet2);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    result = socket2.ReadBack();
    REQUIRE(result);

    packet2.address = addr1;
    socket2.SendPacket(packet2);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    result = socket1.ReadBack();
    REQUIRE(result);
}