#include <algorithm>
#include <sstream>

#include <boost/uuid/uuid_io.hpp>

#include "UuidString.h"

using namespace Forum::Entities;

UuidString::UuidString() : UuidString(boost::uuids::uuid{}) { }

UuidString::UuidString(boost::uuids::uuid value) : value_(value)
{
    auto stringValue = boost::uuids::to_string(value);
    std::copy(stringValue.begin(), stringValue.end(), characters_.begin());
}

UuidString::UuidString(const std::string& value) : value_({})
{
    std::istringstream stream(value);
    stream >> value_;
    auto stringValue = boost::uuids::to_string(value_);
    std::copy(stringValue.begin(), stringValue.end(), characters_.begin());
}

const UuidString UuidString::empty = {};
