#include <lob/matching.hpp>
#include <lob/book.hpp>
#include <lob/order.hpp>
#include <lob/events.hpp>

#include <catch.hpp>

using namespace lob;

// Small helpers to keep tests readable.
namespace {

NewOrder make_buy(OrderId id, Price p, Quantity q)  { return NewOrder{id, Side::Bid, p, q}; }
NewOrder make_sell(OrderId id, Price p, Quantity q) { return NewOrder{id, Side::Ask, p, q}; }

}  // namespace

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
    // Rest a sell first.
    submit_new_order(book, make_sell(1, 10100, 5));
    // Now a buy that exactly matches it.
    auto trades = submit_new_order(book, make_buy(2, 10100, 5));

    REQUIRE(trades.size() == 1);
    REQUIRE(trades[0].buyer_id  == 2);
    REQUIRE(trades[0].seller_id == 1);
    REQUIRE(trades[0].price     == 10100);
    REQUIRE(trades[0].quantity  == 5);

    // Book is now empty.
    REQUIRE_FALSE(book.best_bid().has_value());
    REQUIRE_FALSE(book.best_ask().has_value());
    REQUIRE(book.order_count() == 0);
}

TEST_CASE("Crossing buy at higher-than-needed price executes at the resting (lower) price", "[matching]") {
    OrderBook book;
    submit_new_order(book, make_sell(1, 10100, 5));
    auto trades = submit_new_order(book, make_buy(2, 10200, 5));

    REQUIRE(trades.size() == 1);
    // Execution price is the resting (maker) price, NOT the aggressor's 10200.
    REQUIRE(trades[0].price == 10100);
}

TEST_CASE("Partial fill: taker exceeds maker", "[matching]") {
    OrderBook book;
    submit_new_order(book, make_sell(1, 10100, 5));
    auto trades = submit_new_order(book, make_buy(2, 10100, 8));

    REQUIRE(trades.size() == 1);
    REQUIRE(trades[0].quantity == 5);

    // Taker had 8; matched 5; rests 3 on the bid side.
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

    // Maker had 8; 5 filled; 3 still rest as an ask.
    REQUIRE(book.best_ask() == 10100);
    REQUIRE(book.depth_at(Side::Ask, 10100) == 3);
    REQUIRE_FALSE(book.best_bid().has_value());
    REQUIRE(book.order_count() == 1);
}

TEST_CASE("Time priority within a price level: FIFO", "[matching]") {
    OrderBook book;
    // Three asks at the same price, arriving in order 1, 2, 3.
    submit_new_order(book, make_sell(1, 10100, 5));
    submit_new_order(book, make_sell(2, 10100, 5));
    submit_new_order(book, make_sell(3, 10100, 5));

    // A buy comes in for 7 shares. Should fill all of #1 (5) and 2 of #2.
    auto trades = submit_new_order(book, make_buy(99, 10100, 7));

    REQUIRE(trades.size() == 2);
    REQUIRE(trades[0].seller_id == 1);
    REQUIRE(trades[0].quantity  == 5);
    REQUIRE(trades[1].seller_id == 2);
    REQUIRE(trades[1].quantity  == 2);

    // #2 has 3 remaining; #3 has 5 remaining; depth at 10100 is 8.
    REQUIRE(book.depth_at(Side::Ask, 10100) == 8);
}

TEST_CASE("Price priority: best (lowest) ask matches first", "[matching]") {
    OrderBook book;
    submit_new_order(book, make_sell(1, 10120, 5));
    submit_new_order(book, make_sell(2, 10100, 5));  // better price
    submit_new_order(book, make_sell(3, 10110, 5));

    // A buy willing to pay 10125 for 12 shares. Should fill 5 @ 10100, 5 @ 10110, 2 @ 10120.
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

    // 3 left at 10120.
    REQUIRE(book.best_ask() == 10120);
    REQUIRE(book.depth_at(Side::Ask, 10120) == 3);
}

TEST_CASE("Sweeping the book: walk multiple levels then rest leftover", "[matching]") {
    OrderBook book;
    submit_new_order(book, make_sell(1, 10100, 10));
    submit_new_order(book, make_sell(2, 10110, 10));

    // Buy 25 shares willing to pay 10115. Fills 10@10100, 10@10110, then rests 5 @ 10115.
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

    // Seller willing to accept >= 9995 for 15 shares.
    // Crosses the 10000 bid (fills 10), does NOT cross the 9990 bid.
    // 5 shares rest as an ask at 9995.
    auto trades = submit_new_order(book, make_sell(99, 9995, 15));

    REQUIRE(trades.size() == 1);
    REQUIRE(trades[0].buyer_id  == 1);
    REQUIRE(trades[0].seller_id == 99);
    REQUIRE(trades[0].price     == 10000);   // resting (maker) price wins
    REQUIRE(trades[0].quantity  == 10);

    // 9990 bid is untouched; 5-share ask now rests at 9995.
    REQUIRE(book.best_bid() == 9990);
    REQUIRE(book.depth_at(Side::Bid, 9990) == 10);
    REQUIRE(book.best_ask() == 9995);
    REQUIRE(book.depth_at(Side::Ask, 9995) == 5);
    REQUIRE(book.order_count() == 2);
}


TEST_CASE("Submit correctly dispatches all three event types", "[matching]") {
    OrderBook book;

    // NewOrder: adds to the book.
    auto trades1 = submit(book, OrderEvent{NewOrder{1, Side::Bid, 10000, 5}});
    REQUIRE(trades1.empty());
    REQUIRE(book.best_bid() == 10000);
    REQUIRE(book.order_count() == 1);

    // Modify: change quantity at the same price (keeps priority).
    auto trades2 = submit(book, OrderEvent{Modify{1, 10000, 3}});
    REQUIRE(trades2.empty());
    REQUIRE(book.depth_at(Side::Bid, 10000) == 3);

    // Cancel: removes the order from the book.
    auto trades3 = submit(book, OrderEvent{Cancel{1}});
    REQUIRE(trades3.empty());
    REQUIRE_FALSE(book.best_bid().has_value());
    REQUIRE(book.order_count() == 0);
}