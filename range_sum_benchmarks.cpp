#include <benchmark/benchmark.h>

#include "sparse-table.h"
#include <cinttypes>
#include <random>

static void BM_rangeSum_init_PSA(benchmark::State& state) {
    const auto maxN = static_cast<std::size_t>(state.range(0));

    using T = std::int64_t;

    std::mt19937 gen(10);
    std::uniform_int_distribution<T> vals(1, 10000);

    std::vector<T> cool(maxN);
    for (std::size_t i = 0; i < maxN; i++)
        cool[i] = vals(gen);


    std::uniform_int_distribution<std::size_t> queryDist(1, maxN);
    for (auto _ : state) {
        std::vector<T> psa(maxN+1);
        for (std::size_t i = 0; i < maxN; i++) {
            psa[i+1] = psa[i] + cool[i];
        }

        // Generate a random question, to prevent optimizer from removing everything.
        auto l = queryDist(gen);
        auto r = queryDist(gen);
        if (r < l)
            std::swap(r, l);

        T ans = psa[r] - psa[l-1];
        benchmark::DoNotOptimize(ans);
    }

    state.SetItemsProcessed(state.iterations());
}

static void BM_rangeSum_queryAll_PSA(benchmark::State& state) {
    const auto maxN = static_cast<std::size_t>(state.range(0));

    using T = std::int64_t;

    std::mt19937 gen(10);
    std::uniform_int_distribution<T> vals(1, 10000);

    std::vector<T> cool(maxN);
    for (std::size_t i = 0; i < maxN; i++)
        cool[i] = vals(gen);

    std::vector<T> psa(maxN+1);
    for (std::size_t i = 0; i < maxN; i++) {
        psa[i+1] = psa[i] + cool[i];
    }

    std::uniform_int_distribution<std::size_t> queryDist(1, maxN);
    for (auto _ : state) {
        // Generate a random question, to prevent optimizer from removing everything.
        auto l = queryDist(gen);
        auto r = queryDist(gen);
        if (r < l)
            std::swap(r, l);

        T ans = psa[r] - psa[l-1];
        benchmark::DoNotOptimize(ans);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetComplexityN(state.range(0));
}

static void BM_rangeSum_queryCacheMiss_PSA(benchmark::State& state) {
    const auto maxN = static_cast<std::size_t>(state.range(0));

    using T = std::int64_t;

    std::mt19937 gen(10);
    std::uniform_int_distribution<T> vals(1, 10000);

    std::vector<T> cool(maxN);
    for (std::size_t i = 0; i < maxN; i++)
        cool[i] = vals(gen);

    std::vector<T> psa(maxN+1);
    for (std::size_t i = 0; i < maxN; i++) {
        psa[i+1] = psa[i] + cool[i];
    }

    const auto cacheLine = std::hardware_constructive_interference_size;
    constexpr auto stride = cacheLine / sizeof(std::int64_t);

    std::uniform_int_distribution<std::size_t> queryDist(1, stride-1);
    std::uniform_int_distribution<std::size_t> strideDist(1, 10);

    std::size_t cur_idx = 0;
    for (auto _ : state) {
        // Generate a random question, to prevent optimizer from removing everything.
        auto l = cur_idx;
        auto r = l + queryDist(gen);
        T ans = psa[r] - psa[l];
        benchmark::DoNotOptimize(ans);


        cur_idx = (cur_idx + strideDist(gen)* stride) % (maxN - stride);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetComplexityN(state.range(0));
}

static void BM_rangeSum_querySmall_PSA(benchmark::State& state) {
    const auto maxN = static_cast<std::size_t>(state.range(0));

    using T = std::int64_t;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<T> vals(1, 10000);

    std::vector<T> cool(maxN);
    for (std::size_t i = 0; i < maxN; i++)
        cool[i] = vals(gen);

    std::vector<T> psa(maxN+1);
    for (std::size_t i = 0; i < maxN; i++) {
        psa[i+1] = psa[i] + cool[i];
    }

    const auto startRng = std::uniform_int_distribution<std::size_t>(1, maxN-1024)(gen);

    std::uniform_int_distribution<std::size_t> queryDist(startRng, startRng+1024);
    for (auto _ : state) {
        // Generate a random question, to prevent optimizer from removing everything.
        auto l = queryDist(gen);
        auto r = queryDist(gen);
        if (r < l)
            std::swap(r, l);

        T ans = psa[r] - psa[l-1];
        benchmark::DoNotOptimize(ans);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetComplexityN(state.range(0));
}


static void BM_rangeSum_init_SparseTable(benchmark::State& state) {
    const auto maxN = static_cast<std::size_t>(state.range(0));

    using T = std::int64_t;

    std::mt19937 gen(10);
    std::uniform_int_distribution<T> vals(1, 10000);

    auto f = [](const T a, const T b) { return a + b; };


    std::vector<T> cool(maxN);
    for (std::size_t i = 0; i < maxN; i++)
        cool[i] = vals(gen);


    std::uniform_int_distribution<std::size_t> queryDist(0, maxN-1);
    for (auto _ : state) {
        SparseTable<T, decltype(f), false> st(maxN);
        st.precompute(cool.begin(), cool.end());

        // Generate a random question, to prevent optimizer from removing everything.
        auto l = queryDist(gen);
        auto r = queryDist(gen);
        if (r < l)
            std::swap(r, l);

        T ans = st.query(l, r);
        benchmark::DoNotOptimize(ans);
    }

    state.SetItemsProcessed(state.iterations());
}

static void BM_rangeSum_queryAll_SparseTable(benchmark::State& state) {
    const auto maxN = static_cast<std::size_t>(state.range(0));

    using T = std::int64_t;

    std::mt19937 gen(10);
    std::uniform_int_distribution<T> vals(1, 10000);

    auto f = [](const T a, const T b) { return a + b; };
    SparseTable<T, decltype(f), false> st(maxN);


    std::vector<T> cool(maxN);
    for (std::size_t i = 0; i < maxN; i++)
        cool[i] = vals(gen);

    st.precompute(cool.begin(), cool.end());


    std::uniform_int_distribution<std::size_t> queryDist(0, maxN-1);
    for (auto _ : state) {
        // Generate a random question, to prevent optimizer from removing everything.
        auto l = queryDist(gen);
        auto r = queryDist(gen);
        if (r < l)
            std::swap(r, l);

        T ans = st.query(l, r);
        benchmark::DoNotOptimize(ans);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetComplexityN(state.range(0));
}

static void BM_rangeSum_querySmall_SparseTable(benchmark::State& state) {
    const auto maxN = static_cast<std::size_t>(state.range(0));

    using T = std::int64_t;

    std::mt19937 gen(10);
    std::uniform_int_distribution<T> vals(1, 10000);

    auto f = [](const T a, const T b) { return a + b; };
    SparseTable<T, decltype(f), false> st(maxN);


    std::vector<T> cool(maxN);
    for (std::size_t i = 0; i < maxN; i++)
        cool[i] = vals(gen);

    st.precompute(cool.begin(), cool.end());


    const auto startRng = std::uniform_int_distribution<std::size_t>(1, maxN-1024)(gen);
    std::uniform_int_distribution<std::size_t> queryDist(startRng, startRng+1024);
    for (auto _ : state) {
        // Generate a random question, to prevent optimizer from removing everything.
        auto l = queryDist(gen);
        auto r = queryDist(gen);
        if (r < l)
            std::swap(r, l);

        T ans = st.query(l, r);
        benchmark::DoNotOptimize(ans);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetComplexityN(state.range(0));
}


BENCHMARK(BM_rangeSum_init_PSA)->Range(8, 8<<12);
BENCHMARK(BM_rangeSum_init_SparseTable)->Range(8, 8<<12);

BENCHMARK(BM_rangeSum_queryAll_PSA)->RangeMultiplier(2)->Range(1<<10, 1<<26)->Complexity(); // ->Range(1<<10, 1<<20);
BENCHMARK(BM_rangeSum_querySmall_PSA)->RangeMultiplier(2)->Range(1<<12, 1<<20)->Complexity(); // ->Range(1<<10, 1<<20);
BENCHMARK(BM_rangeSum_queryCacheMiss_PSA)->RangeMultiplier(2)->Range(1<<12, 1<<25)->Complexity(); // ->Range(1<<10, 1<<20);
BENCHMARK(BM_rangeSum_queryAll_SparseTable)->Range(1<<10, 1<<20);
BENCHMARK(BM_rangeSum_querySmall_SparseTable)->RangeMultiplier(2)->Range(1<<12, 1<<20)->Complexity(); // ->Range(1<<10, 1<<20);
