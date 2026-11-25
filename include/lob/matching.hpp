#pragma once

#include <lob/book.hpp>
#include <lob/events.hpp>
#include <lob/order.hpp>

#include <vector>

namespace lob {

std::vector<Trade> submit(OrderBook& book, const OrderEvent& event);

std::vector<Trade> submit_new_order(OrderBook& book, const NewOrder& order);
std::vector<Trade> submit_modify(OrderBook& book, const Modify& mod);

}