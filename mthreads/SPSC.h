#pragma once

#include <cinttypes>
#include <condition_variable>
#include <mutex>
#include <memory>
#include <optional>

// Taken from https://www.youtube.com/watch?v=K3P_Lmq6pw0
template <typename T, std::size_t Capacity, typename Alloc = std::allocator<T>>
class SPSCFifo final : private Alloc {
private:
    // using allocator_traits = typename std::allocator_traits<Alloc>::template rebind_traits<T>;
    using allocator_traits = typename std::allocator_traits<Alloc>;

    std::mutex mtx_;
    std::condition_variable cv_;

    T* ring_{nullptr};

    std::size_t pushCursor_{0};
    std::size_t popCursor_{0};

public:
    constexpr explicit SPSCFifo(const Alloc& alloc = Alloc{})
        : Alloc{alloc}, ring_{allocator_traits::allocate(*this, Capacity)} {}

    ~SPSCFifo() {
        while (!empty()) {
            allocator_traits::destroy(*this, &ring_[popCursor_ % Capacity]);
            ++popCursor_;
        }
        allocator_traits::deallocate(*this, ring_, Capacity);
    }

    // Delete the copy constructor, we don't want that.
    SPSCFifo(const SPSCFifo&) = delete;
    SPSCFifo& operator=(const SPSCFifo&) = delete;

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

    constexpr void push(const T& value) {
        std::unique_lock lk(mtx_);
        cv_.wait(lk, [this]{ return !this->full(); });

        allocator_traits::construct(*this, &ring_[pushCursor_ % Capacity], value);
        ++pushCursor_;

        lk.unlock();
        cv_.notify_one();
    }

    constexpr T pop() {
        std::unique_lock lk(mtx_);
        cv_.wait(lk, [this]{ return !this->empty(); });

        auto val = std::move(ring_[popCursor_ % Capacity]);
        allocator_traits::destroy(*this, &ring_[popCursor_ % Capacity]);
        ++popCursor_;

        lk.unlock();
        cv_.notify_one();
        return val;
    }


    // TODO(rHermes): Provide move constructor here also.
    [[nodiscard]] constexpr bool try_push(const T& value) {
        std::scoped_lock lk(mtx_);
        if (full())
            return false;

        allocator_traits::construct(*this, &ring_[pushCursor_ % Capacity], value);
        ++pushCursor_;

        return true;
    }

    [[nodiscard]] constexpr std::optional<T> try_pop() {
        std::scoped_lock lk(mtx_);
        if (empty())
            return std::nullopt;

        auto val = std::move(ring_[popCursor_ % Capacity]);
        allocator_traits::destroy(*this, &ring_[popCursor_ % Capacity]);
        ++popCursor_;

        return val;
    }

};
