#pragma once

#include <lob/order.hpp>

#include <cstddef>
#include <deque>
#include <map>
#include <optional>
#include <unordered_map>
#include <utility>

namespace lob {


class PriceLevel {
public:
    std::deque<Order> orders;

    Quantity total_quantity() const noexcept;

    std::size_t count() const noexcept { return orders.size(); }

    bool empty() const noexcept { return orders.empty(); }
};


class OrderBook {
public:
    OrderBook() = default;


    std::optional<Price> best_bid() const;

    std::optional<Price> best_ask() const;

    std::optional<Price> spread() const;

    Quantity depth_at(Side side, Price price) const;

    std::size_t order_count() const noexcept { return locations_.size(); }


    void insert_resting(const Order& order);
    const Order* peek_front(Side side) const;

    void consume_front(Side side, Price price, Quantity amount);
    bool remove_by_id(OrderId id);
    const Order* lookup_resting(OrderId id) const;

    void update_quantity_in_place(OrderId id, Quantity new_quantity);
    std::optional<std::pair<Side, Price>> location_of(OrderId id) const;

private:
    std::map<Price, PriceLevel> bids_;
    std::map<Price, PriceLevel> asks_;
    std::unordered_map<OrderId, std::pair<Side, Price>> locations_;
};

}