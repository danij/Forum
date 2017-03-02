#include "CommandsCommon.h"
#include "RandomGenerator.h"
#include "TestHelpers.h"

using namespace Forum::Configuration;
using namespace Forum::Context;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;

struct SignalAutoDisconnector
{
    explicit SignalAutoDisconnector(const boost::signals2::connection& connection) : connection_(connection)
    {
    }

    ~SignalAutoDisconnector()
    {
        connection_.disconnect();
    }

    SignalAutoDisconnector(const SignalAutoDisconnector&) = delete;
    SignalAutoDisconnector(SignalAutoDisconnector&&) = default;
    SignalAutoDisconnector& operator=(const SignalAutoDisconnector&) = delete;
    SignalAutoDisconnector& operator=(SignalAutoDisconnector&&) = default;

private:
    boost::signals2::connection connection_;
};

template <typename TSignal, typename TCallback>
auto addHandler(TSignal&& signal, TCallback&& callback)
{
    return SignalAutoDisconnector(signal.connect(callback));
}

BOOST_AUTO_TEST_CASE( Counting_entities_invokes_observer )
{
    bool observerCalled = false;
    auto handler = createCommandHandler();

    auto ___ = addHandler(handler->getReadRepository()->readEvents().onGetEntitiesCount, 
                          [&](auto& _) { observerCalled = true; });

    handlerToObj(handler, Forum::Commands::COUNT_ENTITIES);
    BOOST_REQUIRE(observerCalled);
}

BOOST_AUTO_TEST_CASE( Retrieving_users_invokes_observer )
{
    int observerCalledNTimes = 0;
    auto handler = createCommandHandler();

    auto ___ = addHandler(handler->getReadRepository()->readEvents().onGetUsers, 
                          [&](auto& _) { observerCalledNTimes += 1; });

    Forum::Commands::Command commands[] =
    {
        Forum::Commands::GET_USERS_BY_NAME,
        Forum::Commands::GET_USERS_BY_CREATED,
        Forum::Commands::GET_USERS_BY_LAST_SEEN,
        Forum::Commands::GET_USERS_BY_MESSAGE_COUNT
    };

    SortOrder sortOrders[] = { SortOrder::Ascending, SortOrder::Descending };

    for (auto command : commands)
    {
        for (auto sortOrder : sortOrders)
        {
            handlerToObj(handler, command, sortOrder);
        }
    }

    auto nrOfCalls = std::extent<decltype(commands)>::value * std::extent<decltype(sortOrders)>::value;
    BOOST_REQUIRE_EQUAL(nrOfCalls, observerCalledNTimes);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_invokes_observer )
{
    std::string newUserName;
    auto handler = createCommandHandler();

    auto ___ = addHandler(handler->getWriteRepository()->writeEvents().onAddNewUser, 
                          [&](auto& _, auto& newUser) { newUserName = newUser.name(); });

    handlerToObj(handler, Forum::Commands::ADD_USER, { "Foo" });
    BOOST_REQUIRE_EQUAL("Foo", newUserName);
}

BOOST_AUTO_TEST_CASE( Retrieving_users_by_id_invokes_observer )
{
    std::string idToBeRetrieved;
    auto handler = createCommandHandler();

    auto ___ = addHandler(handler->getReadRepository()->readEvents().onGetUserById,
                          [&](auto& _, auto& id) { idToBeRetrieved = static_cast<std::string>(id); });

    auto userId = createUserAndGetId(handler, "User");
    handlerToObj(handler, Forum::Commands::GET_USER_BY_ID, { userId });
    BOOST_REQUIRE_EQUAL(userId, idToBeRetrieved);
}

BOOST_AUTO_TEST_CASE( Retrieving_users_by_name_invokes_observer )
{
    std::string nameToBeRetrieved;
    auto handler = createCommandHandler();

    auto ___ = addHandler(handler->getReadRepository()->readEvents().onGetUserByName,
                          [&](auto& _, auto& name) { nameToBeRetrieved = name; });

    createUserAndGetId(handler, "SampleUser");
    handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "SampleUser" });
    BOOST_REQUIRE_EQUAL("SampleUser", nameToBeRetrieved);
}

BOOST_AUTO_TEST_CASE( Modifying_a_user_name_invokes_observer )
{
    std::string newName;
    auto userChange = User::ChangeType::None;
    auto handler = createCommandHandler();

    auto ___ = addHandler(handler->getWriteRepository()->writeEvents().onChangeUser,
                          [&](auto& _, auto& user, auto change)
                          {
                              newName = user.name();
                              userChange = change;
                          });

    handlerToObj(handler, Forum::Commands::ADD_USER, { "Abc" });
    auto user = handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "Abc" });
    auto userId = user.get<std::string>("user.id");

    handlerToObj(handler, Forum::Commands::CHANGE_USER_NAME, { userId, "Xyz" });
    BOOST_REQUIRE_EQUAL("Xyz", newName);
    BOOST_REQUIRE_EQUAL(User::ChangeType::Name, userChange);
}

BOOST_AUTO_TEST_CASE( Modifying_a_user_info_invokes_observer )
{
    std::string newInfo;
    auto userChange = User::ChangeType::None;
    auto handler = createCommandHandler();

    auto ___ = addHandler(handler->getWriteRepository()->writeEvents().onChangeUser,
                          [&](auto& _, auto& user, auto change)
                          {
                              newInfo = user.info();
                              userChange = change;
                          });

    auto userId = createUserAndGetId(handler, "Abc");

    handlerToObj(handler, Forum::Commands::CHANGE_USER_INFO, { userId, "Hello World" });
    BOOST_REQUIRE_EQUAL("Hello World", newInfo);
    BOOST_REQUIRE_EQUAL(User::ChangeType::Info, userChange);
}

BOOST_AUTO_TEST_CASE( Deleting_a_user_invokes_observer )
{
    std::string deletedUserName;
    auto handler = createCommandHandler();

    auto ___ = addHandler(handler->getWriteRepository()->writeEvents().onDeleteUser,
                          [&](auto& _, auto& user) { deletedUserName = user.name(); });

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

    auto ___ = addHandler(handler->getReadRepository()->readEvents().onGetDiscussionThreadsOfUser, 
                          [&](auto& _, auto& __) { methodCalledNrTimes += 1; });

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

    auto ___ = addHandler(handler->getReadRepository()->readEvents().onGetDiscussionThreadMessagesOfUser, 
                          [&](auto& _, auto& __) { methodCalledNrTimes += 1; });

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

    auto ___ = addHandler(handler->getReadRepository()->readEvents().onGetDiscussionThreads, 
                          [&](auto& _) { observerCalledNTimes += 1; });

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

    auto ___ = addHandler(handler->getReadRepository()->readEvents().onGetDiscussionThreadById, 
                          [&](auto& _, auto& id) { idOfThread = static_cast<std::string>(id); });

    auto threadId = createDiscussionThreadAndGetId(handler, "Thread");
    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { threadId });
    BOOST_REQUIRE_EQUAL(threadId, idOfThread);
}

BOOST_AUTO_TEST_CASE( Modifying_a_discussion_thread_invokes_observer )
{
    std::string newName;
    auto threadChange = DiscussionThread::ChangeType::None;
    auto handler = createCommandHandler();

    auto ___ = addHandler(handler->getWriteRepository()->writeEvents().onChangeDiscussionThread, 
                          [&](auto& _, auto& thread, auto change)
                          {
                              newName = thread.name();
                              threadChange = change;
                          });

    auto threadId = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { "Abc" }).get<std::string>("id");

    handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_THREAD_NAME, { threadId, "Xyz" });
    BOOST_REQUIRE_EQUAL("Xyz", newName);
    BOOST_REQUIRE_EQUAL(DiscussionThread::ChangeType::Name, threadChange);
}

BOOST_AUTO_TEST_CASE( Deleting_a_discussion_thread_invokes_observer )
{
    std::string deletedThreadId;
    auto handler = createCommandHandler();

    auto ___ = addHandler(handler->getWriteRepository()->writeEvents().onDeleteDiscussionThread, 
                          [&](auto& _, auto& thread) { deletedThreadId = static_cast<std::string>(thread.id()); });

    auto threadId = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { "Abc" }).get<std::string>("id");

    handlerToObj(handler, Forum::Commands::DELETE_DISCUSSION_THREAD, { threadId });
    BOOST_REQUIRE_EQUAL(threadId, deletedThreadId);
}

BOOST_AUTO_TEST_CASE( Merging_discussion_threads_invokes_observer )
{
    std::string observedFromThreadId, observedToThreadId;
    auto handler = createCommandHandler();

    auto ___ = addHandler(handler->getWriteRepository()->writeEvents().onMergeDiscussionThreads, 
                          [&](auto& _, auto& fromThread, auto& toThread)
                          {
                              observedFromThreadId = static_cast<std::string>(fromThread.id());
                              observedToThreadId = static_cast<std::string>(toThread.id());
                          });

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

    auto ___ = addHandler(handler->getWriteRepository()->writeEvents().onMoveDiscussionThreadMessage, 
                          [&](auto& _, auto& message, auto& intoThread)
                          {
                              observedMessageId = static_cast<std::string>(message.id());
                              observedToThreadId = static_cast<std::string>(intoThread.id());
                          });

    auto fromThreadId = createDiscussionThreadAndGetId(handler, "Thread1");
    auto messageId = createDiscussionMessageAndGetId(handler, fromThreadId, "Message1");
    auto toThreadId = createDiscussionThreadAndGetId(handler, "Thread2");

    handlerToObj(handler, Forum::Commands::MOVE_DISCUSSION_THREAD_MESSAGE, { messageId, toThreadId });

    BOOST_REQUIRE_EQUAL(messageId, observedMessageId);
    BOOST_REQUIRE_EQUAL(toThreadId, observedToThreadId);
}

BOOST_AUTO_TEST_CASE( Modifying_the_content_of_a_discussion_message_invokes_observer )
{
    std::string newContent, changeReason;
    auto messageChange = DiscussionThreadMessage::ChangeType::None;
    auto handler = createCommandHandler();

    auto ___ = addHandler(handler->getWriteRepository()->writeEvents().onChangeDiscussionThreadMessage, 
                          [&](auto& _, auto& message, auto change)
                          {
                              newContent = message.content();
                              changeReason = message.lastUpdatedReason();
                              messageChange = change;
                          });

    auto userId = createUserAndGetId(handler, "User");
    LoggedInUserChanger _(userId);

    auto threadId = createDiscussionThreadAndGetId(handler, "Abc");
    auto messageId = createDiscussionMessageAndGetId(handler, threadId, "Message");

    handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT, { messageId, "New Message", "Reason" });
    BOOST_REQUIRE_EQUAL("New Message", newContent);
    BOOST_REQUIRE_EQUAL("Reason", changeReason);
    BOOST_REQUIRE_EQUAL(DiscussionThreadMessage::ChangeType::Content, messageChange);
}

BOOST_AUTO_TEST_CASE( Deleting_a_discussion_message_invokes_observer )
{
    std::string deletedMessageId;
    auto handler = createCommandHandler();

    auto ___ = addHandler(handler->getWriteRepository()->writeEvents().onDeleteDiscussionThreadMessage, 
                          [&](auto& _, auto& message) { deletedMessageId = static_cast<std::string>(message.id()); });

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

    auto ___ = addHandler(handler->getReadRepository()->readEvents().onGetEntitiesCount, 
                          [&](auto& context)
                          {
                              userIdFromContext = static_cast<std::string>(context.performedBy.id());
                              userNameFromContext = context.performedBy.name();
                          });
    user1 = createUserAndGetId(handler, "User1");
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

    auto ___ = addHandler(handler->getReadRepository()->readEvents().onGetEntitiesCount, 
                          [&](auto& context)
                          {
                              userIdFromContext = static_cast<std::string>(context.performedBy.id());
                              userNameFromContext = context.performedBy.name();
                          });

    handlerToObj(handler, Forum::Commands::COUNT_ENTITIES);

    BOOST_REQUIRE_EQUAL(static_cast<std::string>(UuidString::empty), userIdFromContext);
    BOOST_REQUIRE_EQUAL("<anonymous>", userNameFromContext);
}

BOOST_AUTO_TEST_CASE( Observer_context_includes_timestamp_of_action )
{
    auto handler = createCommandHandler();
    const Timestamp timestamp = 1000;
    Timestamp timestampFromContext = 0;

    auto ___ = addHandler(handler->getReadRepository()->readEvents().onGetEntitiesCount, 
                          [&](auto& context) { timestampFromContext = context.timestamp; });
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

    auto ___ = addHandler(handler->getWriteRepository()->writeEvents().onAddNewDiscussionTag,
                          [&](auto& _, auto& newTag) { newTagName = newTag.name(); });

    createDiscussionTagAndGetId(handler, "Foo");
    BOOST_REQUIRE_EQUAL("Foo", newTagName);
}

BOOST_AUTO_TEST_CASE( Retrieving_discussion_tags_invokes_observer )
{
    int observerCalledNTimes = 0;
    auto handler = createCommandHandler();

    auto ___ = addHandler(handler->getReadRepository()->readEvents().onGetDiscussionTags,
                          [&](auto& _) { observerCalledNTimes += 1; });

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

    auto ___ = addHandler(handler->getWriteRepository()->writeEvents().onChangeDiscussionTag, 
                          [&](auto& _, auto& tag, auto change)
                          {
                              newName = tag.name();
                              tagChange = change;
                          });

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

    auto ___ = addHandler(handler->getWriteRepository()->writeEvents().onChangeDiscussionTag, 
                          [&](auto& _, auto& tag, auto change)
                          {
                              newBlob = tag.uiBlob();
                              tagChange = change;
                          });

    auto tagId = createDiscussionTagAndGetId(handler, "Abc");

    handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_TAG_UI_BLOB, { tagId, "sample blob" });
    BOOST_REQUIRE_EQUAL("sample blob", newBlob);
    BOOST_REQUIRE_EQUAL(DiscussionTag::ChangeType::UIBlob, tagChange);
}

BOOST_AUTO_TEST_CASE( Deleting_a_discussion_tag_invokes_observer )
{
    std::string deletedTagId;
    auto handler = createCommandHandler();

    auto ___ = addHandler(handler->getWriteRepository()->writeEvents().onDeleteDiscussionTag, 
                          [&](auto& _, auto& tag)
                          {
                              deletedTagId = static_cast<std::string>(tag.id());
                          });

    auto tagId = createDiscussionTagAndGetId(handler, "Abc");

    handlerToObj(handler, Forum::Commands::DELETE_DISCUSSION_TAG, { tagId });
    BOOST_REQUIRE_EQUAL(tagId, deletedTagId);
}

BOOST_AUTO_TEST_CASE( Attaching_a_discussion_tag_to_a_thread_invokes_observer )
{
    std::string observedTagId, observedThreadId;
    auto handler = createCommandHandler();

    auto ___ = addHandler(handler->getWriteRepository()->writeEvents().onAddDiscussionTagToThread, 
                          [&](auto& _, auto& tag, auto& thread)
                          {
                              observedTagId = static_cast<std::string>(tag.id());
                              observedThreadId = static_cast<std::string>(thread.id());
                          });

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

    auto ___ = addHandler(handler->getWriteRepository()->writeEvents().onRemoveDiscussionTagFromThread, 
                          [&](auto& _, auto& tag, auto& thread)
                          {
                              observedTagId = static_cast<std::string>(tag.id());
                              observedThreadId = static_cast<std::string>(thread.id());
                          });

    auto tagId = createDiscussionTagAndGetId(handler, "Tag");
    auto threadId = createDiscussionThreadAndGetId(handler, "Thread");

    handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_TAG_TO_THREAD, { tagId, threadId });
    handlerToObj(handler, Forum::Commands::REMOVE_DISCUSSION_TAG_FROM_THREAD, { tagId, threadId });

    BOOST_REQUIRE_EQUAL(tagId, observedTagId);
    BOOST_REQUIRE_EQUAL(threadId, observedThreadId);
}

BOOST_AUTO_TEST_CASE( Retrieving_discussion_threads_attached_to_tags_invokes_observer )
{
    int observerCalledNTimes = 0;
    std::string observedTagId;
    auto handler = createCommandHandler();

    auto ___ = addHandler(handler->getReadRepository()->readEvents().onGetDiscussionThreadsWithTag, 
                          [&](auto& _, auto& tag)
                          {
                              observerCalledNTimes += 1;
                              observedTagId = static_cast<std::string>(tag.id());
                          });

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

    auto ___ = addHandler(handler->getWriteRepository()->writeEvents().onMergeDiscussionTags, 
                          [&](auto& _, auto& fromTag, auto& toTag)
                          {
                              observedFromTagId = static_cast<std::string>(fromTag.id());
                              observedToTagId = static_cast<std::string>(toTag.id());
                          });

    auto fromTagId = createDiscussionTagAndGetId(handler, "Tag1");
    auto toTagId = createDiscussionTagAndGetId(handler, "Tag2");

    handlerToObj(handler, Forum::Commands::MERGE_DISCUSSION_TAG_INTO_OTHER_TAG, { fromTagId, toTagId });

    BOOST_REQUIRE_EQUAL(fromTagId, observedFromTagId);
    BOOST_REQUIRE_EQUAL(toTagId, observedToTagId);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_category_invokes_observer )
{
    std::string newCategoryName;
    auto handler = createCommandHandler();

    auto ___ = addHandler(handler->getWriteRepository()->writeEvents().onAddNewDiscussionCategory,
                          [&](auto& _, auto& newCategory) { newCategoryName = newCategory.name(); });

    createDiscussionCategoryAndGetId(handler, "Foo");
    BOOST_REQUIRE_EQUAL("Foo", newCategoryName);
}

BOOST_AUTO_TEST_CASE( Retrieving_discussion_categories_invokes_observer )
{
    int observerCalledNTimes = 0;
    auto handler = createCommandHandler();

    auto ___ = addHandler(handler->getReadRepository()->readEvents().onGetDiscussionCategories,
                          [&](auto& _) { observerCalledNTimes += 1; });

    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_CATEGORIES_BY_NAME, SortOrder::Ascending);
    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_CATEGORIES_BY_NAME, SortOrder::Descending);
    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_CATEGORIES_BY_MESSAGE_COUNT, SortOrder::Ascending);
    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_CATEGORIES_BY_MESSAGE_COUNT, SortOrder::Descending);

    BOOST_REQUIRE_EQUAL(4, observerCalledNTimes);
}

BOOST_AUTO_TEST_CASE(Retrieving_root_discussion_categories_invokes_observer)
{
    int observerCalledNTimes = 0;
    auto handler = createCommandHandler();

    auto ___ = addHandler(handler->getReadRepository()->readEvents().onGetRootDiscussionCategories,
        [&](auto& _) { observerCalledNTimes += 1; });

    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_CATEGORIES_FROM_ROOT);

    BOOST_REQUIRE_EQUAL(1, observerCalledNTimes);
}

BOOST_AUTO_TEST_CASE( Renaming_a_discussion_category_invokes_observer )
{
    std::string newName;
    auto categoryChange = DiscussionCategory::ChangeType::None;
    auto handler = createCommandHandler();

    auto ___ = addHandler(handler->getWriteRepository()->writeEvents().onChangeDiscussionCategory, 
                          [&](auto& _, auto& category, auto change)
                          {
                              newName = category.name();
                              categoryChange = change;
                          });

    auto categoryId = createDiscussionCategoryAndGetId(handler, "Abc");

    handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_CATEGORY_NAME, { categoryId, "Xyz" });
    BOOST_REQUIRE_EQUAL("Xyz", newName);
    BOOST_REQUIRE_EQUAL(DiscussionCategory::ChangeType::Name, categoryChange);
}

BOOST_AUTO_TEST_CASE( Changing_a_discussion_category_description_invokes_observer )
{
    std::string newDescription;
    auto categoryChange = DiscussionCategory::ChangeType::None;
    auto handler = createCommandHandler();

    auto ___ = addHandler(handler->getWriteRepository()->writeEvents().onChangeDiscussionCategory, 
                          [&](auto& _, auto& category, auto change)
                          {
                              newDescription = category.description();
                              categoryChange = change;
                          });

    auto categoryId = createDiscussionCategoryAndGetId(handler, "Abc");

    handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_CATEGORY_DESCRIPTION, { categoryId, "Xyz" });
    BOOST_REQUIRE_EQUAL("Xyz", newDescription);
    BOOST_REQUIRE_EQUAL(DiscussionCategory::ChangeType::Description, categoryChange);
}

BOOST_AUTO_TEST_CASE( Changing_a_discussion_category_displayOrder_invokes_observer )
{
    uint16_t newDisplayOrder;
    auto categoryChange = DiscussionCategory::ChangeType::None;
    auto handler = createCommandHandler();

    auto ___ = addHandler(handler->getWriteRepository()->writeEvents().onChangeDiscussionCategory,
                          [&](auto& _, auto& category, auto change)
                          {
                              newDisplayOrder = category.displayOrder();
                              categoryChange = change;
                          });

    auto categoryId = createDiscussionCategoryAndGetId(handler, "Abc");

    handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_CATEGORY_DISPLAY_ORDER, { categoryId, "100" });
    BOOST_REQUIRE_EQUAL(100, newDisplayOrder);
    BOOST_REQUIRE_EQUAL(DiscussionCategory::ChangeType::DisplayOrder, categoryChange);
}

BOOST_AUTO_TEST_CASE( Changing_a_discussion_category_parent_invokes_observer )
{
    std::string newParentId;
    auto categoryChange = DiscussionCategory::ChangeType::None;
    auto handler = createCommandHandler();

    auto ___ = addHandler(handler->getWriteRepository()->writeEvents().onChangeDiscussionCategory, 
                          [&](auto& _, auto& category, auto change)
                          {
                              if (auto parent = category.parentWeak().lock())
                              {
                                  newParentId = static_cast<std::string>(parent->id());
                              }
                              categoryChange = change;
                          });

    auto category1Id = createDiscussionCategoryAndGetId(handler, "Category1");
    auto childCategory1Id = createDiscussionCategoryAndGetId(handler, "ChildCategory1", category1Id);
    auto category2Id = createDiscussionCategoryAndGetId(handler, "Category2");

    handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_CATEGORY_PARENT, { childCategory1Id, category2Id });
    BOOST_REQUIRE_EQUAL(category2Id, newParentId);
    BOOST_REQUIRE_EQUAL(DiscussionCategory::ChangeType::Parent, categoryChange);
}

BOOST_AUTO_TEST_CASE( Deleting_a_discussion_category_invokes_observer )
{
    std::string deletedCategoryId;
    auto handler = createCommandHandler();

    auto ___ = addHandler(handler->getWriteRepository()->writeEvents().onDeleteDiscussionCategory, 
                          [&](auto& _, auto& tag)
                          {
                              deletedCategoryId = static_cast<std::string>(tag.id());
                          });

    auto categoryId = createDiscussionCategoryAndGetId(handler, "Abc");

    handlerToObj(handler, Forum::Commands::DELETE_DISCUSSION_CATEGORY, { categoryId });
    BOOST_REQUIRE_EQUAL(categoryId, deletedCategoryId);
}

BOOST_AUTO_TEST_CASE( Attaching_a_discussion_tag_to_a_category_invokes_observer )
{
    std::string observedTagId, observedCategoryId;
    auto handler = createCommandHandler();

    auto ___ = addHandler(handler->getWriteRepository()->writeEvents().onAddDiscussionTagToCategory, 
                          [&](auto& _, auto& tag, auto& category)
                          {
                              observedTagId = static_cast<std::string>(tag.id());
                              observedCategoryId = static_cast<std::string>(category.id());
                          });

    auto tagId = createDiscussionTagAndGetId(handler, "Tag");
    auto categoryId = createDiscussionCategoryAndGetId(handler, "Category");

    handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_TAG_TO_CATEGORY, { tagId, categoryId });

    BOOST_REQUIRE_EQUAL(tagId, observedTagId);
    BOOST_REQUIRE_EQUAL(categoryId, observedCategoryId);
}

BOOST_AUTO_TEST_CASE( Detaching_a_discussion_tag_from_a_category_invokes_observer )
{
    std::string observedTagId, observedCategoryId;
    auto handler = createCommandHandler();

    auto ___ = addHandler(handler->getWriteRepository()->writeEvents().onRemoveDiscussionTagFromCategory, 
                          [&](auto& _, auto& tag, auto& category)
                          {
                              observedTagId = static_cast<std::string>(tag.id());
                              observedCategoryId = static_cast<std::string>(category.id());
                          });

    auto tagId = createDiscussionTagAndGetId(handler, "Tag");
    auto categoryId = createDiscussionCategoryAndGetId(handler, "Category");

    handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_TAG_TO_CATEGORY, { tagId, categoryId });
    handlerToObj(handler, Forum::Commands::REMOVE_DISCUSSION_TAG_FROM_CATEGORY, { tagId, categoryId });

    BOOST_REQUIRE_EQUAL(tagId, observedTagId);
    BOOST_REQUIRE_EQUAL(categoryId, observedCategoryId);
}


BOOST_AUTO_TEST_CASE( Retrieving_discussion_threads_attached_to_categories_invokes_observer )
{
    int observerCalledNTimes = 0;
    std::string observedCategoryId;
    auto handler = createCommandHandler();

    auto ___ = addHandler(handler->getReadRepository()->readEvents().onGetDiscussionThreadsOfCategory,
                          [&](auto& _, auto& category)
                          {
                              observerCalledNTimes += 1;
                              observedCategoryId = static_cast<std::string>(category.id());
                          });

    auto categoryId = createDiscussionCategoryAndGetId(handler, "Category");

    Forum::Commands::Command commands[] =
    {
        Forum::Commands::GET_DISCUSSION_THREADS_OF_CATEGORY_BY_NAME,
        Forum::Commands::GET_DISCUSSION_THREADS_OF_CATEGORY_BY_CREATED,
        Forum::Commands::GET_DISCUSSION_THREADS_OF_CATEGORY_BY_LAST_UPDATED,
        Forum::Commands::GET_DISCUSSION_THREADS_OF_CATEGORY_BY_MESSAGE_COUNT
    };

    for (auto command : commands)
        for (auto sortOrder : { SortOrder::Ascending, SortOrder::Descending })
        {
            handlerToObj(handler, command, sortOrder, { categoryId });
        }

    BOOST_REQUIRE_EQUAL(8, observerCalledNTimes);
    BOOST_REQUIRE_EQUAL(categoryId, observedCategoryId);
}

BOOST_AUTO_TEST_CASE( Voting_discussion_thread_messages_invokes_observers )
{
    std::string upVoteUser, downVoteUser, resetVoteUser, votedMessages[3];

    auto handler = createCommandHandler();

    auto __1 = addHandler(handler->getWriteRepository()->writeEvents().onDiscussionThreadMessageUpVote,
                          [&](auto& context, auto& message)
                          {
                             upVoteUser = static_cast<std::string>(context.performedBy.id());
                             votedMessages[0] = static_cast<std::string>(message.id());
                          });
    auto __2 = addHandler(handler->getWriteRepository()->writeEvents().onDiscussionThreadMessageDownVote,
                          [&](auto& context, auto& message)
                          {
                             downVoteUser = static_cast<std::string>(context.performedBy.id());
                             votedMessages[1] = static_cast<std::string>(message.id());
                          });
    auto __3 = addHandler(handler->getWriteRepository()->writeEvents().onDiscussionThreadMessageResetVote,
                          [&](auto& context, auto& message)
                          {
                             resetVoteUser = static_cast<std::string>(context.performedBy.id());
                             votedMessages[2] = static_cast<std::string>(message.id());
                          });

    auto user1Id = createUserAndGetId(handler, "User1");
    auto user2Id = createUserAndGetId(handler, "User2");
    auto threadId = createDiscussionThreadAndGetId(handler, "Thread");
    auto messageId = createDiscussionMessageAndGetId(handler, threadId, "Message");
    
    {
        LoggedInUserChanger _(user1Id);
        handlerToObj(handler, Forum::Commands::UP_VOTE_DISCUSSION_THREAD_MESSAGE, { messageId });
    }
    {
        LoggedInUserChanger _(user2Id);
        handlerToObj(handler, Forum::Commands::DOWN_VOTE_DISCUSSION_THREAD_MESSAGE, { messageId });
    }
    {
        LoggedInUserChanger _(user1Id);
        handlerToObj(handler, Forum::Commands::RESET_VOTE_DISCUSSION_THREAD_MESSAGE, { messageId });
    }

    BOOST_REQUIRE_EQUAL(user1Id, upVoteUser);
    BOOST_REQUIRE_EQUAL(user2Id, downVoteUser);
    BOOST_REQUIRE_EQUAL(user1Id, resetVoteUser);

    for (auto& id : votedMessages)
    {
        BOOST_REQUIRE_EQUAL(messageId, id);
    }
}
