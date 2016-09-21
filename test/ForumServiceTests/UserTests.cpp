#include <boost/test/unit_test.hpp>

#include "CommandHandler.h"
#include "TestHelpers.h"
#include "Version.h"
#include "CommandsCommon.h"

using namespace Forum::Helpers;

BOOST_AUTO_TEST_CASE( User_count_is_initially_zero )
{
    auto handler = createCommandHandler();
    auto versionObj = handlerToObj(handler, Forum::Commands::COUNT_USERS);
    BOOST_REQUIRE_EQUAL(0, versionObj.get<int>("count"));
}
