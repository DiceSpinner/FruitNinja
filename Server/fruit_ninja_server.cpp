#include <iostream>
#include "networking/networking.hpp"
#include "state/time.hpp"
#include "default_state.hpp"

int main() {
    Networking::init();

    Server server(
        TimeoutSetting{
            .connectionTimeout = std::chrono::seconds(5),
            .connectionRetryInterval = std::chrono::milliseconds(300),
            .requestTimeout = std::chrono::seconds(3),
            .requestRetryInterval = std::chrono::milliseconds(300),
            .impRetryInterval = std::chrono::milliseconds(200)
        }
     );
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