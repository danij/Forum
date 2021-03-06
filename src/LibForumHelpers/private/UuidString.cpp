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

#include "UuidString.h"

#include <boost/uuid/uuid_io.hpp>

using namespace Forum::Helpers;

UuidString::UuidString() : value_(boost::uuids::uuid{})
{}

UuidString::UuidString(const boost::uuids::uuid value) : value_(value)
{}

static const uint8_t hexValues[] =
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

static const int uuidValuePositions[] =
{
    /* 8*/ 0, 1, 2, 3, 4, 5, 6, 7,
    /* 4*/ 9, 10, 11, 12,
    /* 4*/ 14, 15, 16, 17,
    /* 4*/ 19, 20, 21, 22,
    /*12*/ 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35
};

static bool parseUuidDashed(const char* data, const size_t size, boost::uuids::uuid& destination)
{
    static_assert(std::size(hexValues) >= sizeof(uint8_t), "hexValues does not contain enough values");

    for (int src = 0, dst = 0; dst < 16; src += 2, dst += 1)
    {
        destination.data[dst] = hexValues[static_cast<uint8_t>(data[uuidValuePositions[src]])] << 4
                              | hexValues[static_cast<uint8_t>(data[uuidValuePositions[src + 1]])];
    }
    return true;
}

static bool parseUuidCompact(const char* data, const size_t size, boost::uuids::uuid& destination)
{
    static_assert(std::size(hexValues) >= sizeof(uint8_t), "hexValues does not contain enough values");

    for (int src = 0, dst = 0; dst < 16; src += 2, dst += 1)
    {
        destination.data[dst] = hexValues[static_cast<uint8_t>(data[src])] << 4
                              | hexValues[static_cast<uint8_t>(data[src + 1])];
    }
    return true;
}

static bool parseUuid(const char* data, const size_t size, boost::uuids::uuid& destination)
{
    switch(size)
    {
        case UuidString::StringRepresentationSizeCompact:
            return parseUuidCompact(data, size, destination);
        case UuidString::StringRepresentationSizeDashed:
            return parseUuidDashed(data, size, destination);
        default:
            return false;
    }
}

UuidString::UuidString(const std::string& value) : UuidString(std::string_view(value))
{
}

UuidString::UuidString(std::string_view value) : value_{}
{
    if ( ! parseUuid(value.data(), value.size(), value_))
    {
        value_ = {};
    }
}

UuidString::UuidString(const uint8_t* uuidArray) : value_{}
{
    std::copy(uuidArray, uuidArray + boost::uuids::uuid::static_size(), value_.data);
}

static const char* hexCharsLowercase = "0123456789abcdef";

void UuidString::toStringDashed(char* buffer) const
{
    const auto data = value_.data;

    for (size_t source = 0, destination = 0; source < boost::uuids::uuid::static_size(); ++source)
    {
        const auto value = data[source];

        buffer[destination++] = hexCharsLowercase[(value / 16) & 0xF];
        buffer[destination++] = hexCharsLowercase[(value % 16) & 0xF];

        if (8 == destination || 13 == destination || 18 == destination || 23 == destination)
        {
            buffer[destination++] = '-';
        }
    }
}

void UuidString::toStringCompact(char* buffer) const
{
    const auto data = value_.data;

    for (size_t source = 0, destination = 0; source < boost::uuids::uuid::static_size(); ++source)
    {
        const auto value = data[source];

        buffer[destination++] = hexCharsLowercase[(value / 16) & 0xF];
        buffer[destination++] = hexCharsLowercase[(value % 16) & 0xF];
    }
}

std::string UuidString::toStringDashed() const
{
    std::string result;
    result.resize(StringRepresentationSizeDashed, ' ');

    toStringDashed(result.data());

    return result;
}

std::string UuidString::toStringCompact() const
{
    std::string result;
    result.resize(StringRepresentationSizeCompact, ' ');

    toStringCompact(result.data());

    return result;
}

const UuidString UuidString::empty = {};
