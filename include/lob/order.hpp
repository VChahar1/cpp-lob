#pragma once

#include <cstdint>
#include <chrono>

namespace lob {

// ---- Type aliases ----
//
// We use strong-typed aliases for clarity. In a production system these
// would be wrapped in `struct` types (e.g., `struct Price { uint64_t v; };`)
// to prevent accidentally passing a Quantity where a Price is expected.
// For this learning project, plain aliases are clearer and easier to read.

using OrderId   = std::uint64_t;
using Price     = std::uint64_t;   // integer ticks, NOT floating-point
using Quantity  = std::uint64_t;
using Timestamp = std::uint64_t;   // nanoseconds since Unix epoch

// ---- Side enum ----
//
// `enum class` (not plain `enum`) gives us scoping and type safety: you
// must write `Side::Bid`, not just `Bid`, and Side values don't implicitly
// convert to int.

enum class Side : std::uint8_t {
    Bid,
    Ask,
};

// Returns the opposite side. Useful in matching: a buy crosses against asks.
constexpr Side opposite(Side s) noexcept {
    return s == Side::Bid ? Side::Ask : Side::Bid;
}

// ---- Order struct ----
//
// A resting order in the book. All fields are public because Order is a
// passive data carrier, not an entity with invariants. (Compare to the
// OrderBook below, where invariants are enforced by methods.)

struct Order {
    OrderId   id;
    Side      side;
    Price     price;
    Quantity  quantity;
    Timestamp timestamp;

    // Default constructor produces a zero-initialized order.
    // Useful for tests and containers that need default values.
    Order() = default;

    // Convenience constructor that stamps the current time.
    static Order with_now(OrderId id, Side side, Price price, Quantity quantity);

    // Explicit-timestamp constructor used in tests for determinism.
    Order(OrderId id, Side side, Price price, Quantity quantity, Timestamp ts)
        : id(id), side(side), price(price), quantity(quantity), timestamp(ts) {}
};

}  // namespace lob