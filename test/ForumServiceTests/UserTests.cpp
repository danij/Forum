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

BOOST_AUTO_TEST_CASE( Creating_a_user_with_a_too_short_name_fails )
{
    auto config = getGlobalConfig();
    std::string username(config->user.minNameLength - 1, 'a');
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_USER, { username });
    assertStatusCodeEqual(StatusCode::VALUE_TOO_SHORT, returnObject);
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

BOOST_AUTO_TEST_CASE( Adding_multiple_users_with_same_name_fails )
{
    auto handler = createCommandHandler();

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_USER, { "Abc" }));
    assertStatusCodeEqual(StatusCode::ALREADY_EXISTS, handlerToObj(handler, Forum::Commands::ADD_USER, { "Abc" }));

    std::vector<std::string> retrievedNames;
    fillPropertyFromCollection(handlerToObj(handler, Forum::Commands::GET_USERS).get_child("users"),
                               "name", std::back_inserter(retrievedNames), std::string());

    BOOST_REQUIRE_EQUAL(1, retrievedNames.size());
    BOOST_REQUIRE_EQUAL("Abc", retrievedNames[0]);
}

BOOST_AUTO_TEST_CASE( Adding_multiple_users_with_same_name_but_different_case_fails )
{
    auto handler = createCommandHandler();

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_USER, { "Abc" }));
    assertStatusCodeEqual(StatusCode::ALREADY_EXISTS, handlerToObj(handler, Forum::Commands::ADD_USER, { "ABC" }));

    std::vector<std::string> retrievedNames;
    fillPropertyFromCollection(handlerToObj(handler, Forum::Commands::GET_USERS).get_child("users"),
                               "name", std::back_inserter(retrievedNames), std::string());

    BOOST_REQUIRE_EQUAL(1, retrievedNames.size());
    BOOST_REQUIRE_EQUAL("Abc", retrievedNames[0]);
}

BOOST_AUTO_TEST_CASE( Adding_multiple_users_with_same_name_but_different_accents_fails )
{
    auto handler = createCommandHandler();

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_USER, { "HélĹǬ" }));
    assertStatusCodeEqual(StatusCode::ALREADY_EXISTS, handlerToObj(handler, Forum::Commands::ADD_USER, { "Hello" }));

    std::vector<std::string> retrievedNames;
    fillPropertyFromCollection(handlerToObj(handler, Forum::Commands::GET_USERS).get_child("users"),
                               "name", std::back_inserter(retrievedNames), std::string());

    BOOST_REQUIRE_EQUAL(1, retrievedNames.size());
    BOOST_REQUIRE_EQUAL("HélĹǬ", retrievedNames[0]);
}

BOOST_AUTO_TEST_CASE( Missing_users_retrieved_by_name_returns_not_found )
{
    auto handler = createCommandHandler();
    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_USER, { "Abc" }));
    assertStatusCodeEqual(StatusCode::NOT_FOUND, handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "Ghi" }));
}

BOOST_AUTO_TEST_CASE( Users_can_be_retrieved_by_name )
{
    auto handler = createCommandHandler();
    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_USER, { "Abc" }));

    auto user = handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "Abc" });

    BOOST_REQUIRE_EQUAL(false, user.get<std::string>("user.id").empty());
    BOOST_REQUIRE_EQUAL("Abc", user.get<std::string>("user.name"));
}

BOOST_AUTO_TEST_CASE( Users_can_be_retrieved_by_name_case_and_accent_insensitive )
{
    auto handler = createCommandHandler();
    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_USER, { "HélĹǬ" }));

    auto user = handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "Hello" });

    BOOST_REQUIRE_EQUAL(false, user.get<std::string>("user.id").empty());
    BOOST_REQUIRE_EQUAL("HélĹǬ", user.get<std::string>("user.name"));
}

BOOST_AUTO_TEST_CASE( Modifying_a_user_name_succeds )
{
    auto handler = createCommandHandler();
    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_USER, { "Abc" }));

    auto user = handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "Abc" });
    BOOST_REQUIRE_EQUAL("Abc", user.get<std::string>("user.name"));
    auto userId = user.get<std::string>("user.id");

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::CHANGE_USER_NAME, { userId, "Xyz" }));
    auto modifiedUser = handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "Xyz" });

    BOOST_REQUIRE_EQUAL("Xyz", modifiedUser.get<std::string>("user.name"));
    BOOST_REQUIRE_EQUAL(userId, modifiedUser.get<std::string>("user.id"));
}

BOOST_AUTO_TEST_CASE( Modifying_a_user_name_with_an_already_existent_value_fails )
{
    auto handler = createCommandHandler();
    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_USER, { "Abc" }));
    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_USER, { "Def" }));

    auto user = handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "Abc" });
    auto userId = user.get<std::string>("user.id");
    assertStatusCodeEqual(StatusCode::ALREADY_EXISTS,
                          handlerToObj(handler, Forum::Commands::CHANGE_USER_NAME, { userId, "Def" }));
}

BOOST_AUTO_TEST_CASE( Modifying_an_inexistent_user_name_returns_not_found )
{
    auto handler = createCommandHandler();
    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_USER, { "Abc" }));
    assertStatusCodeEqual(StatusCode::NOT_FOUND,
                          handlerToObj(handler, Forum::Commands::CHANGE_USER_NAME, { "bogus id", "Xyz" }));
}

BOOST_AUTO_TEST_CASE( Modifying_a_user_name_reorders_users )
{
    auto handler = createCommandHandler();
    std::vector<std::string> names = { "Abc", "Ghi", "Def" };

    for (auto& name : names)
    {
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_USER, { name }));
    }

    auto user = handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "Abc" });
    BOOST_REQUIRE_EQUAL("Abc", user.get<std::string>("user.name"));
    auto userId = user.get<std::string>("user.id");

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::CHANGE_USER_NAME, { userId, "Xyz" }));

    std::vector<std::string> retrievedNames;
    fillPropertyFromCollection(handlerToObj(handler, Forum::Commands::GET_USERS).get_child("users"),
                               "name", std::back_inserter(retrievedNames), std::string());

    BOOST_REQUIRE_EQUAL(names.size(), retrievedNames.size());
    BOOST_REQUIRE_EQUAL("Def", retrievedNames[0]);
    BOOST_REQUIRE_EQUAL("Ghi", retrievedNames[1]);
    BOOST_REQUIRE_EQUAL("Xyz", retrievedNames[2]);
}

BOOST_AUTO_TEST_CASE( Deleting_an_inexistent_user_name_returns_not_found )
{
    auto handler = createCommandHandler();
    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_USER, { "Abc" }));
    assertStatusCodeEqual(StatusCode::NOT_FOUND,
                          handlerToObj(handler, Forum::Commands::DELETE_USER, { "bogus id" }));
}

BOOST_AUTO_TEST_CASE( Deleted_users_can_no_longer_be_retrieved )
{
    auto handler = createCommandHandler();
    std::vector<std::string> names = { "Abc", "Ghi", "Def" };

    for (auto& name : names)
    {
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_USER, { name }));
    }

    auto user = handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "Abc" });
    BOOST_REQUIRE_EQUAL("Abc", user.get<std::string>("user.name"));
    auto userId = user.get<std::string>("user.id");

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::DELETE_USER, { userId }));
    assertStatusCodeEqual(StatusCode::NOT_FOUND, handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "Abc" }));

    std::vector<std::string> retrievedNames;
    fillPropertyFromCollection(handlerToObj(handler, Forum::Commands::GET_USERS).get_child("users"),
                               "name", std::back_inserter(retrievedNames), std::string());

    BOOST_REQUIRE_EQUAL(names.size() - 1, retrievedNames.size());
    BOOST_REQUIRE_EQUAL("Def", retrievedNames[0]);
    BOOST_REQUIRE_EQUAL("Ghi", retrievedNames[1]);
}
