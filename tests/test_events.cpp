#include <lob/events.hpp>
#include <catch.hpp>

using namespace lob;

TEST_CASE("NewOrder event holds expected fields", "[events]") {
    OrderEvent e = NewOrder{1, Side::Bid, 10000, 5};
    REQUIRE(std::holds_alternative<NewOrder>(e));

    const auto& n = std::get<NewOrder>(e);
    REQUIRE(n.id == 1);
    REQUIRE(n.side == Side::Bid);
    REQUIRE(n.price == 10000);
    REQUIRE(n.quantity == 5);
}

TEST_CASE("Cancel event holds the id", "[events]") {
    OrderEvent e = Cancel{42};
    REQUIRE(std::holds_alternative<Cancel>(e));
    REQUIRE(std::get<Cancel>(e).id == 42);
}

TEST_CASE("Modify event holds new fields", "[events]") {
    OrderEvent e = Modify{7, 12345, 100};
    REQUIRE(std::holds_alternative<Modify>(e));
    const auto& m = std::get<Modify>(e);
    REQUIRE(m.id == 7);
    REQUIRE(m.new_price == 12345);
    REQUIRE(m.new_quantity == 100);
}

TEST_CASE("Trade equality compares all fields", "[events]") {
    Trade t1{1, 2, 100, 5, 1000};
    Trade t2{1, 2, 100, 5, 1000};
    Trade t3{1, 2, 100, 5, 1001};  // different timestamp
    REQUIRE(t1 == t2);
    REQUIRE_FALSE(t1 == t3);
}