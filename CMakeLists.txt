cmake_minimum_required(VERSION 3.28)
project(measure_everything)

include(FetchContent)

set(BENCHMARK_DOWNLOAD_DEPENDENCIES on)
FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG 654d8d6cf368233018c3df2f84f1118603839ac5
)
FetchContent_MakeAvailable(benchmark)



set(CMAKE_CXX_STANDARD 20)

add_executable(measure_everything main.cpp
        vector2d.h
        sparse-table.h
        vector_benches.cpp
        range_sum_benchmarks.cpp
        range_min_benchmarks.cpp)

target_link_libraries(measure_everything benchmark::benchmark_main)