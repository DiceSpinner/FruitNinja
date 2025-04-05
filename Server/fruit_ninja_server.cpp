#include <iostream>
#include "networking/networking.hpp"
#include "state/time.hpp"
#include "default_state.hpp"

int main() {
    Networking::init();

    Server server;
    server.SetInitialState<DefaultState>();

    while (true) {
        server.ProcessInput();
        server.Step();
        server.BroadCastState();
    }
    
    Networking::destroy();
}