#include "UuidString.h"

#include <boost/test/unit_test.hpp>

using namespace Forum::Entities;

BOOST_AUTO_TEST_CASE( UuidString_can_be_converted_to_and_from_strings )
{
    const std::string uuidString("084904c2-22a1-4c79-8284-7c78dd065048");

    UuidString uuid(uuidString);

    auto convertedString = static_cast<std::string>(uuid);

    BOOST_REQUIRE_EQUAL(uuidString, convertedString);
}

BOOST_AUTO_TEST_CASE( UuidString_can_be_converted_to_and_from_string_views )
{
    const char* uuidString = "084904c2-22a1-4c79-8284-7c78dd065048";

    UuidString uuid(boost::string_view{ uuidString });

    auto convertedString = static_cast<std::string>(uuid);

    BOOST_REQUIRE_EQUAL(std::string(uuidString), convertedString);
}

