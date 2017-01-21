#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE CppTests

#include <boost/test/unit_test.hpp>

#include <unicode/uclean.h>

#include "StringHelpers.h"

struct ICUCleanupFixture
{
    ~ICUCleanupFixture()
    {
        Forum::Helpers::cleanupStringHelpers();

        //clean up resources cached by ICU so that they don't show up as memory leaks
        u_cleanup();
    }
};

BOOST_GLOBAL_FIXTURE(ICUCleanupFixture);
