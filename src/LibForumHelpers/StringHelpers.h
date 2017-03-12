#pragma once

#include <algorithm>
#include <cstdint>
#include <string>

namespace Forum
{
    namespace Helpers
    {
        /**
         * Returns the number of characters from a valid UTF-8 input
         */
        inline auto countUTF8Characters(const std::string& value)
        {
            return static_cast<int_fast32_t>(std::count_if(value.cbegin(), value.cend(), [](auto c)
            {
                auto v = static_cast<uint8_t>(c);
                return v < 0b10000000 || v >= 0b11000000;
            }));
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
    }
}
