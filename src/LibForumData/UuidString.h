#pragma once

#include <array>
#include <ostream>

#include <boost/uuid/uuid.hpp>

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

            static constexpr int MaxCharacters = 36;
            typedef std::array<char, MaxCharacters> CharacterArrayType;

            inline const boost::uuids::uuid&      value() const { return value_; }
            inline const CharacterArrayType& characters() const { return characters_; }

            inline bool operator==(const UuidString& other) const
            {
                return value_ == other.value_;
            }

            inline bool operator!=(const UuidString& other) const
            {
                return value_ != other.value_;
            }

            inline bool operator<(const UuidString& other) const
            {
                return value_ < other.value_;
            }

            inline bool operator<=(const UuidString& other) const
            {
                return value_ <= other.value_;
            }

            inline bool operator>(const UuidString& other) const
            {
                return value_ > other.value_;
            }

            inline bool operator>=(const UuidString& other) const
            {
                return value_ >= other.value_;
            }

            inline explicit operator std::string() const
            {
                return std::string(characters_.data(), MaxCharacters);
            }

            static const UuidString empty;

        private:
            boost::uuids::uuid value_;
            CharacterArrayType characters_;
        };

        inline std::size_t hash_value(const UuidString& value)
        {
            return hash_value(value.value());
        }

        inline std::ostream& operator<<(std::ostream& stream, const UuidString& uuidString)
        {
            stream.write(uuidString.characters().data(), UuidString::MaxCharacters);
            return stream;
        }
    }
}
