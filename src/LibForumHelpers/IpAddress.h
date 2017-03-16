#pragma once

#include <cstdint>
#include <cstddef>

namespace Forum
{
    namespace Helpers
    {
        struct IpAddress
        {
            constexpr static int MaxIPv4CharacterCount = 15;
            constexpr static int MaxIPv6CharacterCount = 39;

            bool isv4() const noexcept
            {
                return data_.int32[1] == data_.int32[2]
                    && data_.int32[2] == data_.int32[3]
                    && data_.int32[1] == 0;
            }

            void parse(const char* string);

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
