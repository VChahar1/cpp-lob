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

// ---- Cancel ----

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

    // Cancel an id that was never submitted.
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

    // After cancel, the id is free to reuse.
    submit_new_order(book, make_buy(1, 10010, 7));
    REQUIRE(book.best_bid() == 10010);
    REQUIRE(book.depth_at(Side::Bid, 10010) == 7);
}

// ---- Modify: keep priority path ----

TEST_CASE("Modify with strictly smaller quantity at same price keeps priority", "[modify]") {
    OrderBook book;
    // Two orders at the same price; #1 is first in queue.
    submit_new_order(book, make_buy(1, 10000, 10));
    submit_new_order(book, make_buy(2, 10000, 10));

    // Shrink #1 to 7 shares.
    submit(book, OrderEvent{Modify{1, 10000, 7}});
    REQUIRE(book.depth_at(Side::Bid, 10000) == 17);

    // A sell of 8 should hit #1 first (now 7 shares) then #2 (1 share).
    auto trades = submit_new_order(book, make_sell(99, 10000, 8));
    REQUIRE(trades.size() == 2);
    REQUIRE(trades[0].buyer_id == 1);
    REQUIRE(trades[0].quantity == 7);
    REQUIRE(trades[1].buyer_id == 2);
    REQUIRE(trades[1].quantity == 1);
}

// ---- Modify: lose priority paths ----

TEST_CASE("Modify with price change loses priority", "[modify]") {
    OrderBook book;
    submit_new_order(book, make_buy(1, 10000, 10));
    submit_new_order(book, make_buy(2, 10000, 10));

    // Reprice #1 to 10005. It's now at a different (better) level alone.
    submit(book, OrderEvent{Modify{1, 10005, 10}});
    REQUIRE(book.best_bid() == 10005);
    REQUIRE(book.depth_at(Side::Bid, 10005) == 10);
    REQUIRE(book.depth_at(Side::Bid, 10000) == 10);  // #2 remains
}

TEST_CASE("Modify with quantity increase loses priority", "[modify]") {
    OrderBook book;
    submit_new_order(book, make_buy(1, 10000, 5));
    submit_new_order(book, make_buy(2, 10000, 5));

    // Increase #1 to 8. Loses priority -> goes to back of queue at 10000.
    submit(book, OrderEvent{Modify{1, 10000, 8}});

    // Now a sell of 6 should hit #2 first (5 shares), then #1 (1 share).
    auto trades = submit_new_order(book, make_sell(99, 10000, 6));
    REQUIRE(trades.size() == 2);
    REQUIRE(trades[0].buyer_id == 2);
    REQUIRE(trades[0].quantity == 5);
    REQUIRE(trades[1].buyer_id == 1);
    REQUIRE(trades[1].quantity == 1);
}

TEST_CASE("Modify with same price and same quantity loses priority", "[modify]") {
    // Per our policy, only STRICTLY decreasing quantity keeps priority.
    // Same quantity is not strictly less, so this loses priority.
    OrderBook book;
    submit_new_order(book, make_buy(1, 10000, 10));
    submit_new_order(book, make_buy(2, 10000, 10));

    submit(book, OrderEvent{Modify{1, 10000, 10}});

    auto trades = submit_new_order(book, make_sell(99, 10000, 5));
    REQUIRE(trades.size() == 1);
    REQUIRE(trades[0].buyer_id == 2);  // #2 is now first
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
    // The unknown id should NOT have been added as a new order.
    REQUIRE_FALSE(book.lookup_resting(999) != nullptr);
}

TEST_CASE("Modify that crosses the book produces trades", "[modify]") {
    OrderBook book;
    submit_new_order(book, make_sell(1, 10100, 10));
    submit_new_order(book, make_buy(2,  10000, 5));   // resting bid

    // Modify the bid up to 10100. Loses priority (price change) and now crosses.
    auto trades = submit(book, OrderEvent{Modify{2, 10100, 5}});
    REQUIRE(trades.size() == 1);
    REQUIRE(trades[0].buyer_id  == 2);
    REQUIRE(trades[0].seller_id == 1);
    REQUIRE(trades[0].price     == 10100);
    REQUIRE(trades[0].quantity  == 5);

    // 5 shares of #1 remain on the ask side.
    REQUIRE(book.best_ask() == 10100);
    REQUIRE(book.depth_at(Side::Ask, 10100) == 5);
}

// ---- Other edge cases ----

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

    // Same id, different price/quantity. Should be dropped.
    auto trades = submit_new_order(book, make_buy(1, 10010, 7));
    REQUIRE(trades.empty());
    REQUIRE(book.order_count() == 1);
    REQUIRE(book.best_bid() == 10000);  // original is intact
}

TEST_CASE("Long sequence of mixed events maintains consistency", "[stress][edge]") {
    OrderBook book;

    submit(book, OrderEvent{NewOrder{1, Side::Bid, 10000, 10}});
    submit(book, OrderEvent{NewOrder{2, Side::Bid,  9990, 20}});
    submit(book, OrderEvent{NewOrder{3, Side::Ask, 10100, 15}});
    submit(book, OrderEvent{NewOrder{4, Side::Ask, 10110, 25}});

    REQUIRE(book.order_count() == 4);
    REQUIRE(book.spread() == 100);  // 10100 - 10000

    // Cancel #2; reduce #3 in place.
    submit(book, OrderEvent{Cancel{2}});
    submit(book, OrderEvent{Modify{3, 10100, 10}});  // 15 -> 10, keep priority

    REQUIRE(book.order_count() == 3);
    REQUIRE(book.depth_at(Side::Ask, 10100) == 10);

    // Aggressor buy sweeps the ask side.
    auto trades = submit(book, OrderEvent{NewOrder{5, Side::Bid, 10115, 30}});
    REQUIRE(trades.size() == 2);
    REQUIRE(trades[0].seller_id == 3);
    REQUIRE(trades[0].quantity  == 10);
    REQUIRE(trades[1].seller_id == 4);
    REQUIRE(trades[1].quantity  == 20);

    // #4 has 5 left; #5 was fully filled.
    REQUIRE(book.depth_at(Side::Ask, 10110) == 5);
    REQUIRE(book.best_bid() == 10000);
}