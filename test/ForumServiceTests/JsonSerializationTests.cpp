/*
Fast Forum Backend
Copyright (C) 2016-2017 Daniel Jurcau

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

#include "JsonWriter.h"

#include <boost/test/unit_test.hpp>

using namespace Json;

BOOST_AUTO_TEST_CASE( Json_serialization_works_for_nulls )
{
    StringBuffer buffer;
    JsonWriter writer(buffer);

    writer.startObject();
    writer.newPropertyWithSafeName("prop1") << 1;
    writer.newPropertyWithSafeName("prop2").null();
    writer.endObject();

    auto str = buffer.view();
    BOOST_REQUIRE_EQUAL("{\"prop1\":1,\"prop2\":null}", str);
}

BOOST_AUTO_TEST_CASE( Json_serialization_works_for_integers )
{
    StringBuffer buffer;
    JsonWriter writer(buffer);

    writer.startObject();
    writer.newPropertyWithSafeName("prop1") << -1234;
    writer.newPropertyWithSafeName("prop2") << 0;
    writer.newPropertyWithSafeName("prop3") << 2147483647;
    writer.newPropertyWithSafeName("prop4") << static_cast<int8_t>(-128);
    writer.newPropertyWithSafeName("prop5") << static_cast<int32_t>(-2147483648);
    writer.endObject();

    auto str = buffer.view();
    BOOST_REQUIRE_EQUAL("{\"prop1\":-1234,\"prop2\":0,\"prop3\":2147483647,\"prop4\":-128,\"prop5\":-2147483648}", str);
}

BOOST_AUTO_TEST_CASE( Json_serialization_escapes_property_names )
{
    StringBuffer buffer;
    JsonWriter writer(buffer);

    writer.startObject();
    writer.newProperty(std::string("prop\"1")) << 1;
    writer.newProperty("prop\n\"2\"").null();
    writer.endObject();

    auto str = buffer.view();
    BOOST_REQUIRE_EQUAL("{\"prop\\\"1\":1,\"prop\\n\\\"2\\\"\":null}", str);
}

BOOST_AUTO_TEST_CASE( Json_serialization_escapes_well_known_patterns_in_strings )
{
    StringBuffer buffer;
    JsonWriter writer(buffer);

    writer.startObject();
    writer.newPropertyWithSafeName("prop") << "a\"b\\/\bc\fde\n\r\tz";
    writer.endObject();

    auto str = buffer.view();
    BOOST_REQUIRE_EQUAL("{\"prop\":\"a\\\"b\\\\\\/\\bc\\fde\\n\\r\\tz\"}", str);
}

BOOST_AUTO_TEST_CASE( Json_serialization_escapes_strings_with_hex_digits )
{
    StringBuffer buffer;
    JsonWriter writer(buffer);

    writer.startObject();
    writer.newPropertyWithSafeName("prop") << "a\x01\x02\x03 bc\x1f";
    writer.endObject();

    auto str = buffer.view();
    BOOST_REQUIRE_EQUAL("{\"prop\":\"a\\u0001\\u0002\\u0003 bc\\u001F\"}", str);
}

BOOST_AUTO_TEST_CASE( Json_serialization_escapes_very_large_strings )
{
    StringBuffer buffer;
    JsonWriter writer(buffer);

    std::string largeString(1000000, 'a');

    writer.startObject();
    writer.newPropertyWithSafeName("prop") << ("\n" + largeString + "\n");
    writer.endObject();

    auto str = buffer.view();

    std::string expectedString = "{\"prop\":\"\\n" + largeString + "\\n\"}";

    BOOST_REQUIRE_EQUAL(expectedString, str);
}

BOOST_AUTO_TEST_CASE( JsonReadyString_can_add_quotes_for_strings_that_dont_require_escaping )
{
    for (auto str : { "", "a", "ab", "abc", "abcd", "abcde", "abcdef" })
    {
        auto view = boost::string_view(str);
        auto quotedString = "\"" + std::string(str) + "\"";

        JsonReadyString<5> jsonReadyString(view);
        BOOST_REQUIRE( ! jsonReadyString.needsJsonEscape());
        BOOST_REQUIRE_EQUAL(view, jsonReadyString.string());
        BOOST_REQUIRE_EQUAL(quotedString, jsonReadyString.quotedString());

        JsonReadyString<5> copy(jsonReadyString);
        BOOST_REQUIRE( ! copy.needsJsonEscape());
        BOOST_REQUIRE_EQUAL(view, copy.string());
        BOOST_REQUIRE_EQUAL(quotedString, copy.quotedString());
    }
}

BOOST_AUTO_TEST_CASE( JsonReadyString_does_not_add_quotes_for_strings_that_require_escaping )
{
    for (auto str : { "a\\", "a\\b", "ab\\c", "abc\\d", "abc\\de", "abcd\\ef" })
    {
        auto view = boost::string_view(str);

        JsonReadyString<5> jsonReadyString(view);
        BOOST_REQUIRE(jsonReadyString.needsJsonEscape());
        BOOST_REQUIRE_EQUAL(view, jsonReadyString.string());

        JsonReadyString<5> copy(jsonReadyString);
        BOOST_REQUIRE(copy.needsJsonEscape());
        BOOST_REQUIRE_EQUAL(view, copy.string());
    }
}
