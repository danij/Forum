#include "MemoryRepositoryDiscussionThreadMessage.h"

#include "Configuration.h"
#include "ContextProviders.h"
#include "EntitySerialization.h"
#include "OutputHelpers.h"
#include "RandomGenerator.h"
#include "StateHelpers.h"
#include "StringHelpers.h"

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

StatusCode MemoryRepositoryDiscussionThreadMessage::getDiscussionThreadMessagesOfUserByCreated(const IdType& id,
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
        auto& currentUser = performedBy.get(collection, store());

        const auto& indexById = collection.usersById();
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

        const auto& messages = (*it)->messagesByCreated();
        BoolTemporaryChanger _(serializationSettings.hideDiscussionThreadCreatedBy, true);
        BoolTemporaryChanger __(serializationSettings.hideDiscussionThreadMessageCreatedBy, true);
        BoolTemporaryChanger ___(serializationSettings.hideDiscussionThreadMessages, true);

        auto pageSize = getGlobalConfig()->discussionThreadMessage.maxMessagesPerPage;
        auto& displayContext = Context::getDisplayContext();

        status.disable();

        SerializationRestriction restriction(collection.grantedPrivileges(), currentUser, Context::getCurrentTime());

        writeEntitiesWithPagination(messages, "messages", output, displayContext.pageNumber, pageSize,
            displayContext.sortOrder == Context::SortOrder::Ascending, restriction);

        readEvents().onGetDiscussionThreadMessagesOfUser(createObserverContext(currentUser), **it);
    });
    return status;
}


StatusCode MemoryRepositoryDiscussionThreadMessage::addNewDiscussionMessageInThread(const IdType& threadId,
                                                                                    StringView content,
                                                                                    OutStream& output)
{
    StatusWriter status(output);
    if ( ! threadId)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    auto config = getGlobalConfig();
    auto validationCode = validateString(content, INVALID_PARAMETERS_FOR_EMPTY_STRING,
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

                           auto& threadIndex = collection.threads();
                           auto threadIt = threadIndex.find(threadId);
                           if (threadIt == threadIndex.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           if ( ! (status = authorization_->addNewDiscussionMessageInThread(*currentUser, **threadIt, content)))
                           {
                               return;
                           }

                           auto statusWithResource = addNewDiscussionMessageInThread(collection, threadId, content);
                           auto& message = statusWithResource.resource;
                           if ( ! (status = statusWithResource.status)) return;

                           auto& user = *currentUser;
                           writeEvents().onSubscribeToDiscussionThread(createObserverContext(user), **threadIt);
                           writeEvents().onAddNewDiscussionThreadMessage(createObserverContext(user), *message);

                           status.writeNow([&](auto& writer)
                                           {
                                               writer << Json::propertySafeName("id", message->id());
                                               writer << Json::propertySafeName("parentId", (*threadIt)->id());
                                               writer << Json::propertySafeName("created", message->created());
                                           });
                       });
    return status;
}

StatusWithResource<DiscussionThreadMessageRef>
    MemoryRepositoryDiscussionThreadMessage::addNewDiscussionMessageInThread(EntityCollection& collection,
                                                                             IdTypeRef threadId, StringView content)
{
    auto& threadIndex = collection.threads();
    auto threadIt = threadIndex.find(threadId);
    if (threadIt == threadIndex.end())
    {
        return StatusCode::NOT_FOUND;
    }

    auto currentUser = getCurrentUser(collection);

    auto message = std::make_shared<DiscussionThreadMessage>(*currentUser);
    message->id() = generateUUIDString();
    message->parentThread() = *threadIt;
    message->content() = content;
    updateCreated(*message);

    collection.insertMessage(message);
    collection.modifyDiscussionThread(threadIt,
        [&collection, &message, &threadIt, &currentUser] (DiscussionThread& thread)
        {
            thread.insertMessage(message);
            thread.resetVisitorsSinceLastEdit();
            thread.latestVisibleChange() = message->created();
            thread.subscribedUsers().insert(currentUser);

            for (auto& tagWeak : thread.tagsWeak())
            {
                if (auto tagShared = tagWeak.lock())
                {
                    collection.modifyDiscussionTagById(tagShared->id(),
                        [&thread](auto& tag)
                        {
                            tag.messageCount() += 1;
                            //notify the thread collection of each tag that the thread has a new message
                            tag.modifyDiscussionThreadById(thread.id(), {});
                        });
                }
            }
            for (auto& categoryWeak : thread.categoriesWeak())
            {
                if (auto categoryShared = categoryWeak.lock())
                {
                    collection.modifyDiscussionCategoryById(categoryShared->id(),
                        [&thread, &threadIt](auto& category)
                        {
                            category.updateMessageCount(*threadIt, 1);
                            //notify the thread collection of each category that the thread has a new message
                            category.modifyDiscussionThreadById(thread.id(), {});
                        });
                }
            }

            //add privileges for the user that created the message
            auto changePrivilegeDuration = optionalOrZero(
                    thread.getDiscussionThreadMessageDefaultPrivilegeDuration(
                            DiscussionThreadMessageDefaultPrivilegeDuration::CHANGE_CONTENT));
            if (changePrivilegeDuration > 0)
            {
                auto privilege = DiscussionThreadMessagePrivilege::CHANGE_CONTENT;
                auto valueNeeded = optionalOrZero(thread.getDiscussionThreadMessagePrivilege(privilege));

                if (valueNeeded > 0)
                {
                    auto expiresAt = message->created() + changePrivilegeDuration;

                    collection.grantedPrivileges().grantDiscussionThreadMessagePrivilege(
                            currentUser->id(), message->id(), privilege, valueNeeded, expiresAt);
                }
            }

            auto deletePrivilegeDuration = optionalOrZero(
                    thread.getDiscussionThreadMessageDefaultPrivilegeDuration(
                            DiscussionThreadMessageDefaultPrivilegeDuration::DELETE));
            if (deletePrivilegeDuration > 0)
            {
                auto privilege = DiscussionThreadMessagePrivilege::DELETE;
                auto valueNeeded = optionalOrZero(thread.getDiscussionThreadMessagePrivilege(privilege));

                if (valueNeeded)
                {
                    auto expiresAt = message->created() + changePrivilegeDuration;

                    collection.grantedPrivileges().grantDiscussionThreadMessagePrivilege(
                            currentUser->id(), message->id(), privilege, valueNeeded, expiresAt);
                }
            }
        });

    collection.modifyUserById(currentUser->id(), [&](User& user)
                                                 {
                                                     user.insertMessage(message);
                                                     user.subscribedThreads().insertDiscussionThread(*threadIt);
                                                 });
    return message;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::deleteDiscussionMessage(const IdType& id, OutStream& output)
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

                           auto& indexById = collection.messages().get<EntityCollection::DiscussionThreadMessageCollectionById>();
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
    auto& indexById = collection.messages().get<EntityCollection::DiscussionThreadMessageCollectionById>();
    auto it = indexById.find(id);
    if (it == indexById.end())
    {
        return StatusCode::NOT_FOUND;
    }
    collection.deleteDiscussionThreadMessage(it);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::changeDiscussionThreadMessageContent(const IdType& id,
                                                                                         StringView newContent,
                                                                                         StringView changeReason,
                                                                                         OutStream& output)
{
    StatusWriter status(output);

    auto config = getGlobalConfig();

    auto contentValidationCode = validateString(newContent, INVALID_PARAMETERS_FOR_EMPTY_STRING,
                                                config->discussionThreadMessage.minContentLength,
                                                config->discussionThreadMessage.maxContentLength,
                                                &MemoryRepositoryBase::doesNotContainLeadingOrTrailingWhitespace);
    if (contentValidationCode != StatusCode::OK)
    {
        return status = contentValidationCode;
    }

    auto reasonEmptyValidation = 0 == config->discussionThreadMessage.minChangeReasonLength
                                 ? ALLOW_EMPTY_STRING : INVALID_PARAMETERS_FOR_EMPTY_STRING;
    auto reasonValidationCode = validateString(newContent, reasonEmptyValidation,
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

                           auto& indexById = collection.messages()
                                   .get<EntityCollection::DiscussionThreadMessageCollectionById>();
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
    auto& indexById = collection.messages()
            .get<EntityCollection::DiscussionThreadMessageCollectionById>();
    auto it = indexById.find(id);
    if (it == indexById.end())
    {
        return StatusCode::NOT_FOUND;
    }

    auto currentUser = getCurrentUser(collection);

    collection.modifyDiscussionThreadMessage(it, [&](DiscussionThreadMessage& message)
    {
        message.content() = newContent;
        updateLastUpdated(message, {});
        message.lastUpdatedReason() = toString(changeReason);
        if (&message.createdBy() != currentUser.get())
        {
            message.lastUpdatedBy() = currentUser;
        }
        message.executeActionWithParentThreadIfAvailable([&](auto& thread)
            {
                thread.resetVisitorsSinceLastEdit();
                thread.latestVisibleChange() = message.lastUpdated();
            });
    });
    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::moveDiscussionThreadMessage(const IdType& messageId,
                                                                                const IdType& intoThreadId,
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

                           auto& messagesIndexById = collection.messages().get<EntityCollection::DiscussionThreadMessageCollectionById>();
                           auto messageIt = messagesIndexById.find(messageId);
                           if (messageIt == messagesIndexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           auto& threadsIndexById = collection.threads().get<EntityCollection::DiscussionThreadCollectionById>();
                           auto itInto = threadsIndexById.find(intoThreadId);
                           if (itInto == threadsIndexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           auto messageRef = *messageIt;
                           auto threadIntoRef = *itInto;
                           auto threadFromRef = messageRef->parentThread().lock();

                           if (threadFromRef == threadIntoRef)
                           {
                               status = StatusCode::NO_EFFECT;
                               return;
                           }

                           if ( ! (status = authorization_->moveDiscussionThreadMessage(*currentUser, *messageRef, *threadIntoRef)))
                           {
                               return;
                           }

                           //make sure the message is not deleted before being passed to the observers
                           writeEvents().onMoveDiscussionThreadMessage(createObserverContext(*currentUser),
                                                                       *messageRef, *threadIntoRef);

                           status = moveDiscussionThreadMessage(collection, messageId, intoThreadId);
                       });
    return status;
}

static void updateThreadOnMoveMessage(EntityCollection& collection, DiscussionThreadRef&& threadRef, bool insert)
{
    threadRef->resetVisitorsSinceLastEdit();
    threadRef->latestVisibleChange() = Context::getCurrentTime();

    int_fast32_t increment = insert ? 1 : -1;

    for (auto& tagWeak : threadRef->tagsWeak())
    {
        if (auto tagShared = tagWeak.lock())
        {
            collection.modifyDiscussionTagById(tagShared->id(), [increment](auto& tag)
                                                                {
                                                                    tag.messageCount() += increment;
                                                                });
        }
    }
    for (auto& categoryWeak : threadRef->categoriesWeak())
    {
        if (auto categoryShared = categoryWeak.lock())
        {
            collection.modifyDiscussionCategoryById(categoryShared->id(), [&](auto& category)
                                                                          {
                                                                              category.updateMessageCount(threadRef, increment);
                                                                          });
        }
    }
}

StatusCode MemoryRepositoryDiscussionThreadMessage::moveDiscussionThreadMessage(EntityCollection& collection,
                                                                                IdTypeRef messageId,
                                                                                IdTypeRef intoThreadId)
{
    auto& messagesIndexById = collection.messages().get<EntityCollection::DiscussionThreadMessageCollectionById>();
    auto messageIt = messagesIndexById.find(messageId);
    if (messageIt == messagesIndexById.end())
    {
        return StatusCode::NOT_FOUND;
    }
    auto& threadsIndexById = collection.threads().get<EntityCollection::DiscussionThreadCollectionById>();
    auto itInto = threadsIndexById.find(intoThreadId);
    if (itInto == threadsIndexById.end())
    {
        return StatusCode::NOT_FOUND;
    }

    auto messageRef = *messageIt;
    auto threadIntoRef = *itInto;
    auto threadFromRef = messageRef->parentThread().lock();

    if (threadFromRef == threadIntoRef)
    {
        return StatusCode::NO_EFFECT;
    }

    collection.modifyDiscussionThread(itInto, [&collection, &messageRef]
                                              (DiscussionThread& thread)
                                              {
                                                  thread.insertMessage(messageRef);
                                                  updateThreadOnMoveMessage(collection, thread.shared_from_this(), true);
                                              });
    if (threadFromRef)
    {
        collection.modifyDiscussionThreadById(threadFromRef->id(),
            [&collection, &messageRef](DiscussionThread& thread)
            {
                thread.deleteDiscussionThreadMessageById(messageRef->id());
                updateThreadOnMoveMessage(collection, thread.shared_from_this(), true);
            });
    }

    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::upVoteDiscussionThreadMessage(const IdType& id, OutStream& output)
{
    return voteDiscussionThreadMessage(id, output, true);
}

StatusCode MemoryRepositoryDiscussionThreadMessage::downVoteDiscussionThreadMessage(const IdType& id, OutStream& output)
{
    return voteDiscussionThreadMessage(id, output, false);
}

StatusCode MemoryRepositoryDiscussionThreadMessage::voteDiscussionThreadMessage(const IdType& id, OutStream& output,
                                                                                bool up)
{
    StatusWriter status(output);
    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    if (Context::getCurrentUserId() == AnonymousUserId)
    {
        return status = StatusCode::NOT_ALLOWED;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.messages().get<EntityCollection::DiscussionThreadMessageCollectionById>();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           auto& messageRef = *it;
                           auto& message = **it;

                           if (&message.createdBy() == currentUser.get())
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
    auto& indexById = collection.messages().get<EntityCollection::DiscussionThreadMessageCollectionById>();
    auto it = indexById.find(id);
    if (it == indexById.end())
    {
        return StatusCode::NOT_FOUND;
    }
    auto& messageRef = *it;
    auto& message = **it;

    auto currentUser = getCurrentUser(collection);

    UserWeakRef userWeak(currentUser);
    if (message.hasVoted(userWeak))
    {
        return StatusCode::NO_EFFECT;
    }

    auto timestamp = Context::getCurrentTime();
    currentUser->registerVote(messageRef);

    if (up)
    {
        message.addUpVote(std::move(userWeak), timestamp);
    }
    else
    {
        message.addDownVote(std::move(userWeak), timestamp);
    }

    if (auto thread = message.parentThread().lock())
    {
        auto resetVotePrivilegeDuration = optionalOrZero(
            thread->getDiscussionThreadMessageDefaultPrivilegeDuration(
                DiscussionThreadMessageDefaultPrivilegeDuration::RESET_VOTE));
        if (resetVotePrivilegeDuration > 0)
        {
            auto privilege = DiscussionThreadMessagePrivilege::RESET_VOTE;
            auto valueNeeded = optionalOrZero(thread->getDiscussionThreadMessagePrivilege(privilege));
            if (valueNeeded > 0)
            {
                auto expiresAt = timestamp + resetVotePrivilegeDuration;

                collection.grantedPrivileges().grantDiscussionThreadMessagePrivilege(
                        currentUser->id(), message.id(), privilege, valueNeeded, expiresAt);
            }
        }
    }
    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::resetVoteDiscussionThreadMessage(const IdType& id, OutStream& output)
{
    StatusWriter status(output);
    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    if (Context::getCurrentUserId() == AnonymousUserId)
    {
        return status = StatusCode::NOT_ALLOWED;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.messages().get<EntityCollection::DiscussionThreadMessageCollectionById>();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           auto& message = **it;

                           if (&message.createdBy() == currentUser.get())
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
    auto& indexById = collection.messages().get<EntityCollection::DiscussionThreadMessageCollectionById>();
    auto it = indexById.find(id);
    if (it == indexById.end())
    {
        return StatusCode::NOT_FOUND;
    }
    auto& message = **it;

    auto currentUser = getCurrentUser(collection);

    if ( ! message.removeVote(currentUser))
    {
        return StatusCode::NO_EFFECT;
    }
    return StatusCode::OK;
}

template<typename Collection>
static void writeMessageComments(const Collection& collection, OutStream& output,
                                 const GrantedPrivilegeStore& privilegeStore, const User& currentUser)
{
    auto pageSize = getGlobalConfig()->discussionThreadMessage.maxMessagesCommentsPerPage;
    auto& displayContext = Context::getDisplayContext();

    SerializationRestriction restriction(privilegeStore, currentUser, Context::getCurrentTime());

    writeEntitiesWithPagination(collection, "message_comments", output,
        displayContext.pageNumber, pageSize, displayContext.sortOrder == Context::SortOrder::Ascending, restriction);
}

StatusCode MemoryRepositoryDiscussionThreadMessage::getMessageComments(OutStream& output) const
{
    StatusWriter status(output);
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, store());

                          if ( ! (status = authorization_->getMessageComments(currentUser)))
                          {
                              return;
                          }

                          writeMessageComments(collection.messageCommentsByCreated(), output,
                                               collection.grantedPrivileges(), currentUser);
                          readEvents().onGetMessageComments(createObserverContext(currentUser));
                      });
    return status;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::getMessageCommentsOfDiscussionThreadMessage(const IdType& id,
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
                          auto& currentUser = performedBy.get(collection, store());

                          const auto& indexById = collection.messagesById();
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

                          writeMessageComments(message.messageCommentsByCreated(), output,
                                               collection.grantedPrivileges(), currentUser);

                          readEvents().onGetMessageCommentsOfMessage(createObserverContext(currentUser), message);
                      });
    return status;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::getMessageCommentsOfUser(const IdType& id, OutStream& output) const
{
    StatusWriter status(output);
    PerformedByWithLastSeenUpdateGuard performedBy;

    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, store());

                          const auto& indexById = collection.usersById();
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

                          writeMessageComments(user.messageCommentsByCreated(), output,
                                               collection.grantedPrivileges(), currentUser);

                          readEvents().onGetMessageCommentsOfUser(createObserverContext(currentUser), user);
                      });
    return status;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::addCommentToDiscussionThreadMessage(const IdType& messageId,
                                                                                        StringView content,
                                                                                        OutStream& output)
{
    StatusWriter status(output);
    if ( ! messageId)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    auto config = getGlobalConfig();
    auto validationCode = validateString(content, INVALID_PARAMETERS_FOR_EMPTY_STRING,
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

                           auto& messageIndex = collection.messages();
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

                           auto statusWithResource = addCommentToDiscussionThreadMessage(collection, messageId, content);
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

StatusWithResource<MessageCommentRef>
    MemoryRepositoryDiscussionThreadMessage::addCommentToDiscussionThreadMessage(EntityCollection& collection,
                                                                                 IdTypeRef messageId, StringView content)
{
    auto& messageIndex = collection.messages();
    auto messageIt = messageIndex.find(messageId);
    if (messageIt == messageIndex.end())
    {
        return StatusCode::NOT_FOUND;
    }

    auto currentUser = getCurrentUser(collection);

    auto comment = std::make_shared<MessageComment>(*currentUser);
    comment->id() = generateUUIDString();
    comment->content() = content;
    updateCreated(*comment);

    collection.messageComments().insert(comment);

    collection.modifyDiscussionThreadMessageById((*messageIt)->id(), [&](auto& message)
                                                                     {
                                                                         message.messageComments().insert(comment);
                                                                     });

    collection.modifyUserById(currentUser->id(), [&](User& user)
                                                 {
                                                     user.messageComments().insert(comment);
                                                 });
    return comment;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::setMessageCommentToSolved(const IdType& id, OutStream& output)
{
    StatusWriter status(output);
    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    if (Context::getCurrentUserId() == AnonymousUserId)
    {
        return status = StatusCode::NOT_ALLOWED;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.messageComments().get<EntityCollection::MessageCommentCollectionById>();
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
    auto& indexById = collection.messageComments().get<EntityCollection::MessageCommentCollectionById>();
    auto it = indexById.find(id);
    if (it == indexById.end())
    {
        return StatusCode::NOT_FOUND;
    }
    auto& comment = **it;

    if (comment.solved())
    {
        return StatusCode::NO_EFFECT;
    }

    comment.solved() = true;
    comment.executeActionWithParentMessageIfAvailable([&](DiscussionThreadMessage& message)
                                                        {
                                                            message.solvedCommentsCount() += 1;
                                                        });
    return StatusCode::OK;
}
