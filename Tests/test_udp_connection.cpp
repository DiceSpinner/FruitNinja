#include <catch2/catch_test_macros.hpp>
#include "networking/connection.hpp"

TEST_CASE("UDPConnection 3-way handshake succeeds and closure", "[UDPConnection]") {
    UDPConnectionManager<2> host1(30000, 10, 1500, std::chrono::milliseconds(10));
    REQUIRE(host1.Good());

    UDPConnectionManager<2> host2(40000, 10, 1500, std::chrono::milliseconds(10));
    REQUIRE(host2.Good());
    host1.isListening = true;
    host2.isListening = true;

    TimeoutSetting timeout = {
        .connectionTimeout = std::chrono::milliseconds(1000),
        .connectionRetryInterval = std::chrono::milliseconds(500),
        .requestTimeout = std::chrono::milliseconds(500),
        .requestRetryInterval = std::chrono::milliseconds(250)
    };

    sockaddr_in addr1 = {};
    addr1.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr1.sin_family = AF_INET;
    addr1.sin_port = htons(30000);

    sockaddr_in addr2 = {};
    addr2.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr2.sin_family = AF_INET;
    addr2.sin_port = htons(40000);

    auto c1 = host1.ConnectPeer(addr2, timeout).lock();
    REQUIRE(c1);
    REQUIRE(host1.Count() == 1);

    auto s1 = host2.Accept(timeout).lock();
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

TEST_CASE("UDPConnection 3-way handshake client retry connection", "[UDPConnection]") {
    UDPConnectionManager<2> host1(30000, 10, 1500, std::chrono::milliseconds(10));
    REQUIRE(host1.Good());

    TimeoutSetting timeout = {
        .connectionTimeout = std::chrono::milliseconds(500),
        .connectionRetryInterval = std::chrono::milliseconds(300),
        .requestTimeout = std::chrono::milliseconds(500),
        .requestRetryInterval = std::chrono::milliseconds(250)
    };

    sockaddr_in addr1 = {};
    addr1.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr1.sin_family = AF_INET;
    addr1.sin_port = htons(30000);

    sockaddr_in addr2 = {};
    addr2.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr2.sin_family = AF_INET;
    addr2.sin_port = htons(40000);
    UDPSocket server(40000, 10, 1500);

    auto client = host1.ConnectPeer(addr2, timeout).lock();
    REQUIRE(client);
    REQUIRE(host1.Count() == 1);

    // Verify the first syn packet
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto pkt = server.ReadFront();
        REQUIRE(pkt.has_value());
        auto udpPkt = reinterpret_cast<UDPPacket&>(pkt.value());
        UDPHeader header = udpPkt.Header();
        REQUIRE(header.flag & UDPHeaderFlag::SYN);
        REQUIRE(udpPkt.DataSize() == 0);
        // Currently no more packets should be in the queue
        REQUIRE(!server.ReadFront().has_value());
    }
    // Verify the retry syn packet
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        auto pkt = server.ReadFront();
        REQUIRE(pkt.has_value());
        auto udpPkt = reinterpret_cast<UDPPacket&>(pkt.value());
        UDPHeader header = udpPkt.Header();
        REQUIRE(header.flag & UDPHeaderFlag::SYN);
        REQUIRE(udpPkt.DataSize() == 0);
        // Currently no more packets should be in the queue
        REQUIRE(!server.ReadFront().has_value());
    }

    // Verify the connection has timed out
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        REQUIRE(client->Closed());
        REQUIRE(host1.Count() == 0);
    }
}

TEST_CASE("UDPConnection 3-way handshake server retry connection", "[UDPConnection]") {
    UDPConnectionManager<2> serverHost(30000, 10, 1500, std::chrono::milliseconds(10));
    REQUIRE(serverHost.Good());
    serverHost.isListening = true;

    TimeoutSetting timeout = {
        .connectionTimeout = std::chrono::milliseconds(500),
        .connectionRetryInterval = std::chrono::milliseconds(300),
        .requestTimeout = std::chrono::milliseconds(500),
        .requestRetryInterval = std::chrono::milliseconds(250)
    };

    sockaddr_in serverAddr = {};
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(30000);

    sockaddr_in clientAddr = {};
    clientAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(40000);
    UDPSocket client (40000, 10, 1500);

    // Client initialize connection
    {
        UDPPacket pkt;
        pkt.address = serverAddr;
        UDPHeader header = {
            .index = 5,
            .sessionID = 0,
            .flag = UDPHeaderFlag::SYN
        };
        pkt.SetHeader(header);
        client.SendPacket(reinterpret_cast<Packet&>(pkt));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto server = serverHost.Accept(timeout).lock();
    REQUIRE(server);
    REQUIRE(serverHost.Count() == 1);
    
    // Verify server ack
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto pkt = client.ReadFront();
        REQUIRE(pkt.has_value());
        UDPPacket& udpPacket = reinterpret_cast<UDPPacket&>(pkt.value());
        UDPHeader header = udpPacket.Header();
        REQUIRE(header.flag == (UDPHeaderFlag::SYN | UDPHeaderFlag::ACK));
        REQUIRE(header.sessionID == 5);

        // Currently no more packets should be present in the queue
        REQUIRE(!client.ReadFront().has_value());

        // Verify server retransmission, 2 packets should be received
        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        // Case 1 client ACK is lost so server request for retransmission
        pkt = client.ReadFront();
        REQUIRE(pkt.has_value());
        udpPacket = reinterpret_cast<UDPPacket&>(pkt.value());
        UDPHeader header2 = udpPacket.Header();
        REQUIRE(header2.flag == UDPHeaderFlag::SYN);
        REQUIRE(header2.sessionID == header.index);

        // Case 2 server ACK is lost so server retransmits sessionID
        pkt = client.ReadFront();
        REQUIRE(pkt.has_value());
        udpPacket = reinterpret_cast<UDPPacket&>(pkt.value());
        UDPHeader header3 = udpPacket.Header();
        REQUIRE(header3 == header);
    }

    // Verify the connection has timed out
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        REQUIRE(server->Closed());
        REQUIRE(serverHost.Count() == 0);
    }
}

TEST_CASE("UDPConnection 3-way handshake client retransmit acknoledgement", "[UDPConnection]") {
    UDPConnectionManager<2> clientHost(30000, 10, 1500, std::chrono::milliseconds(10));
    REQUIRE(clientHost.Good());

    TimeoutSetting timeout = {
        .connectionTimeout = std::chrono::milliseconds(1000),
        .connectionRetryInterval = std::chrono::milliseconds(500),
        .requestTimeout = std::chrono::milliseconds(500),
        .requestRetryInterval = std::chrono::milliseconds(250)
    };

    sockaddr_in clientAddr = {};
    clientAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(30000);

    sockaddr_in serverAddr = {};
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(40000);
    UDPSocket server(40000, 10, 1500);

    auto client = clientHost.ConnectPeer(serverAddr, timeout).lock();
    REQUIRE(client);
    REQUIRE(clientHost.Count() == 1);

    // Verify the syn packet
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto pkt = server.ReadFront();
        REQUIRE(pkt.has_value());
        auto udpPkt = reinterpret_cast<UDPPacket&>(pkt.value());
        UDPHeader header = udpPkt.Header();
        REQUIRE(header.flag & UDPHeaderFlag::SYN);
        REQUIRE(udpPkt.DataSize() == 0);
        // Currently no more packets should be in the queue
        REQUIRE(!server.ReadFront().has_value());

        UDPPacket reply;
        reply.address = clientAddr;
        UDPHeader replyHeader = {
            .index = 10,
            .sessionID = header.index,
            .flag = UDPHeaderFlag::SYN | UDPHeaderFlag::ACK,
        };
        reply.SetHeader(replyHeader);
        server.SendPacket(reinterpret_cast<Packet&>(reply));
    }
    // Verify client connection and ask for retransmission of ack
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(client->Connected());

        // Client should send back ACK
        auto clientReplyPkt = server.ReadBack();
        REQUIRE(clientReplyPkt.has_value());
        UDPPacket& clientReply = reinterpret_cast<UDPPacket&>(clientReplyPkt.value());
        UDPHeader clientReplyHeader = clientReply.Header();
        REQUIRE(clientReplyHeader.index == 0);
        REQUIRE(clientReplyHeader.sessionID == 10);
        REQUIRE(clientReplyHeader.flag == UDPHeaderFlag::ACK);

        // Send first reply for the case where client did not receive the first syn-ack packet
        UDPPacket reply;
        reply.address = clientAddr;
        UDPHeader replyHeader = {
            .index = 10,
            .sessionID = client->SessionID(),
            .flag = UDPHeaderFlag::SYN | UDPHeaderFlag::ACK,
        };
        reply.SetHeader(replyHeader);
        server.SendPacket(reinterpret_cast<Packet&>(reply));

        // Client should ignore since sessionID does not match
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(!server.ReadFront().has_value());

        // Send syc request
        UDPPacket reply2;
        reply2.address = clientAddr;
        UDPHeader replyHeader2 = {
            .sessionID = 10,
            .flag = UDPHeaderFlag::SYN
        };
        reply2.SetHeader(replyHeader2);
        server.SendPacket(reinterpret_cast<Packet&>(reply2));

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto pkt = server.ReadFront();
        REQUIRE(pkt.has_value());
        auto udpPkt = reinterpret_cast<UDPPacket&>(pkt.value());
        UDPHeader header = udpPkt.Header();
        REQUIRE(header == clientReplyHeader);
        // Currently no more packets should be in the queue
        REQUIRE(!server.ReadFront().has_value());
    }
}

TEST_CASE("UDPConnection drop overflowed connection requests", "[UDPConnection]") {
    UDPConnectionManager<2> host1(30000, 10, 1500, std::chrono::milliseconds(10));
    REQUIRE(host1.Good());

    UDPConnectionManager<1> host2(40000, 10, 1500, std::chrono::milliseconds(10));
    REQUIRE(host2.Good());
    host1.isListening = true;
    host2.isListening = true;

    TimeoutSetting timeout = {
        .connectionTimeout = std::chrono::milliseconds(1000),
        .connectionRetryInterval = std::chrono::milliseconds(500),
        .requestTimeout = std::chrono::milliseconds(500),
        .requestRetryInterval = std::chrono::milliseconds(250)
    };

    sockaddr_in addr1 = {};
    addr1.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr1.sin_family = AF_INET;
    addr1.sin_port = htons(30000);

    sockaddr_in addr2 = {};
    addr2.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr2.sin_family = AF_INET;
    addr2.sin_port = htons(40000);

    auto c1 = host1.ConnectPeer(addr2, timeout).lock();
    REQUIRE(c1);
    REQUIRE(host1.Count() == 1);

    auto s1 = host2.Accept(timeout).lock();
    REQUIRE(s1);
    REQUIRE(host2.Count() == 1);

    auto s2 = host2.ConnectPeer(addr1, timeout);
    REQUIRE(s2.expired());
    auto c2 = host1.ConnectPeer(addr2, timeout).lock();

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    REQUIRE(c1->Connected());
    REQUIRE(s1->Connected());
    REQUIRE(!c2->Connected());

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    REQUIRE(!c1->Closed());
    REQUIRE(!s1->Closed());
    REQUIRE(c2->Closed());
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

    TimeoutSetting timeout = {
        .connectionTimeout = std::chrono::milliseconds(1000),
        .connectionRetryInterval = std::chrono::milliseconds(500),
        .requestTimeout = std::chrono::milliseconds(500),
        .requestRetryInterval = std::chrono::milliseconds(250)
    };

    auto c1 = host1.ConnectPeer(addr2, timeout).lock();
    REQUIRE(c1);
    REQUIRE(host1.Count() == 1);

    auto s1 = host2.Accept(timeout).lock();
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

TEST_CASE("UDPConnection send regular data packets with respect to queue capacity constraint", "[UDPConnection]") {
    UDPConnectionManager<2> host1(30000, 2, 1500, std::chrono::milliseconds(10));
    REQUIRE(host1.Good());

    UDPConnectionManager<2> host2(40000, 1, 1500, std::chrono::milliseconds(10));
    REQUIRE(host2.Good());
    host1.isListening = true;
    host2.isListening = true;

    TimeoutSetting timeout = {
        .connectionTimeout = std::chrono::milliseconds(1000),
        .connectionRetryInterval = std::chrono::milliseconds(500),
        .requestTimeout = std::chrono::milliseconds(500),
        .requestRetryInterval = std::chrono::milliseconds(250)
    };

    sockaddr_in addr1 = {};
    addr1.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr1.sin_family = AF_INET;
    addr1.sin_port = htons(30000);

    sockaddr_in addr2 = {};
    addr2.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr2.sin_family = AF_INET;
    addr2.sin_port = htons(40000);

    auto c1 = host1.ConnectPeer(addr2, timeout).lock();
    REQUIRE(c1);
    REQUIRE(host1.Count() == 1);

    auto s1 = host2.Accept(timeout).lock();
    REQUIRE(s1);
    REQUIRE(host2.Count() == 1);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    REQUIRE(c1->Connected());
    REQUIRE(s1->Connected());

    // Client send 1 packet to server
    UDPPacket dgram1;
    UDPHeader dgram1Header = {
        .flag = UDPHeaderFlag::None
    };
    dgram1.SetHeader(dgram1Header);
    std::string msg1 = "Hello from client!\n";
    dgram1.SetData({msg1.data(), msg1.size()});
    c1->Send(dgram1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto item = s1->Receive();

    REQUIRE(item.has_value());
    auto recvDgram1 = item.value();
    auto span1 = recvDgram1.Data();
    std::string recvMsg1(span1.begin(), span1.end());
    REQUIRE(recvMsg1 == msg1);

    // Server send 1 packet to client
    UDPPacket dgram2;
    UDPHeader dgram2Header = {
        .flag = UDPHeaderFlag::None
    };
    dgram2.SetHeader(dgram1Header);
    std::string msg2 = "Hello from server!\n";
    dgram2.SetData({ msg2.data(), msg2.size() });
    s1->Send(dgram2);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    item = c1->Receive();

    REQUIRE(item.has_value());
    auto recvDgram2 = item.value();
    auto span2 = recvDgram2.Data();
    std::string recvMsg2(span2.begin(), span2.end());
    REQUIRE(recvMsg2 == msg2);

    // Client send 2 packets to server
    std::string msg3 = "Hello from client!\n";
    dgram1.SetData({ msg3.data(), msg3.size() });
    c1->Send(dgram1);

    std::string msg4 = "Overloaded!\n";
    dgram1.SetData({ msg4.data(), msg4.size() });
    c1->Send(dgram1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    item = s1->Receive();

    REQUIRE(item.has_value());
    auto recvDgram3 = item.value();
    auto span3 = recvDgram3.Data();
    std::string recvMsg3(span3.begin(), span3.end());
    REQUIRE(recvMsg3 == msg3);

    // The second packet should be dropped due to packet queue capacity limit
    REQUIRE(!s1->Receive().has_value());

    // Server sends 2 packet to client
    std::string msg5 = "Hello from server!\n";
    dgram2.SetData({ msg5.data(), msg5.size() });
    s1->Send(dgram2);

    std::string msg6 = "Hello again from server!\n";
    dgram2.SetData({ msg6.data(), msg6.size() });
    s1->Send(dgram2);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
   
    item = c1->Receive();
    REQUIRE(item.has_value());
    auto recvDgram5 = item.value();
    auto span5 = recvDgram5.Data();
    std::string recvMsg5(span5.begin(), span5.end());
    REQUIRE(recvMsg5 == msg5);

    item = c1->Receive();
    REQUIRE(item.has_value());
    auto recvDgram6 = item.value();
    auto span6 = recvDgram6.Data();
    std::string recvMsg6(span6.begin(), span6.end());
    REQUIRE(recvMsg6 == msg6);

    // No more packets should be left in the queue
    REQUIRE(!c1->Receive().has_value());

    c1->Disconnect();
    REQUIRE(c1->Closed());
    REQUIRE(host1.Count() == 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(s1->Closed());
    REQUIRE(host2.Count() == 0);
}

TEST_CASE("UDPConnection send request data packets", "[UDPConnection]") {
    UDPConnectionManager<2> host1(30000, 5, 1500, std::chrono::milliseconds(10));
    REQUIRE(host1.Good());

    UDPConnectionManager<2> host2(40000, 5, 1500, std::chrono::milliseconds(10));
    REQUIRE(host2.Good());
    host1.isListening = true;
    host2.isListening = true;

    TimeoutSetting timeout = {
        .connectionTimeout = std::chrono::milliseconds(1000),
        .connectionRetryInterval = std::chrono::milliseconds(500),
        .requestTimeout = std::chrono::milliseconds(500),
        .requestRetryInterval = std::chrono::milliseconds(250)
    };

    sockaddr_in addr1 = {};
    addr1.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr1.sin_family = AF_INET;
    addr1.sin_port = htons(30000);

    sockaddr_in addr2 = {};
    addr2.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr2.sin_family = AF_INET;
    addr2.sin_port = htons(40000);

    auto c1 = host1.ConnectPeer(addr2, timeout).lock();
    REQUIRE(c1);
    REQUIRE(host1.Count() == 1);

    auto s1 = host2.Accept(timeout).lock();
    REQUIRE(s1);
    REQUIRE(host2.Count() == 1);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    REQUIRE(c1->Connected());
    REQUIRE(s1->Connected());

    // Client send request packet synchronously
    UDPPacket dgram;
    
    {
        UDPHeader dgramHeader1 = {
        .flag = UDPHeaderFlag::REQ
        };
        dgram.SetHeader(dgramHeader1);
        std::string msg1 = "Hello from client!\n";
        dgram.SetData({ msg1.data(), msg1.size() });

        std::thread serverThread([&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            auto item = s1->Receive();
            REQUIRE(item.has_value());
            UDPPacket& pkt = item.value();
            UDPHeader h1 = pkt.Header();
            REQUIRE(h1.flag == UDPHeaderFlag::REQ);
            auto span1 = pkt.Data();
            REQUIRE(std::string(span1.begin(), span1.end()) == msg1);
            UDPPacket replyDgram;
            UDPHeader replyHeader = {
                .ackIndex = h1.index,
                .flag = UDPHeaderFlag::ACK
            };
            replyDgram.SetHeader(replyHeader);
            std::string replyMsg = "Hello back from server!";
            replyDgram.SetData({ replyMsg.data(), replyMsg.size() });
            s1->Send(replyDgram);
            });
        auto response = c1->Send(dgram);
        serverThread.join();
        REQUIRE(response.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
        auto resp = response.get();
        REQUIRE(resp.has_value());
        UDPPacket& pkt = resp.value();
        UDPHeader rh = pkt.Header();
        REQUIRE(rh.flag == UDPHeaderFlag::ACK);
        auto sp1 = pkt.Data();
        REQUIRE(std::string(sp1.begin(), sp1.end()) == std::string("Hello back from server!"));

        // The received response should not remain in the packet queue
        REQUIRE(!c1->Receive().has_value());
    }

    // Client send request packet asynchronously
    {
        UDPHeader dgramHeader2 = {
            .flag = UDPHeaderFlag::REQ | UDPHeaderFlag::ASY
        };
        dgram.SetHeader(dgramHeader2);
        std::string msg2("Hello again from client");
        dgram.SetData(std::span<char>{ msg2.data(), msg2.size() });
        auto asyncResult = c1->Send(dgram);
        REQUIRE(asyncResult.wait_for(std::chrono::seconds(0)) == std::future_status::timeout);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto item = s1->Receive();
        REQUIRE(item.has_value());
        auto pkt = item.value();
        auto receivedMsg = pkt.Data();
        REQUIRE(std::string(receivedMsg.begin(), receivedMsg.end()) == msg2);

        UDPHeader responseHeader = {
            .ackIndex = pkt.Header().index,
            .flag = UDPHeaderFlag::ACK
        };
        dgram.SetHeader(responseHeader);
        std::string responseString("Hello again from server");
        dgram.SetData(std::span<char>{responseString.data(), responseString.size()});
        REQUIRE(!s1->Send(dgram).valid());
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(asyncResult.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
        auto result = asyncResult.get();
        REQUIRE(result.has_value());
        auto str = result.value().Data();
        REQUIRE(std::string(str.begin(), str.end()) == responseString);
    }

    c1->Disconnect();
    REQUIRE(c1->Closed());
    REQUIRE(host1.Count() == 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(s1->Closed());
    REQUIRE(host2.Count() == 0);
}

TEST_CASE("UDPConnection send request data packets back and forth", "[UDPConnection]") {
    UDPConnectionManager<2> host1(30000, 2, 1500, std::chrono::milliseconds(10));
    REQUIRE(host1.Good());

    UDPConnectionManager<2> host2(40000, 1, 1500, std::chrono::milliseconds(10));
    REQUIRE(host2.Good());
    host1.isListening = true;
    host2.isListening = true;

    TimeoutSetting timeout = {
        .connectionTimeout = std::chrono::milliseconds(2000),
        .connectionRetryInterval = std::chrono::milliseconds(500),
        .requestTimeout = std::chrono::milliseconds(500),
        .requestRetryInterval = std::chrono::milliseconds(250)
    };

    sockaddr_in addr1 = {};
    addr1.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr1.sin_family = AF_INET;
    addr1.sin_port = htons(30000);

    sockaddr_in addr2 = {};
    addr2.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr2.sin_family = AF_INET;
    addr2.sin_port = htons(40000);

    auto c1 = host1.ConnectPeer(addr2, timeout).lock();
    REQUIRE(c1);
    REQUIRE(host1.Count() == 1);

    auto s1 = host2.Accept(timeout).lock();
    REQUIRE(s1);
    REQUIRE(host2.Count() == 1);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    REQUIRE(c1->Connected());
    REQUIRE(s1->Connected());

    UDPPacket dgram;

    {
        UDPHeader dgramHeader = {
            .flag = UDPHeaderFlag::REQ | UDPHeaderFlag::ASY
        };
        // Client sends message
        dgram.SetHeader(dgramHeader);
        std::string msg2("Hello from client");
        dgram.SetData(std::span<char>{ msg2.data(), msg2.size() });
        auto asyncResult = c1->Send(dgram);
        REQUIRE(asyncResult.wait_for(std::chrono::seconds(0)) == std::future_status::timeout);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Server receives and send response
        auto item = s1->Receive();
        REQUIRE(item.has_value());
        auto pkt = item.value();
        auto receivedMsg = pkt.Data();
        REQUIRE(std::string(receivedMsg.begin(), receivedMsg.end()) == msg2);

        UDPHeader responseHeader = {
            .ackIndex = pkt.Header().index,
            .flag = UDPHeaderFlag::ACK | UDPHeaderFlag::REQ | UDPHeaderFlag::ASY
        };
        dgram.SetHeader(responseHeader);
        std::string responseString("Hello from server");
        dgram.SetData(std::span<char>{responseString.data(), responseString.size()});
        
        auto clientResponse1 = s1->Send(dgram);
        REQUIRE(clientResponse1.valid());
        REQUIRE(clientResponse1.wait_for(std::chrono::seconds(0)) == std::future_status::timeout);

        // Client receives the server response and write back another message
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(asyncResult.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
        auto result = asyncResult.get();
        REQUIRE(result.has_value());
        auto serverReply = result.value();
        auto str = serverReply.Data();
        REQUIRE(std::string(str.begin(), str.end()) == responseString);

        UDPHeader clientResponseHeader = {
            .ackIndex = serverReply.Header().index,
            .flag = UDPHeaderFlag::ACK | UDPHeaderFlag::REQ | UDPHeaderFlag::ASY
        };
        dgram.SetHeader(clientResponseHeader);
        std::string clientReply("Fairwell from client");
        dgram.SetData(std::span<char>{clientReply.data(), clientReply.size()});
        auto timeoutAsync = c1->Send(dgram);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Server receives the last message from client
        REQUIRE(clientResponse1.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
        auto cr = clientResponse1.get();
        REQUIRE(cr.has_value());
        auto crVal = cr.value();
        auto crStr = crVal.Data();
        REQUIRE(std::string(crStr.begin(), crStr.end()) == clientReply);
        // No more packets should remain in the queue for now
        REQUIRE(!s1->Receive().has_value());

        // Timeout request for client
        std::this_thread::sleep_for(std::chrono::seconds(1));
        REQUIRE(timeoutAsync.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
        REQUIRE(!timeoutAsync.get().has_value());

        // Client will resend packet, but the packet queue should be empty since the server does not recognize any request that needs to be acknowledged
        REQUIRE(!s1->Receive().has_value());
    }

    c1->Disconnect();
    REQUIRE(c1->Closed());
    REQUIRE(host1.Count() == 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(s1->Closed());
    REQUIRE(host2.Count() == 0);
}

TEST_CASE("UDPConnection send IMP data packets with respect to queue capacity constraint", "[UDPConnection]") {
    UDPConnectionManager<2> host1(30000, 2, 1500, std::chrono::milliseconds(10));
    REQUIRE(host1.Good());

    UDPConnectionManager<2> host2(40000, 1, 1500, std::chrono::milliseconds(10));
    REQUIRE(host2.Good());
    host1.isListening = true;
    host2.isListening = true;

    TimeoutSetting timeout = {
        .connectionTimeout = std::chrono::milliseconds(1000),
        .connectionRetryInterval = std::chrono::milliseconds(500),
        .requestTimeout = std::chrono::milliseconds(500),
        .requestRetryInterval = std::chrono::milliseconds(250),
        .impRetryInterval = std::chrono::milliseconds(200)
    };

    sockaddr_in addr1 = {};
    addr1.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr1.sin_family = AF_INET;
    addr1.sin_port = htons(30000);

    sockaddr_in addr2 = {};
    addr2.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr2.sin_family = AF_INET;
    addr2.sin_port = htons(40000);

    auto c1 = host1.ConnectPeer(addr2, timeout).lock();
    REQUIRE(c1);
    REQUIRE(host1.Count() == 1);

    auto s1 = host2.Accept(timeout).lock();
    REQUIRE(s1);
    REQUIRE(host2.Count() == 1);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    REQUIRE(c1->Connected());
    REQUIRE(s1->Connected());

    // Client send 1 packet to server
    UDPPacket dgram1;
    UDPHeader dgram1Header = {
        .flag = UDPHeaderFlag::IMP
    };
    dgram1.SetHeader(dgram1Header);
    std::string msg1 = "Hello from client!\n";
    dgram1.SetData({ msg1.data(), msg1.size() });
    c1->Send(dgram1);
    REQUIRE(c1->NumImpMsg() == 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    REQUIRE(c1->NumImpMsg() == 0);
    auto item = s1->Receive();

    REQUIRE(item.has_value());
    auto recvDgram1 = item.value();
    auto span1 = recvDgram1.Data();
    std::string recvMsg1(span1.begin(), span1.end());
    REQUIRE(recvMsg1 == msg1);

    // Server send 1 packet to client
    UDPPacket dgram2;
    UDPHeader dgram2Header = {
        .flag = UDPHeaderFlag::IMP
    };
    dgram2.SetHeader(dgram1Header);
    std::string msg2 = "Hello from server!\n";
    dgram2.SetData({ msg2.data(), msg2.size() });
    s1->Send(dgram2);
    REQUIRE(s1->NumImpMsg() == 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(s1->NumImpMsg() == 0);
    item = c1->Receive();

    REQUIRE(item.has_value());
    auto recvDgram2 = item.value();
    auto span2 = recvDgram2.Data();
    std::string recvMsg2(span2.begin(), span2.end());
    REQUIRE(recvMsg2 == msg2);

    // Client send 2 packets to server
    std::string msg3 = "Hello from client!\n";
    dgram1.SetData({ msg3.data(), msg3.size() });
    c1->Send(dgram1);

    std::string msg4 = "Overloaded!\n";
    dgram1.SetData({ msg4.data(), msg4.size() });
    c1->Send(dgram1);

    REQUIRE(c1->NumImpMsg() == 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(c1->NumImpMsg() == 1);
    item = s1->Receive();
    // The second packet should be dropped due to packet queue capacity limit
    REQUIRE(!s1->Receive().has_value());
    REQUIRE(item.has_value());
    auto recvDgram3 = item.value();
    auto span3 = recvDgram3.Data();
    std::string recvMsg3(span3.begin(), span3.end());
    REQUIRE(recvMsg3 == msg3);

    // Wait for retry of second packet
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    item = s1->Receive();
    REQUIRE(item.has_value());
    auto recvDgram4 = item.value();
    auto span4 = recvDgram4.Data();
    std::string recvMsg4(span4.begin(), span4.end());
    REQUIRE(recvMsg4 == msg4);

    // Server sends 2 packet to client
    std::string msg5 = "Hello from server!\n";
    dgram2.SetData({ msg5.data(), msg5.size() });
    s1->Send(dgram2);

    std::string msg6 = "Hello again from server!\n";
    dgram2.SetData({ msg6.data(), msg6.size() });
    s1->Send(dgram2);
    REQUIRE(s1->NumImpMsg() == 2);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(s1->NumImpMsg() == 0);
    item = c1->Receive();
    REQUIRE(item.has_value());
    auto recvDgram5 = item.value();
    auto span5 = recvDgram5.Data();
    std::string recvMsg5(span5.begin(), span5.end());
    REQUIRE(recvMsg5 == msg5);

    item = c1->Receive();
    REQUIRE(item.has_value());
    auto recvDgram6 = item.value();
    auto span6 = recvDgram6.Data();
    std::string recvMsg6(span6.begin(), span6.end());
    REQUIRE(recvMsg6 == msg6);

    // No more packets should be left in the queue
    REQUIRE(!c1->Receive().has_value());

    c1->Disconnect();
    REQUIRE(c1->Closed());
    REQUIRE(host1.Count() == 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(s1->Closed());
    REQUIRE(host2.Count() == 0);
}