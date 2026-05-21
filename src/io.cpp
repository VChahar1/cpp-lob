#include <lob/io.hpp>
#include <lob/events.hpp>
#include <lob/order.hpp>

#include <istream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

namespace lob {

std::optional<OrderEvent> parse_event_line(const std::string& line) {
    // Skip blank lines and comments.
    std::size_t i = 0;
    while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i]))) ++i;
    if (i == line.size()) return std::nullopt;
    if (line[i] == '#') return std::nullopt;

    std::istringstream iss(line);
    std::string keyword;
    iss >> keyword;

    if (keyword == "NEW") {
        OrderId  id;
        char     side_ch;
        Price    price;
        Quantity qty;
        if (!(iss >> id >> side_ch >> price >> qty)) return std::nullopt;
        Side side;
        if      (side_ch == 'B' || side_ch == 'b') side = Side::Bid;
        else if (side_ch == 'S' || side_ch == 's') side = Side::Ask;
        else return std::nullopt;
        return OrderEvent{NewOrder{id, side, price, qty}};
    }

    if (keyword == "CANCEL") {
        OrderId id;
        if (!(iss >> id)) return std::nullopt;
        return OrderEvent{Cancel{id}};
    }

    if (keyword == "MOD") {
        OrderId  id;
        Price    new_price;
        Quantity new_qty;
        if (!(iss >> id >> new_price >> new_qty)) return std::nullopt;
        return OrderEvent{Modify{id, new_price, new_qty}};
    }

    return std::nullopt;
}

std::vector<OrderEvent> read_events(std::istream& in, std::size_t* skipped_out) {
    std::vector<OrderEvent> events;
    std::string line;
    std::size_t skipped = 0;
    while (std::getline(in, line)) {
        auto ev = parse_event_line(line);
        if (ev) {
            events.push_back(std::move(*ev));
        } else if (!line.empty()) {
            // We can't easily tell apart "blank/comment" from "malformed"
            // without re-running the leading-whitespace check, so do that.
            std::size_t i = 0;
            while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i]))) ++i;
            if (i < line.size() && line[i] != '#') {
                ++skipped;
            }
        }
    }
    if (skipped_out) *skipped_out = skipped;
    return events;
}

void write_trade(std::ostream& out, const Trade& t) {
    out << "TRADE "
        << t.buyer_id  << ' '
        << t.seller_id << ' '
        << t.price     << ' '
        << t.quantity  << '\n';
}

}  // namespace lob