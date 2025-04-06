#include <catch2/catch_session.hpp>
#include "networking/networking.hpp"

void main(int argc, char* argv[]) {
    Networking::init();
    int result = Catch::Session().run(argc, argv);
    Networking::destroy();
}