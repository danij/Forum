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

#include "StringHelpers.h"

#include <boost/test/unit_test.hpp>

#include <array>

using namespace Forum::Helpers;

BOOST_AUTO_TEST_CASE( SplitString_populates_empty_string_on_empty_input )
{
    std::array<StringView, 2> output;

    const auto it = splitString("", ' ', output.begin(), output.end());
    
    BOOST_REQUIRE_EQUAL(true, std::next(output.begin()) == it);
    BOOST_REQUIRE_EQUAL(0u, output[0].size());
}

BOOST_AUTO_TEST_CASE( SplitString_populates_entire_value_if_no_separator_is_present )
{
    std::array<StringView, 2> output;

    const auto it = splitString("abcd", ' ', output.begin(), output.end());
    
    BOOST_REQUIRE_EQUAL(true, std::next(output.begin()) == it);
    BOOST_REQUIRE_EQUAL("abcd", output[0]);
}

BOOST_AUTO_TEST_CASE( SplitString_populates_splitted_values )
{
    std::array<StringView, 2> output;

    const auto it = splitString("ab cd", ' ', output.begin(), output.end());
    
    BOOST_REQUIRE_EQUAL(true, std::next(output.begin(), 2) == it);
    BOOST_REQUIRE_EQUAL("ab", output[0]);
    BOOST_REQUIRE_EQUAL("cd", output[1]);
}

BOOST_AUTO_TEST_CASE( SplitString_populates_splitted_values_when_separator_is_at_start )
{
    std::array<StringView, 3> output;

    const auto it = splitString(" ab cd", ' ', output.begin(), output.end());
    
    BOOST_REQUIRE_EQUAL(true, std::next(output.begin(), 3) == it);
    BOOST_REQUIRE_EQUAL("", output[0]);
    BOOST_REQUIRE_EQUAL("ab", output[1]);
    BOOST_REQUIRE_EQUAL("cd", output[2]);
}

BOOST_AUTO_TEST_CASE( SplitString_populates_splitted_values_when_multiple_consecutive_separator_values_are_found )
{
    std::array<StringView, 4> output;

    const auto it = splitString("ab   cd", ' ', output.begin(), output.end());
    
    BOOST_REQUIRE_EQUAL(true, std::next(output.begin(), 4) == it);
    BOOST_REQUIRE_EQUAL("ab", output[0]);
    BOOST_REQUIRE_EQUAL("", output[1]);
    BOOST_REQUIRE_EQUAL("", output[2]);
    BOOST_REQUIRE_EQUAL("cd", output[3]);
}

BOOST_AUTO_TEST_CASE( SplitString_populates_splitted_values_when_separator_is_at_end )
{
    std::array<StringView, 3> output;

    const auto it = splitString("ab cd ", ' ', output.begin(), output.end());
    
    BOOST_REQUIRE_EQUAL(true, std::next(output.begin(), 3) == it);
    BOOST_REQUIRE_EQUAL("ab", output[0]);
    BOOST_REQUIRE_EQUAL("cd", output[1]);
    BOOST_REQUIRE_EQUAL("", output[2]);
}

BOOST_AUTO_TEST_CASE( SplitString_returns_early_if_no_place_for_output )
{
    std::array<StringView, 2> output;

    const auto it = splitString("", ' ', output.begin(), output.begin());
    
    BOOST_REQUIRE_EQUAL(true, output.begin() == it);
    BOOST_REQUIRE_EQUAL(0u, output[0].size());
}

BOOST_AUTO_TEST_CASE( SplitString_returns_early_when_output_gets_full )
{
    std::array<StringView, 3> output;

    const auto it = splitString("ab cd ef", ' ', output.begin(), std::next(output.begin(), 2));
    
    BOOST_REQUIRE_EQUAL(true, std::next(output.begin(), 2) == it);
    BOOST_REQUIRE_EQUAL("ab", output[0]);
    BOOST_REQUIRE_EQUAL("cd", output[1]);
    BOOST_REQUIRE_EQUAL("", output[2]);
}
