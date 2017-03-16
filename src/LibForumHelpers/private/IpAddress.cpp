#include "IpAddress.h"

#include <boost/asio/ip/address.hpp>

#include <algorithm>
#include <cassert>

using namespace Forum::Helpers;

void IpAddress::parse(const char* string)
{
    boost::system::error_code ec;
    auto address = boost::asio::ip::address::from_string(string, ec);

    if ( ! ec)
    {
        if (address.is_v4())
        {
            auto v4Bytes = address.to_v4().to_bytes();
            std::copy(begin(v4Bytes), end(v4Bytes), data_.bytes);
        }
        else if (address.is_v6())
        {
            auto v6Bytes = address.to_v6().to_bytes();
            std::copy(begin(v6Bytes), end(v6Bytes), data_.bytes);
        }
    }
}

static int writeUInt8(char* buffer, uint8_t value) noexcept
{
    if (value > 99)
    {
        buffer[0] = '0' + value / 100;
        value %= 100;
        buffer[1] = '0' + value / 10;
        buffer[2] = '0' + value % 10;
        return 3;
    }
    if (value > 9)
    {
        buffer[0] = '0' + value / 10;
        buffer[1] = '0' + value % 10;
        return 2;
    }
    buffer[0] = '0' + value;
    return 1;
}

const char* hexDigits = "0123456789ABCDEF";

static int writeUInt16Hex(char* buffer, uint8_t mostSignificant, uint8_t leastSignificant) noexcept
{
    const int base = 16;
    if (mostSignificant > 0)
    {
        if (mostSignificant > base)
        {
            buffer[0] = hexDigits[mostSignificant / base];
            buffer[1] = hexDigits[mostSignificant % base];
            buffer[2] = hexDigits[leastSignificant / base];
            buffer[3] = hexDigits[leastSignificant % base];
            return 4;
        }
        buffer[0] = hexDigits[mostSignificant % base];
        buffer[1] = hexDigits[leastSignificant / base];
        buffer[2] = hexDigits[leastSignificant % base];
        return 3;
    }
    if (leastSignificant > base)
    {
        buffer[0] = hexDigits[leastSignificant / base];
        buffer[1] = hexDigits[leastSignificant % base];
        return 2;
    }
    buffer[0] = hexDigits[leastSignificant % base];
    return 1;
}

size_t IpAddress::toString(char* buffer, size_t bufferSize) const noexcept
{
    if (isv4())
    {
        assert(bufferSize >= MaxIPv4CharacterCount);

        auto originalBufferPtr = buffer;
        buffer += writeUInt8(buffer, data_.bytes[0]);
        *buffer++ = '.';
        buffer += writeUInt8(buffer, data_.bytes[1]);
        *buffer++ = '.';
        buffer += writeUInt8(buffer, data_.bytes[2]);
        *buffer++ = '.';
        buffer += writeUInt8(buffer, data_.bytes[3]);

        return buffer - originalBufferPtr;
    }
    else
    {
        assert(bufferSize >= MaxIPv6CharacterCount);

        auto originalBufferPtr = buffer;
        for (size_t i = 0; i < 14; i += 2)
        {
            buffer += writeUInt16Hex(buffer, data_.bytes[i], data_.bytes[i + 1]);
            *buffer++ = ':';
        }
        buffer += writeUInt16Hex(buffer, data_.bytes[14], data_.bytes[15]);

        return buffer - originalBufferPtr;
    }
}