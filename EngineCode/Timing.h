#pragma once
#include <chrono>


namespace Magic
{
namespace Timing
{
template <
    class result_t   = std::chrono::milliseconds,
    class clock_t    = std::chrono::steady_clock,
    class duration_t = std::chrono::milliseconds
>
auto SinceMS(std::chrono::time_point<clock_t, duration_t> const& start)
{
    return std::chrono::duration_cast<result_t>(clock_t::now() - start);
}

template <
    class result_t   = std::chrono::microseconds,
    class clock_t    = std::chrono::steady_clock,
    class duration_t = std::chrono::microseconds
>
auto SinceUS(std::chrono::time_point<clock_t, duration_t> const& start)
{
    return std::chrono::duration_cast<result_t>(clock_t::now() - start);
}
}
}
