// include/padded.hpp
#pragma once
#include <cstddef>
#include <type_traits>

// =============================================================================
// Cache-Line Aligned Padding Utility
// =============================================================================
// This template wrapper prevents false sharing by ensuring each instance
// occupies its own cache line. False sharing occurs when multiple threads
// access different variables that happen to share the same cache line,
// causing unnecessary cache coherency traffic.
//
// Usage:
//   padded<std::atomic<int>> counter1;  // On its own cache line
//   padded<std::atomic<int>> counter2;  // On a different cache line
//
// Without padding, accessing counter1 and counter2 from different threads
// could cause cache line ping-ponging, severely degrading performance.
// =============================================================================

// Cache line size for most modern CPUs (x86-64, ARM64)
constexpr size_t CACHE_LINE_SIZE = 64;

template<typename T>
struct alignas(CACHE_LINE_SIZE) padded {
    T value;
    
    // Default constructor
    padded() = default;
    
    // Constructor from value (for regular types)
    padded(const T& v) : value(v) {}
    padded(T&& v) : value(std::move(v)) {}
    
    // Transparent access to the wrapped value
    operator T&() { return value; }
    operator const T&() const { return value; }
    
    // Assignment operators
    T& operator=(const T& v) { value = v; return value; }
    T& operator=(T&& v) { value = std::move(v); return value; }
    
    // Forward common operations for counters
    auto operator++() -> decltype(++value) { return ++value; }
    auto operator++(int) -> decltype(value++) { return value++; }
    auto operator--() -> decltype(--value) { return --value; }
    auto operator--(int) -> decltype(value--) { return value--; }
};

// Specialization for atomic types
template<typename T>
struct alignas(CACHE_LINE_SIZE) padded<std::atomic<T>> {
    std::atomic<T> value;
    
    // Default constructor
    padded() : value(T{}) {}
    
    // Constructor from underlying value type
    explicit padded(T initial_value) : value(initial_value) {}
    
    // Transparent access
    operator std::atomic<T>&() { return value; }
    operator const std::atomic<T>&() const { return value; }
    
    // Forward atomic operations
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