#include <lob/order.hpp>
#include <catch.hpp>

using namespace lob;

TEST_CASE("Side opposite is involutive", "[order]") {
    REQUIRE(opposite(Side::Bid) == Side::Ask);
    REQUIRE(opposite(Side::Ask) == Side::Bid);
    REQUIRE(opposite(opposite(Side::Bid)) == Side::Bid);
}

TEST_CASE("Order explicit-timestamp constructor sets fields", "[order]") {
    Order o(42, Side::Bid, 10000, 5, 1'000'000);
    REQUIRE(o.id == 42);
    REQUIRE(o.side == Side::Bid);
    REQUIRE(o.price == 10000);
    REQUIRE(o.quantity == 5);
    REQUIRE(o.timestamp == 1'000'000);
}

TEST_CASE("Order::with_now assigns a recent timestamp", "[order]") {
    const auto before = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    const auto o = Order::with_now(1, Side::Ask, 10000, 5);

    const auto after = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    REQUIRE(o.timestamp >= static_cast<Timestamp>(before));
    REQUIRE(o.timestamp <= static_cast<Timestamp>(after));
}