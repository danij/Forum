/*
Fast Forum Backend
Copyright (C) 2016-2017 Daniel Jurcau

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

#include <boost/uuid/uuid.hpp>
#include <boost/utility/string_view.hpp>

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
            UuidString(const boost::string_view& value);
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
