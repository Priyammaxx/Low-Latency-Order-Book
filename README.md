# Order Book Engine (C++)

A price-time-priority limit order book with a lock-free market data feed, built to demonstrate systems-level performance engineering — cache-conscious data layout, lock-free concurrency, and measured (not assumed) hardware behavior.

## Status: In progress

## Implemented so far

- **Order representation** (`order.hpp`/`order.cpp`) — heap-allocated `Order` struct with intrusive `prev`/`next` links, alignment/padding checked via `sizeof`/`alignof`.
- **Matching engine** (`order_book.hpp`/`order_book.cpp`) — array-indexed price levels (no `std::map`), integer tick-based price↔index mapping, intrusive doubly-linked FIFO per level, O(1) cancel via an id→pointer lookup table.
- **Price-time priority matching** — partial fills, multi-level walks, `bestBid`/`bestAsk` maintained incrementally and corrected on partial drains and cancels at the touch.
- **Correctness harness** — deterministic trade-log assertions, randomized fuzz testing, invariant checks (no crossed book, count/pointer consistency), a brute-force cross-check for best bid/ask, all validated clean under AddressSanitizer/UBSan.
- **Bounded thread-safe queue** (`mutex_queue.hpp`) — `std::mutex` + `std::condition_variable`, spurious-wakeup-safe waits, clean shutdown handshake. Verified: produced == consumed counts, strict sequence ordering preserved, backpressure path actually exercised, clean under ThreadSanitizer.
- **Memory ordering fundamentals** — isolated acquire/release vs. relaxed-ordering test confirming the happens-before guarantee needed for the lock-free queue, verified via ThreadSanitizer.

## Remaining work, in order

- [ ] Lock-free SPSC ring buffer (fixed-capacity, atomic head/tail), correctness-first with `seq_cst`, verified single-threaded then multi-threaded under ThreadSanitizer.
- [ ] Relax memory ordering to acquire/release; re-verify race-freedom and functional correctness are both unchanged.
- [ ] Pin producer/consumer threads to separate CPU cores.
- [ ] Allocate the ring buffer's backing storage via `mmap`, page-aligned.
- [ ] Latency measurement harness (p50/p99/p999) comparing the mutex-based queue against the lock-free queue.
- [ ] Profile with `perf`, identify false sharing between head/tail, fix via cache-line padding, capture before/after cache-miss numbers.
- [ ] Check syscall count on the hot path with `strace`.
- [ ] Final benchmark results, concepts-demonstrated table, and build/run instructions.

## Build

```bash
make debug     # ASan/UBSan build, for correctness testing
make tsan      # ThreadSanitizer build, for concurrency testing
make release   # optimized build, for benchmarking
```

