#pragma once

#include <cinttypes>
#include <condition_variable>
#include <mutex>
#include <memory>
#include <optional>

// Taken from https://www.youtube.com/watch?v=K3P_Lmq6pw0
template <typename T, std::size_t Capacity, typename Alloc = std::allocator<T>>
class AtomicSPSCFifo final : private Alloc {
private:
    using allocator_traits = typename std::allocator_traits<Alloc>::template rebind_traits<T>;

    T* ring_{nullptr};

#ifdef WOW
    alignas(std::hardware_destructive_interference_size) std::atomic<std::size_t> pushCursor_{0};
    alignas(std::hardware_destructive_interference_size) std::atomic<std::size_t> popCursor_{0};

    char padding_[std::hardware_destructive_interference_size - sizeof(std::size_t)];
#else
    std::atomic<std::size_t> pushCursor_{0};
    std::atomic<std::size_t> popCursor_{0};
#endif

    static_assert(decltype(pushCursor_)::is_always_lock_free, "We require std::atomic<std::size_t> to be lockfree");
    static_assert(decltype(popCursor_)::is_always_lock_free, "We require std::atomic<std::size_t> to be lockfree");

public:
    constexpr explicit AtomicSPSCFifo(const Alloc& alloc = Alloc{})
        : Alloc{alloc}, ring_{allocator_traits::allocate(*this, Capacity)} {}

    ~AtomicSPSCFifo() {
        while (!empty()) {
            allocator_traits::destroy(*this, &ring_[popCursor_ % Capacity]);
            ++popCursor_;
        }
        allocator_traits::deallocate(*this, ring_, Capacity);
    }

    // Delete the copy constructor, we don't want that.
    AtomicSPSCFifo(const AtomicSPSCFifo&) = delete;
    AtomicSPSCFifo& operator=(const AtomicSPSCFifo&) = delete;

    [[nodiscard]] constexpr auto capacity() const {
        return Capacity;
    }

    [[nodiscard]] constexpr auto size() const {
        return pushCursor_ - popCursor_;
    }

    [[nodiscard]] constexpr bool empty() const {
        return size() == 0;
    }

    [[nodiscard]] constexpr bool full() const {
        return size() == Capacity;
    }

    /*
    constexpr void push(const T& value) {
        std::unique_lock lk(mtx_);
        cv_.wait(lk, [this]{ return !this->noLockFull(); });

        allocator_traits::construct(*this, &ring_[pushCursor_ % Capacity], value);
        ++pushCursor_;

        lk.unlock();
        cv_.notify_one();
    }

    constexpr T pop() {
        std::unique_lock lk(mtx_);
        cv_.wait(lk, [this]{ return !this->noLockEmpty(); });

        auto val = std::move(ring_[popCursor_ % Capacity]);
        allocator_traits::destroy(*this, &ring_[popCursor_ % Capacity]);
        ++popCursor_;

        lk.unlock();
        cv_.notify_one();
        return val;
    }
    */
    
    [[nodiscard]] constexpr bool try_push(const T& value) {
        const auto curPop = popCursor_.load();
        const auto curPush = pushCursor_.load();
        if (Capacity <= (curPush - curPop))
            return false;

        allocator_traits::construct(*this, &ring_[curPush % Capacity], value);
        ++pushCursor_;

        return true;
    }

    [[nodiscard]] constexpr std::optional<T> try_pop() {
        const auto curPush = pushCursor_.load();
        const auto curPop = popCursor_.load();
        if (curPush == curPop)
            return std::nullopt;

        auto val = std::move(ring_[curPop % Capacity]);
        allocator_traits::destroy(*this, &ring_[curPop % Capacity]);

        ++popCursor_;
        return val;
    }

};