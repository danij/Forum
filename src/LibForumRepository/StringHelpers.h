#pragma once

#include <algorithm>
#include <string>

#include <boost/locale.hpp>
#include <boost/locale/boundary.hpp>

namespace Forum
{
    namespace Helpers
    {
        extern const boost::locale::generator localeGenerator;
        extern const std::locale en_US_UTF8Locale;

        template <typename T>
        inline auto getUTF8CharactersIterator(T begin, T end)
        {
            return boost::locale::boundary::ssegment_index(boost::locale::boundary::character,
                                                           begin, end, en_US_UTF8Locale);
        }

        inline auto getUTF8CharactersIterator(const std::string& value)
        {
            return getUTF8CharactersIterator(value.cbegin(), value.cend());
        }

        inline size_t countUTF8Characters(const std::string& value)
        {
            auto iterator = getUTF8CharactersIterator(value);
            return std::distance(iterator.begin(), iterator.end());
        }

        struct StringAccentAndCaseInsensitiveLess
        {
            bool operator()(const std::string& lhs, const std::string& rhs) const;
        };
    }
}
