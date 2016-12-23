#include "CommandHandler.h"
#include "TestHelpers.h"
#include "Version.h"
#include "CommandsCommon.h"

#include <boost/test/unit_test.hpp>

using namespace Forum::Helpers;

BOOST_AUTO_TEST_CASE( Version_is_successfully_returned )
{
    auto handler = createCommandHandler();
    auto versionObj = handlerToObj(handler, Forum::Commands::SHOW_VERSION);
    BOOST_REQUIRE_EQUAL(Forum::VERSION, versionObj.get<std::string>("version"));
}
