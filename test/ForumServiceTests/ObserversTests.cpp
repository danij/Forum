/*
Fast Forum Backend
Copyright (C) 2016-present Daniel Jurcau

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

    auto disconnector = addHandler(handler->readEvents().onGetEntitiesCount, 
                                   [&](auto&) { observerCalled = true; });
    (void)disconnector;

    handlerToObj(handler, Forum::Commands::COUNT_ENTITIES);
    BOOST_REQUIRE(observerCalled);
}

BOOST_AUTO_TEST_CASE( Retrieving_users_invokes_observer )
{
    auto observerCalledNTimes = 0u;
    auto handler = createCommandHandler();

    auto disconnector = addHandler(handler->readEvents().onGetUsers,
                                   [&](auto&) { observerCalledNTimes += 1; });
    (void)disconnector;

    Forum::Commands::View views[] =
    {
        Forum::Commands::GET_USERS_BY_NAME,
        Forum::Commands::GET_USERS_BY_CREATED,
        Forum::Commands::GET_USERS_BY_LAST_SEEN,
        Forum::Commands::GET_USERS_BY_MESSAGE_COUNT
    };

    SortOrder sortOrders[] = { SortOrder::Ascending, SortOrder::Descending };

    for (auto view : views)
    {
        for (auto sortOrder : sortOrders)
        {
            handlerToObj(handler, view, sortOrder);
        }
    }

    const auto nrOfCalls = std::size(views) * std::size(sortOrders);
    BOOST_REQUIRE_EQUAL(nrOfCalls, observerCalledNTimes);
}

BOOST_AUTO_TEST_CASE( Creating_a_user_invokes_observer )
{
    std::string newUserName;
    auto handler = createCommandHandler();

    auto disconnector = addHandler(handler->writeEvents().onAddNewUser,
                                   [&](auto&, auto& newUser) { newUserName = toString(newUser.name().string()); });
    (void)disconnector;

    createUserAndGetId(handler, "Foo");
    BOOST_REQUIRE_EQUAL("Foo", newUserName);
}

BOOST_AUTO_TEST_CASE( Retrieving_users_by_id_invokes_observer )
{
    std::string idToBeRetrieved;
    auto handler = createCommandHandler();

    auto disconnector = addHandler(handler->readEvents().onGetUserById,
                                   [&](auto&, auto& user) { idToBeRetrieved = user.id().toStringCompact(); });
    (void)disconnector;

    const auto userId = createUserAndGetId(handler, "User");
    handlerToObj(handler, Forum::Commands::GET_USER_BY_ID, { userId });
    BOOST_REQUIRE_EQUAL(userId, idToBeRetrieved);
}

BOOST_AUTO_TEST_CASE( Retrieving_users_by_name_invokes_observer )
{
    std::string nameToBeRetrieved;
    auto handler = createCommandHandler();

    auto disconnector = addHandler(handler->readEvents().onGetUserByName,
                                   [&](auto&, auto name) { nameToBeRetrieved = toString(name); });
    (void)disconnector;

    createUserAndGetId(handler, "SampleUser");
    handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "SampleUser" });
    BOOST_REQUIRE_EQUAL("SampleUser", nameToBeRetrieved);
}

BOOST_AUTO_TEST_CASE( Modifying_a_user_name_invokes_observer )
{
    std::string newName;
    auto userChange = User::ChangeType::None;
    auto handler = createCommandHandler();

    auto disconnector = addHandler(handler->writeEvents().onChangeUser,
                                   [&](auto&, auto& user, auto change)
                                   {
                                       newName = toString(user.name().string());
                                       userChange = change;
                                   });
    (void)disconnector;

    createUserAndGetId(handler, "Abc");
    auto user = handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "Abc" });
    const auto userId = user.get<std::string>("user.id");

    handlerToObj(handler, Forum::Commands::CHANGE_USER_NAME, { userId, "Xyz" });
    BOOST_REQUIRE_EQUAL("Xyz", newName);
    BOOST_REQUIRE_EQUAL(User::ChangeType::Name, userChange);
}

BOOST_AUTO_TEST_CASE( Modifying_a_user_info_invokes_observer )
{
    std::string newInfo;
    auto userChange = User::ChangeType::None;
    auto handler = createCommandHandler();

    auto disconnector = addHandler(handler->writeEvents().onChangeUser,
                                   [&](auto&, auto& user, auto change)
                                   {
                                       newInfo = toString(user.info().string());
                                       userChange = change;
                                   });
    (void)disconnector;

    const auto userId = createUserAndGetId(handler, "Abc");

    handlerToObj(handler, Forum::Commands::CHANGE_USER_INFO, { userId, "Hello World" });
    BOOST_REQUIRE_EQUAL("Hello World", newInfo);
    BOOST_REQUIRE_EQUAL(User::ChangeType::Info, userChange);
}

BOOST_AUTO_TEST_CASE( Deleting_a_user_invokes_observer )
{
    std::string deletedUserName;
    auto handler = createCommandHandler();

    auto disconnector = addHandler(handler->writeEvents().onDeleteUser,
                                   [&](auto&, auto& user) { deletedUserName = toString(user.name().string()); });
    (void)disconnector;

    createUserAndGetId(handler, "Abc");
    auto user = handlerToObj(handler, Forum::Commands::GET_USER_BY_NAME, { "Abc" });
    const auto userId = user.get<std::string>("user.id");

    handlerToObj(handler, Forum::Commands::DELETE_USER, { userId });
    BOOST_REQUIRE_EQUAL("Abc", deletedUserName);
}

BOOST_AUTO_TEST_CASE( Retrieving_discussion_threads_of_user_invokes_observer )
{
    int methodCalledNrTimes = 0;
    auto handler = createCommandHandler();

    auto disconnector = addHandler(handler->readEvents().onGetDiscussionThreadsOfUser,
                                   [&](auto&, auto&) { methodCalledNrTimes += 1; });
    (void)disconnector;

    const auto user1 = createUserAndGetId(handler, "User1");

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

    auto disconnector = addHandler(handler->readEvents().onGetDiscussionThreadMessagesOfUser,
                                   [&](auto&, auto&) { methodCalledNrTimes += 1; });
    (void)disconnector;

    const auto user1 = createUserAndGetId(handler, "User1");

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

    auto disconnector = addHandler(handler->readEvents().onGetDiscussionThreads,
                                   [&](auto&) { observerCalledNTimes += 1; });
    (void)disconnector;

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

    auto disconnector = addHandler(handler->readEvents().onGetDiscussionThreadById,
                                   [&](auto&, auto& thread, uint32_t)
                                   {
                                       idOfThread = thread.id().toStringCompact();
                                   });
    (void)disconnector;

    const auto threadId = createDiscussionThreadAndGetId(handler, "Thread");
    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_THREAD_BY_ID, { threadId });
    BOOST_REQUIRE_EQUAL(threadId, idOfThread);
}

BOOST_AUTO_TEST_CASE( Modifying_a_discussion_thread_invokes_observer )
{
    std::string newName;
    auto threadChange = DiscussionThread::ChangeType::None;
    auto handler = createCommandHandler();

    LoggedInUserChanger __(createUserAndGetId(handler, "User"));

    auto disconnector = addHandler(handler->writeEvents().onChangeDiscussionThread,
                                   [&](auto&, auto& thread, auto change)
                                   {
                                       newName = toString(thread.name().string());
                                       threadChange = change;
                                   });
    (void)disconnector;

    const auto threadId = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { "Abc" }).get<std::string>("id");

    handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_THREAD_NAME, { threadId, "Xyz" });
    BOOST_REQUIRE_EQUAL("Xyz", newName);
    BOOST_REQUIRE_EQUAL(DiscussionThread::ChangeType::Name, threadChange);
}

BOOST_AUTO_TEST_CASE( Deleting_a_discussion_thread_invokes_observer )
{
    std::string deletedThreadId;
    auto handler = createCommandHandler();

    auto disconnector = addHandler(handler->writeEvents().onDeleteDiscussionThread,
                                   [&](auto&, auto& thread) { deletedThreadId = thread.id().toStringCompact(); });
    (void)disconnector;

    const auto threadId = handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_THREAD, { "Abc" }).get<std::string>("id");

    handlerToObj(handler, Forum::Commands::DELETE_DISCUSSION_THREAD, { threadId });
    BOOST_REQUIRE_EQUAL(threadId, deletedThreadId);
}

BOOST_AUTO_TEST_CASE( Merging_discussion_threads_invokes_observer )
{
    std::string observedFromThreadId, observedToThreadId;
    auto handler = createCommandHandler();

    auto disconnector = addHandler(handler->writeEvents().onMergeDiscussionThreads,
                                   [&](auto&, auto& fromThread, auto& toThread)
                                   {
                                       observedFromThreadId = fromThread.id().toStringCompact();
                                       observedToThreadId = toThread.id().toStringCompact();
                                   });
    (void)disconnector;

    const auto fromThreadId = createDiscussionThreadAndGetId(handler, "Thread1");
    const auto toThreadId = createDiscussionThreadAndGetId(handler, "Thread2");

    handlerToObj(handler, Forum::Commands::MERGE_DISCUSSION_THREADS, { fromThreadId, toThreadId });

    BOOST_REQUIRE_EQUAL(fromThreadId, observedFromThreadId);
    BOOST_REQUIRE_EQUAL(toThreadId, observedToThreadId);
}

BOOST_AUTO_TEST_CASE( Moving_discussion_thread_messages_invokes_observer )
{
    std::string observedMessageId, observedToThreadId;
    auto handler = createCommandHandler();

    auto disconnector = addHandler(handler->writeEvents().onMoveDiscussionThreadMessage,
                                   [&](auto&, auto& message, auto& intoThread)
                                   {
                                       observedMessageId = message.id().toStringCompact();
                                       observedToThreadId = intoThread.id().toStringCompact();
                                   });
    (void)disconnector;

    const auto fromThreadId = createDiscussionThreadAndGetId(handler, "Thread1");
    const auto messageId = createDiscussionMessageAndGetId(handler, fromThreadId, "Message1");
    const auto toThreadId = createDiscussionThreadAndGetId(handler, "Thread2");

    handlerToObj(handler, Forum::Commands::MOVE_DISCUSSION_THREAD_MESSAGE, { messageId, toThreadId });

    BOOST_REQUIRE_EQUAL(messageId, observedMessageId);
    BOOST_REQUIRE_EQUAL(toThreadId, observedToThreadId);
}

BOOST_AUTO_TEST_CASE( Modifying_the_content_of_a_discussion_message_invokes_observer )
{
    std::string newContent, changeReason;
    auto messageChange = DiscussionThreadMessage::ChangeType::None;
    auto handler = createCommandHandler();

    auto disconnector = addHandler(handler->writeEvents().onChangeDiscussionThreadMessage,
                                   [&](auto&, auto& message, auto change)
                                   {
                                       newContent = toString(message.content());
                                       changeReason = toString(message.lastUpdatedReason());
                                       messageChange = change;
                                   });
    (void)disconnector;

    const auto userId = createUserAndGetId(handler, "User");
    LoggedInUserChanger _(userId);

    const auto threadId = createDiscussionThreadAndGetId(handler, "Abc");
    const auto messageId = createDiscussionMessageAndGetId(handler, threadId, "Message");

    handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT, { messageId, "New Message", "Reason" });
    BOOST_REQUIRE_EQUAL("New Message", newContent);
    BOOST_REQUIRE_EQUAL("Reason", changeReason);
    BOOST_REQUIRE_EQUAL(DiscussionThreadMessage::ChangeType::Content, messageChange);
}

BOOST_AUTO_TEST_CASE( Deleting_a_discussion_message_invokes_observer )
{
    std::string deletedMessageId;
    auto handler = createCommandHandler();

    auto disconnector = addHandler(handler->writeEvents().onDeleteDiscussionThreadMessage,
                                   [&](auto&, auto& message) { deletedMessageId = message.id().toStringCompact(); });
    (void)disconnector;

    const auto threadId = createDiscussionThreadAndGetId(handler, "Abc");
    const auto messageId = createDiscussionMessageAndGetId(handler, threadId, "aaaaaaaaaaa");

    handlerToObj(handler, Forum::Commands::DELETE_DISCUSSION_THREAD_MESSAGE, { messageId });
    BOOST_REQUIRE_EQUAL(messageId, deletedMessageId);
}

BOOST_AUTO_TEST_CASE( Observer_context_includes_user_that_performs_the_action )
{
    auto handler = createCommandHandler();
    std::string userIdFromContext;
    std::string userNameFromContext;

    auto disconnector = addHandler(handler->readEvents().onGetEntitiesCount,
                                   [&](auto& context)
                                   {
                                       userIdFromContext = context.performedBy.id().toStringCompact();
                                       userNameFromContext = toString(context.performedBy.name().string());
                                   });
    (void)disconnector;

    const auto user1 = createUserAndGetId(handler, "User1");
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

    auto disconnector = addHandler(handler->readEvents().onGetEntitiesCount,
                                   [&](auto& context)
                                   {
                                       userIdFromContext = context.performedBy.id().toStringCompact();
                                       userNameFromContext = toString(context.performedBy.name().string());
                                   });
    (void)disconnector;

    handlerToObj(handler, Forum::Commands::COUNT_ENTITIES);

    BOOST_REQUIRE_EQUAL(UuidString::empty.toStringCompact(), userIdFromContext);
    BOOST_REQUIRE_EQUAL("<anonymous>", userNameFromContext);
}

BOOST_AUTO_TEST_CASE( Observer_context_includes_timestamp_of_action )
{
    auto handler = createCommandHandler();
    const Timestamp timestamp = 1000;
    Timestamp timestampFromContext = 0;

    auto disconnector = addHandler(handler->readEvents().onGetEntitiesCount,
                                   [&](auto& context) { timestampFromContext = context.timestamp; });
    (void)disconnector;
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

    auto disconnector = addHandler(handler->writeEvents().onAddNewDiscussionTag,
                                   [&](auto&, auto& newTag) { newTagName = toString(newTag.name().string()); });
    (void)disconnector;

    createDiscussionTagAndGetId(handler, "Foo");
    BOOST_REQUIRE_EQUAL("Foo", newTagName);
}

BOOST_AUTO_TEST_CASE( Retrieving_discussion_tags_invokes_observer )
{
    int observerCalledNTimes = 0;
    auto handler = createCommandHandler();

    auto disconnector = addHandler(handler->readEvents().onGetDiscussionTags,
                                   [&](auto&) { observerCalledNTimes += 1; });
    (void)disconnector;

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

    auto disconnector = addHandler(handler->writeEvents().onChangeDiscussionTag,
                                   [&](auto&, auto& tag, auto change)
                                   {
                                       newName = toString(tag.name().string());
                                       tagChange = change;
                                   });
    (void)disconnector;

    const auto tagId = createDiscussionTagAndGetId(handler, "Abc");

    handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_TAG_NAME, { tagId, "Xyz" });
    BOOST_REQUIRE_EQUAL("Xyz", newName);
    BOOST_REQUIRE_EQUAL(DiscussionTag::ChangeType::Name, tagChange);
}

BOOST_AUTO_TEST_CASE( Changing_a_discussion_tag_ui_blob_invokes_observer )
{
    std::string newBlob;
    auto tagChange = DiscussionTag::ChangeType::None;
    auto handler = createCommandHandler();

    auto disconnector = addHandler(handler->writeEvents().onChangeDiscussionTag,
                                   [&](auto&, auto& tag, auto change)
                                   {
                                       newBlob = toString(tag.uiBlob());
                                       tagChange = change;
                                   });
    (void)disconnector;

    const auto tagId = createDiscussionTagAndGetId(handler, "Abc");

    handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_TAG_UI_BLOB, { tagId, "sample blob" });
    BOOST_REQUIRE_EQUAL("sample blob", newBlob);
    BOOST_REQUIRE_EQUAL(DiscussionTag::ChangeType::UIBlob, tagChange);
}

BOOST_AUTO_TEST_CASE( Deleting_a_discussion_tag_invokes_observer )
{
    std::string deletedTagId;
    auto handler = createCommandHandler();

    auto disconnector = addHandler(handler->writeEvents().onDeleteDiscussionTag,
                                   [&](auto&, auto& tag)
                                   {
                                       deletedTagId = tag.id().toStringCompact();
                                   });
    (void)disconnector;

    const auto tagId = createDiscussionTagAndGetId(handler, "Abc");

    handlerToObj(handler, Forum::Commands::DELETE_DISCUSSION_TAG, { tagId });
    BOOST_REQUIRE_EQUAL(tagId, deletedTagId);
}

BOOST_AUTO_TEST_CASE( Attaching_a_discussion_tag_to_a_thread_invokes_observer )
{
    std::string observedTagId, observedThreadId;
    auto handler = createCommandHandler();

    auto disconnector = addHandler(handler->writeEvents().onAddDiscussionTagToThread,
                                   [&](auto&, auto& tag, auto& thread)
                                   {
                                       observedTagId = tag.id().toStringCompact();
                                       observedThreadId = thread.id().toStringCompact();
                                   });
    (void)disconnector;

    const auto tagId = createDiscussionTagAndGetId(handler, "Tag");
    const auto threadId = createDiscussionThreadAndGetId(handler, "Thread");

    handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_TAG_TO_THREAD, { tagId, threadId });

    BOOST_REQUIRE_EQUAL(tagId, observedTagId);
    BOOST_REQUIRE_EQUAL(threadId, observedThreadId);
}

BOOST_AUTO_TEST_CASE( Detaching_a_discussion_tag_from_a_thread_invokes_observer )
{
    std::string observedTagId, observedThreadId;
    auto handler = createCommandHandler();

    auto disconnector = addHandler(handler->writeEvents().onRemoveDiscussionTagFromThread,
                                   [&](auto&, auto& tag, auto& thread)
                                   {
                                       observedTagId = tag.id().toStringCompact();
                                       observedThreadId = thread.id().toStringCompact();
                                   });
    (void)disconnector;

    const auto tagId = createDiscussionTagAndGetId(handler, "Tag");
    const auto threadId = createDiscussionThreadAndGetId(handler, "Thread");

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

    auto disconnector = addHandler(handler->readEvents().onGetDiscussionThreadsWithTag,
                                   [&](auto&, auto& tag)
                                   {
                                       observerCalledNTimes += 1;
                                       observedTagId = tag.id().toStringCompact();
                                   });
    (void)disconnector;

    const auto tagId = createDiscussionTagAndGetId(handler, "Tag");

    Forum::Commands::View views[] =
    {
        Forum::Commands::GET_DISCUSSION_THREADS_WITH_TAG_BY_NAME,
        Forum::Commands::GET_DISCUSSION_THREADS_WITH_TAG_BY_CREATED,
        Forum::Commands::GET_DISCUSSION_THREADS_WITH_TAG_BY_LAST_UPDATED,
        Forum::Commands::GET_DISCUSSION_THREADS_WITH_TAG_BY_MESSAGE_COUNT
    };

    for (auto view : views)
        for (auto sortOrder : { SortOrder::Ascending, SortOrder::Descending })
        {
            handlerToObj(handler, view, sortOrder, { tagId });
        }

    BOOST_REQUIRE_EQUAL(8, observerCalledNTimes);
    BOOST_REQUIRE_EQUAL(tagId, observedTagId);
}

BOOST_AUTO_TEST_CASE( Merging_discussion_tags_invokes_observer)
{
    std::string observedFromTagId, observedToTagId;
    auto handler = createCommandHandler();

    auto disconnector = addHandler(handler->writeEvents().onMergeDiscussionTags,
                                   [&](auto&, auto& fromTag, auto& toTag)
                                   {
                                       observedFromTagId = fromTag.id().toStringCompact();
                                       observedToTagId = toTag.id().toStringCompact();
                                   });
    (void)disconnector;

    const auto fromTagId = createDiscussionTagAndGetId(handler, "Tag1");
    const auto toTagId = createDiscussionTagAndGetId(handler, "Tag2");

    handlerToObj(handler, Forum::Commands::MERGE_DISCUSSION_TAG_INTO_OTHER_TAG, { fromTagId, toTagId });

    BOOST_REQUIRE_EQUAL(fromTagId, observedFromTagId);
    BOOST_REQUIRE_EQUAL(toTagId, observedToTagId);
}

BOOST_AUTO_TEST_CASE( Creating_a_discussion_category_invokes_observer )
{
    std::string newCategoryName;
    auto handler = createCommandHandler();

    auto disconnector = addHandler(handler->writeEvents().onAddNewDiscussionCategory,
                                   [&](auto&, auto& newCategory) { newCategoryName = toString(newCategory.name().string()); });
    (void)disconnector;

    createDiscussionCategoryAndGetId(handler, "Foo");
    BOOST_REQUIRE_EQUAL("Foo", newCategoryName);
}

BOOST_AUTO_TEST_CASE( Retrieving_discussion_categories_invokes_observer )
{
    int observerCalledNTimes = 0;
    auto handler = createCommandHandler();

    auto disconnector = addHandler(handler->readEvents().onGetDiscussionCategories,
                                   [&](auto&) { observerCalledNTimes += 1; });
    (void)disconnector;

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

    auto disconnector = addHandler(handler->readEvents().onGetRootDiscussionCategories,
                                   [&](auto&) { observerCalledNTimes += 1; });
    (void)disconnector;

    handlerToObj(handler, Forum::Commands::GET_DISCUSSION_CATEGORIES_FROM_ROOT);

    BOOST_REQUIRE_EQUAL(1, observerCalledNTimes);
}

BOOST_AUTO_TEST_CASE( Renaming_a_discussion_category_invokes_observer )
{
    std::string newName;
    auto categoryChange = DiscussionCategory::ChangeType::None;
    auto handler = createCommandHandler();

    auto disconnector = addHandler(handler->writeEvents().onChangeDiscussionCategory,
                                   [&](auto&, auto& category, auto change)
                                   {
                                       newName = toString(category.name().string());
                                       categoryChange = change;
                                   });
    (void)disconnector;

    const auto categoryId = createDiscussionCategoryAndGetId(handler, "Abc");

    handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_CATEGORY_NAME, { categoryId, "Xyz" });
    BOOST_REQUIRE_EQUAL("Xyz", newName);
    BOOST_REQUIRE_EQUAL(DiscussionCategory::ChangeType::Name, categoryChange);
}

BOOST_AUTO_TEST_CASE( Changing_a_discussion_category_description_invokes_observer )
{
    std::string newDescription;
    auto categoryChange = DiscussionCategory::ChangeType::None;
    auto handler = createCommandHandler();

    auto disconnector = addHandler(handler->writeEvents().onChangeDiscussionCategory,
                                   [&](auto&, auto& category, auto change)
                                   {
                                       newDescription = toString(category.description());
                                       categoryChange = change;
                                   });
    (void)disconnector;

    const auto categoryId = createDiscussionCategoryAndGetId(handler, "Abc");

    handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_CATEGORY_DESCRIPTION, { categoryId, "Xyz" });
    BOOST_REQUIRE_EQUAL("Xyz", newDescription);
    BOOST_REQUIRE_EQUAL(DiscussionCategory::ChangeType::Description, categoryChange);
}

BOOST_AUTO_TEST_CASE( Changing_a_discussion_category_displayOrder_invokes_observer )
{
    uint16_t newDisplayOrder;
    auto categoryChange = DiscussionCategory::ChangeType::None;
    auto handler = createCommandHandler();

    auto disconnector = addHandler(handler->writeEvents().onChangeDiscussionCategory,
                                   [&](auto&, auto& category, auto change)
                                   {
                                       newDisplayOrder = category.displayOrder();
                                       categoryChange = change;
                                   });
    (void)disconnector;

    const auto categoryId = createDiscussionCategoryAndGetId(handler, "Abc");

    handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_CATEGORY_DISPLAY_ORDER, { categoryId, "100" });
    BOOST_REQUIRE_EQUAL(100, newDisplayOrder);
    BOOST_REQUIRE_EQUAL(DiscussionCategory::ChangeType::DisplayOrder, categoryChange);
}

BOOST_AUTO_TEST_CASE( Changing_a_discussion_category_parent_invokes_observer )
{
    std::string newParentId;
    auto categoryChange = DiscussionCategory::ChangeType::None;
    auto handler = createCommandHandler();

    auto disconnector = addHandler(handler->writeEvents().onChangeDiscussionCategory,
                                   [&](auto&, auto& category, auto change)
                                   {
                                       if (auto parent = category.parent())
                                       {
                                           newParentId = parent->id().toStringCompact();
                                       }
                                       categoryChange = change;
                                   });
    (void)disconnector;

    const auto category1Id = createDiscussionCategoryAndGetId(handler, "Category1");
    const auto childCategory1Id = createDiscussionCategoryAndGetId(handler, "ChildCategory1", category1Id);
    const auto category2Id = createDiscussionCategoryAndGetId(handler, "Category2");

    handlerToObj(handler, Forum::Commands::CHANGE_DISCUSSION_CATEGORY_PARENT, { childCategory1Id, category2Id });
    BOOST_REQUIRE_EQUAL(category2Id, newParentId);
    BOOST_REQUIRE_EQUAL(DiscussionCategory::ChangeType::Parent, categoryChange);
}

BOOST_AUTO_TEST_CASE( Deleting_a_discussion_category_invokes_observer )
{
    std::string deletedCategoryId;
    auto handler = createCommandHandler();

    auto disconnector = addHandler(handler->writeEvents().onDeleteDiscussionCategory,
                                   [&](auto&, auto& tag)
                                   {
                                       deletedCategoryId = tag.id().toStringCompact();
                                   });
    (void)disconnector;

    const auto categoryId = createDiscussionCategoryAndGetId(handler, "Abc");

    handlerToObj(handler, Forum::Commands::DELETE_DISCUSSION_CATEGORY, { categoryId });
    BOOST_REQUIRE_EQUAL(categoryId, deletedCategoryId);
}

BOOST_AUTO_TEST_CASE( Attaching_a_discussion_tag_to_a_category_invokes_observer )
{
    std::string observedTagId, observedCategoryId;
    auto handler = createCommandHandler();

    auto disconnector = addHandler(handler->writeEvents().onAddDiscussionTagToCategory,
                                   [&](auto&, auto& tag, auto& category)
                                   {
                                       observedTagId = tag.id().toStringCompact();
                                       observedCategoryId = category.id().toStringCompact();
                                   });
    (void)disconnector;

    const auto tagId = createDiscussionTagAndGetId(handler, "Tag");
    const auto categoryId = createDiscussionCategoryAndGetId(handler, "Category");

    handlerToObj(handler, Forum::Commands::ADD_DISCUSSION_TAG_TO_CATEGORY, { tagId, categoryId });

    BOOST_REQUIRE_EQUAL(tagId, observedTagId);
    BOOST_REQUIRE_EQUAL(categoryId, observedCategoryId);
}

BOOST_AUTO_TEST_CASE( Detaching_a_discussion_tag_from_a_category_invokes_observer )
{
    std::string observedTagId, observedCategoryId;
    auto handler = createCommandHandler();

    auto disconnector = addHandler(handler->writeEvents().onRemoveDiscussionTagFromCategory,
                                   [&](auto&, auto& tag, auto& category)
                                   {
                                       observedTagId = tag.id().toStringCompact();
                                       observedCategoryId = category.id().toStringCompact();
                                   });
    (void)disconnector;

    const auto tagId = createDiscussionTagAndGetId(handler, "Tag");
    const auto categoryId = createDiscussionCategoryAndGetId(handler, "Category");

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

    auto disconnector = addHandler(handler->readEvents().onGetDiscussionThreadsOfCategory,
                                   [&](auto&, auto& category)
                                   {
                                       observerCalledNTimes += 1;
                                       observedCategoryId = category.id().toStringCompact();
                                   });
    (void)disconnector;

    const auto categoryId = createDiscussionCategoryAndGetId(handler, "Category");

    Forum::Commands::View views[] =
    {
        Forum::Commands::GET_DISCUSSION_THREADS_OF_CATEGORY_BY_NAME,
        Forum::Commands::GET_DISCUSSION_THREADS_OF_CATEGORY_BY_CREATED,
        Forum::Commands::GET_DISCUSSION_THREADS_OF_CATEGORY_BY_LAST_UPDATED,
        Forum::Commands::GET_DISCUSSION_THREADS_OF_CATEGORY_BY_MESSAGE_COUNT
    };

    for (auto view : views)
        for (auto sortOrder : { SortOrder::Ascending, SortOrder::Descending })
        {
            handlerToObj(handler, view, sortOrder, { categoryId });
        }

    BOOST_REQUIRE_EQUAL(8, observerCalledNTimes);
    BOOST_REQUIRE_EQUAL(categoryId, observedCategoryId);
}

BOOST_AUTO_TEST_CASE( Voting_discussion_thread_messages_invokes_observers )
{
    std::string upVoteUser, downVoteUser, resetVoteUser, votedMessages[3];

    auto handler = createCommandHandler();

    auto disconnector = addHandler(handler->writeEvents().onDiscussionThreadMessageUpVote,
                                   [&](auto& context, auto& message)
                                   {
                                      upVoteUser = context.performedBy.id().toStringCompact();
                                      votedMessages[0] = message.id().toStringCompact();
                                   });
    (void)disconnector;
    auto disconnector_ = addHandler(handler->writeEvents().onDiscussionThreadMessageDownVote,
                                    [&](auto& context, auto& message)
                                    {
                                       downVoteUser = context.performedBy.id().toStringCompact();
                                       votedMessages[1] = message.id().toStringCompact();
                                    });
    (void)disconnector_;
    auto disconnector__ = addHandler(handler->writeEvents().onDiscussionThreadMessageResetVote,
                                     [&](auto& context, auto& message)
                                     {
                                        resetVoteUser = context.performedBy.id().toStringCompact();
                                        votedMessages[2] = message.id().toStringCompact();
                                     });
    (void)disconnector__;

    const auto user1Id = createUserAndGetId(handler, "User1");
    const auto user2Id = createUserAndGetId(handler, "User2");
    const auto user3Id = createUserAndGetId(handler, "User3");
    const auto threadId = createDiscussionThreadAndGetId(handler, "Thread");
    const auto messageId = createDiscussionMessageAndGetId(handler, threadId, "Message");

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

//BOOST_AUTO_TEST_CASE( Adding_comments_to_discussion_messages_invokes_observer )
//BOOST_AUTO_TEST_CASE( Setting_discussion_message_comments_to_solved_invokes_observer )
//BOOST_AUTO_TEST_CASE( Retrieving_discussion_message_comments_invokes_observer )
//BOOST_AUTO_TEST_CASE( Retrieving_comments_of_a_specific_discussion_message_invokes_observer )
