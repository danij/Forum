#pragma once

#include <boost/test/unit_test.hpp>

#include "Repository.h"

namespace Forum
{
    namespace Helpers
    {
        inline void assertStatusCodeEqual(Forum::Repository::StatusCode expected, Forum::Repository::StatusCode actual)
        {
            BOOST_REQUIRE_EQUAL(expected, actual);
        }

        template <typename TStatusObj>
        inline void assertStatusCodeEqual(Forum::Repository::StatusCode expected, const TStatusObj& obj)
        {
            assertStatusCodeEqual(expected, Forum::Repository::StatusCode(obj.template get<uint32_t>("status")));
        }
    }
}
