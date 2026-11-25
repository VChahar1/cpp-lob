# cpp-lob

[![CI](https://github.com/VChahar1/cpp-lob/actions/workflows/ci.yml/badge.svg)](https://github.com/VChahar1/cpp-lob/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

A limit order book and matching engine implemented in modern C++17. Built as a focused exercise in the data structures, algorithms, and engineering tradeoffs underlying continuous-double-auction exchange systems.

The engine implements price-time priority matching, supports new/cancel/modify order events, and has a reproducible single-threaded synthetic-flow benchmark (see [Benchmarks](#benchmarks)).
## Quick start

```bash
git clone https://github.com/VChahar1/cpp-lob.git
cd cpp-lob
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/tests/lob_tests       # 50 test cases / 209 assertions in the cleanup run
./scripts/run_benchmark.sh    # generates 1M synthetic events and benchmarks
```

For a quick demo:

```bash
echo "NEW 1 S 10100 5
NEW 2 B 10100 5" | ./build/lob_cli
# Output: TRADE 2 1 10100 5
```
## What's not implemented (and why)

This is a learning project, not a production exchange. Scope was deliberately bounded to keep the codebase focused. Notable omissions:

- **Networking**: no socket layer, no FIX/ITCH/OUCH protocols. Events come from a file or stdin.
- **Concurrency**: single-threaded only. Real engines either use a single hot thread with strict isolation or shard across multiple matching threads with cross-thread coordination.
- **Persistence and recovery**: nothing is durable. A real engine writes every event to a log and can replay to reconstruct state after a crash.
- **Self-trade prevention (STP)**: not implemented because the current data model lacks a `TraderId` field on orders. Adding this would be straightforward but cascades through every event and test; left as documented future work.
- **Order types beyond basic limit orders**: no market orders (a marketable limit order is effectively a market order with a price cap), no stop orders, no iceberg orders, no IOC/FOK time-in-force flags.
- **Multiple instruments**: one book at a time. Multi-instrument support would mean a `std::unordered_map<InstrumentId, OrderBook>` and dispatch by instrument.

## Benchmarks

| Metric                | Value                  |
|-----------------------|------------------------|
| Events processed      | 1,000,000              |
| Trades emitted        | 490,992                |
| Wall time             | 0.0944904 s            |
| **Throughput**        | **10.5831 M events/sec** |
| Wall time / event     | 94.4904 ns             |
| Final book size       | 38,467 orders          |

Reproduce with `./scripts/run_benchmark.sh`. The synthetic flow has roughly 60% new orders, 30% cancels, 10% modifies, with prices clustered geometrically around a random-walking midpoint; approximately half of the new orders end up marketable and produce fills.

These are observations from one cleanup-environment run with GCC 15.2.0, Release and the default `LOB_WERROR=OFF`; the exact output is saved in [results/cleanup-benchmark.txt](results/cleanup-benchmark.txt). Timing depends on the machine, compiler and workload. The engine uses `std::map` and returns a heap-allocated `std::vector<Trade>` per event. No comparison with other engines was verified; TODO: add a controlled cross-engine benchmark before making comparative speed claims.

## Validation status

The documented Release build passed 50 test cases with 209 assertions. The stricter `LOB_WERROR=ON` build fails on existing shadowing and possible null-dereference diagnostics with GCC 15.2.0. The [CI badge](https://github.com/VChahar1/cpp-lob/actions/workflows/ci.yml) resolves correctly and reported a failing existing workflow during cleanup. Engine source and CI settings were not changed.


## License

MIT — see [LICENSE](LICENSE).
