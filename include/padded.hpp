// include/padded.hpp
#pragma once
#include <atomic>
#include <cstddef>
#include <type_traits>
#include <utility>

// =============================================================================
// Cache-Line Aligned Padding Utility
// =============================================================================
// Prevents false sharing by ensuring each instance occupies its own cache line.
// =============================================================================

constexpr size_t CACHE_LINE_SIZE = 64;

template <typename T>
struct alignas(CACHE_LINE_SIZE) padded {
    T value;

    padded() = default;
    padded(const T& v) : value(v) {}
    padded(T&& v) : value(std::move(v)) {}

    operator T&() { return value; }
    operator const T&() const { return value; }

    T& operator=(const T& v) {
        value = v;
        return value;
    }
    T& operator=(T&& v) {
        value = std::move(v);
        return value;
    }

    auto operator++() -> decltype(++value) { return ++value; }
    auto operator++(int) -> decltype(value++) { return value++; }
    auto operator--() -> decltype(--value) { return --value; }
    auto operator--(int) -> decltype(value--) { return value--; }
};

template <typename T>
struct alignas(CACHE_LINE_SIZE) padded<std::atomic<T>> {
    std::atomic<T> value;

    padded() : value(T{}) {}
    explicit padded(T initial_value) : value(initial_value) {}

    operator std::atomic<T>&() { return value; }
    operator const std::atomic<T>&() const { return value; }

    T load(std::memory_order order = std::memory_order_seq_cst) const {
        return value.load(order);
    }

    void store(T desired, std::memory_order order = std::memory_order_seq_cst) {
        value.store(desired, order);
    }

    T fetch_add(T arg, std::memory_order order = std::memory_order_seq_cst) {
        return value.fetch_add(arg, order);
    }

    T fetch_sub(T arg, std::memory_order order = std::memory_order_seq_cst) {
        return value.fetch_sub(arg, order);
    }

    T operator++() { return value.fetch_add(1) + 1; }
    T operator++(int) { return value.fetch_add(1); }
    T operator--() { return value.fetch_sub(1) - 1; }
    T operator--(int) { return value.fetch_sub(1); }
};
