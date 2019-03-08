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

#include <algorithm>
#include <functional>
#include <string>
#include <string_view>
#include <cstdint>
#include <cstring>

#include <boost/uuid/uuid.hpp>

namespace Forum::Helpers
{
    /**
     * Stores a binary and string representation of a uuid on the stack
     */
    struct UuidString final
    {
        UuidString();
        UuidString(boost::uuids::uuid value);
        UuidString(const std::string& value);
        UuidString(std::string_view value);
        explicit UuidString(const uint8_t* uuidArray);
        ~UuidString() = default;

        UuidString(const UuidString&) = default;
        UuidString(UuidString&&) = default;

        UuidString& operator=(const UuidString&) = default;
        UuidString& operator=(UuidString&&) = default;

        /**
         * How many characters are needed to store the string representation (without null terminator)
         */
        static constexpr size_t StringRepresentationSize = boost::uuids::uuid::static_size() * 2 + 4;

        const boost::uuids::uuid& value() const { return value_; }

        size_t hashValue() const
        {
            size_t result;
            memcpy(&result, std::cend(value_.data) - sizeof(size_t) / sizeof(decltype(value_.data[0])), sizeof(size_t));
            return result;
        }

        bool operator==(const UuidString& other) const
        {
            return value_ == other.value_;
        }

        bool operator!=(const UuidString& other) const
        {
            return value_ != other.value_;
        }

        bool operator<(const UuidString& other) const
        {
            return value_ < other.value_;
        }

        bool operator<=(const UuidString& other) const
        {
            return value_ <= other.value_;
        }

        bool operator>(const UuidString& other) const
        {
            return value_ > other.value_;
        }

        bool operator>=(const UuidString& other) const
        {
            return value_ >= other.value_;
        }

        explicit operator std::string() const;

        /**
         * Writes the string representation to a buffer
         * The buffer must be at least StringRepresentationSize large
         */
        void toString(char* buffer) const;

        operator bool() const
        {
            return *this != empty;
        }

        static const UuidString empty;

    private:
        boost::uuids::uuid value_;
    };

    inline size_t hash_value(const UuidString& value) //used by boost::hash
    {
        return value.hashValue();
    }
    
    static const int8_t OccursInUuids[] =
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
        0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };

    /**
     * Parses uuid strings separated by any character that is not part of the uuid string representation.
     */
    template<typename It>
    It parseMultipleUuidStrings(std::string_view input, It outputBegin, It outputEnd)
    {
        if (outputBegin == outputEnd) return outputBegin;

        auto previousStart = input.data();
        auto inputIt = input.data();
        for (const auto inputEnd = inputIt + input.size(); inputIt != inputEnd; ++inputIt)
        {
            if (0 == OccursInUuids[static_cast<uint8_t>(*inputIt)])
            {
                if ((inputIt - previousStart) == UuidString::StringRepresentationSize)
                {
                    *outputBegin = UuidString(std::string_view(previousStart, UuidString::StringRepresentationSize));
                    if (++outputBegin == outputEnd) return outputBegin;
                }
                previousStart = inputIt + 1;
            }
        }
        if ((inputIt - previousStart) == UuidString::StringRepresentationSize)
        {
            *outputBegin++ = UuidString(std::string_view(previousStart, UuidString::StringRepresentationSize));
        }
        return outputBegin;
    }
    
    /**
    * Extract user ids in the form of @00000000-0000-0000-0000-000000000000@
    */
    template<typename It>
    void extractUuidReferences(std::string_view input, It output)
    {
        constexpr size_t referenceSize = UuidString::StringRepresentationSize + 2u;
        constexpr char wrapper = '@';
        constexpr size_t hexIndexes[] =
        {
            1, 2, 3, 4, 5, 6, 7, 8,
            10, 11, 12, 13,
            15, 16, 17, 18,
            20, 21, 22, 23,
            25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36
        };

        for (int i = 0, n = static_cast<int>(input.size()) - referenceSize; i <= n; ++i)
        {
            const bool possibleReferenceFound =
                (wrapper == input[i]) && (wrapper == input[i + UuidString::StringRepresentationSize + 1])
                && ('-' == input[i + 9]) && ('-' == input[i + 14]) && ('-' == input[i + 19]) && ('-' == input[i + 24]);
            if (possibleReferenceFound)
            {
                if (std::all_of(std::begin(hexIndexes), std::end(hexIndexes), [input, i](const size_t index)
                {
                    return 1 == OccursInUuids[static_cast<int>(input[i + index])];
                }))
                {
                    *output++ = UuidString(std::string_view(input.data() + i + 1, UuidString::StringRepresentationSize));
                }
            }
        }
    }
}

namespace std
{
    template<>
    struct hash<Forum::Helpers::UuidString>
    {
        size_t operator()(const Forum::Helpers::UuidString& value) const
        {
            return value.hashValue();
        }
    };
}
