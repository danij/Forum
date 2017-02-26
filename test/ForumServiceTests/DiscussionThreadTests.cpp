#include "CommandsCommon.h"
#include "EntityCollection.h"
#include "TestHelpers.h"

#include <algorithm>
#include <vector>

using namespace Forum::Configuration;
using namespace Forum::Context;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;

/**
 * Stores only the information that is sent out about a user referenced in a discussion thread or message
 */
struct SerializedDiscussionThreadOrMessageUser
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

struct SerializedLatestDiscussionThreadMessage
{
    Timestamp created = 0;
    SerializedDiscussionThreadOrMessageUser createdBy;

    void populate(const boost::property_tree::ptree& tree)
    {
        created = tree.get<Timestamp>("created");

        createdBy.populate(tree.get_child("createdBy"));
    }
};

struct SerializedDiscussionMessageVote
{
    std::string userId;
    std::string userName;
    Timestamp at;

    void populate(const boost::property_tree::ptree& tree)
    {
        userId = tree.get<std::string>("userId");
        userName = tree.get<std::string>("userName");
        at = tree.get<Timestamp>("at");
    }
};

static bool SerializedDiscussionMessageVoteLess(const SerializedDiscussionMessageVote& first, 
                                                const SerializedDiscussionMessageVote& second)
{
    return first.at < second.at;
};

struct SerializedDiscussionMessageLastUpdated
{
    Timestamp at = 0;
    std::string userId;
    std::string userName;
    std::string ip;
    std::string userAgent;

    void populate(const boost::property_tree::ptree& tree)
    {
        userId = tree.get<std::string>("userId", "");
        userName = tree.get<std::string>("userName", "");
        at = tree.get<Timestamp>("at", 0);
        ip = tree.get<std::string>("ip", "");
        userAgent = tree.get<std::string>("userAgent", "");
    }
};

struct SerializedDiscussionMessage
{
    std::string id;
    std::string content;
    Timestamp created = 0;
    std::string ip;
    std::string userAgent;
    SerializedDiscussionThreadOrMessageUser createdBy;
    std::vector<SerializedDiscussionMessageVote> upVotes;
    std::vector<SerializedDiscussionMessageVote> downVotes;
    std::unique_ptr<SerializedDiscussionMessageLastUpdated> lastUpdated;

    void populate(const boost::property_tree::ptree& tree)
    {
        id = tree.get<std::string>("id");
        content = tree.get<std::string>("content");
        created = tree.get<Timestamp>("created");
        ip = tree.get<std::string>("ip", "");
        userAgent = tree.get<std::string>("userAgent", "");
        for (auto& pair : tree)
        {
            if (pair.first == "lastUpdated")
            {
                lastUpdated = std::make_unique<SerializedDiscussionMessageLastUpdated>();
                lastUpdated->populate(pair.second);
            }
            //votes need to be sorted on the client to avoid the complexity of using multi-index containers on each message
            if (pair.first == "upVotes")
            {
                upVotes = deserializeEntities<SerializedDiscussionMessageVote>(tree.get_child("upVotes"));
                std::sort(upVotes.begin(), upVotes.end(), SerializedDiscussionMessageVoteLess);
            }
            if (pair.first == "downVotes")
            {
                downVotes = deserializeEntities<SerializedDiscussionMessageVote>(tree.get_child("downVotes"));
                std::sort(downVotes.begin(), downVotes.end(), SerializedDiscussionMessageVoteLess);
            }
        }

        createdBy.populate(tree.get_child("createdBy"));
    }
};

struct SerializedDiscussionThread
{
    std::string id;
    std::string name;
    Timestamp created = 0;
    Timestamp lastUpdated = 0;
    SerializedDiscussionThreadOrMessageUser createdBy;
    int64_t visited = 0;
    int64_t messageCount = 0;
    std::vector<SerializedDiscussionMessage> messages;
    SerializedLatestDiscussionThreadMessage latestMessage;
    bool visitedSinceLastChange = false;
    int64_t voteScore = 0;
    
    void populate(const boost::property_tree::ptree& tree)
    {
        id = tree.get<std::string>("id");
        name = tree.get<std::string>("name");
        created = tree.get<Timestamp>("created");
        lastUpdated = tree.get<Timestamp>("lastUpdated");
        visited = tree.get<int64_t>("visited");
        messageCount = tree.get<int64_t>("messageCount");
        visitedSinceLastChange = tree.get<bool>("visitedSinceLastChange", false);
        voteScore = tree.get<int64_t>("voteScore");

        createdBy.populate(tree.get_child("createdBy"));
        for (auto& pair : tree)
        {
            if (pair.first == "latestMessage")
            {
                latestMessage.populate(pair.second);
            }
        }

        for (auto& pair : tree)
        {
            if (pair.first == "messages")
            {
                messages = deserializeEntities<SerializedDiscussionMessage>(pair.second);
            }
        }
    }
};

CREATE_FUNCTION_ALIAS(deserializeThread, deserializeEntity<SerializedDiscussionThread>)
CREATE_FUNCTION_ALIAS(deserializeThreads, deserializeEntities<SerializedDiscussionThread>)

BOOST_AUTO_TEST_CASE( Discussion_thread_count_is_initially_zero )
{
    auto handler = createCommandHandler();
    auto returnObject = handlerToObj(handler, Forum::Commands::COUNT_ENTITIES);
    BOOST_REQUIRE_EQUAL(0, returnObject.get<int>("count.discussionThreads"));
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_thread_with_no_parameters_fails )
{
    auto handler = createCommandHandler();
    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD);
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_thread_with_empty_name_fails )
{
    auto handler = createCommandHandler();
    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { "" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_thread_returns_the_id_name_and_created )
{
    TimestampChanger changer(20000);
    auto handler = createCommandHandler();
    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { "Foo" });

    assertStatusCodeEqual(StatusCode::OK, returnObject);
    BOOST_REQUIRE( ! isIdEmpty(returnObject.get<std::string>("id")));
    BOOST_REQUIRE_EQUAL("Foo", returnObject.get<std::string>("name"));
    BOOST_REQUIRE_EQUAL(20000, returnObject.get<Timestamp>("created"));
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_thread_with_only_whitespace_in_the_name_fails )
{
    auto handler = createCommandHandler();
    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { " \t\r\n" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_thread_with_leading_whitespace_in_the_name_fails )
{
    auto handler = createCommandHandler();
    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { " Foo" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_thread_with_trailing_whitespace_in_the_name_fails )
{
    auto handler = createCommandHandler();
    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { "Foo\t" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_thread_with_a_too_short_name_fails )
{
    auto config = getGlobalConfig();
    std::string name(config->discussionThread.minNameLength - 1, 'a');
    auto handler = createCommandHandler();
    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { name });
    assertStatusCodeEqual(StatusCode::VALUE_TOO_SHORT, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_thread_with_a_longer_name_fails )
{
    auto config = getGlobalConfig();
    std::string name(config->discussionThread.maxNameLength + 1, 'a');
    auto handler = createCommandHandler();
    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { name });
    assertStatusCodeEqual(StatusCode::VALUE_TOO_LONG, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_thread_with_unicode_name_of_valid_length_succeeds )
{
    ConfigChanger _([](auto& config)
                    {
                        config.discussionThread.maxNameLength = 3;
                    });

    //test a simple text that can also be represented as ASCII
    auto handler = createCommandHandler();
    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { "AAA" });
    assertStatusCodeEqual(StatusCode::OK, returnObject);

    //test a 3 characters text that requires multiple bytes for representation using UTF-8
    returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { "早上好" });
    assertStatusCodeEqual(StatusCode::OK, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_thread_with_a_name_that_contains_invalid_characters_fails_with_appropriate_message)
{
    auto handler = createCommandHandler();
    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { "\xFF\xFF\xFF\xFF" });
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

    auto threads = deserializeThreads(handlerToObj(handler, 
                                                   Forum::Commands::GET_DISCUSSION_THREADS_BY_NAME, 
                                                   SortOrder::Ascending).get_child("threads"));

    BOOST_REQUIRE_EQUAL(names.size(), threads.size());
    BOOST_REQUIRE_EQUAL("Abc", threads[0].name);
    BOOST_REQUIRE_EQUAL("Def", threads[1].name);
    BOOST_REQUIRE_EQUAL("Ghi", threads[2].name);

    threads = deserializeThreads(handlerToObj(handler, 
                                              Forum::Commands::GET_DISCUSSION_THREADS_BY_NAME, 
                                              SortOrder::Descending).get_child("threads"));

    BOOST_REQUIRE_EQUAL(names.size(), threads.size());
    BOOST_REQUIRE_EQUAL("Ghi", threads[0].name);
    BOOST_REQUIRE_EQUAL("Def", threads[1].name);
    BOOST_REQUIRE_EQUAL("Abc", threads[2].name);
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

    auto threads = deserializeThreads(handlerToObj(handler, 
                                                   Forum::Commands::GET_DISCUSSION_THREADS_BY_CREATED, 
                                                   SortOrder::Ascending).get_child("threads"));

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

    auto threads = deserializeThreads(handlerToObj(handler, 
                                                   Forum::Commands::GET_DISCUSSION_THREADS_BY_CREATED, 
                                                   SortOrder::Descending).get_child("threads"));

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
                                                   Forum::Commands::GET_DISCUSSION_THREADS_BY_LAST_UPDATED,
                                                   SortOrder::Ascending).get_child("threads"));

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
                                              Forum::Commands::GET_DISCUSSION_THREADS_BY_LAST_UPDATED,
                                              SortOrder::Descending).get_child("threads"));

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

    auto result = handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_CREATED, SortOrder::Ascending);
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

    auto result = handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_CREATED, SortOrder::Ascending);
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

    auto result = handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_CREATED, SortOrder::Ascending);
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

    auto result = handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_CREATED, SortOrder::Ascending);
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
        handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_CREATED, SortOrder::Ascending);
        handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_CREATED, SortOrder::Descending);
        handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_LAST_UPDATED, SortOrder::Ascending);
        handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_LAST_UPDATED, SortOrder::Descending);
        handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { thread1Id });
        handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { thread2Id });
    }

    result = handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_CREATED, SortOrder::Ascending);
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
    auto handler = createCommandHandler();

    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { "Foo" });

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

    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { threadId, " \t\r\n " });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_message_with_leading_whitespace_in_the_name_fails )
{
    auto handler = createCommandHandler();
    auto threadId = createDiscussionThreadAndGetId(handler, "Abc");

    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { threadId, "  Foo" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_message_with_trailing_whitespace_in_the_name_fails )
{
    auto handler = createCommandHandler();
    auto threadId = createDiscussionThreadAndGetId(handler, "Abc");

    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { threadId, "Foo\t\t" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_message_with_a_too_short_name_fails )
{
    auto config = getGlobalConfig();
    std::string content(config->discussionThreadMessage.minContentLength - 1, 'a');

    auto handler = createCommandHandler();
    auto threadId = createDiscussionThreadAndGetId(handler, "Abc");

    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { threadId, content });
    assertStatusCodeEqual(StatusCode::VALUE_TOO_SHORT, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_message_with_a_longer_name_fails )
{
    auto config = getGlobalConfig();
    std::string content(config->discussionThreadMessage.maxContentLength + 1, 'a');

    auto handler = createCommandHandler();
    auto threadId = createDiscussionThreadAndGetId(handler, "Abc");

    auto returnObject = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { threadId, content });
    assertStatusCodeEqual(StatusCode::VALUE_TOO_LONG, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_message_with_unicode_name_of_valid_length_succeeds )
{
    ConfigChanger _([](auto& config)
                    {
                        config.discussionThreadMessage.minContentLength = 3;
                        config.discussionThreadMessage.maxContentLength = 3;
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

    auto result = handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_CREATED, SortOrder::Ascending);
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

BOOST_AUTO_TEST_CASE( Changing_a_discussion_thread_message_content_succeeds_only_if_creation_criteria_are_met )
{
    auto handler = createCommandHandler();
    auto threadId = createDiscussionThreadAndGetId(handler, "Thread");
    auto messageId = createDiscussionMessageAndGetId(handler, threadId, "Message");

    auto config = getGlobalConfig();

    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, 
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, 
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT, 
                                       { messageId }));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, 
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT, 
                                       { messageId, "" }));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, 
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT, 
                                       { messageId, " \t\r\n " }));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, 
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT, 
                                       { messageId, " Message" }));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, 
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT, 
                                       { messageId, "Message\t" }));
    assertStatusCodeEqual(StatusCode::VALUE_TOO_SHORT, 
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT, 
                                       { messageId, std::string(config->discussionThreadMessage.minContentLength - 1, 'a') }));
    assertStatusCodeEqual(StatusCode::VALUE_TOO_LONG, 
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT, 
                                       { messageId, std::string(config->discussionThreadMessage.maxContentLength + 1, 'a') }));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, 
                          handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT, 
                                       { messageId, "\xFF\xFF\xFF\xFF\xFF" }));
}

BOOST_AUTO_TEST_CASE( Changing_a_discussion_thread_message_content_succeeds )
{
    auto handler = createCommandHandler();

    auto userId = createUserAndGetId(handler, "User");
    auto threadId = createDiscussionThreadAndGetId(handler, "Abc");
    std::string message1Id, message2Id;
    {
        LoggedInUserChanger _(userId);
        TimestampChanger __(1000);
        message1Id = createDiscussionMessageAndGetId(handler, threadId, "Message1");
    }
    {
        LoggedInUserChanger _(userId);
        TimestampChanger __(2000);
        message2Id = createDiscussionMessageAndGetId(handler, threadId, "Message2");
    }
    {
        LoggedInUserChanger _(userId);
        TimestampChanger __(3000);
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, 
                                                           Forum::Commands::CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT,
                                                           { message1Id, "Message1 - Updated" }));
    }

    auto thread = deserializeThread(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { threadId })
                                    .get_child("thread"));

    BOOST_REQUIRE_EQUAL(threadId, thread.id);
    BOOST_REQUIRE_EQUAL("Abc", thread.name);
    BOOST_REQUIRE_EQUAL(2, thread.messages.size());

    BOOST_REQUIRE_EQUAL(message1Id, thread.messages[0].id);
    BOOST_REQUIRE_EQUAL("Message1 - Updated", thread.messages[0].content);
    BOOST_REQUIRE_EQUAL(1000, thread.messages[0].created);
    BOOST_REQUIRE(thread.messages[0].lastUpdated);
    BOOST_REQUIRE_EQUAL(3000, thread.messages[0].lastUpdated->at);
    BOOST_REQUIRE_EQUAL(userId, thread.messages[0].createdBy.id);
    BOOST_REQUIRE_EQUAL("User", thread.messages[0].createdBy.name);

    BOOST_REQUIRE_EQUAL(message2Id, thread.messages[1].id);
    BOOST_REQUIRE_EQUAL("Message2", thread.messages[1].content);
    BOOST_REQUIRE_EQUAL(2000, thread.messages[1].created);
    BOOST_REQUIRE( ! thread.messages[1].lastUpdated);
    BOOST_REQUIRE_EQUAL(userId, thread.messages[1].createdBy.id);
    BOOST_REQUIRE_EQUAL("User", thread.messages[1].createdBy.name);
}

BOOST_AUTO_TEST_CASE( Changing_a_discussion_thread_message_content_stores_the_user_only_if_it_differs_from_the_original_author )
{
    auto handler = createCommandHandler();

    auto user1Id = createUserAndGetId(handler, "User1");
    auto user2Id = createUserAndGetId(handler, "User2");
    auto threadId = createDiscussionThreadAndGetId(handler, "Abc");
    std::string message1Id, message2Id;
    {
        LoggedInUserChanger _(user1Id);
        TimestampChanger __(1000);
        message1Id = createDiscussionMessageAndGetId(handler, threadId, "Message1");
    }
    {
        LoggedInUserChanger _(user1Id);
        TimestampChanger __(2000);
        message2Id = createDiscussionMessageAndGetId(handler, threadId, "Message2");
    }
    {
        LoggedInUserChanger _(user1Id);
        TimestampChanger __(3000);
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, 
                                                           Forum::Commands::CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT,
                                                           { message1Id, "Message1 - Updated" }));
    }
    {
        LoggedInUserChanger _(user2Id);
        TimestampChanger __(4000);
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, 
                                                           Forum::Commands::CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT,
                                                           { message2Id, "Message2 - Updated" }));
    }

    auto thread = deserializeThread(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { threadId })
                                    .get_child("thread"));

    BOOST_REQUIRE_EQUAL(threadId, thread.id);
    BOOST_REQUIRE_EQUAL("Abc", thread.name);
    BOOST_REQUIRE_EQUAL(2, thread.messages.size());

    BOOST_REQUIRE_EQUAL(message1Id, thread.messages[0].id);
    BOOST_REQUIRE_EQUAL("Message1 - Updated", thread.messages[0].content);
    BOOST_REQUIRE_EQUAL(1000, thread.messages[0].created);
    BOOST_REQUIRE(thread.messages[0].lastUpdated);
    BOOST_REQUIRE_EQUAL(3000, thread.messages[0].lastUpdated->at);
    BOOST_REQUIRE_EQUAL("", thread.messages[0].lastUpdated->userId);
    BOOST_REQUIRE_EQUAL("", thread.messages[0].lastUpdated->userName);
    BOOST_REQUIRE_EQUAL(user1Id, thread.messages[0].createdBy.id);
    BOOST_REQUIRE_EQUAL("User1", thread.messages[0].createdBy.name);

    BOOST_REQUIRE_EQUAL(message2Id, thread.messages[1].id);
    BOOST_REQUIRE_EQUAL("Message2 - Updated", thread.messages[1].content);
    BOOST_REQUIRE_EQUAL(2000, thread.messages[1].created);
    BOOST_REQUIRE(thread.messages[1].lastUpdated);
    BOOST_REQUIRE_EQUAL(4000, thread.messages[1].lastUpdated->at);
    BOOST_REQUIRE_EQUAL(user2Id, thread.messages[1].lastUpdated->userId);
    BOOST_REQUIRE_EQUAL("User2", thread.messages[1].lastUpdated->userName);
    BOOST_REQUIRE_EQUAL(user1Id, thread.messages[1].createdBy.id);
    BOOST_REQUIRE_EQUAL("User1", thread.messages[1].createdBy.name);
}

BOOST_AUTO_TEST_CASE( Discussion_thread_message_store_the_ip_address_and_user_agent_of_author_and_editor )
{
    auto handler = createCommandHandler();

    auto user1Id = createUserAndGetId(handler, "User1");
    auto user2Id = createUserAndGetId(handler, "User2");
    auto threadId = createDiscussionThreadAndGetId(handler, "Abc");
    std::string message1Id, message2Id;
    {
        LoggedInUserChanger _(user1Id);
        TimestampChanger __(1000);
        IpUserAgentChanger ___("1.2.3.4", "Browser 1");
        message1Id = createDiscussionMessageAndGetId(handler, threadId, "Message1");
    }
    {
        LoggedInUserChanger _(user1Id);
        TimestampChanger __(2000);
        IpUserAgentChanger ___("1.2.3.4", "Browser 1");
        message2Id = createDiscussionMessageAndGetId(handler, threadId, "Message2");
    }
    {
        LoggedInUserChanger _(user1Id);
        TimestampChanger __(3000);
        IpUserAgentChanger ___("1.2.3.4", "Browser 2");
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler,
                                                           Forum::Commands::CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT,
                                                           { message1Id, "Message1 - Updated" }));
    }
    {
        LoggedInUserChanger _(user2Id);
        TimestampChanger __(4000);
        IpUserAgentChanger ___("2.3.4.5", "Browser 3");
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler,
                                                           Forum::Commands::CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT,
                                                           { message2Id, "Message2 - Updated" }));
    }

    auto thread = deserializeThread(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { threadId })
                                    .get_child("thread"));

    BOOST_REQUIRE_EQUAL(threadId, thread.id);
    BOOST_REQUIRE_EQUAL("Abc", thread.name);
    BOOST_REQUIRE_EQUAL(2, thread.messages.size());

    BOOST_REQUIRE_EQUAL(message1Id, thread.messages[0].id);
    BOOST_REQUIRE_EQUAL("Message1 - Updated", thread.messages[0].content);
    BOOST_REQUIRE_EQUAL(1000, thread.messages[0].created);
    BOOST_REQUIRE_EQUAL("1.2.3.4", thread.messages[0].ip);
    BOOST_REQUIRE_EQUAL("Browser 1", thread.messages[0].userAgent);
    BOOST_REQUIRE(thread.messages[0].lastUpdated);
    BOOST_REQUIRE_EQUAL(3000, thread.messages[0].lastUpdated->at);
    BOOST_REQUIRE_EQUAL("", thread.messages[0].lastUpdated->userId);
    BOOST_REQUIRE_EQUAL("", thread.messages[0].lastUpdated->userName);
    BOOST_REQUIRE_EQUAL("1.2.3.4", thread.messages[0].lastUpdated->ip);
    BOOST_REQUIRE_EQUAL("Browser 2", thread.messages[0].lastUpdated->userAgent);
    BOOST_REQUIRE_EQUAL(user1Id, thread.messages[0].createdBy.id);
    BOOST_REQUIRE_EQUAL("User1", thread.messages[0].createdBy.name);

    BOOST_REQUIRE_EQUAL(message2Id, thread.messages[1].id);
    BOOST_REQUIRE_EQUAL("Message2 - Updated", thread.messages[1].content);
    BOOST_REQUIRE_EQUAL(2000, thread.messages[1].created);
    BOOST_REQUIRE_EQUAL("1.2.3.4", thread.messages[1].ip);
    BOOST_REQUIRE_EQUAL("Browser 1", thread.messages[1].userAgent);
    BOOST_REQUIRE(thread.messages[1].lastUpdated);
    BOOST_REQUIRE_EQUAL(4000, thread.messages[1].lastUpdated->at);
    BOOST_REQUIRE_EQUAL(user2Id, thread.messages[1].lastUpdated->userId);
    BOOST_REQUIRE_EQUAL("User2", thread.messages[1].lastUpdated->userName);
    BOOST_REQUIRE_EQUAL("2.3.4.5", thread.messages[1].lastUpdated->ip);
    BOOST_REQUIRE_EQUAL("Browser 3", thread.messages[1].lastUpdated->userAgent);
    BOOST_REQUIRE_EQUAL(user1Id, thread.messages[1].createdBy.id);
    BOOST_REQUIRE_EQUAL("User1", thread.messages[1].createdBy.name);
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

    BOOST_REQUIRE_EQUAL(2, handlerToObj(handler, Forum::Commands::COUNT_ENTITIES).get<int>("count.users"));
    BOOST_REQUIRE_EQUAL(1, handlerToObj(handler, Forum::Commands::COUNT_ENTITIES).get<int>("count.discussionThreads"));
    BOOST_REQUIRE_EQUAL(3, handlerToObj(handler, Forum::Commands::COUNT_ENTITIES).get<int>("count.discussionMessages"));

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::DELETE_USER, { user2 }));

    BOOST_REQUIRE_EQUAL(1, handlerToObj(handler, Forum::Commands::COUNT_ENTITIES).get<int>("count.users"));
    BOOST_REQUIRE_EQUAL(1, handlerToObj(handler, Forum::Commands::COUNT_ENTITIES).get<int>("count.discussionThreads"));
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

BOOST_AUTO_TEST_CASE( Discussion_threads_include_info_about_latest_message )
{
    auto handler = createCommandHandler();

    std::string user1, user2;
    {
        TimestampChanger _(500);
        user1 = createUserAndGetId(handler, "User1");
        user2 = createUserAndGetId(handler, "User2");
    }
    std::string thread1Id, thread2Id;
    {
        TimestampChanger _(1000);
        LoggedInUserChanger changer(user1);
        thread1Id = createDiscussionThreadAndGetId(handler, "Abc");
        thread2Id = createDiscussionThreadAndGetId(handler, "Def");
    }
    {
        LoggedInUserChanger changer(user1);
        {
            TimestampChanger _(1000);
            handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { thread1Id, "aaaaaaaaaaa" });
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
            handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { thread2Id, "bbbbbbbbbbb" });
        }
    }

    auto threads = deserializeThreads(handlerToObj(handler, 
                                                   Forum::Commands::GET_DISCUSSION_THREADS_BY_CREATED, 
                                                   SortOrder::Ascending).get_child("threads"));

    BOOST_REQUIRE_EQUAL(2, threads.size());
    BOOST_REQUIRE_EQUAL("Abc", threads[0].name);
    BOOST_REQUIRE_EQUAL(1000, threads[0].created);
    BOOST_REQUIRE_EQUAL(3000, threads[0].latestMessage.created);
    BOOST_REQUIRE_EQUAL(user1, threads[0].latestMessage.createdBy.id);
    BOOST_REQUIRE_EQUAL("User1", threads[0].latestMessage.createdBy.name);
    BOOST_REQUIRE_EQUAL(500, threads[0].latestMessage.createdBy.created);
    BOOST_REQUIRE_EQUAL(3000, threads[0].latestMessage.createdBy.lastSeen);
    BOOST_REQUIRE_EQUAL(2, threads[0].latestMessage.createdBy.threadCount);
    BOOST_REQUIRE_EQUAL(2, threads[0].latestMessage.createdBy.messageCount);

    BOOST_REQUIRE_EQUAL("Def", threads[1].name);
    BOOST_REQUIRE_EQUAL(1000, threads[1].created);
    BOOST_REQUIRE_EQUAL(2000, threads[1].latestMessage.created);
    BOOST_REQUIRE_EQUAL(user2, threads[1].latestMessage.createdBy.id);
    BOOST_REQUIRE_EQUAL("User2", threads[1].latestMessage.createdBy.name);
    BOOST_REQUIRE_EQUAL(500, threads[1].latestMessage.createdBy.created);
    BOOST_REQUIRE_EQUAL(2000, threads[1].latestMessage.createdBy.lastSeen);
    BOOST_REQUIRE_EQUAL(0, threads[1].latestMessage.createdBy.threadCount);
    BOOST_REQUIRE_EQUAL(1, threads[1].latestMessage.createdBy.messageCount);

    auto thread = deserializeThread(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { thread1Id })
                                      .get_child("thread"));

    BOOST_REQUIRE_EQUAL("Abc", thread.name);
    BOOST_REQUIRE_EQUAL(1000, thread.created);
    BOOST_REQUIRE_EQUAL(3000, thread.latestMessage.created);
    BOOST_REQUIRE_EQUAL(user1, thread.latestMessage.createdBy.id);
    BOOST_REQUIRE_EQUAL("User1", thread.latestMessage.createdBy.name);
    BOOST_REQUIRE_EQUAL(500, thread.latestMessage.createdBy.created);
    BOOST_REQUIRE_EQUAL(3000, thread.latestMessage.createdBy.lastSeen);
    BOOST_REQUIRE_EQUAL(2, thread.latestMessage.createdBy.threadCount);
    BOOST_REQUIRE_EQUAL(2, thread.latestMessage.createdBy.messageCount);

    thread = deserializeThread(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { thread2Id })
                                      .get_child("thread"));

    BOOST_REQUIRE_EQUAL("Def", thread.name);
    BOOST_REQUIRE_EQUAL(1000, thread.created);
    BOOST_REQUIRE_EQUAL(2000, thread.latestMessage.created);
    BOOST_REQUIRE_EQUAL(user2, thread.latestMessage.createdBy.id);
    BOOST_REQUIRE_EQUAL("User2", thread.latestMessage.createdBy.name);
    BOOST_REQUIRE_EQUAL(500, thread.latestMessage.createdBy.created);
    BOOST_REQUIRE_EQUAL(2000, thread.latestMessage.createdBy.lastSeen);
    BOOST_REQUIRE_EQUAL(0, thread.latestMessage.createdBy.threadCount);
    BOOST_REQUIRE_EQUAL(1, thread.latestMessage.createdBy.messageCount);
}

BOOST_AUTO_TEST_CASE( Latest_discussion_message_of_thread_does_not_include_message_content )
{
    auto handler = createCommandHandler();

    auto userId = createUserAndGetId(handler, "User");
    auto threadId = createDiscussionThreadAndGetId(handler, "Thread");
    auto messageId = createDiscussionMessageAndGetId(handler, threadId, "Message");

    auto result = handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_CREATED, SortOrder::Ascending);
    auto resultThreads = result.get_child("threads");

    for (auto& pair : resultThreads)
    {
        for (auto& threadProperty : pair.second)
        {
            if (threadProperty.first == "latestMessage")
            {
                BOOST_REQUIRE( ! treeContains(threadProperty.second, "content"));
            }
        }
    }   
}


BOOST_AUTO_TEST_CASE( Discussion_threads_include_total_message_count )
{
    auto handler = createCommandHandler();

    std::string user1, user2;
    {
        TimestampChanger _(500);
        user1 = createUserAndGetId(handler, "User1");
        user2 = createUserAndGetId(handler, "User2");
    }
    std::string thread1Id, thread2Id;
    {
        TimestampChanger _(1000);
        LoggedInUserChanger changer(user1);
        thread1Id = createDiscussionThreadAndGetId(handler, "Abc");
        thread2Id = createDiscussionThreadAndGetId(handler, "Def");
    }
    {
        LoggedInUserChanger changer(user1);
        {
            TimestampChanger _(1000);
            handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { thread1Id, "aaaaaaaaaaa" });
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
            handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { thread2Id, "bbbbbbbbbbb" });
        }
    }

    auto threads = deserializeThreads(handlerToObj(handler, 
                                                   Forum::Commands::GET_DISCUSSION_THREADS_BY_CREATED, 
                                                   SortOrder::Ascending).get_child("threads"));

    BOOST_REQUIRE_EQUAL(2, threads.size());
    BOOST_REQUIRE_EQUAL("Abc", threads[0].name);
    BOOST_REQUIRE_EQUAL(2, threads[0].messageCount);

    BOOST_REQUIRE_EQUAL("Def", threads[1].name);
    BOOST_REQUIRE_EQUAL(1, threads[1].messageCount);

    auto thread = deserializeThread(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { thread1Id })
                                      .get_child("thread"));

    BOOST_REQUIRE_EQUAL("Abc", thread.name);
    BOOST_REQUIRE_EQUAL(2, thread.messageCount);

    thread = deserializeThread(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { thread2Id })
                                      .get_child("thread"));

    BOOST_REQUIRE_EQUAL("Def", thread.name);
    BOOST_REQUIRE_EQUAL(1, thread.messageCount);
}

BOOST_AUTO_TEST_CASE( Discussion_threads_can_be_retrieved_sorted_by_message_count_ascending_and_descending )
{
    auto handler = createCommandHandler();

    std::string user1, user2;
    {
        TimestampChanger _(500);
        user1 = createUserAndGetId(handler, "User1");
        user2 = createUserAndGetId(handler, "User2");
    }
    std::string thread1Id, thread2Id, thread3Id;
    {
        TimestampChanger _(1000);
        LoggedInUserChanger changer(user1);
        thread1Id = createDiscussionThreadAndGetId(handler, "Abc");
        thread2Id = createDiscussionThreadAndGetId(handler, "Def");
        thread3Id = createDiscussionThreadAndGetId(handler, "Ghi");
    }
    {
        LoggedInUserChanger changer(user1);
        {
            TimestampChanger _(1000);
            handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { thread1Id, "aaaaaaaaaaa" });
        }
        {
            TimestampChanger _(3000);
            handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { thread1Id, "bbbbbbbbbbb" });
            handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { thread1Id, "ccccccccccc" });
            handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD_MESSAGE, { thread3Id, "ccccccccccc" });
        }
    }
    std::vector<std::string> messagesToDelete;
    {
        LoggedInUserChanger changer(user2);
        {
            TimestampChanger _(2000);
            messagesToDelete.push_back(createDiscussionMessageAndGetId(handler, thread2Id, "ddddddddddd"));
            messagesToDelete.push_back(createDiscussionMessageAndGetId(handler, thread2Id, "eeeeeeeeeee"));
        }
    }

    auto threads = deserializeThreads(handlerToObj(handler, 
                                                   Forum::Commands::GET_DISCUSSION_THREADS_BY_MESSAGE_COUNT, 
                                                   SortOrder::Ascending).get_child("threads"));

    BOOST_REQUIRE_EQUAL(3, threads.size());
    BOOST_REQUIRE_EQUAL("Ghi", threads[0].name);
    BOOST_REQUIRE_EQUAL(1, threads[0].messageCount);
    BOOST_REQUIRE_EQUAL("Def", threads[1].name);
    BOOST_REQUIRE_EQUAL(2, threads[1].messageCount);
    BOOST_REQUIRE_EQUAL("Abc", threads[2].name);
    BOOST_REQUIRE_EQUAL(3, threads[2].messageCount);

    threads = deserializeThreads(handlerToObj(handler, 
                                              Forum::Commands::GET_DISCUSSION_THREADS_BY_MESSAGE_COUNT, 
                                              SortOrder::Descending).get_child("threads"));

    BOOST_REQUIRE_EQUAL(3, threads.size());
    BOOST_REQUIRE_EQUAL("Abc", threads[0].name);
    BOOST_REQUIRE_EQUAL(3, threads[0].messageCount);
    BOOST_REQUIRE_EQUAL("Def", threads[1].name);
    BOOST_REQUIRE_EQUAL(2, threads[1].messageCount);
    BOOST_REQUIRE_EQUAL("Ghi", threads[2].name);
    BOOST_REQUIRE_EQUAL(1, threads[2].messageCount);

    for (auto& messageId : messagesToDelete)
    {
        assertStatusCodeEqual(StatusCode::OK,
                              handlerToObj(handler, Forum::Commands::DELETE_DISCUSSION_THREAD_MESSAGE, {messageId}));
    }

    threads = deserializeThreads(handlerToObj(handler, 
                                              Forum::Commands::GET_DISCUSSION_THREADS_BY_MESSAGE_COUNT, 
                                              SortOrder::Ascending).get_child("threads"));

    BOOST_REQUIRE_EQUAL(3, threads.size());
    BOOST_REQUIRE_EQUAL("Def", threads[0].name);
    BOOST_REQUIRE_EQUAL(0, threads[0].messageCount);
    BOOST_REQUIRE_EQUAL("Ghi", threads[1].name);
    BOOST_REQUIRE_EQUAL(1, threads[1].messageCount);
    BOOST_REQUIRE_EQUAL("Abc", threads[2].name);
    BOOST_REQUIRE_EQUAL(3, threads[2].messageCount);

    threads = deserializeThreads(handlerToObj(handler, 
                                              Forum::Commands::GET_DISCUSSION_THREADS_BY_MESSAGE_COUNT, 
                                              SortOrder::Descending).get_child("threads"));

    BOOST_REQUIRE_EQUAL(3, threads.size());
    BOOST_REQUIRE_EQUAL("Abc", threads[0].name);
    BOOST_REQUIRE_EQUAL(3, threads[0].messageCount);
    BOOST_REQUIRE_EQUAL("Ghi", threads[1].name);
    BOOST_REQUIRE_EQUAL(1, threads[1].messageCount);
    BOOST_REQUIRE_EQUAL("Def", threads[2].name);
    BOOST_REQUIRE_EQUAL(0, threads[2].messageCount);
}

BOOST_AUTO_TEST_CASE( Merging_discussion_threads_requires_two_valid_thread_ids )
{
    auto handler = createCommandHandler();

    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS,
                          handlerToObj(handler, Forum::Commands::MERGE_DISCUSSION_THREADS));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS,
                          handlerToObj(handler, Forum::Commands::MERGE_DISCUSSION_THREADS, { sampleValidIdString }));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS,
                          handlerToObj(handler, Forum::Commands::MERGE_DISCUSSION_THREADS, 
                                       { "bogus id1", "bogus id2" }));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS,
                          handlerToObj(handler, Forum::Commands::MERGE_DISCUSSION_THREADS, 
                                       { sampleValidIdString, "bogus id2" }));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS,
                          handlerToObj(handler, Forum::Commands::MERGE_DISCUSSION_THREADS, 
                                       { "bogus id1", sampleValidIdString }));
    assertStatusCodeEqual(StatusCode::NOT_FOUND,
                          handlerToObj(handler, Forum::Commands::MERGE_DISCUSSION_THREADS, 
                                       { sampleValidIdString, sampleValidIdString2 }));
}

BOOST_AUTO_TEST_CASE( Merging_discussion_threads_fails_if_the_same_id_is_provided_twice )
{
    auto handler = createCommandHandler();

    auto threadId = createDiscussionThreadAndGetId(handler, "Thread");

    assertStatusCodeEqual(StatusCode::NO_EFFECT,
                          handlerToObj(handler, Forum::Commands::MERGE_DISCUSSION_THREADS,
                                       { threadId, threadId }));
}


BOOST_AUTO_TEST_CASE( Merging_discussion_threads_works_ok )
{
    auto handler = createCommandHandler();

    auto user1Id = createUserAndGetId(handler, "User1");
    auto user2Id = createUserAndGetId(handler, "User2");
    auto thread1Id = createDiscussionThreadAndGetId(handler, "Thread1");
    auto thread2Id = createDiscussionThreadAndGetId(handler, "Thread2");
    std::string message1Id, message2Id, message3Id;

    {
        LoggedInUserChanger _(user1Id);
        TimestampChanger __(1000);
        message1Id = createDiscussionMessageAndGetId(handler, thread1Id, "Message 1");
    }
    {
        LoggedInUserChanger _(user2Id);
        TimestampChanger __(2000);
        message2Id = createDiscussionMessageAndGetId(handler, thread2Id, "Message 2");
    }
    {
        TimestampChanger _(3000);
        message3Id = createDiscussionMessageAndGetId(handler, thread1Id, "Message 3");
    }

    assertStatusCodeEqual(StatusCode::OK,
                          handlerToObj(handler, Forum::Commands::MERGE_DISCUSSION_THREADS,
                                       { thread1Id, thread2Id }));

    assertStatusCodeEqual(StatusCode::NOT_FOUND,
                          handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID,
                                       { thread1Id }));

    auto thread = deserializeThread(handlerToObj(handler, 
                                                 Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, 
                                                 { thread2Id }).get_child("thread"));

    BOOST_REQUIRE_EQUAL(thread2Id, thread.id);
    BOOST_REQUIRE_EQUAL("Thread2", thread.name);

    BOOST_REQUIRE_EQUAL(3, thread.messages.size());

    BOOST_REQUIRE_EQUAL(message1Id, thread.messages[0].id);
    BOOST_REQUIRE_EQUAL("Message 1", thread.messages[0].content);
    BOOST_REQUIRE_EQUAL(user1Id, thread.messages[0].createdBy.id);
    BOOST_REQUIRE_EQUAL("User1", thread.messages[0].createdBy.name);
    BOOST_REQUIRE_EQUAL(1000, thread.messages[0].created);

    BOOST_REQUIRE_EQUAL(message2Id, thread.messages[1].id);
    BOOST_REQUIRE_EQUAL("Message 2", thread.messages[1].content);
    BOOST_REQUIRE_EQUAL(user2Id, thread.messages[1].createdBy.id);
    BOOST_REQUIRE_EQUAL("User2", thread.messages[1].createdBy.name);
    BOOST_REQUIRE_EQUAL(2000, thread.messages[1].created);

    BOOST_REQUIRE_EQUAL(message3Id, thread.messages[2].id);
    BOOST_REQUIRE_EQUAL("Message 3", thread.messages[2].content);
    BOOST_REQUIRE_EQUAL(3000, thread.messages[2].created);
}

BOOST_AUTO_TEST_CASE( Moving_discussion_threads_messages_requires_a_valid_message_id_and_a_valid_thread_id )
{
    auto handler = createCommandHandler();

    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS,
                          handlerToObj(handler, Forum::Commands::MOVE_DISCUSSION_THREAD_MESSAGE));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS,
                          handlerToObj(handler, Forum::Commands::MOVE_DISCUSSION_THREAD_MESSAGE, 
                                       { sampleValidIdString }));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS,
                          handlerToObj(handler, Forum::Commands::MOVE_DISCUSSION_THREAD_MESSAGE,
                                       { "bogus id1", "bogus id2" }));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS,
                          handlerToObj(handler, Forum::Commands::MOVE_DISCUSSION_THREAD_MESSAGE,
                                       { sampleValidIdString, "bogus id2" }));
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS,
                          handlerToObj(handler, Forum::Commands::MOVE_DISCUSSION_THREAD_MESSAGE,
                                       { "bogus id1", sampleValidIdString }));
    assertStatusCodeEqual(StatusCode::NOT_FOUND,
                          handlerToObj(handler, Forum::Commands::MOVE_DISCUSSION_THREAD_MESSAGE,
                                       { sampleValidIdString, sampleValidIdString }));
}

BOOST_AUTO_TEST_CASE( Moving_discussion_thread_messages_fails_if_the_message_is_to_be_moved_to_the_thread_it_already_belongs_to )
{
    auto handler = createCommandHandler();

    auto threadId = createDiscussionThreadAndGetId(handler, "Thread");
    auto messageId = createDiscussionMessageAndGetId(handler, threadId, "Thread");

    assertStatusCodeEqual(StatusCode::NO_EFFECT,
                          handlerToObj(handler, Forum::Commands::MOVE_DISCUSSION_THREAD_MESSAGE,
                                       { messageId, threadId }));
}

BOOST_AUTO_TEST_CASE( Moving_discussion_thread_messages_works_ok )
{
    auto handler = createCommandHandler();

    auto thread1Id = createDiscussionThreadAndGetId(handler, "Thread1");
    auto thread2Id = createDiscussionThreadAndGetId(handler, "Thread2");
    std::string message1Id, message2Id, message3Id;

    {
        TimestampChanger _(1000);
        message1Id = createDiscussionMessageAndGetId(handler, thread1Id, "Message 1");
    }
    {
        TimestampChanger _(2000);
        message2Id = createDiscussionMessageAndGetId(handler, thread2Id, "Message 2");
    }
    {
        TimestampChanger _(3000);
        message3Id = createDiscussionMessageAndGetId(handler, thread1Id, "Message 3");
    }

    assertStatusCodeEqual(StatusCode::OK,
                          handlerToObj(handler, Forum::Commands::MOVE_DISCUSSION_THREAD_MESSAGE,
                                       { message1Id, thread2Id }));

    auto thread1 = deserializeThread(handlerToObj(handler, 
                                                 Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, 
                                                 { thread1Id }).get_child("thread"));

    BOOST_REQUIRE_EQUAL(thread1Id, thread1.id);
    BOOST_REQUIRE_EQUAL("Thread1", thread1.name);

    BOOST_REQUIRE_EQUAL(1, thread1.messages.size());
    
    BOOST_REQUIRE_EQUAL(message3Id, thread1.messages[0].id);
    BOOST_REQUIRE_EQUAL("Message 3", thread1.messages[0].content);
    BOOST_REQUIRE_EQUAL(3000, thread1.messages[0].created);

    auto thread2 = deserializeThread(handlerToObj(handler, 
                                                 Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, 
                                                 { thread2Id }).get_child("thread"));

    BOOST_REQUIRE_EQUAL(thread2Id, thread2.id);
    BOOST_REQUIRE_EQUAL("Thread2", thread2.name);

    BOOST_REQUIRE_EQUAL(2, thread2.messages.size());

    BOOST_REQUIRE_EQUAL(message1Id, thread2.messages[0].id);
    BOOST_REQUIRE_EQUAL("Message 1", thread2.messages[0].content);
    BOOST_REQUIRE_EQUAL(1000, thread2.messages[0].created);

    BOOST_REQUIRE_EQUAL(message2Id, thread2.messages[1].id);
    BOOST_REQUIRE_EQUAL("Message 2", thread2.messages[1].content);
    BOOST_REQUIRE_EQUAL(2000, thread2.messages[1].created);
}

BOOST_AUTO_TEST_CASE( Retrieved_discussion_threads_have_visitedSinceLastChange_false_initially )
{
    auto handler = createCommandHandler();

    auto userId = createUserAndGetId(handler, "User");
    auto threadId = createDiscussionThreadAndGetId(handler, "Thread");
    createDiscussionMessageAndGetId(handler, threadId, "Message");

    {
        LoggedInUserChanger _(userId);
        auto threads = deserializeThreads(handlerToObj(handler, 
                                                       Forum::Commands::GET_DISCUSSION_THREADS_BY_NAME, 
                                                       SortOrder::Ascending).get_child("threads"));

        BOOST_REQUIRE_EQUAL(1, threads.size());
        BOOST_REQUIRE_EQUAL(threadId, threads[0].id);
        BOOST_REQUIRE_EQUAL(false, threads[0].visitedSinceLastChange);
    }
}

BOOST_AUTO_TEST_CASE( Discussion_threads_visitedSinceLastChange_is_true_after_requesting_a_thread )
{
    auto handler = createCommandHandler();

    auto userId = createUserAndGetId(handler, "User");

    auto thread1Id = createDiscussionThreadAndGetId(handler, "Thread1");
    createDiscussionMessageAndGetId(handler, thread1Id, "Message1");
    auto thread2Id = createDiscussionThreadAndGetId(handler, "Thread2");
    createDiscussionMessageAndGetId(handler, thread2Id, "Message2");

    {
        LoggedInUserChanger _(userId);
        handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { thread1Id });

        auto threads = deserializeThreads(handlerToObj(handler, 
                                                       Forum::Commands::GET_DISCUSSION_THREADS_BY_NAME, 
                                                       SortOrder::Ascending).get_child("threads"));

        BOOST_REQUIRE_EQUAL(2, threads.size());
        BOOST_REQUIRE_EQUAL(thread1Id, threads[0].id);
        BOOST_REQUIRE_EQUAL(true, threads[0].visitedSinceLastChange);
        BOOST_REQUIRE_EQUAL(thread2Id, threads[1].id);
        BOOST_REQUIRE_EQUAL(false, threads[1].visitedSinceLastChange);
    }
}

BOOST_AUTO_TEST_CASE( Discussion_threads_visitedSinceLastChange_depends_on_user )
{
    auto handler = createCommandHandler();

    auto user1Id = createUserAndGetId(handler, "User1");
    auto user2Id = createUserAndGetId(handler, "User2");

    auto thread1Id = createDiscussionThreadAndGetId(handler, "Thread1");
    createDiscussionMessageAndGetId(handler, thread1Id, "Message1");
    auto thread2Id = createDiscussionThreadAndGetId(handler, "Thread2");
    createDiscussionMessageAndGetId(handler, thread2Id, "Message2");

    {
        LoggedInUserChanger _(user1Id);
        handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { thread1Id });

        auto threads = deserializeThreads(handlerToObj(handler, 
                                                       Forum::Commands::GET_DISCUSSION_THREADS_BY_NAME, 
                                                       SortOrder::Ascending).get_child("threads"));

        BOOST_REQUIRE_EQUAL(2, threads.size());
        BOOST_REQUIRE_EQUAL(thread1Id, threads[0].id);
        BOOST_REQUIRE_EQUAL(true, threads[0].visitedSinceLastChange);
        BOOST_REQUIRE_EQUAL(thread2Id, threads[1].id);
        BOOST_REQUIRE_EQUAL(false, threads[1].visitedSinceLastChange);
    }
    {
        LoggedInUserChanger _(user2Id);
        handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { thread2Id });

        auto threads = deserializeThreads(handlerToObj(handler, 
                                                       Forum::Commands::GET_DISCUSSION_THREADS_BY_NAME, 
                                                       SortOrder::Ascending).get_child("threads"));

        BOOST_REQUIRE_EQUAL(2, threads.size());
        BOOST_REQUIRE_EQUAL(thread1Id, threads[0].id);
        BOOST_REQUIRE_EQUAL(false, threads[0].visitedSinceLastChange);
        BOOST_REQUIRE_EQUAL(thread2Id, threads[1].id);
        BOOST_REQUIRE_EQUAL(true, threads[1].visitedSinceLastChange);
    }
}

BOOST_AUTO_TEST_CASE( Discussion_threads_visitedSinceLastChange_is_reset_after_adding_a_new_message )
{
    auto handler = createCommandHandler();

    auto userId = createUserAndGetId(handler, "User");

    auto thread1Id = createDiscussionThreadAndGetId(handler, "Thread1");
    auto thread2Id = createDiscussionThreadAndGetId(handler, "Thread2");
    {
        TimestampChanger _(1000);
        createDiscussionMessageAndGetId(handler, thread1Id, "Message1");
        createDiscussionMessageAndGetId(handler, thread2Id, "Message2");
    }
    {
        LoggedInUserChanger _(userId);
        handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { thread1Id });
        handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { thread2Id });

        auto threads = deserializeThreads(handlerToObj(handler, 
                                                       Forum::Commands::GET_DISCUSSION_THREADS_BY_NAME, 
                                                       SortOrder::Ascending).get_child("threads"));

        BOOST_REQUIRE_EQUAL(2, threads.size());
        BOOST_REQUIRE_EQUAL(thread1Id, threads[0].id);
        BOOST_REQUIRE_EQUAL(true, threads[0].visitedSinceLastChange);
        BOOST_REQUIRE_EQUAL(thread2Id, threads[1].id);
        BOOST_REQUIRE_EQUAL(true, threads[1].visitedSinceLastChange);

        {
            TimestampChanger __(2000);
            createDiscussionMessageAndGetId(handler, thread1Id, "Message3");
        }

        threads = deserializeThreads(handlerToObj(handler, 
                                                  Forum::Commands::GET_DISCUSSION_THREADS_BY_NAME, 
                                                  SortOrder::Ascending).get_child("threads"));

        BOOST_REQUIRE_EQUAL(2, threads.size());
        BOOST_REQUIRE_EQUAL(thread1Id, threads[0].id);
        BOOST_REQUIRE_EQUAL(false, threads[0].visitedSinceLastChange);
        BOOST_REQUIRE_EQUAL(thread2Id, threads[1].id);
        BOOST_REQUIRE_EQUAL(true, threads[1].visitedSinceLastChange);
    }
}

BOOST_AUTO_TEST_CASE( Discussion_threads_visitedSinceLastChange_is_reset_after_editing_a_message_of_the_thread )
{
    auto handler = createCommandHandler();

    auto userId = createUserAndGetId(handler, "User");

    auto thread1Id = createDiscussionThreadAndGetId(handler, "Thread1");
    auto thread2Id = createDiscussionThreadAndGetId(handler, "Thread2");
    std::string message1Id, message2Id;
    {
        TimestampChanger _(1000);
        message1Id = createDiscussionMessageAndGetId(handler, thread1Id, "Message1");
        message2Id = createDiscussionMessageAndGetId(handler, thread2Id, "Message2");
    }
    {
        LoggedInUserChanger _(userId);
        handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { thread1Id });
        handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { thread2Id });

        auto threads = deserializeThreads(handlerToObj(handler, 
                                                       Forum::Commands::GET_DISCUSSION_THREADS_BY_NAME, 
                                                       SortOrder::Ascending).get_child("threads"));

        BOOST_REQUIRE_EQUAL(2, threads.size());
        BOOST_REQUIRE_EQUAL(thread1Id, threads[0].id);
        BOOST_REQUIRE_EQUAL(true, threads[0].visitedSinceLastChange);
        BOOST_REQUIRE_EQUAL(thread2Id, threads[1].id);
        BOOST_REQUIRE_EQUAL(true, threads[1].visitedSinceLastChange);

        {
            TimestampChanger __(2000);
            assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, 
                                                               Forum::Commands::CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT,
                                                               { message1Id, "Message1 - New" }));
        }

        threads = deserializeThreads(handlerToObj(handler, 
                                                  Forum::Commands::GET_DISCUSSION_THREADS_BY_NAME, 
                                                  SortOrder::Ascending).get_child("threads"));

        BOOST_REQUIRE_EQUAL(2, threads.size());
        BOOST_REQUIRE_EQUAL(thread1Id, threads[0].id);
        BOOST_REQUIRE_EQUAL(false, threads[0].visitedSinceLastChange);
        BOOST_REQUIRE_EQUAL(thread2Id, threads[1].id);
        BOOST_REQUIRE_EQUAL(true, threads[1].visitedSinceLastChange);
    }
}

BOOST_AUTO_TEST_CASE( Voting_a_discussion_thread_message_fails_if_message_is_invalid )
{
    auto handler = createCommandHandler();
    auto user = createUserAndGetId(handler, "User");

    LoggedInUserChanger _(user);

    Forum::Commands::Command commands[] = 
    {
        Forum::Commands::UP_VOTE_DISCUSSION_THREAD_MESSAGE,
        Forum::Commands::DOWN_VOTE_DISCUSSION_THREAD_MESSAGE,
        Forum::Commands::RESET_VOTE_DISCUSSION_THREAD_MESSAGE
    };

    for (auto command : commands)
    {
        assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, handlerToObj(handler, command));
        assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, handlerToObj(handler, command, { "bogusId" }));
        assertStatusCodeEqual(StatusCode::NOT_FOUND, handlerToObj(handler, command, { sampleValidIdString }));
    }
}

BOOST_AUTO_TEST_CASE( Voting_a_discussion_thread_message_fails_if_the_voter_is_the_author_of_the_message )
{
    auto handler = createCommandHandler();

    auto userId = createUserAndGetId(handler, "User");
    auto threadId = createDiscussionThreadAndGetId(handler, "Thread");
    auto messageId = createDiscussionMessageAndGetId(handler, threadId, "Message");

    Forum::Commands::Command commands[] = 
    {
        Forum::Commands::UP_VOTE_DISCUSSION_THREAD_MESSAGE,
        Forum::Commands::DOWN_VOTE_DISCUSSION_THREAD_MESSAGE
    };

    for (auto command : commands)
    {
        assertStatusCodeEqual(StatusCode::NOT_ALLOWED, handlerToObj(handler, command, { messageId }));
    }
}

BOOST_AUTO_TEST_CASE( Voting_a_discussion_thread_message_can_only_occur_once_unless_reset )
{
    auto handler = createCommandHandler();

    auto user1Id = createUserAndGetId(handler, "User1");
    auto user2Id = createUserAndGetId(handler, "User2");
    auto threadId = createDiscussionThreadAndGetId(handler, "Thread");
    std::string message1Id, message2Id;

    {
        LoggedInUserChanger _(user1Id);
        {
            TimestampChanger __(1000);
            message1Id = createDiscussionMessageAndGetId(handler, threadId, "Message1");
        }
        {
            TimestampChanger __(2000);
            message2Id = createDiscussionMessageAndGetId(handler, threadId, "Message2");
        }
    }

    auto thread = deserializeThread(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { threadId })
                                    .get_child("thread"));

    BOOST_REQUIRE_EQUAL(2, thread.messages.size());
    BOOST_REQUIRE_EQUAL(message1Id, thread.messages[0].id);
    BOOST_REQUIRE_EQUAL(0, thread.messages[0].upVotes.size());
    BOOST_REQUIRE_EQUAL(0, thread.messages[0].downVotes.size());

    {
        LoggedInUserChanger _(user2Id);
        TimestampChanger __(3000);
        assertStatusCodeEqual(StatusCode::NO_EFFECT, handlerToObj(handler,
                                                                  Forum::Commands::RESET_VOTE_DISCUSSION_THREAD_MESSAGE,
                                                                  { message1Id }));
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler,
                                                           Forum::Commands::UP_VOTE_DISCUSSION_THREAD_MESSAGE,
                                                           { message1Id }));
        assertStatusCodeEqual(StatusCode::NO_EFFECT, handlerToObj(handler,
                                                                  Forum::Commands::UP_VOTE_DISCUSSION_THREAD_MESSAGE,
                                                                  { message1Id }));
        assertStatusCodeEqual(StatusCode::NO_EFFECT, handlerToObj(handler,
                                                                  Forum::Commands::RESET_VOTE_DISCUSSION_THREAD_MESSAGE,
                                                                  { message2Id }));
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler,
                                                           Forum::Commands::DOWN_VOTE_DISCUSSION_THREAD_MESSAGE,
                                                           { message2Id }));
    }

    thread = deserializeThread(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { threadId })
                               .get_child("thread"));

    BOOST_REQUIRE_EQUAL(2, thread.messages.size());
    BOOST_REQUIRE_EQUAL(message1Id, thread.messages[0].id);
    BOOST_REQUIRE_EQUAL(1, thread.messages[0].upVotes.size());
    BOOST_REQUIRE_EQUAL(user2Id, thread.messages[0].upVotes[0].userId);
    BOOST_REQUIRE_EQUAL("User2", thread.messages[0].upVotes[0].userName);
    BOOST_REQUIRE_EQUAL(3000, thread.messages[0].upVotes[0].at);
    BOOST_REQUIRE_EQUAL(0, thread.messages[0].downVotes.size());

    BOOST_REQUIRE_EQUAL(message2Id, thread.messages[1].id);
    BOOST_REQUIRE_EQUAL(0, thread.messages[1].upVotes.size());
    BOOST_REQUIRE_EQUAL(1, thread.messages[1].downVotes.size());
    BOOST_REQUIRE_EQUAL(user2Id, thread.messages[1].downVotes[0].userId);
    BOOST_REQUIRE_EQUAL("User2", thread.messages[1].downVotes[0].userName);
    BOOST_REQUIRE_EQUAL(3000, thread.messages[1].downVotes[0].at);

    {
        LoggedInUserChanger _(user2Id);
        TimestampChanger __(4000);
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler,
                                                           Forum::Commands::RESET_VOTE_DISCUSSION_THREAD_MESSAGE,
                                                           { message2Id }));
        assertStatusCodeEqual(StatusCode::NO_EFFECT, handlerToObj(handler,
                                                                  Forum::Commands::RESET_VOTE_DISCUSSION_THREAD_MESSAGE,
                                                                  { message2Id }));
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler,
                                                           Forum::Commands::UP_VOTE_DISCUSSION_THREAD_MESSAGE,
                                                           { message2Id }));
    }

    thread = deserializeThread(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { threadId })
                               .get_child("thread"));

    BOOST_REQUIRE_EQUAL(2, thread.messages.size());
    BOOST_REQUIRE_EQUAL(message1Id, thread.messages[0].id);
    BOOST_REQUIRE_EQUAL(1, thread.messages[0].upVotes.size());
    BOOST_REQUIRE_EQUAL(user2Id, thread.messages[0].upVotes[0].userId);
    BOOST_REQUIRE_EQUAL("User2", thread.messages[0].upVotes[0].userName);
    BOOST_REQUIRE_EQUAL(3000, thread.messages[0].upVotes[0].at);
    BOOST_REQUIRE_EQUAL(0, thread.messages[0].downVotes.size());

    BOOST_REQUIRE_EQUAL(message2Id, thread.messages[1].id);
    BOOST_REQUIRE_EQUAL(1, thread.messages[1].upVotes.size());
    BOOST_REQUIRE_EQUAL(user2Id, thread.messages[1].upVotes[0].userId);
    BOOST_REQUIRE_EQUAL("User2", thread.messages[1].upVotes[0].userName);
    BOOST_REQUIRE_EQUAL(4000, thread.messages[1].upVotes[0].at);
    BOOST_REQUIRE_EQUAL(0, thread.messages[1].downVotes.size());
}

BOOST_AUTO_TEST_CASE( Deleting_a_user_removes_all_votes_cast_by_that_user )
{
    auto handler = createCommandHandler();

    auto user1Id = createUserAndGetId(handler, "User1");
    auto user2Id = createUserAndGetId(handler, "User2");
    auto user3Id = createUserAndGetId(handler, "User3");
    auto threadId = createDiscussionThreadAndGetId(handler, "Thread");
    std::string message1Id, message2Id;

    {
        LoggedInUserChanger _(user1Id);
        {
            TimestampChanger __(1000);
            message1Id = createDiscussionMessageAndGetId(handler, threadId, "Message1");
        }
        {
            TimestampChanger __(2000);
            message2Id = createDiscussionMessageAndGetId(handler, threadId, "Message2");
        }
    }

    auto thread = deserializeThread(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { threadId })
                                    .get_child("thread"));

    BOOST_REQUIRE_EQUAL(2, thread.messages.size());
    BOOST_REQUIRE_EQUAL(message1Id, thread.messages[0].id);
    BOOST_REQUIRE_EQUAL(0, thread.messages[0].upVotes.size());
    BOOST_REQUIRE_EQUAL(0, thread.messages[0].downVotes.size());

    {
        LoggedInUserChanger _(user3Id);
        TimestampChanger __(3000);
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler,
                                                           Forum::Commands::DOWN_VOTE_DISCUSSION_THREAD_MESSAGE,
                                                           { message1Id }));
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler,
                                                           Forum::Commands::DOWN_VOTE_DISCUSSION_THREAD_MESSAGE,
                                                           { message2Id }));
    }
    {
        LoggedInUserChanger _(user2Id);
        TimestampChanger __(4000);
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler,
                                                           Forum::Commands::UP_VOTE_DISCUSSION_THREAD_MESSAGE,
                                                           { message1Id }));
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler,
                                                           Forum::Commands::DOWN_VOTE_DISCUSSION_THREAD_MESSAGE,
                                                           { message2Id }));
    }

    thread = deserializeThread(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { threadId })
                               .get_child("thread"));

    BOOST_REQUIRE_EQUAL(2, thread.messages.size());
    BOOST_REQUIRE_EQUAL(message1Id, thread.messages[0].id);
    BOOST_REQUIRE_EQUAL(1, thread.messages[0].upVotes.size());
    BOOST_REQUIRE_EQUAL(user2Id, thread.messages[0].upVotes[0].userId);
    BOOST_REQUIRE_EQUAL("User2", thread.messages[0].upVotes[0].userName);
    BOOST_REQUIRE_EQUAL(4000, thread.messages[0].upVotes[0].at);
    BOOST_REQUIRE_EQUAL(1, thread.messages[0].downVotes.size());
    BOOST_REQUIRE_EQUAL(user3Id, thread.messages[0].downVotes[0].userId);
    BOOST_REQUIRE_EQUAL("User3", thread.messages[0].downVotes[0].userName);
    BOOST_REQUIRE_EQUAL(3000, thread.messages[0].downVotes[0].at);

    BOOST_REQUIRE_EQUAL(message2Id, thread.messages[1].id);
    BOOST_REQUIRE_EQUAL(0, thread.messages[1].upVotes.size());
    BOOST_REQUIRE_EQUAL(2, thread.messages[1].downVotes.size());
    BOOST_REQUIRE_EQUAL(user3Id, thread.messages[1].downVotes[0].userId);
    BOOST_REQUIRE_EQUAL("User3", thread.messages[1].downVotes[0].userName);
    BOOST_REQUIRE_EQUAL(3000, thread.messages[1].downVotes[0].at);
    BOOST_REQUIRE_EQUAL(user2Id, thread.messages[1].downVotes[1].userId);
    BOOST_REQUIRE_EQUAL("User2", thread.messages[1].downVotes[1].userName);
    BOOST_REQUIRE_EQUAL(4000, thread.messages[1].downVotes[1].at);

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::DELETE_USER, { user3Id }));

    thread = deserializeThread(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { threadId })
                               .get_child("thread"));


    BOOST_REQUIRE_EQUAL(2, thread.messages.size());
    BOOST_REQUIRE_EQUAL(message1Id, thread.messages[0].id);
    BOOST_REQUIRE_EQUAL(1, thread.messages[0].upVotes.size());
    BOOST_REQUIRE_EQUAL(user2Id, thread.messages[0].upVotes[0].userId);
    BOOST_REQUIRE_EQUAL("User2", thread.messages[0].upVotes[0].userName);
    BOOST_REQUIRE_EQUAL(4000, thread.messages[0].upVotes[0].at);
    BOOST_REQUIRE_EQUAL(0, thread.messages[0].downVotes.size());

    BOOST_REQUIRE_EQUAL(message2Id, thread.messages[1].id);
    BOOST_REQUIRE_EQUAL(0, thread.messages[1].upVotes.size());
    BOOST_REQUIRE_EQUAL(1, thread.messages[1].downVotes.size());
    BOOST_REQUIRE_EQUAL(user2Id, thread.messages[1].downVotes[0].userId);
    BOOST_REQUIRE_EQUAL("User2", thread.messages[1].downVotes[0].userName);
    BOOST_REQUIRE_EQUAL(4000, thread.messages[1].downVotes[0].at);
}

BOOST_AUTO_TEST_CASE( Latest_discussion_message_of_thread_does_not_include_votes )
{
    auto handler = createCommandHandler();

    auto user1Id = createUserAndGetId(handler, "User1");
    auto user2Id = createUserAndGetId(handler, "User2");
    auto threadId = createDiscussionThreadAndGetId(handler, "Thread");
    std::string messageId;

    {
        LoggedInUserChanger _(user1Id);
        {
            TimestampChanger __(1000);
            messageId = createDiscussionMessageAndGetId(handler, threadId, "Message");
        }
    }
    {
        LoggedInUserChanger _(user2Id);
        TimestampChanger __(2000);
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler,
                                                           Forum::Commands::UP_VOTE_DISCUSSION_THREAD_MESSAGE,
                                                           { messageId }));
    }

    auto result = handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_CREATED, SortOrder::Ascending);
    auto resultThreads = result.get_child("threads");

    for (auto& pair : resultThreads)
    {
        for (auto& threadProperty : pair.second)
        {
            if (threadProperty.first == "latestMessage")
            {
                BOOST_REQUIRE( ! treeContains(threadProperty.second, "upVotes"));
                BOOST_REQUIRE( ! treeContains(threadProperty.second, "downVotes"));
            }
        }
    }
}

BOOST_AUTO_TEST_CASE( Retrievng_a_list_of_threads_includes_the_vote_score_of_the_first_message )
{
    auto handler = createCommandHandler();

    auto user1Id = createUserAndGetId(handler, "User1");
    auto user2Id = createUserAndGetId(handler, "User2");
    auto user3Id = createUserAndGetId(handler, "User3");
    auto threadId = createDiscussionThreadAndGetId(handler, "Thread");
    std::string message1Id, message2Id;

    {
        LoggedInUserChanger _(user1Id);
        {
            TimestampChanger __(1000);
            message1Id = createDiscussionMessageAndGetId(handler, threadId, "Message1");
        }
        {
            TimestampChanger __(2000);
            message2Id = createDiscussionMessageAndGetId(handler, threadId, "Message2");
        }
    }

    auto thread = deserializeThread(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { threadId })
                                    .get_child("thread"));

    BOOST_REQUIRE_EQUAL(2, thread.messages.size());
    BOOST_REQUIRE_EQUAL(message1Id, thread.messages[0].id);
    BOOST_REQUIRE_EQUAL(0, thread.voteScore);

    {
        LoggedInUserChanger _(user3Id);
        TimestampChanger __(3000);
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler,
                                                           Forum::Commands::DOWN_VOTE_DISCUSSION_THREAD_MESSAGE,
                                                           { message1Id }));
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler,
                                                           Forum::Commands::DOWN_VOTE_DISCUSSION_THREAD_MESSAGE,
                                                           { message2Id }));
    }

    thread = deserializeThread(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { threadId })
                               .get_child("thread"));

    BOOST_REQUIRE_EQUAL(2, thread.messages.size());
    BOOST_REQUIRE_EQUAL(message1Id, thread.messages[0].id);
    BOOST_REQUIRE_EQUAL(-1, thread.voteScore);

    {
        LoggedInUserChanger _(user2Id);
        TimestampChanger __(4000);
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler,
                                                           Forum::Commands::UP_VOTE_DISCUSSION_THREAD_MESSAGE,
                                                           { message1Id }));
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler,
                                                           Forum::Commands::DOWN_VOTE_DISCUSSION_THREAD_MESSAGE,
                                                           { message2Id }));
    }

    thread = deserializeThread(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { threadId })
                               .get_child("thread"));

    BOOST_REQUIRE_EQUAL(2, thread.messages.size());
    BOOST_REQUIRE_EQUAL(message1Id, thread.messages[0].id);
    BOOST_REQUIRE_EQUAL(0, thread.voteScore);

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::DELETE_USER, { user3Id }));

    thread = deserializeThread(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { threadId })
                               .get_child("thread"));

    BOOST_REQUIRE_EQUAL(2, thread.messages.size());
    BOOST_REQUIRE_EQUAL(message1Id, thread.messages[0].id);
    BOOST_REQUIRE_EQUAL(1, thread.voteScore);
}

//BOOST_AUTO_TEST_CASE( Retrieving_discussion_threads_can_check_for_latest_visible_change )
//{
//}
//
//BOOST_AUTO_TEST_CASE( Discussion_thread_latest_visible_change_is_updated_on_thread_creation )
//{
//}
//
//BOOST_AUTO_TEST_CASE( Discussion_thread_latest_visible_change_is_updated_on_thread_update )
//{
//}
//
//BOOST_AUTO_TEST_CASE( Discussion_thread_latest_visible_change_is_updated_on_adding_messages_to_thread )
//{
//}
//
//BOOST_AUTO_TEST_CASE( Discussion_thread_latest_visible_change_is_updated_on_editing_messages_from_thread )
//{
//}
//
//BOOST_AUTO_TEST_CASE( Discussion_thread_latest_visible_change_is_updated_on_removing_messages_from_thread )
//{
//}
//
//BOOST_AUTO_TEST_CASE( Discussion_thread_latest_visible_change_is_updated_on_moving_messages_from_thread )
//{
//}
//
//BOOST_AUTO_TEST_CASE( Discussion_thread_latest_visible_change_is_updated_on_merging_threads )
//{
//}
//
//BOOST_AUTO_TEST_CASE( Discussion_thread_latest_visible_change_is_updated_on_thread_tag_link_change )
//{
//}
//
//BOOST_AUTO_TEST_CASE( Discussion_thread_latest_visible_change_is_updated_on_thread_category_link_change )
//{
//}

