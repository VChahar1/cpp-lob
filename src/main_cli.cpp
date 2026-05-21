#include <lob/book.hpp>
#include <lob/events.hpp>
#include <lob/io.hpp>
#include <lob/matching.hpp>

#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    // Read events from a file if a path is given, otherwise from stdin.
    std::istream* in_ptr = &std::cin;
    std::ifstream file_in;
    if (argc >= 2) {
        file_in.open(argv[1]);
        if (!file_in) {
            std::cerr << "Failed to open: " << argv[1] << '\n';
            return 1;
        }
        in_ptr = &file_in;
    }

    lob::OrderBook book;
    std::string line;
    std::size_t total_events = 0;
    std::size_t skipped      = 0;
    std::size_t total_trades = 0;

    while (std::getline(*in_ptr, line)) {
        const auto ev = lob::parse_event_line(line);
        if (!ev) {
            // Could be blank/comment or malformed. Quick check:
            std::size_t i = 0;
            while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i]))) ++i;
            if (i < line.size() && line[i] != '#') ++skipped;
            continue;
        }
        ++total_events;

        const auto trades = lob::submit(book, *ev);
        for (const auto& t : trades) {
            lob::write_trade(std::cout, t);
            ++total_trades;
        }
    }

    // Summary to stderr, so stdout stays clean for the trade output.
    std::cerr << "Processed " << total_events << " events, "
              << "emitted " << total_trades << " trades, "
              << "skipped " << skipped << " malformed lines.\n";

    // Final book state to stderr.
    if (auto bb = book.best_bid()) std::cerr << "Best bid: " << *bb << '\n';
    if (auto ba = book.best_ask()) std::cerr << "Best ask: " << *ba << '\n';
    if (auto sp = book.spread())   std::cerr << "Spread: "   << *sp << '\n';
    std::cerr << "Orders remaining: " << book.order_count() << '\n';

    return 0;
}