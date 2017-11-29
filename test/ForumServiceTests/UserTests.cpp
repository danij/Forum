/*
Fast Forum Backend
Copyright (C) 2016-2017 Daniel Jurcau

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "CommandsCommon.h"
#include "TestHelpers.h"

#include <vector>

using namespace Forum::Configuration;
using namespace Forum::Context;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;

/**
 * Stores only the information that is sent out about a discussion thread referenced in a user
 */
struct SerializedUserDiscussionThread
{
    std::string id;
    std::string name;
    Timestamp created = 0;
    Timestamp lastUpdated = 0;
    int visited = 0;
    int messageCount = 0;

    void populate(const boost::property_tree::ptree& tree)
    {
        id = tree.get<std::string>("id");
        name = tree.get<std::string>("name");
        created = tree.get<Timestamp>("created");
        lastUpdated = tree.get<Timestamp>("lastUpdated");
        visited = tree.get<int>("visited");
        messageCount = tree.get<int>("messageCount");
    }
};

CREATE_FUNCTION_ALIAS(deserializeUserThreads, deserializeEntities<SerializedUserDiscussionThread>)

struct SerializedDiscussionMessageParentThread
{
    std::string id;
    std::string name;
    Timestamp created = 0;
    Timestamp lastUpdated = 0;
    int visited = 0;
};

struct SerializedUserDiscussionThreadMessage
{
    std::string id;
    std::string content;
    Timestamp created = 0;
    SerializedDiscussionMessageParentThread parentThread;

    void populate(const boost::property_tree::ptree& tree)
    {
        id = tree.get<std::string>("id");
        content = tree.get<std::string>("content");
        created = tree.get<Timestamp>("created");

        parentThread.id = tree.get<std::string>("parentThread.id");
        parentThread.name = tree.get<std::string>("parentThread.name");
        parentThread.created = tree.get<Timestamp>("parentThread.created");
        parentThread.lastUpdated = tree.get<Timestamp>("parentThread.lastUpdated");
        parentThread.visited = tree.get<int>("parentThread.visited");
    }
};

CREATE_FUNCTION_ALIAS(deserializeUserThreadMessages, deserializeEntities<SerializedUserDiscussionThreadMessage>)

struct SerializedUser
{
    std::string id;
    std::string name;
    Timestamp created = 0;
    Timestamp lastSeen = 0;
    int threadCount = 0;
    int messageCount = 0;

    void populate(const boost::property_tree::ptree& tree)
    {
        id = tree.get<std::string>("id");
        name = tree.get<std::string>("name");
        created = tree.get<Timestamp>("created");
        lastSeen = tree.get<Timestamp>("lastSeen");
        threadCount = tree.get<int>("threadCount");
        messageCount = tree.get<int>("messageCount");
    }
};

CREATE_FUNCTION_ALIAS(deserializeUsers, deserializeEntities<SerializedUser>)

BOOST_AUTO_TEST_CASE( User_count_is_initially_zero )
{
    auto handler = createCommandHandler();
    auto returnObject = handlerToObj(handler, Forum::Commands::COUNT_ENTITIES);
    BOOST_REQUIRE_EQUAL(0, returnObject.get<int>("count.users"));
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_no_parameters_fails )
{
    auto handler = createCommandHandler();
    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_USER);
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_empty_name_fails )
{
    auto handler = createCommandHandler();
    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_USER, { "" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_returns_the_id_name_and_created )
{
    TimestampChanger changer(20000);
    auto handler = createCommandHandler();
    auto returnObject = createUser(handler, "Foo");

    assertStatusCodeEqual(StatusCode::OK, returnObject);
    BOOST_REQUIRE( ! isIdEmpty(returnObject.get<std::string>("id")));
    BOOST_REQUIRE_EQUAL("Foo", returnObject.get<std::string>("name"));
    BOOST_REQUIRE_EQUAL(20000, returnObject.get<Timestamp>("created"));
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_only_whitespace_in_the_name_fails )
{
    auto handler = createCommandHandler();
    auto returnObject = createUser(handler, " \t\r\n");
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_leading_whitespace_in_the_name_fails )
{
    auto handler = createCommandHandler();
    auto returnObject = createUser(handler, " Foo");
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_trailing_whitespace_in_the_name_fails )
{
    auto handler = createCommandHandler();
    auto returnObject = createUser(handler, "Foo\t");
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_leading_nonletter_nonnumber_in_the_name_fails )
{
    auto handler = createCommandHandler();
    auto returnObject = createUser(handler, ":Foo");
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_trailing_nonletter_nonnumber_in_the_name_fails )
{
    auto handler = createCommandHandler();
    auto returnObject = createUser(handler, "Foo?");
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_whitespace_in_the_middle_of_the_name_succeeds )
{
    auto handler = createCommandHandler();
    auto returnObject = createUser(handler, "Foo Bar");
    assertStatusCodeEqual(StatusCode::OK, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_dash_in_the_middle_of_the_name_succeeds )
{
    auto handler = createCommandHandler();
    auto returnObject = createUser(handler, "Foo-Bar");
    assertStatusCodeEqual(StatusCode::OK, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_underscore_in_the_middle_of_the_name_succeeds )
{
    auto handler = createCommandHandler();
    auto returnObject = createUser(handler, "Foo_Bar");
    assertStatusCodeEqual(StatusCode::OK, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_newline_in_the_middle_of_the_name_fails )
{
    auto handler = createCommandHandler();
    auto returnObject = createUser(handler, "Foo\nBar");
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_strange_character_in_the_middle_of_the_name_fails )
{
    auto handler = createCommandHandler();
    auto returnObject = createUser(handler, "Foo☂Bar");
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_only_numbers_in_the_name_succeeds )
{
    auto handler = createCommandHandler();
    auto returnObject = createUser(handler, "0123456789");
    assertStatusCodeEqual(StatusCode::OK, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_accented_letters_in_the_name_succeeds )
{
    auto handler = createCommandHandler();
    auto returnObject = createUser(handler, "FȭǬ");
    assertStatusCodeEqual(StatusCode::OK, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_a_too_short_name_fails )
{
    auto config = getGlobalConfig();
    std::string username(config->user.minNameLength - 1, 'a');
    auto handler = createCommandHandler();
    auto returnObject = createUser(handler, username);
    assertStatusCodeEqual(StatusCode::VALUE_TOO_SHORT, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_a_longer_name_fails )
{
    auto config = getGlobalConfig();
    std::string username(config->user.maxNameLength + 1, 'a');
    auto handler = createCommandHandler();
    auto returnObject = createUser(handler, username);
    assertStatusCodeEqual(StatusCode::VALUE_TOO_LONG, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_unicode_name_of_valid_length_succeeds )
{
    ConfigChanger _([](auto& config)
                    {
                        config.user.maxNameLength = 3;
                    });

    //test a simple text that can also be represented as ASCII
    auto handler = createCommandHandler();
    auto returnObject = createUser(handler, "AAA");
    assertStatusCodeEqual(StatusCode::OK, returnObject);

    //test a 3 characters text that requires multiple bytes for representation using UTF-8
    returnObject = createUser(handler, "早上好");
    assertStatusCodeEqual(StatusCode::OK, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_with_a_name_that_contains_invalid_characters_fails_with_appropriate_message)
{
    auto handler = createCommandHandler();
    auto returnObject = createUser(handler, "\xFF\xFF\xFF\xFF");
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( A_user_that_was_created_can_be_retrieved_and_has_a_distinct_id )
{
    auto handler = createCommandHandler();
    assertStatusCodeEqual(StatusCode::OK, createUser(handler, "User1"));
    assertStatusCodeEqual(StatusCode::OK, createUser(handler, "User2"));

    std::vector<std::string> retrievedIds;
    std::vector<std::string> retrievedNames;

    fillPropertyFromCollection(handlerToObj(handler, Forum::Commands::GET_USERS_BY_NAME).get_child("users"),
                               "id", std::back_inserter(retrievedIds), std::string());
    fillPropertyFromCollection(handlerToObj(handler, Forum::Commands::GET_USERS_BY_NAME).get_child("users"),
                               "name", std::back_inserter(retrievedNames), std::string());

    BOOST_REQUIRE_EQUAL(2u, retrievedNames.size());
    BOOST_REQUIRE( ! isIdEmpty(retrievedIds[0]));
    BOOST_REQUIRE( ! isIdEmpty(retrievedIds[1]));
    BOOST_REQUIRE_NE(retrievedIds[0], retrievedIds[1]);

    BOOST_REQUIRE_EQUAL(2u, retrievedNames.size());
    BOOST_REQUIRE_EQUAL("User1", retrievedNames[0]);
    BOOST_REQUIRE_EQUAL("User2", retrievedNames[1]);
}

BOOST_AUTO_TEST_CASE( Users_are_retrieved_by_name )
{
    auto handler = createCommandHandler();
    std::vector<std::string> names = { "Abc", "Ghi", "Def" };

    for (auto& name : names)
    {
        assertStatusCodeEqual(StatusCode::OK, createUser(handler, name));
    }

    BOOST_REQUIRE_EQUAL(3, handlerToObj(handler, Forum::Commands::COUNT_ENTITIES).get<int>("count.users"));

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

    assertStatusCodeEqual(StatusCode::OK, createUser(handler, "Abc"));
    assertStatusCodeEqual(StatusCode::ALREADY_EXISTS, createUser(handler, "Abc"));

    std::vector<std::string> retrievedNames;
    fillPropertyFromCollection(handlerToObj(handler, Forum::Commands::GET_USERS_BY_NAME).get_child("users"),
                               "name", std::back_inserter(retrievedNames), std::string());

    BOOST_REQUIRE_EQUAL(1u, retrievedNames.size());
    BOOST_REQUIRE_EQUAL("Abc", retrievedNames[0]);
}

BOOST_AUTO_TEST_CASE( Adding_multiple_users_with_same_name_but_different_case_fails )
{
    auto handler = createCommandHandler();

    assertStatusCodeEqual(StatusCode::OK, createUser(handler, "Abc"));
    assertStatusCodeEqual(StatusCode::ALREADY_EXISTS, createUser(handler, "ABC"));

    std::vector<std::string> retrievedNames;
    fillPropertyFromCollection(handlerToObj(handler, Forum::Commands::GET_USERS_BY_NAME).get_child("users"),
                               "name", std::back_inserter(retrievedNames), std::string());

    BOOST_REQUIRE_EQUAL(1u, retrievedNames.size());
    BOOST_REQUIRE_EQUAL("Abc", retrievedNames[0]);
}

BOOST_AUTO_TEST_CASE( Adding_multiple_users_with_same_name_but_different_accents_fails )
{
    auto handler = createCommandHandler();

    assertStatusCodeEqual(StatusCode::OK, createUser(handler, "HélĹǬ"));
    assertStatusCodeEqual(StatusCode::ALREADY_EXISTS, createUser(handler, "Hello"));

    std::vector<std::string> retrievedNames;
    fillPropertyFromCollection(handlerToObj(handler, Forum::Commands::GET_USERS_BY_NAME).get_child("users"),
                               "name", std::back_inserter(retrievedNames), std::string());

    BOOST_REQUIRE_EQUAL(1u, retrievedNames.size());
    BOOST_REQUIRE_EQUAL("HélĹǬ", retrievedNames[0]);
}

BOOST_AUTO_TEST_CASE( Missing_users_retrieved_by_name_returns_not_found )
{
    auto handler = createCommandHandler();
    assertStatusCodeEqual(StatusCode::OK, createUser(handler, "Abc"));
    assertStatusCodeEqual(StatusCode::NOT_FOUND, handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "Ghi" }));
}

BOOST_AUTO_TEST_CASE( Users_can_be_retrieved_by_id )
{
    auto handler = createCommandHandler();
    auto userId = createUserAndGetId(handler, "Abc");

    auto user = handlerToObj(handler, Forum::Commands::GET_USER_BY_ID, { userId });

    BOOST_REQUIRE( ! isIdEmpty(user.get<std::string>("user.id")));
    BOOST_REQUIRE_EQUAL("Abc", user.get<std::string>("user.name"));
}

BOOST_AUTO_TEST_CASE( Users_can_be_retrieved_by_name )
{
    auto handler = createCommandHandler();
    assertStatusCodeEqual(StatusCode::OK, createUser(handler, "Abc"));

    auto user = handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "Abc" });

    BOOST_REQUIRE( ! isIdEmpty(user.get<std::string>("user.id")));
    BOOST_REQUIRE_EQUAL("Abc", user.get<std::string>("user.name"));
}

BOOST_AUTO_TEST_CASE( Users_can_be_retrieved_by_name_case_and_accent_insensitive )
{
    auto handler = createCommandHandler();
    assertStatusCodeEqual(StatusCode::OK, createUser(handler, "HélĹǬ"));

    auto user = handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "Hello" });

    BOOST_REQUIRE( ! isIdEmpty(user.get<std::string>("user.id")));
    BOOST_REQUIRE_EQUAL("HélĹǬ", user.get<std::string>("user.name"));
}

BOOST_AUTO_TEST_CASE( Users_can_be_retrieved_by_name_even_if_using_a_different_normalization_form )
{
    //"HélĹǬ" as UTF-8
    const char nameFormC[] = { 72, -61, -87, 108, -60, -71, -57, -84, 0 };
    const char nameFormD[] = { 72, 101, -52, -127, 108, 76, -52, -127, 79, -52, -88, -52, -124, 0 };

    auto handler = createCommandHandler();
    assertStatusCodeEqual(StatusCode::OK, createUser(handler, nameFormC));

    auto user = handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { nameFormD });

    BOOST_REQUIRE( ! isIdEmpty(user.get<std::string>("user.id")));
    BOOST_REQUIRE_EQUAL(nameFormC, user.get<std::string>("user.name"));
}

BOOST_AUTO_TEST_CASE( Modifying_a_user_name_succeds )
{
    auto handler = createCommandHandler();
    assertStatusCodeEqual(StatusCode::OK, createUser(handler, "Abc"));

    auto user = handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "Abc" });
    BOOST_REQUIRE_EQUAL("Abc", user.get<std::string>("user.name"));
    auto userId = user.get<std::string>("user.id");

    BOOST_REQUIRE_EQUAL(1, handlerToObj(handler, Forum::Commands::COUNT_ENTITIES).get<int>("count.users"));

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::CHANGE_USER_NAME, { userId, "Xyz" }));
    auto modifiedUser = handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "Xyz" });

    BOOST_REQUIRE_EQUAL(1, handlerToObj(handler, Forum::Commands::COUNT_ENTITIES).get<int>("count.users"));
    BOOST_REQUIRE_EQUAL("Xyz", modifiedUser.get<std::string>("user.name"));
    BOOST_REQUIRE_EQUAL(userId, modifiedUser.get<std::string>("user.id"));
}

BOOST_AUTO_TEST_CASE( Modifying_a_user_name_with_an_already_existent_value_fails )
{
    auto handler = createCommandHandler();
    assertStatusCodeEqual(StatusCode::OK, createUser(handler, "Abc"));
    assertStatusCodeEqual(StatusCode::OK, createUser(handler, "Def"));

    auto user = handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "Abc" });
    auto userId = user.get<std::string>("user.id");
    assertStatusCodeEqual(StatusCode::ALREADY_EXISTS,
                          handlerToObj(handler, Forum::Commands::CHANGE_USER_NAME, { userId, "Def" }));
}

BOOST_AUTO_TEST_CASE( Modifying_an_inexistent_user_name_returns_not_found )
{
    auto handler = createCommandHandler();
    assertStatusCodeEqual(StatusCode::OK, createUser(handler, "Abc"));
    assertStatusCodeEqual(StatusCode::NOT_FOUND,
                          handlerToObj(handler, Forum::Commands::CHANGE_USER_NAME, { "bogus id", "Xyz" }));
}

BOOST_AUTO_TEST_CASE( Modifying_a_user_name_reorders_users )
{
    auto handler = createCommandHandler();
    std::vector<std::string> names = { "Abc", "Ghi", "Def" };

    for (auto& name : names)
    {
        assertStatusCodeEqual(StatusCode::OK, createUser(handler, name));
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

BOOST_AUTO_TEST_CASE( Deleting_a_user_name_with_an_invalid_id_returns_invalid_parameters )
{
    auto handler = createCommandHandler();
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS,
                          handlerToObj(handler, Forum::Commands::DELETE_USER, { "bogus id" }));
}

BOOST_AUTO_TEST_CASE( Deleting_an_inexistent_user_name_returns_not_found )
{
    auto handler = createCommandHandler();
    assertStatusCodeEqual(StatusCode::NOT_FOUND,
                          handlerToObj(handler, Forum::Commands::DELETE_USER, { sampleValidIdString }));
}

BOOST_AUTO_TEST_CASE( Deleted_users_can_no_longer_be_retrieved )
{
    auto handler = createCommandHandler();
    std::vector<std::string> names = { "Abc", "Ghi", "Def" };

    for (auto& name : names)
    {
        assertStatusCodeEqual(StatusCode::OK, createUser(handler, name));
    }

    auto user = handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "Abc" });
    BOOST_REQUIRE_EQUAL("Abc", user.get<std::string>("user.name"));
    auto userId = user.get<std::string>("user.id");

    BOOST_REQUIRE_EQUAL(3, handlerToObj(handler, Forum::Commands::COUNT_ENTITIES).get<int>("count.users"));

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::DELETE_USER, { userId }));
    assertStatusCodeEqual(StatusCode::NOT_FOUND, handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "Abc" }));

    std::vector<std::string> retrievedNames;
    fillPropertyFromCollection(handlerToObj(handler, Forum::Commands::GET_USERS_BY_NAME).get_child("users"),
                               "name", std::back_inserter(retrievedNames), std::string());

    BOOST_REQUIRE_EQUAL(2, handlerToObj(handler, Forum::Commands::COUNT_ENTITIES).get<int>("count.users"));

    BOOST_REQUIRE_EQUAL(names.size() - 1, retrievedNames.size());
    BOOST_REQUIRE_EQUAL("Def", retrievedNames[0]);
    BOOST_REQUIRE_EQUAL("Ghi", retrievedNames[1]);
}

BOOST_AUTO_TEST_CASE( Users_are_retrieved_by_their_creation_date_in_ascending_and_descending_order )
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
        assertStatusCodeEqual(StatusCode::OK, createUser(handler, pair.first));
    }

    std::vector<std::string> retrievedNames;
    fillPropertyFromCollection(handlerToObj(handler, Forum::Commands::GET_USERS_BY_CREATED, SortOrder::Ascending)
                               .get_child("users"), "name", std::back_inserter(retrievedNames), std::string());

    BOOST_REQUIRE_EQUAL(3u, retrievedNames.size());
    BOOST_REQUIRE_EQUAL("Abc", retrievedNames[0]);
    BOOST_REQUIRE_EQUAL("Def", retrievedNames[1]);
    BOOST_REQUIRE_EQUAL("Ghi", retrievedNames[2]);

    retrievedNames.clear();
    fillPropertyFromCollection(handlerToObj(handler, Forum::Commands::GET_USERS_BY_CREATED, SortOrder::Descending)
                               .get_child("users"), "name", std::back_inserter(retrievedNames), std::string());

    BOOST_REQUIRE_EQUAL(3u, retrievedNames.size());
    BOOST_REQUIRE_EQUAL("Ghi", retrievedNames[0]);
    BOOST_REQUIRE_EQUAL("Def", retrievedNames[1]);
    BOOST_REQUIRE_EQUAL("Abc", retrievedNames[2]);
}

BOOST_AUTO_TEST_CASE( Users_without_activity_have_last_seen_empty )
{
    auto handler = createCommandHandler();
    std::vector<std::string> names = { "Abc", "Ghi", "Def" };

    for (auto& name : names)
    {
        assertStatusCodeEqual(StatusCode::OK, createUser(handler, name));
    }

    std::vector<Timestamp> retrievedLastSeen;
    fillPropertyFromCollection(handlerToObj(handler, Forum::Commands::GET_USERS_BY_LAST_SEEN, SortOrder::Ascending)
                               .get_child("users"), "lastSeen", std::back_inserter(retrievedLastSeen), Timestamp());

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
        assertStatusCodeEqual(StatusCode::OK, createUser(handler, name));
    }

    //perform an action while "logged in" as each user
    {
        TimestampChanger changer(10000);
        auto userId = handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "Abc" }).get<std::string>("user.id");
        LoggedInUserChanger loggedInChanger(userId);
        BOOST_REQUIRE_EQUAL(3, handlerToObj(handler, Forum::Commands::COUNT_ENTITIES).get<int>("count.users"));
    }
    {
        TimestampChanger changer(30000);
        auto userId = handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "Ghi" }).get<std::string>("user.id");
        LoggedInUserChanger loggedInChanger(userId);
        assertStatusCodeEqual(StatusCode::OK, createUser(handler, "Xyz"));
    }
    IdType userToDelete;
    {
        TimestampChanger changer(20000);
        auto userId = handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "Def" }).get<std::string>("user.id");
        LoggedInUserChanger loggedInChanger(userId);
        userToDelete = handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "Xyz" }).get<std::string>("user.id");
    }
    {
        TimestampChanger changer(20050);//difference to previous action is lower than the minimum for updating last seen
        auto userId = handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "Def" }).get<std::string>("user.id");
        LoggedInUserChanger loggedInChanger(userId);
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler,
                                                           Forum::Commands::DELETE_USER, { static_cast<std::string>(userToDelete) }));
    }

    std::vector<Timestamp> retrievedLastSeen;
    fillPropertyFromCollection(handlerToObj(handler, Forum::Commands::GET_USERS_BY_LAST_SEEN, SortOrder::Ascending)
                               .get_child("users"), "lastSeen", std::back_inserter(retrievedLastSeen), Timestamp());

    BOOST_REQUIRE_EQUAL(names.size(), retrievedLastSeen.size());
    BOOST_REQUIRE_EQUAL(10000, retrievedLastSeen[0]);
    BOOST_REQUIRE_EQUAL(20000, retrievedLastSeen[1]);
    BOOST_REQUIRE_EQUAL(30000, retrievedLastSeen[2]);

    retrievedLastSeen.clear();
    fillPropertyFromCollection(handlerToObj(handler, Forum::Commands::GET_USERS_BY_LAST_SEEN, SortOrder::Descending)
                               .get_child("users"), "lastSeen", std::back_inserter(retrievedLastSeen), Timestamp());

    BOOST_REQUIRE_EQUAL(names.size(), retrievedLastSeen.size());
    BOOST_REQUIRE_EQUAL(30000, retrievedLastSeen[0]);
    BOOST_REQUIRE_EQUAL(20000, retrievedLastSeen[1]);
    BOOST_REQUIRE_EQUAL(10000, retrievedLastSeen[2]);

    std::vector<std::string> retrievedNames;
    fillPropertyFromCollection(handlerToObj(handler, Forum::Commands::GET_USERS_BY_LAST_SEEN, SortOrder::Descending)
                               .get_child("users"), "name", std::back_inserter(retrievedNames), std::string());

    BOOST_REQUIRE_EQUAL(names.size(), retrievedNames.size());
    BOOST_REQUIRE_EQUAL("Ghi", retrievedNames[0]);
    BOOST_REQUIRE_EQUAL("Def", retrievedNames[1]);
    BOOST_REQUIRE_EQUAL("Abc", retrievedNames[2]);
}

BOOST_AUTO_TEST_CASE( Retrieving_discussion_threads_of_invalid_user_returns_invalid_parameters )
{
    auto handler = createCommandHandler();

    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS,
                          handlerToObj(handler,
                                       Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_NAME,
                                       SortOrder::Ascending, { "bogusId" }));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS,
                          handlerToObj(handler,
                                       Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_NAME,
                                       SortOrder::Descending, { "bogusId" }));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS,
                          handlerToObj(handler,
                                       Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_CREATED,
                                       SortOrder::Ascending, { "bogusId" }));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS,
                          handlerToObj(handler,
                                       Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_CREATED,
                                       SortOrder::Descending, { "bogusId" }));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS,
                          handlerToObj(handler,
                                       Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_LAST_UPDATED,
                                       SortOrder::Ascending, { "bogusId" }));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS,
                          handlerToObj(handler,
                                       Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_LAST_UPDATED,
                                       SortOrder::Descending, { "bogusId" }));
}

BOOST_AUTO_TEST_CASE( Discussion_threads_created_user_can_be_retrieved_sorted_by_various_criteria )
{
    auto handler = createCommandHandler();

    auto user1 = createUserAndGetId(handler, "User1");
    {
        LoggedInUserChanger changer(user1);
        {
            TimestampChanger timestampChanger(1000);
            createDiscussionThreadAndGetId(handler, "Def-User1");
        }
        {
            TimestampChanger timestampChanger(2000);
            createDiscussionThreadAndGetId(handler, "Abc-User1");
        }
        {
            TimestampChanger timestampChanger(3000);
            createDiscussionThreadAndGetId(handler, "Ghi-User1");
        }
    }
    auto user2 = createUserAndGetId(handler, "User2");
    {
        LoggedInUserChanger changer(user2);
        std::string user2Thread1;
        {
            TimestampChanger timestampChanger(1000);
            user2Thread1 = createDiscussionThreadAndGetId(handler, "Def-User2");
            //increase visited of user2Thread1
            handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { user2Thread1 });
        }
        {
            TimestampChanger timestampChanger(2000);
            createDiscussionThreadAndGetId(handler, "Abc-User2");
        }
        {
            TimestampChanger timestampChanger(3000);
            assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler,
                                                               Forum::Commands::CHANGE_DISCUSSION_THREAD_NAME,
                                                               { user2Thread1, "AaDef-User2" }));
        }
    }

    auto user1ThreadsByName = deserializeUserThreads(handlerToObj(handler,
                                                                  Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_NAME,
                                                                  SortOrder::Ascending, { user1 }).get_child("threads"));

    BOOST_REQUIRE_EQUAL(3u, user1ThreadsByName.size());
    BOOST_REQUIRE( ! isIdEmpty(user1ThreadsByName[0].id));
    BOOST_REQUIRE_EQUAL("Abc-User1", user1ThreadsByName[0].name);
    BOOST_REQUIRE_EQUAL(2000, user1ThreadsByName[0].created);
    BOOST_REQUIRE_EQUAL(2000, user1ThreadsByName[0].lastUpdated);
    BOOST_REQUIRE_EQUAL(0, user1ThreadsByName[0].visited);
    BOOST_REQUIRE( ! isIdEmpty(user1ThreadsByName[1].id));
    BOOST_REQUIRE_EQUAL("Def-User1", user1ThreadsByName[1].name);
    BOOST_REQUIRE_EQUAL(1000, user1ThreadsByName[1].created);
    BOOST_REQUIRE_EQUAL(1000, user1ThreadsByName[1].lastUpdated);
    BOOST_REQUIRE_EQUAL(0, user1ThreadsByName[1].visited);
    BOOST_REQUIRE( ! isIdEmpty(user1ThreadsByName[2].id));
    BOOST_REQUIRE_EQUAL("Ghi-User1", user1ThreadsByName[2].name);
    BOOST_REQUIRE_EQUAL(3000, user1ThreadsByName[2].created);
    BOOST_REQUIRE_EQUAL(3000, user1ThreadsByName[2].lastUpdated);
    BOOST_REQUIRE_EQUAL(0, user1ThreadsByName[2].visited);

    user1ThreadsByName = deserializeUserThreads(handlerToObj(handler,
                                                             Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_NAME,
                                                             SortOrder::Descending, { user1 }).get_child("threads"));

    BOOST_REQUIRE_EQUAL(3u, user1ThreadsByName.size());
    BOOST_REQUIRE( ! isIdEmpty(user1ThreadsByName[0].id));
    BOOST_REQUIRE_EQUAL("Ghi-User1", user1ThreadsByName[0].name);
    BOOST_REQUIRE_EQUAL(3000, user1ThreadsByName[0].created);
    BOOST_REQUIRE_EQUAL(3000, user1ThreadsByName[0].lastUpdated);
    BOOST_REQUIRE_EQUAL(0, user1ThreadsByName[0].visited);
    BOOST_REQUIRE( ! isIdEmpty(user1ThreadsByName[1].id));
    BOOST_REQUIRE_EQUAL("Def-User1", user1ThreadsByName[1].name);
    BOOST_REQUIRE_EQUAL(1000, user1ThreadsByName[1].created);
    BOOST_REQUIRE_EQUAL(1000, user1ThreadsByName[1].lastUpdated);
    BOOST_REQUIRE_EQUAL(0, user1ThreadsByName[1].visited);
    BOOST_REQUIRE( ! isIdEmpty(user1ThreadsByName[2].id));
    BOOST_REQUIRE_EQUAL("Abc-User1", user1ThreadsByName[2].name);
    BOOST_REQUIRE_EQUAL(2000, user1ThreadsByName[2].created);
    BOOST_REQUIRE_EQUAL(2000, user1ThreadsByName[2].lastUpdated);
    BOOST_REQUIRE_EQUAL(0, user1ThreadsByName[2].visited);

    auto user1ThreadsByCreated = deserializeUserThreads(
        handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_CREATED, SortOrder::Ascending, { user1 })
            .get_child("threads"));

    BOOST_REQUIRE_EQUAL(3u, user1ThreadsByCreated.size());
    BOOST_REQUIRE( ! isIdEmpty(user1ThreadsByCreated[0].id));
    BOOST_REQUIRE_EQUAL("Def-User1", user1ThreadsByCreated[0].name);
    BOOST_REQUIRE_EQUAL(1000, user1ThreadsByCreated[0].created);
    BOOST_REQUIRE_EQUAL(1000, user1ThreadsByCreated[0].lastUpdated);
    BOOST_REQUIRE_EQUAL(0, user1ThreadsByCreated[0].visited);
    BOOST_REQUIRE( ! isIdEmpty(user1ThreadsByCreated[1].id));
    BOOST_REQUIRE_EQUAL("Abc-User1", user1ThreadsByCreated[1].name);
    BOOST_REQUIRE_EQUAL(2000, user1ThreadsByCreated[1].created);
    BOOST_REQUIRE_EQUAL(2000, user1ThreadsByCreated[1].lastUpdated);
    BOOST_REQUIRE_EQUAL(0, user1ThreadsByCreated[1].visited);
    BOOST_REQUIRE( ! isIdEmpty(user1ThreadsByCreated[2].id));
    BOOST_REQUIRE_EQUAL("Ghi-User1", user1ThreadsByCreated[2].name);
    BOOST_REQUIRE_EQUAL(3000, user1ThreadsByCreated[2].created);
    BOOST_REQUIRE_EQUAL(3000, user1ThreadsByCreated[2].lastUpdated);
    BOOST_REQUIRE_EQUAL(0, user1ThreadsByCreated[2].visited);

    user1ThreadsByCreated = deserializeUserThreads(
        handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_CREATED, SortOrder::Descending, { user1 })
            .get_child("threads"));

    BOOST_REQUIRE_EQUAL(3u, user1ThreadsByCreated.size());
    BOOST_REQUIRE( ! isIdEmpty(user1ThreadsByCreated[0].id));
    BOOST_REQUIRE_EQUAL("Ghi-User1", user1ThreadsByCreated[0].name);
    BOOST_REQUIRE_EQUAL(3000, user1ThreadsByCreated[0].created);
    BOOST_REQUIRE_EQUAL(3000, user1ThreadsByCreated[0].lastUpdated);
    BOOST_REQUIRE_EQUAL(0, user1ThreadsByCreated[0].visited);
    BOOST_REQUIRE( ! isIdEmpty(user1ThreadsByCreated[1].id));
    BOOST_REQUIRE_EQUAL("Abc-User1", user1ThreadsByCreated[1].name);
    BOOST_REQUIRE_EQUAL(2000, user1ThreadsByCreated[1].created);
    BOOST_REQUIRE_EQUAL(2000, user1ThreadsByCreated[1].lastUpdated);
    BOOST_REQUIRE_EQUAL(0, user1ThreadsByCreated[1].visited);
    BOOST_REQUIRE( ! isIdEmpty(user1ThreadsByCreated[2].id));
    BOOST_REQUIRE_EQUAL("Def-User1", user1ThreadsByCreated[2].name);
    BOOST_REQUIRE_EQUAL(1000, user1ThreadsByCreated[2].created);
    BOOST_REQUIRE_EQUAL(1000, user1ThreadsByCreated[2].lastUpdated);
    BOOST_REQUIRE_EQUAL(0, user1ThreadsByCreated[2].visited);

    auto user1ThreadsByLastUpdated = deserializeUserThreads(
            handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_LAST_UPDATED, SortOrder::Ascending, { user1 })
                .get_child("threads"));

    BOOST_REQUIRE_EQUAL(3u, user1ThreadsByLastUpdated.size());
    BOOST_REQUIRE( ! isIdEmpty(user1ThreadsByLastUpdated[0].id));
    BOOST_REQUIRE_EQUAL("Def-User1", user1ThreadsByLastUpdated[0].name);
    BOOST_REQUIRE_EQUAL(1000, user1ThreadsByLastUpdated[0].created);
    BOOST_REQUIRE_EQUAL(1000, user1ThreadsByLastUpdated[0].lastUpdated);
    BOOST_REQUIRE_EQUAL(0, user1ThreadsByLastUpdated[0].visited);
    BOOST_REQUIRE( ! isIdEmpty(user1ThreadsByLastUpdated[1].id));
    BOOST_REQUIRE_EQUAL("Abc-User1", user1ThreadsByLastUpdated[1].name);
    BOOST_REQUIRE_EQUAL(2000, user1ThreadsByLastUpdated[1].created);
    BOOST_REQUIRE_EQUAL(2000, user1ThreadsByLastUpdated[1].lastUpdated);
    BOOST_REQUIRE_EQUAL(0, user1ThreadsByLastUpdated[1].visited);
    BOOST_REQUIRE( ! isIdEmpty(user1ThreadsByLastUpdated[2].id));
    BOOST_REQUIRE_EQUAL("Ghi-User1", user1ThreadsByLastUpdated[2].name);
    BOOST_REQUIRE_EQUAL(3000, user1ThreadsByLastUpdated[2].created);
    BOOST_REQUIRE_EQUAL(3000, user1ThreadsByLastUpdated[2].lastUpdated);
    BOOST_REQUIRE_EQUAL(0, user1ThreadsByLastUpdated[2].visited);

    user1ThreadsByLastUpdated = deserializeUserThreads(
        handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_LAST_UPDATED, SortOrder::Descending, { user1 })
            .get_child("threads"));

    BOOST_REQUIRE_EQUAL(3u, user1ThreadsByLastUpdated.size());
    BOOST_REQUIRE( ! isIdEmpty(user1ThreadsByLastUpdated[0].id));
    BOOST_REQUIRE_EQUAL("Ghi-User1", user1ThreadsByLastUpdated[0].name);
    BOOST_REQUIRE_EQUAL(3000, user1ThreadsByLastUpdated[0].created);
    BOOST_REQUIRE_EQUAL(3000, user1ThreadsByLastUpdated[0].lastUpdated);
    BOOST_REQUIRE_EQUAL(0, user1ThreadsByLastUpdated[0].visited);
    BOOST_REQUIRE( ! isIdEmpty(user1ThreadsByLastUpdated[1].id));
    BOOST_REQUIRE_EQUAL("Abc-User1", user1ThreadsByLastUpdated[1].name);
    BOOST_REQUIRE_EQUAL(2000, user1ThreadsByLastUpdated[1].created);
    BOOST_REQUIRE_EQUAL(2000, user1ThreadsByLastUpdated[1].lastUpdated);
    BOOST_REQUIRE_EQUAL(0, user1ThreadsByLastUpdated[1].visited);
    BOOST_REQUIRE( ! isIdEmpty(user1ThreadsByLastUpdated[2].id));
    BOOST_REQUIRE_EQUAL("Def-User1", user1ThreadsByLastUpdated[2].name);
    BOOST_REQUIRE_EQUAL(1000, user1ThreadsByLastUpdated[2].created);
    BOOST_REQUIRE_EQUAL(1000, user1ThreadsByLastUpdated[2].lastUpdated);
    BOOST_REQUIRE_EQUAL(0, user1ThreadsByLastUpdated[2].visited);

    auto user2ThreadsByName = deserializeUserThreads(handlerToObj(handler,
                                                                  Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_NAME,
                                                                  SortOrder::Ascending,
                                                                  { user2 }).get_child("threads"));

    BOOST_REQUIRE_EQUAL(2u, user2ThreadsByName.size());
    BOOST_REQUIRE( ! isIdEmpty(user2ThreadsByName[0].id));
    BOOST_REQUIRE_EQUAL("AaDef-User2", user2ThreadsByName[0].name);
    BOOST_REQUIRE_EQUAL(1000, user2ThreadsByName[0].created);
    BOOST_REQUIRE_EQUAL(3000, user2ThreadsByName[0].lastUpdated);
    BOOST_REQUIRE_EQUAL(1, user2ThreadsByName[0].visited);
    BOOST_REQUIRE( ! isIdEmpty(user2ThreadsByName[1].id));
    BOOST_REQUIRE_EQUAL("Abc-User2", user2ThreadsByName[1].name);
    BOOST_REQUIRE_EQUAL(2000, user2ThreadsByName[1].created);
    BOOST_REQUIRE_EQUAL(2000, user2ThreadsByName[1].lastUpdated);
    BOOST_REQUIRE_EQUAL(0, user2ThreadsByName[1].visited);

    user2ThreadsByName = deserializeUserThreads(handlerToObj(handler,
                                                             Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_NAME,
                                                             SortOrder::Descending,
                                                             { user2 }).get_child("threads"));

    BOOST_REQUIRE_EQUAL(2u, user2ThreadsByName.size());
    BOOST_REQUIRE( ! isIdEmpty(user2ThreadsByName[0].id));
    BOOST_REQUIRE_EQUAL("Abc-User2", user2ThreadsByName[0].name);
    BOOST_REQUIRE_EQUAL(2000, user2ThreadsByName[0].created);
    BOOST_REQUIRE_EQUAL(2000, user2ThreadsByName[0].lastUpdated);
    BOOST_REQUIRE_EQUAL(0, user2ThreadsByName[0].visited);
    BOOST_REQUIRE( ! isIdEmpty(user2ThreadsByName[1].id));
    BOOST_REQUIRE_EQUAL("AaDef-User2", user2ThreadsByName[1].name);
    BOOST_REQUIRE_EQUAL(1000, user2ThreadsByName[1].created);
    BOOST_REQUIRE_EQUAL(3000, user2ThreadsByName[1].lastUpdated);
    BOOST_REQUIRE_EQUAL(1, user2ThreadsByName[1].visited);

    auto user2ThreadsByCreated = deserializeUserThreads(
            handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_CREATED, SortOrder::Ascending, { user2 })
                .get_child("threads"));

    BOOST_REQUIRE_EQUAL(2u, user2ThreadsByCreated.size());
    BOOST_REQUIRE( ! isIdEmpty(user2ThreadsByCreated[0].id));
    BOOST_REQUIRE_EQUAL("AaDef-User2", user2ThreadsByCreated[0].name);
    BOOST_REQUIRE_EQUAL(1000, user2ThreadsByCreated[0].created);
    BOOST_REQUIRE_EQUAL(3000, user2ThreadsByCreated[0].lastUpdated);
    BOOST_REQUIRE_EQUAL(1, user2ThreadsByCreated[0].visited);
    BOOST_REQUIRE( ! isIdEmpty(user2ThreadsByCreated[1].id));
    BOOST_REQUIRE_EQUAL("Abc-User2", user2ThreadsByCreated[1].name);
    BOOST_REQUIRE_EQUAL(2000, user2ThreadsByCreated[1].created);
    BOOST_REQUIRE_EQUAL(2000, user2ThreadsByCreated[1].lastUpdated);
    BOOST_REQUIRE_EQUAL(0, user2ThreadsByCreated[1].visited);

    user2ThreadsByCreated = deserializeUserThreads(
        handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_CREATED, SortOrder::Descending, { user2 })
            .get_child("threads"));

    BOOST_REQUIRE_EQUAL(2u, user2ThreadsByCreated.size());
    BOOST_REQUIRE( ! isIdEmpty(user2ThreadsByCreated[0].id));
    BOOST_REQUIRE_EQUAL("Abc-User2", user2ThreadsByCreated[0].name);
    BOOST_REQUIRE_EQUAL(2000, user2ThreadsByCreated[0].created);
    BOOST_REQUIRE_EQUAL(2000, user2ThreadsByCreated[0].lastUpdated);
    BOOST_REQUIRE_EQUAL(0, user2ThreadsByCreated[0].visited);
    BOOST_REQUIRE( ! isIdEmpty(user2ThreadsByCreated[1].id));
    BOOST_REQUIRE_EQUAL("AaDef-User2", user2ThreadsByCreated[1].name);
    BOOST_REQUIRE_EQUAL(1000, user2ThreadsByCreated[1].created);
    BOOST_REQUIRE_EQUAL(3000, user2ThreadsByCreated[1].lastUpdated);
    BOOST_REQUIRE_EQUAL(1, user2ThreadsByCreated[1].visited);

    auto user2ThreadsByLastUpdated = deserializeUserThreads(
            handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_LAST_UPDATED, SortOrder::Ascending, { user2 })
                .get_child("threads"));

    BOOST_REQUIRE_EQUAL(2u, user2ThreadsByLastUpdated.size());
    BOOST_REQUIRE( ! isIdEmpty(user2ThreadsByLastUpdated[0].id));
    BOOST_REQUIRE_EQUAL("Abc-User2", user2ThreadsByLastUpdated[0].name);
    BOOST_REQUIRE_EQUAL(2000, user2ThreadsByLastUpdated[0].created);
    BOOST_REQUIRE_EQUAL(2000, user2ThreadsByLastUpdated[0].lastUpdated);
    BOOST_REQUIRE_EQUAL(0, user2ThreadsByLastUpdated[0].visited);
    BOOST_REQUIRE( ! isIdEmpty(user2ThreadsByLastUpdated[1].id));
    BOOST_REQUIRE_EQUAL("AaDef-User2", user2ThreadsByLastUpdated[1].name);
    BOOST_REQUIRE_EQUAL(1000, user2ThreadsByLastUpdated[1].created);
    BOOST_REQUIRE_EQUAL(3000, user2ThreadsByLastUpdated[1].lastUpdated);
    BOOST_REQUIRE_EQUAL(1, user2ThreadsByLastUpdated[1].visited);

    user2ThreadsByLastUpdated = deserializeUserThreads(
        handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_LAST_UPDATED, SortOrder::Descending, { user2 })
            .get_child("threads"));

    BOOST_REQUIRE_EQUAL(2u, user2ThreadsByLastUpdated.size());
    BOOST_REQUIRE( ! isIdEmpty(user2ThreadsByLastUpdated[0].id));
    BOOST_REQUIRE_EQUAL("AaDef-User2", user2ThreadsByLastUpdated[0].name);
    BOOST_REQUIRE_EQUAL(1000, user2ThreadsByLastUpdated[0].created);
    BOOST_REQUIRE_EQUAL(3000, user2ThreadsByLastUpdated[0].lastUpdated);
    BOOST_REQUIRE_EQUAL(1, user2ThreadsByLastUpdated[0].visited);
    BOOST_REQUIRE( ! isIdEmpty(user2ThreadsByLastUpdated[1].id));
    BOOST_REQUIRE_EQUAL("Abc-User2", user2ThreadsByLastUpdated[1].name);
    BOOST_REQUIRE_EQUAL(2000, user2ThreadsByLastUpdated[1].created);
    BOOST_REQUIRE_EQUAL(2000, user2ThreadsByLastUpdated[1].lastUpdated);
    BOOST_REQUIRE_EQUAL(0, user2ThreadsByLastUpdated[1].visited);
}

BOOST_AUTO_TEST_CASE( Retrieving_discussion_threads_of_user_does_not_show_creating_user )
{
    auto handler = createCommandHandler();

    auto user1 = createUserAndGetId(handler, "User1");
    std::string thread1Id;

    {
        TimestampChanger changer(1000);
        LoggedInUserChanger userChanger(user1);
        thread1Id = createDiscussionThreadAndGetId(handler, "Abc");
    }
    {
        TimestampChanger changer(2000);
        LoggedInUserChanger userChanger(user1);
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { "Def" }));
    }

    auto commands =
            {
                    Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_NAME,
                    Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_CREATED,
                    Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_LAST_UPDATED
            };
    auto sortOrders = { SortOrder::Ascending, SortOrder::Descending };

    for (auto command : commands)
    for (auto sortOrder : sortOrders)
    {
        auto result = handlerToObj(handler, command, sortOrder, { user1 });

        for (auto& item : result.get_child("threads"))
        {
            BOOST_REQUIRE( ! treeContains(item.second, "createdBy"));
        }
    }
}

BOOST_AUTO_TEST_CASE( Retrieving_discussion_threads_of_user_does_not_include_messages )
{
    auto handler = createCommandHandler();

    auto user1 = createUserAndGetId(handler, "User1");
    std::string thread1Id;

    {
        TimestampChanger changer(1000);
        LoggedInUserChanger userChanger(user1);
        thread1Id = createDiscussionThreadAndGetId(handler, "Abc");
    }
    {
        TimestampChanger changer(2000);
        LoggedInUserChanger userChanger(user1);
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { "Def" }));
    }

    auto commands =
            {
                    Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_NAME,
                    Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_CREATED,
                    Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_LAST_UPDATED,
            };
    auto sortOrders = { SortOrder::Ascending, SortOrder::Descending };

    for (auto command : commands)
    for (auto sortOrder : sortOrders)
    {
        auto result = handlerToObj(handler, command, sortOrder, { user1 });

        for (auto& item : result.get_child("threads"))
        {
            BOOST_REQUIRE( ! treeContains(item.second, "messages"));
        }
    }
}

BOOST_AUTO_TEST_CASE( Retrieving_a_user_includes_count_of_discussion_threads_created_by_user )
{
    auto handler = createCommandHandler();

    auto user1 = createUserAndGetId(handler, "User1");
    auto user2 = createUserAndGetId(handler, "User2");

    {
        LoggedInUserChanger changer(user1);
        createDiscussionThreadAndGetId(handler, "Abc");
        createDiscussionThreadAndGetId(handler, "Def");
        createDiscussionThreadAndGetId(handler, "Ghi");
    }

    {
        LoggedInUserChanger changer(user2);
        createDiscussionThreadAndGetId(handler, "Abc2");
        createDiscussionThreadAndGetId(handler, "Def2");
    }

    auto user1Result = handlerToObj(handler, Forum::Commands::GET_USER_BY_ID, { user1 });
    auto user2Result = handlerToObj(handler, Forum::Commands::GET_USER_BY_ID, { user2 });

    BOOST_REQUIRE_EQUAL(3, user1Result.get<int>("user.threadCount"));
    BOOST_REQUIRE_EQUAL(2, user2Result.get<int>("user.threadCount"));
}

BOOST_AUTO_TEST_CASE( Deleted_discussion_threads_are_no_longer_retrieved_when_requesting_threads_of_a_user )
{
    auto handler = createCommandHandler();

    auto user1 = createUserAndGetId(handler, "User1");
    auto user2 = createUserAndGetId(handler, "User2");
    std::string threadToDeleteFromUser1;
    std::string threadToDeleteFromUser2;

    {
        LoggedInUserChanger changer(user1);
        createDiscussionThreadAndGetId(handler, "Abc");
        createDiscussionThreadAndGetId(handler, "Def");
        threadToDeleteFromUser1 = createDiscussionThreadAndGetId(handler, "Ghi");
    }

    {
        LoggedInUserChanger changer(user2);
        createDiscussionThreadAndGetId(handler, "Abc2");
        threadToDeleteFromUser2 = createDiscussionThreadAndGetId(handler, "Def2");
    }

    auto user1Result = handlerToObj(handler, Forum::Commands::GET_USER_BY_ID, { user1 });
    auto user2Result = handlerToObj(handler, Forum::Commands::GET_USER_BY_ID, { user2 });

    BOOST_REQUIRE_EQUAL(3, user1Result.get<int>("user.threadCount"));
    BOOST_REQUIRE_EQUAL(2, user2Result.get<int>("user.threadCount"));

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::DELETE_DISCUSSION_THREAD,
                                                       { threadToDeleteFromUser1 }));
    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::DELETE_DISCUSSION_THREAD,
                                                       { threadToDeleteFromUser2 }));

    user1Result = handlerToObj(handler, Forum::Commands::GET_USER_BY_ID, { user1 });
    user2Result = handlerToObj(handler, Forum::Commands::GET_USER_BY_ID, { user2 });

    BOOST_REQUIRE_EQUAL(2, user1Result.get<int>("user.threadCount"));
    BOOST_REQUIRE_EQUAL(1, user2Result.get<int>("user.threadCount"));

    auto user1Threads = deserializeUserThreads(handlerToObj(handler,
                                                            Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_NAME,
                                                            { user1 }).get_child("threads"));
    BOOST_REQUIRE_EQUAL(2u, user1Threads.size());
    BOOST_REQUIRE( ! isIdEmpty(user1Threads[0].id));
    BOOST_REQUIRE_EQUAL("Abc", user1Threads[0].name);
    BOOST_REQUIRE( ! isIdEmpty(user1Threads[1].id));
    BOOST_REQUIRE_EQUAL("Def", user1Threads[1].name);

    auto user2Threads = deserializeUserThreads(handlerToObj(handler,
                                                            Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_NAME,
                                                            { user2 }).get_child("threads"));
    BOOST_REQUIRE_EQUAL(1u, user2Threads.size());
    BOOST_REQUIRE( ! isIdEmpty(user2Threads[0].id));
    BOOST_REQUIRE_EQUAL("Abc2", user2Threads[0].name);
}

BOOST_AUTO_TEST_CASE( Retrieving_a_user_includes_count_of_discussion_messages_created_by_user )
{
    auto handler = createCommandHandler();

    auto user1 = createUserAndGetId(handler, "User1");
    auto user2 = createUserAndGetId(handler, "User2");

    {
        LoggedInUserChanger changer(user1);
        auto thread1Id = createDiscussionThreadAndGetId(handler, "Abc");
        auto thread2Id = createDiscussionThreadAndGetId(handler, "Def");

        createDiscussionMessageAndGetId(handler, thread1Id, "aaaaaaaaaaa");
        createDiscussionMessageAndGetId(handler, thread1Id, "aaaaaaaaaaa");
        createDiscussionMessageAndGetId(handler, thread1Id, "bbbbbbbbbbb");
        createDiscussionMessageAndGetId(handler, thread2Id, "aaaaaaaaaaa");
    }

    {
        LoggedInUserChanger changer(user2);
        auto threadId = createDiscussionThreadAndGetId(handler, "Abc2");
        createDiscussionMessageAndGetId(handler, threadId, "aaaaaaaaaaa");
    }

    auto user1Result = handlerToObj(handler, Forum::Commands::GET_USER_BY_ID, { user1 });
    auto user2Result = handlerToObj(handler, Forum::Commands::GET_USER_BY_ID, { user2 });

    BOOST_REQUIRE_EQUAL(4, user1Result.get<int>("user.messageCount"));
    BOOST_REQUIRE_EQUAL(1, user2Result.get<int>("user.messageCount"));
}

BOOST_AUTO_TEST_CASE( Retrieving_discussion_messages_of_invalid_user_returns_invalid_parameters )
{
    auto handler = createCommandHandler();

    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS,
                          handlerToObj(handler,
                                       Forum::Commands::GET_DISCUSSION_THREAD_MESSAGES_OF_USER_BY_CREATED,
                                       SortOrder::Ascending, { "bogusId" }));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS,
                          handlerToObj(handler,
                                       Forum::Commands::GET_DISCUSSION_THREAD_MESSAGES_OF_USER_BY_CREATED,
                                       SortOrder::Descending, { "bogusId" }));
}

BOOST_AUTO_TEST_CASE( Discussion_thread_messages_created_user_can_be_retrieved_sorted_by_various_criteria )
{
    auto handler = createCommandHandler();

    auto user1 = createUserAndGetId(handler, "User1");
    std::string thread1Id, thread2Id;

    {
        LoggedInUserChanger changer(user1);
        {
            TimestampChanger timestampChanger(1000);
            thread1Id = createDiscussionThreadAndGetId(handler, "Abc-User1");
            createDiscussionMessageAndGetId(handler, thread1Id, "Msg-1-1000-User1");
        }
        {
            TimestampChanger timestampChanger(3000);
            createDiscussionMessageAndGetId(handler, thread1Id, "Msg-1-3000-User1");
        }
        //increase visited of thread1
        handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { thread1Id });
    }
    auto user2 = createUserAndGetId(handler, "User2");
    {
        LoggedInUserChanger changer(user2);
        {
            TimestampChanger timestampChanger(1500);
            createDiscussionMessageAndGetId(handler, thread1Id, "Msg-1-1500-User2");
        }
        {
            TimestampChanger timestampChanger(2000);
            thread2Id = createDiscussionThreadAndGetId(handler, "Def-User2");
            createDiscussionMessageAndGetId(handler, thread2Id, "Msg-2-2000-User2");
        }
    }
    {
        LoggedInUserChanger changer(user1);
        TimestampChanger timestampChanger(2500);
        createDiscussionMessageAndGetId(handler, thread2Id, "Msg-2-2500-User1");
    }

    auto user1Messages = deserializeUserThreadMessages(
            handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_MESSAGES_OF_USER_BY_CREATED, SortOrder::Ascending, { user1 })
                    .get_child("messages"));

    BOOST_REQUIRE_EQUAL(3u, user1Messages.size());
    BOOST_REQUIRE( ! isIdEmpty(user1Messages[0].id));
    BOOST_REQUIRE_EQUAL("Msg-1-1000-User1", user1Messages[0].content);
    BOOST_REQUIRE_EQUAL(1000, user1Messages[0].created);
    BOOST_REQUIRE_EQUAL(thread1Id, user1Messages[0].parentThread.id);
    BOOST_REQUIRE_EQUAL(1000, user1Messages[0].parentThread.created);
    BOOST_REQUIRE_EQUAL(1000, user1Messages[0].parentThread.lastUpdated);
    BOOST_REQUIRE_EQUAL(1, user1Messages[0].parentThread.visited);
    BOOST_REQUIRE( ! isIdEmpty(user1Messages[1].id));
    BOOST_REQUIRE_EQUAL("Msg-2-2500-User1", user1Messages[1].content);
    BOOST_REQUIRE_EQUAL(2500, user1Messages[1].created);
    BOOST_REQUIRE_EQUAL(thread2Id, user1Messages[1].parentThread.id);
    BOOST_REQUIRE_EQUAL(2000, user1Messages[1].parentThread.created);
    BOOST_REQUIRE_EQUAL(2000, user1Messages[1].parentThread.lastUpdated);
    BOOST_REQUIRE_EQUAL(0, user1Messages[1].parentThread.visited);
    BOOST_REQUIRE( ! isIdEmpty(user1Messages[2].id));
    BOOST_REQUIRE_EQUAL("Msg-1-3000-User1", user1Messages[2].content);
    BOOST_REQUIRE_EQUAL(3000, user1Messages[2].created);
    BOOST_REQUIRE_EQUAL(thread1Id, user1Messages[2].parentThread.id);
    BOOST_REQUIRE_EQUAL(1000, user1Messages[2].parentThread.created);
    BOOST_REQUIRE_EQUAL(1000, user1Messages[2].parentThread.lastUpdated);
    BOOST_REQUIRE_EQUAL(1, user1Messages[2].parentThread.visited);

    auto user2Messages = deserializeUserThreadMessages(
            handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_MESSAGES_OF_USER_BY_CREATED, SortOrder::Ascending, { user2 })
                    .get_child("messages"));

    BOOST_REQUIRE_EQUAL(2u, user2Messages.size());
    BOOST_REQUIRE( ! isIdEmpty(user2Messages[0].id));
    BOOST_REQUIRE_EQUAL("Msg-1-1500-User2", user2Messages[0].content);
    BOOST_REQUIRE_EQUAL(1500, user2Messages[0].created);
    BOOST_REQUIRE_EQUAL(thread1Id, user2Messages[0].parentThread.id);
    BOOST_REQUIRE_EQUAL(1000, user2Messages[0].parentThread.created);
    BOOST_REQUIRE_EQUAL(1000, user2Messages[0].parentThread.lastUpdated);
    BOOST_REQUIRE_EQUAL(1, user2Messages[0].parentThread.visited);
    BOOST_REQUIRE( ! isIdEmpty(user2Messages[1].id));
    BOOST_REQUIRE_EQUAL("Msg-2-2000-User2", user2Messages[1].content);
    BOOST_REQUIRE_EQUAL(2000, user2Messages[1].created);
    BOOST_REQUIRE_EQUAL(thread2Id, user2Messages[1].parentThread.id);
    BOOST_REQUIRE_EQUAL(2000, user2Messages[1].parentThread.created);
    BOOST_REQUIRE_EQUAL(2000, user2Messages[1].parentThread.lastUpdated);
    BOOST_REQUIRE_EQUAL(0, user2Messages[1].parentThread.visited);

    user1Messages = deserializeUserThreadMessages(
            handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_MESSAGES_OF_USER_BY_CREATED, SortOrder::Descending, { user1 })
                    .get_child("messages"));

    BOOST_REQUIRE_EQUAL(3u, user1Messages.size());
    BOOST_REQUIRE( ! isIdEmpty(user1Messages[0].id));
    BOOST_REQUIRE_EQUAL("Msg-1-3000-User1", user1Messages[0].content);
    BOOST_REQUIRE_EQUAL(3000, user1Messages[0].created);
    BOOST_REQUIRE_EQUAL(thread1Id, user1Messages[0].parentThread.id);
    BOOST_REQUIRE_EQUAL(1000, user1Messages[0].parentThread.created);
    BOOST_REQUIRE_EQUAL(1000, user1Messages[0].parentThread.lastUpdated);
    BOOST_REQUIRE_EQUAL(1, user1Messages[0].parentThread.visited);
    BOOST_REQUIRE( ! isIdEmpty(user1Messages[1].id));
    BOOST_REQUIRE_EQUAL("Msg-2-2500-User1", user1Messages[1].content);
    BOOST_REQUIRE_EQUAL(2500, user1Messages[1].created);
    BOOST_REQUIRE_EQUAL(thread2Id, user1Messages[1].parentThread.id);
    BOOST_REQUIRE_EQUAL(2000, user1Messages[1].parentThread.created);
    BOOST_REQUIRE_EQUAL(2000, user1Messages[1].parentThread.lastUpdated);
    BOOST_REQUIRE_EQUAL(0, user1Messages[1].parentThread.visited);
    BOOST_REQUIRE( ! isIdEmpty(user1Messages[2].id));
    BOOST_REQUIRE_EQUAL("Msg-1-1000-User1", user1Messages[2].content);
    BOOST_REQUIRE_EQUAL(1000, user1Messages[2].created);
    BOOST_REQUIRE_EQUAL(thread1Id, user1Messages[2].parentThread.id);
    BOOST_REQUIRE_EQUAL(1000, user1Messages[2].parentThread.created);
    BOOST_REQUIRE_EQUAL(1000, user1Messages[2].parentThread.lastUpdated);
    BOOST_REQUIRE_EQUAL(1, user1Messages[2].parentThread.visited);

    user2Messages = deserializeUserThreadMessages(
            handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_MESSAGES_OF_USER_BY_CREATED, SortOrder::Descending, { user2 })
                    .get_child("messages"));

    BOOST_REQUIRE_EQUAL(2u, user2Messages.size());
    BOOST_REQUIRE( ! isIdEmpty(user2Messages[0].id));
    BOOST_REQUIRE_EQUAL("Msg-2-2000-User2", user2Messages[0].content);
    BOOST_REQUIRE_EQUAL(2000, user2Messages[0].created);
    BOOST_REQUIRE_EQUAL(thread2Id, user2Messages[0].parentThread.id);
    BOOST_REQUIRE_EQUAL(2000, user2Messages[0].parentThread.created);
    BOOST_REQUIRE_EQUAL(2000, user2Messages[0].parentThread.lastUpdated);
    BOOST_REQUIRE_EQUAL(0, user2Messages[0].parentThread.visited);
    BOOST_REQUIRE( ! isIdEmpty(user2Messages[1].id));
    BOOST_REQUIRE_EQUAL("Msg-1-1500-User2", user2Messages[1].content);
    BOOST_REQUIRE_EQUAL(1500, user2Messages[1].created);
    BOOST_REQUIRE_EQUAL(thread1Id, user2Messages[1].parentThread.id);
    BOOST_REQUIRE_EQUAL(1000, user2Messages[1].parentThread.created);
    BOOST_REQUIRE_EQUAL(1000, user2Messages[1].parentThread.lastUpdated);
    BOOST_REQUIRE_EQUAL(1, user2Messages[1].parentThread.visited);
}

BOOST_AUTO_TEST_CASE( Retrieving_discussion_thread_messages_of_user_does_not_show_creating_user )
{
    auto handler = createCommandHandler();

    auto user1 = createUserAndGetId(handler, "User1");
    std::string thread1Id;

    {
        TimestampChanger changer(1000);
        LoggedInUserChanger userChanger(user1);
        thread1Id = createDiscussionThreadAndGetId(handler, "Abc");
        createDiscussionMessageAndGetId(handler, thread1Id, "Message1-User1");
    }
    {
        TimestampChanger changer(2000);
        LoggedInUserChanger userChanger(user1);
        createDiscussionMessageAndGetId(handler, thread1Id, "Message2-User1");
    }

    auto commands =
            {
                    Forum::Commands::GET_DISCUSSION_THREAD_MESSAGES_OF_USER_BY_CREATED,
            };
    auto sortOrders = { SortOrder::Ascending, SortOrder::Descending };

    for (auto command : commands)
    for (auto sortOrder : sortOrders)
    {
        auto result = handlerToObj(handler, command, sortOrder, { user1 });

        for (auto& item : result.get_child("messages"))
        {
            BOOST_REQUIRE( ! treeContains(item.second, "createdBy"));
        }
    }
}

BOOST_AUTO_TEST_CASE( Deleted_discussion_thread_messages_are_no_longer_retrieved_when_requesting_thread_messages_of_a_user )
{
    auto handler = createCommandHandler();

    auto user1 = createUserAndGetId(handler, "User1");
    std::string thread1Id, thread2Id;
    std::string messageToDelete;

    {
        LoggedInUserChanger changer(user1);
        {
            TimestampChanger timestampChanger(1000);
            thread1Id = createDiscussionThreadAndGetId(handler, "Abc-User1");
            createDiscussionMessageAndGetId(handler, thread1Id, "Msg-1-1000-User1");
        }
        {
            TimestampChanger timestampChanger(3000);
            createDiscussionMessageAndGetId(handler, thread1Id, "Msg-1-3000-User1");
        }
        //increase visited of thread1
        handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { thread1Id });
    }
    auto user2 = createUserAndGetId(handler, "User2");
    {
        LoggedInUserChanger changer(user2);
        {
            TimestampChanger timestampChanger(1500);
            createDiscussionMessageAndGetId(handler, thread1Id, "Msg-1-1500-User2");
        }
        {
            TimestampChanger timestampChanger(2000);
            thread2Id = createDiscussionThreadAndGetId(handler, "Def-User2");
            createDiscussionMessageAndGetId(handler, thread2Id, "Msg-2-2000-User2");
        }
    }
    {
        LoggedInUserChanger changer(user1);
        TimestampChanger timestampChanger(2500);
        messageToDelete = createDiscussionMessageAndGetId(handler, thread2Id, "Msg-2-2500-User1");
    }

    auto user1Messages = deserializeUserThreadMessages(
            handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_MESSAGES_OF_USER_BY_CREATED, SortOrder::Ascending, { user1 })
                    .get_child("messages"));

    BOOST_REQUIRE_EQUAL(3u, user1Messages.size());
    BOOST_REQUIRE( ! isIdEmpty(user1Messages[0].id));
    BOOST_REQUIRE_EQUAL("Msg-1-1000-User1", user1Messages[0].content);
    BOOST_REQUIRE_EQUAL(1000, user1Messages[0].created);
    BOOST_REQUIRE_EQUAL(thread1Id, user1Messages[0].parentThread.id);
    BOOST_REQUIRE_EQUAL(1000, user1Messages[0].parentThread.created);
    BOOST_REQUIRE_EQUAL(1000, user1Messages[0].parentThread.lastUpdated);
    BOOST_REQUIRE_EQUAL(1, user1Messages[0].parentThread.visited);
    BOOST_REQUIRE( ! isIdEmpty(user1Messages[1].id));
    BOOST_REQUIRE_EQUAL("Msg-2-2500-User1", user1Messages[1].content);
    BOOST_REQUIRE_EQUAL(2500, user1Messages[1].created);
    BOOST_REQUIRE_EQUAL(thread2Id, user1Messages[1].parentThread.id);
    BOOST_REQUIRE_EQUAL(2000, user1Messages[1].parentThread.created);
    BOOST_REQUIRE_EQUAL(2000, user1Messages[1].parentThread.lastUpdated);
    BOOST_REQUIRE_EQUAL(0, user1Messages[1].parentThread.visited);
    BOOST_REQUIRE( ! isIdEmpty(user1Messages[2].id));
    BOOST_REQUIRE_EQUAL("Msg-1-3000-User1", user1Messages[2].content);
    BOOST_REQUIRE_EQUAL(3000, user1Messages[2].created);
    BOOST_REQUIRE_EQUAL(thread1Id, user1Messages[2].parentThread.id);
    BOOST_REQUIRE_EQUAL(1000, user1Messages[2].parentThread.created);
    BOOST_REQUIRE_EQUAL(1000, user1Messages[2].parentThread.lastUpdated);
    BOOST_REQUIRE_EQUAL(1, user1Messages[2].parentThread.visited);

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::DELETE_DISCUSSION_THREAD_MESSAGE,
                                                       { messageToDelete }));

    user1Messages = deserializeUserThreadMessages(
            handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_MESSAGES_OF_USER_BY_CREATED, SortOrder::Ascending, { user1 })
                    .get_child("messages"));

    BOOST_REQUIRE_EQUAL(2u, user1Messages.size());
    BOOST_REQUIRE( ! isIdEmpty(user1Messages[0].id));
    BOOST_REQUIRE_EQUAL("Msg-1-1000-User1", user1Messages[0].content);
    BOOST_REQUIRE_EQUAL(1000, user1Messages[0].created);
    BOOST_REQUIRE_EQUAL(thread1Id, user1Messages[0].parentThread.id);
    BOOST_REQUIRE_EQUAL(1000, user1Messages[0].parentThread.created);
    BOOST_REQUIRE_EQUAL(1000, user1Messages[0].parentThread.lastUpdated);
    BOOST_REQUIRE_EQUAL(1, user1Messages[0].parentThread.visited);
    BOOST_REQUIRE( ! isIdEmpty(user1Messages[1].id));
    BOOST_REQUIRE_EQUAL("Msg-1-3000-User1", user1Messages[1].content);
    BOOST_REQUIRE_EQUAL(3000, user1Messages[1].created);
    BOOST_REQUIRE_EQUAL(thread1Id, user1Messages[1].parentThread.id);
    BOOST_REQUIRE_EQUAL(1000, user1Messages[1].parentThread.created);
    BOOST_REQUIRE_EQUAL(1000, user1Messages[1].parentThread.lastUpdated);
    BOOST_REQUIRE_EQUAL(1, user1Messages[1].parentThread.visited);
}

BOOST_AUTO_TEST_CASE( Deleting_a_discussion_thread_hides_messages_when_requesting_thread_messages_of_a_user )
{
    auto handler = createCommandHandler();

    auto user1 = createUserAndGetId(handler, "User1");
    std::string thread1Id, thread2Id;

    {
        LoggedInUserChanger changer(user1);
        {
            TimestampChanger timestampChanger(1000);
            thread1Id = createDiscussionThreadAndGetId(handler, "Abc-User1");
            createDiscussionMessageAndGetId(handler, thread1Id, "Msg-1-1000-User1");
        }
        {
            TimestampChanger timestampChanger(3000);
            createDiscussionMessageAndGetId(handler, thread1Id, "Msg-1-3000-User1");
        }
        //increase visited of thread1
        handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { thread1Id });
    }
    auto user2 = createUserAndGetId(handler, "User2");
    {
        LoggedInUserChanger changer(user2);
        {
            TimestampChanger timestampChanger(1500);
            createDiscussionMessageAndGetId(handler, thread1Id, "Msg-1-1500-User2");
        }
        {
            TimestampChanger timestampChanger(2000);
            thread2Id = createDiscussionThreadAndGetId(handler, "Def-User2");
            createDiscussionMessageAndGetId(handler, thread2Id, "Msg-2-2000-User2");
        }
    }
    {
        LoggedInUserChanger changer(user1);
        TimestampChanger timestampChanger(2500);
        createDiscussionMessageAndGetId(handler, thread2Id, "Msg-2-2500-User1");
    }

    auto user1Messages = deserializeUserThreadMessages(
            handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_MESSAGES_OF_USER_BY_CREATED, SortOrder::Ascending, { user1 })
                    .get_child("messages"));

    BOOST_REQUIRE_EQUAL(3u, user1Messages.size());
    BOOST_REQUIRE( ! isIdEmpty(user1Messages[0].id));
    BOOST_REQUIRE_EQUAL("Msg-1-1000-User1", user1Messages[0].content);
    BOOST_REQUIRE_EQUAL(1000, user1Messages[0].created);
    BOOST_REQUIRE_EQUAL(thread1Id, user1Messages[0].parentThread.id);
    BOOST_REQUIRE_EQUAL(1000, user1Messages[0].parentThread.created);
    BOOST_REQUIRE_EQUAL(1000, user1Messages[0].parentThread.lastUpdated);
    BOOST_REQUIRE_EQUAL(1, user1Messages[0].parentThread.visited);
    BOOST_REQUIRE( ! isIdEmpty(user1Messages[1].id));
    BOOST_REQUIRE_EQUAL("Msg-2-2500-User1", user1Messages[1].content);
    BOOST_REQUIRE_EQUAL(2500, user1Messages[1].created);
    BOOST_REQUIRE_EQUAL(thread2Id, user1Messages[1].parentThread.id);
    BOOST_REQUIRE_EQUAL(2000, user1Messages[1].parentThread.created);
    BOOST_REQUIRE_EQUAL(2000, user1Messages[1].parentThread.lastUpdated);
    BOOST_REQUIRE_EQUAL(0, user1Messages[1].parentThread.visited);
    BOOST_REQUIRE( ! isIdEmpty(user1Messages[2].id));
    BOOST_REQUIRE_EQUAL("Msg-1-3000-User1", user1Messages[2].content);
    BOOST_REQUIRE_EQUAL(3000, user1Messages[2].created);
    BOOST_REQUIRE_EQUAL(thread1Id, user1Messages[2].parentThread.id);
    BOOST_REQUIRE_EQUAL(1000, user1Messages[2].parentThread.created);
    BOOST_REQUIRE_EQUAL(1000, user1Messages[2].parentThread.lastUpdated);
    BOOST_REQUIRE_EQUAL(1, user1Messages[2].parentThread.visited);

    BOOST_REQUIRE_EQUAL(2, handlerToObj(handler, Forum::Commands::COUNT_ENTITIES).get<int>("count.discussionThreads"));
    BOOST_REQUIRE_EQUAL(5, handlerToObj(handler, Forum::Commands::COUNT_ENTITIES).get<int>("count.discussionMessages"));

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::DELETE_DISCUSSION_THREAD,
                                                       { thread1Id }));

    BOOST_REQUIRE_EQUAL(1, handlerToObj(handler, Forum::Commands::COUNT_ENTITIES).get<int>("count.discussionThreads"));
    BOOST_REQUIRE_EQUAL(2, handlerToObj(handler, Forum::Commands::COUNT_ENTITIES).get<int>("count.discussionMessages"));

    user1Messages = deserializeUserThreadMessages(
            handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_MESSAGES_OF_USER_BY_CREATED, SortOrder::Ascending, { user1 })
                    .get_child("messages"));

    BOOST_REQUIRE_EQUAL(1u, user1Messages.size());
    BOOST_REQUIRE( ! isIdEmpty(user1Messages[0].id));
    BOOST_REQUIRE_EQUAL("Msg-2-2500-User1", user1Messages[0].content);
    BOOST_REQUIRE_EQUAL(2500, user1Messages[0].created);
    BOOST_REQUIRE_EQUAL(thread2Id, user1Messages[0].parentThread.id);
    BOOST_REQUIRE_EQUAL(2000, user1Messages[0].parentThread.created);
    BOOST_REQUIRE_EQUAL(2000, user1Messages[0].parentThread.lastUpdated);
    BOOST_REQUIRE_EQUAL(0, user1Messages[0].parentThread.visited);
}

BOOST_AUTO_TEST_CASE( Discussion_threads_of_users_can_be_retrieved_sorted_by_message_count_ascending_and_descending )
{
    auto handler = createCommandHandler();

    std::string user1;
    {
        TimestampChanger _(500);
        user1 = createUserAndGetId(handler, "User1");
    }
    std::string thread1Id, thread2Id;
    {
        TimestampChanger _(1000);
        LoggedInUserChanger changer(user1);
        thread1Id = createDiscussionThreadAndGetId(handler, "Abc");
        thread2Id = createDiscussionThreadAndGetId(handler, "Def");
    }
    std::vector<std::string> messagesToDelete;
    {
        LoggedInUserChanger changer(user1);
        {
            TimestampChanger _(1000);
            messagesToDelete.push_back(createDiscussionMessageAndGetId(handler, thread1Id, "aaaaaaaaaaa"));
        }
        {
            TimestampChanger _(3000);
            messagesToDelete.push_back(createDiscussionMessageAndGetId(handler, thread1Id, "ccccccccccc"));
            createDiscussionMessageAndGetId(handler, thread2Id, "ccccccccccc");
        }
    }

    auto threads = deserializeUserThreads(
            handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_MESSAGE_COUNT, SortOrder::Ascending, { user1 })
                    .get_child("threads"));

    BOOST_REQUIRE_EQUAL(2u, threads.size());
    BOOST_REQUIRE_EQUAL("Def", threads[0].name);
    BOOST_REQUIRE_EQUAL(1, threads[0].messageCount);
    BOOST_REQUIRE_EQUAL("Abc", threads[1].name);
    BOOST_REQUIRE_EQUAL(2, threads[1].messageCount);

    threads = deserializeUserThreads(
            handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_MESSAGE_COUNT, SortOrder::Descending, { user1 })
                    .get_child("threads"));

    BOOST_REQUIRE_EQUAL(2u, threads.size());
    BOOST_REQUIRE_EQUAL("Abc", threads[0].name);
    BOOST_REQUIRE_EQUAL(2, threads[0].messageCount);
    BOOST_REQUIRE_EQUAL("Def", threads[1].name);
    BOOST_REQUIRE_EQUAL(1, threads[1].messageCount);

    for (auto& messageId : messagesToDelete)
    {
        assertStatusCodeEqual(StatusCode::OK,
                              handlerToObj(handler, Forum::Commands::DELETE_DISCUSSION_THREAD_MESSAGE, { messageId }));
    }

    threads = deserializeUserThreads(
            handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_MESSAGE_COUNT, SortOrder::Ascending, { user1 })
                    .get_child("threads"));

    BOOST_REQUIRE_EQUAL(2u, threads.size());
    BOOST_REQUIRE_EQUAL("Abc", threads[0].name);
    BOOST_REQUIRE_EQUAL(0, threads[0].messageCount);
    BOOST_REQUIRE_EQUAL("Def", threads[1].name);
    BOOST_REQUIRE_EQUAL(1, threads[1].messageCount);

    threads = deserializeUserThreads(
            handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_MESSAGE_COUNT, SortOrder::Descending, { user1 })
                    .get_child("threads"));

    BOOST_REQUIRE_EQUAL(2u, threads.size());
    BOOST_REQUIRE_EQUAL("Def", threads[0].name);
    BOOST_REQUIRE_EQUAL(1, threads[0].messageCount);
    BOOST_REQUIRE_EQUAL("Abc", threads[1].name);
    BOOST_REQUIRE_EQUAL(0, threads[1].messageCount);

}

BOOST_AUTO_TEST_CASE( Retrieving_users_involves_pagination )
{
    auto handler = createCommandHandler();
    std::vector<std::string> userIds;
    for (size_t i = 0; i < 10; i++)
    {
        userIds.push_back(createUserAndGetId(handler, "User" + std::to_string(i + 101)));
    }
    const int pageSize = 3;

    ConfigChanger _([pageSize](auto& config)
                    {
                        config.user.maxUsersPerPage = pageSize;
                    });

    DisplaySettings settings;
    settings.sortOrder = SortOrder::Ascending;

    //get full pages
    for (size_t i = 0; i < pageSize; i++)
    {
        settings.pageNumber = i;
        auto page = handlerToObj(handler, Forum::Commands::GET_USERS_BY_NAME, settings);

        BOOST_REQUIRE_EQUAL(10, page.get<int>("totalCount"));
        BOOST_REQUIRE_EQUAL(pageSize, page.get<int>("pageSize"));
        BOOST_REQUIRE_EQUAL(settings.pageNumber, page.get<int>("page"));

        auto users = deserializeUsers(page.get_child("users"));
        BOOST_REQUIRE_EQUAL(static_cast<size_t>(pageSize), users.size());

        for (size_t j = 0; j < users.size(); j++)
        {
            BOOST_REQUIRE_EQUAL(userIds[pageSize*i + j], users[j].id);
            BOOST_REQUIRE_EQUAL("User" + std::to_string(pageSize*i + j + 101), users[j].name);
        }
    }

    //get last, partial page
    settings.pageNumber = 3;
    auto page = handlerToObj(handler, Forum::Commands::GET_USERS_BY_NAME, settings);

    BOOST_REQUIRE_EQUAL(10, page.get<int>("totalCount"));
    BOOST_REQUIRE_EQUAL(pageSize, page.get<int>("pageSize"));
    BOOST_REQUIRE_EQUAL(settings.pageNumber, page.get<int>("page"));

    auto users = deserializeUsers(page.get_child("users"));
    BOOST_REQUIRE_EQUAL(1u, users.size());

    BOOST_REQUIRE_EQUAL(userIds[9], users[0].id);
    BOOST_REQUIRE_EQUAL("User110", users[0].name);

    //get empty page
    settings.pageNumber = 4;
    page = handlerToObj(handler, Forum::Commands::GET_USERS_BY_NAME, settings);

    BOOST_REQUIRE_EQUAL(10, page.get<int>("totalCount"));
    BOOST_REQUIRE_EQUAL(pageSize, page.get<int>("pageSize"));
    BOOST_REQUIRE_EQUAL(settings.pageNumber, page.get<int>("page"));

    users = deserializeUsers(page.get_child("users"));
    BOOST_REQUIRE_EQUAL(0u, users.size());
}

BOOST_AUTO_TEST_CASE( Retrieving_users_with_pagination_works_ok_also_in_descending_order )
{
    auto handler = createCommandHandler();
    std::vector<std::string> userIds;
    for (size_t i = 0; i < 10; i++)
    {
        userIds.push_back(createUserAndGetId(handler, "User" + std::to_string(i + 101)));
    }
    const int pageSize = 3;

    ConfigChanger _([pageSize](auto& config)
                    {
                        config.user.maxUsersPerPage = pageSize;
                    });

    DisplaySettings settings;
    settings.sortOrder = SortOrder::Descending;

    //get full pages
    for (size_t i = 0; i < pageSize; i++)
    {
        settings.pageNumber = i;
        auto page = handlerToObj(handler, Forum::Commands::GET_USERS_BY_NAME, settings);

        BOOST_REQUIRE_EQUAL(10, page.get<int>("totalCount"));
        BOOST_REQUIRE_EQUAL(pageSize, page.get<int>("pageSize"));
        BOOST_REQUIRE_EQUAL(settings.pageNumber, page.get<int>("page"));

        auto users = deserializeUsers(page.get_child("users"));
        BOOST_REQUIRE_EQUAL(static_cast<size_t>(pageSize), users.size());

        for (size_t j = 0; j < users.size(); j++)
        {
            BOOST_REQUIRE_EQUAL(userIds[9 - (pageSize*i + j)], users[j].id);
            BOOST_REQUIRE_EQUAL("User" + std::to_string(9 - (pageSize*i + j) + 101), users[j].name);
        }
    }

    //get last, partial page
    settings.pageNumber = 3;
    auto page = handlerToObj(handler, Forum::Commands::GET_USERS_BY_NAME, settings);

    BOOST_REQUIRE_EQUAL(10, page.get<int>("totalCount"));
    BOOST_REQUIRE_EQUAL(pageSize, page.get<int>("pageSize"));
    BOOST_REQUIRE_EQUAL(settings.pageNumber, page.get<int>("page"));

    auto users = deserializeUsers(page.get_child("users"));
    BOOST_REQUIRE_EQUAL(1u, users.size());

    BOOST_REQUIRE_EQUAL(userIds[0], users[0].id);
    BOOST_REQUIRE_EQUAL("User101", users[0].name);

    //get empty page
    settings.pageNumber = 4;
    page = handlerToObj(handler, Forum::Commands::GET_USERS_BY_NAME, settings);

    BOOST_REQUIRE_EQUAL(10, page.get<int>("totalCount"));
    BOOST_REQUIRE_EQUAL(pageSize, page.get<int>("pageSize"));
    BOOST_REQUIRE_EQUAL(settings.pageNumber, page.get<int>("page"));

    users = deserializeUsers(page.get_child("users"));
    BOOST_REQUIRE_EQUAL(0u, users.size());
}

BOOST_AUTO_TEST_CASE( Users_can_be_retrieved_by_discussion_thread_count )
{
    auto handler = createCommandHandler();

    auto user1Id = createUserAndGetId(handler, "User1");
    auto user2Id = createUserAndGetId(handler, "User2");
    auto user3Id = createUserAndGetId(handler, "User3");

    std::string thread1Id, thread2Id;
    std::string threadToDelete;

    {
        LoggedInUserChanger _(user1Id);
        thread1Id = createDiscussionThreadAndGetId(handler, "Thread1");
        thread2Id = createDiscussionThreadAndGetId(handler, "Thread2");
        createDiscussionThreadAndGetId(handler, "Thread2");
        createDiscussionThreadAndGetId(handler, "Thread2");
    }
    {
        LoggedInUserChanger _(user2Id);
        createDiscussionThreadAndGetId(handler, "Thread");
    }
    {
        LoggedInUserChanger _(user3Id);
        createDiscussionThreadAndGetId(handler, "Thread");
        createDiscussionThreadAndGetId(handler, "Thread");
        threadToDelete = createDiscussionThreadAndGetId(handler, "Thread");
    }

    auto users = deserializeUsers(handlerToObj(handler, Forum::Commands::GET_USERS_BY_THREAD_COUNT).get_child("users"));

    BOOST_REQUIRE_EQUAL(3u, users.size());

    BOOST_REQUIRE_EQUAL(user2Id, users[0].id);
    BOOST_REQUIRE_EQUAL("User2", users[0].name);
    BOOST_REQUIRE_EQUAL(1, users[0].threadCount);

    BOOST_REQUIRE_EQUAL(user3Id, users[1].id);
    BOOST_REQUIRE_EQUAL("User3", users[1].name);
    BOOST_REQUIRE_EQUAL(3, users[1].threadCount);

    BOOST_REQUIRE_EQUAL(user1Id, users[2].id);
    BOOST_REQUIRE_EQUAL("User1", users[2].name);
    BOOST_REQUIRE_EQUAL(4, users[2].threadCount);

    {
        LoggedInUserChanger _(user1Id);
        createDiscussionThreadAndGetId(handler, "Thread");
    }
    {
        LoggedInUserChanger _(user3Id);
        deleteDiscussionThread(handler, threadToDelete);
    }

    users = deserializeUsers(handlerToObj(handler, Forum::Commands::GET_USERS_BY_THREAD_COUNT).get_child("users"));

    BOOST_REQUIRE_EQUAL(3u, users.size());

    BOOST_REQUIRE_EQUAL(user2Id, users[0].id);
    BOOST_REQUIRE_EQUAL("User2", users[0].name);
    BOOST_REQUIRE_EQUAL(1, users[0].threadCount);

    BOOST_REQUIRE_EQUAL(user3Id, users[1].id);
    BOOST_REQUIRE_EQUAL("User3", users[1].name);
    BOOST_REQUIRE_EQUAL(2, users[1].threadCount);

    BOOST_REQUIRE_EQUAL(user1Id, users[2].id);
    BOOST_REQUIRE_EQUAL("User1", users[2].name);
    BOOST_REQUIRE_EQUAL(5, users[2].threadCount);

    {
        LoggedInUserChanger _(user1Id);
        handlerToObj(handler, Forum::Commands::MERGE_DISCUSSION_THREADS, { thread1Id, thread2Id });
    }

    users = deserializeUsers(handlerToObj(handler, Forum::Commands::GET_USERS_BY_THREAD_COUNT).get_child("users"));

    BOOST_REQUIRE_EQUAL(3u, users.size());

    BOOST_REQUIRE_EQUAL(user2Id, users[0].id);
    BOOST_REQUIRE_EQUAL("User2", users[0].name);
    BOOST_REQUIRE_EQUAL(1, users[0].threadCount);

    BOOST_REQUIRE_EQUAL(user3Id, users[1].id);
    BOOST_REQUIRE_EQUAL("User3", users[1].name);
    BOOST_REQUIRE_EQUAL(2, users[1].threadCount);

    BOOST_REQUIRE_EQUAL(user1Id, users[2].id);
    BOOST_REQUIRE_EQUAL("User1", users[2].name);
    BOOST_REQUIRE_EQUAL(4, users[2].threadCount);
}

BOOST_AUTO_TEST_CASE( Users_can_be_retrieved_by_discussion_message_count )
{
    auto handler = createCommandHandler();

    auto user1Id = createUserAndGetId(handler, "User1");
    auto user2Id = createUserAndGetId(handler, "User2");
    auto user3Id = createUserAndGetId(handler, "User3");

    std::string thread1Id, thread2Id, thread3Id;
    std::string messageToDelete;

    {
        LoggedInUserChanger _(user1Id);
        thread1Id = createDiscussionThreadAndGetId(handler, "Thread1");
        thread2Id = createDiscussionThreadAndGetId(handler, "Thread2");
        thread3Id = createDiscussionThreadAndGetId(handler, "Thread3");

        createDiscussionMessageAndGetId(handler, thread1Id, "Message");
        createDiscussionMessageAndGetId(handler, thread2Id, "Message");

        messageToDelete = createDiscussionMessageAndGetId(handler, thread1Id, "Message");
    }
    {
        LoggedInUserChanger _(user2Id);
        createDiscussionMessageAndGetId(handler, thread1Id, "Message");
    }
    {
        LoggedInUserChanger _(user3Id);
        createDiscussionMessageAndGetId(handler, thread1Id, "Message");
        createDiscussionMessageAndGetId(handler, thread1Id, "Message");

        createDiscussionMessageAndGetId(handler, thread2Id, "Message");
        createDiscussionMessageAndGetId(handler, thread2Id, "Message");
    }

    auto users = deserializeUsers(handlerToObj(handler, Forum::Commands::GET_USERS_BY_MESSAGE_COUNT).get_child("users"));

    BOOST_REQUIRE_EQUAL(3u, users.size());

    BOOST_REQUIRE_EQUAL(user2Id, users[0].id);
    BOOST_REQUIRE_EQUAL("User2", users[0].name);
    BOOST_REQUIRE_EQUAL(1, users[0].messageCount);

    BOOST_REQUIRE_EQUAL(user1Id, users[1].id);
    BOOST_REQUIRE_EQUAL("User1", users[1].name);
    BOOST_REQUIRE_EQUAL(3, users[1].messageCount);

    BOOST_REQUIRE_EQUAL(user3Id, users[2].id);
    BOOST_REQUIRE_EQUAL("User3", users[2].name);
    BOOST_REQUIRE_EQUAL(4, users[2].messageCount);

    {
        LoggedInUserChanger _(user3Id);
        createDiscussionMessageAndGetId(handler, thread2Id, "Message");
    }
    {
        LoggedInUserChanger _(user1Id);
        deleteDiscussionThreadMessage(handler, messageToDelete);
    }

    users = deserializeUsers(handlerToObj(handler, Forum::Commands::GET_USERS_BY_MESSAGE_COUNT).get_child("users"));

    BOOST_REQUIRE_EQUAL(3u, users.size());

    BOOST_REQUIRE_EQUAL(user2Id, users[0].id);
    BOOST_REQUIRE_EQUAL("User2", users[0].name);
    BOOST_REQUIRE_EQUAL(1, users[0].messageCount);

    BOOST_REQUIRE_EQUAL(user1Id, users[1].id);
    BOOST_REQUIRE_EQUAL("User1", users[1].name);
    BOOST_REQUIRE_EQUAL(2, users[1].messageCount);

    BOOST_REQUIRE_EQUAL(user3Id, users[2].id);
    BOOST_REQUIRE_EQUAL("User3", users[2].name);
    BOOST_REQUIRE_EQUAL(5, users[2].messageCount);

    {
        LoggedInUserChanger _(user1Id);
        deleteDiscussionThread(handler, thread1Id);
    }

    users = deserializeUsers(handlerToObj(handler, Forum::Commands::GET_USERS_BY_MESSAGE_COUNT).get_child("users"));

    BOOST_REQUIRE_EQUAL(3u, users.size());

    BOOST_REQUIRE_EQUAL(user2Id, users[0].id);
    BOOST_REQUIRE_EQUAL("User2", users[0].name);
    BOOST_REQUIRE_EQUAL(0, users[0].messageCount);

    BOOST_REQUIRE_EQUAL(user1Id, users[1].id);
    BOOST_REQUIRE_EQUAL("User1", users[1].name);
    BOOST_REQUIRE_EQUAL(1, users[1].messageCount);

    BOOST_REQUIRE_EQUAL(user3Id, users[2].id);
    BOOST_REQUIRE_EQUAL("User3", users[2].name);
    BOOST_REQUIRE_EQUAL(3, users[2].messageCount);

    {
        LoggedInUserChanger _(user1Id);
        handlerToObj(handler, Forum::Commands::MERGE_DISCUSSION_THREADS, { thread2Id, thread3Id });
    }

    users = deserializeUsers(handlerToObj(handler, Forum::Commands::GET_USERS_BY_MESSAGE_COUNT).get_child("users"));

    BOOST_REQUIRE_EQUAL(3u, users.size());

    BOOST_REQUIRE_EQUAL(user2Id, users[0].id);
    BOOST_REQUIRE_EQUAL("User2", users[0].name);
    BOOST_REQUIRE_EQUAL(0, users[0].messageCount);

    BOOST_REQUIRE_EQUAL(user1Id, users[1].id);
    BOOST_REQUIRE_EQUAL("User1", users[1].name);
    BOOST_REQUIRE_EQUAL(1, users[1].messageCount);

    BOOST_REQUIRE_EQUAL(user3Id, users[2].id);
    BOOST_REQUIRE_EQUAL("User3", users[2].name);
    BOOST_REQUIRE_EQUAL(3, users[2].messageCount);
}
