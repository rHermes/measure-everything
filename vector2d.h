#pragma once

// This is a generic 2D vector, in a single vector. It will be removed once we get
// std::mdspan in c++23

#include <vector>

template <typename T>
class Vector2D {
private:
    std::size_t rows_{0};
    std::size_t cols_{0};
    std::vector<T> data_;

public:

    Vector2D(std::size_t rows, std::size_t cols) : rows_{rows}, cols_{cols}, data_(rows_*cols_) {}

    Vector2D(const Vector2D& other) :
            rows_{other.rows_} , cols_{other.cols_}
            , data_{other.data_}
    {}

    Vector2D(Vector2D&& other) noexcept :
            rows_{other.rows_} , cols_{other.cols_}
            , data_{std::move(other.data_)}
    {}

    Vector2D& operator=(const Vector2D& other) {
        *this = Vector2D(other);
        return *this;
    }

    Vector2D& operator=(Vector2D&& other) noexcept {
        rows_ = other.rows_;
        cols_ = other.cols_;
        data_ = std::move(other.data_);
        return *this;
    }

    [[nodiscard]] std::size_t rows() const {
        return rows_;
    }

    [[nodiscard]] std::size_t columns() const {
        return cols_;
    }

    [[nodiscard]] std::size_t idx(std::size_t row, std::size_t col) const {
        return row*cols_ + col;
    }

    [[nodiscard]] const T& get(std::size_t row, std::size_t col) const {
        return data_[idx(row, col)];
    }

    [[nodiscard]] T& get(std::size_t row, std::size_t col) {
        return data_[idx(row, col)];
    }

    [[nodiscard]] T* data() noexcept {
        return data_.data();
    }

    [[nodiscard]] const T* data() const {
        return data_.data();
    }
};