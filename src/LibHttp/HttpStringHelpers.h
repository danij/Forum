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

#include <cstddef>
#include <ctime>
#include <limits>
#include <type_traits>
#include <string_view>

#include <boost/lexical_cast.hpp>

namespace Http
{
    typedef std::string_view HttpStringView;

    /**
     * Matches a string against another one, optionally ignoring case
     *
     * @pre source must point to (AgainstSize - 1) / 2 characters
     * @param source String to be searched
     * @param against A string where each character appears as both upper and lower case (e.g. HhEeLlLlO  WwOoRrLlDd)
     */
    template<size_t AgainstSize>
    bool matchStringUpperOrLower(const char* source, const char (&against)[AgainstSize])
    {
        char result = 0;
        auto size = (AgainstSize - 1) / 2; //-1 because AgainstSize also contains terminating null character
        for (size_t iSource = 0, iAgainst = 0; iSource < size; ++iSource, iAgainst += 2)
        {
            result |= (source[iSource] ^ against[iAgainst]) & (source[iSource] ^ against[iAgainst + 1]);
        }
        return result == 0;
    }

    /**
     * Matches a string against another one, optionally ignoring case and checking the length of the soruce string
     *
     * @pre source must point to (AgainstSize - 1) / 2 characters
     * @param source String to be searched
     * @param sourceSize Size of string to be searched
     * @param against A string where each character appears as both upper and lower case (e.g. HhEeLlLlO  WwOoRrLlDd)
     */
    template<size_t AgainstSize>
    bool matchStringUpperOrLower(const char* source, size_t sourceSize, const char(&against)[AgainstSize])
    {
        auto expectedSourceSize = (AgainstSize - 1) / 2;
        if (sourceSize != expectedSourceSize)
        {
            return false;
        }
        return matchStringUpperOrLower(source, against);
    }

    template<size_t AgainstSize>
    bool matchStringUpperOrLower(HttpStringView view, const char(&against)[AgainstSize])
    {
        return matchStringUpperOrLower(view.data(), view.size(), against);
    }

    template<typename T>
    void fromStringOrDefault(HttpStringView view, T& toChange, T defaultValue)
    {
        if ( ! boost::conversion::try_lexical_convert(view.data(), view.size(), toChange))
        {
            toChange = defaultValue;
        }
    }

    inline void trimLeadingChar(HttpStringView& view, char toTrim)
    {
        size_t toRemove = 0;
        for (auto c : view)
        {
            if (c == toTrim)
            {
                ++toRemove;
            }
            else
            {
                break;
            }
        }
        view.remove_prefix(toRemove);
    }

    static constexpr unsigned char CharToLower[] =
    {
          0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
         16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
         32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
         48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
         64,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
        112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122,  91,  92,  93,  94,  95,
         96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
        112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
        128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
        144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
        160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
        176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
        192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
        208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
        224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
        240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255
    };

    static const int HexParsingValues[] =
    {
        0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0,
        0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };

    static const char HexToStringUpperCase[] = "0123456789ABCDEF";

    inline size_t decodeUrlEncodingInPlace(char* value, size_t size)
    {
        static_assert(std::extent<decltype(HexParsingValues)>::value > std::numeric_limits<unsigned char>::max(),
                      "The HexParsingValues array is too small");

        if (nullptr == value) return 0;

        const char* source = value;
        auto* destination = reinterpret_cast<unsigned char*>(value);

        while (size)
        {
            if (*source == '%')
            {
                if (size > 2)
                {
                    *destination = HexParsingValues[static_cast<unsigned char>(source[1])] * 16 +
                        HexParsingValues[static_cast<unsigned char>(source[2])];
                    size -= 2;
                    source += 2;
                }
                else
                {
                    break;
                }
            }
            else
            {
                *destination = *source;
            }
            --size;
            ++destination;
            ++source;
        }

        return reinterpret_cast<char*>(destination) - value;
    }

    inline HttpStringView viewAfterDecodingUrlEncodingInPlace(char* value, size_t size)
    {
        return HttpStringView(value, decodeUrlEncodingInPlace(value, size));
    }

    static const char ReservedCharactersForUrlEncoding[] =
    {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
    };

    static constexpr size_t MaxPercentEncodingInputSize = 2000;
    static constexpr size_t MaxPercentEncodingOutputSize = MaxPercentEncodingInputSize * 3;

    template<size_t OutputSize, size_t TableSize>
    HttpStringView percentEncode(HttpStringView input, char (&output)[OutputSize], const char (&table)[TableSize])
    {
        static_assert(TableSize > std::numeric_limits<uint8_t>::max(), "Not enough entries in table");
        if ((input.size() * 3) > OutputSize)
        {
            return{};
        }
        char* currentOutput = output;
        for (auto c : input)
        {
            const auto unsignedC = static_cast<uint8_t>(c);
            if (table[static_cast<uint8_t>(unsignedC)])
            {
                *currentOutput++ = '%';
                *currentOutput++ = HexToStringUpperCase[unsignedC / 16];
                *currentOutput++ = HexToStringUpperCase[unsignedC % 16];
            }
            else
            {
                *currentOutput++ = c;
            }
        }

        return HttpStringView(output, currentOutput - output);
    }

    /**
     * Performes URL encoding leaving out only unreserved characters
     * as listed under https://tools.ietf.org/html/rfc3986#section-2.3
     */
    template<size_t Size>
    HttpStringView urlEncode(HttpStringView input, char (&output)[Size])
    {
        return percentEncode(input, output, ReservedCharactersForUrlEncoding);
    }

    /**
    * Performes percent encoding on an input string of maximum MaxPercentEncodingInputSize characters.
    * Use the result as soon as possible, calling the function again will overrite the result
    *
    * @return A view to a thread-local buffer.
    */
    template<size_t TableSize>
    HttpStringView percentEncode(HttpStringView input, const char (&table)[TableSize])
    {
        static thread_local char output[MaxPercentEncodingOutputSize];
        return percentEncode(input, output, table);
    }

    /**
     * Performes URL encoding on an input string of maximum MaxPercentEncodingInputSize characters.
     * Use the result as soon as possible, calling the function again will overrite the result
     *
     * @return A view to a thread-local buffer.
     */
    inline HttpStringView urlEncode(HttpStringView input)
    {
        static thread_local char output[MaxPercentEncodingOutputSize];
        return urlEncode(input, output);
    }

    /**
     * Writes a date string as expected by the HTTP protocol, e.g. Tue, 18 Apr 2017 09:00:00 GMT
     * The functions doesn't check bounds, the output buffer must be large enough
     *
     * @return The amount of characters written
     */
    inline size_t writeHttpDateGMT(time_t value, char* output)
    {
        //https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Date
        static const char DayNames[][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
        static const char MonthNames[][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                              "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
        std::tm time;
#if defined(_WIN32) || defined (WIN32)
        if (0 != gmtime_s(&time, &value))
        {
            return 0;
        }
#else
        //gmtime is not guaranteed to be thread-safe
        if (nullptr == gmtime_r(&value, &time))
        {
            return 0;
        }
#endif
        char* currentOutput = output;

        currentOutput = std::copy(DayNames[time.tm_wday], DayNames[time.tm_wday] + 3, currentOutput);
        *currentOutput++ = ',';
        *currentOutput++ = ' ';

        *currentOutput++ = '0' + (time.tm_mday / 10);
        *currentOutput++ = '0' + (time.tm_mday % 10);
        *currentOutput++ = ' ';

        currentOutput = std::copy(MonthNames[time.tm_mon], MonthNames[time.tm_mon] + 3, currentOutput);
        *currentOutput++ = ' ';

        const auto year = 1900 + time.tm_year;
        *currentOutput++ = '0' + (year / 1000);
        *currentOutput++ = '0' + ((year % 1000) / 100);
        *currentOutput++ = '0' + ((year % 100) / 10);
        *currentOutput++ = '0' + (year % 10);
        *currentOutput++ = ' ';

        *currentOutput++ = '0' + (time.tm_hour / 10);
        *currentOutput++ = '0' + (time.tm_hour % 10);
        *currentOutput++ = ':';
        *currentOutput++ = '0' + (time.tm_min / 10);
        *currentOutput++ = '0' + (time.tm_min % 10);
        *currentOutput++ = ':';
        *currentOutput++ = '0' + (time.tm_sec / 10);
        *currentOutput++ = '0' + (time.tm_sec % 10);
        *currentOutput++ = ' ';
        *currentOutput++ = 'G';
        *currentOutput++ = 'M';
        *currentOutput++ = 'T';

        return currentOutput - output;
    }
}
