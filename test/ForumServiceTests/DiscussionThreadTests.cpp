#include <vector>

#include "CommandsCommon.h"
#include "DelegateObserver.h"
#include "RandomGenerator.h"
#include "TestHelpers.h"

using namespace Forum::Configuration;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;

struct SerializedDiscussionThread
{
    std::string id;
    std::string name;
    Timestamp created = 0;
    Timestamp lastUpdated = 0;

    void populate(const boost::property_tree::ptree& tree)
    {
        id = tree.get<std::string>("id");
        name = tree.get<std::string>("name");
        created = tree.get<Timestamp>("created");
        lastUpdated = tree.get<Timestamp>("lastUpdated");
    }
};

auto deserializeThreads(const boost::property_tree::ptree& collection)
{
    std::vector<SerializedDiscussionThread> result;
    for (auto& tree : collection)
    {
        result.push_back({});
        result.rbegin()->populate(tree.second);
    }
    return result;
}

BOOST_AUTO_TEST_CASE( Discussion_thread_count_is_initially_zero )
{
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::COUNT_DISCUSSION_THREADS);
    BOOST_REQUIRE_EQUAL(0, returnObject.get<int>("count"));
}

BOOST_AUTO_TEST_CASE( Counting_discussion_threads_invokes_observer )
{
    bool observerCalled = false;
    auto handler = createCommandHandler();

    DisposingDelegateObserver observer(*handler);
    observer->getDiscussionThreadCountAction = [&](auto& _) { observerCalled = true; };

    handlerToObj(handler, Forum::Commands::COUNT_DISCUSSION_THREADS);
    BOOST_REQUIRE(observerCalled);
}

BOOST_AUTO_TEST_CASE( Retrieving_discussion_threads_invokes_observer )
{
    int observerCalledNTimes = 0;
    auto handler = createCommandHandler();

    DisposingDelegateObserver observer(*handler);
    observer->getDiscussionThreadsAction = [&](auto& _) { observerCalledNTimes += 1; };

    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_NAME);
    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_CREATED);
    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_LAST_UPDATED);

    BOOST_REQUIRE_EQUAL(3, observerCalledNTimes);
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
    BOOST_REQUIRE_NE("", returnObject.get<std::string>("id"));
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
    std::string username(config->discussionThread.minNameLength - 1, 'a');
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_DISCUSSION_THREAD, { username });
    assertStatusCodeEqual(StatusCode::VALUE_TOO_SHORT, returnObject);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_thread_with_a_longer_name_fails )
{
    auto config = getGlobalConfig();
    std::string username(config->discussionThread.maxNameLength + 1, 'a');
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_DISCUSSION_THREAD, { username });
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
    auto returnObject = handlerToObj(createCommandHandler(), Forum::Commands::ADD_USER, { "\xFF\xFF" });
    assertStatusCodeEqual(StatusCode::INVALID_PARAMETERS, returnObject);
}

BOOST_AUTO_TEST_CASE( A_discussion_thread_that_was_created_can_be_retrieved_and_has_a_distinct_id )
{
    auto handler = createCommandHandler();
    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { "Thread1" }));
    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { "Thread2" }));

    auto threads = deserializeThreads(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_NAME)
                                              .get_child("threads"));

    BOOST_REQUIRE_NE(emptyIdString, threads[0].id);
    BOOST_REQUIRE_NE(emptyIdString, threads[1].id);
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

    BOOST_REQUIRE_EQUAL(1, handlerToObj(handler, Forum::Commands::COUNT_DISCUSSION_THREADS).get<int>("count"));

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_THREAD_NAME,
                                                       { threadId, "Xyz" }));
    SerializedDiscussionThread modifiedThread;
    modifiedThread.populate(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { threadId })
                                    .get_child("thread"));

    BOOST_REQUIRE_EQUAL(1, handlerToObj(handler, Forum::Commands::COUNT_DISCUSSION_THREADS).get<int>("count"));
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

BOOST_AUTO_TEST_CASE( Discussion_threads_can_be_retrieved_sorted_by_name )
{
    auto handler = createCommandHandler();
    std::vector<std::string> names = { "Abc", "Ghi", "Def" };

    for (auto& name : names)
    {
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { name }));
    }

    BOOST_REQUIRE_EQUAL(3, handlerToObj(handler, Forum::Commands::COUNT_DISCUSSION_THREADS).get<int>("count"));

    auto threads = deserializeThreads(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_NAME)
                                              .get_child("threads"));

    BOOST_REQUIRE_EQUAL(names.size(), threads.size());
    BOOST_REQUIRE_EQUAL("Abc", threads[0].name);
    BOOST_REQUIRE_EQUAL("Def", threads[1].name);
    BOOST_REQUIRE_EQUAL("Ghi", threads[2].name);
}

BOOST_AUTO_TEST_CASE( Discussion_threads_can_be_retrieved_sorted_by_creation_date )
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

    BOOST_REQUIRE_EQUAL(3, handlerToObj(handler, Forum::Commands::COUNT_DISCUSSION_THREADS).get<int>("count"));

    auto threads = deserializeThreads(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_CREATED)
                                              .get_child("threads"));

    BOOST_REQUIRE_EQUAL(names.size(), threads.size());
    BOOST_REQUIRE_EQUAL("Abc", threads[0].name);
    BOOST_REQUIRE_EQUAL("Ghi", threads[1].name);
    BOOST_REQUIRE_EQUAL("Def", threads[2].name);
    BOOST_REQUIRE_EQUAL(1000, threads[0].created);
    BOOST_REQUIRE_EQUAL(2000, threads[1].created);
    BOOST_REQUIRE_EQUAL(3000, threads[2].created);
}

BOOST_AUTO_TEST_CASE( Discussion_threads_can_be_retrieved_sorted_by_last_updated )
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

    BOOST_REQUIRE_EQUAL(3, handlerToObj(handler, Forum::Commands::COUNT_DISCUSSION_THREADS).get<int>("count"));

    {
        TimestampChanger timestampChanger(currentTime);
        assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_THREAD_NAME,
                                                           { ids[0], "Aabc" }));
    }

    auto threads = deserializeThreads(handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_LAST_UPDATED)
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

BOOST_AUTO_TEST_CASE( Deleting_an_inexistent_discussion_thread_returns_not_found )
{
    auto handler = createCommandHandler();
    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { "Abc" }));
    assertStatusCodeEqual(StatusCode::NOT_FOUND,
                          handlerToObj(handler, Forum::Commands::DELETE_DISCUSSION_THREAD, { "bogus id" }));
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

    BOOST_REQUIRE_EQUAL(3, handlerToObj(handler, Forum::Commands::COUNT_DISCUSSION_THREADS).get<int>("count"));

    assertStatusCodeEqual(StatusCode::OK, handlerToObj(handler, Forum::Commands::DELETE_DISCUSSION_THREAD, { ids[0] }));
    assertStatusCodeEqual(StatusCode::NOT_FOUND, handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID,
                                                              { ids[0] }));

    BOOST_REQUIRE_EQUAL(2, handlerToObj(handler, Forum::Commands::COUNT_DISCUSSION_THREADS).get<int>("count"));

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
