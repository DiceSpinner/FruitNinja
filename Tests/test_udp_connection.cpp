#include <catch2/catch_test_macros.hpp>
#include "networking/lite_conn.hpp"

TEST_CASE("UDPConnection 3-way handshake succeeds and closure", "[UDPConnection]") {
    LiteConnManager host1(30000, 2, 10, 1500, std::chrono::milliseconds(10));
    REQUIRE(host1.Good());

    LiteConnManager host2(40000, 2, 10, 1500, std::chrono::milliseconds(10));
    REQUIRE(host2.Good());
    host1.isListening = true;
    host2.isListening = true;

    TimeoutSetting timeout = {
        .connectionTimeout = std::chrono::milliseconds(1000),
        .connectionRetryInterval = std::chrono::milliseconds(500),
        .impRetryInterval = std::chrono::milliseconds(250),
        .replyKeepDuration = std::chrono::seconds(1)
    };

    sockaddr_in addr1 = {};
    addr1.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr1.sin_family = AF_INET;
    addr1.sin_port = htons(30000);

    sockaddr_in addr2 = {};
    addr2.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr2.sin_family = AF_INET;
    addr2.sin_port = htons(40000);

    auto c1 = host1.ConnectPeer(addr2, timeout);
    REQUIRE(c1);
    REQUIRE(host1.Count() == 1);

    auto s1 = host2.Accept(timeout);
    REQUIRE(s1);
    REQUIRE(host2.Count() == 1);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    REQUIRE(c1->IsConnected());
    REQUIRE(s1->IsConnected());

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    REQUIRE(!c1->IsDisconnected());
    REQUIRE(!s1->IsDisconnected());
    REQUIRE(host1.Count() == 1);
    REQUIRE(host2.Count() == 1);

    c1->Disconnect();
    REQUIRE(c1->IsDisconnected());
    REQUIRE(host1.Count() == 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(s1->IsDisconnected());
    REQUIRE(host2.Count() == 0);
}

TEST_CASE("UDPConnection 3-way handshake client retry connection", "[UDPConnection]") {
    LiteConnManager host1(30000, 2, 10, 1500, std::chrono::milliseconds(10));
    REQUIRE(host1.Good());

    TimeoutSetting timeout = {
        .connectionTimeout = std::chrono::milliseconds(500),
        .connectionRetryInterval = std::chrono::milliseconds(300),
        .impRetryInterval = std::chrono::milliseconds(250),
        .replyKeepDuration = std::chrono::seconds(1)
    };

    sockaddr_in addr1 = {};
    addr1.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr1.sin_family = AF_INET;
    addr1.sin_port = htons(30000);

    sockaddr_in addr2 = {};
    addr2.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr2.sin_family = AF_INET;
    addr2.sin_port = htons(40000);
    UDPSocket server(40000, 1500);

    auto client = host1.ConnectPeer(addr2, timeout);
    REQUIRE(client);
    REQUIRE(host1.Count() == 1);

    // Verify the first syn packet
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto pkt = server.Read();
        REQUIRE(pkt.has_value());
        auto optHeader = LiteConnHeader::Deserialize(pkt.value().payload);
        REQUIRE(optHeader.has_value());
        LiteConnHeader& header = optHeader.value();
        REQUIRE(header.flag & LiteConnHeaderFlag::SYN);
        REQUIRE(!(header.flag & LiteConnHeaderFlag::DATA));
        // Currently no more packets should be in the queue
        REQUIRE(!server.Read().has_value());
    }
    // Verify the retry syn packet
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        auto pkt = server.Read();
        REQUIRE(pkt.has_value());
        auto optHeader = LiteConnHeader::Deserialize(pkt.value().payload);
        REQUIRE(optHeader.has_value());
        LiteConnHeader& header = optHeader.value();
        REQUIRE(header.flag & LiteConnHeaderFlag::SYN);
        REQUIRE(!(header.flag & LiteConnHeaderFlag::DATA));
        // Currently no more packets should be in the queue
        REQUIRE(!server.Read().has_value());
    }

    // Verify the connection has timed out
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        REQUIRE(client->IsDisconnected());
        REQUIRE(host1.Count() == 0);
    }
}

TEST_CASE("UDPConnection 3-way handshake server retry connection", "[UDPConnection]") {
    LiteConnManager serverHost(30000, 2, 10, 1500, std::chrono::milliseconds(10));
    REQUIRE(serverHost.Good());
    serverHost.isListening = true;

    TimeoutSetting timeout = {
        .connectionTimeout = std::chrono::milliseconds(500),
        .connectionRetryInterval = std::chrono::milliseconds(300),
        .impRetryInterval = std::chrono::milliseconds(250)
    };

    sockaddr_in serverAddr = {};
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(30000);

    sockaddr_in clientAddr = {};
    clientAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(40000);
    UDPSocket client (40000, 1500);

    // Client initialize connection
    {
        LiteConnHeader header = {
            .sessionID = 0,
            .flag = LiteConnHeaderFlag::SYN,
            .id32 = 5,
        };
        
        client.SendPacket(LiteConnHeader::Serialize(header), serverAddr);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto server = serverHost.Accept(timeout);
    REQUIRE(server);
    REQUIRE(serverHost.Count() == 1);
    
    // Verify server ack
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto pkt = client.Read();
        REQUIRE(pkt.has_value());
        auto optHeader = LiteConnHeader::Deserialize(pkt.value().payload);
        REQUIRE(optHeader.has_value());
        LiteConnHeader& header = optHeader.value();
        REQUIRE(header.flag == (LiteConnHeaderFlag::SYN | LiteConnHeaderFlag::ACK));
        REQUIRE(header.sessionID == 5);

        // Currently no more packets should be present in the queue
        REQUIRE(!client.Read().has_value());

        // Verify server retransmission, 2 packets should be received
        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        // Case 1 client ACK is lost so server request for retransmission
        pkt = client.Read();
        REQUIRE(pkt.has_value());
        auto optHeader2 = LiteConnHeader::Deserialize(pkt.value().payload);
        REQUIRE(optHeader2.has_value());
        LiteConnHeader& header2 = optHeader2.value();
        REQUIRE(header2.flag == LiteConnHeaderFlag::SYN);
        REQUIRE(header2.sessionID == header.id32);

        // Case 2 server ACK is lost so server retransmits sessionID
        pkt = client.Read();
        REQUIRE(pkt.has_value());
        auto optHeader3 = LiteConnHeader::Deserialize(pkt.value().payload);
        REQUIRE(optHeader3.has_value());
        LiteConnHeader& header3 = optHeader3.value();
        REQUIRE(header3 == header);
    }

    // Verify the connection has timed out
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        REQUIRE(server->IsDisconnected());
        REQUIRE(serverHost.Count() == 0);
    }
}

TEST_CASE("UDPConnection 3-way handshake client retransmit acknoledgement", "[UDPConnection]") {
    LiteConnManager clientHost(30000, 2, 10, 1500, std::chrono::milliseconds(10));
    REQUIRE(clientHost.Good());

    TimeoutSetting timeout = {
        .connectionTimeout = std::chrono::milliseconds(1000),
        .connectionRetryInterval = std::chrono::milliseconds(500),
        .impRetryInterval = std::chrono::milliseconds(250),
        .replyKeepDuration = std::chrono::seconds(1)
    };

    sockaddr_in clientAddr = {};
    clientAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(30000);

    sockaddr_in serverAddr = {};
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(40000);
    UDPSocket server(40000, 1500);

    auto client = clientHost.ConnectPeer(serverAddr, timeout);
    REQUIRE(client);
    REQUIRE(clientHost.Count() == 1);

    // Verify the syn packet
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto pkt = server.Read();
        REQUIRE(pkt.has_value());
        auto optHeader = LiteConnHeader::Deserialize(pkt.value().payload);
        REQUIRE(optHeader.has_value());
        LiteConnHeader& header = optHeader.value();
        REQUIRE(header.flag == LiteConnHeaderFlag::SYN);
        // Currently no more packets should be in the queue
        REQUIRE(!server.Read().has_value());

        LiteConnHeader replyHeader = {
            .sessionID = header.id32,
            .flag = LiteConnHeaderFlag::SYN | LiteConnHeaderFlag::ACK,
            .id32 = 10,
        };
        server.SendPacket(LiteConnHeader::Serialize(replyHeader), clientAddr);
    }
    // Verify client connection and ask for retransmission of ack
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(client->IsConnected());

        // Client should send back ACK
        auto clientReplyPkt = server.Read();
        REQUIRE(clientReplyPkt.has_value());
        auto optClientHeader = LiteConnHeader::Deserialize(clientReplyPkt.value().payload);
        REQUIRE(optClientHeader.has_value());
        LiteConnHeader& clientReplyHeader = optClientHeader.value();
        REQUIRE(clientReplyHeader.sessionID == 10);
        REQUIRE(clientReplyHeader.flag == LiteConnHeaderFlag::ACK);

        // Send first reply for the case where client did not receive the first syn-ack packet
        LiteConnHeader replyHeader = {
            .sessionID = client->SessionID(),
            .flag = LiteConnHeaderFlag::SYN | LiteConnHeaderFlag::ACK,
            .id32 = 10,
        };
        server.SendPacket(LiteConnHeader::Serialize(replyHeader), clientAddr);

        // Client should ignore since sessionID does not match
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(!server.Read().has_value());

        // Send syc request
        LiteConnHeader replyHeader2 = {
            .sessionID = 10,
            .flag = LiteConnHeaderFlag::SYN
        };
        server.SendPacket(LiteConnHeader::Serialize(replyHeader2), clientAddr);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto pkt = server.Read();
        REQUIRE(pkt.has_value());
        auto optHeader = LiteConnHeader::Deserialize(pkt.value().payload);
        REQUIRE(optHeader.has_value());
        LiteConnHeader& header = optHeader.value();
        REQUIRE(header == clientReplyHeader);
        // Currently no more packets should be in the queue
        REQUIRE(!server.Read().has_value());
    }
}

TEST_CASE("UDPConnection drop overflowed connection requests", "[UDPConnection]") {
    LiteConnManager host1(30000, 2, 10, 1500, std::chrono::milliseconds(10));
    REQUIRE(host1.Good());

    LiteConnManager host2(40000, 1, 10, 1500, std::chrono::milliseconds(10));
    REQUIRE(host2.Good());
    host1.isListening = true;
    host2.isListening = true;

    TimeoutSetting timeout = {
        .connectionTimeout = std::chrono::milliseconds(1000),
        .connectionRetryInterval = std::chrono::milliseconds(500),
        .impRetryInterval = std::chrono::milliseconds(250),
        .replyKeepDuration = std::chrono::seconds(1)
    };

    sockaddr_in addr1 = {};
    addr1.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr1.sin_family = AF_INET;
    addr1.sin_port = htons(30000);

    sockaddr_in addr2 = {};
    addr2.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr2.sin_family = AF_INET;
    addr2.sin_port = htons(40000);

    auto c1 = host1.ConnectPeer(addr2, timeout);
    REQUIRE(c1);
    REQUIRE(host1.Count() == 1);

    auto s1 = host2.Accept(timeout);
    REQUIRE(s1);
    REQUIRE(host2.Count() == 1);

    auto s2 = host2.ConnectPeer(addr1, timeout);
    REQUIRE(!s2);
    auto c2 = host1.ConnectPeer(addr2, timeout);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    REQUIRE(c1->IsConnected());
    REQUIRE(s1->IsConnected());
    REQUIRE(!c2->IsConnected());

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    REQUIRE(!c1->IsDisconnected());
    REQUIRE(!s1->IsDisconnected());
    REQUIRE(c2->IsDisconnected());
    REQUIRE(host1.Count() == 1);
    REQUIRE(host2.Count() == 1);

    c1->Disconnect();
    REQUIRE(c1->IsDisconnected());
    REQUIRE(host1.Count() == 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(s1->IsDisconnected());
    REQUIRE(host2.Count() == 0);
}

TEST_CASE("UDPConnection timeout after one side forcefully disconnect", "[UDPConnection]") {
    LiteConnManager host1(30000, 2, 10, 1500, std::chrono::milliseconds(10));
    REQUIRE(host1.Good());

    LiteConnManager host2(40000, 2, 10, 1500, std::chrono::milliseconds(10));
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
        .impRetryInterval = std::chrono::milliseconds(250),
        .replyKeepDuration = std::chrono::seconds(1)
    };

    auto c1 = host1.ConnectPeer(addr2, timeout);
    REQUIRE(c1);
    REQUIRE(host1.Count() == 1);

    auto s1 = host2.Accept(timeout);
    REQUIRE(s1);
    REQUIRE(host2.Count() == 1);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    REQUIRE(c1->IsConnected());
    REQUIRE(s1->IsConnected());

    c1->SimulateDisconnect();
    c1->Disconnect();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(c1->IsDisconnected());
    REQUIRE(host1.Count() == 0);
    REQUIRE(s1->IsConnected());
    REQUIRE(host2.Count() == 1);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    REQUIRE(s1->IsDisconnected());
    REQUIRE(host2.Count() == 0);
}

TEST_CASE("UDPConnection send regular data packets with respect to queue capacity constraint", "[UDPConnection]") {
    LiteConnManager host1(30000, 2, 2, 1500, std::chrono::milliseconds(10));
    REQUIRE(host1.Good());

    LiteConnManager host2(40000, 2, 1, 1500, std::chrono::milliseconds(10));
    REQUIRE(host2.Good());
    host1.isListening = true;
    host2.isListening = true;

    TimeoutSetting timeout = {
        .connectionTimeout = std::chrono::milliseconds(1000),
        .connectionRetryInterval = std::chrono::milliseconds(500),
        .impRetryInterval = std::chrono::milliseconds(250),
        .replyKeepDuration = std::chrono::seconds(1)
    };

    sockaddr_in addr1 = {};
    addr1.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr1.sin_family = AF_INET;
    addr1.sin_port = htons(30000);

    sockaddr_in addr2 = {};
    addr2.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr2.sin_family = AF_INET;
    addr2.sin_port = htons(40000);

    auto c1 = host1.ConnectPeer(addr2, timeout);
    REQUIRE(c1);
    REQUIRE(host1.Count() == 1);

    auto s1 = host2.Accept(timeout);
    REQUIRE(s1);
    REQUIRE(host2.Count() == 1);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    REQUIRE(c1->IsConnected());
    REQUIRE(s1->IsConnected());

    // Client send 1 packet to server
    std::string msg1 = "Hello from client!\n";
    c1->SendData(msg1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto item = s1->Receive();

    REQUIRE(item.has_value());
    auto& recvDgram1 = item.value();
    auto& span1 = recvDgram1.data;
    std::string recvMsg1(span1.begin(), span1.end());
    REQUIRE(recvMsg1 == msg1);

    // Server send 1 packet to client
    std::string msg2 = "Hello from server!\n";
    s1->SendData(msg2);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    item = c1->Receive();

    REQUIRE(item.has_value());
    auto& recvDgram2 = item.value();
    auto& span2 = recvDgram2.data;
    std::string recvMsg2(span2.begin(), span2.end());
    REQUIRE(recvMsg2 == msg2);

    // Client send 2 packets to server
    std::string msg3 = "Hello from client!\n";
    c1->SendData(msg3);

    std::string msg4 = "Overloaded!\n";
    c1->SendData(msg4);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    item = s1->Receive();
    REQUIRE(item.has_value());
    auto& recvDgram3 = item.value();
    auto& span3 = recvDgram3.data;
    std::string recvMsg3(span3.begin(), span3.end());
    REQUIRE(recvMsg3 == msg3);

    // The second packet should be dropped due to packet queue capacity limit
    REQUIRE(!s1->Receive().has_value());

    // Server sends 2 packet to client
    std::string msg5 = "Hello from server!\n";
    s1->SendData(msg5);

    std::string msg6 = "Hello again from server!\n";
    s1->SendData(msg6);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
   
    item = c1->Receive();
    REQUIRE(item.has_value());
    auto& recvDgram5 = item.value();
    auto& span5 = recvDgram5.data;
    std::string recvMsg5(span5.begin(), span5.end());
    REQUIRE(recvMsg5 == msg5);

    item = c1->Receive();
    REQUIRE(item.has_value());
    auto& recvDgram6 = item.value();
    auto& span6 = recvDgram6.data;
    std::string recvMsg6(span6.begin(), span6.end());
    REQUIRE(recvMsg6 == msg6);

    // No more packets should be left in the queue
    REQUIRE(!c1->Receive().has_value());

    c1->Disconnect();
    REQUIRE(c1->IsDisconnected());
    REQUIRE(host1.Count() == 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(s1->IsDisconnected());
    REQUIRE(host2.Count() == 0);
}

TEST_CASE("UDPConnection send request data packets", "[UDPConnection]") {
    LiteConnManager host1(30000, 2, 5, 1500, std::chrono::milliseconds(10));
    REQUIRE(host1.Good());

    LiteConnManager host2(40000, 2, 5, 1500, std::chrono::milliseconds(10));
    REQUIRE(host2.Good());
    host1.isListening = true;
    host2.isListening = true;

    TimeoutSetting timeout = {
        .connectionTimeout = std::chrono::milliseconds(1000),
        .connectionRetryInterval = std::chrono::milliseconds(500),
        .impRetryInterval = std::chrono::milliseconds(250),
        .replyKeepDuration = std::chrono::seconds(1)
    };

    sockaddr_in addr1 = {};
    addr1.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr1.sin_family = AF_INET;
    addr1.sin_port = htons(30000);

    sockaddr_in addr2 = {};
    addr2.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr2.sin_family = AF_INET;
    addr2.sin_port = htons(40000);

    auto c1 = host1.ConnectPeer(addr2, timeout);
    REQUIRE(c1);
    REQUIRE(host1.Count() == 1);

    auto s1 = host2.Accept(timeout);
    REQUIRE(s1);
    REQUIRE(host2.Count() == 1);

    REQUIRE(c1->WaitForConnectionComplete(std::chrono::milliseconds(200)));
    REQUIRE(s1->WaitForConnectionComplete(std::chrono::milliseconds(200)));
    REQUIRE(c1->IsConnected());
    REQUIRE(s1->IsConnected());

    std::string cm1 = "C: Do you love me?";
    std::string sm1 = "S: Yes, what about you?";
    std::string cm2 = "C: No! I do not love you";
    std::string sm2 = "S: NOOOO! Please love me!!!!";

    std::thread cThread([&]() {
            // Send first message
            auto crHandle1Opt = c1->SendRequest(cm1);
            REQUIRE(crHandle1Opt.has_value());

            // Verify the response
            auto& crHandle1 = crHandle1Opt.value();
            REQUIRE(crHandle1.WaitForResponse(std::chrono::milliseconds(1000)));
            auto reply1Opt = crHandle1.GetResponse();
            REQUIRE(crHandle1.WaitForResponse(std::chrono::milliseconds()));
            REQUIRE(!crHandle1.GetResponse().has_value());
            REQUIRE(reply1Opt.has_value());
            auto& reply1 = reply1Opt.value();
            std::string replyStr1 = { reply1.data.begin(), reply1.data.end()};
            REQUIRE(sm1 == replyStr1);
            REQUIRE(reply1.requestHandle.has_value());

            // Send second request
            auto& crpHandle1 = reply1.requestHandle.value();
            REQUIRE(crpHandle1.IsValid());
            auto crHandle2Opt = crpHandle1.Converse(cm2);
            REQUIRE(crHandle2Opt.has_value());
            REQUIRE(!crpHandle1.IsValid());

            // Verify response
            auto& crHandle2 = crHandle2Opt.value();
            REQUIRE(crHandle2.WaitForResponse(std::chrono::milliseconds(1000)));
            auto reply2Opt = crHandle2.GetResponse();
            REQUIRE(reply2Opt.has_value());
            REQUIRE(crHandle2.WaitForResponse(std::chrono::milliseconds()));
            REQUIRE(!crHandle2.GetResponse().has_value());
            auto& reply2 = reply2Opt.value();
            std::string replyStr2 = { reply2.data.begin(), reply2.data.end() };
            REQUIRE(sm2 == replyStr2);
            REQUIRE(reply2.requestHandle.has_value());

            // Close request
            auto& crpHandle2 = reply2.requestHandle.value();
            REQUIRE(crpHandle2.IsValid());
            crpHandle2.Reject();
            REQUIRE(!crpHandle2.IsValid());
        });
    std::thread sThread([&]() {
        // Listen for first request
        REQUIRE(s1->WaitForDataPacket(std::chrono::milliseconds(1000)));
        auto inquiry = s1->Receive();
        REQUIRE(inquiry.has_value());
        auto& msg1 = inquiry.value();
        std::string inquiryStr = {msg1.data.begin(), msg1.data.end()};
        REQUIRE(inquiryStr == cm1);

        // Send reply
        REQUIRE(msg1.requestHandle.has_value());
        auto& srqHandle1 = msg1.requestHandle.value();
        REQUIRE(srqHandle1.IsValid());
        auto sr1HandleOpt = srqHandle1.Converse(sm1);
        REQUIRE(!srqHandle1.IsValid());
        REQUIRE(sr1HandleOpt.has_value());
        
        // Wait for second request
        auto& sr1Handle = sr1HandleOpt.value();
        REQUIRE(sr1Handle.WaitForResponse(std::chrono::milliseconds(1000)));
        auto msg2Opt = sr1Handle.GetResponse();
        REQUIRE(msg2Opt.has_value());
        REQUIRE(!sr1Handle.GetResponse().has_value());
        auto& msg2 = msg2Opt.value();
        std::string msgStr2 = { msg2.data.begin(), msg2.data.end() };
        REQUIRE(msgStr2 == cm2);
        
        // Sent second reply
        REQUIRE(msg2.requestHandle.has_value());
        auto& srqHandle2 = msg2.requestHandle.value();
        REQUIRE(srqHandle2.IsValid());
        auto sr2HandleOpt = srqHandle2.Converse(sm2);
        REQUIRE(!srqHandle2.IsValid());
        REQUIRE(sr2HandleOpt.has_value());
        
        // Verify the request is closed
        auto& sr2Handle = sr2HandleOpt.value();
        // Verify the handle is not closed immediately, but after peer has sent back rejection
        REQUIRE(!sr2Handle.WaitForResponse(std::chrono::seconds()));
        REQUIRE(sr2Handle.WaitForResponse(std::chrono::milliseconds(1000)));
        REQUIRE(!sr2Handle.GetResponse().has_value());
    });
    cThread.join();
    sThread.join();

    c1->Disconnect();
    REQUIRE(c1->IsDisconnected());
    REQUIRE(host1.Count() == 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(s1->IsDisconnected());
    REQUIRE(host2.Count() == 0);
}

TEST_CASE("UDPConnection send IMP data packets with respect to queue capacity constraint", "[UDPConnection]") {
    LiteConnManager host1(30000, 2, 2, 1500, std::chrono::milliseconds(10));
    REQUIRE(host1.Good());

    LiteConnManager host2(40000, 2, 1, 1500, std::chrono::milliseconds(10));
    REQUIRE(host2.Good());
    host1.isListening = true;
    host2.isListening = true;

    TimeoutSetting timeout = {
        .connectionTimeout = std::chrono::milliseconds(1000),
        .connectionRetryInterval = std::chrono::milliseconds(500),
        .impRetryInterval = std::chrono::milliseconds(200),
        .replyKeepDuration = std::chrono::seconds(1)
    };

    sockaddr_in addr1 = {};
    addr1.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr1.sin_family = AF_INET;
    addr1.sin_port = htons(30000);

    sockaddr_in addr2 = {};
    addr2.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr2.sin_family = AF_INET;
    addr2.sin_port = htons(40000);

    auto c1 = host1.ConnectPeer(addr2, timeout);
    REQUIRE(c1);
    REQUIRE(host1.Count() == 1);

    auto s1 = host2.Accept(timeout);
    REQUIRE(s1);
    REQUIRE(host2.Count() == 1);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    REQUIRE(c1->IsConnected());
    REQUIRE(s1->IsConnected());

    const std::string msg1 = "Hello from client!\n";
    const std::string msg2 = "Hello from server!\n";
    const std::string msg3 = "Hello from client!\n";
    const std::string msg4 = "Overloaded!\n";
    const std::string msg5 = "Hello from server!\n";
    const std::string msg6 = "Hello again from server!\n";

    // Client send 1 packet to server
    c1->SendReliableData(msg1);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    auto item = s1->Receive();

    REQUIRE(item.has_value());
    auto& recvDgram1 = item.value();
    auto& span1 = recvDgram1.data;
    std::string recvMsg1(span1.begin(), span1.end());
    REQUIRE(recvMsg1 == msg1);

    // Server send 1 packet to client
    s1->SendReliableData(msg2);
    REQUIRE(s1->NumImpMsg() == 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(s1->NumImpMsg() == 0);
    item = c1->Receive();

    REQUIRE(item.has_value());
    auto& recvDgram2 = item.value();
    auto& span2 = recvDgram2.data;
    std::string recvMsg2(span2.begin(), span2.end());
    REQUIRE(recvMsg2 == msg2);

    // Client send 2 packets to server
    
    c1->SendReliableData(msg3);
    c1->SendReliableData(msg4);

    REQUIRE(c1->NumImpMsg() == 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(c1->NumImpMsg() == 1);
    item = s1->Receive();
    // The second packet should be dropped due to packet queue capacity limit
    REQUIRE(!s1->Receive().has_value());
    REQUIRE(item.has_value());
    auto& recvDgram3 = item.value();
    auto& span3 = recvDgram3.data;
    std::string recvMsg3(span3.begin(), span3.end());
    REQUIRE(recvMsg3 == msg3);

    // Wait for retry of second packet
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    item = s1->Receive();
    REQUIRE(item.has_value());
    auto& recvDgram4 = item.value();
    auto& span4 = recvDgram4.data;
    std::string recvMsg4(span4.begin(), span4.end());
    REQUIRE(recvMsg4 == msg4);

    // Server sends 2 packet to client
    s1->SendReliableData(msg5);
    s1->SendReliableData(msg6);
    REQUIRE(s1->NumImpMsg() == 2);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(s1->NumImpMsg() == 0);
    item = c1->Receive();
    REQUIRE(item.has_value());
    auto& recvDgram5 = item.value();
    auto& span5 = recvDgram5.data;
    std::string recvMsg5(span5.begin(), span5.end());
    REQUIRE(recvMsg5 == msg5);

    item = c1->Receive();
    REQUIRE(item.has_value());
    auto& recvDgram6 = item.value();
    auto& span6 = recvDgram6.data;
    std::string recvMsg6(span6.begin(), span6.end());
    REQUIRE(recvMsg6 == msg6);

    // No more packets should be left in the queue
    REQUIRE(!c1->Receive().has_value());

    c1->Disconnect();
    REQUIRE(c1->IsDisconnected());
    REQUIRE(host1.Count() == 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(s1->IsDisconnected());
    REQUIRE(host2.Count() == 0);
}