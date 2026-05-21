#include <lob/matching.hpp>
#include <lob/book.hpp>
#include <lob/order.hpp>
#include <lob/events.hpp>
#include <cassert>  
#include <algorithm>
#include <chrono>
#include <variant>

namespace lob {

namespace {

// Returns true if `incoming_price` crosses against `best_opposite_price`
// for an aggressor of side `incoming_side`.
//
//   A Bid (buyer) crosses when it is willing to pay >= the ask.
//   An Ask (seller) crosses when it accepts <= the bid.
bool crosses(Side incoming_side, Price incoming_price, Price best_opposite_price) {
    if (incoming_side == Side::Bid) {
        return incoming_price >= best_opposite_price;
    } else {
        return incoming_price <= best_opposite_price;
    }
}

// Current time as a Timestamp, used to stamp emitted trades.
Timestamp now_ns() {
    const auto now = std::chrono::system_clock::now();
    return static_cast<Timestamp>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            now.time_since_epoch()).count());
}

}  // anonymous namespace

std::vector<Trade> submit_new_order(OrderBook& book, const NewOrder& incoming) {
    std::vector<Trade> trades;

    // Edge case: zero quantity. Drop silently.
    if (incoming.quantity == 0) return trades;

    // Edge case: duplicate id. Drop silently to avoid corrupting the book.
    if (book.lookup_resting(incoming.id) != nullptr) return trades;

    Quantity remaining = incoming.quantity;
    const Side opp = ::lob::opposite(incoming.side);

    // Matching loop.
    while (remaining > 0) {
        const Order* maker = book.peek_front(opp);
        if (maker == nullptr) break;                       // opposite side empty
        if (!crosses(incoming.side, incoming.price, maker->price)) break;

        const Quantity fill_qty = std::min(maker->quantity, remaining);
        const Price    fill_price = maker->price;          // resting price wins
        const OrderId  maker_id = maker->id;
        const Side     maker_side = maker->side;

        // Build the trade. Buyer is whoever is on the Bid side.
        Trade t{};
        if (incoming.side == Side::Bid) {
            t.buyer_id  = incoming.id;
            t.seller_id = maker_id;
        } else {
            t.buyer_id  = maker_id;
            t.seller_id = incoming.id;
        }
        t.price     = fill_price;
        t.quantity  = fill_qty;
        t.timestamp = now_ns();
        trades.push_back(t);

        // Apply the fill to the book. consume_front handles all the bookkeeping:
        // partial fill, full fill, level removal, locations cleanup.
        book.consume_front(maker_side, fill_price, fill_qty);

        // Reduce the taker's remaining quantity.
        remaining -= fill_qty;
    }

    // Rest any leftover quantity as a new resting order.
    if (remaining > 0) {
        Order rest(incoming.id, incoming.side, incoming.price, remaining, now_ns());
        book.insert_resting(rest);
    }

    return trades;
}
std::vector<Trade> submit_modify(OrderBook& book, const Modify& mod) {
    // Look up the existing order. If unknown, no-op.
    const auto loc = book.location_of(mod.id);
    if (!loc) return {};

    const Side  side      = loc->first;
    const Price old_price = loc->second;

    // Decision: keep time priority, or treat as cancel + new?
    //
    // Keep priority only if:
    //   - price is unchanged
    //   - new quantity is strictly less than old quantity (decrease only)
    //   - new quantity is > 0
    //
    // Anything else (price change, quantity increase, new quantity zero) is
    // treated as cancel + new.
    //
    // We need the old quantity for the comparison. The cheapest way to get
    // it is via depth_at if there's only one order at this level — but in
    // general we need to find the specific order. Add a helper to OrderBook
    // for this (see book.hpp: lookup_resting).

    const Order* existing = book.lookup_resting(mod.id);
    assert(existing != nullptr && "modify: location_of returned a valid loc but lookup failed");

    const bool keep_priority =
        (mod.new_price == old_price) &&
        (mod.new_quantity > 0) &&
        (mod.new_quantity < existing->quantity);

    if (keep_priority) {
        // In-place update: reduce the quantity, leave time priority intact.
        book.update_quantity_in_place(mod.id, mod.new_quantity);
        return {};
    }

    // Cancel + new path. Remove the old order; if new quantity is > 0,
    // resubmit as a NewOrder which goes through matching.
    book.remove_by_id(mod.id);
    if (mod.new_quantity == 0) {
        return {};
    }
    NewOrder replacement{mod.id, side, mod.new_price, mod.new_quantity};
    return submit_new_order(book, replacement);
}
std::vector<Trade> submit(OrderBook& book, const OrderEvent& event) {
    return std::visit([&book](const auto& concrete) -> std::vector<Trade> {
        using T = std::decay_t<decltype(concrete)>;
        if constexpr (std::is_same_v<T, NewOrder>) {
            return submit_new_order(book, concrete);
        } else if constexpr (std::is_same_v<T, Cancel>) {
            book.remove_by_id(concrete.id);
            return {};
        } else if constexpr (std::is_same_v<T, Modify>) {
            return submit_modify(book, concrete);
        }
    }, event);
}
}  // namespace lob