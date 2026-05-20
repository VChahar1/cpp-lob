#include <lob/book.hpp>
#include <lob/order.hpp>
#include <catch.hpp>

using namespace lob;

TEST_CASE("New book is empty", "[book]") {
    OrderBook book;
    REQUIRE_FALSE(book.best_bid().has_value());
    REQUIRE_FALSE(book.best_ask().has_value());
    REQUIRE_FALSE(book.spread().has_value());
    REQUIRE(book.order_count() == 0);
}

TEST_CASE("Insert single bid", "[book]") {
    OrderBook book;
    book.insert_resting(Order(1, Side::Bid, 10000, 5, 1));
    REQUIRE(book.best_bid() == 10000);
    REQUIRE_FALSE(book.best_ask().has_value());
    REQUIRE(book.depth_at(Side::Bid, 10000) == 5);
    REQUIRE(book.order_count() == 1);
}

TEST_CASE("Insert single ask", "[book]") {
    OrderBook book;
    book.insert_resting(Order(1, Side::Ask, 10100, 3, 1));
    REQUIRE(book.best_ask() == 10100);
    REQUIRE(book.depth_at(Side::Ask, 10100) == 3);
}

TEST_CASE("Best bid is highest price", "[book]") {
    OrderBook book;
    book.insert_resting(Order(1, Side::Bid,  9900, 5, 1));
    book.insert_resting(Order(2, Side::Bid, 10000, 5, 2));
    book.insert_resting(Order(3, Side::Bid,  9950, 5, 3));
    REQUIRE(book.best_bid() == 10000);
}

TEST_CASE("Best ask is lowest price", "[book]") {
    OrderBook book;
    book.insert_resting(Order(1, Side::Ask, 10200, 5, 1));
    book.insert_resting(Order(2, Side::Ask, 10100, 5, 2));
    book.insert_resting(Order(3, Side::Ask, 10150, 5, 3));
    REQUIRE(book.best_ask() == 10100);
}

TEST_CASE("Spread is correct", "[book]") {
    OrderBook book;
    book.insert_resting(Order(1, Side::Bid, 10000, 5, 1));
    book.insert_resting(Order(2, Side::Ask, 10010, 5, 2));
    REQUIRE(book.spread() == 10);
}

TEST_CASE("Depth aggregates multiple orders at same price", "[book]") {
    OrderBook book;
    book.insert_resting(Order(1, Side::Bid, 10000, 5, 1));
    book.insert_resting(Order(2, Side::Bid, 10000, 3, 2));
    book.insert_resting(Order(3, Side::Bid, 10000, 7, 3));
    REQUIRE(book.depth_at(Side::Bid, 10000) == 15);
}

TEST_CASE("locations map uses order id as key", "[book][invariant]") {
    OrderBook book;
    book.insert_resting(Order(42, Side::Bid, 10000, 5, 1));
    book.insert_resting(Order(99, Side::Ask, 10100, 3, 2));

    const auto loc42 = book.location_of(42);
    const auto loc99 = book.location_of(99);
    REQUIRE(loc42.has_value());
    REQUIRE(loc99.has_value());
    REQUIRE(loc42->first  == Side::Bid);
    REQUIRE(loc42->second == 10000);
    REQUIRE(loc99->first  == Side::Ask);
    REQUIRE(loc99->second == 10100);

    REQUIRE_FALSE(book.location_of(0).has_value());
    REQUIRE_FALSE(book.location_of(1).has_value());
}

TEST_CASE("order_count tracks insertions", "[book]") {
    OrderBook book;
    REQUIRE(book.order_count() == 0);
    book.insert_resting(Order(1, Side::Bid, 10000, 5, 1));
    REQUIRE(book.order_count() == 1);
    book.insert_resting(Order(2, Side::Ask, 10100, 3, 2));
    REQUIRE(book.order_count() == 2);
    book.insert_resting(Order(3, Side::Bid,  9900, 7, 3));
    REQUIRE(book.order_count() == 3);
}