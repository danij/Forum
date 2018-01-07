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

#include <boost/test/unit_test.hpp>

#include "SortedVector.h"

using namespace Forum::Entities;

struct Foo final
{
    explicit Foo(const int value) : Foo(value, {}) {}
    explicit Foo(const int value, const int extra) : value_(value), extra_(extra) {}
    Foo(const Foo&) = default;
    Foo(Foo&&) = default;
    ~Foo() = default;
    Foo& operator=(const Foo&) = default;
    Foo& operator=(Foo&&) = default;

    int getValue() const { return value_; }
    int getExtra() const { return extra_; }

private:
    int value_;
    int extra_;
};

struct FooValueExtractor
{
    auto operator()(const Foo& value) const
    {
        return value.getValue();
    }
};

struct FooValueCompare
{
    auto operator()(const Foo& first, const Foo& second) const
    {
        return first.getValue() < second.getValue();
    }
    auto operator()(int value, const Foo& foo) const
    {
        return value < foo.getValue();
    }
    auto operator()(const Foo& foo, int value) const
    {
        return foo.getValue() < value;
    }
};

BOOST_AUTO_TEST_CASE( SortedVectorMultiValue_returns_an_iterator_to_the_inserted_elements )
{
    SortedVectorMultiValue<Foo, int, FooValueExtractor, FooValueCompare> vector;
    {
        const auto it1 = vector.insert(Foo(1));
        BOOST_REQUIRE_EQUAL(1, it1->getValue());
        BOOST_REQUIRE_EQUAL(true, it1 == vector.begin());
    }
    {
        const auto it3 = vector.insert(Foo(3));
        BOOST_REQUIRE_EQUAL(3, it3->getValue());
        BOOST_REQUIRE_EQUAL(true, it3 > vector.begin());
    }
    {
        const auto it11 = vector.insert(Foo(1));
        BOOST_REQUIRE_EQUAL(1, it11->getValue());
        BOOST_REQUIRE_EQUAL(true, it11 > vector.begin());
    }
}

BOOST_AUTO_TEST_CASE( SortedVectorMultiValue_can_retrieve_items_in_sorted_order )
{
    SortedVectorMultiValue<Foo, int, FooValueExtractor, FooValueCompare> vector;
    vector.insert(Foo(1));
    vector.insert(Foo(3));
    vector.insert(Foo(2));

    BOOST_REQUIRE_EQUAL(3, vector.size());

    std::vector<int> values(3);
    std::transform(vector.begin(), vector.end(), values.begin(), [](const Foo& foo)
    {
        return foo.getValue();
    });

    BOOST_REQUIRE_EQUAL(1, values[0]);
    BOOST_REQUIRE_EQUAL(2, values[1]);
    BOOST_REQUIRE_EQUAL(3, values[2]);
}

BOOST_AUTO_TEST_CASE( SortedVectorUnique_can_find_items_by_value_or_by_comparable_key )
{
    SortedVectorUnique<Foo, int, FooValueExtractor, FooValueCompare> vector;
    vector.insert(Foo(1));
    vector.insert(Foo(4));
    vector.insert(Foo(2));

    const auto find1ByValue = vector.find(Foo(1));
    const auto find1ByKey = vector.find(1);

    BOOST_REQUIRE_EQUAL(true, find1ByKey == find1ByValue);
    BOOST_REQUIRE_EQUAL(true, 1 == find1ByValue->getValue());

    BOOST_REQUIRE_EQUAL(true, vector.find(3) == vector.end());
    BOOST_REQUIRE_EQUAL(true, 4 == vector.find(4)->getValue());
}

BOOST_AUTO_TEST_CASE( SortedVectorMultiValue_can_return_an_equal_range )
{
    SortedVectorMultiValue<Foo, int, FooValueExtractor, FooValueCompare> vector;
    vector.insert(Foo(1));
    vector.insert(Foo(4));
    vector.insert(Foo(2));
    vector.insert(Foo(1));

    const auto rangeByValue = vector.equal_range(Foo(1));
    const auto rangeByKey = vector.equal_range(1);

    BOOST_REQUIRE_EQUAL(true, rangeByValue == rangeByKey);
    BOOST_REQUIRE_EQUAL(2, rangeByValue.second - rangeByValue.first);
    BOOST_REQUIRE_EQUAL(1, rangeByValue.first->getValue());
    BOOST_REQUIRE_EQUAL(1, (rangeByValue.first + 1)->getValue());
}

BOOST_AUTO_TEST_CASE( SortedVectorMultiValue_can_remove_single_items )
{
    SortedVectorMultiValue<Foo, int, FooValueExtractor, FooValueCompare> vector;
    vector.insert(Foo(1));
    vector.insert(Foo(4));
    vector.insert(Foo(2));
    vector.insert(Foo(1));

    {
        const auto afterErase2 = vector.erase(vector.equal_range(2).first);
        BOOST_REQUIRE_EQUAL(true, vector.equal_range(4).first == afterErase2);
    }
    BOOST_REQUIRE_EQUAL(3, vector.size());
    BOOST_REQUIRE_EQUAL(1, vector.begin()->getValue());
    BOOST_REQUIRE_EQUAL(1, (vector.begin() + 1)->getValue());
    BOOST_REQUIRE_EQUAL(4, (vector.begin() + 2)->getValue());

    {
        const auto afterErase4 = vector.erase(vector.equal_range(4).first);
        BOOST_REQUIRE_EQUAL(true, vector.end() == afterErase4);
    }
    BOOST_REQUIRE_EQUAL(2, vector.size());
    BOOST_REQUIRE_EQUAL(1, vector.begin()->getValue());
    BOOST_REQUIRE_EQUAL(1, (vector.begin() + 1)->getValue());
}

BOOST_AUTO_TEST_CASE( SortedVectorMultiValue_can_remove_multiple_items )
{
    SortedVectorMultiValue<Foo, int, FooValueExtractor, FooValueCompare> vector;
    vector.insert(Foo(1));
    vector.insert(Foo(4));
    vector.insert(Foo(2));
    vector.insert(Foo(1));

    const auto range = vector.equal_range(1);
    {
        const auto afterErase = vector.erase(range.first, range.second);
        BOOST_REQUIRE_EQUAL(true, vector.equal_range(2).first == afterErase);
    }
    BOOST_REQUIRE_EQUAL(2, vector.size());
    BOOST_REQUIRE_EQUAL(2, vector.begin()->getValue());
    BOOST_REQUIRE_EQUAL(4, (vector.begin() + 1)->getValue());
}

BOOST_AUTO_TEST_CASE( SortedVectorMultiValue_can_replace_single_items )
{
    SortedVectorMultiValue<Foo, int, FooValueExtractor, FooValueCompare> vector;
    vector.insert(Foo(1));
    vector.insert(Foo(6));
    vector.insert(Foo(2));
    vector.insert(Foo(4));
    vector.insert(Foo(1));

    const auto afterReplace = vector.replace(vector.equal_range(2).first, Foo(5));
    BOOST_REQUIRE_EQUAL(true, afterReplace == (vector.begin() + 3));

    BOOST_REQUIRE_EQUAL(5, vector.size());
    BOOST_REQUIRE_EQUAL(1, vector.begin()->getValue());
    BOOST_REQUIRE_EQUAL(1, (vector.begin() + 1)->getValue());
    BOOST_REQUIRE_EQUAL(4, (vector.begin() + 2)->getValue());
    BOOST_REQUIRE_EQUAL(5, (vector.begin() + 3)->getValue());
    BOOST_REQUIRE_EQUAL(6, (vector.begin() + 4)->getValue());
}

BOOST_AUTO_TEST_CASE( SortedVectorMultiValue_can_replace_single_items_that_keep_their_order )
{
    SortedVectorMultiValue<Foo, int, FooValueExtractor, FooValueCompare> vector;
    vector.insert(Foo(1));
    vector.insert(Foo(5));
    vector.insert(Foo(2));
    vector.insert(Foo(4));
    vector.insert(Foo(1));

    const auto afterReplace = vector.replace(vector.equal_range(2).first, Foo(3));
    BOOST_REQUIRE_EQUAL(true, afterReplace == (vector.begin() + 2));

    BOOST_REQUIRE_EQUAL(5, vector.size());
    BOOST_REQUIRE_EQUAL(1, vector.begin()->getValue());
    BOOST_REQUIRE_EQUAL(1, (vector.begin() + 1)->getValue());
    BOOST_REQUIRE_EQUAL(3, (vector.begin() + 2)->getValue());
    BOOST_REQUIRE_EQUAL(4, (vector.begin() + 3)->getValue());
    BOOST_REQUIRE_EQUAL(5, (vector.begin() + 4)->getValue());
}

BOOST_AUTO_TEST_CASE( SortedVectorMultiValue_can_replace_single_items_when_there_is_only_one_item )
{
    SortedVectorMultiValue<Foo, int, FooValueExtractor, FooValueCompare> vector;
    vector.insert(Foo(1));

    const auto afterReplace = vector.replace(vector.equal_range(1).first, Foo(2));
    BOOST_REQUIRE_EQUAL(true, afterReplace == vector.begin());

    BOOST_REQUIRE_EQUAL(1, vector.size());
    BOOST_REQUIRE_EQUAL(2, vector.begin()->getValue());
}

BOOST_AUTO_TEST_CASE( SortedVectorMultiValue_can_replace_single_items_so_that_they_end_up_at_the_beginning )
{
    SortedVectorMultiValue<Foo, int, FooValueExtractor, FooValueCompare> vector;
    vector.insert(Foo(1));
    vector.insert(Foo(4));
    vector.insert(Foo(2));
    vector.insert(Foo(1));

    const auto afterReplace = vector.replace(vector.equal_range(2).first, Foo(0));
    BOOST_REQUIRE_EQUAL(true, vector.begin() == afterReplace);

    BOOST_REQUIRE_EQUAL(4, vector.size());
    BOOST_REQUIRE_EQUAL(0, vector.begin()->getValue());
    BOOST_REQUIRE_EQUAL(1, (vector.begin() + 1)->getValue());
    BOOST_REQUIRE_EQUAL(1, (vector.begin() + 2)->getValue());
    BOOST_REQUIRE_EQUAL(4, (vector.begin() + 3)->getValue());
}

BOOST_AUTO_TEST_CASE( SortedVectorMultiValue_can_replace_single_items_so_that_they_end_up_at_the_end )
{
    SortedVectorMultiValue<Foo, int, FooValueExtractor, FooValueCompare> vector;
    vector.insert(Foo(1));
    vector.insert(Foo(4));
    vector.insert(Foo(2));
    vector.insert(Foo(1));

    const auto afterReplace = vector.replace(vector.equal_range(1).first, Foo(10));
    BOOST_REQUIRE_EQUAL(true, (vector.begin() + 3) == afterReplace);

    BOOST_REQUIRE_EQUAL(4, vector.size());
    BOOST_REQUIRE_EQUAL(1, vector.begin()->getValue());
    BOOST_REQUIRE_EQUAL(2, (vector.begin() + 1)->getValue());
    BOOST_REQUIRE_EQUAL(4, (vector.begin() + 2)->getValue());
    BOOST_REQUIRE_EQUAL(10, (vector.begin() + 3)->getValue());
}

BOOST_AUTO_TEST_CASE( SortedVectorMultiValue_can_return_the_nth_item )
{
    SortedVectorMultiValue<Foo, int, FooValueExtractor, FooValueCompare> emptyVector;

    BOOST_REQUIRE_EQUAL(true, emptyVector.nth(0) == emptyVector.end());
    BOOST_REQUIRE_EQUAL(true, emptyVector.nth(1) == emptyVector.end());
    BOOST_REQUIRE_EQUAL(true, emptyVector.nth(2) == emptyVector.end());

    SortedVectorMultiValue<Foo, int, FooValueExtractor, FooValueCompare> vector;
    vector.insert(Foo(1));
    vector.insert(Foo(4));
    vector.insert(Foo(2));
    vector.insert(Foo(1));

    BOOST_REQUIRE_EQUAL(true, vector.nth(0) == vector.equal_range(1).first);
    BOOST_REQUIRE_EQUAL(1, vector.nth(0)->getValue());
    BOOST_REQUIRE_EQUAL(1, vector.nth(1)->getValue());
    BOOST_REQUIRE_EQUAL(2, vector.nth(2)->getValue());
    BOOST_REQUIRE_EQUAL(4, vector.nth(3)->getValue());
    BOOST_REQUIRE_EQUAL(true, vector.nth(5) == vector.end());
    BOOST_REQUIRE_EQUAL(true, vector.nth(6) == vector.end());
}

BOOST_AUTO_TEST_CASE( SortedVectorUnique_can_return_the_lower_bound )
{
    SortedVectorUnique<Foo, int, FooValueExtractor, FooValueCompare> vector;
    vector.insert(Foo(1));
    vector.insert(Foo(4));
    vector.insert(Foo(2));

    const auto lowerBound1ByValue = vector.lower_bound(Foo(1));
    const auto lowerBound1ByKey = vector.lower_bound(1);

    BOOST_REQUIRE_EQUAL(true, lowerBound1ByKey == lowerBound1ByValue);
    BOOST_REQUIRE_EQUAL(true, 1 == lowerBound1ByValue->getValue());

    BOOST_REQUIRE_EQUAL(true, vector.lower_bound(3) == vector.find(4));
    BOOST_REQUIRE_EQUAL(true, 4 == vector.lower_bound(4)->getValue());
    BOOST_REQUIRE_EQUAL(true, vector.lower_bound(5) == vector.end());
}

BOOST_AUTO_TEST_CASE( SortedVectorMultiValue_can_return_the_rank_of_the_lower_bound )
{
    SortedVectorMultiValue<Foo, int, FooValueExtractor, FooValueCompare> vector;
    vector.insert(Foo(1));
    vector.insert(Foo(4));
    vector.insert(Foo(2));

    const auto lowerBoundRank1ByValue = vector.lower_bound_rank(Foo(1));
    const auto lowerBoundRank1ByKey = vector.lower_bound_rank(1);

    BOOST_REQUIRE_EQUAL(lowerBoundRank1ByKey, lowerBoundRank1ByValue);
    BOOST_REQUIRE_EQUAL(0, lowerBoundRank1ByValue);

    BOOST_REQUIRE_EQUAL(2, vector.lower_bound_rank(3));
    BOOST_REQUIRE_EQUAL(2, vector.lower_bound_rank(4));
    BOOST_REQUIRE_EQUAL(3, vector.lower_bound_rank(5));
}

BOOST_AUTO_TEST_CASE( SortedVectorUnique_can_return_the_upper_bound )
{
    SortedVectorUnique<Foo, int, FooValueExtractor, FooValueCompare> vector;
    vector.insert(Foo(1));
    vector.insert(Foo(4));
    vector.insert(Foo(2));

    const auto upperBound1ByValue = vector.upper_bound(Foo(1));
    const auto upperBound1ByKey = vector.upper_bound(1);

    BOOST_REQUIRE_EQUAL(true, upperBound1ByKey == upperBound1ByValue);
    BOOST_REQUIRE_EQUAL(true, 2 == upperBound1ByValue->getValue());

    BOOST_REQUIRE_EQUAL(true, vector.upper_bound(3) == vector.find(4));
    BOOST_REQUIRE_EQUAL(true, vector.upper_bound(4) == vector.end());
    BOOST_REQUIRE_EQUAL(true, vector.upper_bound(5) == vector.end());
}

BOOST_AUTO_TEST_CASE( SortedVectorUnique_returns_an_iterator_to_the_inserted_or_found_elements )
{
    SortedVectorUnique<Foo, int, FooValueExtractor, FooValueCompare> vector;
    {
        const auto pair1 = vector.insert(Foo(1));
        BOOST_REQUIRE_EQUAL(true, pair1.second);
        BOOST_REQUIRE_EQUAL(1, pair1.first->getValue());
        BOOST_REQUIRE_EQUAL(true, pair1.first == vector.begin());
    }
    {
        const auto pair3 = vector.insert(Foo(3));
        BOOST_REQUIRE_EQUAL(true, pair3.second);
        BOOST_REQUIRE_EQUAL(3, pair3.first->getValue());
        BOOST_REQUIRE_EQUAL(true, pair3.first > vector.begin());
    }
    {
        const auto pair1 = vector.insert(Foo(1));
        BOOST_REQUIRE_EQUAL(false, pair1.second);
        BOOST_REQUIRE_EQUAL(1, pair1.first->getValue());
        BOOST_REQUIRE_EQUAL(true, pair1.first == vector.begin());
    }
}

BOOST_AUTO_TEST_CASE( SortedVectorUnique_removes_the_item_on_replacement_if_a_unique_constraint_fails )
{
    SortedVectorUnique<Foo, int, FooValueExtractor, FooValueCompare> vector;
    vector.insert(Foo(1, 1));
    vector.insert(Foo(4, 4));
    vector.insert(Foo(2, 2));

    const auto afterReplace = vector.replace(vector.find(2), Foo(1, 100));
    BOOST_REQUIRE_EQUAL(true, afterReplace == (vector.begin() + 1));

    BOOST_REQUIRE_EQUAL(2, vector.size());
    BOOST_REQUIRE_EQUAL(1, vector.begin()->getValue());
    BOOST_REQUIRE_EQUAL(1, vector.begin()->getExtra());
    BOOST_REQUIRE_EQUAL(4, (vector.begin() + 1)->getValue());
    BOOST_REQUIRE_EQUAL(4, (vector.begin() + 1)->getExtra());
}
