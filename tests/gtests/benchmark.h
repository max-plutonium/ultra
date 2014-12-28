#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <cstdint>

namespace details {

#include <stdlib.h>
#include <time.h>

class benchmark_cpuclock_timer
{
    clockid_t clockid;
    double start_time;

public:
    benchmark_cpuclock_timer();
    void start();
    double elapsed();
};


class benchmark_omp_timer
{
    double start_time;

public:
    benchmark_omp_timer();
    void start();
    double elapsed();
};


class benchmark_controller
{
    const char *name;
    std::uint32_t iteration;
    const std::uint32_t iterations;
    benchmark_cpuclock_timer cpu_timer;
    benchmark_omp_timer omp_timer;

public:
    benchmark_controller(const char *aname, std::uint32_t aiterations);
    ~benchmark_controller();
    bool is_done();
};

} // namespace details

#define benchmark(name, iterations) \
        for(details::benchmark_controller __benchmark(name, iterations); \
            !__benchmark.is_done();)

#endif // BENCHMARK_H
