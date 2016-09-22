#include <boost/test/unit_test.hpp>

#include "CommandHandler.h"
#include "TestHelpers.h"
#include "Version.h"
#include "CommandsCommon.h"

using namespace Forum::Helpers;
using Forum::Repository::StatusCode;

BOOST_AUTO_TEST_CASE( User_count_is_initially_zero )
{
    auto handler = createCommandHandler();
    auto versionObj = handlerToObj(handler, Forum::Commands::COUNT_USERS);
    BOOST_REQUIRE_EQUAL(0, versionObj.get<int>("count"));
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_no_parameters_fails )
{
    auto handler = createCommandHandler();
    auto versionObj = handlerToObj(handler, Forum::Commands::ADD_USER);
    BOOST_REQUIRE_EQUAL(int(StatusCode::INVALID_PARAMETERS), versionObj.get<int>("status"));
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_empty_name_fails )
{
    auto handler = createCommandHandler();
    auto versionObj = handlerToObj(handler, Forum::Commands::ADD_USER, { "" });
    BOOST_REQUIRE_EQUAL(int(StatusCode::INVALID_PARAMETERS), versionObj.get<int>("status"));
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_whitespace_name_fails )
{
    auto handler = createCommandHandler();
    auto versionObj = handlerToObj(handler, Forum::Commands::ADD_USER, { " \t\r\n" });
    BOOST_REQUIRE_EQUAL(int(StatusCode::INVALID_PARAMETERS), versionObj.get<int>("status"));
}

