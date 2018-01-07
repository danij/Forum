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
