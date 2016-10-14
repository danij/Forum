#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "CommandsCommon.h"
#include "CommandHandler.h"
#include "DelegateObserver.h"
#include "TestHelpers.h"
#include "Version.h"

using namespace Forum::Configuration;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;

BOOST_AUTO_TEST_CASE( User_count_is_initially_zero )
{
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::COUNT_USERS);
    BOOST_REQUIRE_EQUAL(0, returnObject.get<int>("count"));
}

BOOST_AUTO_TEST_CASE( Counting_users_invokes_observer )
{
    bool observerCalled = false;
    auto handler = createCommandHandler();

    DisposingDelegateObserver observer(*handler);
    observer->getUserCountAction = [&](auto& _) { observerCalled = true; };

    handlerToObj(handler, Forum::Commands::COUNT_USERS);
    BOOST_REQUIRE(observerCalled);
}

BOOST_AUTO_TEST_CASE( Retrieving_users_invokes_observer )
{
    bool observerCalled = false;
    auto handler = createCommandHandler();

    DisposingDelegateObserver observer(*handler);
    observer->getUsersAction = [&](auto& _) { observerCalled = true; };

    handlerToObj(handler, Forum::Commands::GET_USERS_BY_NAME);
    BOOST_REQUIRE(observerCalled);
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

BOOST_AUTO_TEST_CASE( Creating_a_user_invokes_observer )
{
    std::string newUserName;
    auto handler = createCommandHandler();

    DisposingDelegateObserver observer(*handler);
    observer->addNewUserAction = [&](auto& _, auto& newUser)
    {
        newUserName = newUser.name();
    };

    handlerToObj(handler, Forum::Commands::ADD_USER, { "Foo" });
    BOOST_REQUIRE_EQUAL("Foo", newUserName);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_returns_the_id_name_and_created )
{
    TimestampChanger changer(20000);
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_USER, { "Foo" });

    assertStatusCodeEqual(StatusCode::OK, returnObject);
    BOOST_REQUIRE_NE("", returnObject.get<std::string>("id"));
    BOOST_REQUIRE_EQUAL("Foo", returnObject.get<std::string>("name"));
    BOOST_REQUIRE_EQUAL(20000, returnObject.get<Timestamp>("created"));
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
    auto configWithShorterName = ConfigChanger([](auto& config)
                                               {
                                                   config.user.maxNameLength = 3;
                                               });

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

    fillPropertyFromCollection(handlerToObj(handler, Forum::Commands::GET_USERS_BY_NAME).get_child("users"),
                               "id", std::back_inserter(retrievedIds), std::string());
    fillPropertyFromCollection(handlerToObj(handler, Forum::Commands::GET_USERS_BY_NAME).get_child("users"),
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

    BOOST_REQUIRE_EQUAL(3, handlerToObj(handler, Forum::Commands::COUNT_USERS).get<int>("count"));

    std::vector<std::string> retrievedNames;
    fillPropertyFromCollection(handlerToObj(handler, Forum::Commands::GET_USERS_BY_NAME).get_child("users"),
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
    fillPropertyFromCollection(handlerToObj(handler, Forum::Commands::GET_USERS_BY_NAME).get_child("users"),
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
    fillPropertyFromCollection(handlerToObj(handler, Forum::Commands::GET_USERS_BY_NAME).get_child("users"),
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
    fillPropertyFromCollection(handlerToObj(handler, Forum::Commands::GET_USERS_BY_NAME).get_child("users"),
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

BOOST_AUTO_TEST_CASE( Retrieving_users_by_name_invokes_observer )
{
    std::string nameToBeRetrieved;
    auto handler = createCommandHandler();

    DisposingDelegateObserver observer(*handler);
    observer->getUsersByNameAction = [&](auto& _, auto& name) { nameToBeRetrieved = name; };

    handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "SampleUser" });
    BOOST_REQUIRE_EQUAL("SampleUser", nameToBeRetrieved);
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

    BOOST_REQUIRE_EQUAL(1, handlerToObj(handler, Forum::Commands::COUNT_USERS).get<int>("count"));

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::CHANGE_USER_NAME, { userId, "Xyz" }));
    auto modifiedUser = handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "Xyz" });

    BOOST_REQUIRE_EQUAL(1, handlerToObj(handler, Forum::Commands::COUNT_USERS).get<int>("count"));
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
    fillPropertyFromCollection(handlerToObj(handler, Forum::Commands::GET_USERS_BY_NAME).get_child("users"),
                               "name", std::back_inserter(retrievedNames), std::string());

    BOOST_REQUIRE_EQUAL(names.size(), retrievedNames.size());
    BOOST_REQUIRE_EQUAL("Def", retrievedNames[0]);
    BOOST_REQUIRE_EQUAL("Ghi", retrievedNames[1]);
    BOOST_REQUIRE_EQUAL("Xyz", retrievedNames[2]);
}

BOOST_AUTO_TEST_CASE( Modifying_a_user_invokes_observer )
{
    std::string newName;
    auto userChange = User::ChangeType::None;
    auto handler = createCommandHandler();

    DisposingDelegateObserver observer(*handler);
    observer->changeUserAction = [&](auto& _, auto& user, auto change)
    {
        newName = user.name();
        userChange = change;
    };

    handlerToObj(handler, Forum::Commands::ADD_USER, { "Abc" });
    auto user = handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "Abc" });
    auto userId = user.get<std::string>("user.id");

    handlerToObj(handler, Forum::Commands::CHANGE_USER_NAME, { userId, "Xyz" });
    BOOST_REQUIRE_EQUAL("Xyz", newName);
    BOOST_REQUIRE_EQUAL(User::ChangeType::Name, userChange);
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

    BOOST_REQUIRE_EQUAL(3, handlerToObj(handler, Forum::Commands::COUNT_USERS).get<int>("count"));

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::DELETE_USER, { userId }));
    assertStatusCodeEqual(StatusCode::NOT_FOUND, handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "Abc" }));

    std::vector<std::string> retrievedNames;
    fillPropertyFromCollection(handlerToObj(handler, Forum::Commands::GET_USERS_BY_NAME).get_child("users"),
                               "name", std::back_inserter(retrievedNames), std::string());

    BOOST_REQUIRE_EQUAL(2, handlerToObj(handler, Forum::Commands::COUNT_USERS).get<int>("count"));

    BOOST_REQUIRE_EQUAL(names.size() - 1, retrievedNames.size());
    BOOST_REQUIRE_EQUAL("Def", retrievedNames[0]);
    BOOST_REQUIRE_EQUAL("Ghi", retrievedNames[1]);
}

BOOST_AUTO_TEST_CASE( Deleting_a_user_invokes_observer )
{
    std::string deletedUserName;
    auto handler = createCommandHandler();

    DisposingDelegateObserver observer(*handler);
    observer->deleteUserAction = [&](auto& _, auto& user) { deletedUserName = user.name(); };

    handlerToObj(handler, Forum::Commands::ADD_USER, { "Abc" });
    auto user = handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "Abc" });
    auto userId = user.get<std::string>("user.id");

    handlerToObj(handler, Forum::Commands::DELETE_USER, { userId });
    BOOST_REQUIRE_EQUAL("Abc", deletedUserName);
}

BOOST_AUTO_TEST_CASE( Users_are_retrieved_by_their_creation_date )
{
    auto handler = createCommandHandler();
    auto namesWithCreationDates =
            {
                    std::make_pair("Abc", 1000),
                    std::make_pair("Ghi", 3000),
                    std::make_pair("Def", 2000),
            };

    for (auto& pair : namesWithCreationDates)
    {
        TimestampChanger changer(pair.second);
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_USER, { pair.first }));
    }

    std::vector<std::string> retrievedNames;
    fillPropertyFromCollection(handlerToObj(handler, Forum::Commands::GET_USERS_BY_CREATED).get_child("users"),
                               "name", std::back_inserter(retrievedNames), std::string());

    BOOST_REQUIRE_EQUAL(3, retrievedNames.size());
    BOOST_REQUIRE_EQUAL("Abc", retrievedNames[0]);
    BOOST_REQUIRE_EQUAL("Def", retrievedNames[1]);
    BOOST_REQUIRE_EQUAL("Ghi", retrievedNames[2]);
}

BOOST_AUTO_TEST_CASE( Users_without_activity_have_last_seen_empty )
{
    auto handler = createCommandHandler();
    std::vector<std::string> names = { "Abc", "Ghi", "Def" };

    for (auto& name : names)
    {
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_USER, { name }));
    }

    std::vector<Timestamp> retrievedLastSeen;
    fillPropertyFromCollection(handlerToObj(handler, Forum::Commands::GET_USERS_BY_LAST_SEEN).get_child("users"),
                               "lastSeen", std::back_inserter(retrievedLastSeen), Timestamp());

    BOOST_REQUIRE_EQUAL(names.size(), retrievedLastSeen.size());
    BOOST_REQUIRE_EQUAL(0, retrievedLastSeen[0]);
    BOOST_REQUIRE_EQUAL(0, retrievedLastSeen[1]);
    BOOST_REQUIRE_EQUAL(0, retrievedLastSeen[2]);
}

BOOST_AUTO_TEST_CASE( User_last_seen_is_correctly_updated )
{
    auto handler = createCommandHandler();
    std::vector<std::string> names = { "Abc", "Ghi", "Def" };

    for (auto& name : names)
    {
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_USER, { name }));
    }

    //perform an action while "logged in" as each user
    {
        TimestampChanger changer(10000);
        auto userId = handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "Abc" }).get<std::string>("user.id");
        LoggedInUserChanger loggedInChanger(userId);
        BOOST_REQUIRE_EQUAL(3, handlerToObj(handler, Forum::Commands::COUNT_USERS).get<int>("count"));
    }
    {
        TimestampChanger changer(30000);
        auto userId = handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "Ghi" }).get<std::string>("user.id");
        LoggedInUserChanger loggedInChanger(userId);
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_USER, { "Xyz" }));
    }
    IdType userToDelete;
    {
        TimestampChanger changer(20000);
        auto userId = handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "Def" }).get<std::string>("user.id");
        LoggedInUserChanger loggedInChanger(userId);
        userToDelete = handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "Xyz" }).get<std::string>("user.id");
    }
    {
        TimestampChanger changer(20050);//lower then the minimum for updating last seen
        auto userId = handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, {"Def"}).get<std::string>("user.id");
        LoggedInUserChanger loggedInChanger(userId);
        assertStatusCodeEqual(StatusCode::OK,
                              handlerToObj(handler, Forum::Commands::DELETE_USER, { (std::string) userToDelete }));
    }

    std::vector<Timestamp> retrievedLastSeen;
    fillPropertyFromCollection(handlerToObj(handler, Forum::Commands::GET_USERS_BY_LAST_SEEN).get_child("users"),
                               "lastSeen", std::back_inserter(retrievedLastSeen), Timestamp());

    BOOST_REQUIRE_EQUAL(names.size(), retrievedLastSeen.size());
    BOOST_REQUIRE_EQUAL(30000, retrievedLastSeen[0]);
    BOOST_REQUIRE_EQUAL(20000, retrievedLastSeen[1]);
    BOOST_REQUIRE_EQUAL(10000, retrievedLastSeen[2]);

    std::vector<std::string> retrievedNames;
    fillPropertyFromCollection(handlerToObj(handler, Forum::Commands::GET_USERS_BY_LAST_SEEN).get_child("users"),
                               "name", std::back_inserter(retrievedNames), std::string());

    BOOST_REQUIRE_EQUAL(names.size(), retrievedNames.size());
    BOOST_REQUIRE_EQUAL("Ghi", retrievedNames[0]);
    BOOST_REQUIRE_EQUAL("Def", retrievedNames[1]);
    BOOST_REQUIRE_EQUAL("Abc", retrievedNames[2]);
}
