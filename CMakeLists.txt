cmake_minimum_required(VERSION 3.28)
project(measure_everything)

include(FetchContent)

set(BENCHMARK_DOWNLOAD_DEPENDENCIES on)
FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG 654d8d6cf368233018c3df2f84f1118603839ac5
)


if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    #set(MI_TRACK_ASAN ON
    # set(MI_USE_CXX ON)
    # set(MI_TRACK_VALGRIND ON)
    # set(MI_DEBUG_FULL ON)
endif()

set(MI_OVERRIDE OFF)
FetchContent_Declare(
        mimalloc
        #GIT_REPOSITORY https://github.com/microsoft/mimalloc.git
        GIT_REPOSITORY https://github.com/rHermes/mimalloc.git
        #GIT_TAG dev
        GIT_TAG fix-passing-heap-v2
)

FetchContent_MakeAvailable(benchmark mimalloc)

set(CMAKE_CXX_STANDARD 20)

# I really don't know how to do this and I'm giving up. I hate cmake :(
#add_library(pcg INTERFACE)
#target_sources(pcg FILE_SET HEADERS TYPE HEADERS FILES third_party/pcg_extras.hpp third_party/pcg_uint128.hpp third_party/pcg_random.hpp)

add_executable(measure_everything main.cpp
        vector2d.h
        sparse-table.h
        vector_benches.cpp
        range_sum_benchmarks.cpp
        range_min_benchmarks.cpp
        third_party/pcg_extras.hpp third_party/pcg_uint128.hpp third_party/pcg_random.hpp
)

target_link_libraries(measure_everything benchmark::benchmark_main)

add_executable(mimalloc-stuff
        third_party/pcg_extras.hpp third_party/pcg_uint128.hpp third_party/pcg_random.hpp
        mimalloc-stuff/main.cpp
)
target_link_libraries(mimalloc-stuff mimalloc benchmark::benchmark_main)

add_executable(mthreads
        third_party/pcg_extras.hpp third_party/pcg_uint128.hpp third_party/pcg_random.hpp
        mthreads/main.cpp
        mthreads/SPSC.h
)

target_link_libraries(mthreads mimalloc)


if(MSVC)
    target_compile_options(mthreads PRIVATE /W4 /WX)
    target_compile_options(mimalloc-stuff PRIVATE /W4 /WX)
    target_compile_options(measure_everything PRIVATE /W4 /WX)
else()
    target_compile_options(mthreads PRIVATE -Wall -Wextra -Wpedantic -Werror -Wconversion -Wno-interference-size)
    target_compile_options(mimalloc-stuff PRIVATE -Wall -Wextra -Wpedantic -Werror -Wconversion -Wno-interference-size)
    target_compile_options(measure_everything PRIVATE -Wall -Wextra -Wpedantic -Werror -Wconversion -Wno-interference-size)

    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
       # target_compile_options(mthreads PRIVATE -fsanitize=address)
        #target_link_options(mthreads PRIVATE -fsanitize=address)

        target_compile_options(mthreads PRIVATE -fsanitize=thread)
        target_link_options(mthreads PRIVATE -fsanitize=thread)
        #set(MI_TRACK_ASAN ON
        # set(MI_USE_CXX ON)
        # set(MI_TRACK_VALGRIND ON)
        # set(MI_DEBUG_FULL ON)
    endif()
endif()

