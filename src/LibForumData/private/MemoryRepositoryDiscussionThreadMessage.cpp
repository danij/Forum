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

#include "MemoryRepositoryDiscussionThreadMessage.h"

#include "Configuration.h"
#include "ContextProviders.h"
#include "EntitySerialization.h"
#include "OutputHelpers.h"
#include "RandomGenerator.h"
#include "StateHelpers.h"
#include "StringHelpers.h"
#include "Logging.h"

#include <boost/thread/tss.hpp>

using namespace Forum;
using namespace Forum::Configuration;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;
using namespace Forum::Authorization;

MemoryRepositoryDiscussionThreadMessage::MemoryRepositoryDiscussionThreadMessage(MemoryStoreRef store,
                                                                                 DiscussionThreadMessageAuthorizationRef authorization)
    : MemoryRepositoryBase(std::move(store)), authorization_(std::move(authorization))
{
    if ( ! authorization_)
    {
        throw std::runtime_error("Authorization implementation not provided");
    }
}

StatusCode MemoryRepositoryDiscussionThreadMessage::getMultipleDiscussionThreadMessagesById(StringView ids,
                                                                                            OutStream& output) const
{
    StatusWriter status(output);
    PerformedByWithLastSeenUpdateGuard performedBy;

    constexpr size_t MaxIdBuffer = 64;
    static boost::thread_specific_ptr<std::array<UuidString, MaxIdBuffer>> parsedIdsPtr;
    static boost::thread_specific_ptr<std::array<const DiscussionThreadMessage*, MaxIdBuffer>> threadMessagesFoundPtr;

    if ( ! parsedIdsPtr.get())
    {
        parsedIdsPtr.reset(new std::array<UuidString, MaxIdBuffer>);
    }
    auto& parsedIds = *parsedIdsPtr;
    if ( ! threadMessagesFoundPtr.get())
    {
        threadMessagesFoundPtr.reset(new std::array<const DiscussionThreadMessage*, MaxIdBuffer>);
    }
    auto& threadMessagesFound = *threadMessagesFoundPtr;

    const auto maxThreadsToSearch = std::min(MaxIdBuffer, 
                                             static_cast<size_t>(getGlobalConfig()->discussionThreadMessage.maxMessagesPerPage));
    auto lastParsedId = parseMultipleUuidStrings(ids, parsedIds.begin(), parsedIds.begin() + maxThreadsToSearch);

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);
                          
                          const auto& indexById = collection.threadMessages().byId();
                          auto lastThreadMessageFound = std::transform(parsedIds.begin(), lastParsedId, 
                                  threadMessagesFound.begin(), 
                                  [&indexById](auto id)
                                  {
                                      auto it = indexById.find(id);
                                      return (it == indexById.end()) ? nullptr : *it;
                                  });
                          
                          status = StatusCode::OK;
                          status.disable();

                          SerializationRestriction restriction(collection.grantedPrivileges(), collection,
                                                               currentUser.id(), Context::getCurrentTime());

                          TemporaryChanger<UserPtr> _(serializationSettings.userToCheckVotesOf, currentUser.pointer());

                          writeAllEntities(threadMessagesFound.begin(), lastThreadMessageFound,
                                           "thread_messages", output, restriction);
                          
                          readEvents().onGetMultipleDiscussionThreadMessagesById(createObserverContext(currentUser), ids);
                      });
    return status;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::getDiscussionThreadMessagesOfUserByCreated(IdTypeRef id,
                                                                                               OutStream& output) const
{
    StatusWriter status(output);
    PerformedByWithLastSeenUpdateGuard performedBy;

    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    collection().read([&](const EntityCollection& collection)
    {
        auto& currentUser = performedBy.get(collection, *store_);

        const auto& indexById = collection.users().byId();
        auto it = indexById.find(id);
        if (it == indexById.end())
        {
            status = StatusCode::NOT_FOUND;
            return;
        }

        if ( ! (status = authorization_->getDiscussionThreadMessagesOfUserByCreated(currentUser, **it)))
        {
            return;
        }

        const User* user = *it;

        const auto& messages = user->threadMessages().byCreated();
        BoolTemporaryChanger _(serializationSettings.hideDiscussionThreadMessageCreatedBy, true);
        BoolTemporaryChanger __(serializationSettings.hideDiscussionThreadMessages, true);
        BoolTemporaryChanger ___(serializationSettings.hideLatestMessage, true);
        TemporaryChanger<UserPtr> ____(serializationSettings.userToCheckVotesOf, currentUser.pointer());

        auto pageSize = getGlobalConfig()->discussionThreadMessage.maxMessagesPerPage;
        auto& displayContext = Context::getDisplayContext();

        status.disable();

        SerializationRestriction restriction(collection.grantedPrivileges(), collection, 
                                             currentUser.id(), Context::getCurrentTime());

        writeEntitiesWithPagination(messages, "messages", output, displayContext.pageNumber, pageSize,
            displayContext.sortOrder == Context::SortOrder::Ascending, restriction);

        readEvents().onGetDiscussionThreadMessagesOfUser(createObserverContext(currentUser), **it);
    });
    return status;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::getLatestDiscussionThreadMessages(OutStream& output) const
{
    StatusWriter status(output);
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
    {
        auto& currentUser = performedBy.get(collection, *store_);

        const auto& messages = collection.threadMessages().byCreated();

        BoolTemporaryChanger _(serializationSettings.hideDiscussionThreadMessages, true);
        BoolTemporaryChanger __(serializationSettings.hideLatestMessage, true);
        TemporaryChanger<UserPtr> ___(serializationSettings.userToCheckVotesOf, currentUser.pointer());

        auto pageSize = getGlobalConfig()->discussionThreadMessage.maxMessagesPerPage;
        auto& displayContext = Context::getDisplayContext();

        status = StatusCode::OK;
        status.disable();

        SerializationRestriction restriction(collection.grantedPrivileges(), collection,
                                             currentUser.id(), Context::getCurrentTime());

        writeEntitiesWithPagination(messages, "messages", output, displayContext.pageNumber, pageSize, false, restriction);

        readEvents().onGetLatestDiscussionThreadMessages(createObserverContext(currentUser));
    });
    return status;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::getDiscussionThreadMessageRank(IdTypeRef id, OutStream& output) const
{
    StatusWriter status(output);
    PerformedByWithLastSeenUpdateGuard performedBy;

    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);

                          const auto& indexById = collection.threadMessages().byId();
                          auto it = indexById.find(id);
                          if (it == indexById.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }

                          const DiscussionThreadMessage& message = **it;

                          if ( ! (status = authorization_->getDiscussionThreadMessageRank(currentUser, message)))
                          {
                              return;
                          }

                          assert(message.parentThread());
                          const DiscussionThread& parentThread = *message.parentThread();

                          auto rank = parentThread.messages().findRankByCreated(message.id());
                          if ( ! rank)
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }
                          auto pageSize = getGlobalConfig()->discussionThreadMessage.maxMessagesPerPage;

                          status.writeNow([&](auto& writer)
                                          {
                                              writer << Json::propertySafeName("id", message.id());
                                              writer << Json::propertySafeName("parentId", parentThread.id());
                                              writer << Json::propertySafeName("rank", *rank);
                                              writer << Json::propertySafeName("pageSize", pageSize);
                                          });

                          readEvents().onGetDiscussionThreadMessageRank(createObserverContext(currentUser), message);
                      });
    return status;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::addNewDiscussionMessageInThread(IdTypeRef threadId,
                                                                                    StringView content,
                                                                                    OutStream& output)
{
    StatusWriter status(output);
    if ( ! threadId)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    const auto config = getGlobalConfig();
    const auto validationCode = validateString(content, INVALID_PARAMETERS_FOR_EMPTY_STRING,
                                               config->discussionThreadMessage.minContentLength,
                                               config->discussionThreadMessage.maxContentLength,
                                               &MemoryRepositoryBase::doesNotContainLeadingOrTrailingWhitespace);

    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto threadPtr = collection.threads().findById(threadId);
                           if ( ! threadPtr)
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           auto& thread = *threadPtr;

                           if ( ! (status = authorization_->addNewDiscussionMessageInThread(*currentUser, thread, content)))
                           {
                               return;
                           }

                           auto& user = *currentUser;
                           auto alreadySubscribed = user.subscribedThreads().contains(threadPtr);

                           auto statusWithResource = addNewDiscussionMessageInThread(collection, generateUniqueId(),
                                                                                     threadId, content);
                           auto& message = statusWithResource.resource;
                           if ( ! (status = statusWithResource.status)) return;

                           writeEvents().onAddNewDiscussionThreadMessage(createObserverContext(user), *message);
                           if ( ! alreadySubscribed)
                           {
                               writeEvents().onSubscribeToDiscussionThread(createObserverContext(user), thread);
                           }

                           status.writeNow([&](auto& writer)
                                           {
                                               writer << Json::propertySafeName("id", message->id());
                                               writer << Json::propertySafeName("parentId", thread.id());
                                               writer << Json::propertySafeName("created", message->created());
                                           });
                       });
    return status;
}

StatusWithResource<DiscussionThreadMessagePtr>
    MemoryRepositoryDiscussionThreadMessage::addNewDiscussionMessageInThread(EntityCollection& collection,
                                                                             IdTypeRef messageId, IdTypeRef threadId,
                                                                             StringView content)
{
    return addNewDiscussionMessageInThread(collection, messageId, threadId, content, 0, 0);
}

StatusWithResource<DiscussionThreadMessagePtr>
    MemoryRepositoryDiscussionThreadMessage::addNewDiscussionMessageInThread(EntityCollection& collection,
                                                                             IdTypeRef messageId, IdTypeRef threadId,
                                                                             size_t contentSize, size_t contentOffset)
{
    return addNewDiscussionMessageInThread(collection, messageId, threadId, {}, contentSize, contentOffset);
}

StatusWithResource<DiscussionThreadMessagePtr>
    MemoryRepositoryDiscussionThreadMessage::addNewDiscussionMessageInThread(EntityCollection& collection,
                                                                             IdTypeRef messageId, IdTypeRef threadId,
                                                                             StringView content, size_t contentSize,
                                                                             size_t contentOffset)
{
    auto threadPtr = collection.threads().findById(threadId);
    if ( ! threadPtr)
    {
        FORUM_LOG_ERROR << "Could not find discussion thread: " << static_cast<std::string>(threadId);
        return StatusCode::NOT_FOUND;
    }

    auto currentUser = getCurrentUser(collection);

    auto message = collection.createDiscussionThreadMessage(messageId, *currentUser, Context::getCurrentTime(),
                                                            { Context::getCurrentUserIpAddress() });
    message->parentThread() = threadPtr;
    if ((contentSize > 0) && (contentOffset > 0))
    {
        auto messageContent = collection.getMessageContentPointer(contentOffset, contentSize);
        if (messageContent.empty())
        {
            FORUM_LOG_ERROR << "Could not find message at offset " << contentOffset << " with length " << contentSize;
            return StatusCode::INVALID_PARAMETERS;
        }
        message->content() = WholeChangeableString::onlyTakePointer(messageContent);
    }
    else
    {
        message->content() = WholeChangeableString::copyFrom(content);
    }
    collection.insertDiscussionThreadMessage(message);

    DiscussionThread& thread = *threadPtr;

    thread.insertMessage(message);
    thread.resetVisitorsSinceLastEdit();
    thread.latestVisibleChange() = message->created();

    if (anonymousUser() != currentUser)
    {
        thread.subscribedUsers().insert(std::make_pair(currentUser->id(), currentUser));
    }

    for (DiscussionTagPtr tag : thread.tags())
    {
        assert(tag);
        tag->updateMessageCount(1);
    }
    for (DiscussionCategoryPtr category : thread.categories())
    {
        assert(category);
        category->updateMessageCount(threadPtr, 1);
    }

    currentUser->threadMessages().add(message);
    currentUser->subscribedThreads().add(threadPtr);

    return message;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::deleteDiscussionMessage(IdTypeRef id, OutStream& output)
{
    StatusWriter status(output);
    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.threadMessages().byId();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           if ( ! (status = authorization_->deleteDiscussionMessage(*currentUser, **it)))
                           {
                               return;
                           }

                           //make sure the message is not deleted before being passed to the observers
                           writeEvents().onDeleteDiscussionThreadMessage(createObserverContext(*currentUser), **it);

                           status = deleteDiscussionMessage(collection, id);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::deleteDiscussionMessage(EntityCollection& collection, IdTypeRef id)
{
    auto& indexById = collection.threadMessages().byId();
    const auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion thread message: " << static_cast<std::string>(id);
        return StatusCode::NOT_FOUND;
    }
    collection.deleteDiscussionThreadMessage(*it);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::changeDiscussionThreadMessageContent(IdTypeRef id,
                                                                                         StringView newContent,
                                                                                         StringView changeReason,
                                                                                         OutStream& output)
{
    StatusWriter status(output);

    const auto config = getGlobalConfig();

    const auto contentValidationCode = validateString(newContent, INVALID_PARAMETERS_FOR_EMPTY_STRING,
                                                      config->discussionThreadMessage.minContentLength,
                                                      config->discussionThreadMessage.maxContentLength,
                                                      &MemoryRepositoryBase::doesNotContainLeadingOrTrailingWhitespace);
    if (contentValidationCode != StatusCode::OK)
    {
        return status = contentValidationCode;
    }

    const auto reasonEmptyValidation = 0 == config->discussionThreadMessage.minChangeReasonLength
                                       ? ALLOW_EMPTY_STRING : INVALID_PARAMETERS_FOR_EMPTY_STRING;
    const auto reasonValidationCode = validateString(changeReason, reasonEmptyValidation,
                                                     config->discussionThreadMessage.minChangeReasonLength,
                                                     config->discussionThreadMessage.maxChangeReasonLength,
                                                     &MemoryRepositoryBase::doesNotContainLeadingOrTrailingWhitespace);
    if (reasonValidationCode != StatusCode::OK)
    {
        return status = reasonValidationCode;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.threadMessages().byId();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           if ( ! (status = authorization_->changeDiscussionThreadMessageContent(*currentUser, **it, newContent, changeReason)))
                           {
                               return;
                           }

                           if ( ! (status = changeDiscussionThreadMessageContent(collection, id, newContent, changeReason))) return;

                           writeEvents().onChangeDiscussionThreadMessage(createObserverContext(*currentUser), **it,
                                                                         DiscussionThreadMessage::ChangeType::Content);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::changeDiscussionThreadMessageContent(EntityCollection& collection,
                                                                                         IdTypeRef id,
                                                                                         StringView newContent,
                                                                                         StringView changeReason)
{
    auto& indexById = collection.threadMessages().byId();
    const auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion thread message: " << static_cast<std::string>(id);
        return StatusCode::NOT_FOUND;
    }

    auto currentUser = getCurrentUser(collection);
    DiscussionThreadMessagePtr messagePtr = *it;
    DiscussionThreadMessage& message = *messagePtr;

    message.content() = WholeChangeableString::copyFrom(newContent);
    message.updateLastUpdated(Context::getCurrentTime());
    message.updateLastUpdatedDetails({ Context::getCurrentUserIpAddress() });
    message.updateLastUpdatedReason(toString(changeReason));

    if (&message.createdBy() != currentUser.ptr())
    {
        message.updateLastUpdatedBy(currentUser);
    }

    DiscussionThread& parentThread = *(message.parentThread());

    parentThread.resetVisitorsSinceLastEdit();
    parentThread.latestVisibleChange() = message.lastUpdated();

    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::moveDiscussionThreadMessage(IdTypeRef messageId,
                                                                                IdTypeRef intoThreadId,
                                                                                OutStream& output)
{
    StatusWriter status(output);
    if ( ! messageId || ! intoThreadId)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& messagesIndexById = collection.threadMessages().byId();
                           auto messageIt = messagesIndexById.find(messageId);
                           if (messageIt == messagesIndexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           auto threadIntoPtr = collection.threads().findById(intoThreadId);
                           if ( ! threadIntoPtr)
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           DiscussionThreadMessagePtr messagePtr = *messageIt;
                           DiscussionThreadPtr threadFromPtr = messagePtr->parentThread();

                           if (threadFromPtr == threadIntoPtr)
                           {
                               status = StatusCode::NO_EFFECT;
                               return;
                           }

                           if ( ! (status = authorization_->moveDiscussionThreadMessage(*currentUser, *messagePtr, *threadIntoPtr)))
                           {
                               return;
                           }

                           //make sure the message is not deleted before being passed to the observers
                           writeEvents().onMoveDiscussionThreadMessage(createObserverContext(*currentUser),
                                                                       *messagePtr, *threadIntoPtr);

                           status = moveDiscussionThreadMessage(collection, messageId, intoThreadId);
                       });
    return status;
}

static void updateThreadOnMoveMessage(DiscussionThreadPtr threadPtr, int_fast32_t delta)
{
    DiscussionThread& thread = *threadPtr;

    thread.resetVisitorsSinceLastEdit();
    thread.latestVisibleChange() = Context::getCurrentTime();

    for (DiscussionTagPtr tag : thread.tags())
    {
        assert(tag);
        tag->updateMessageCount(delta);
    }

    for (DiscussionCategoryPtr category : thread.categories())
    {
        assert(category);
        category->updateMessageCount(threadPtr, delta);
    }
}

StatusCode MemoryRepositoryDiscussionThreadMessage::moveDiscussionThreadMessage(EntityCollection& collection,
                                                                                IdTypeRef messageId,
                                                                                IdTypeRef intoThreadId)
{
    auto& messagesIndexById = collection.threadMessages().byId();
    const auto messageIt = messagesIndexById.find(messageId);
    if (messageIt == messagesIndexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion thread message: " << static_cast<std::string>(messageId);
        return StatusCode::NOT_FOUND;
    }

    auto threadIntoPtr = collection.threads().findById(intoThreadId);
    if ( ! threadIntoPtr)
    {
        FORUM_LOG_ERROR << "Could not find discussion thread: " << static_cast<std::string>(intoThreadId);
        return StatusCode::NOT_FOUND;
    }

    DiscussionThreadMessagePtr messagePtr = *messageIt;
    DiscussionThread& threadInto = *threadIntoPtr;
    DiscussionThreadPtr threadFromPtr = messagePtr->parentThread();
    assert(threadFromPtr);
    DiscussionThread& threadFrom = *threadFromPtr;

    if (threadFromPtr == threadIntoPtr)
    {
        FORUM_LOG_WARNING << "Threa thread into which to move the discussion thread message is the same as the current one: "
                          << static_cast<std::string>(intoThreadId);

        return StatusCode::NO_EFFECT;
    }

    threadInto.insertMessage(messagePtr);
    updateThreadOnMoveMessage(threadIntoPtr, 1);

    threadFrom.deleteDiscussionThreadMessage(messagePtr);
    updateThreadOnMoveMessage(threadFromPtr, -1);

    messagePtr->parentThread() = threadIntoPtr;

    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::upVoteDiscussionThreadMessage(IdTypeRef id, OutStream& output)
{
    return voteDiscussionThreadMessage(id, output, true);
}

StatusCode MemoryRepositoryDiscussionThreadMessage::downVoteDiscussionThreadMessage(IdTypeRef id, OutStream& output)
{
    return voteDiscussionThreadMessage(id, output, false);
}

StatusCode MemoryRepositoryDiscussionThreadMessage::voteDiscussionThreadMessage(IdTypeRef id, OutStream& output,
                                                                                bool up)
{
    StatusWriter status(output);
    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    if (Context::getCurrentUserId() == anonymousUserId())
    {
        return status = StatusCode::NOT_ALLOWED;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.threadMessages().byId();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           auto& messageRef = *it;
                           auto& message = **it;

                           if (&message.createdBy() == currentUser.ptr())
                           {
                               status = StatusCode::NOT_ALLOWED;
                               return;
                           }

                           if (up)
                           {
                               if ( ! (status = authorization_->upVoteDiscussionThreadMessage(*currentUser, *messageRef))) return;
                               if ( ! (status = upVoteDiscussionThreadMessage(collection, id))) return;

                               writeEvents().onDiscussionThreadMessageUpVote(createObserverContext(*currentUser),
                                                                             message);
                           }
                           else
                           {
                               if ( ! (status = authorization_->downVoteDiscussionThreadMessage(*currentUser, *messageRef))) return;
                               if ( ! (status = downVoteDiscussionThreadMessage(collection, id))) return;
                               writeEvents().onDiscussionThreadMessageDownVote(createObserverContext(*currentUser),
                                                                               message);
                           }
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::upVoteDiscussionThreadMessage(EntityCollection& collection,
                                                                                  IdTypeRef id)
{
    return voteDiscussionThreadMessage(collection, id, true);
}

StatusCode MemoryRepositoryDiscussionThreadMessage::downVoteDiscussionThreadMessage(EntityCollection& collection,
                                                                                    IdTypeRef id)
{
    return voteDiscussionThreadMessage(collection, id, false);
}

StatusCode MemoryRepositoryDiscussionThreadMessage::voteDiscussionThreadMessage(EntityCollection& collection,
                                                                                IdTypeRef id, bool up)
{
    auto& indexById = collection.threadMessages().byId();
    const auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion thread message: " << static_cast<std::string>(id);
        return StatusCode::NOT_FOUND;
    }
    DiscussionThreadMessagePtr messagePtr = *it;
    DiscussionThreadMessage& message = *messagePtr;

    auto currentUser = getCurrentUser(collection);

    if (message.hasVoted(currentUser))
    {
        FORUM_LOG_WARNING << "User "
                          << static_cast<std::string>(currentUser->id()) << " has already voted discussion thread message "
                          << static_cast<std::string>(message.id());
        return StatusCode::NO_EFFECT;
    }

    const auto timestamp = Context::getCurrentTime();
    currentUser->registerVote(messagePtr);

    if (up)
    {
        message.addUpVote(currentUser, timestamp);
    }
    else
    {
        message.addDownVote(currentUser, timestamp);
    }

    User& targetUser = message.createdBy();

    if (up)
    {
        targetUser.receivedUpVotes() += 1;
    }
    else
    {
        targetUser.receivedDownVotes() += 1;
    }

    targetUser.voteHistory().push_back(
    {
        message.id(),
        currentUser->id(),
        timestamp,
        up ? User::ReceivedVoteHistoryEntryType::UpVote : User::ReceivedVoteHistoryEntryType::DownVote
    });

    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::resetVoteDiscussionThreadMessage(IdTypeRef id, OutStream& output)
{
    StatusWriter status(output);
    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    if (Context::getCurrentUserId() == anonymousUserId())
    {
        return status = StatusCode::NOT_ALLOWED;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.threadMessages().byId();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           auto& message = **it;

                           if (&message.createdBy() == currentUser.ptr())
                           {
                               status = StatusCode::NOT_ALLOWED;
                               return;
                           }

                           //check if the reset is still allowed at the current time
                           auto votedAt = message.votedAt(currentUser);
                           if ( ! votedAt)
                           {
                               status = StatusCode::NO_EFFECT;
                               return;
                           }

                           auto expiresInSeconds = static_cast<Timestamp>(getGlobalConfig()->user.resetVoteExpiresInSeconds);
                           if ((*votedAt + expiresInSeconds) < Context::getCurrentTime())
                           {
                               status = StatusCode::NOT_ALLOWED;
                               return;
                           }

                           if ( ! (status = authorization_->resetVoteDiscussionThreadMessage(*currentUser, **it)))
                           {
                               return;
                           }

                           if ( ! (status = resetVoteDiscussionThreadMessage(collection, id))) return;


                           writeEvents().onDiscussionThreadMessageResetVote(createObserverContext(*currentUser),
                                                                            message);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::resetVoteDiscussionThreadMessage(EntityCollection& collection,
                                                                                     IdTypeRef id)
{
    auto& indexById = collection.threadMessages().byId();
    const auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion thread message: " << static_cast<std::string>(id);
        return StatusCode::NOT_FOUND;
    }
    DiscussionThreadMessagePtr messagePtr = *it;
    DiscussionThreadMessage& message = *messagePtr;

    auto currentUser = getCurrentUser(collection);

    const auto removeVoteStatus = message.removeVote(currentUser);
    if (DiscussionThreadMessage::RemoveVoteStatus::Missing == removeVoteStatus)
    {
        FORUM_LOG_WARNING << "Could not find discussion vote of user "
                          << static_cast<std::string>(currentUser->id()) << " for discussion thread message "
                          << static_cast<std::string>(message.id());
        return StatusCode::NO_EFFECT;
    }

    User& targetUser = message.createdBy();

    if (DiscussionThreadMessage::RemoveVoteStatus::WasUpVote == removeVoteStatus)
    {
        targetUser.receivedUpVotes() -= 1;
    }

    if (DiscussionThreadMessage::RemoveVoteStatus::WasDownVote == removeVoteStatus)
    {
        targetUser.receivedDownVotes() -= 1;
    }

    targetUser.voteHistory().push_back(
    {
        message.id(),
        currentUser->id(),
        Context::getCurrentTime(),
        User::ReceivedVoteHistoryEntryType::ResetVote
    });

    return StatusCode::OK;
}

template<typename Collection>
static void writeMessageComments(const Collection& collection, OutStream& output,
                                 const GrantedPrivilegeStore& privilegeStore, 
                                 const ForumWidePrivilegeStore& forumWidePrivilegeStore, const User& currentUser)
{
    auto pageSize = getGlobalConfig()->discussionThreadMessage.maxMessagesCommentsPerPage;
    auto& displayContext = Context::getDisplayContext();

    SerializationRestriction restriction(privilegeStore, forumWidePrivilegeStore, 
                                         currentUser.id(), Context::getCurrentTime());

    writeEntitiesWithPagination(collection, "messageComments", output,
        displayContext.pageNumber, pageSize, displayContext.sortOrder == Context::SortOrder::Ascending, restriction);
}

template<typename Collection>
static void writeAllMessageComments(const Collection& collection, OutStream& output,
                                    const GrantedPrivilegeStore& privilegeStore, 
                                    const ForumWidePrivilegeStore& forumWidePrivilegeStore, const User& currentUser)
{
    SerializationRestriction restriction(privilegeStore, forumWidePrivilegeStore, 
                                         currentUser.id(), Context::getCurrentTime());

    writeAllEntities(collection, "messageComments", output, false, restriction);
}

StatusCode MemoryRepositoryDiscussionThreadMessage::getMessageComments(OutStream& output) const
{
    StatusWriter status(output);
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);

                          if ( ! (status = authorization_->getMessageComments(currentUser)))
                          {
                              return;
                          }

                          status.disable();
                          BoolTemporaryChanger _(serializationSettings.hideDiscussionThreadCreatedBy, true);
                          BoolTemporaryChanger __(serializationSettings.hideLatestMessage, true);

                          writeMessageComments(collection.messageComments().byCreated(), output,
                                               collection.grantedPrivileges(), collection, currentUser);
                          readEvents().onGetMessageComments(createObserverContext(currentUser));
                      });
    return status;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::getMessageCommentsOfDiscussionThreadMessage(IdTypeRef id,
                                                                                                OutStream& output) const
{
    StatusWriter status(output);
    PerformedByWithLastSeenUpdateGuard performedBy;

    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);

                          const auto& indexById = collection.threadMessages().byId();
                          auto it = indexById.find(id);
                          if (it == indexById.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }
                          auto& message = **it;

                          if ( ! (status = authorization_->getMessageCommentsOfDiscussionThreadMessage(currentUser, message)))
                          {
                              return;
                          }

                          BoolTemporaryChanger _(serializationSettings.hideMessageCommentMessage, true);
                          BoolTemporaryChanger __(serializationSettings.hideDiscussionThreadCreatedBy, true);
                          BoolTemporaryChanger ___(serializationSettings.hideLatestMessage, true);

                          status.disable();
                          writeAllMessageComments(message.comments().byCreated(), output,
                                                  collection.grantedPrivileges(), collection, currentUser);

                          readEvents().onGetMessageCommentsOfMessage(createObserverContext(currentUser), message);
                      });
    return status;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::getMessageCommentsOfUser(IdTypeRef id, OutStream& output) const
{
    StatusWriter status(output);
    PerformedByWithLastSeenUpdateGuard performedBy;

    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);

                          const auto& indexById = collection.users().byId();
                          auto it = indexById.find(id);
                          if (it == indexById.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }
                          auto& user = **it;

                          if ( ! (status = authorization_->getMessageCommentsOfUser(currentUser, user)))
                          {
                              return;
                          }

                          BoolTemporaryChanger _(serializationSettings.hideMessageCommentUser, true);
                          BoolTemporaryChanger __(serializationSettings.hideDiscussionThreadCreatedBy, true);
                          BoolTemporaryChanger ___(serializationSettings.hideLatestMessage, true);

                          status.disable();
                          writeMessageComments(user.messageComments().byCreated(), output,
                                               collection.grantedPrivileges(), collection, currentUser);

                          readEvents().onGetMessageCommentsOfUser(createObserverContext(currentUser), user);
                      });
    return status;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::addCommentToDiscussionThreadMessage(IdTypeRef messageId,
                                                                                        StringView content,
                                                                                        OutStream& output)
{
    StatusWriter status(output);
    if ( ! messageId)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    const auto config = getGlobalConfig();
    const auto validationCode = validateString(content, INVALID_PARAMETERS_FOR_EMPTY_STRING,
                                               config->discussionThreadMessage.minCommentLength,
                                               config->discussionThreadMessage.maxCommentLength,
                                               &MemoryRepositoryBase::doesNotContainLeadingOrTrailingWhitespace);
    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& messageIndex = collection.threadMessages().byId();
                           auto messageIt = messageIndex.find(messageId);
                           if (messageIt == messageIndex.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           if ( ! (status = authorization_->addCommentToDiscussionThreadMessage(*currentUser, **messageIt, content)))
                           {
                               return;
                           }

                           auto statusWithResource = addCommentToDiscussionThreadMessage(collection, generateUniqueId(),
                                                                                         messageId, content);
                           auto& comment = statusWithResource.resource;
                           if ( ! (status = statusWithResource.status)) return;

                           writeEvents().onAddCommentToDiscussionThreadMessage(createObserverContext(*currentUser),
                                                                               *comment);

                           status.writeNow([&](auto& writer)
                                           {
                                               writer << Json::propertySafeName("id", comment->id());
                                               writer << Json::propertySafeName("messageId", (*messageIt)->id());
                                               writer << Json::propertySafeName("created", comment->created());
                                           });
                       });
    return status;
}

StatusWithResource<MessageCommentPtr>
    MemoryRepositoryDiscussionThreadMessage::addCommentToDiscussionThreadMessage(EntityCollection& collection,
                                                                                 IdTypeRef commentId,
                                                                                 IdTypeRef messageId, StringView content)
{
    auto& messageIndex = collection.threadMessages().byId();
    const auto messageIt = messageIndex.find(messageId);
    if (messageIt == messageIndex.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion thread message: " << static_cast<std::string>(messageId);
        return StatusCode::NOT_FOUND;
    }

    auto currentUser = getCurrentUser(collection);
    DiscussionThreadMessagePtr messagePtr = *messageIt;
    DiscussionThreadMessage& message = *messagePtr;

    //IdType id, DiscussionThreadMessage& message, User& createdBy, Timestamp created, VisitDetails creationDetails
    auto comment = collection.createMessageComment(commentId, message, *currentUser, Context::getCurrentTime(),
                                                   { Context::getCurrentUserIpAddress() });
    comment->content() = WholeChangeableString::copyFrom(content);

    collection.insertMessageComment(comment);

    message.addComment(comment);
    currentUser->messageComments().add(comment);

    return comment;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::setMessageCommentToSolved(IdTypeRef id, OutStream& output)
{
    StatusWriter status(output);
    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    if (Context::getCurrentUserId() == anonymousUserId())
    {
        return status = StatusCode::NOT_ALLOWED;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.messageComments().byId();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           auto& comment = **it;

                           if ( ! (status = authorization_->setMessageCommentToSolved(*currentUser, comment)))
                           {
                               return;
                           }

                           if ( ! (status = setMessageCommentToSolved(collection, id))) return;

                           writeEvents().onSolveDiscussionThreadMessageComment(createObserverContext(*currentUser),
                                                                               comment);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::setMessageCommentToSolved(EntityCollection& collection, IdTypeRef id)
{
    auto& indexById = collection.messageComments().byId();
    const auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion thread message: " << static_cast<std::string>(id);
        return StatusCode::NOT_FOUND;
    }

    MessageCommentPtr commentPtr = *it;
    MessageComment& comment = *commentPtr;

    if (comment.solved())
    {
        FORUM_LOG_WARNING << "Comment " << static_cast<std::string>(comment.id()) << " is already solved";
        return StatusCode::NO_EFFECT;
    }

    comment.solved() = true;
    comment.parentMessage().solvedCommentsCount() += 1;

    return StatusCode::OK;
}
