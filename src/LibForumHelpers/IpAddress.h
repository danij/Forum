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

#include <algorithm>
#include <cstdint>
#include <cstddef>

#include <boost/asio/ip/address.hpp>
#include <boost/functional/hash.hpp>

#ifdef DELETE
#undef DELETE
#endif

namespace Forum
{
    namespace Helpers
    {
        struct IpAddress final
        {
            constexpr static int MaxIPv4CharacterCount = 15;
            constexpr static int MaxIPv6CharacterCount = 39;

            IpAddress();
            IpAddress(const boost::asio::ip::address& value);
            IpAddress(const char* string);
            explicit IpAddress(const uint8_t* dataArray);

            IpAddress(const IpAddress&) = default;
            IpAddress(IpAddress&&) = default;
            IpAddress& operator=(const IpAddress&) = default;
            IpAddress& operator=(IpAddress&&) = default;

            bool isV4() const noexcept
            {
                return data_.int32[1] == data_.int32[2]
                    && data_.int32[2] == data_.int32[3]
                    && data_.int32[1] == 0;
            }

            auto nrOfBytes() const noexcept
            {
                return isV4() ? 4 : 16;
            }

            const uint8_t* data() const { return data_.bytes; }
            const uint32_t* intData() const { return data_.int32; }

            static constexpr size_t dataSize() { return 16; }

            /**
             * Writes the string representation of the address to a buffer and returns the amount of bytes written
             */
            size_t toString(char* buffer, size_t bufferSize) const noexcept;

            bool operator==(const IpAddress& other) const
            {
                return (data_.int32[0] == other.data_.int32[0])
                    && (data_.int32[1] == other.data_.int32[1])
                    && (data_.int32[2] == other.data_.int32[2])
                    && (data_.int32[3] == other.data_.int32[3]);
            }

            bool operator!=(const IpAddress& other) const
            {
                return ! this->operator==(other);
            }

            bool operator<(const IpAddress& other) const
            {
                return std::lexicographical_compare(data_.bytes, data_.bytes + nrOfBytes(),
                                                    other.data_.bytes, other.data_.bytes + nrOfBytes());
            }

            bool operator<=(const IpAddress& other) const
            {
                return ! this->operator>(other);
            }

            bool operator>(const IpAddress& other) const
            {
                return std::lexicographical_compare(other.data_.bytes, other.data_.bytes + nrOfBytes(),
                                                    data_.bytes, data_.bytes + nrOfBytes());
            }

            bool operator>=(const IpAddress& other) const
            {
                return ! this->operator<(other);
            }

        private:
            union
            {
                uint8_t bytes[16];
                uint32_t int32[4] = { 0, 0, 0, 0 };
            } data_;
        };
    }
}

namespace std
{
    template<>
    struct hash<Forum::Helpers::IpAddress>
    {
        size_t operator()(const Forum::Helpers::IpAddress& value) const
        {
            if (value.isV4())
            {
                return static_cast<size_t>(value.intData()[0]);
            }
            auto ints = value.intData();
            return boost::hash_range(ints, ints + 4);
        }
    };
}
