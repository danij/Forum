#include "CommandHandler.h"
#include "TestHelpers.h"
#include "Version.h"
#include "CommandsCommon.h"

#include <boost/test/unit_test.hpp>

using namespace Forum::Helpers;
using namespace Forum::Repository;

BOOST_AUTO_TEST_CASE( Version_is_successfully_returned )
{
    auto handler = createCommandHandler();
    auto versionObj = handlerToObj(handler, Forum::Commands::SHOW_VERSION);
    BOOST_REQUIRE_EQUAL(Forum::VERSION, versionObj.get<std::string>("version"));
}

BOOST_AUTO_TEST_CASE( Executing_a_command_beyond_the_range_of_available_commands_returns_not_found )
{
    auto handler = createCommandHandler();
    assertStatusCodeEqual(StatusCode::NOT_FOUND, 
                          std::get<1>(handlerToObjAndStatus(handler, static_cast<Forum::Commands::Command>(-1))));
    assertStatusCodeEqual(StatusCode::NOT_FOUND, 
                          std::get<1>(handlerToObjAndStatus(handler, static_cast<Forum::Commands::Command>(0xFFFFFF))));
}
