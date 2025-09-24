#pragma once
#include <chrono>

namespace timing {

class high_resolution_timer {
public:
    high_resolution_timer();
    void reset();
    double elapsed_seconds() const;
    double elapsed_milliseconds() const;
    double elapsed_microseconds() const;

private:
    std::chrono::high_resolution_clock::time_point start_time;
};

} // namespace timing