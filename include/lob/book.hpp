#pragma once

#include <lob/order.hpp>

#include <cstddef>
#include <deque>
#include <map>
#include <optional>
#include <unordered_map>
#include <utility>

namespace lob {

// ---- PriceLevel ----
//
// All resting orders at a single price, in FIFO time order.
// Thin wrapper around std::deque<Order>.
//
// Why std::deque? Because price levels need:
//   - O(1) push_back: newly arrived orders join the back
//   - O(1) pop_front: oldest orders match first during matching
//
// std::vector gives O(1) push_back but O(n) pop_front (shifts all elements).
// std::deque uses a segmented array internally and supports both ends in O(1).

class PriceLevel {
public:
    std::deque<Order> orders;

    // Total resting quantity at this level.
    Quantity total_quantity() const noexcept;

    // Number of distinct resting orders at this level.
    std::size_t count() const noexcept { return orders.size(); }

    bool empty() const noexcept { return orders.empty(); }
};

// ---- OrderBook ----
//
// Design:
//   - bids and asks are each std::map<Price, PriceLevel>.
//     std::map is sorted by key, giving O(log n) insert/lookup and fast
//     access to both ends (best_bid = max bid = largest key,
//     best_ask = min ask = smallest key).
//   - Each PriceLevel holds orders in FIFO time order (std::deque).
//   - `locations` is an O(1) auxiliary index: order id -> (side, price).
//     Lets cancel find the right price level immediately.
//
// Invariant: every id in `locations` corresponds to exactly one order
// resting in `bids` or `asks` at the recorded (side, price).

class OrderBook {
public:
    OrderBook() = default;

    // ---- Read-only inspection ----

    // Best (highest) bid price, if any.
    std::optional<Price> best_bid() const;

    // Best (lowest) ask price, if any.
    std::optional<Price> best_ask() const;

    // Spread = best_ask - best_bid. None if either side is empty
    // or the book is crossed (defensive guard).
    std::optional<Price> spread() const;

    // Total resting quantity at a given (side, price).
    Quantity depth_at(Side side, Price price) const;

    // Number of resting orders in the book (both sides).
    std::size_t order_count() const noexcept { return locations_.size(); }

    // ---- Internal helpers ----
    //
    // These are exposed for tests and for the matching engine (Day 2).
    // They are NOT the public submit() API -- that comes on Day 2 and
    // performs matching before resting any leftover quantity.

    // Insert an order into the book WITHOUT matching.
    // Precondition: no order with this id is currently resting.
    // Violating the precondition triggers an assertion failure in debug builds.
    void insert_resting(const Order& order);

    // Test-only accessor for the locations map.
    // Returns the (side, price) of a resting order, if it exists.
    std::optional<std::pair<Side, Price>> location_of(OrderId id) const;

private:
    std::map<Price, PriceLevel> bids_;
    std::map<Price, PriceLevel> asks_;
    std::unordered_map<OrderId, std::pair<Side, Price>> locations_;
};

}  // namespace lob