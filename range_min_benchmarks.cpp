#include <benchmark/benchmark.h>

#include "sparse-table.h"
#include <cinttypes>
#include <random>
#include <set>

static void BM_rangeMin_query_SparseTable(benchmark::State& state) {
    const auto maxN = static_cast<std::size_t>(state.range(0));

    std::multiset<int> wow;
    using T = std::int64_t;

    std::mt19937 gen(10);
    std::uniform_int_distribution<T> vals(1, 10000);

    auto f = [](const T a, const T b) { return std::min(a, b); };
    SparseTable<T, decltype(f), true> st(maxN);


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



BENCHMARK(BM_rangeMin_query_SparseTable)->RangeMultiplier(2)->Range(1<<10, 1<<20)->Complexity();
