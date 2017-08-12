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
