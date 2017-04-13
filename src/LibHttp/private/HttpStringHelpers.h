#pragma once

#include <cstddef>
#include <limits>
#include <type_traits>

#include <boost/lexical_cast.hpp>
#include <boost/utility/string_view.hpp>

namespace Http
{
    typedef boost::string_view StringView;

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
    bool matchStringUpperOrLower(StringView view, const char(&against)[AgainstSize])
    {
        return matchStringUpperOrLower(view.data(), view.size(), against);
    }

    template<typename T>
    void fromStringOrDefault(StringView view, T& toChange, T defaultValue)
    {
        if ( ! boost::conversion::try_lexical_convert(view.data(), view.size(), toChange))
        {
            toChange = defaultValue;
        }
    }

    inline void trimLeadingChar(StringView& view, char toTrim)
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

    static constexpr uint8_t CharToLower[] =
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
        static_assert(std::extent<decltype(HexParsingValues)>::value > std::numeric_limits<uint8_t>::max(),
                      "The HexParsingValues array is too small");

        if (nullptr == value) return 0;

        const char* source = value;
        uint8_t* destination = reinterpret_cast<uint8_t*>(value);

        while (size)
        {
            if (*source == '%')
            {
                if (size > 2)
                {
                    *destination = HexParsingValues[static_cast<uint8_t>(source[1])] * 16 +
                        HexParsingValues[static_cast<uint8_t>(source[2])];
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

    inline StringView viewAfterDecodingUrlEncodingInPlace(char* value, size_t size)
    {
        return StringView(value, decodeUrlEncodingInPlace(value, size));
    }
    
    static const bool ReservedCharactersForUrlEncoding[] = 
    {
        true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
        true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
        true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, true,
        false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true,
        true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, false,
        true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, true, true, true, false, true,
        true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
        true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
        true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
        true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
        true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
        true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
        true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
        true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
    };

    static constexpr size_t MaxUrlEncodingInputSize = 2000;

    /**
     * Performes URL encoding leaving out only unreserved characters 
     * as listed under https://tools.ietf.org/html/rfc3986#section-2.3
     */
    template<size_t Size>
    StringView urlEncode(StringView input, char (&output)[Size])
    {
        static_assert(std::extent<decltype(ReservedCharactersForUrlEncoding)>::value > std::numeric_limits<uint8_t>::max(),
                      "Not enough entries in reservedCharactersForUrlEncoding");
        if ((input.size() * 3) > Size)
        {
            return{};
        }
        char* currentOutput = output;
        for (auto c : input)
        {
            auto unsignedC = static_cast<uint8_t>(c);
            if (ReservedCharactersForUrlEncoding[static_cast<uint8_t>(unsignedC)])
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

        return StringView(output, currentOutput - output);
    }

    /**
     * Performes URL encoding on an input string of maximum MaxUrlEncodingInputSize characters.
     * Use the result as soon as possible, calling the function again will overrite the result
     * 
     * @return A view to a thread-local buffer. 
     */
    inline StringView urlEncode(StringView input)
    {
        static thread_local char output[MaxUrlEncodingInputSize * 3];
        return urlEncode(input, output);
    }
}