// Synthetic order flow generator.
//
// Generates a stream of OrderEvents around a random-walking midpoint
// price. Writes them in the text format that lob_cli accepts.
//
// Usage:
//   simulated_flow [--count N] [--seed S] [--mid M] [--out PATH]
//
// Defaults: 100,000 events, seed 42, midpoint 10000, stdout output.

#include <lob/events.hpp>
#include <lob/order.hpp>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <random>
#include <string>

namespace {

struct Config {
    std::uint64_t count = 100'000;
    std::uint64_t seed  = 42;
    std::int64_t  mid   = 10'000;     // signed so we can move it both ways
    std::string   out_path;            // empty = stdout
};

Config parse_args(int argc, char** argv) {
    Config c;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&](const std::string& name) -> std::string {
            if (i + 1 >= argc) {
                std::cerr << "Missing value for " << name << '\n';
                std::exit(1);
            }
            return argv[++i];
        };
        if      (a == "--count") c.count = std::stoull(next(a));
        else if (a == "--seed")  c.seed  = std::stoull(next(a));
        else if (a == "--mid")   c.mid   = std::stoll(next(a));
        else if (a == "--out")   c.out_path = next(a);
        else {
            std::cerr << "Unknown arg: " << a << '\n';
            std::exit(1);
        }
    }
    return c;
}

}  // namespace

int main(int argc, char** argv) {
    const auto cfg = parse_args(argc, argv);

    std::ofstream file_out;
    std::ostream* out = &std::cout;
    if (!cfg.out_path.empty()) {
        file_out.open(cfg.out_path);
        if (!file_out) {
            std::cerr << "Failed to open output: " << cfg.out_path << '\n';
            return 1;
        }
        out = &file_out;
    }

    std::mt19937_64 rng(cfg.seed);

    // Event-type mix tuned to be roughly realistic:
    //   60% new orders
    //   30% cancels (referencing previously-issued ids)
    //   10% modifies
    std::uniform_int_distribution<int> type_dist(0, 99);

    // Price offset from the midpoint, in ticks. Positive numbers move
    // away from the touch on the appropriate side. Most orders cluster
    // near the touch (geometric falloff).
    std::geometric_distribution<int> offset_dist(0.25);

    // Quantity: small, log-spaced.
    std::uniform_int_distribution<int> qty_dist(1, 20);

    // Side: 50/50 with a small random walk on the midpoint to keep prices moving.
    std::bernoulli_distribution side_dist(0.5);
    std::bernoulli_distribution mid_drift(0.05);  // 5% chance per event to nudge mid

    std::int64_t mid = cfg.mid;
    std::uint64_t next_id = 1;
    std::vector<std::uint64_t> live_ids;
    live_ids.reserve(cfg.count);

    *out << "# Synthetic order flow: count=" << cfg.count
         << " seed=" << cfg.seed << " starting_mid=" << cfg.mid << '\n';

    for (std::uint64_t i = 0; i < cfg.count; ++i) {
        // Maybe drift the midpoint.
        if (mid_drift(rng)) {
            mid += side_dist(rng) ? 1 : -1;
            if (mid < 100) mid = 100;  // clamp
        }

        const int t = type_dist(rng);

        if (t < 60 || live_ids.empty()) {
            // NEW
            const bool is_buy = side_dist(rng);
            const int offset  = offset_dist(rng);
            const std::int64_t price = is_buy ? (mid - offset) : (mid + offset);
            const int qty = qty_dist(rng);
            const std::uint64_t id = next_id++;
            *out << "NEW " << id << ' '
                 << (is_buy ? 'B' : 'S') << ' '
                 << price << ' ' << qty << '\n';
            live_ids.push_back(id);
        } else if (t < 90) {
            // CANCEL a live id (chosen uniformly).
            std::uniform_int_distribution<std::size_t> pick(0, live_ids.size() - 1);
            const std::size_t idx = pick(rng);
            const std::uint64_t id = live_ids[idx];
            // Swap-and-pop so we don't try to cancel the same id twice.
            // (Real engines tolerate stale cancels; we just keep our generator clean.)
            live_ids[idx] = live_ids.back();
            live_ids.pop_back();
            *out << "CANCEL " << id << '\n';
        } else {
            // MODIFY a live id.
            std::uniform_int_distribution<std::size_t> pick(0, live_ids.size() - 1);
            const std::size_t idx = pick(rng);
            const std::uint64_t id = live_ids[idx];
            const int offset = offset_dist(rng);
            const std::int64_t new_price = mid + (side_dist(rng) ? offset : -offset);
            const int new_qty = qty_dist(rng);
            *out << "MOD " << id << ' ' << new_price << ' ' << new_qty << '\n';
        }
    }

    return 0;
}