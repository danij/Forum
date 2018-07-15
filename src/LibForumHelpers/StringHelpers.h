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

#include "TypeHelpers.h"
#include "JsonReadyString.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string>

namespace Forum::Helpers
{
    /**
     * Returns whether the character is the first byte in a sequence composing a UTF-8 encoded string
     */
    inline auto isFirstByteInUTF8Sequence(char c)
    {
        const auto s = static_cast<uint8_t>(c);
        return s < 0b10000000 || s >= 0b11000000;
    }

    /**
     * Returns the number of characters from a valid UTF-8 input
     */
    template<typename It>
    auto countUTF8Characters(It begin, It end)
    {
        return static_cast<int_fast32_t>(std::count_if(begin, end, isFirstByteInUTF8Sequence));
    }

    inline auto countUTF8Characters(StringView view)
    {
        return countUTF8Characters(view.begin(), view.end());
    }

    inline size_t nrOfBytesForUTF8Character(const char c)
    {
        const auto u = static_cast<uint8_t>(c);
        switch (u >> 4)
        {
        case 0b1110:
            return 3;
        case 0b1111:
            return 4;
        default:
            if ((u >> 5) == 0b110)
            {
                return 2;
            }
            return 1;
        }
    }

    inline auto getFirstCharacterInUTF8Array(StringView view)
    {
        if (view.empty())
        {
            return view;
        }
        return StringView(view.data(), std::min(nrOfBytesForUTF8Character(view[0]), view.size()));
    }

    inline auto getLastCharacterInUTF8Array(StringView view)
    {
        if (view.empty())
        {
            return view;
        }
        for (int i = static_cast<int>(view.size()) - 1; i >= 0; --i)
        {
            if (isFirstByteInUTF8Sequence(view[i]))
            {
                return StringView(view.data() + i, view.size() - i);
            }
        }
        return StringView{};
    }

    /**
     * Enables a deterministic release of all cached resources used by string helpers
     * Used before cleaning up ICU
     */
    void cleanupStringHelpers();

    inline std::string toString(StringView view)
    {
        return std::string(view.data(), view.size());
    }

    /**
     * Stores a string in a custom location in memory
     * The string only supports changing the whole content, not individual bytes
     * Providing a string_view to it is easier than defining an std::basic_string with a custom allocator
     */
    struct WholeChangeableString final
    {
        WholeChangeableString() : ptr_(nullptr), info_{ 0, 0 }
        {
        }

        ~WholeChangeableString()
        {
            if ((info_.ownsMemory != 0) && (ptr_ != nullptr))
            {
                delete[] ptr_;
            }
        }

        WholeChangeableString(const WholeChangeableString& other)
        {
            info_.ownsMemory = other.info_.ownsMemory;
            info_.size = other.info_.size;
            ptr_ = nullptr;

            if ((other.info_.ownsMemory != 0) && (ptr_ != nullptr))
            {
                auto ptr = new char[other.info_.size];
                ptr_ = ptr;
                memcpy(ptr, other.ptr_, other.info_.size);
            }
            else
            {
                ptr_ = other.ptr_;
            }
        }

        WholeChangeableString& operator=(const WholeChangeableString& other)
        {
            WholeChangeableString copy(other);
            swap(*this, copy);
            return *this;
        }

        WholeChangeableString(WholeChangeableString&& other) noexcept : ptr_(nullptr), info_{ 0, 0 }
        {
            swap(*this, other);
        }

        WholeChangeableString& operator=(WholeChangeableString&& other) noexcept
        {
            swap(*this, other);
            return *this;
        }

        static WholeChangeableString copyFrom(const StringView view)
        {
            return WholeChangeableString(view, true);
        }

        static WholeChangeableString onlyTakePointer(const StringView view)
        {
            return WholeChangeableString(view, false);
        }

        friend void swap(WholeChangeableString& first, WholeChangeableString& second) noexcept
        {
            using std::swap;
            swap(first.ptr_, second.ptr_);
            swap(first.info_, second.info_);
        }

        operator StringView() const
        {
            return std::string_view(ptr_, info_.size);
        }

    private:
        WholeChangeableString(StringView view, bool copy)
        {
            info_.ownsMemory = copy ? 1 : 0;
            info_.size = view.size();
            ptr_ = nullptr;

            if (copy)
            {
                if (info_.size > 0)
                {
                    auto ptr = new char[info_.size];
                    ptr_ = ptr;
                    memcpy(ptr, view.data(), view.size());
                }
                else
                {
                    ptr_ = nullptr;
                }
            }
            else
            {
                ptr_ = view.data();
            }
        }

        const char* ptr_;
        struct
        {
            uint32_t ownsMemory :  1;
            uint32_t size       : 31;
        } info_{};
    };

    namespace Detail
    {
        struct SizeWithBoolAndSortKeySize final
        {
            uint32_t boolean :  1;
            uint32_t size    : 31;
            uint32_t sortKeySize;

            SizeWithBoolAndSortKeySize() : boolean(0), size(0), sortKeySize(0) {}
            explicit SizeWithBoolAndSortKeySize(const size_t size)
                : boolean(0), size(static_cast<decltype(SizeWithBoolAndSortKeySize::size)>(size)), sortKeySize(0) {}
            ~SizeWithBoolAndSortKeySize() = default;

            SizeWithBoolAndSortKeySize(const SizeWithBoolAndSortKeySize&) = default;
            SizeWithBoolAndSortKeySize& operator=(const SizeWithBoolAndSortKeySize&) = default;

            explicit operator size_t() const noexcept
            {
                return static_cast<size_t>(size);
            }

            SizeWithBoolAndSortKeySize& operator=(const size_t value) noexcept
            {
                size = static_cast<decltype(size)>(value);
                return *this;
            }

            explicit operator bool() const noexcept
            {
                return boolean != 0;
            }

            SizeWithBoolAndSortKeySize& operator=(const bool value) noexcept
            {
                boolean = value ? 1 : 0;
                return *this;
            }

            void setExtraBytesNeeded(const size_t value) noexcept
            {
                sortKeySize = static_cast<decltype(sortKeySize)>(value);
            }
        };
    }

    size_t calculateSortKey(StringView view);
    char* getCurrentSortKey();
    size_t getCurrentSortKeyLength();

    template<size_t StackSize>
    class JsonReadyStringWithSortKey final
        : public Json::JsonReadyStringBase<StackSize, JsonReadyStringWithSortKey<StackSize>, Detail::SizeWithBoolAndSortKeySize>
    {
    public:
        explicit JsonReadyStringWithSortKey(StringView source);
        ~JsonReadyStringWithSortKey() = default;

        JsonReadyStringWithSortKey(const JsonReadyStringWithSortKey& other) = default;
        JsonReadyStringWithSortKey(JsonReadyStringWithSortKey&& other) noexcept = default;

        JsonReadyStringWithSortKey& operator=(const JsonReadyStringWithSortKey& other) = default;
        JsonReadyStringWithSortKey& operator=(JsonReadyStringWithSortKey&& other) noexcept = default;

        bool operator==(const JsonReadyStringWithSortKey& other) const;
        bool operator<(const JsonReadyStringWithSortKey& other) const;
        bool operator<=(const JsonReadyStringWithSortKey& other) const;
        bool operator>(const JsonReadyStringWithSortKey& other) const;
        bool operator>=(const JsonReadyStringWithSortKey& other) const;

        StringView sortKey() const noexcept;
        size_t getExtraSize() const noexcept;

        static size_t extraBytesNeeded(StringView source);
    };

    template <size_t StackSize>
    JsonReadyStringWithSortKey<StackSize>::JsonReadyStringWithSortKey(const StringView source)
        : Json::JsonReadyStringBase<StackSize, JsonReadyStringWithSortKey<StackSize>, Detail::SizeWithBoolAndSortKeySize>(source)
    {
        auto sortKeyStart = getCurrentSortKey();
        auto& sizeInfo = this->container_.size();
        sizeInfo.sortKeySize = static_cast<decltype(sizeInfo.sortKeySize)>(getCurrentSortKeyLength());

        std::copy(sortKeyStart, sortKeyStart + sizeInfo.sortKeySize,
                  *(this->container_) + sizeInfo.size - sizeInfo.sortKeySize);
    }

    template <size_t StackSize>
    bool JsonReadyStringWithSortKey<StackSize>::operator==(const JsonReadyStringWithSortKey& other) const
    {
        return std::equal(sortKey().begin(), sortKey().end(), other.sortKey().begin(), other.sortKey().end());
    }

    template <size_t StackSize>
    bool JsonReadyStringWithSortKey<StackSize>::operator<(const JsonReadyStringWithSortKey& other) const
    {
        return std::lexicographical_compare(sortKey().begin(), sortKey().end(), 
                                            other.sortKey().begin(), other.sortKey().end());
    }

    template <size_t StackSize>
    bool JsonReadyStringWithSortKey<StackSize>::operator<=(const JsonReadyStringWithSortKey& other) const
    {
        return ! (*this > other);
    }

    template <size_t StackSize>
    bool JsonReadyStringWithSortKey<StackSize>::operator>(const JsonReadyStringWithSortKey& other) const
    {
        return std::lexicographical_compare(other.sortKey().begin(), other.sortKey().end(),
                                            sortKey().begin(), sortKey().end());
    }

    template <size_t StackSize>
    bool JsonReadyStringWithSortKey<StackSize>::operator>=(const JsonReadyStringWithSortKey& other) const
    {
        return ! (*this < other);
    }

    template <size_t StackSize>
    StringView JsonReadyStringWithSortKey<StackSize>::sortKey() const noexcept
    {
        const auto& size = this->container_.size();
        assert(size.size >= size.sortKeySize);
        auto start = *(this->container_) + static_cast<size_t>(size.size - size.sortKeySize);
        return StringView(start, static_cast<size_t>(size.sortKeySize));
    }

    template <size_t StackSize>
    size_t JsonReadyStringWithSortKey<StackSize>::extraBytesNeeded(const StringView source)
    {
        const auto result = calculateSortKey(source);
        return static_cast<decltype(Detail::SizeWithBoolAndSortKeySize::sortKeySize)>(result);
    }

    template <size_t StackSize>
    size_t JsonReadyStringWithSortKey<StackSize>::getExtraSize() const noexcept
    {
        const auto& size = this->container_.size();
        return static_cast<size_t>(size.sortKeySize);
    }
}
