/*
Fast Forum Backend
Copyright (C) Daniel Jurcau

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

#include "Trie.h"

#include <map>
#include <string>

#include <boost/test/unit_test.hpp>

using namespace Http;
using namespace std::literals::string_literals;

BOOST_AUTO_TEST_CASE( Empty_ImmutableTrie_does_not_match_any_keys )
{
    ImmutableTrie<char, int> trie;

    BOOST_REQUIRE_EQUAL(nullptr, trie.find(std::string_view{ "test" }));
}

BOOST_AUTO_TEST_CASE( ImmutableTrie_can_be_created_using_iterator_pairs )
{
    std::map<std::string, int> values
    {
        std::make_pair("abc"s, 1),
        std::make_pair("abcd"s, 2)
    };
    ImmutableTrie<char, int> trie{ values.cbegin(), values.cend() };

    BOOST_REQUIRE_EQUAL(2u, trie.size());
    BOOST_REQUIRE_EQUAL(nullptr, trie.find("test"s));
}

BOOST_AUTO_TEST_CASE( ImmutableTrie_can_be_created_using_initializer_list )
{
    ImmutableTrie<char, int> trie
    {
        std::make_pair("abc"s, 1),
        std::make_pair("abcd"s, 2),
        std::make_pair("bcde"s, 2)
    };

    BOOST_REQUIRE_EQUAL(3u, trie.size());
    BOOST_REQUIRE_EQUAL(nullptr, trie.find("test"s));

}

BOOST_AUTO_TEST_CASE( ImmutableTrie_only_finds_expected_values )
{
    ImmutableTrie<char, int> trie
    {
        std::make_pair("abc"s, 1),
        std::make_pair("abcd"s, 2),
        std::make_pair("bcde"s, 2)
    };

    BOOST_REQUIRE_EQUAL(nullptr, trie.find("a"s));
    BOOST_REQUIRE_EQUAL(nullptr, trie.find("A"s));
    BOOST_REQUIRE_EQUAL(nullptr, trie.find("b"s));
    BOOST_REQUIRE_EQUAL(nullptr, trie.find("ab"s));

    BOOST_REQUIRE_NE(nullptr, trie.find("abc"s));
    BOOST_REQUIRE_EQUAL(1, *trie.find("abc"s));
    
    BOOST_REQUIRE_NE(nullptr, trie.find("abcd"s));
    BOOST_REQUIRE_EQUAL(2, *trie.find("abcd"s));

    BOOST_REQUIRE_EQUAL(nullptr, trie.find("abcde"s));
    BOOST_REQUIRE_EQUAL(nullptr, trie.find("acde"s));
    BOOST_REQUIRE_EQUAL(nullptr, trie.find("bcd"s));

    BOOST_REQUIRE_NE(nullptr, trie.find("bcde"s));
    BOOST_REQUIRE_EQUAL(2, *trie.find("bcde"s));
}

BOOST_AUTO_TEST_CASE( ImmutableAsciiCaseInsensitiveTrie_only_finds_expected_values )
{
    ImmutableAsciiCaseInsensitiveTrie<int> trie
    {
        std::make_pair("abc"s, 1),
        std::make_pair("abcd"s, 2),
        std::make_pair("abcd-e"s, 2),
        std::make_pair("bcde"s, 3)
    };

    BOOST_REQUIRE_EQUAL(nullptr, trie.find("a"s));
    BOOST_REQUIRE_EQUAL(nullptr, trie.find("A"s));
    BOOST_REQUIRE_EQUAL(nullptr, trie.find("b"s));
    BOOST_REQUIRE_EQUAL(nullptr, trie.find("ab"s));

    BOOST_REQUIRE_NE(nullptr, trie.find("abc"s));
    BOOST_REQUIRE_EQUAL(1, *trie.find("abc"s));
    
    BOOST_REQUIRE_NE(nullptr, trie.find("aBC"s));
    BOOST_REQUIRE_EQUAL(1, *trie.find("aBC"s));
    
    BOOST_REQUIRE_NE(nullptr, trie.find("abcd"s));
    BOOST_REQUIRE_EQUAL(2, *trie.find("abcd"s));

    BOOST_REQUIRE_NE(nullptr, trie.find("AbCd"s));
    BOOST_REQUIRE_EQUAL(2, *trie.find("AbCd"s));

    BOOST_REQUIRE_NE(nullptr, trie.find("AbCD-E"s));
    BOOST_REQUIRE_EQUAL(2, *trie.find("AbCD-E"s));

    BOOST_REQUIRE_EQUAL(nullptr, trie.find("aBCde"s));
    BOOST_REQUIRE_EQUAL(nullptr, trie.find("aCde"s));
    BOOST_REQUIRE_EQUAL(nullptr, trie.find("bcd"s));

    BOOST_REQUIRE_NE(nullptr, trie.find("BCDE"s));
    BOOST_REQUIRE_EQUAL(3, *trie.find("BCDE"s));
}
