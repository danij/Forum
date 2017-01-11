#pragma once

#include <boost/locale.hpp>
#include <boost/locale/boundary.hpp>

#include <algorithm>
#include <cstdint>
#include <string>

namespace Forum
{
    namespace Helpers
    {
        extern thread_local const boost::locale::generator localeGenerator;
        extern thread_local const std::locale en_US_UTF8Locale;

        template <typename T>
        auto getUTF8CharactersIterator(T begin, T end)
        {
            return boost::locale::boundary::ssegment_index(boost::locale::boundary::character,
                                                           begin, end, en_US_UTF8Locale);
        }

        inline auto getUTF8CharactersIterator(const std::string& value)
        {
            return getUTF8CharactersIterator(value.cbegin(), value.cend());
        }

        /*
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

        struct StringAccentAndCaseInsensitiveLess
        {
            bool operator()(const std::string& lhs, const std::string& rhs) const;
        };
    }
}
