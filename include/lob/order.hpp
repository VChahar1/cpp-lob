#pragma once

#include <cstdint>
#include <chrono>

namespace lob {


using OrderId   = std::uint64_t;
using Price     = std::uint64_t;
using Quantity  = std::uint64_t;
using Timestamp = std::uint64_t;


enum class Side : std::uint8_t {
    Bid,
    Ask,
};

constexpr Side opposite(Side s) noexcept {
    return s == Side::Bid ? Side::Ask : Side::Bid;
}


struct Order {
    OrderId   id;
    Side      side;
    Price     price;
    Quantity  quantity;
    Timestamp timestamp;

    Order() = default;

    static Order with_now(OrderId id, Side side, Price price, Quantity quantity);

    Order(OrderId id, Side side, Price price, Quantity quantity, Timestamp ts)
        : id(id), side(side), price(price), quantity(quantity), timestamp(ts) {}
};

}