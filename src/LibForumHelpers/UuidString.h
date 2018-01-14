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

#include <functional>
#include <string>
#include <cstdint>

#include <boost/uuid/uuid.hpp>
#include <boost/utility/string_view.hpp>
#include <boost/asio/ip/impl/address.ipp>

namespace Forum
{
    namespace Entities
    {
        /**
         * Stores a binary and string representation of a uuid on the stack
         */
        struct UuidString final
        {
            UuidString();
            UuidString(boost::uuids::uuid value);
            UuidString(const std::string& value);
            UuidString(boost::string_view value);
            explicit UuidString(const uint8_t* uuidArray);

            UuidString(const UuidString&) = default;
            UuidString(UuidString&&) = default;

            UuidString& operator=(const UuidString&) = default;
            UuidString& operator=(UuidString&&) = default;

            /**
             * How many characters are needed to store the string representation (without null terminator)
             */
            static constexpr size_t StringRepresentationSize = boost::uuids::uuid::static_size() * 2 + 4;

            const boost::uuids::uuid& value() const { return value_; }
            auto                  hashValue() const { return hashValue_; }

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

            /**
             * Required by ConstMapAdapter
             */
            auto toConst() const
            {
                return *this;
            }

            operator bool() const
            {
                return *this != empty;
            }

            static const UuidString empty;

        private:
            void updateHashValue();

            boost::uuids::uuid value_;
            size_t hashValue_;
        };

        inline size_t hash_value(const UuidString& value)
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
        It parseMultipleUuidStrings(boost::string_view input, It outputBegin, It outputEnd)
        {
            if (outputBegin == outputEnd) return outputBegin;

            auto previousStart = input.cbegin();
            auto inputIt = input.cbegin();
            for (const auto inputEnd = input.cend(); inputIt != inputEnd; ++inputIt)
            {
                if (0 == OccursInUuids[*inputIt])
                {
                    if ((inputIt - previousStart) == UuidString::StringRepresentationSize)
                    {
                        *outputBegin = UuidString(boost::string_view(previousStart, UuidString::StringRepresentationSize));
                        if (++outputBegin == outputEnd) return outputBegin;
                    }
                    previousStart = inputIt + 1;
                }
            }
            if ((inputIt - previousStart) == UuidString::StringRepresentationSize)
            {
                *outputBegin++ = UuidString(boost::string_view(previousStart, UuidString::StringRepresentationSize));
            }
            return outputBegin;
        }
    }
}

namespace std
{
    template<>
    struct hash<Forum::Entities::UuidString>
    {
        size_t operator()(const Forum::Entities::UuidString& value) const
        {
            return value.hashValue();
        }
    };
}
