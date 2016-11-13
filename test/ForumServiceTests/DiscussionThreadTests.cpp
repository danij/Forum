#include <vector>

#include "CommandsCommon.h"
#include "DelegateObserver.h"
#include "EntityCollection.h"
#include "RandomGenerator.h"
#include "TestHelpers.h"

using namespace Forum::Configuration;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;

/**
 * Stores only the information that is sent out about a user referenced in a discussion message
 */
struct SerializedDiscussionMessageUser
{
    std::string id;
    std::string name;
    Timestamp created = 0;
    Timestamp lastSeen = 0;
};

struct SerializedDiscussionMessage
{
    std::string id;
    std::string content;
    Timestamp created = 0;
    SerializedDiscussionMessageUser createdBy;

    void populate(const boost::property_tree::ptree& tree)
    {
        id = tree.get<std::string>("id");
        content = tree.get<std::string>("content");
        created = tree.get<Timestamp>("created");

        createdBy.id = tree.get<std::string>("createdBy.id");
        createdBy.name = tree.get<std::string>("createdBy.name");
        createdBy.created = tree.get<Timestamp>("createdBy.created");
        createdBy.lastSeen = tree.get<Timestamp>("createdBy.lastSeen");
    }
};

auto deserializeMessage(const boost::property_tree::ptree& tree)
{
    SerializedDiscussionMessage result;
    result.populate(tree);
    return result;
}

auto deserializeMessages(const boost::property_tree::ptree& collection)
{
    std::vector<SerializedDiscussionMessage> result;
    for (auto& tree : collection)
    {
        result.push_back(deserializeMessage(tree.second));
    }
    return result;
}

/**
 * Stores only the information that is sent out about a user referenced in a discussion thread
 */
struct SerializedDiscussionThreadUser
{
    std::string id;
    std::string name;
    Timestamp created = 0;
    Timestamp lastSeen = 0;
};

struct SerializedDiscussionThread
{
    std::string id;
    std::string name;
    Timestamp created = 0;
    Timestamp lastUpdated = 0;
    SerializedDiscussionThreadUser createdBy;
    int64_t visited = 0;
    std::vector<SerializedDiscussionMessage> messages;

    void populate(const boost::property_tree::ptree& tree)
    {
        id = tree.get<std::string>("id");
        name = tree.get<std::string>("name");
        created = tree.get<Timestamp>("created");
        lastUpdated = tree.get<Timestamp>("lastUpdated");
        visited = tree.get<int64_t>("visited");

        createdBy.id = tree.get<std::string>("createdBy.id");
        createdBy.name = tree.get<std::string>("createdBy.name");
        createdBy.created = tree.get<Timestamp>("createdBy.created");
        createdBy.lastSeen = tree.get<Timestamp>("createdBy.lastSeen");

        for (auto& pair : tree)
        {
            if (pair.first == "messages")
            {
                messages = deserializeMessages(pair.second);
            }
        }
    }
};

auto deserializeThread(const boost::property_tree::ptree& tree)
{
    SerializedDiscussionThread result;
    result.populate(tree);
    return result;
}

auto deserializeThreads(const boost::property_tree::ptree& collection)
{
    std::vector<SerializedDiscussionThread> result;
    for (auto& tree : collection)
    {
        result.push_back(deserializeThread(tree.second));
    }
    return result;
}

BOOST_AUTO_TEST_CASE( Discussion_thread_count_is_initially_zero )
{
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::COUNT_ENTITIES);
    BOOST_REQUIRE_EQUAL(0, returnObject.get<int>("count.discussionThreads"));
}

BOOST_AUTO_TEST_CASE( Retrieving_discussion_threads_invokes_observer )
{
    int observerCalledNTimes = 0;
    auto handler = createCommandHandler();

    DisposingDelegateObserver observer(*handler);
    observer->getDiscussionThreadsAction = [&](auto& _) { observerCalledNTimes += 1; };

    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_NAME);
    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_CREATED_ASCENDING);
    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_CREATED_DESCENDING);
    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_LAST_UPDATED_ASCENDING);
    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_LAST_UPDATED_DESCENDING);

    BOOST_REQUIRE_EQUAL(5, observerCalledNTimes);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_thread_with_no_parameters_fails )
{
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_DISCUSSION_THREAD);
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_thread_with_empty_name_fails )
{
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_DISCUSSION_THREAD, { "" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_thread_returns_the_id_name_and_created )
{
    TimestampChanger changer(20000);
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_DISCUSSION_THREAD, { "Foo" });

    assertStatusCodeEqual(StatusCode::OK, returnObject);
    BOOST_REQUIRE( ! isIdEmpty(returnObject.get<std::string>("id")));
    BOOST_REQUIRE_EQUAL("Foo", returnObject.get<std::string>("name"));
    BOOST_REQUIRE_EQUAL(20000, returnObject.get<Timestamp>("created"));
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_thread_with_only_whitespace_in_the_name_fails )
{
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_DISCUSSION_THREAD, { " \t\r\n" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_thread_with_leading_whitespace_in_the_name_fails )
{
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_DISCUSSION_THREAD, { " Foo" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_thread_with_trailing_whitespace_in_the_name_fails )
{
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_DISCUSSION_THREAD, { "Foo\t" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_thread_with_a_too_short_name_fails )
{
    auto config = getGlobalConfig();
    std::string name(config->discussionThread.minNameLength - 1, 'a');
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_DISCUSSION_THREAD, { name });
    assertStatusCodeEqual(StatusCode::VALUE_TOO_SHORT, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_thread_with_a_longer_name_fails )
{
    auto config = getGlobalConfig();
    std::string name(config->discussionThread.maxNameLength + 1, 'a');
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_DISCUSSION_THREAD, { name });
    assertStatusCodeEqual(StatusCode::VALUE_TOO_LONG, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_thread_with_unicode_name_of_valid_length_succeeds )
{
    auto configWithShorterName = ConfigChanger([](auto& config)
                                               {
                                                   config.discussionThread.maxNameLength = 3;
                                               });

    //test a simple text that can also be represented as ASCII
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_DISCUSSION_THREAD, { "AAA" });
    assertStatusCodeEqual(StatusCode::OK, returnObject);

    //test a 3 characters text that requires multiple bytes for representation using UTF-8
    returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_DISCUSSION_THREAD, { "早上好" });
    assertStatusCodeEqual(StatusCode::OK, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_thread_with_a_name_that_contains_invalid_characters_fails_with_appropriate_message)
{
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_DISCUSSION_THREAD, { "\xFF\xFF" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( A_discussion_thread_that_was_created_can_be_retrieved_and_has_a_distinct_id )
{
    auto handler = createCommandHandler();
    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { "Thread1" }));
    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { "Thread2" }));

    auto threads = deserializeThreads(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_NAME)
                                              .get_child("threads"));

    BOOST_REQUIRE( ! isIdEmpty(threads[0].id));
    BOOST_REQUIRE( ! isIdEmpty(threads[1].id));
    BOOST_REQUIRE_NE(threads[0].id, threads[1].id);
    BOOST_REQUIRE_EQUAL("Thread1", threads[0].name);
    BOOST_REQUIRE_EQUAL("Thread2", threads[1].name);
}

BOOST_AUTO_TEST_CASE( Discussion_threads_can_be_retrieved_by_id )
{
    auto handler = createCommandHandler();
    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { "Abc" }));
    auto result = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { "Def" });
    assertStatusCodeEqual(StatusCode::OK, result);
    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { "Ghi" }));

    auto newThreadId = result.get<std::string>("id");
    SerializedDiscussionThread thread;
    thread.populate(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { newThreadId })
                            .get_child("thread"));

    BOOST_REQUIRE_EQUAL(newThreadId, thread.id);
    BOOST_REQUIRE_EQUAL("Def", thread.name);
}

BOOST_AUTO_TEST_CASE( Retrieving_discussion_threads_by_id_invokes_observer )
{
    std::string idOfThread;
    auto handler = createCommandHandler();

    DisposingDelegateObserver observer(*handler);
    observer->getDiscussionThreadByIdAction = [&](auto& _, auto& id) { idOfThread = static_cast<std::string>(id); };

    auto idToSearch = static_cast<std::string>(generateUUIDString());
    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { idToSearch });
    BOOST_REQUIRE_EQUAL(idToSearch, idOfThread);
}

BOOST_AUTO_TEST_CASE( Modifying_a_discussion_thread_name_succeds )
{
    auto handler = createCommandHandler();
    auto result = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { "Abc" });
    assertStatusCodeEqual(StatusCode::OK, result);

    BOOST_REQUIRE_EQUAL("Abc", result.get<std::string>("name"));
    auto threadId = result.get<std::string>("id");

    BOOST_REQUIRE_EQUAL(1, handlerToObj(handler, Forum::Commands::COUNT_ENTITIES).get<int>("count.discussionThreads"));

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_THREAD_NAME,
                                                       { threadId, "Xyz" }));
    SerializedDiscussionThread modifiedThread;
    modifiedThread.populate(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { threadId })
                                    .get_child("thread"));

    BOOST_REQUIRE_EQUAL(1, handlerToObj(handler, Forum::Commands::COUNT_ENTITIES).get<int>("count.discussionThreads"));
    BOOST_REQUIRE_EQUAL("Xyz", modifiedThread.name);
    BOOST_REQUIRE_EQUAL(threadId, modifiedThread.id);
}

BOOST_AUTO_TEST_CASE( Modifying_a_inexistent_discussion_thread_name_returns_not_found )
{
    auto handler = createCommandHandler();
    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { "Abc" }));
    assertStatusCodeEqual(StatusCode::NOT_FOUND,
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_THREAD_NAME, { "bogus id", "Xyz" }));
}

BOOST_AUTO_TEST_CASE( Modifying_a_discussion_thread_invokes_observer )
{
    std::string newName;
    auto threadChange = DiscussionThread::ChangeType::None;
    auto handler = createCommandHandler();

    DisposingDelegateObserver observer(*handler);
    observer->changeDiscussionThreadAction = [&](auto& _, auto& thread, auto change)
    {
        newName = thread.name();
        threadChange = change;
    };

    auto threadId = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { "Abc" }).get<std::string>("id");

    handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_THREAD_NAME, { threadId, "Xyz" });
    BOOST_REQUIRE_EQUAL("Xyz", newName);
    BOOST_REQUIRE_EQUAL(DiscussionThread::ChangeType::Name, threadChange);
}

BOOST_AUTO_TEST_CASE( Multiple_discussion_threads_can_be_share_the_same_name )
{
    auto handler = createCommandHandler();
    std::vector<std::string> names = { "Abc", "abc", "Åbc" };

    Timestamp currentTime = 1000;
    for (auto& name : names)
    {
        TimestampChanger timestampChanger(currentTime);
        currentTime += 1000;
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { name }));
    }

    BOOST_REQUIRE_EQUAL(3, handlerToObj(handler, Forum::Commands::COUNT_ENTITIES).get<int>("count.discussionThreads"));

    auto threads = deserializeThreads(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_NAME)
                                              .get_child("threads"));

    BOOST_REQUIRE_EQUAL(names.size(), threads.size());
    BOOST_REQUIRE_EQUAL("Abc", threads[0].name);
    BOOST_REQUIRE_EQUAL("abc", threads[1].name);
    BOOST_REQUIRE_EQUAL("Åbc", threads[2].name);
    BOOST_REQUIRE_EQUAL(1000, threads[0].created);
    BOOST_REQUIRE_EQUAL(2000, threads[1].created);
    BOOST_REQUIRE_EQUAL(3000, threads[2].created);
}

BOOST_AUTO_TEST_CASE( Discussion_threads_can_be_retrieved_sorted_by_name )
{
    auto handler = createCommandHandler();
    std::vector<std::string> names = { "Abc", "Ghi", "Def" };

    for (auto& name : names)
    {
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { name }));
    }

    BOOST_REQUIRE_EQUAL(3, handlerToObj(handler, Forum::Commands::COUNT_ENTITIES).get<int>("count.discussionThreads"));

    auto threads = deserializeThreads(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_NAME)
                                              .get_child("threads"));

    BOOST_REQUIRE_EQUAL(names.size(), threads.size());
    BOOST_REQUIRE_EQUAL("Abc", threads[0].name);
    BOOST_REQUIRE_EQUAL("Def", threads[1].name);
    BOOST_REQUIRE_EQUAL("Ghi", threads[2].name);
}

BOOST_AUTO_TEST_CASE( Discussion_threads_can_be_retrieved_sorted_by_creation_date_ascending )
{
    auto handler = createCommandHandler();
    std::vector<std::string> names = { "Abc", "Ghi", "Def" };

    Timestamp currentTime = 1000;
    for (auto& name : names)
    {
        TimestampChanger timestampChanger(currentTime);
        currentTime += 1000;
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { name }));
    }

    BOOST_REQUIRE_EQUAL(3, handlerToObj(handler, Forum::Commands::COUNT_ENTITIES).get<int>("count.discussionThreads"));

    auto threads = deserializeThreads(
            handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_CREATED_ASCENDING).get_child("threads"));

    BOOST_REQUIRE_EQUAL(names.size(), threads.size());
    BOOST_REQUIRE_EQUAL("Abc", threads[0].name);
    BOOST_REQUIRE_EQUAL("Ghi", threads[1].name);
    BOOST_REQUIRE_EQUAL("Def", threads[2].name);
    BOOST_REQUIRE_EQUAL(1000, threads[0].created);
    BOOST_REQUIRE_EQUAL(2000, threads[1].created);
    BOOST_REQUIRE_EQUAL(3000, threads[2].created);
}

BOOST_AUTO_TEST_CASE( Discussion_threads_can_be_retrieved_sorted_by_creation_date_descending )
{
    auto handler = createCommandHandler();
    std::vector<std::string> names = { "Abc", "Ghi", "Def" };

    Timestamp currentTime = 1000;
    for (auto& name : names)
    {
        TimestampChanger timestampChanger(currentTime);
        currentTime += 1000;
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { name }));
    }

    BOOST_REQUIRE_EQUAL(3, handlerToObj(handler, Forum::Commands::COUNT_ENTITIES).get<int>("count.discussionThreads"));

    auto threads = deserializeThreads(
            handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_CREATED_DESCENDING).get_child("threads"));

    BOOST_REQUIRE_EQUAL(names.size(), threads.size());
    BOOST_REQUIRE_EQUAL("Def", threads[0].name);
    BOOST_REQUIRE_EQUAL("Ghi", threads[1].name);
    BOOST_REQUIRE_EQUAL("Abc", threads[2].name);
    BOOST_REQUIRE_EQUAL(3000, threads[0].created);
    BOOST_REQUIRE_EQUAL(2000, threads[1].created);
    BOOST_REQUIRE_EQUAL(1000, threads[2].created);
}

BOOST_AUTO_TEST_CASE( Discussion_threads_can_be_retrieved_sorted_by_last_updated_ascending_and_descending )
{
    auto handler = createCommandHandler();
    std::vector<std::string> names = { "Abc", "Ghi", "Def" };
    std::vector<std::string> ids;

    Timestamp currentTime = 1000;
    for (auto& name : names)
    {
        TimestampChanger timestampChanger(currentTime);
        currentTime += 1000;
        auto result = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { name });
        assertStatusCodeEqual(StatusCode::OK, result);
        ids.push_back(result.get<std::string>("id"));
    }

    BOOST_REQUIRE_EQUAL(3, handlerToObj(handler, Forum::Commands::COUNT_ENTITIES).get<int>("count.discussionThreads"));

    {
        TimestampChanger timestampChanger(currentTime);
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_THREAD_NAME,
                                                           { ids[0], "Aabc" }));
    }

    auto threads = deserializeThreads(handlerToObj(handler,
                                                   Forum::Commands::GET_DISCUSSION_THREADS_BY_LAST_UPDATED_ASCENDING)
                                              .get_child("threads"));

    BOOST_REQUIRE_EQUAL(names.size(), threads.size());
    BOOST_REQUIRE_EQUAL("Ghi", threads[0].name);
    BOOST_REQUIRE_EQUAL("Def", threads[1].name);
    BOOST_REQUIRE_EQUAL("Aabc", threads[2].name);
    BOOST_REQUIRE_EQUAL(2000, threads[0].lastUpdated);
    BOOST_REQUIRE_EQUAL(3000, threads[1].lastUpdated);
    BOOST_REQUIRE_EQUAL(4000, threads[2].lastUpdated);
    BOOST_REQUIRE_EQUAL(2000, threads[0].created);
    BOOST_REQUIRE_EQUAL(3000, threads[1].created);
    BOOST_REQUIRE_EQUAL(1000, threads[2].created);

    threads = deserializeThreads(handlerToObj(handler,
                                              Forum::Commands::GET_DISCUSSION_THREADS_BY_LAST_UPDATED_DESCENDING)
                                          .get_child("threads"));

    BOOST_REQUIRE_EQUAL(names.size(), threads.size());
    BOOST_REQUIRE_EQUAL("Aabc", threads[0].name);
    BOOST_REQUIRE_EQUAL("Def", threads[1].name);
    BOOST_REQUIRE_EQUAL("Ghi", threads[2].name);
    BOOST_REQUIRE_EQUAL(4000, threads[0].lastUpdated);
    BOOST_REQUIRE_EQUAL(3000, threads[1].lastUpdated);
    BOOST_REQUIRE_EQUAL(2000, threads[2].lastUpdated);
    BOOST_REQUIRE_EQUAL(1000, threads[0].created);
    BOOST_REQUIRE_EQUAL(3000, threads[1].created);
    BOOST_REQUIRE_EQUAL(2000, threads[2].created);
}

BOOST_AUTO_TEST_CASE( Deleting_a_discussion_thread_with_an_invalid_id_returns_invalid_parameters )
{
    auto handler = createCommandHandler();
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS,
                          handlerToObj(handler, Forum::Commands::DELETE_DISCUSSION_THREAD, { "bogus id" }));
}

BOOST_AUTO_TEST_CASE( Deleting_an_inexistent_discussion_thread_returns_not_found )
{
    auto handler = createCommandHandler();
    assertStatusCodeEqual(StatusCode::NOT_FOUND,
                          handlerToObj(handler, Forum::Commands::DELETE_DISCUSSION_THREAD, { sampleValidIdString }));
}

BOOST_AUTO_TEST_CASE( Deleted_discussion_threads_can_no_longer_be_retrieved )
{
    auto handler = createCommandHandler();
    std::vector<std::string> names = { "Abc", "Ghi", "Def" };
    std::vector<std::string> ids;

    for (auto& name : names)
    {
        auto result = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { name });
        assertStatusCodeEqual(StatusCode::OK, result);
        ids.push_back(result.get<std::string>("id"));
    }

    BOOST_REQUIRE_EQUAL(3, handlerToObj(handler, Forum::Commands::COUNT_ENTITIES).get<int>("count.discussionThreads"));

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::DELETE_DISCUSSION_THREAD, { ids[0] }));
    assertStatusCodeEqual(StatusCode::NOT_FOUND, handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID,
                                                              { ids[0] }));

    BOOST_REQUIRE_EQUAL(2, handlerToObj(handler, Forum::Commands::COUNT_ENTITIES).get<int>("count.discussionThreads"));

    auto threads = deserializeThreads(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_NAME)
                                              .get_child("threads"));

    BOOST_REQUIRE_EQUAL(names.size() - 1, threads.size());
    BOOST_REQUIRE_EQUAL("Def", threads[0].name);
    BOOST_REQUIRE_EQUAL("Ghi", threads[1].name);
}

BOOST_AUTO_TEST_CASE( Deleting_a_discussion_thread_invokes_observer )
{
    std::string deletedThreadId;
    auto handler = createCommandHandler();

    DisposingDelegateObserver observer(*handler);
    observer->deleteDiscussionThreadAction = [&](auto& _, auto& thread)
    {
        deletedThreadId = static_cast<std::string>(thread.id());
    };

    auto threadId = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { "Abc" }).get<std::string>("id");

    handlerToObj(handler, Forum::Commands::DELETE_DISCUSSION_THREAD, { threadId });
    BOOST_REQUIRE_EQUAL(threadId, deletedThreadId);
}

BOOST_AUTO_TEST_CASE( Retrieving_discussion_threads_returns_creation_and_last_update_dates )
{
    auto handler = createCommandHandler();

    {
        TimestampChanger changer(1000);
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { "Abc" }));
    }
    {
        TimestampChanger changer(2000);
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { "Def" }));
    }

    auto result = handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_CREATED_ASCENDING);
    auto threads = deserializeThreads(result.get_child("threads"));

    BOOST_REQUIRE( ! isIdEmpty(threads[0].id));
    BOOST_REQUIRE_EQUAL("Abc", threads[0].name);
    BOOST_REQUIRE_EQUAL(1000, threads[0].created);
    BOOST_REQUIRE_EQUAL(1000, threads[0].lastUpdated);

    BOOST_REQUIRE( ! isIdEmpty(threads[1].id));
    BOOST_REQUIRE_EQUAL("Def", threads[1].name);
    BOOST_REQUIRE_EQUAL(2000, threads[1].created);
    BOOST_REQUIRE_EQUAL(2000, threads[1].lastUpdated);
}

BOOST_AUTO_TEST_CASE( Retrieving_discussion_threads_returns_each_user_that_created_them )
{
    auto handler = createCommandHandler();

    auto user1 = createUserAndGetId(handler, "User1");
    auto user2 = createUserAndGetId(handler, "User2");

    {
        TimestampChanger changer(1000);
        LoggedInUserChanger userChanger(user2);
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { "Abc" }));
    }
    {
        TimestampChanger changer(2000);
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { "Def" }));
    }
    {
        TimestampChanger changer(3000);
        LoggedInUserChanger userChanger(user1);
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { "Ghi" }));
    }

    auto result = handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_CREATED_ASCENDING);
    auto threads = deserializeThreads(result.get_child("threads"));

    BOOST_REQUIRE( ! isIdEmpty(threads[0].id));
    BOOST_REQUIRE_EQUAL("Abc", threads[0].name);
    BOOST_REQUIRE( ! isIdEmpty(threads[0].createdBy.id));
    BOOST_REQUIRE_EQUAL("User2", threads[0].createdBy.name);

    BOOST_REQUIRE( ! isIdEmpty(threads[1].id));
    BOOST_REQUIRE_EQUAL("Def", threads[1].name);
    BOOST_REQUIRE(isIdEmpty(threads[1].createdBy.id));
    BOOST_REQUIRE_EQUAL(AnonymousUser->name(), threads[1].createdBy.name);

    BOOST_REQUIRE( ! isIdEmpty(threads[2].id));
    BOOST_REQUIRE_EQUAL("Ghi", threads[2].name);
    BOOST_REQUIRE( ! isIdEmpty(threads[2].createdBy.id));
    BOOST_REQUIRE_EQUAL("User1", threads[2].createdBy.name);
}

BOOST_AUTO_TEST_CASE( Retrieving_discussion_threads_does_not_show_other_topics_of_creating_user )
{
    auto handler = createCommandHandler();

    auto user1 = createUserAndGetId(handler, "User1");
    std::string thread1Id;

    {
        TimestampChanger changer(1000);
        LoggedInUserChanger userChanger(user1);
        thread1Id = static_cast<std::string>(createDiscussionThreadAndGetId(handler, "Abc"));
    }
    {
        TimestampChanger changer(2000);
        LoggedInUserChanger userChanger(user1);
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { "Def" }));
    }

    auto result = handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_CREATED_ASCENDING);
    auto resultThreads = result.get_child("threads");
    auto threads = deserializeThreads(resultThreads);

    BOOST_REQUIRE( ! isIdEmpty(threads[0].id));
    BOOST_REQUIRE_EQUAL("Abc", threads[0].name);
    BOOST_REQUIRE( ! isIdEmpty(threads[0].createdBy.id));
    BOOST_REQUIRE_EQUAL("User1", threads[0].createdBy.name);

    BOOST_REQUIRE( ! isIdEmpty(threads[1].id));
    BOOST_REQUIRE_EQUAL("Def", threads[1].name);
    BOOST_REQUIRE( ! isIdEmpty(threads[1].createdBy.id));
    BOOST_REQUIRE_EQUAL("User1", threads[1].createdBy.name);

    for (auto& item : resultThreads)
    {
        auto createdBy = item.second.get_child("createdBy");
        BOOST_REQUIRE( ! treeContains(createdBy, "threads"));
    }

    auto thread1 = handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { thread1Id });
    auto thread1CreatedBy = thread1.get_child("thread.createdBy");
    BOOST_REQUIRE( ! treeContains(thread1CreatedBy, "threads"));
}

BOOST_AUTO_TEST_CASE( Deleting_a_user_removes_all_discussion_threads_created_by_that_user )
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

    auto threads = deserializeThreads(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_NAME)
                                              .get_child("threads"));

    BOOST_REQUIRE_EQUAL(5, threads.size());
    BOOST_REQUIRE( ! isIdEmpty(threads[0].id));
    BOOST_REQUIRE_EQUAL("Abc", threads[0].name);
    BOOST_REQUIRE( ! isIdEmpty(threads[1].id));
    BOOST_REQUIRE_EQUAL("Abc2", threads[1].name);
    BOOST_REQUIRE( ! isIdEmpty(threads[2].id));
    BOOST_REQUIRE_EQUAL("Def", threads[2].name);
    BOOST_REQUIRE( ! isIdEmpty(threads[3].id));
    BOOST_REQUIRE_EQUAL("Def2", threads[3].name);
    BOOST_REQUIRE( ! isIdEmpty(threads[4].id));
    BOOST_REQUIRE_EQUAL("Ghi", threads[4].name);

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::DELETE_USER, { user1 }));

    threads = deserializeThreads(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_NAME)
                                              .get_child("threads"));

    BOOST_REQUIRE_EQUAL(2, threads.size());
    BOOST_REQUIRE( ! isIdEmpty(threads[0].id));
    BOOST_REQUIRE_EQUAL("Abc2", threads[0].name);
    BOOST_REQUIRE( ! isIdEmpty(threads[1].id));
    BOOST_REQUIRE_EQUAL("Def2", threads[1].name);
}

BOOST_AUTO_TEST_CASE( Retrieving_discussion_threads_increments_the_visited_counter_only_when_individual_threads_are_accessed )
{
    auto handler = createCommandHandler();

    auto user1 = createUserAndGetId(handler, "User1");
    auto user2 = createUserAndGetId(handler, "User2");
    std::string thread1Id, thread2Id;

    {
        TimestampChanger changer(1000);
        LoggedInUserChanger userChanger(user1);
        thread1Id = createDiscussionThreadAndGetId(handler, "Abc");
    }
    {
        TimestampChanger changer(2000);
        LoggedInUserChanger userChanger(user2);
        thread2Id = createDiscussionThreadAndGetId(handler, "Def");
    }

    auto result = handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_CREATED_ASCENDING);
    auto threads = deserializeThreads(result.get_child("threads"));

    BOOST_REQUIRE( ! isIdEmpty(threads[0].id));
    BOOST_REQUIRE_EQUAL("Abc", threads[0].name);
    BOOST_REQUIRE_EQUAL(0, threads[0].visited);

    BOOST_REQUIRE( ! isIdEmpty(threads[1].id));
    BOOST_REQUIRE_EQUAL("Def", threads[1].name);
    BOOST_REQUIRE_EQUAL(0, threads[1].visited);

    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { thread1Id });
    {
        LoggedInUserChanger userChanger(user1);
        handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_NAME);
        handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { thread1Id });
        handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { thread2Id });
        handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { thread1Id });
    }
    {
        LoggedInUserChanger userChanger(user2);
        handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_CREATED_ASCENDING);
        handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_CREATED_DESCENDING);
        handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_LAST_UPDATED_ASCENDING);
        handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_LAST_UPDATED_DESCENDING);
        handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { thread1Id });
        handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { thread2Id });
    }

    result = handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_CREATED_ASCENDING);
    threads = deserializeThreads(result.get_child("threads"));

    BOOST_REQUIRE( ! isIdEmpty(threads[0].id));
    BOOST_REQUIRE_EQUAL("Abc", threads[0].name);
    BOOST_REQUIRE_EQUAL(4, threads[0].visited);

    BOOST_REQUIRE( ! isIdEmpty(threads[1].id));
    BOOST_REQUIRE_EQUAL("Def", threads[1].name);
    BOOST_REQUIRE_EQUAL(2, threads[1].visited);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_message_returns_the_id_parent_id_and_created )
{
    TimestampChanger changer(20000);
    auto handler = createCommandHandler();
    auto threadId = createDiscussionThreadAndGetId(handler, "Abc");

    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE,
                                     { threadId, sampleMessageContent });

    assertStatusCodeEqual(StatusCode::OK, returnObject);
    BOOST_REQUIRE( ! isIdEmpty(returnObject.get<std::string>("id")));
    BOOST_REQUIRE_EQUAL(threadId, returnObject.get<std::string>("parentId"));
    BOOST_REQUIRE_EQUAL(20000, returnObject.get<Timestamp>("created"));
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_message_without_specifying_the_discussion_thread_fails )
{
    TimestampChanger changer(20000);

    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { "Foo" });

    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_message_for_a_bogus_discussion_thread_fails )
{
    TimestampChanger changer(20000);
    auto handler = createCommandHandler();

    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, handlerToObj(handler,
                                                                       Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE,
                                                                       { "bogus", sampleMessageContent }));
    assertStatusCodeEqual(StatusCode::NOT_FOUND, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE,
                                                              { sampleValidIdString, sampleMessageContent }));
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_message_with_only_whitespace_in_the_name_fails )
{
    auto handler = createCommandHandler();
    auto threadId = createDiscussionThreadAndGetId(handler, "Abc");

    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { threadId, " \t\r\n" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_message_with_leading_whitespace_in_the_name_fails )
{
    auto handler = createCommandHandler();
    auto threadId = createDiscussionThreadAndGetId(handler, "Abc");

    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { threadId, " Foo" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_message_with_trailing_whitespace_in_the_name_fails )
{
    auto handler = createCommandHandler();
    auto threadId = createDiscussionThreadAndGetId(handler, "Abc");

    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { threadId, "Foo\t" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_message_with_a_too_short_name_fails )
{
    auto config = getGlobalConfig();
    std::string content(config->discussionMessage.minContentLength - 1, 'a');

    auto handler = createCommandHandler();
    auto threadId = createDiscussionThreadAndGetId(handler, "Abc");

    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { threadId, content });
    assertStatusCodeEqual(StatusCode::VALUE_TOO_SHORT, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_message_with_a_longer_name_fails )
{
    auto config = getGlobalConfig();
    std::string content(config->discussionMessage.maxContentLength + 1, 'a');

    auto handler = createCommandHandler();
    auto threadId = createDiscussionThreadAndGetId(handler, "Abc");

    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { threadId, content });
    assertStatusCodeEqual(StatusCode::VALUE_TOO_LONG, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_message_with_unicode_name_of_valid_length_succeeds )
{
    auto configWithShorterName = ConfigChanger([](auto& config)
                                               {
                                                   config.discussionMessage.minContentLength = 3;
                                                   config.discussionMessage.maxContentLength = 3;
                                               });

    auto handler = createCommandHandler();
    auto threadId = createDiscussionThreadAndGetId(handler, "Abc");

    //test a simple text that can also be represented as ASCII
    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { threadId, "AAA" });
    assertStatusCodeEqual(StatusCode::OK, returnObject);

    //test a 3 characters text that requires multiple bytes for representation using UTF-8
    returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { threadId, "早上好" });
    assertStatusCodeEqual(StatusCode::OK, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_message_with_a_name_that_contains_invalid_characters_fails)
{
    auto handler = createCommandHandler();
    auto threadId = createDiscussionThreadAndGetId(handler, "Abc");

    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { threadId, "\xFF\xFF" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Retrieving_a_discussion_thread_also_returns_messages_ordered_by_their_creation_date )
{
    auto handler = createCommandHandler();

    auto user1 = createUserAndGetId(handler, "User1");
    auto user2 = createUserAndGetId(handler, "User2");

    std::string thread1Id;
    std::string thread2Id;

    {
        LoggedInUserChanger changer(user1);
        thread1Id = createDiscussionThreadAndGetId(handler, "Abc");
    }
    {
        LoggedInUserChanger changer(user2);
        thread2Id = createDiscussionThreadAndGetId(handler, "Def");
    }
    {
        LoggedInUserChanger changer(user1);
        {
            TimestampChanger _(1000);
            handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { thread1Id, "aaaaaaaaaaa" });
            handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { thread2Id, "11111111111" });
        }
        {
            TimestampChanger _(3000);
            handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { thread1Id, "ccccccccccc" });
        }
    }
    {
        LoggedInUserChanger changer(user2);
        {
            TimestampChanger _(2000);
            handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { thread1Id, "bbbbbbbbbbb" });
            handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { thread2Id, "22222222222" });
        }
    }

    auto thread1 = deserializeThread(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { thread1Id })
            .get_child("thread"));

    BOOST_REQUIRE_EQUAL(thread1Id, thread1.id);
    BOOST_REQUIRE_EQUAL("Abc", thread1.name);
    BOOST_REQUIRE_EQUAL(3, thread1.messages.size());

    BOOST_REQUIRE( ! isIdEmpty(thread1.messages[0].id));
    BOOST_REQUIRE_EQUAL("aaaaaaaaaaa", thread1.messages[0].content);
    BOOST_REQUIRE_EQUAL(1000, thread1.messages[0].created);
    BOOST_REQUIRE_EQUAL(user1, thread1.messages[0].createdBy.id);
    BOOST_REQUIRE_EQUAL("User1", thread1.messages[0].createdBy.name);
    BOOST_REQUIRE( ! isIdEmpty(thread1.messages[1].id));
    BOOST_REQUIRE_EQUAL("bbbbbbbbbbb", thread1.messages[1].content);
    BOOST_REQUIRE_EQUAL(2000, thread1.messages[1].created);
    BOOST_REQUIRE_EQUAL(user2, thread1.messages[1].createdBy.id);
    BOOST_REQUIRE_EQUAL("User2", thread1.messages[1].createdBy.name);
    BOOST_REQUIRE( ! isIdEmpty(thread1.messages[2].id));
    BOOST_REQUIRE_EQUAL("ccccccccccc", thread1.messages[2].content);
    BOOST_REQUIRE_EQUAL(3000, thread1.messages[2].created);
    BOOST_REQUIRE_EQUAL(user1, thread1.messages[2].createdBy.id);
    BOOST_REQUIRE_EQUAL("User1", thread1.messages[2].createdBy.name);

    auto thread2 = deserializeThread(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { thread2Id })
            .get_child("thread"));

    BOOST_REQUIRE_EQUAL(thread2Id, thread2.id);
    BOOST_REQUIRE_EQUAL("Def", thread2.name);
    BOOST_REQUIRE_EQUAL(2, thread2.messages.size());

    BOOST_REQUIRE( ! isIdEmpty(thread2.messages[0].id));
    BOOST_REQUIRE_EQUAL("11111111111", thread2.messages[0].content);
    BOOST_REQUIRE_EQUAL(1000, thread2.messages[0].created);
    BOOST_REQUIRE_EQUAL(user1, thread2.messages[0].createdBy.id);
    BOOST_REQUIRE_EQUAL("User1", thread2.messages[0].createdBy.name);
    BOOST_REQUIRE( ! isIdEmpty(thread2.messages[1].id));
    BOOST_REQUIRE_EQUAL("22222222222", thread2.messages[1].content);
    BOOST_REQUIRE_EQUAL(2000, thread2.messages[1].created);
    BOOST_REQUIRE_EQUAL(user2, thread2.messages[1].createdBy.id);
    BOOST_REQUIRE_EQUAL("User2", thread2.messages[1].createdBy.name);
}

BOOST_AUTO_TEST_CASE( Retrieving_discussion_threads_does_not_show_messages )
{
    auto handler = createCommandHandler();

    auto user1 = createUserAndGetId(handler, "User1");
    std::string thread1Id;

    {
        TimestampChanger changer(1000);
        LoggedInUserChanger userChanger(user1);
        thread1Id = static_cast<std::string>(createDiscussionThreadAndGetId(handler, "Abc"));
    }
    {
        TimestampChanger changer(2000);
        LoggedInUserChanger userChanger(user1);
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { "Def" }));
    }

    auto result = handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_CREATED_ASCENDING);
    auto resultThreads = result.get_child("threads");
    auto threads = deserializeThreads(resultThreads);

    BOOST_REQUIRE( ! isIdEmpty(threads[0].id));
    BOOST_REQUIRE_EQUAL("Abc", threads[0].name);
    BOOST_REQUIRE( ! isIdEmpty(threads[0].createdBy.id));
    BOOST_REQUIRE_EQUAL("User1", threads[0].createdBy.name);

    BOOST_REQUIRE( ! isIdEmpty(threads[1].id));
    BOOST_REQUIRE_EQUAL("Def", threads[1].name);
    BOOST_REQUIRE( ! isIdEmpty(threads[1].createdBy.id));
    BOOST_REQUIRE_EQUAL("User1", threads[1].createdBy.name);

    for (auto& item : resultThreads)
    {
        BOOST_REQUIRE( ! treeContains(item.second, "messages"));
    }
}

BOOST_AUTO_TEST_CASE( Retrieving_a_discussion_thread_also_returns_messages_but_excludes_each_parent_thread )
{
    auto handler = createCommandHandler();

    auto user1 = createUserAndGetId(handler, "User1");
    std::string thread1Id;

    {
        LoggedInUserChanger changer(user1);
        TimestampChanger _(1000);

        thread1Id = createDiscussionThreadAndGetId(handler, "Abc");
        handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { thread1Id, "aaaaaaaaaaa" });
        handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { thread1Id, "bbbbbbbbbbb" });
    }

    auto result = handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { thread1Id }).get_child("thread");
    auto thread = deserializeThread(result);

    BOOST_REQUIRE( ! isIdEmpty(thread.id));
    BOOST_REQUIRE_EQUAL("Abc", thread.name);
    BOOST_REQUIRE( ! isIdEmpty(thread.createdBy.id));
    BOOST_REQUIRE_EQUAL("User1", thread.createdBy.name);

    for (auto& item : result.get_child("messages"))
    {
        BOOST_REQUIRE( ! treeContains(item.second, "parentThread"));
    }
}

BOOST_AUTO_TEST_CASE( Deleting_a_discussion_message_with_an_invalid_id_returns_invalid_parameters )
{
    auto handler = createCommandHandler();
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, handlerToObj(handler,
                                                                       Forum::Commands::DELETE_DISCUSSION_THREAD_MESSAGE,
                                                                       { "bogus id" }));
    assertStatusCodeEqual(StatusCode::NOT_FOUND, handlerToObj(handler,
                                                              Forum::Commands::DELETE_DISCUSSION_THREAD_MESSAGE,
                                                              { sampleValidIdString }));
}

BOOST_AUTO_TEST_CASE( Deleting_an_inexistent_discussion_message_returns_not_found )
{
    auto handler = createCommandHandler();
    assertStatusCodeEqual(StatusCode::NOT_FOUND, handlerToObj(handler,
                                                              Forum::Commands::DELETE_DISCUSSION_THREAD_MESSAGE,
                                                              { sampleValidIdString }));
}

BOOST_AUTO_TEST_CASE( Deleting_a_discussion_message_invokes_observer )
{
    std::string deletedMessageId;
    auto handler = createCommandHandler();

    DisposingDelegateObserver observer(*handler);
    observer->deleteDiscussionMessageAction = [&](auto& _, auto& message)
    {
        deletedMessageId = static_cast<std::string>(message.id());
    };

    auto threadId = createDiscussionThreadAndGetId(handler, "Abc");
    auto messageId = createDiscussionMessageAndGetId(handler, threadId, "aaaaaaaaaaa");

    handlerToObj(handler, Forum::Commands::DELETE_DISCUSSION_THREAD_MESSAGE, { messageId });
    BOOST_REQUIRE_EQUAL(messageId, deletedMessageId);
}

BOOST_AUTO_TEST_CASE( Deleted_discussion_messages_no_longer_retrieved )
{
    auto handler = createCommandHandler();

    auto user1 = createUserAndGetId(handler, "User1");
    auto user2 = createUserAndGetId(handler, "User2");

    std::string thread1Id, thread2Id;
    std::string message1Id, message2Id;

    {
        LoggedInUserChanger changer(user1);
        thread1Id = createDiscussionThreadAndGetId(handler, "Abc");
    }
    {
        LoggedInUserChanger changer(user2);
        thread2Id = createDiscussionThreadAndGetId(handler, "Def");
    }
    {
        LoggedInUserChanger changer(user1);
        {
            TimestampChanger _(1000);
            message1Id = createDiscussionMessageAndGetId(handler, thread1Id, "aaaaaaaaaaa");
            handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { thread2Id, "11111111111" });
        }
        {
            TimestampChanger _(3000);
            handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { thread1Id, "ccccccccccc" });
        }
    }
    {
        LoggedInUserChanger changer(user2);
        {
            TimestampChanger _(2000);
            handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { thread1Id, "bbbbbbbbbbb" });
            message2Id = createDiscussionMessageAndGetId(handler, thread2Id, "22222222222");
        }
    }

    BOOST_REQUIRE_EQUAL(5, handlerToObj(handler, Forum::Commands::COUNT_ENTITIES).get<int>("count.discussionMessages"));

    auto thread1 = deserializeThread(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { thread1Id })
                                             .get_child("thread"));

    BOOST_REQUIRE_EQUAL(thread1Id, thread1.id);
    BOOST_REQUIRE_EQUAL("Abc", thread1.name);
    BOOST_REQUIRE_EQUAL(3, thread1.messages.size());

    BOOST_REQUIRE( ! isIdEmpty(thread1.messages[0].id));
    BOOST_REQUIRE_EQUAL("aaaaaaaaaaa", thread1.messages[0].content);
    BOOST_REQUIRE_EQUAL(1000, thread1.messages[0].created);
    BOOST_REQUIRE_EQUAL(user1, thread1.messages[0].createdBy.id);
    BOOST_REQUIRE_EQUAL("User1", thread1.messages[0].createdBy.name);
    BOOST_REQUIRE( ! isIdEmpty(thread1.messages[1].id));
    BOOST_REQUIRE_EQUAL("bbbbbbbbbbb", thread1.messages[1].content);
    BOOST_REQUIRE_EQUAL(2000, thread1.messages[1].created);
    BOOST_REQUIRE_EQUAL(user2, thread1.messages[1].createdBy.id);
    BOOST_REQUIRE_EQUAL("User2", thread1.messages[1].createdBy.name);
    BOOST_REQUIRE( ! isIdEmpty(thread1.messages[2].id));
    BOOST_REQUIRE_EQUAL("ccccccccccc", thread1.messages[2].content);
    BOOST_REQUIRE_EQUAL(3000, thread1.messages[2].created);
    BOOST_REQUIRE_EQUAL(user1, thread1.messages[2].createdBy.id);
    BOOST_REQUIRE_EQUAL("User1", thread1.messages[2].createdBy.name);

    auto thread2 = deserializeThread(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { thread2Id })
                                             .get_child("thread"));

    BOOST_REQUIRE_EQUAL(thread2Id, thread2.id);
    BOOST_REQUIRE_EQUAL("Def", thread2.name);
    BOOST_REQUIRE_EQUAL(2, thread2.messages.size());

    BOOST_REQUIRE( ! isIdEmpty(thread2.messages[0].id));
    BOOST_REQUIRE_EQUAL("11111111111", thread2.messages[0].content);
    BOOST_REQUIRE_EQUAL(1000, thread2.messages[0].created);
    BOOST_REQUIRE_EQUAL(user1, thread2.messages[0].createdBy.id);
    BOOST_REQUIRE_EQUAL("User1", thread2.messages[0].createdBy.name);
    BOOST_REQUIRE( ! isIdEmpty(thread2.messages[1].id));
    BOOST_REQUIRE_EQUAL("22222222222", thread2.messages[1].content);
    BOOST_REQUIRE_EQUAL(2000, thread2.messages[1].created);
    BOOST_REQUIRE_EQUAL(user2, thread2.messages[1].createdBy.id);
    BOOST_REQUIRE_EQUAL("User2", thread2.messages[1].createdBy.name);

    assertStatusCodeEqual(StatusCode::OK,
                          handlerToObj(handler, Forum::Commands::DELETE_DISCUSSION_THREAD_MESSAGE, { message1Id }));
    assertStatusCodeEqual(StatusCode::OK,
                          handlerToObj(handler, Forum::Commands::DELETE_DISCUSSION_THREAD_MESSAGE, { message2Id }));

    BOOST_REQUIRE_EQUAL(3, handlerToObj(handler, Forum::Commands::COUNT_ENTITIES).get<int>("count.discussionMessages"));

    thread1 = deserializeThread(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { thread1Id })
                                             .get_child("thread"));

    BOOST_REQUIRE_EQUAL(thread1Id, thread1.id);
    BOOST_REQUIRE_EQUAL("Abc", thread1.name);
    BOOST_REQUIRE_EQUAL(2, thread1.messages.size());

    BOOST_REQUIRE( ! isIdEmpty(thread1.messages[0].id));
    BOOST_REQUIRE_EQUAL("bbbbbbbbbbb", thread1.messages[0].content);
    BOOST_REQUIRE_EQUAL(2000, thread1.messages[0].created);
    BOOST_REQUIRE_EQUAL(user2, thread1.messages[0].createdBy.id);
    BOOST_REQUIRE_EQUAL("User2", thread1.messages[0].createdBy.name);
    BOOST_REQUIRE( ! isIdEmpty(thread1.messages[1].id));
    BOOST_REQUIRE_EQUAL("ccccccccccc", thread1.messages[1].content);
    BOOST_REQUIRE_EQUAL(3000, thread1.messages[1].created);
    BOOST_REQUIRE_EQUAL(user1, thread1.messages[1].createdBy.id);
    BOOST_REQUIRE_EQUAL("User1", thread1.messages[1].createdBy.name);

    thread2 = deserializeThread(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { thread2Id })
                                             .get_child("thread"));

    BOOST_REQUIRE_EQUAL(thread2Id, thread2.id);
    BOOST_REQUIRE_EQUAL("Def", thread2.name);
    BOOST_REQUIRE_EQUAL(1, thread2.messages.size());

    BOOST_REQUIRE( ! isIdEmpty(thread2.messages[0].id));
    BOOST_REQUIRE_EQUAL("11111111111", thread2.messages[0].content);
    BOOST_REQUIRE_EQUAL(1000, thread2.messages[0].created);
    BOOST_REQUIRE_EQUAL(user1, thread2.messages[0].createdBy.id);
    BOOST_REQUIRE_EQUAL("User1", thread2.messages[0].createdBy.name);
}

BOOST_AUTO_TEST_CASE( Deleting_a_user_removes_all_messages_created_by_that_user )
{
    auto handler = createCommandHandler();

    auto user1 = createUserAndGetId(handler, "User1");
    auto user2 = createUserAndGetId(handler, "User2");

    std::string threadId;

    {
        LoggedInUserChanger changer(user1);
        threadId = createDiscussionThreadAndGetId(handler, "Abc");
    }
    {
        LoggedInUserChanger changer(user1);
        {
            TimestampChanger _(1000);
            handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { threadId, "aaaaaaaaaaa" });
        }
        {
            TimestampChanger _(3000);
            handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { threadId, "ccccccccccc" });
        }
    }
    {
        LoggedInUserChanger changer(user2);
        {
            TimestampChanger _(2000);
            handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { threadId, "bbbbbbbbbbb" });
        }
    }
    BOOST_REQUIRE_EQUAL(3, handlerToObj(handler, Forum::Commands::COUNT_ENTITIES).get<int>("count.discussionMessages"));

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::DELETE_USER, { user2 }));

    BOOST_REQUIRE_EQUAL(2, handlerToObj(handler, Forum::Commands::COUNT_ENTITIES).get<int>("count.discussionMessages"));

    auto thread = deserializeThread(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { threadId })
                                    .get_child("thread"));

    BOOST_REQUIRE_EQUAL(threadId, thread.id);
    BOOST_REQUIRE_EQUAL("Abc", thread.name);
    BOOST_REQUIRE_EQUAL(2, thread.messages.size());

    BOOST_REQUIRE(! isIdEmpty(thread.messages[0].id));
    BOOST_REQUIRE_EQUAL("aaaaaaaaaaa", thread.messages[0].content);
    BOOST_REQUIRE_EQUAL(1000, thread.messages[0].created);
    BOOST_REQUIRE_EQUAL(user1, thread.messages[0].createdBy.id);
    BOOST_REQUIRE_EQUAL("User1", thread.messages[0].createdBy.name);
    BOOST_REQUIRE(! isIdEmpty(thread.messages[1].id));
    BOOST_REQUIRE_EQUAL("ccccccccccc", thread.messages[1].content);
    BOOST_REQUIRE_EQUAL(3000, thread.messages[1].created);
    BOOST_REQUIRE_EQUAL(user1, thread.messages[1].createdBy.id);
    BOOST_REQUIRE_EQUAL("User1", thread.messages[1].createdBy.name);
}
