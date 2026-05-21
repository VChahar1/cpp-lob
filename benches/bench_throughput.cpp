// Throughput benchmark.
//
// Reads a pre-generated event file into memory (no parsing during timing),
// then runs the events through the book and reports events/second.
//
// Usage:
//   bench_throughput <events_file>

#include <lob/book.hpp>
#include <lob/events.hpp>
#include <lob/io.hpp>
#include <lob/matching.hpp>

#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <events_file>\n";
        return 1;
    }

    // Load events into memory. We don't include parsing in the benchmark
    // because we're measuring the engine, not the parser.
    std::ifstream in(argv[1]);
    if (!in) {
        std::cerr << "Failed to open: " << argv[1] << '\n';
        return 1;
    }

    std::size_t skipped = 0;
    const auto events = lob::read_events(in, &skipped);
    std::cerr << "Loaded " << events.size() << " events ("
              << skipped << " skipped).\n";

    if (events.empty()) {
        std::cerr << "No events to benchmark.\n";
        return 1;
    }

    // Warm up: run the events once and discard. This pages in memory and
    // primes branch predictors before the timed run.
    {
        lob::OrderBook warmup;
        std::size_t trade_count = 0;
        for (const auto& ev : events) {
            const auto trades = lob::submit(warmup, ev);
            trade_count += trades.size();
        }
        std::cerr << "Warmup: " << trade_count << " trades.\n";
    }

    // Timed run.
    lob::OrderBook book;
    std::size_t trade_count = 0;

    using clock = std::chrono::steady_clock;
    const auto t0 = clock::now();

    for (const auto& ev : events) {
        const auto trades = lob::submit(book, ev);
        trade_count += trades.size();
    }

    const auto t1 = clock::now();
    const auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
    const double elapsed_s = static_cast<double>(elapsed_ns) / 1e9;
    const double events_per_sec = static_cast<double>(events.size()) / elapsed_s;
    const double ns_per_event = static_cast<double>(elapsed_ns) / static_cast<double>(events.size());

    std::cerr << "\n=== Results ===\n"
              << "Events:           " << events.size() << '\n'
              << "Trades emitted:   " << trade_count << '\n'
              << "Wall time:        " << elapsed_s << " s\n"
              << "Throughput:       " << events_per_sec << " events/sec\n"
              << "Latency/event:    " << ns_per_event << " ns\n"
              << "Final book size:  " << book.order_count() << " orders\n";

    return 0;
}