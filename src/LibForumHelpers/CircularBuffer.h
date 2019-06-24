/*
Fast Forum Backend
Copyright (C) 2016-present Daniel Jurcau

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <array>
#include <cstddef>
#include <iterator>
#include <numeric>

namespace Forum::Helpers
{
    template<typename T, size_t Capacity, typename IndexType = int8_t>
    class CircularBuffer final
    {
    public:
        void push_back(const T& value)
        {
            static_assert(std::is_integral_v<IndexType>);
            static_assert(std::is_signed_v<IndexType>);
            static_assert(static_cast<size_t>(std::numeric_limits<IndexType>::max()) >= Capacity);

            if (index_ >= 0)
            {
                items_[index_] = value;
                index_ += 1;
                if (Capacity == index_)
                {
                    index_ = -index_;
                }
            }
            else
            {
                items_[index_ + Capacity] = value;
                index_ += 1;
                if (0 == index_)
                {
                    index_ = -static_cast<IndexType>(Capacity);
                }
            }
        }
        
        class const_iterator final
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = const T*;
            using reference = const T&;

            const_iterator(const std::array<T, Capacity>& items, const size_t start, const size_t i)
                    : items_(items), start_(start), i_(i)
            { }
            ~const_iterator() = default;
            const_iterator(const const_iterator&) = default;
            const_iterator(const_iterator&&) = default;
            const_iterator& operator=(const const_iterator&) = default;
            const_iterator& operator=(const_iterator&&) = default;

            bool operator==(const const_iterator& other) const noexcept
            {
                return &items_ == &other.items_
                    && start_ == other.start_
                    && i_ == other.i_;
            }

            bool operator!=(const const_iterator& other) const noexcept
            {
                return !(*this == other);
            }

            reference operator*() const
            {
                return items_[(i_ + start_) % Capacity];
            }

            pointer operator->() const
            {
                return &operator*();
            }

            const_iterator& operator++()
            {
                ++i_;
                return *this;
            }

            const_iterator operator++(int)
            {
                auto copy = *this;
                return ++copy;
            }

        private:
            const std::array<T, Capacity>& items_;
            const size_t start_;
            size_t i_;
        };

        class const_reverse_iterator final
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = const T*;
            using reference = const T&;

            const_reverse_iterator(const std::array<T, Capacity>& items, const size_t start, const size_t i)
                : items_(items), start_(start), i_(i)
            { }
            ~const_reverse_iterator() = default;
            const_reverse_iterator(const const_reverse_iterator&) = default;
            const_reverse_iterator(const_reverse_iterator&&) = default;
            const_reverse_iterator& operator=(const const_reverse_iterator&) = default;
            const_reverse_iterator& operator=(const_reverse_iterator&&) = default;

            bool operator==(const const_reverse_iterator& other) const noexcept
            {
                return &items_ == &other.items_
                    && start_ == other.start_
                    && i_ == other.i_;
            }

            bool operator!=(const const_reverse_iterator& other) const noexcept
            {
                return !(*this == other);
            }

            reference operator*() const
            {
                return items_[(i_ - 1 + start_) % Capacity];
            }

            pointer operator->() const
            {
                return &operator*();
            }

            const_reverse_iterator& operator++()
            {
                --i_;
                return *this;
            }

            const_reverse_iterator operator++(int)
            {
                auto copy = *this;
                return ++copy;
            }

        private:
            const std::array<T, Capacity>& items_;
            const size_t start_;
            size_t i_;
        };

        const_iterator begin() const
        {
            return cbegin();
        }

        const_iterator cbegin() const
        {
            if (index_ >= 0)
            {
                return const_iterator(items_, 0, 0);
            }
            else
            {
                return const_iterator(items_, index_ + Capacity, 0);
            }
        }

        const_iterator end() const
        {
            return cend();
        }

        const_iterator cend() const
        {
            if (index_ >= 0)
            {
                return const_iterator(items_, 0, index_);
            }
            else
            {
                return const_iterator(items_, index_ + Capacity, Capacity);
            }
        }

        const_reverse_iterator rbegin() const
        {
            return crbegin();
        }

        const_reverse_iterator crbegin() const
        {
            if (index_ >= 0)
            {
                return const_reverse_iterator(items_, 0, index_);
            }
            else
            {
                return const_reverse_iterator(items_, index_ + Capacity, Capacity);
            }
        }

        const_reverse_iterator rend() const
        {
            return crend();
        }

        const_reverse_iterator crend() const
        {
            if (index_ >= 0)
            {
                return const_reverse_iterator(items_, 0, 0);
            }
            else
            {
                return const_reverse_iterator(items_, index_ + Capacity, 0);
            }
        }

    private:
        std::array<T, Capacity> items_{};
        IndexType index_{};
    };
}