#pragma once
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include "hash_request.h"

template<typename T, size_t Size>
class SPSCQueue {
private:
    static_assert(Size > 0 && ((Size & (Size - 1)) == 0), "Size must be a power of 2");
    static constexpr size_t MASK = Size - 1;

    alignas(64) T buffer[Size];
    alignas(64) std::atomic<size_t> head{0};  // producer
    alignas(64) std::atomic<size_t> tail{0};  // consumer

public:
    bool try_push(T&& item) {
        const size_t current_head = head.load(std::memory_order_relaxed);
        if (((current_head + 1) & MASK) == tail.load(std::memory_order_acquire)) {
            return false;  // queue is full
        }

        buffer[current_head] = std::move(item);
        head.store((current_head + 1) & MASK, std::memory_order_release);
        return true;
    }

    bool try_pop(T& item) {
        const size_t current_tail = tail.load(std::memory_order_relaxed);
        if (current_tail == head.load(std::memory_order_acquire)) {
            return false;  // queue is empty
        }

        item = std::move(buffer[current_tail]);
        tail.store((current_tail + 1) & MASK, std::memory_order_release);
        return true;
    }

    bool empty() const {
        return tail.load(std::memory_order_relaxed) == 
               head.load(std::memory_order_relaxed);
    }

    size_t size() const {
        return (head.load(std::memory_order_relaxed) - 
                tail.load(std::memory_order_relaxed)) & MASK;
    }

    size_t capacity() const {
        return Size - 1;
    }
};


