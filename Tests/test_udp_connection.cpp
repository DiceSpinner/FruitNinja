#include <catch2/catch_test_macros.hpp>
#include "networking/connection.hpp"

TEST_CASE("UDPConnection 3-way handshake succeeds and send data packets", "[UDPConnection]") {
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

    auto c1 = host1.ConnectPeer(addr2, std::chrono::milliseconds(500)).lock();
    REQUIRE(c1);
    REQUIRE(host1.Count() == 1);

    auto s1 = host2.Accept(DEFAULT_TIMEOUT, std::chrono::milliseconds(500)).lock();
    REQUIRE(s1);
    REQUIRE(host2.Count() == 1);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    REQUIRE(c1->Connected());
    REQUIRE(s1->Connected());

    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    REQUIRE(c1->Closed());
    REQUIRE(s1->Closed());
    REQUIRE(host1.Count() == 0);
    REQUIRE(host2.Count() == 0);
}