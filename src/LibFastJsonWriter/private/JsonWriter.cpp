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

#include <iostream>
#include <memory>
#include <cstring>
#include <string>

using namespace Json;

JsonWriter::JsonWriter(std::ostream& stream) : streamOutput_(&stream)
{
    state_.push({ false, false, false });
}

JsonWriter::JsonWriter(std::vector<char>& vector) : vectorOutput_(&vector)
{
    state_.push({ false, false, false });
}

JsonWriter& JsonWriter::null()
{
    if (isCommaNeeded()) writeString(",null"); else writeString("null");
    return *this;
}

JsonWriter& JsonWriter::startArray()
{
    if (isCommaNeeded())
    {
        writeString(",[");
    }
    else
    {
        writeChar('[');
    }
    state_.push({ true, false, false });
    return *this;
}

JsonWriter& JsonWriter::endArray()
{
    writeChar(']');

    state_.pop();
    return *this;
}

JsonWriter& JsonWriter::startObject()
{
    if (isCommaNeeded())
    {
        writeString(",{");
    }
    else
    {
        writeChar('{');
    }
    state_.push({ true, false, false });
    return *this;
}

JsonWriter& JsonWriter::endObject()
{
    writeChar('}');

    state_.pop();
    return *this;
}

JsonWriter& JsonWriter::newProperty(const char* name)
{
    writeEscapedString(name);
    writeChar(':');

    state_.top().propertyNameAdded = true;
    return *this;
}

JsonWriter& JsonWriter::newProperty(const std::string& name)
{
    writeEscapedString(name.c_str(), name.length());
    writeChar(':');

    state_.top().propertyNameAdded = true;
    return *this;
}

JsonWriter& JsonWriter::newPropertyWithSafeName(const char* name, std::size_t length)
{
    if (isCommaNeeded())
    {
        writeString(",\"");
    }
    else
    {
        writeChar('"');
    }
    writeString(name, length);
    writeString("\":");

    state_.top().propertyNameAdded = true;
    return *this;
}

JsonWriter& JsonWriter::newPropertyWithSafeName(const std::string& name)
{
    if (isCommaNeeded())
    {
        writeString(",\"");
    }
    else
    {
        writeChar('"');
    }
    writeString(name);
    writeString("\":");

    state_.top().propertyNameAdded = true;
    return *this;
}

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

JsonWriter& JsonWriter::writeEscapedString(const char* value, size_t length)
{
    if (isCommaNeeded())
    {
        writeString(",\"");
    }
    else
    {
        writeChar('"');
    }

    if (length == 0)
    {
        length = strlen(value);
    }

    static thread_local char twoCharEscapeBuffer[2] = {'\\', 0};
    static thread_local char sixCharEscapeBuffer[6] = {'\\', 'u', '0', '0', 0, 0};

    auto directWriteFrom = value;
    auto endValue = value + length;

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
                    writeString(directWriteFrom, value - directWriteFrom);
                }
                directWriteFrom = value + 1; //skip the current character as it needs escaping
                if (r < 0xFF)
                {
                    //we have a special character for the escape
                    twoCharEscapeBuffer[1] = static_cast<char>(r);
                    writeString(twoCharEscapeBuffer, 2);
                }
                else
                {
                    //we must use the six-character sequence
                    //simplified as we only escape control characters this way
                    sixCharEscapeBuffer[4] = hexDigits[c / 16];
                    sixCharEscapeBuffer[5] = hexDigits[c % 16];
                    writeString(sixCharEscapeBuffer, 6);
                }
            }
        }
        ++value;
    }

    if (directWriteFrom < value)
    {
        //write remaining characters that don't require escaping
        writeString(directWriteFrom, value - directWriteFrom);
    }

    writeChar('"');

    return *this;
}
