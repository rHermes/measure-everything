#pragma once

#include <condition_variable>
#include <mutex>
#include <memory>
#include <optional>

#define AT_USE_ALIGNED

// Taken from https://www.youtube.com/watch?v=K3P_Lmq6pw0
template <typename T, std::size_t Capacity, typename Alloc = std::allocator<T>>
class AtomicSPSCFifo final : private Alloc {
private:
    static_assert(Capacity && ((Capacity & (Capacity - 1)) == 0),
        "As we perform many modulo operations, it's important that a power of 2 is used, "
        "so that they can be transformed to bitmasks.");

    static_assert(8 <= sizeof(std::size_t), "The FIFO relies on std::size_t not wrapping around, "
        "which is not a safe assumption on 32bit hardware.");

    using allocator_traits = typename std::allocator_traits<Alloc>::template rebind_traits<T>;

    T* ring_{nullptr};

#ifdef AT_USE_ALIGNED
    alignas(std::hardware_destructive_interference_size) std::atomic<std::size_t> pushCursor_{0};
    std::size_t cachedPopCursor_{0};

    alignas(std::hardware_destructive_interference_size) std::atomic<std::size_t> popCursor_{0};
    std::size_t cachedPushCursor_{0};

#else
    std::atomic<std::size_t> pushCursor_{0};
    std::atomic<std::size_t> popCursor_{0};

    std::size_t cachedPushCursor_{0};
    std::size_t cachedPopCursor_{0};
#endif

    static_assert(decltype(pushCursor_)::is_always_lock_free, "We require std::atomic<std::size_t> to be lockfree");
    static_assert(decltype(popCursor_)::is_always_lock_free, "We require std::atomic<std::size_t> to be lockfree");

    [[nodiscard]] static constexpr bool full(const std::size_t popCursor, const std::size_t pushCursor) {
        return (pushCursor - popCursor) == Capacity;
    }

    [[nodiscard]] static constexpr bool empty(const std::size_t popCursor, const std::size_t pushCursor) {
        return popCursor == pushCursor;
    }

    [[nodiscard]] constexpr std::size_t& element(const std::size_t cursor) {
        return ring_[cursor % Capacity];
    }

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
        // TODO(rHermes): Figure out how to give the size in an atomic manner.
        return pushCursor_ - popCursor_;
    }

    [[nodiscard]] constexpr bool empty() const {
        return size() == 0;
    }

    [[nodiscard]] constexpr bool full() const {
        return size() == Capacity;
    }

    constexpr void push_futex(const T& value) {
        // we are fighting with noone over this
        const auto curPush = pushCursor_.load(std::memory_order::relaxed);
        if (full(cachedPopCursor_, curPush)) {
            popCursor_.wait(cachedPopCursor_, std::memory_order::acquire);
            cachedPopCursor_= popCursor_.load(std::memory_order::acquire);
        }

        allocator_traits::construct(*this, &element(curPush), value);

        pushCursor_.store(curPush + 1, std::memory_order::release);
        pushCursor_.notify_one();
    }

    constexpr T pop_futex() {
        const auto curPop = popCursor_.load(std::memory_order::relaxed);
        if (empty(curPop, cachedPushCursor_)) {
            pushCursor_.wait(cachedPushCursor_, std::memory_order::acquire);
            cachedPushCursor_ = pushCursor_.load(std::memory_order::acquire);
        }

        auto val = std::move(element(curPop));
        allocator_traits::destroy(*this, &element(curPop));

        popCursor_.store(curPop + 1, std::memory_order::release);
        popCursor_.notify_one();

        return val;
    }

    constexpr void push(const T& value) {
        // we are fighting with noone over this
        const auto curPush = pushCursor_.load(std::memory_order::relaxed);
        while (full(cachedPopCursor_, curPush))
            cachedPopCursor_ = popCursor_.load(std::memory_order::acquire);

        allocator_traits::construct(*this, &element(curPush), value);

        pushCursor_.store(curPush + 1, std::memory_order::release);
    }

    constexpr T pop() {
        const auto curPop = popCursor_.load(std::memory_order::relaxed);


        while (empty(curPop, cachedPushCursor_))
            cachedPushCursor_ = pushCursor_.load(std::memory_order::acquire);

        auto val = std::move(element(curPop));
        allocator_traits::destroy(*this, &element(curPop));

        popCursor_.store(curPop + 1, std::memory_order::release);
        return val;
    }

    
    [[nodiscard]] constexpr bool try_push(const T& value) {
        // This is not what we need to optimize for.
        const auto curPush = pushCursor_.load(std::memory_order::relaxed);
        if (full(cachedPopCursor_, curPush)) {
            cachedPopCursor_ = popCursor_.load(std::memory_order::acquire);
            if (full(cachedPopCursor_, curPush))
                return false;
        }

        allocator_traits::construct(*this, &element(curPush), value);

        pushCursor_.store(curPush+1, std::memory_order::release);
        return true;
    }

    [[nodiscard]] constexpr std::optional<T> try_pop() {
        const auto curPop = popCursor_.load(std::memory_order::relaxed);
        if (empty(curPop, cachedPushCursor_)) {
            cachedPushCursor_ = pushCursor_.load(std::memory_order::acquire);
            if (empty(curPop, cachedPushCursor_))
                return std::nullopt;
        }

        // TODO(rHermes): Try to make sure this is actually using the move constructor later.
        auto val = std::move(element(curPop));
        allocator_traits::destroy(*this, &element(curPop));

        popCursor_.store(curPop+1, std::memory_order::release);
        return val;
    }

};