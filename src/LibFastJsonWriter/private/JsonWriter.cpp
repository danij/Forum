/*
Fast class for writing Json documents
Copyright (C) 2015 dani.user@gmail.com

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

#include "JsonWriter.h"

#include <algorithm>

using namespace Json;

static const unsigned char toEscape[] =
{
   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x62, 0x74, 0x6E, 0xFF, 0x66, 0x72, 0xFF, 0xFF,
   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
   0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2F,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5C, 0x00, 0x00, 0x00
};
static constexpr int toEscapeLength = sizeof(toEscape) / sizeof(toEscape[0]);

static const char hexDigits[16] = {
   '0', '1', '2', '3', '4', '5', '6', '7',
   '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

bool Json::isEscapeNeeded(const char* value, size_t length)
{
    for (size_t i = 0; i < length; ++i)
    {
        auto c = static_cast<unsigned char>(value[i]);
        if (c < toEscapeLength)
        {
            if(toEscape[c])
            {
                return true;
            }
        }
    }

    return false;
}

size_t writeString(char*& destination, const char* source, size_t length)
{
    std::copy(source, source + length, destination);
    destination += length;
    return length;
}

size_t Json::escapeString(const char* value, size_t length, char* destination)
{
    static thread_local char twoCharEscapeBuffer[2] = {'\\', 0};
    static thread_local char sixCharEscapeBuffer[6] = {'\\', 'u', '0', '0', 0, 0};

    auto directWriteFrom = value;
    auto endValue = value + length;
    size_t written = 0;

    while (value < endValue)
    {
        unsigned char c = static_cast<unsigned char>(*value);
        if (c < toEscapeLength)
        {
            auto r = toEscape[c];
            if (r)
            {
                //escape needed
                if (directWriteFrom < value)
                {
                    //flush previous characters that don't require escaping
                    written += writeString(destination, directWriteFrom, value - directWriteFrom);
                }
                directWriteFrom = value + 1; //skip the current character as it needs escaping
                if (r < 0xFF)
                {
                    //we have a special character for the escape
                    twoCharEscapeBuffer[1] = static_cast<char>(r);
                    written += writeString(destination, twoCharEscapeBuffer, 2);
                }
                else
                {
                    //we must use the six-character sequence
                    //simplified as we only escape control characters this way
                    sixCharEscapeBuffer[4] = hexDigits[c / 16];
                    sixCharEscapeBuffer[5] = hexDigits[c % 16];
                    written += writeString(destination, sixCharEscapeBuffer, 6);
                }
            }
        }
        ++value;
    }

    if (directWriteFrom < value)
    {
        //write remaining characters that don't require escaping
        written += writeString(destination, directWriteFrom, value - directWriteFrom);
    }

    return written;
}
