#include <boost/test/unit_test.hpp>
#include "CommandHandlers.h"
#include "TestHelpers.h"
#include "Version.h"

BOOST_AUTO_TEST_CASE( Version_is_successfully_returned )
{
    auto versionString = Forum::Helpers::handlerToString(Forum::Commands::version);
    BOOST_REQUIRE_EQUAL(versionString, std::string("{\"version\":\"") + Forum::VERSION + "\"}");
}
