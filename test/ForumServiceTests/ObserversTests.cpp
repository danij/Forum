#include "CommandsCommon.h"
#include "DelegateObserver.h"
#include "RandomGenerator.h"
#include "TestHelpers.h"

using namespace Forum::Configuration;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;

BOOST_AUTO_TEST_CASE( Counting_entities_invokes_observer )
{
    bool observerCalled = false;
    auto handler = createCommandHandler();

    DisposingDelegateObserver observer(*handler);
    observer->getEntitiesCountAction = [&](auto& _) { observerCalled = true; };

    handlerToObj(handler, Forum::Commands::COUNT_ENTITIES);
    BOOST_REQUIRE(observerCalled);
}

BOOST_AUTO_TEST_CASE( Retrieving_users_invokes_observer )
{
    int observerCalledNTimes = 0;
    auto handler = createCommandHandler();

    DisposingDelegateObserver observer(*handler);
    observer->getUsersAction = [&](auto& _) { observerCalledNTimes += 1; };

    handlerToObj(handler, Forum::Commands::GET_USERS_BY_NAME);
    handlerToObj(handler, Forum::Commands::GET_USERS_BY_CREATED_ASCENDING);
    handlerToObj(handler, Forum::Commands::GET_USERS_BY_CREATED_DESCENDING);
    handlerToObj(handler, Forum::Commands::GET_USERS_BY_LAST_SEEN_ASCENDING);
    handlerToObj(handler, Forum::Commands::GET_USERS_BY_LAST_SEEN_DESCENDING);

    BOOST_REQUIRE_EQUAL(5, observerCalledNTimes);
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

BOOST_AUTO_TEST_CASE( Retrieving_users_by_id_invokes_observer )
{
    IdType idToBeRetrieved;
    auto handler = createCommandHandler();

    DisposingDelegateObserver observer(*handler);
    observer->getUserByIdAction = [&](auto& _, auto& id) { idToBeRetrieved = id; };

    handlerToObj(handler, Forum::Commands::GET_USER_BY_ID, { static_cast<std::string>(sampleValidId) });
    BOOST_REQUIRE_EQUAL(sampleValidId, idToBeRetrieved);
}

BOOST_AUTO_TEST_CASE( Retrieving_users_by_name_invokes_observer )
{
    std::string nameToBeRetrieved;
    auto handler = createCommandHandler();

    DisposingDelegateObserver observer(*handler);
    observer->getUserByNameAction = [&](auto& _, auto& name) { nameToBeRetrieved = name; };

    handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "SampleUser" });
    BOOST_REQUIRE_EQUAL("SampleUser", nameToBeRetrieved);
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

BOOST_AUTO_TEST_CASE( Retrieving_discussion_threads_of_user_invokes_observer )
{
    int methodCalledNrTimes = 0;
    auto handler = createCommandHandler();

    DisposingDelegateObserver observer(*handler);
    observer->getDiscussionThreadsOfUserAction = [&](auto& _, auto& __) { methodCalledNrTimes += 1; };

    auto user1 = createUserAndGetId(handler, "User1");

    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_NAME, { user1 });
    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_CREATED_ASCENDING, { user1 });
    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_CREATED_DESCENDING, { user1 });
    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_LAST_UPDATED_ASCENDING, { user1 });
    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_LAST_UPDATED_DESCENDING, { user1 });

    BOOST_REQUIRE_EQUAL(5, methodCalledNrTimes);
}

BOOST_AUTO_TEST_CASE( Retrieving_discussion_thread_messages_of_user_invokes_observer )
{
    int methodCalledNrTimes = 0;
    auto handler = createCommandHandler();

    DisposingDelegateObserver observer(*handler);
    observer->getDiscussionThreadMessagesOfUserAction = [&](auto& _, auto& __) { methodCalledNrTimes += 1; };

    auto user1 = createUserAndGetId(handler, "User1");

    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_MESSAGES_OF_USER_BY_CREATED_ASCENDING, { user1 });
    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_MESSAGES_OF_USER_BY_CREATED_DESCENDING, { user1 });

    BOOST_REQUIRE_EQUAL(2, methodCalledNrTimes);
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
