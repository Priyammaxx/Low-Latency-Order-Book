#pragma once
#include <condition_variable>
#include <mutex>
#include <queue>

template <typename T>
class MutexQueue {
   private:
    std::mutex mutex_;
    std::condition_variable notEmptpy_, notFull_;
    std::queue<T> queue_;
    size_t capacity_;
    bool done_ = false;

   public:
    explicit MutexQueue(size_t capacity) : capacity_(capacity) {}

    void push(T item) {
        std::unique_lock<std::mutex> lock(mutex_);
        notFull_.wait(lock, [&] { return queue_.size() < capacity_ || done_; });
        queue_.push(std::move(item));
        lock.unlock();
        notEmptpy_.notify_one();
    }

    bool pop(T& out) {
        std::unique_lock<std::mutex> lock(mutex_);
        notFull_.wait(lock, [&] { return !queue_.empty() || done_; });
        if (queue_.empty()) return false;
        out = std::move(queue_.front());
        queue_.pop();
        lock.unlock();
        notFull_.notify_one();
        return true;
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            done_ = true;
        }
        notEmptpy_.notify_all();
        notFull_.notify_all();
    }
};
