/*
Fast Forum Backend
Copyright (C) 2016-present Daniel Jurcau

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

#include "IpAddress.h"

#include <boost/test/unit_test.hpp>

using namespace Forum::Helpers;

BOOST_AUTO_TEST_CASE( String_to_IPv4_Address_to_string_works_as_expected )
{
    std::string v4Address = "100.0.99.1";
    IpAddress address(v4Address.c_str());

    BOOST_REQUIRE(address.isV4());

    char buffer[IpAddress::MaxIPv4CharacterCount];
    auto bytesWritten = address.toString(buffer, std::size(buffer));

    BOOST_REQUIRE_EQUAL(v4Address.size(), bytesWritten);
    BOOST_REQUIRE_EQUAL(v4Address, std::string(buffer, bytesWritten));
}

BOOST_AUTO_TEST_CASE( String_to_IPv6_Address_to_string_works_as_expected )
{
    std::string v6Address = "FF02:0:A0:B:1C0:3EA2:0:2";
    IpAddress address(v6Address.c_str());

    BOOST_REQUIRE( ! address.isV4());

    char buffer[IpAddress::MaxIPv6CharacterCount];
    auto bytesWritten = address.toString(buffer, std::size(buffer));

    BOOST_REQUIRE_EQUAL(v6Address.size(), bytesWritten);
    BOOST_REQUIRE_EQUAL(v6Address, std::string(buffer, bytesWritten));
}
