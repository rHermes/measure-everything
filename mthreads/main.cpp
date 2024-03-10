#include <chrono>
#include <iostream>
#include <latch>
#include <thread>

#include "SPSC.h"

struct separate_thousands : std::numpunct<char> {
    char_type do_thousands_sep() const override { return ','; }  // separate with commas
    string_type do_grouping() const override { return "\3"; } // groups of 3 digit
};


template <typename T>
void preFlight() {
    T fifo;

    if (!fifo.try_push(10))
        throw std::runtime_error("We couldn't pop");

    if (fifo.size() != 1)
        throw std::runtime_error("This has to be 1");

    if (auto k = fifo.try_pop(); !k || *k != 10)
        throw std::runtime_error("We didn't get 10 when we poped!");

    if (fifo.size() != 0)
        throw std::runtime_error("This has to be 1");

    for (std::size_t i = 0; i < fifo.capacity(); i++) {
        if(!fifo.try_push(i))
            throw std::runtime_error("We failed to push onto the queue");
    }

    if (!fifo.full()) {
        throw std::runtime_error("The fifo has to be full at this point");
    }

    if (fifo.try_push(10))
        throw std::runtime_error("We are supposed to not be able to push onto a full queue!");

    for (std::size_t i = 0; i < fifo.capacity(); i++) {
        auto res = fifo.try_pop();
        if (!res || *res != i)
            throw std::runtime_error("We couldn't pop the fifo!");
    }

    if (fifo.size() != 0)
        throw std::runtime_error("This has to be 1");


    std::cout << "passed in flight tests!" << std::endl;
}

template<typename T>
std::size_t benchTrySemantics(T& fifo, const std::size_t N) {
    std::latch all{3};

    std::thread sender([&all, &fifo, N]() {
        all.arrive_and_wait();

        std::size_t i = 1;
        while (i <= N) {
            if (fifo.try_push(i)) {
                i++;
            }
        }

    });

    std::thread receiver([&fifo, &all, N]() {
        all.arrive_and_wait();

        std::size_t i = 1;
        while (i <= N) {
            auto res = fifo.try_pop();
            if (res) {
                if (*res != i) {
                    std::cout << "We expected: " << i << ", but we got " << *res << std::endl;
                    throw std::runtime_error("Our two numbers are not as expected!");
                }
                i++;
            }
        }
    });

    all.arrive_and_wait();
    const auto beginTS = std::chrono::steady_clock::now();
    // std::cout << "We started the sending!" << std::endl;
    sender.join();
    receiver.join();
    const auto endTS = std::chrono::steady_clock::now();

    // We now assert that the fifo is empty
    if (!fifo.empty()) {
        throw std::runtime_error("FIFO was not empty at the end of the run!");
    }

    const std::chrono::duration<double> diff = endTS - beginTS;

    const auto perSecond = static_cast<std::size_t>(static_cast<double>(N) / diff.count());


    std::cout << "The whole process took: " << diff << std::endl;
    auto thousands = std::make_unique<separate_thousands>();
    auto prev = std::cout.imbue(std::locale(std::cout.getloc(), thousands.release()));
    std::cout << "We sent " << N << " elements making it: " << perSecond << " per second" << std::endl;
    std::cout.imbue(prev);

    return perSecond;
}

template<typename T>
std::size_t benchWaitSemantics(T& fifo, const std::size_t N) {
    std::latch all{3};

    std::thread sender([&all, &fifo, N]() {
        all.arrive_and_wait();

        for (std::size_t i = 1; i <= N; i++)
            fifo.push(i);
    });

    std::thread receiver([&fifo, &all, N]() {
        all.arrive_and_wait();
        for (std::size_t i = 1; i <= N; i++) {
            auto res = fifo.pop();
            if (res != i) {
                    std::cout << "We expected: " << i << ", but we got " << res << std::endl;
                    throw std::runtime_error("Our two numbers are not as expected!");
            }
        }
    });

    all.arrive_and_wait();
    const auto beginTS = std::chrono::steady_clock::now();
    // std::cout << "We started the sending!" << std::endl;
    sender.join();
    receiver.join();
    const auto endTS = std::chrono::steady_clock::now();

    // We now assert that the fifo is empty
    if (!fifo.empty()) {
        throw std::runtime_error("FIFO was not empty at the end of the run!");
    }

    const std::chrono::duration<double> diff = endTS - beginTS;

    const auto perSecond = static_cast<std::size_t>(static_cast<double>(N) / diff.count());


    std::cout << "The whole process took: " << diff << std::endl;
    auto thousands = std::make_unique<separate_thousands>();
    auto prev = std::cout.imbue(std::locale(std::cout.getloc(), thousands.release()));
    std::cout << "We sent " << N << " elements making it: " << perSecond << " per second" << std::endl;
    std::cout.imbue(prev);

    return perSecond;
}


int main() {
    // preFlight<SPSCFifo<std::size_t, 512>();
    SPSCFifo<std::size_t, 512> fifo;
    constexpr std::size_t N = 30'000'000;

    benchTrySemantics(fifo, N);
    benchWaitSemantics(fifo, N);

    return 0;
}