
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
    std::int64_t  mid   = 10'000;
    std::string   out_path;
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

}

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

    std::uniform_int_distribution<int> type_dist(0, 99);

    std::geometric_distribution<int> offset_dist(0.25);

    std::uniform_int_distribution<int> qty_dist(1, 20);

    std::bernoulli_distribution side_dist(0.5);
    std::bernoulli_distribution mid_drift(0.05);

    std::int64_t mid = cfg.mid;
    std::uint64_t next_id = 1;
    std::vector<std::uint64_t> live_ids;
    live_ids.reserve(cfg.count);

    *out << "# Synthetic order flow: count=" << cfg.count
         << " seed=" << cfg.seed << " starting_mid=" << cfg.mid << '\n';

    for (std::uint64_t i = 0; i < cfg.count; ++i) {
        if (mid_drift(rng)) {
            mid += side_dist(rng) ? 1 : -1;
            if (mid < 100) mid = 100;
        }

        const int t = type_dist(rng);

        if (t < 60 || live_ids.empty()) {
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
            std::uniform_int_distribution<std::size_t> pick(0, live_ids.size() - 1);
            const std::size_t idx = pick(rng);
            const std::uint64_t id = live_ids[idx];
            live_ids[idx] = live_ids.back();
            live_ids.pop_back();
            *out << "CANCEL " << id << '\n';
        } else {
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