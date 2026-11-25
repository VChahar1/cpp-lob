#include <lob/matching.hpp>
#include <lob/book.hpp>
#include <lob/order.hpp>
#include <lob/events.hpp>

#include <catch.hpp>

using namespace lob;

namespace {
NewOrder make_buy(OrderId id, Price p, Quantity q)  { return NewOrder{id, Side::Bid, p, q}; }
NewOrder make_sell(OrderId id, Price p, Quantity q) { return NewOrder{id, Side::Ask, p, q}; }
}


TEST_CASE("Cancel removes a resting order", "[cancel]") {
    OrderBook book;
    submit_new_order(book, make_buy(1, 10000, 5));
    REQUIRE(book.order_count() == 1);

    submit(book, OrderEvent{Cancel{1}});
    REQUIRE(book.order_count() == 0);
    REQUIRE_FALSE(book.best_bid().has_value());
}

TEST_CASE("Cancel of unknown id is a no-op", "[cancel]") {
    OrderBook book;
    submit_new_order(book, make_buy(1, 10000, 5));
    REQUIRE(book.order_count() == 1);

    submit(book, OrderEvent{Cancel{999}});
    REQUIRE(book.order_count() == 1);
    REQUIRE(book.best_bid() == 10000);
}

TEST_CASE("Cancel preserves other orders at the same level", "[cancel]") {
    OrderBook book;
    submit_new_order(book, make_buy(1, 10000, 5));
    submit_new_order(book, make_buy(2, 10000, 3));
    submit_new_order(book, make_buy(3, 10000, 7));
    REQUIRE(book.depth_at(Side::Bid, 10000) == 15);

    submit(book, OrderEvent{Cancel{2}});
    REQUIRE(book.depth_at(Side::Bid, 10000) == 12);
    REQUIRE(book.order_count() == 2);
}

TEST_CASE("Cancel of last order at a level removes the level", "[cancel]") {
    OrderBook book;
    submit_new_order(book, make_buy(1, 10000, 5));
    submit_new_order(book, make_buy(2,  9900, 3));
    REQUIRE(book.best_bid() == 10000);

    submit(book, OrderEvent{Cancel{1}});
    REQUIRE(book.best_bid() == 9900);
}

TEST_CASE("Cancel then re-add same id is allowed", "[cancel]") {
    OrderBook book;
    submit_new_order(book, make_buy(1, 10000, 5));
    submit(book, OrderEvent{Cancel{1}});

    submit_new_order(book, make_buy(1, 10010, 7));
    REQUIRE(book.best_bid() == 10010);
    REQUIRE(book.depth_at(Side::Bid, 10010) == 7);
}


TEST_CASE("Modify with strictly smaller quantity at same price keeps priority", "[modify]") {
    OrderBook book;
    submit_new_order(book, make_buy(1, 10000, 10));
    submit_new_order(book, make_buy(2, 10000, 10));

    submit(book, OrderEvent{Modify{1, 10000, 7}});
    REQUIRE(book.depth_at(Side::Bid, 10000) == 17);

    auto trades = submit_new_order(book, make_sell(99, 10000, 8));
    REQUIRE(trades.size() == 2);
    REQUIRE(trades[0].buyer_id == 1);
    REQUIRE(trades[0].quantity == 7);
    REQUIRE(trades[1].buyer_id == 2);
    REQUIRE(trades[1].quantity == 1);
}


TEST_CASE("Modify with price change loses priority", "[modify]") {
    OrderBook book;
    submit_new_order(book, make_buy(1, 10000, 10));
    submit_new_order(book, make_buy(2, 10000, 10));

    submit(book, OrderEvent{Modify{1, 10005, 10}});
    REQUIRE(book.best_bid() == 10005);
    REQUIRE(book.depth_at(Side::Bid, 10005) == 10);
    REQUIRE(book.depth_at(Side::Bid, 10000) == 10);
}

TEST_CASE("Modify with quantity increase loses priority", "[modify]") {
    OrderBook book;
    submit_new_order(book, make_buy(1, 10000, 5));
    submit_new_order(book, make_buy(2, 10000, 5));

    submit(book, OrderEvent{Modify{1, 10000, 8}});

    auto trades = submit_new_order(book, make_sell(99, 10000, 6));
    REQUIRE(trades.size() == 2);
    REQUIRE(trades[0].buyer_id == 2);
    REQUIRE(trades[0].quantity == 5);
    REQUIRE(trades[1].buyer_id == 1);
    REQUIRE(trades[1].quantity == 1);
}

TEST_CASE("Modify with same price and same quantity loses priority", "[modify]") {
    OrderBook book;
    submit_new_order(book, make_buy(1, 10000, 10));
    submit_new_order(book, make_buy(2, 10000, 10));

    submit(book, OrderEvent{Modify{1, 10000, 10}});

    auto trades = submit_new_order(book, make_sell(99, 10000, 5));
    REQUIRE(trades.size() == 1);
    REQUIRE(trades[0].buyer_id == 2);
}

TEST_CASE("Modify with zero quantity cancels the order", "[modify]") {
    OrderBook book;
    submit_new_order(book, make_buy(1, 10000, 10));
    REQUIRE(book.order_count() == 1);

    submit(book, OrderEvent{Modify{1, 10000, 0}});
    REQUIRE(book.order_count() == 0);
    REQUIRE_FALSE(book.best_bid().has_value());
}

TEST_CASE("Modify of unknown id is a no-op", "[modify]") {
    OrderBook book;
    submit_new_order(book, make_buy(1, 10000, 5));

    auto trades = submit(book, OrderEvent{Modify{999, 10100, 3}});
    REQUIRE(trades.empty());
    REQUIRE(book.order_count() == 1);
    REQUIRE(book.best_bid() == 10000);
    REQUIRE_FALSE(book.lookup_resting(999) != nullptr);
}

TEST_CASE("Modify that crosses the book produces trades", "[modify]") {
    OrderBook book;
    submit_new_order(book, make_sell(1, 10100, 10));
    submit_new_order(book, make_buy(2,  10000, 5));

    auto trades = submit(book, OrderEvent{Modify{2, 10100, 5}});
    REQUIRE(trades.size() == 1);
    REQUIRE(trades[0].buyer_id  == 2);
    REQUIRE(trades[0].seller_id == 1);
    REQUIRE(trades[0].price     == 10100);
    REQUIRE(trades[0].quantity  == 5);

    REQUIRE(book.best_ask() == 10100);
    REQUIRE(book.depth_at(Side::Ask, 10100) == 5);
}


TEST_CASE("Zero-quantity new order is dropped silently", "[edge]") {
    OrderBook book;
    auto trades = submit_new_order(book, make_buy(1, 10000, 0));
    REQUIRE(trades.empty());
    REQUIRE(book.order_count() == 0);
}

TEST_CASE("Duplicate id on new order is dropped silently", "[edge]") {
    OrderBook book;
    submit_new_order(book, make_buy(1, 10000, 5));
    REQUIRE(book.order_count() == 1);

    auto trades = submit_new_order(book, make_buy(1, 10010, 7));
    REQUIRE(trades.empty());
    REQUIRE(book.order_count() == 1);
    REQUIRE(book.best_bid() == 10000);
}

TEST_CASE("Long sequence of mixed events maintains consistency", "[stress][edge]") {
    OrderBook book;

    submit(book, OrderEvent{NewOrder{1, Side::Bid, 10000, 10}});
    submit(book, OrderEvent{NewOrder{2, Side::Bid,  9990, 20}});
    submit(book, OrderEvent{NewOrder{3, Side::Ask, 10100, 15}});
    submit(book, OrderEvent{NewOrder{4, Side::Ask, 10110, 25}});

    REQUIRE(book.order_count() == 4);
    REQUIRE(book.spread() == 100);

    submit(book, OrderEvent{Cancel{2}});
    submit(book, OrderEvent{Modify{3, 10100, 10}});

    REQUIRE(book.order_count() == 3);
    REQUIRE(book.depth_at(Side::Ask, 10100) == 10);

    auto trades = submit(book, OrderEvent{NewOrder{5, Side::Bid, 10115, 30}});
    REQUIRE(trades.size() == 2);
    REQUIRE(trades[0].seller_id == 3);
    REQUIRE(trades[0].quantity  == 10);
    REQUIRE(trades[1].seller_id == 4);
    REQUIRE(trades[1].quantity  == 20);

    REQUIRE(book.depth_at(Side::Ask, 10110) == 5);
    REQUIRE(book.best_bid() == 10000);
}