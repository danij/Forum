#include "CommandsCommon.h"
#include "DelegateObserver.h"
#include "RandomGenerator.h"
#include "TestHelpers.h"

using namespace Forum::Configuration;
using namespace Forum::Context;
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

    handlerToObj(handler, Forum::Commands::GET_USERS_BY_NAME, SortOrder::Ascending);
    handlerToObj(handler, Forum::Commands::GET_USERS_BY_NAME, SortOrder::Descending);
    handlerToObj(handler, Forum::Commands::GET_USERS_BY_CREATED, SortOrder::Ascending);
    handlerToObj(handler, Forum::Commands::GET_USERS_BY_CREATED, SortOrder::Descending);
    handlerToObj(handler, Forum::Commands::GET_USERS_BY_LAST_SEEN, SortOrder::Ascending);
    handlerToObj(handler, Forum::Commands::GET_USERS_BY_LAST_SEEN, SortOrder::Descending);

    BOOST_REQUIRE_EQUAL(6, observerCalledNTimes);
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

    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_NAME, SortOrder::Ascending, { user1 });
    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_NAME, SortOrder::Descending, { user1 });
    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_CREATED, SortOrder::Ascending, { user1 });
    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_CREATED, SortOrder::Descending, { user1 });
    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_LAST_UPDATED, SortOrder::Ascending, { user1 });
    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_OF_USER_BY_LAST_UPDATED, SortOrder::Descending, { user1 });

    BOOST_REQUIRE_EQUAL(6, methodCalledNrTimes);
}

BOOST_AUTO_TEST_CASE( Retrieving_discussion_thread_messages_of_user_invokes_observer )
{
    int methodCalledNrTimes = 0;
    auto handler = createCommandHandler();

    DisposingDelegateObserver observer(*handler);
    observer->getDiscussionThreadMessagesOfUserAction = [&](auto& _, auto& __) { methodCalledNrTimes += 1; };

    auto user1 = createUserAndGetId(handler, "User1");

    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_MESSAGES_OF_USER_BY_CREATED, SortOrder::Ascending, 
                 { user1 });
    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_MESSAGES_OF_USER_BY_CREATED, SortOrder::Descending, 
                 { user1 });

    BOOST_REQUIRE_EQUAL(2, methodCalledNrTimes);
}

BOOST_AUTO_TEST_CASE( Retrieving_discussion_threads_invokes_observer )
{
    int observerCalledNTimes = 0;
    auto handler = createCommandHandler();

    DisposingDelegateObserver observer(*handler);
    observer->getDiscussionThreadsAction = [&](auto& _) { observerCalledNTimes += 1; };

    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_NAME, SortOrder::Ascending);
    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_NAME, SortOrder::Descending);
    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_CREATED, SortOrder::Ascending);
    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_CREATED, SortOrder::Descending);
    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_LAST_UPDATED, SortOrder::Ascending);
    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREADS_BY_LAST_UPDATED, SortOrder::Descending);

    BOOST_REQUIRE_EQUAL(6, observerCalledNTimes);
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

BOOST_AUTO_TEST_CASE( Merging_discussion_threads_invokes_observer )
{
    std::string observedFromThreadId, observedToThreadId;
    auto handler = createCommandHandler();

    DisposingDelegateObserver observer(*handler);
    observer->mergeDiscussionThreadsAction = [&](auto& _, auto& fromThread, auto& toThread)
    {
        observedFromThreadId = fromThread.id();
        observedToThreadId = toThread.id();
    };

    auto fromThreadId = createDiscussionThreadAndGetId(handler, "Thread1");
    auto toThreadId = createDiscussionThreadAndGetId(handler, "Thread2");

    handlerToObj(handler, Forum::Commands::MERGE_DISCUSSION_THREADS, { fromThreadId, toThreadId });

    BOOST_REQUIRE_EQUAL(fromThreadId, observedFromThreadId);
    BOOST_REQUIRE_EQUAL(toThreadId, observedToThreadId);
}

BOOST_AUTO_TEST_CASE( Moving_discussion_thread_messages_invokes_observer )
{
    std::string observedMessageId, observedToThreadId;
    auto handler = createCommandHandler();

    DisposingDelegateObserver observer(*handler);
    observer->moveDiscussionThreadMessageAction = [&](auto& _, auto& message, auto& intoThread)
    {
        observedMessageId = message.id();
        observedToThreadId = intoThread.id();
    };

    auto fromThreadId = createDiscussionThreadAndGetId(handler, "Thread1");
    auto messageId = createDiscussionMessageAndGetId(handler, fromThreadId, "Message1");
    auto toThreadId = createDiscussionThreadAndGetId(handler, "Thread2");

    handlerToObj(handler, Forum::Commands::MOVE_DISCUSSION_THREAD_MESSAGE, { messageId, toThreadId });

    BOOST_REQUIRE_EQUAL(messageId, observedMessageId);
    BOOST_REQUIRE_EQUAL(toThreadId, observedToThreadId);
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

BOOST_AUTO_TEST_CASE( Observer_context_includes_user_that_performs_the_action )
{
    auto handler = createCommandHandler();
    std::string user1;
    std::string userIdFromContext;
    std::string userNameFromContext;

    DisposingDelegateObserver observer(*handler);
    observer->getEntitiesCountAction = [&](auto& context)
    {
        userIdFromContext = static_cast<std::string>(context.performedBy.id());
        userNameFromContext = context.performedBy.name();
    };

    {
        user1 = createUserAndGetId(handler, "User1");
    }
    {
        LoggedInUserChanger _(user1);
        handlerToObj(handler, Forum::Commands::COUNT_ENTITIES);
    }
    BOOST_REQUIRE_EQUAL(user1, userIdFromContext);
    BOOST_REQUIRE_EQUAL("User1", userNameFromContext);
}

BOOST_AUTO_TEST_CASE( Observer_context_performed_by_is_the_anonymous_user )
{
    auto handler = createCommandHandler();
    std::string userIdFromContext;
    std::string userNameFromContext;

    DisposingDelegateObserver observer(*handler);
    observer->getEntitiesCountAction = [&](auto& context)
    {
        userIdFromContext = static_cast<std::string>(context.performedBy.id());
        userNameFromContext = context.performedBy.name();
    };

    handlerToObj(handler, Forum::Commands::COUNT_ENTITIES);

    BOOST_REQUIRE_EQUAL(static_cast<std::string>(UuidString::empty), userIdFromContext);
    BOOST_REQUIRE_EQUAL("<anonymous>", userNameFromContext);
}

BOOST_AUTO_TEST_CASE( Observer_context_includes_timestamp_of_action )
{
    auto handler = createCommandHandler();
    const Timestamp timestamp = 1000;
    Timestamp timestampFromContext = 0;

    DisposingDelegateObserver observer(*handler);
    observer->getEntitiesCountAction = [&](auto& context) { timestampFromContext = context.timestamp; };

    {
        TimestampChanger _(timestamp);
        handlerToObj(handler, Forum::Commands::COUNT_ENTITIES);
    }
    BOOST_REQUIRE_EQUAL(timestamp, timestampFromContext);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_tag_invokes_observer )
{
    std::string newTagName;
    auto handler = createCommandHandler();

    DisposingDelegateObserver observer(*handler);
    observer->addNewDiscussionTagAction = [&](auto& _, auto& newTag)
    {
        newTagName = newTag.name();
    };

    createDiscussionTagAndGetId(handler, "Foo");
    BOOST_REQUIRE_EQUAL("Foo", newTagName);
}

BOOST_AUTO_TEST_CASE( Retrieving_discussion_tags_invokes_observer )
{
    int observerCalledNTimes = 0;
    auto handler = createCommandHandler();

    DisposingDelegateObserver observer(*handler);
    observer->getDiscussionTagsAction = [&](auto& _) { observerCalledNTimes += 1; };

    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_TAGS_BY_NAME, SortOrder::Ascending);
    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_TAGS_BY_NAME, SortOrder::Descending);
    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_TAGS_BY_MESSAGE_COUNT, SortOrder::Ascending);
    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_TAGS_BY_MESSAGE_COUNT, SortOrder::Descending);

    BOOST_REQUIRE_EQUAL(4, observerCalledNTimes);
}

BOOST_AUTO_TEST_CASE( Renaming_a_discussion_tag_invokes_observer )
{
    std::string newName;
    auto tagChange = DiscussionTag::ChangeType::None;
    auto handler = createCommandHandler();

    DisposingDelegateObserver observer(*handler);
    observer->changeDiscussionTagAction = [&](auto& _, auto& tag, auto change)
    {
        newName = tag.name();
        tagChange = change;
    };

    auto tagId = createDiscussionTagAndGetId(handler, "Abc");

    handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_TAG_NAME, { tagId, "Xyz" });
    BOOST_REQUIRE_EQUAL("Xyz", newName);
    BOOST_REQUIRE_EQUAL(DiscussionTag::ChangeType::Name, tagChange);
}

BOOST_AUTO_TEST_CASE( Changing_a_discussion_tag_ui_blob_invokes_observer )
{
    std::string newBlob;
    auto tagChange = DiscussionTag::ChangeType::None;
    auto handler = createCommandHandler();

    DisposingDelegateObserver observer(*handler);
    observer->changeDiscussionTagAction = [&](auto& _, auto& tag, auto change)
    {
        newBlob = tag.uiBlob();
        tagChange = change;
    };

    auto tagId = createDiscussionTagAndGetId(handler, "Abc");

    handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_TAG_UI_BLOB, { tagId, "sample blob" });
    BOOST_REQUIRE_EQUAL("sample blob", newBlob);
    BOOST_REQUIRE_EQUAL(DiscussionTag::ChangeType::UIBlob, tagChange);
}

BOOST_AUTO_TEST_CASE( Deleting_a_discussion_tag_invokes_observer )
{
    std::string deletedTagId;
    auto handler = createCommandHandler();

    DisposingDelegateObserver observer(*handler);
    observer->deleteDiscussionTagAction = [&](auto& _, auto& tag)
    {
        deletedTagId = static_cast<std::string>(tag.id());
    };

    auto tagId = createDiscussionTagAndGetId(handler, "Abc");

    handlerToObj(handler, Forum::Commands::DELETE_DISCUSSION_TAG, { tagId });
    BOOST_REQUIRE_EQUAL(tagId, deletedTagId);
}

BOOST_AUTO_TEST_CASE( Attaching_a_discussion_tag_to_a_thread_invokes_observer )
{
    std::string observedTagId, observedThreadId;
    auto handler = createCommandHandler();

    DisposingDelegateObserver observer(*handler);
    observer->addDiscussionTagToThreadAction = [&](auto& _, auto& tag, auto& thread)
    {
        observedTagId = tag.id();
        observedThreadId = thread.id();
    };

    auto tagId = createDiscussionTagAndGetId(handler, "Tag");
    auto threadId = createDiscussionThreadAndGetId(handler, "Thread");

    handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_TAG_TO_THREAD, { tagId, threadId });

    BOOST_REQUIRE_EQUAL(tagId, observedTagId);
    BOOST_REQUIRE_EQUAL(threadId, observedThreadId);
}

BOOST_AUTO_TEST_CASE( Detaching_a_discussion_tag_from_a_thread_invokes_observer )
{
    std::string observedTagId, observedThreadId;
    auto handler = createCommandHandler();

    DisposingDelegateObserver observer(*handler);
    observer->removeDiscussionTagFromThreadAction = [&](auto& _, auto& tag, auto& thread)
    {
        observedTagId = tag.id();
        observedThreadId = thread.id();
    };

    auto tagId = createDiscussionTagAndGetId(handler, "Tag");
    auto threadId = createDiscussionThreadAndGetId(handler, "Thread");

    handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_TAG_TO_THREAD, { tagId, threadId });
    handlerToObj(handler, Forum::Commands::REMOVE_DISCUSSION_TAG_FROM_THREAD, { tagId, threadId });

    BOOST_REQUIRE_EQUAL(tagId, observedTagId);
    BOOST_REQUIRE_EQUAL(threadId, observedThreadId);
}

BOOST_AUTO_TEST_CASE(Retrieving_discussion_threads_attached_to_tags_invokes_observer)
{
    int observerCalledNTimes = 0;
    std::string observedTagId;
    auto handler = createCommandHandler();

    DisposingDelegateObserver observer(*handler);
    observer->getDiscussionThreadsWithTagAction = [&](auto& _, auto& tag)
    {
        observerCalledNTimes += 1;
        observedTagId = tag.id();
    };

    auto tagId = createDiscussionTagAndGetId(handler, "Tag");

    Forum::Commands::Command commands[] =
    {
        Forum::Commands::GET_DISCUSSION_THREADS_WITH_TAG_BY_NAME,
        Forum::Commands::GET_DISCUSSION_THREADS_WITH_TAG_BY_CREATED,
        Forum::Commands::GET_DISCUSSION_THREADS_WITH_TAG_BY_LAST_UPDATED,
        Forum::Commands::GET_DISCUSSION_THREADS_WITH_TAG_BY_MESSAGE_COUNT
    };

    for (auto command : commands)
        for (auto sortOrder : { SortOrder::Ascending, SortOrder::Descending })
        {
            handlerToObj(handler, command, sortOrder, { tagId });
        }

    BOOST_REQUIRE_EQUAL(8, observerCalledNTimes);
    BOOST_REQUIRE_EQUAL(tagId, observedTagId);
}

BOOST_AUTO_TEST_CASE( Merging_discussion_tags_invokes_observer)
{
    std::string observedFromTagId, observedToTagId;
    auto handler = createCommandHandler();

    DisposingDelegateObserver observer(*handler);
    observer->mergeDiscussionTagsAction = [&](auto& _, auto& fromTag, auto& toTag)
    {
        observedFromTagId = fromTag.id();
        observedToTagId = toTag.id();
    };

    auto fromTagId = createDiscussionTagAndGetId(handler, "Tag1");
    auto toTagId = createDiscussionTagAndGetId(handler, "Tag2");

    handlerToObj(handler, Forum::Commands::MERGE_DISCUSSION_TAG_INTO_OTHER_TAG, { fromTagId, toTagId });

    BOOST_REQUIRE_EQUAL(fromTagId, observedFromTagId);
    BOOST_REQUIRE_EQUAL(toTagId, observedToTagId);
}
