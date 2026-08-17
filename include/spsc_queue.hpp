// Taken from Erik Rigtorp's blog, Optimizing a ring buffer for throughput
// https://rigtorp.se/ringbuffer/
// modified to give a done signal to consumer

#pragma once

#include <atomic>
#include <cstddef>
#include <vector>

template <typename T>
class RingBuffer {
   private:
    std::vector<T> data_{};
    alignas(64) std::atomic<size_t> readIdx_{0};
    alignas(64) size_t writeIdxCached_{0};
    alignas(64) std::atomic<size_t> writeIdx_{0};
    alignas(64) size_t readIdxCached_{0};
    std::atomic<bool> done_{false};

   public:
    explicit RingBuffer(size_t capacity) : data_(capacity) {}

    bool push(T item) {
        auto const writeIdx = writeIdx_.load(std::memory_order_relaxed);
        auto nextWriteIdx = writeIdx + 1;
        if (nextWriteIdx == data_.size()) {
            nextWriteIdx = 0;
        }
        if (nextWriteIdx == readIdxCached_) {
            readIdxCached_ = readIdx_.load(std::memory_order_acquire);
            if (nextWriteIdx == readIdxCached_) {
                return false;
            }
        }
        data_[writeIdx] = std::move(item);
        writeIdx_.store(nextWriteIdx, std::memory_order_release);
        return true;
    }

    bool pop(T& out) {
        auto const readIdx = readIdx_.load(std::memory_order_relaxed);
        if (readIdx == writeIdxCached_) {
            writeIdxCached_ = writeIdx_.load(std::memory_order_acquire);
            if (readIdx == writeIdxCached_) {
                return false;
            }
        }
        auto nextReadIdx = readIdx + 1;
        if (nextReadIdx == data_.size()) {
            nextReadIdx = 0;
        }
        out = std::move(data_[readIdx]);
        readIdx_.store(nextReadIdx, std::memory_order_release);
        return true;
    }

    void shutdown() { done_.store(true, std::memory_order_release); }

    bool isDone() const { return done_.load(std::memory_order_acquire); }
};
