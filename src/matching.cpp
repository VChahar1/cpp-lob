#include <lob/matching.hpp>
#include <lob/book.hpp>
#include <lob/order.hpp>
#include <lob/events.hpp>

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
    Quantity remaining = incoming.quantity;
    const Side opposite = ::lob::opposite(incoming.side);

    // Matching loop.
    while (remaining > 0) {
        const Order* maker = book.peek_front(opposite);
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

std::vector<Trade> submit(OrderBook& book, const OrderEvent& event) {
    return std::visit([&book](const auto& concrete) -> std::vector<Trade> {
        using T = std::decay_t<decltype(concrete)>;
        if constexpr (std::is_same_v<T, NewOrder>) {
            return submit_new_order(book, concrete);
        } else {
            // Cancel and Modify: implemented on Day 3.
            return {};
        }
    }, event);
}

}  // namespace lob