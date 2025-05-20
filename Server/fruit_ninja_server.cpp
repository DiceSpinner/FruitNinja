#include <iostream>
#include "networking/networking.hpp"
#include "state/time.hpp"
#include "default_state.hpp"

int main() {
    Networking::init();

    Server server;
    std::chrono::steady_clock::duration tickInterval(std::chrono::milliseconds(10));
    std::chrono::steady_clock::time_point tickTime = std::chrono::steady_clock::now();
    while (server.running) {
        server.ProcessInput();
        server.Step();
        server.BroadCastState();
        
        tickTime += tickInterval;
        std::this_thread::sleep_until(tickTime);
    }
    
    Networking::destroy();
}