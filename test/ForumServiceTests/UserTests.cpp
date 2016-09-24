#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "CommandHandler.h"
#include "TestHelpers.h"
#include "Version.h"
#include "CommandsCommon.h"
#include "Configuration.h"

using namespace Forum::Configuration;
using namespace Forum::Helpers;
using Forum::Repository::StatusCode;

BOOST_AUTO_TEST_CASE( User_count_is_initially_zero )
{
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::COUNT_USERS);
    BOOST_REQUIRE_EQUAL(0, returnObject.get<int>("count"));
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_no_parameters_fails )
{
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_USER);
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_empty_name_fails )
{
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_USER, { "" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_only_whitespace_in_the_name_fails )
{
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_USER, { " \t\r\n" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_leading_whitespace_in_the_name_fails )
{
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_USER, { " Foo" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_trailing_whitespace_in_the_name_fails )
{
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_USER, { "Foo\t" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_leading_nonletter_nonnumber_in_the_name_fails )
{
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_USER, { ":Foo" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_trailing_nonletter_nonnumber_in_the_name_fails )
{
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_USER, { "Foo?" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_whitespace_in_the_middle_of_the_name_succeeds )
{
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_USER, { "Foo Bar" });
    assertStatusCodeEqual(StatusCode::OK, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_dash_in_the_middle_of_the_name_succeeds )
{
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_USER, { "Foo-Bar" });
    assertStatusCodeEqual(StatusCode::OK, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_underscore_in_the_middle_of_the_name_succeeds )
{
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_USER, { "Foo_Bar" });
    assertStatusCodeEqual(StatusCode::OK, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_newline_in_the_middle_of_the_name_fails )
{
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_USER, { "Foo\nBar" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_strange_character_in_the_middle_of_the_name_fails )
{
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_USER, { "Foo☂Bar" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_only_numbers_in_the_name_succeeds )
{
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_USER, { "0123456789" });
    assertStatusCodeEqual(StatusCode::OK, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_accented_letters_in_the_name_succeeds )
{
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_USER, { "FȭǬ" });
    assertStatusCodeEqual(StatusCode::OK, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_a_longer_name_fails )
{
    auto config = getGlobalConfig();
    std::string username(config->user.maxNameLength + 1, 'a');
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_USER, { username });
    assertStatusCodeEqual(StatusCode::VALUE_TOO_LONG, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_unicode_name_of_valid_length_succeeds )
{
    auto oldConfig = getGlobalConfig();
    auto configReset = createDisposer([&](){ setGlobalConfig(*oldConfig); });

    auto configWithShorterName = Forum::Configuration::Config(*oldConfig);
    configWithShorterName.user.maxNameLength = 3;

    setGlobalConfig(configWithShorterName);

    //test a simple text that can also be represented as ASCII
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_USER, { "AAA" });
    assertStatusCodeEqual(StatusCode::OK, returnObject);

    //test a 3 characters text that requires multiple bytes for representation using UTF-8
    returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_USER, { "早上好" });
    assertStatusCodeEqual(StatusCode::OK, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_a_name_that_contains_invalid_characters_fails_with_appropriate_message)
{
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_USER, { "\xFF\xFF" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

static const auto emptyIdString = boost::uuids::to_string(boost::uuids::uuid());

BOOST_AUTO_TEST_CASE( A_user_that_was_created_can_be_retrieved_and_has_a_distinct_id )
{
    auto handler = createCommandHandler();
    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_USER, { "User1" }));
    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_USER, { "User2" }));

    std::vector<std::string> retrievedIds;
    std::vector<std::string> retrievedNames;

    fillPropertyFromCollection(handlerToObj(handler, Forum::Commands::GET_USERS).get_child("users"),
                               "id", std::back_inserter(retrievedIds), std::string());
    fillPropertyFromCollection(handlerToObj(handler, Forum::Commands::GET_USERS).get_child("users"),
                               "name", std::back_inserter(retrievedNames), std::string());

    BOOST_REQUIRE_NE(emptyIdString, retrievedIds[0]);
    BOOST_REQUIRE_NE(emptyIdString, retrievedIds[1]);
    BOOST_REQUIRE_NE(retrievedIds[0], retrievedIds[1]);
    BOOST_REQUIRE_EQUAL("User1", retrievedNames[0]);
    BOOST_REQUIRE_EQUAL("User2", retrievedNames[1]);
}

BOOST_AUTO_TEST_CASE( Users_are_retrieved_by_name )
{
    auto handler = createCommandHandler();
    std::vector<std::string> names = { "Abc", "Ghi", "Def" };

    for (auto& name : names)
    {
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_USER, { name }));
    }

    std::vector<std::string> retrievedNames;
    fillPropertyFromCollection(handlerToObj(handler, Forum::Commands::GET_USERS).get_child("users"),
                               "name", std::back_inserter(retrievedNames), std::string());

    BOOST_REQUIRE_EQUAL(names.size(), retrievedNames.size());
    BOOST_REQUIRE_EQUAL("Abc", retrievedNames[0]);
    BOOST_REQUIRE_EQUAL("Def", retrievedNames[1]);
    BOOST_REQUIRE_EQUAL("Ghi", retrievedNames[2]);
}
