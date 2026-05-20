#include <lob/book.hpp>
#include <lob/order.hpp>

#include <cassert>
#include <chrono>
#include <numeric>

namespace lob {

// ---- Order::with_now (kept here for now) ----

Order Order::with_now(OrderId id, Side side, Price price, Quantity quantity) {
    const auto now = std::chrono::system_clock::now();
    const auto ts = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count();
    return Order(id, side, price, quantity, static_cast<Timestamp>(ts));
}

// ---- PriceLevel ----

Quantity PriceLevel::total_quantity() const noexcept {
    Quantity total = 0;
    for (const auto& o : orders) {
        total += o.quantity;
    }
    return total;
}

// ---- OrderBook ----

std::optional<Price> OrderBook::best_bid() const {
    if (bids_.empty()) return std::nullopt;
    // std::map is sorted ascending; best bid is the largest key.
    return bids_.rbegin()->first;
}

std::optional<Price> OrderBook::best_ask() const {
    if (asks_.empty()) return std::nullopt;
    // Best ask is the smallest key.
    return asks_.begin()->first;
}

std::optional<Price> OrderBook::spread() const {
    const auto bid = best_bid();
    const auto ask = best_ask();
    if (!bid || !ask) return std::nullopt;
    if (*ask < *bid) return std::nullopt;  // defensive: crossed book
    return *ask - *bid;
}

Quantity OrderBook::depth_at(Side side, Price price) const {
    const auto& book = (side == Side::Bid) ? bids_ : asks_;
    const auto it = book.find(price);
    if (it == book.end()) return 0;
    return it->second.total_quantity();
}

void OrderBook::insert_resting(const Order& order) {
    // Precondition check. In release builds, assert() is a no-op; the
    // matching engine must enforce this invariant before calling here.
    assert(locations_.find(order.id) == locations_.end()
           && "duplicate order id in insert_resting");

    auto& book = (order.side == Side::Bid) ? bids_ : asks_;

    // operator[] inserts a default-constructed PriceLevel if the key
    // isn't already there. Then we push_back the order.
    book[order.price].orders.push_back(order);

    // Index the location for O(1) cancel lookup.
    locations_.emplace(order.id, std::make_pair(order.side, order.price));
}

std::optional<std::pair<Side, Price>> OrderBook::location_of(OrderId id) const {
    const auto it = locations_.find(id);
    if (it == locations_.end()) return std::nullopt;
    return it->second;
}

}  // namespace lob