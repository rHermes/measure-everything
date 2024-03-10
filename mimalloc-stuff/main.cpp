//
// Created by rhermes on 3/9/24.
//

#include <benchmark/benchmark.h>

#include <mimalloc.h>
// #include <mimalloc-new-delete.h>
#include <vector>
#include <iostream>
#include <list>
#include <random>


#include "../third_party/pcg_random.hpp"
#include "../third_party/pcg_extras.hpp"


static pcg64_fast rng(pcg_extras::static_arbitrary_seed<std::uint64_t>::value);

template <typename Alloc>
static void allocTest(const Alloc& alloc) {
    using UT = std::uint64_t;

    std::bernoulli_distribution d(0.4);

    std::list<UT, std::decay_t<decltype(alloc)>> ls(alloc);
    for (std::size_t i = 0; i < 10'000'000; i++) {
        if (d(rng)) {
            if (!ls.empty())
                ls.pop_front();
        } else {
            ls.push_front(i);
        }
    }

    benchmark::DoNotOptimize(ls.size());
}

// Figure out some workload I think could be affected by using a heap and a non heap allocator?
static void BM_normalAlloc(benchmark::State& state) {
    rng.seed(10);
    for (auto _ : state) {
        mi_stl_allocator<std::uint64_t> ourAlloc;
        allocTest(ourAlloc);
    }
}

// Figure out some workload I think could be affected by using a heap and a non heap allocator?
static void BM_heapAlloc(benchmark::State& state) {
    rng.seed(10);
    for (auto _ : state) {
        mi_heap_stl_allocator<std::uint64_t> ourAlloc;
        allocTest(ourAlloc);
    }
}

// Figure out some workload I think could be affected by using a heap and a non heap allocator?
static void BM_heapDestroyAlloc(benchmark::State& state) {
    using UT = std::uint64_t;
    rng.seed(10);
    for (auto _ : state) {
        mi_heap_destroy_stl_allocator<UT> ourAlloc;
        allocTest(ourAlloc);
    }
}

BENCHMARK(BM_normalAlloc);
BENCHMARK(BM_heapAlloc);
BENCHMARK(BM_heapDestroyAlloc);

BENCHMARK_MAIN();