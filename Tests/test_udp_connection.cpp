#include <catch2/catch_test_macros.hpp>
#include "networking/connection.hpp"

TEST_CASE("UDPConnection 3-way handshake succeeds and send data packets", "[UDPConnection]") {
    UDPConnection server(5);
    UDPConnection client(5);

    // Get server's address for the client
    sockaddr_in serverAddr = server.Address();

    // Start Listen() in a thread since it blocks
    std::thread listenThread([&] {
        server.Listen();  // blocks until client connects
        });

    // Let the server start listening first
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Now connect client ¡ú this should unblock the server
    client.ConnectPeer(serverAddr);

    listenThread.join();  // wait for server to finish handshake

    // Both sides should now be connected to each other
    REQUIRE(server.ConnectedPeer().has_value());
    REQUIRE(client.ConnectedPeer().has_value());

    REQUIRE(SockAddrInEqual(server.ConnectedPeer().value(), client.Address()));
    REQUIRE(SockAddrInEqual(client.ConnectedPeer().value(), server.Address()));

    UDPPacket p;
    p.address = server.Address();
    p.SetHeader(UDPHeader{.flag = UDPHeaderFlag::REQ});

    std::optional<UDPPacket> response;
    std::thread clientRequestThread([&]() { response = client.Send(p); });

    UDPPacket r;
    r.address = client.Address();
    r.SetHeader(UDPHeader{ .ackIndex = 3, .flag = UDPHeaderFlag::ACK });
    server.Send(r);

    clientRequestThread.join();
    REQUIRE(response.has_value());
    REQUIRE(response.value().Header().flag == UDPHeaderFlag::ACK);
}