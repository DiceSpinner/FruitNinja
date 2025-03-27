#include <iostream>
#include <thread>
#include <functional>
#include <string>
#include <cassert>
#include "networking/networking.hpp"
#include "networking/socket.hpp"
#include "state/time.hpp"
#include "infrastructure/coroutine.hpp"

static addrinfo* serverAddr;
static std::atomic<bool> running = true;
static std::atomic<bool> waitingConnection = false;

static Coroutine logStatus(const sockaddr_in& server) {
    UDPSocket client(5);

    while (running) {
        co_yield Coroutine::YieldOption::Wait(1);
        std::string msg = "Time - " + std::to_string(Time::time()) + "\n";
        std::cout << "[Client] Sending " << msg;
        UDPPacket packet = {
            .payload = {msg.c_str(), msg.c_str() + msg.length() + 1}
        };
        client.SendPacket(packet, server);
    }
}

static void runTasks(const sockaddr_in& server) {
    CoroutineManager taskManager = {};
    taskManager.AddCoroutine(logStatus(server));
    while (!taskManager.Empty()) {
        Time::updateTime();
        taskManager.Run();
    }
}

int main() {
    Networking::init();
    
    UDPSocket server(5);

    std::thread task(runTasks, server.Address());
    while (true) {
        auto packet = server.ReadPacket();
        if (packet) std::cout << "[Server] Received: " << packet.value().payload.data();
    }
    
    task.join();
    Networking::destroy();
}