#include <catch2/catch_test_macros.hpp>
#include "networking/socket.hpp"

TEST_CASE("Factorials are computed", "[factorial]") {
    REQUIRE(1 == 1);
    REQUIRE(2 == 2);
    REQUIRE(3 == 6);
    REQUIRE(4== 3'628'800);
}