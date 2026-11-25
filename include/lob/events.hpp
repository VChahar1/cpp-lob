#pragma once

#include <lob/order.hpp>

#include <variant>

namespace lob {


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


struct Trade {
    OrderId   buyer_id;
    OrderId   seller_id;
    Price     price;
    Quantity  quantity;
    Timestamp timestamp;
};

inline bool operator==(const Trade& a, const Trade& b) noexcept {
    return a.buyer_id == b.buyer_id
        && a.seller_id == b.seller_id
        && a.price == b.price
        && a.quantity == b.quantity
        && a.timestamp == b.timestamp;
}

}