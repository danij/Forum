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
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, versionObj);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_empty_name_fails )
{
    auto handler = createCommandHandler();
    auto versionObj = handlerToObj(handler, Forum::Commands::ADD_USER, { "" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, versionObj);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_only_whitespace_in_the_name_fails )
{
    auto handler = createCommandHandler();
    auto versionObj = handlerToObj(handler, Forum::Commands::ADD_USER, { " \t\r\n" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, versionObj);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_leading_whitespace_in_the_name_fails )
{
    auto handler = createCommandHandler();
    auto versionObj = handlerToObj(handler, Forum::Commands::ADD_USER, { " Foo" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, versionObj);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_trailing_whitespace_in_the_name_fails )
{
    auto handler = createCommandHandler();
    auto versionObj = handlerToObj(handler, Forum::Commands::ADD_USER, { "Foo\t" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, versionObj);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_leading_nonletter_nonnumber_in_the_name_fails )
{
    auto handler = createCommandHandler();
    auto versionObj = handlerToObj(handler, Forum::Commands::ADD_USER, { ":Foo" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, versionObj);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_trailing_nonletter_nonnumber_in_the_name_fails )
{
    auto handler = createCommandHandler();
    auto versionObj = handlerToObj(handler, Forum::Commands::ADD_USER, { "Foo?" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, versionObj);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_whitespace_in_the_middle_of_the_name_succeeds )
{
    auto handler = createCommandHandler();
    auto versionObj = handlerToObj(handler, Forum::Commands::ADD_USER, { "Foo Bar" });
    assertStatusCodeEqual(StatusCode::OK, versionObj);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_dash_in_the_middle_of_the_name_succeeds )
{
    auto handler = createCommandHandler();
    auto versionObj = handlerToObj(handler, Forum::Commands::ADD_USER, { "Foo-Bar" });
    assertStatusCodeEqual(StatusCode::OK, versionObj);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_underscore_in_the_middle_of_the_name_succeeds )
{
    auto handler = createCommandHandler();
    auto versionObj = handlerToObj(handler, Forum::Commands::ADD_USER, { "Foo_Bar" });
    assertStatusCodeEqual(StatusCode::OK, versionObj);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_newline_in_the_middle_of_the_name_fails )
{
    auto handler = createCommandHandler();
    auto versionObj = handlerToObj(handler, Forum::Commands::ADD_USER, { "Foo\nBar" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, versionObj);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_strange_character_in_the_middle_of_the_name_fails )
{
    auto handler = createCommandHandler();
    auto versionObj = handlerToObj(handler, Forum::Commands::ADD_USER, { "Foo☂Bar" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, versionObj);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_only_numbers_in_the_name_succeeds )
{
    auto handler = createCommandHandler();
    auto versionObj = handlerToObj(handler, Forum::Commands::ADD_USER, { "0123456789" });
    assertStatusCodeEqual(StatusCode::OK, versionObj);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_accented_letters_in_the_name_succeeds )
{
    auto handler = createCommandHandler();
    auto versionObj = handlerToObj(handler, Forum::Commands::ADD_USER, { "FȭǬ" });
    assertStatusCodeEqual(StatusCode::OK, versionObj);
}
