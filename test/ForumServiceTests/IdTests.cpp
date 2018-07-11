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

    UuidString uuid(std::string_view{ uuidString });

    auto convertedString = static_cast<std::string>(uuid);

    BOOST_REQUIRE_EQUAL(std::string(uuidString), convertedString);
}

BOOST_AUTO_TEST_CASE( Multiple_UuidStrings_can_be_parsed_from_a_string_view )
{
    const char* input = "  084904c2-22a1-4c79-8284-7c78dd065048, {084904c2-22A1-4C79-8284-7c78dd065048};E99A4894-D285-43D5-AA0C-E4DA00DAC2A0";

    std::vector<UuidString> output{ 3 };
    const auto lastInsertAt = parseMultipleUuidStrings(input, output.begin(), output.end());

    const auto distance = std::distance(output.begin(), lastInsertAt);
    BOOST_REQUIRE_EQUAL(static_cast<decltype(distance)>(output.size()), distance);
    BOOST_REQUIRE_EQUAL(UuidString(std::string_view("084904c2-22a1-4c79-8284-7c78dd065048")), output[0]);
    BOOST_REQUIRE_EQUAL(UuidString(std::string_view("084904c2-22A1-4C79-8284-7c78dd065048")), output[1]);
    BOOST_REQUIRE_EQUAL(output[0], output[1]);
    BOOST_REQUIRE_EQUAL(UuidString(std::string_view("E99A4894-D285-43D5-AA0C-E4DA00DAC2A0")), output[2]);
}

BOOST_AUTO_TEST_CASE( Multiple_UuidStrings_can_be_parsed_from_a_string_view_ignoring_invalid_values )
{
    const char* input = "  084904c2-22a1-, {8284-7c78dd065048};E99A4894-D285-43D5-AA0C-E4DA00DAC2A0  ";

    std::vector<UuidString> output{ 3 };
    const auto lastInsertAt = parseMultipleUuidStrings(input, output.begin(), output.end());

    const auto distance = std::distance(output.begin(), lastInsertAt);
    BOOST_REQUIRE_EQUAL(static_cast<decltype(distance)>(1), distance);
    BOOST_REQUIRE_EQUAL(UuidString(std::string_view("E99A4894-D285-43D5-AA0C-E4DA00DAC2A0")), output[0]);
}

BOOST_AUTO_TEST_CASE( Multiple_UuidStrings_can_be_parsed_from_a_string_view_without_exceeding_the_output_container )
{
    const char* input = "  084904c2-22a1-4c79-8284-7c78dd065048, {084904c2-22A1-4C79-8284-7c78dd065048};E99A4894-D285-43D5-AA0C-E4DA00DAC2A0";

    std::vector<UuidString> output{ 2 };
    const auto lastInsertAt = parseMultipleUuidStrings(input, output.begin(), output.end());

    const auto distance = std::distance(output.begin(), lastInsertAt);
    BOOST_REQUIRE_EQUAL(static_cast<decltype(distance)>(2), distance);
    BOOST_REQUIRE_EQUAL(UuidString(std::string_view("084904c2-22a1-4c79-8284-7c78dd065048")), output[0]);
    BOOST_REQUIRE_EQUAL(UuidString(std::string_view("084904c2-22A1-4C79-8284-7c78dd065048")), output[1]);
    BOOST_REQUIRE_EQUAL(output[0], output[1]);
}
