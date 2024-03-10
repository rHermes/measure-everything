#include <chrono>
#include <iostream>
#include <latch>
#include <thread>

#include "AtomicSPSC.h"
#include "MutexSPSC.h"

#include "../third_party/SPSCQueue.h"

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
std::size_t benchTrySemantics(T& fifo, const std::size_t N, const int sendCpu, const int recvCpu) {
    std::latch all{3};

    std::thread sender([&all, &fifo, N]() {
        all.arrive_and_wait();
        // std::cout << "Sender was started on thread: " << sched_getcpu() << std::endl;

        std::size_t i = 1;
        while (i <= N) {
            if (fifo.try_push(i)) {
                i++;
            }
        }

    });

    {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(sendCpu, &cpuset);
        const int rc = pthread_setaffinity_np(sender.native_handle(), sizeof(cpu_set_t), &cpuset);
        if (rc < 0) {
            std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
        }
    }

    std::thread receiver([&fifo, &all, N]() {
        all.arrive_and_wait();
        // std::cout << "Receiver was started on thread: " << sched_getcpu() << std::endl;

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

    {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(recvCpu, &cpuset);
        const int rc = pthread_setaffinity_np(receiver.native_handle(), sizeof(cpu_set_t), &cpuset);
        if (rc < 0) {
            std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
        }
    }

    const auto beginTS = std::chrono::steady_clock::now();
    all.arrive_and_wait();
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


    // std::cout << "The whole process took: " << diff << std::endl;
    auto thousands = std::make_unique<separate_thousands>();
    const auto prev = std::cout.imbue(std::locale(std::cout.getloc(), thousands.release()));
    std::cout << "We sent " << N << " elements in " << std::setprecision(3) << diff << " making it: " << perSecond << " per second" << std::endl;
    std::cout.imbue(prev);

    return perSecond;
}

template<typename T>
std::size_t benchWaitSemantics(T& fifo, const std::size_t N, const int sendCpu, const int recvCpu) {
    std::latch all{3};

    std::thread sender([&all, &fifo, N]() {
        all.arrive_and_wait();

        for (std::size_t i = 1; i <= N; i++)
            fifo.push(i);
    });

    {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(sendCpu, &cpuset);
        const int rc = pthread_setaffinity_np(sender.native_handle(), sizeof(cpu_set_t), &cpuset);
        if (rc < 0) {
            std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
        }
    }

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

    {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(recvCpu, &cpuset);
        const int rc = pthread_setaffinity_np(receiver.native_handle(), sizeof(cpu_set_t), &cpuset);
        if (rc < 0) {
            std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
        }
    }
    const auto beginTS = std::chrono::steady_clock::now();
    all.arrive_and_wait();
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


    // std::cout << "The whole process took: " <<  << std::endl;
    auto thousands = std::make_unique<separate_thousands>();
    auto prev = std::cout.imbue(std::locale(std::cout.getloc(), thousands.release()));
    std::cout << "We sent " << N << " elements in " << std::setprecision(3) << diff << " making it: " << perSecond << " per second" << std::endl;

    std::cout.imbue(prev);

    return perSecond;
}

template<>
std::size_t benchWaitSemantics<rigtorp::SPSCQueue<std::size_t>>(rigtorp::SPSCQueue<std::size_t>& fifo, const std::size_t N, const int sendCpu, const int recvCpu) {
    std::latch all{3};

    std::thread sender([&all, &fifo, N]() {
        all.arrive_and_wait();

        for (std::size_t i = 1; i <= N; i++)
            fifo.push(i);
    });

    {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(sendCpu, &cpuset);
        const int rc = pthread_setaffinity_np(sender.native_handle(), sizeof(cpu_set_t), &cpuset);
        if (rc < 0) {
            std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
        }
    }

    std::thread receiver([&fifo, &all, N]() {
        all.arrive_and_wait();
        std::size_t* res = nullptr;
        for (std::size_t i = 1; i <= N; i++) {
            res = fifo.front();
            while (res == nullptr)
                res = fifo.front();

            if (*res != i) {
                std::cout << "We expected: " << i << ", but we got " << *res << std::endl;
                throw std::runtime_error("Our two numbers are not as expected!");
            }

            fifo.pop();
        }
    });

    {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(recvCpu, &cpuset);
        const int rc = pthread_setaffinity_np(receiver.native_handle(), sizeof(cpu_set_t), &cpuset);
        if (rc < 0) {
            std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
        }
    }
    const auto beginTS = std::chrono::steady_clock::now();
    all.arrive_and_wait();
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


    // std::cout << "The whole process took: " <<  << std::endl;
    auto thousands = std::make_unique<separate_thousands>();
    auto prev = std::cout.imbue(std::locale(std::cout.getloc(), thousands.release()));
    std::cout << "We sent " << N << " elements in " << std::setprecision(3) << diff << " making it: " << perSecond << " per second" << std::endl;

    std::cout.imbue(prev);

    return perSecond;
}




template<typename T>
void testFifo(const std::string& name, const std::size_t N) {
    constexpr int times = 2;

    T fifo;
    /*
    std::cout << name << ": try semantics: same core" << std::endl;
    for (int i = 0; i < times; i++) {
        benchTrySemantics(fifo, N, 11, 23);
    }
    std::cout << name << ": try semantics: same l3" << std::endl;
    for (int i = 0; i < times; i++) {
        benchTrySemantics(fifo, N, 22, 23);
    }
    std::cout << name << ": try semantics: differnt" << std::endl;
    for (int i = 0; i < times; i++) {
        benchTrySemantics(fifo, N, 20, 23);
    }
    */

    std::cout << name << ": wait semantics: same core" << std::endl;
    for (int i = 0; i < times; i++) {
        benchWaitSemantics(fifo, N, 11, 23);
    }
    std::cout << name << ": wait semantics: same l3" << std::endl;
    for (int i = 0; i < times; i++) {
        benchWaitSemantics(fifo, N, 22, 23);
    }

    std::cout << name << ": wait semantics: differnt" << std::endl;
    for (int i = 0; i < times; i++) {
        benchWaitSemantics(fifo, N, 20, 23);
    }
}


void testBestFifo(const std::string& name, const std::size_t N) {
    constexpr int times = 2;

    rigtorp::SPSCQueue<std::size_t> fifo(512);

    /*
    std::cout << name << ": try semantics: same core" << std::endl;
    for (int i = 0; i < times; i++) {
        benchTrySemantics(fifo, N, 11, 23);
    }
    std::cout << name << ": try semantics: same l3" << std::endl;
    for (int i = 0; i < times; i++) {
        benchTrySemantics(fifo, N, 22, 23);
    }
    std::cout << name << ": try semantics: differnt" << std::endl;
    for (int i = 0; i < times; i++) {
        benchTrySemantics(fifo, N, 20, 23);
    }
    */

    std::cout << name << ": wait semantics: same core" << std::endl;
    for (int i = 0; i < times; i++) {
        benchWaitSemantics(fifo, N, 11, 23);
    }
    std::cout << name << ": wait semantics: same l3" << std::endl;
    for (int i = 0; i < times; i++) {
        benchWaitSemantics(fifo, N, 22, 23);
    }

    std::cout << name << ": wait semantics: differnt" << std::endl;
    for (int i = 0; i < times; i++) {
        benchWaitSemantics(fifo, N, 20, 23);
    }
}

int main(int, char**) {
    // preFlight<SPSCFifo<std::size_t, 512>();
    constexpr std::size_t N = 100'000'000;
    // constexpr std::size_t N = 500'000;

    std::cout << "The padding on our atomic is: " << AtomicSPSCFifo<std::size_t, 2048>::padding << std::endl;
    // testMutex(N);
    // testFifo<MutexSPSCFifo<std::size_t, 512>>("mutex", N);
    // std::cout << "\n\nSPACE\n\n" << std::endl;
    testFifo<AtomicSPSCFifo<std::size_t, 512>>("atomic", N);
    testBestFifo("best", N);




    return 0;
}