#include <iostream>
#include <random>
#include <benchmark/benchmark.h>
#include <vector>

#include "vector2d.h"


static void BM_plainVector_readAllSeq(benchmark::State& state) {
    std::vector<std::vector<float>> mdim(state.range(0));
    for (auto& dim : mdim)
        dim.resize(state.range(1));

    // Fixed seed
    std::mt19937 gen(10);
    std::uniform_real_distribution<float> dis(0.0, 1.0);
    for (auto& dim : mdim)
        for (auto& x : dim)
            x = dis(gen);



    for (auto _ : state) {
        for (const auto& row : mdim) {
            float x = 0.0;
            for (const auto& elem : row)
                benchmark::DoNotOptimize(x = elem);
        }
    }
    state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations() * state.range(0) * state.range(1)));
}

static void BM_plainVector_readRandom(benchmark::State& state) {
    std::vector<std::vector<float>> mdim(state.range(0));
    for (auto& dim : mdim)
        dim.resize(state.range(1));

    // Fixed seed
    std::mt19937 gen(10);
    std::uniform_real_distribution<float> dis(0.0, 1.0);
    for (auto& dim : mdim)
        for (auto& x : dim)
            x = dis(gen);


    std::uniform_int_distribution<std::size_t> rowDist(0, state.range(0)-1);
    std::uniform_int_distribution<std::size_t> colDist(0, state.range(1)-1);

    float x = 0;
    for (auto _ : state) {
        const auto row = rowDist(gen);
        const auto col = colDist(gen);
        benchmark::DoNotOptimize(x = mdim[row][col]);
    }
    benchmark::DoNotOptimize(x++);
    state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations()));
}

static void BM_2dvec_readAllSeq(benchmark::State& state) {
    Vector2D<float> mdim(state.range(0), state.range(1));

    // Fixed seed
    std::mt19937 gen(10);
    std::uniform_real_distribution<float> dis(0.0, 1.0);
    for (std::size_t row = 0; row < mdim.rows(); row++) {
        for (std::size_t col = 0; col < mdim.columns(); col++) {
            mdim.get(row, col) = dis(gen);
        }
    }


    for (auto _ : state) {
        for (std::size_t row = 0; row < mdim.rows(); row++) {
            float x = 0;
            for (std::size_t col = 0; col < mdim.columns(); col++) {
                 benchmark::DoNotOptimize(x = mdim.get(row, col));
            }
        }
    }
    state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations() * mdim.rows() * mdim.columns()));
}

static void BM_2dvec_readRandom(benchmark::State& state) {
    Vector2D<float> mdim(state.range(0), state.range(1));

    // Fixed seed
    std::mt19937 gen(10);
    std::uniform_real_distribution<float> dis(0.0, 1.0);
    for (std::size_t row = 0; row < mdim.rows(); row++) {
        for (std::size_t col = 0; col < mdim.columns(); col++) {
            mdim.get(row, col) = dis(gen);
        }
    }

    std::uniform_int_distribution<std::size_t> rowDist(0, state.range(0)-1);
    std::uniform_int_distribution<std::size_t> colDist(0, state.range(1)-1);
    float x = 0;

    for (auto _ : state) {
        const auto row = rowDist(gen);
        const auto col = colDist(gen);
        benchmark::DoNotOptimize(x = mdim.get(row, col));
    }

    benchmark::DoNotOptimize(x++);
    state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations()));
}

BENCHMARK(BM_plainVector_readAllSeq)
->ArgsProduct( {
    benchmark::CreateRange(8, 4*2048, 8),
    benchmark::CreateRange(8, 4*2048, 8),
});

BENCHMARK(BM_plainVector_readRandom)
->ArgsProduct( {
    benchmark::CreateRange(8, 4*2048, 8),
    benchmark::CreateRange(8, 4*2048, 8),
});

BENCHMARK(BM_2dvec_readAllSeq)
->ArgsProduct( {
    benchmark::CreateRange(8, 4*2048, 8),
    benchmark::CreateRange(8, 4*2048, 8),
});

BENCHMARK(BM_2dvec_readRandom)
->ArgsProduct( {
    benchmark::CreateRange(8, 4*2048, 8),
    benchmark::CreateRange(8, 4*2048, 8),
});

