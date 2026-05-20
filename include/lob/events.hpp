#pragma once

#include <lob/order.hpp>

#include <variant>

namespace lob {

// ---- OrderEvent ----
//
// An incoming event the book must process. Represented as a `std::variant`,
// which is C++'s tagged union (equivalent to Rust's enum-with-data).
// Three event types:
//
//   NewOrder  -- submit a fresh limit order, try to match, rest the remainder
//   Cancel    -- remove a resting order by id (no-op if id unknown)
//   Modify    -- change price and/or quantity of a resting order
//                * loses time priority if price changes or quantity increases
//                * keeps time priority if quantity strictly decreases at same price

struct NewOrder {
    OrderId  id;
    Side     side;
    Price    price;
    Quantity quantity;
};

struct Cancel {
    OrderId id;
};

struct Modify {
    OrderId  id;
    Price    new_price;
    Quantity new_quantity;
};

using OrderEvent = std::variant<NewOrder, Cancel, Modify>;

// ---- Trade ----
//
// Emitted by the matching engine when two orders cross. The price is the
// resting order's price (standard exchange convention), not the aggressor's.

struct Trade {
    OrderId   buyer_id;
    OrderId   seller_id;
    Price     price;
    Quantity  quantity;
    Timestamp timestamp;
};

// Equality for trades, useful in tests.
inline bool operator==(const Trade& a, const Trade& b) noexcept {
    return a.buyer_id == b.buyer_id
        && a.seller_id == b.seller_id
        && a.price == b.price
        && a.quantity == b.quantity
        && a.timestamp == b.timestamp;
}

}  // namespace lob