#include "UuidString.h"

#include <boost/uuid/uuid_io.hpp>

#include <sstream>

using namespace Forum::Entities;

UuidString::UuidString() : UuidString(boost::uuids::uuid{}) { }

UuidString::UuidString(boost::uuids::uuid value) : value_(std::move(value))
{
}

UuidString::UuidString(const std::string& value) : value_({})
{
    std::istringstream stream(value);
    stream >> value_;
}

UuidString::operator std::string() const
{
    return to_string(value_);
}


const UuidString UuidString::empty = {};
