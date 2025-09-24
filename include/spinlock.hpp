// include/spinlock.hpp
#pragma once
#include <atomic>
#include <thread>

// =============================================================================
// Custom Spinlock Implementation
// =============================================================================
// A simple spinlock using std::atomic_flag. Useful for very short critical
// sections where the overhead of OS mutex is too high.
//
// WARNING: Can waste CPU cycles and cause priority inversion if used incorrectly.
// Best for: Short critical sections, known thread count, real-time systems
// Avoid for: Long critical sections, unknown load, general-purpose code
// =============================================================================

class spinlock {
public:
    void lock() {
        while (flag.test_and_set(std::memory_order_acquire)) {
            // CPU pause instruction to be nice to hyperthreading
            // Reduces power consumption and improves performance
            #if defined(__x86_64__) || defined(__i386__)
                __builtin_ia32_pause();
            #elif defined(__aarch64__)
                __asm__ __volatile__("yield");
            #else
                std::this_thread::yield();
            #endif
        }
    }
    
    void unlock() {
        flag.clear(std::memory_order_release);
    }
    
    bool try_lock() {
        return !flag.test_and_set(std::memory_order_acquire);
    }

private:
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
};