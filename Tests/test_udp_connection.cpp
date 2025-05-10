#include <catch2/catch_test_macros.hpp>
#include "networking/connection.hpp"

TEST_CASE("UDPConnection 3-way handshake succeeds and closure", "[UDPConnection]") {
    UDPConnectionManager<2> host1(30000, 10, 1500, std::chrono::milliseconds(10));
    REQUIRE(host1.Good());

    UDPConnectionManager<2> host2(40000, 10, 1500, std::chrono::milliseconds(10));
    REQUIRE(host2.Good());
    host1.isListening = true;
    host2.isListening = true;

    sockaddr_in addr1 = {};
    addr1.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr1.sin_family = AF_INET;
    addr1.sin_port = htons(30000);

    sockaddr_in addr2 = {};
    addr2.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr2.sin_family = AF_INET;
    addr2.sin_port = htons(40000);

    auto c1 = host1.ConnectPeer(addr2, std::chrono::milliseconds(1000)).lock();
    REQUIRE(c1);
    REQUIRE(host1.Count() == 1);

    auto s1 = host2.Accept(DEFAULT_TIMEOUT, std::chrono::milliseconds(1000)).lock();
    REQUIRE(s1);
    REQUIRE(host2.Count() == 1);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    REQUIRE(c1->Connected());
    REQUIRE(s1->Connected());

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    REQUIRE(!c1->Closed());
    REQUIRE(!s1->Closed());
    REQUIRE(host1.Count() == 1);
    REQUIRE(host2.Count() == 1);

    c1->Disconnect();
    REQUIRE(c1->Closed());
    REQUIRE(host1.Count() == 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(s1->Closed());
    REQUIRE(host2.Count() == 0);
}

TEST_CASE("UDPConnection timeout after one side forcefully disconnect", "[UDPConnection]") {
    UDPConnectionManager<2> host1(30000, 10, 1500, std::chrono::milliseconds(10));
    REQUIRE(host1.Good());

    UDPConnectionManager<2> host2(40000, 10, 1500, std::chrono::milliseconds(10));
    REQUIRE(host2.Good());
    host1.isListening = true;
    host2.isListening = true;

    sockaddr_in addr1 = {};
    addr1.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr1.sin_family = AF_INET;
    addr1.sin_port = htons(30000);

    sockaddr_in addr2 = {};
    addr2.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr2.sin_family = AF_INET;
    addr2.sin_port = htons(40000);

    auto c1 = host1.ConnectPeer(addr2, std::chrono::milliseconds(1000)).lock();
    REQUIRE(c1);
    REQUIRE(host1.Count() == 1);

    auto s1 = host2.Accept(DEFAULT_TIMEOUT, std::chrono::milliseconds(1000)).lock();
    REQUIRE(s1);
    REQUIRE(host2.Count() == 1);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    REQUIRE(c1->Connected());
    REQUIRE(s1->Connected());

    c1->SimulateDisconnect();
    c1->Disconnect();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(c1->Closed());
    REQUIRE(host1.Count() == 0);
    REQUIRE(s1->Connected());
    REQUIRE(host2.Count() == 1);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    REQUIRE(s1->Closed());
    REQUIRE(host2.Count() == 0);
}