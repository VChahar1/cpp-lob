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

bool crosses(Side incoming_side, Price incoming_price, Price best_opposite_price) {
    if (incoming_side == Side::Bid) {
        return incoming_price >= best_opposite_price;
    } else {
        return incoming_price <= best_opposite_price;
    }
}

Timestamp now_ns() {
    const auto now = std::chrono::system_clock::now();
    return static_cast<Timestamp>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            now.time_since_epoch()).count());
}

}

std::vector<Trade> submit_new_order(OrderBook& book, const NewOrder& incoming) {
    std::vector<Trade> trades;

    if (incoming.quantity == 0) return trades;

    if (book.lookup_resting(incoming.id) != nullptr) return trades;

    Quantity remaining = incoming.quantity;
    const Side opp = ::lob::opposite(incoming.side);

    while (remaining > 0) {
        const Order* maker = book.peek_front(opp);
        if (maker == nullptr) break;
        if (!crosses(incoming.side, incoming.price, maker->price)) break;

        const Quantity fill_qty = std::min(maker->quantity, remaining);
        const Price    fill_price = maker->price;
        const OrderId  maker_id = maker->id;
        const Side     maker_side = maker->side;

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

        book.consume_front(maker_side, fill_price, fill_qty);

        remaining -= fill_qty;
    }

    if (remaining > 0) {
        Order rest(incoming.id, incoming.side, incoming.price, remaining, now_ns());
        book.insert_resting(rest);
    }

    return trades;
}
std::vector<Trade> submit_modify(OrderBook& book, const Modify& mod) {
    const auto loc = book.location_of(mod.id);
    if (!loc) return {};

    const Side  side      = loc->first;
    const Price old_price = loc->second;


    const Order* existing = book.lookup_resting(mod.id);
    assert(existing != nullptr && "modify: location_of returned a valid loc but lookup failed");

    const bool keep_priority =
        (mod.new_price == old_price) &&
        (mod.new_quantity > 0) &&
        (mod.new_quantity < existing->quantity);

    if (keep_priority) {
        book.update_quantity_in_place(mod.id, mod.new_quantity);
        return {};
    }

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
}