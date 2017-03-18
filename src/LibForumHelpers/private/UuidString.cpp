#include "UuidString.h"

#include <boost/uuid/uuid_io.hpp>

using namespace Forum::Entities;

UuidString::UuidString() : UuidString(boost::uuids::uuid{}) { }

UuidString::UuidString(boost::uuids::uuid value) : value_(std::move(value))
{
}

static const int hexValues[] = 
{
    0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0,
    0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static const int uuidValuePositions[] = 
{
    /* 8*/ 0, 1, 2, 3, 4, 5, 6, 7, 
    /* 4*/ 9, 10, 11, 12,
    /* 4*/ 14, 15, 16, 17,
    /* 4*/ 19, 20, 21, 22,
    /*12*/ 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35
};

static bool parseUuid(const char* data, size_t size, boost::uuids::uuid& destination)
{
    if (size != 36)
    {
        return false;
    }

    for (int src = 0, dst = 0; dst < 16; src += 2, dst += 1)
    {
        destination.data[dst] = hexValues[data[uuidValuePositions[src]]] << 4 
                              | hexValues[data[uuidValuePositions[src + 1]]];
    }

    return true;
}

UuidString::UuidString(const std::string& value)
{
    if ( ! parseUuid(value.data(), value.size(), value_))
    {
        value_ = {};
    }
}

UuidString::UuidString(const boost::string_view& value)
{
    if ( ! parseUuid(value.data(), value.size(), value_))
    {
        value_ = {};
    }
}

UuidString::operator std::string() const
{
    return to_string(value_);
}

const UuidString UuidString::empty = {};
