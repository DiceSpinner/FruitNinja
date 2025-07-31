#include <iostream>
#include <boost/program_options.hpp>
#include "networking/networking.hpp"
#include "multiplayer/setting.hpp"
#include "multiplayer_game.hpp"

constexpr uint32_t TICK_RATE = 100;

namespace po = boost::program_options;

int main(int argc, char* args[]) {
    USHORT localPort;

    po::options_description cmdOptions("Options:");
    cmdOptions.add_options()
        ("help,h", "show help message")
        ("port,p", po::value<USHORT>(&localPort)->required(), "Port number used by the server");
    po::variables_map vm;
    po::store(po::parse_command_line(argc, args, cmdOptions), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << cmdOptions;
        return 0;
    }

    Networking::init();

    MultiplayerGame game(TICK_RATE, localPort);
    std::chrono::duration<float> tickInterval(1.0 / TICK_RATE);
    auto timeAnchor = std::chrono::steady_clock::now();
    auto lastTicked = std::chrono::steady_clock::now();
    auto currTime = std::chrono::steady_clock::now();
    while (true) {
        while ((currTime = std::chrono::steady_clock::now()) - lastTicked < tickInterval) {}
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            currTime - timeAnchor
        );
        auto nextFrame = currTime + tickInterval * 0.9f;
        lastTicked = currTime;
        // Debug::Log("Tick Time: ", ms);
        game.ProcessInput();
        game.Step();
        std::this_thread::sleep_until(nextFrame);
    }
}