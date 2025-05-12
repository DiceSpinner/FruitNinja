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
        .requestTimeout = std::chrono::milliseconds(500),
        .timeoutRetries = 2
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
        .requestTimeout = std::chrono::milliseconds(500),
        .timeoutRetries = 2
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
        .requestTimeout = std::chrono::milliseconds(500),
        .timeoutRetries = 2
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
    c1->Send(dgram2);

    std::string msg6 = "Hello again from server!\n";
    dgram2.SetData({ msg6.data(), msg6.size() });
    s1->Send(dgram2);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
   
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
    UDPConnectionManager<2> host1(30000, 2, 1500, std::chrono::milliseconds(10));
    REQUIRE(host1.Good());

    UDPConnectionManager<2> host2(40000, 1, 1500, std::chrono::milliseconds(10));
    REQUIRE(host2.Good());
    host1.isListening = true;
    host2.isListening = true;

    TimeoutSetting timeout = {
        .connectionTimeout = std::chrono::milliseconds(1000),
        .requestTimeout = std::chrono::milliseconds(500),
        .timeoutRetries = 1
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

    // Client send 1 request packet to server and wait
    UDPPacket dgram;
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
    REQUIRE(response.has_value());
    
    UDPPacket& pkt = response.value();
    UDPHeader rh = pkt.Header();
    REQUIRE(rh.flag == UDPHeaderFlag::ACK);
    auto sp1 = pkt.Data();
    REQUIRE(std::string(sp1.begin(), sp1.end()) == std::string("Hello back from server!"));

    // The received response should not remain in the packet queue
    REQUIRE(!c1->Receive().has_value());

    // Client send request packet asynchronously
    // TODO


    c1->Disconnect();
    REQUIRE(c1->Closed());
    REQUIRE(host1.Count() == 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(s1->Closed());
    REQUIRE(host2.Count() == 0);
}