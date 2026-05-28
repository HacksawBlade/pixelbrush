#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <Windows.h>

#define BENCHMARK_ON

#ifdef BENCHMARK_ON

typedef struct
{
    LARGE_INTEGER start;
    LARGE_INTEGER freq;
    double        elapsed_us;
} HiResTimer;

extern inline void
timer_start(HiResTimer *t)
{
    QueryPerformanceFrequency(&t->freq);
    QueryPerformanceCounter(&t->start);
}

extern inline void
timer_stop(HiResTimer *t)
{
    LARGE_INTEGER end;
    QueryPerformanceCounter(&end);
    t->elapsed_us = (double) (end.QuadPart - t->start.QuadPart) * 1000000.0 /
                    (double) t->freq.QuadPart;
}

#else

typedef struct
{
    int _unused;
} HiResTimer;
    #define timer_start(t) ((void) 0)
    #define timer_stop(t)  ((void) 0)

#endif // BENCHMARK_ON

#endif // BENCHMARK_H
