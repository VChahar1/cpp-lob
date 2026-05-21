#pragma once

#include <lob/book.hpp>
#include <lob/events.hpp>
#include <lob/order.hpp>

#include <vector>

namespace lob {

// Process an OrderEvent against the book.
//
// For NewOrder: matches against the opposite side using price-time priority,
// emits Trades for each fill, and rests any leftover quantity on the book.
// For Cancel and Modify: see Day 3 (these are currently unimplemented).
//
// Returns the list of trades emitted (possibly empty).
std::vector<Trade> submit(OrderBook& book, const OrderEvent& event);

// Internal helper exposed for testing. Processes a NewOrder against the book.
// Returns the list of trades emitted.
std::vector<Trade> submit_new_order(OrderBook& book, const NewOrder& order);

}  // namespace lob