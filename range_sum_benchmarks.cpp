#include <benchmark/benchmark.h>

#include "sparse-table.h"
#include <cinttypes>
#include <random>

static void BM_rangeSum_init_PSA(benchmark::State& state) {
    const auto maxN = state.range(0);

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

static void BM_rangeSum_query_PSA(benchmark::State& state) {
    const auto maxN = state.range(0);

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


static void BM_rangeSum_init_SparseTable(benchmark::State& state) {
    const auto maxN = state.range(0);

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

static void BM_rangeSum_query_SparseTable(benchmark::State& state) {
    const auto maxN = state.range(0);

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


BENCHMARK(BM_rangeSum_init_PSA)->Range(8, 8<<12);
BENCHMARK(BM_rangeSum_init_SparseTable)->Range(8, 8<<12);

BENCHMARK(BM_rangeSum_query_PSA)->Range(1<<10, 1<<20);
BENCHMARK(BM_rangeSum_query_SparseTable)->Range(1<<10, 1<<20);
