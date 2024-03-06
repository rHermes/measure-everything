#pragma once

#include "vector2d.h"

#include <bit>
#include <stdexcept>

template <typename T, typename F, bool IDEMPOTENT = false>
class SparseTable {
    F func_{};
    const std::size_t maxN_;
    const std::size_t maxK_{static_cast<std::size_t>(std::bit_width(maxN_)-1)};

    Vector2D<T> data_{maxK_+1, maxN_+1};

public:

    explicit SparseTable(std::size_t maxN) : maxN_{maxN} {}
    SparseTable(F fn, std::size_t maxN) : func_{fn},  maxN_{maxN} {}

    template <typename IT>
    void precompute(IT first, IT last) {
        // This is a bit dirty, but we know the internals of the 2D data.
        std::copy(first, last, data_.data());

        for (std::size_t i = 1; i <= maxK_; i++) {
            for (std::size_t j = 0; j + (1 << i) <= maxN_; j++) {
                data_.get(i, j) = func_(
                        data_.get(i-1, j), // range [j, j + 2^(i-1) -1]
                        data_.get(i-1, j + (1 << (i -1))) // range [j + 2^(i-1), j + 2^i - 1]
                );
            }
        }
    }

    [[nodiscard]] T query(std::size_t l, const std::size_t r) const {
        // we assume that l < r
        if constexpr (IDEMPOTENT) {
            // as this operation is idempotent, there is no need to carefully
            // select the items, we just pick the two ranges that might overlap,
            // but which cover the whole array
            auto i = static_cast<std::size_t>(std::bit_width(r-l+1) - 1);
            return func_(data_.get(i, l), data_.get(i, r- (static_cast<std::size_t>(1) << i) + 1));
        } else {
            std::size_t i = maxK_+1;
            // We need the first hit here.
            for (; 0 < i; i--) {
                const auto ii = i-1;
                // If we can fit the size.
                if ((static_cast<std::size_t>(1) << ii) <= r - l + 1) {
                    break;
                }
            }

            if (i == 0)
                throw std::runtime_error("we couldn't find a query range?");

            T ans = data_.get(i-1, l);
            l += (1 << (i-1));

            // decrement ones, as we have already done this.
            i--;
            // Now we repeat the pattern, for the rest.
            for (; 0 < i; i--) {
                const auto ii = i-1;
                if ((static_cast<std::size_t>(1) << ii) <= r - l + 1) {
                    ans = func_(ans, data_.get(ii, l));
                    l += (1 << ii);
                }
            }

            return ans;
        }
    }
};
