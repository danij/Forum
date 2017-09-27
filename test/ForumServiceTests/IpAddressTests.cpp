#include "IpAddress.h"

#include <boost/test/unit_test.hpp>

using namespace Forum::Helpers;

BOOST_AUTO_TEST_CASE( String_to_IPv4_Address_to_string_works_as_expected )
{
    std::string v4Address = "100.0.99.1";
    IpAddress address(v4Address.c_str());

    BOOST_REQUIRE(address.isV4());

    char buffer[IpAddress::MaxIPv4CharacterCount];
    auto bytesWritten = address.toString(buffer, std::extent<decltype(buffer)>::value);

    BOOST_REQUIRE_EQUAL(v4Address.size(), bytesWritten);
    BOOST_REQUIRE_EQUAL(v4Address, std::string(buffer, bytesWritten));
}

BOOST_AUTO_TEST_CASE( String_to_IPv6_Address_to_string_works_as_expected )
{
    std::string v6Address = "FF02:0:A0:B:1C0:3EA2:0:2";
    IpAddress address(v6Address.c_str());

    BOOST_REQUIRE( ! address.isV4());

    char buffer[IpAddress::MaxIPv6CharacterCount];
    auto bytesWritten = address.toString(buffer, std::extent<decltype(buffer)>::value);

    BOOST_REQUIRE_EQUAL(v6Address.size(), bytesWritten);
    BOOST_REQUIRE_EQUAL(v6Address, std::string(buffer, bytesWritten));
}
