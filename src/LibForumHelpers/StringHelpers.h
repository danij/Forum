#pragma once

#include "TypeHelpers.h"

#include <algorithm>
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

        struct StringWithSortKey final
        {
            StringWithSortKey() noexcept;
            StringWithSortKey(StringView view);

            StringWithSortKey(const StringWithSortKey& other);
            StringWithSortKey(StringWithSortKey&& other) noexcept;

            StringWithSortKey& operator=(const StringWithSortKey& other);
            StringWithSortKey& operator=(StringWithSortKey&& other) noexcept;

            bool operator==(const StringWithSortKey& other) const;
            bool operator<(const StringWithSortKey& other) const;
            bool operator<=(const StringWithSortKey& other) const;
            bool operator>(const StringWithSortKey& other) const;
            bool operator>=(const StringWithSortKey& other) const;

            friend void swap(StringWithSortKey& first, StringWithSortKey& second) noexcept;
            friend std::ostream& operator<<(std::ostream& stream, const StringWithSortKey& string);

            StringView string() const;
            StringView sortKey() const;

        private:
            std::unique_ptr<char[]> bytes_;
            size_t stringLength_;
            size_t sortKeyLength_;
        };

        void swap(StringWithSortKey& first, StringWithSortKey& second) noexcept;
        std::ostream& operator<<(std::ostream& stream, const StringWithSortKey& string);
        
        inline std::string toString(const StringWithSortKey& value)
        {
            return toString(value.string());
        }
    }
}
