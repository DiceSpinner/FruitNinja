#include <catch2/catch_test_macros.hpp>
#include "networking/socket.hpp"

TEST_CASE("UDPSocket send and receive msg", "[UDPSocket]") {
    UDPSocket socket1(5);
    socket1.blocked = false;
    UDPSocket socket2(5);
    socket2.blocked = false;
    const std::string msg = "Hello from socket 1";
    Packet packet{ 
        .payload = {msg.data(), msg.data() + msg.length()},
        .address = socket2.Address()
    };
    socket1.SendPacket(packet);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto result = socket2.ReadBack();
    REQUIRE(result);
    auto& recvPacket = result.value();
    REQUIRE(std::string(recvPacket.payload.begin(), recvPacket.payload.end()) == msg);

    const std::string reply = "Hello from socket 2";
    Packet replyPacket{
        .payload = {reply.data(), reply.data() + reply.length()},
        .address = recvPacket.address
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
    receiver.blocked = false;

    Packet p1 = {
        .payload = {},
        .address = receiver.Address(),
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

TEST_CASE("UDPSocket address filter", "[UDPSocket]") {
    UDPSocket sender1(1);
    UDPSocket sender2(1);
    UDPSocket receiver(2);
    receiver.blocked = false;

    Packet p = {
        .payload = {},
        .address = receiver.Address(),
    };

    sender1.SendPacket(p);
    sender2.SendPacket(p);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(receiver.ReadBack());
    REQUIRE(receiver.ReadBack());
    REQUIRE(!receiver.ReadBack());

    receiver.SetMatchAddress(sender2.Address());
    sender1.SendPacket(p);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(!receiver.ReadBack());
    sender2.SendPacket(p);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto result = receiver.ReadBack();
    REQUIRE(result);
    REQUIRE(SockAddrInEqual(result.value().address, sender2.Address()));

    receiver.SetMatchAddress(sender1.Address());
    sender1.SendPacket(p);
    sender2.SendPacket(p);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    result = receiver.ReadBack();
    REQUIRE(result);
    REQUIRE(SockAddrInEqual(result.value().address, sender1.Address()));
    REQUIRE(!receiver.ReadBack());

    receiver.SetMatchAddress({});

    sender1.SendPacket(p);
    sender2.SendPacket(p);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(receiver.ReadBack());
    REQUIRE(receiver.ReadBack());
}

TEST_CASE("UDPSocket wait for required packet", "[UDPSocket]") {
    UDPSocket sender(1);
    UDPSocket receiver(2);
    receiver.blocked = false;

    const std::string msg1 = "Message 1!";
    const std::string msg2 = "Message 2!";
    Packet p1 = {
        .payload = {msg1.data(), msg1.data() + msg1.length()},
        .address = receiver.Address(),
    };
    Packet p2 = {
        .payload = {msg2.data(), msg2.data() + msg2.length()},
        .address = receiver.Address(),
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