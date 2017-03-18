#pragma once

#include "TypeHelpers.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>

namespace Forum
{
    namespace Helpers
    {
        /**
         * Returns the number of characters from a valid UTF-8 input
         */
        template<typename It>
        auto countUTF8Characters(It begin, It end)
        {
            return static_cast<int_fast32_t>(std::count_if(begin, end, [](auto c)
            {
                auto v = static_cast<uint8_t>(c);
                return v < 0b10000000 || v >= 0b11000000;
            }));
        }

        inline auto countUTF8Characters(StringView view)
        {
            return countUTF8Characters(view.begin(), view.end());
        }

        /**
         * Enables a deterministic release of all cached resources used by string helpers
         * Used before cleaning up ICU
         */
        void cleanupStringHelpers();

        struct StringAccentAndCaseInsensitiveLess
        {
            StringAccentAndCaseInsensitiveLess();
            bool operator()(const std::string& lhs, const std::string& rhs) const;
        };
        
        inline std::string toString(const StringView& view)
        {
            return std::string(view.data(), view.size());
        }

        inline std::string& toString(const StringView& view, std::string& buffer)
        {
            buffer.clear();
            buffer.insert(buffer.end(), view.begin(), view.end());

            return buffer;
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
            
            
            ImmutableString(const StringView& view)
            {
                copyFrom(view);
            }

            ImmutableString& operator=(const StringView& view)
            {
                copyFrom(view);
                return *this;
            }

            operator StringView() const
            {
                return boost::string_view(ptr_.get(), size_);
            }

        private:

            void copyFrom(const StringView& view)
            {
                if (view.size())
                {
                    ptr_ = std::make_unique<char[]>(view.size());
                    std::copy(view.begin(), view.end(), ptr_.get());
                    size_ = view.size();
                }
            }

            std::unique_ptr<char[]> ptr_;
            std::size_t size_ = 0;
        };
    }
}
