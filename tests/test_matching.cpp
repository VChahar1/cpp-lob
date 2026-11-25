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

TEST_CASE("Non-crossing order rests on the book", "[matching]") {
    OrderBook book;
    auto trades = submit_new_order(book, make_buy(1, 10000, 5));

    REQUIRE(trades.empty());
    REQUIRE(book.best_bid() == 10000);
    REQUIRE(book.depth_at(Side::Bid, 10000) == 5);
    REQUIRE(book.order_count() == 1);
}

TEST_CASE("Crossing order fully fills against a single resting order at same size", "[matching]") {
    OrderBook book;
    submit_new_order(book, make_sell(1, 10100, 5));
    auto trades = submit_new_order(book, make_buy(2, 10100, 5));

    REQUIRE(trades.size() == 1);
    REQUIRE(trades[0].buyer_id  == 2);
    REQUIRE(trades[0].seller_id == 1);
    REQUIRE(trades[0].price     == 10100);
    REQUIRE(trades[0].quantity  == 5);

    REQUIRE_FALSE(book.best_bid().has_value());
    REQUIRE_FALSE(book.best_ask().has_value());
    REQUIRE(book.order_count() == 0);
}

TEST_CASE("Crossing buy at higher-than-needed price executes at the resting (lower) price", "[matching]") {
    OrderBook book;
    submit_new_order(book, make_sell(1, 10100, 5));
    auto trades = submit_new_order(book, make_buy(2, 10200, 5));

    REQUIRE(trades.size() == 1);
    REQUIRE(trades[0].price == 10100);
}

TEST_CASE("Partial fill: taker exceeds maker", "[matching]") {
    OrderBook book;
    submit_new_order(book, make_sell(1, 10100, 5));
    auto trades = submit_new_order(book, make_buy(2, 10100, 8));

    REQUIRE(trades.size() == 1);
    REQUIRE(trades[0].quantity == 5);

    REQUIRE(book.best_bid() == 10100);
    REQUIRE(book.depth_at(Side::Bid, 10100) == 3);
    REQUIRE_FALSE(book.best_ask().has_value());
    REQUIRE(book.order_count() == 1);
}

TEST_CASE("Partial fill: maker exceeds taker", "[matching]") {
    OrderBook book;
    submit_new_order(book, make_sell(1, 10100, 8));
    auto trades = submit_new_order(book, make_buy(2, 10100, 5));

    REQUIRE(trades.size() == 1);
    REQUIRE(trades[0].quantity == 5);

    REQUIRE(book.best_ask() == 10100);
    REQUIRE(book.depth_at(Side::Ask, 10100) == 3);
    REQUIRE_FALSE(book.best_bid().has_value());
    REQUIRE(book.order_count() == 1);
}

TEST_CASE("Time priority within a price level: FIFO", "[matching]") {
    OrderBook book;
    submit_new_order(book, make_sell(1, 10100, 5));
    submit_new_order(book, make_sell(2, 10100, 5));
    submit_new_order(book, make_sell(3, 10100, 5));

    auto trades = submit_new_order(book, make_buy(99, 10100, 7));

    REQUIRE(trades.size() == 2);
    REQUIRE(trades[0].seller_id == 1);
    REQUIRE(trades[0].quantity  == 5);
    REQUIRE(trades[1].seller_id == 2);
    REQUIRE(trades[1].quantity  == 2);

    REQUIRE(book.depth_at(Side::Ask, 10100) == 8);
}

TEST_CASE("Price priority: best (lowest) ask matches first", "[matching]") {
    OrderBook book;
    submit_new_order(book, make_sell(1, 10120, 5));
    submit_new_order(book, make_sell(2, 10100, 5));
    submit_new_order(book, make_sell(3, 10110, 5));

    auto trades = submit_new_order(book, make_buy(99, 10125, 12));

    REQUIRE(trades.size() == 3);
    REQUIRE(trades[0].seller_id == 2);
    REQUIRE(trades[0].price     == 10100);
    REQUIRE(trades[0].quantity  == 5);
    REQUIRE(trades[1].seller_id == 3);
    REQUIRE(trades[1].price     == 10110);
    REQUIRE(trades[1].quantity  == 5);
    REQUIRE(trades[2].seller_id == 1);
    REQUIRE(trades[2].price     == 10120);
    REQUIRE(trades[2].quantity  == 2);

    REQUIRE(book.best_ask() == 10120);
    REQUIRE(book.depth_at(Side::Ask, 10120) == 3);
}

TEST_CASE("Sweeping the book: walk multiple levels then rest leftover", "[matching]") {
    OrderBook book;
    submit_new_order(book, make_sell(1, 10100, 10));
    submit_new_order(book, make_sell(2, 10110, 10));

    auto trades = submit_new_order(book, make_buy(99, 10115, 25));

    REQUIRE(trades.size() == 2);
    REQUIRE(trades[0].price == 10100);
    REQUIRE(trades[1].price == 10110);

    REQUIRE_FALSE(book.best_ask().has_value());
    REQUIRE(book.best_bid() == 10115);
    REQUIRE(book.depth_at(Side::Bid, 10115) == 5);
}

TEST_CASE("Sell aggressor matches against bids", "[matching]") {
    OrderBook book;
    submit_new_order(book, make_buy(1, 10000, 10));
    submit_new_order(book, make_buy(2,  9990, 10));

    auto trades = submit_new_order(book, make_sell(99, 9995, 15));

    REQUIRE(trades.size() == 1);
    REQUIRE(trades[0].buyer_id  == 1);
    REQUIRE(trades[0].seller_id == 99);
    REQUIRE(trades[0].price     == 10000);
    REQUIRE(trades[0].quantity  == 10);

    REQUIRE(book.best_bid() == 9990);
    REQUIRE(book.depth_at(Side::Bid, 9990) == 10);
    REQUIRE(book.best_ask() == 9995);
    REQUIRE(book.depth_at(Side::Ask, 9995) == 5);
    REQUIRE(book.order_count() == 2);
}


TEST_CASE("Submit correctly dispatches all three event types", "[matching]") {
    OrderBook book;

    auto trades1 = submit(book, OrderEvent{NewOrder{1, Side::Bid, 10000, 5}});
    REQUIRE(trades1.empty());
    REQUIRE(book.best_bid() == 10000);
    REQUIRE(book.order_count() == 1);

    auto trades2 = submit(book, OrderEvent{Modify{1, 10000, 3}});
    REQUIRE(trades2.empty());
    REQUIRE(book.depth_at(Side::Bid, 10000) == 3);

    auto trades3 = submit(book, OrderEvent{Cancel{1}});
    REQUIRE(trades3.empty());
    REQUIRE_FALSE(book.best_bid().has_value());
    REQUIRE(book.order_count() == 0);
}