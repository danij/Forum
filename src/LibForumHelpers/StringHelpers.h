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

namespace Forum
{
    namespace Helpers
    {
        /**
         * Returns whether the character is the first byte in a sequence composing a UTF-8 encoded string
         */
        inline auto isFirstByteInUTF8Sequence(char c)
        {
            auto s = static_cast<uint8_t>(c);
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

        inline size_t nrOfBytesForUTF8Character(char c)
        {
            auto u = static_cast<uint8_t>(c);
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
            if (view.size() < 1)
            {
                return view;
            }
            return StringView(view.data(), std::min(nrOfBytesForUTF8Character(view[0]), view.size()));
        }

        inline auto getLastCharacterInUTF8Array(StringView view)
        {
            if (view.size() < 1)
            {
                return view;
            }
            for (int i = view.size() - 1; i >= 0; --i)
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
         * Stores an immutable string in a custom location in memory
         * Providing a string_view to it is easier than defining an std::basic_string with a custom allocator
         */
        struct ImmutableString final
        {
            ImmutableString() = default;
            ~ImmutableString() = default;

            ImmutableString(const ImmutableString&) = delete;
            ImmutableString& operator=(const ImmutableString&) = delete;

            ImmutableString(ImmutableString&&) = default;
            ImmutableString& operator=(ImmutableString&&) = default;


            ImmutableString(StringView view)
            {
                copyFrom(view);
            }

            ImmutableString& operator=(StringView view)
            {
                copyFrom(view);
                return *this;
            }

            operator StringView() const
            {
                return boost::string_view(ptr_.get(), size_);
            }

        private:

            void copyFrom(StringView view)
            {
                if (view.size())
                {
                    ptr_ = std::make_unique<char[]>(view.size());
                    std::copy(view.begin(), view.end(), ptr_.get());
                    size_ = view.size();
                }
            }

            std::unique_ptr<char[]> ptr_;
            size_t size_ = 0;
        };

        namespace Detail
        {
            struct SizeWithBoolAndSortKeySize
            {
                uint32_t boolean : 1;
                uint32_t size : 31;
                uint32_t sortKeySize;

                SizeWithBoolAndSortKeySize() : boolean(0), size(0), sortKeySize(0) {}
                SizeWithBoolAndSortKeySize(size_t size) : boolean(0), size(static_cast<decltype(size)>(size)), sortKeySize(0) {}

                SizeWithBoolAndSortKeySize(const SizeWithBoolAndSortKeySize&) = default;
                SizeWithBoolAndSortKeySize& operator=(const SizeWithBoolAndSortKeySize&) = default;

                explicit operator size_t() const noexcept
                {
                    return static_cast<size_t>(size);
                }

                SizeWithBoolAndSortKeySize& operator=(size_t value) noexcept
                {
                    size = static_cast<decltype(size)>(value);
                    return *this;
                }

                explicit operator bool() const noexcept
                {
                    return boolean != 0;
                }

                SizeWithBoolAndSortKeySize& operator=(bool value) noexcept
                {
                    boolean = value ? 1 : 0;
                    return *this;
                }

                void setExtraBytesNeeded(size_t value) noexcept
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
        JsonReadyStringWithSortKey<StackSize>::JsonReadyStringWithSortKey(StringView source)
            : Json::JsonReadyStringBase<StackSize, JsonReadyStringWithSortKey<StackSize>, Detail::SizeWithBoolAndSortKeySize>(source)
        {
            auto sortKeyStart = getCurrentSortKey();
            auto& sizeInfo = this->container_.size();
            sizeInfo.sortKeySize = getCurrentSortKeyLength();

            std::copy(sortKeyStart, sortKeyStart + sizeInfo.sortKeySize, 
                      *(this->container_) + sizeInfo.size - sizeInfo.sortKeySize);
        }

        template <size_t StackSize>
        bool JsonReadyStringWithSortKey<StackSize>::operator==(const JsonReadyStringWithSortKey& other) const
        {
            return strcmp(sortKey().data(), other.sortKey().data()) == 0;
        }

        template <size_t StackSize>
        bool JsonReadyStringWithSortKey<StackSize>::operator<(const JsonReadyStringWithSortKey& other) const
        {
            return strcmp(sortKey().data(), other.sortKey().data()) < 0;
        }

        template <size_t StackSize>
        bool JsonReadyStringWithSortKey<StackSize>::operator<=(const JsonReadyStringWithSortKey& other) const
        {
            return ! (*this > other);
        }

        template <size_t StackSize>
        bool JsonReadyStringWithSortKey<StackSize>::operator>(const JsonReadyStringWithSortKey& other) const
        {
            return strcmp(sortKey().data(), other.sortKey().data()) > 0;
        }

        template <size_t StackSize>
        bool JsonReadyStringWithSortKey<StackSize>::operator>=(const JsonReadyStringWithSortKey& other) const
        {
            return ! (*this < other);
        }

        template <size_t StackSize>
        StringView JsonReadyStringWithSortKey<StackSize>::sortKey() const noexcept
        {
            auto& size = this->container_.size();
            assert(size.size >= size.sortKeySize);
            auto start = *(this->container_) + static_cast<size_t>(size.size - size.sortKeySize);
            return StringView(start, static_cast<size_t>(size.sortKeySize));
        }

        template <size_t StackSize>
        size_t JsonReadyStringWithSortKey<StackSize>::extraBytesNeeded(StringView source)
        {
            auto result = calculateSortKey(source);
            return static_cast<decltype(Detail::SizeWithBoolAndSortKeySize::sortKeySize)>(result);
        }

        template <size_t StackSize>
        size_t JsonReadyStringWithSortKey<StackSize>::getExtraSize() const noexcept
        {
            auto& size = this->container_.size();
            return static_cast<size_t>(size.sortKeySize);
        }
    }
}
