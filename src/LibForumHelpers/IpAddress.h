#pragma once

#include <cstdint>
#include <cstddef>

#include <boost/asio/ip/address.hpp>

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

            IpAddress(const IpAddress&) = default;
            IpAddress(IpAddress&&) = default;
            IpAddress& operator=(const IpAddress&) = default;
            IpAddress& operator=(IpAddress&&) = default;

            bool isv4() const noexcept
            {
                return data_.int32[1] == data_.int32[2]
                    && data_.int32[2] == data_.int32[3]
                    && data_.int32[1] == 0;
            }

            const uint8_t* data() const { return data_.bytes; }

            static size_t dataSize() { return 16;  }
            
            /**
             * Writes the string representation of the address to a buffer and returns the amount of bytes written
             */
            size_t toString(char* buffer, size_t bufferSize) const noexcept;

        private:
            union
            {
                uint8_t bytes[16];
                uint32_t int32[4] = { 0, 0, 0, 0 };
            } data_;
        };
    }
}
