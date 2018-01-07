#pragma once

#include <algorithm>
#include <functional>
#include <vector>

namespace Forum
{
    namespace Entities
    {
        namespace Detail
        {
            template<typename T, typename Key, typename KeyExtractor, typename Compare>
            class SortedVectorBase
            {
            public:
                SortedVectorBase() = default;
                SortedVectorBase(const SortedVectorBase&) = default;
                SortedVectorBase(SortedVectorBase&&) = default;
                virtual ~SortedVectorBase() = default;
                SortedVectorBase& operator=(const SortedVectorBase&) = default;
                SortedVectorBase& operator=(SortedVectorBase&&) = default;

                using value_type = typename std::vector<T>::value_type;
                using size_type = typename std::vector<T>::size_type;
                using iterator = typename std::vector<T>::const_iterator;

                auto size() const noexcept
                {
                    return vector_.size();
                }

                auto begin() const noexcept
                {
                    return cbegin();
                }
                auto cbegin() const noexcept
                {
                    return vector_.cbegin();
                }
                auto rbegin() const noexcept
                {
                    return crbegin();
                }
                auto crbegin() const noexcept
                {
                    return vector_.crbegin();
                }

                auto end() const noexcept
                {
                    return cend();
                }
                auto cend() const noexcept
                {
                    return vector_.cend();
                }
                auto rend() const noexcept
                {
                    return crend();
                }
                auto crend() const noexcept
                {
                    return vector_.crend();
                }

                auto lower_bound(const T& value) const noexcept
                {
                    return lower_bound(KeyExtractor{}(value));
                }
                auto lower_bound(const Key& key) const noexcept
                {
                    return std::lower_bound(vector_.cbegin(), vector_.cend(), key,
                        [](auto&& first, auto&& second)
                        {
                            return Compare{}(first, second);
                        });
                }
                auto lower_bound_rank(const T& value) const noexcept
                {
                    return lower_bound(value) - cbegin();
                }
                auto lower_bound_rank(const Key& key) const noexcept
                {
                    return lower_bound(key) - cbegin();
                }

                auto upper_bound(const T& value) const noexcept
                {
                    return upper_bound(KeyExtractor{}(value));
                }
                auto upper_bound(const Key& key) const noexcept
                {
                    return std::upper_bound(vector_.cbegin(), vector_.cend(), key,
                        [](auto&& first, auto&& second)
                        {
                            return Compare{}(first, second);
                        });
                }

                auto equal_range(const T& value) const noexcept
                {
                    return equal_range(KeyExtractor{}(value));
                }
                auto equal_range(const Key& key) const noexcept
                {
                    return std::equal_range(vector_.cbegin(), vector_.cend(), key,
                        [](auto&& first, auto&& second)
                        {
                            return Compare{}(first, second);
                        });
                }

                auto nth(size_type value) const noexcept
                {
                    value = std::min(value, size());
                    return cbegin() + value;
                }

                auto index_of(iterator position) const noexcept
                {
                    return position - begin();
                }

                void clear()
                {
                    return vector_.clear();
                }

                auto erase(iterator position)
                {
                    return vector_.erase(position);
                }

                auto erase(iterator first, iterator last)
                {
                    return vector_.erase(first, last);
                }

                virtual iterator replace(iterator position, T value)
                {
                    auto first = vector_.begin();
                    auto last = first + (vector_.size() - 1);

                    //get a mutable iterator from the const one
                    auto it = first + (position - first);

                    std::swap(*it, value);

                    bool replacedToTheLeft = false;
                    //try to replace to the left
                    while ((it > first) && Compare{}(*it, *(it - 1)))
                    {
                        std::iter_swap(it, it - 1);
                        it -= 1;
                        replacedToTheLeft = true;
                    }
                    if (replacedToTheLeft)
                    {
                        return it;
                    }
                    //try to replace to the right
                    while ((it < last) && Compare{}(*(it + 1), *it))
                    {
                        std::iter_swap(it, it + 1);
                        it += 1;
                    }
                    return it;
                }

            protected:
                std::vector<T> vector_;
            };
        }

        template<typename T, typename Key, typename KeyExtractor, typename Compare>
        class SortedVectorMultiValue : public Detail::SortedVectorBase<T, Key, KeyExtractor, Compare>
        {
        public:
            auto insert(T value)
            {
                auto position = this->upper_bound(value);
                return this->vector_.insert(position, std::move(value));
            }

            template<typename It>
            void insert(It begin, It end)
            {
                this->vector_.insert(this->vector_.end(), begin, end);
                std::sort(this->vector_.begin(), this->vector_.end(),
                    [](auto&& first, auto&& second)
                    {
                        return Compare{}(first, second);
                    });
            }
        };

        template<typename T, typename Key, typename KeyExtractor, typename Compare>
        class SortedVectorUnique : public Detail::SortedVectorBase<T, Key, KeyExtractor, Compare>
        {
        public:
            using iterator = typename Detail::SortedVectorBase<T, Key, KeyExtractor, Compare>::iterator;

            std::pair<typename std::vector<T>::const_iterator, bool> insert(T value)
            {
                auto range = this->equal_range(value);
                if (range.first != range.second)
                {
                    return std::make_pair(range.first, false);
                }
                return std::make_pair(this->vector_.insert(range.first, std::move(value)), true);
            }

            auto find(const T& value) const
            {
                return find(KeyExtractor{}(value));
            }
            auto find(const Key& key) const
            {
                auto range = this->equal_range(key);
                return (range.first == range.second) ? this->end() : range.first;
            }

            iterator replace(iterator position, T value) override
            {
                if (find(value) != this->end())
                {
                    //the new value already exists, so remove the item that cannot be replaced
                    return this->erase(position);
                }

                return Detail::SortedVectorBase<T, Key, KeyExtractor, Compare>::replace(position, std::move(value));
            }
        };
    }
}
