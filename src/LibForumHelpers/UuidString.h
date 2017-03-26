#pragma once

#include <boost/uuid/uuid.hpp>
#include <boost/utility/string_view.hpp>

namespace Forum
{
    namespace Entities
    {
        /**
         * Stores a binary and string representation of a uuid on the stack
         */
        struct UuidString
        {
            UuidString();
            UuidString(boost::uuids::uuid value);
            UuidString(const std::string& value);
            UuidString(const boost::string_view& value);

            /**
             * How many characters are needed to represent the string represation (without null terminator)
             */
            static constexpr size_t StringRepresentationSize = boost::uuids::uuid::static_size() * 2 + 4;

            const boost::uuids::uuid& value() const { return value_; }

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

        inline std::size_t hash_value(const UuidString& value)
        {
            return hash_value(value.value());
        }
    }
}
