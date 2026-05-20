// SPDX-License-Identifier: MIT
// Copyright (c) 2026 锯条

#pragma once

#ifdef BENCHMARK_ON

    #include <Windows.h>

struct HiResTimer
{
    LARGE_INTEGER start{};
    LARGE_INTEGER freq{};
    double        elapsed_us{};
};

inline void
timer_start(HiResTimer &t)
{
    QueryPerformanceFrequency(&t.freq);
    QueryPerformanceCounter(&t.start);
}

inline void
timer_stop(HiResTimer &t)
{
    LARGE_INTEGER end;
    QueryPerformanceCounter(&end);
    t.elapsed_us = static_cast<double>(end.QuadPart - t.start.QuadPart) * 1000000.0 /
                   static_cast<double>(t.freq.QuadPart);
}

#else

struct HiResTimer
{
    int unused_{};
};

[[maybe_unused]] inline void
timer_start(HiResTimer &t)
{
}
[[maybe_unused]] inline void
timer_stop(HiResTimer &t)
{
}

#endif // BENCHMARK_ON
