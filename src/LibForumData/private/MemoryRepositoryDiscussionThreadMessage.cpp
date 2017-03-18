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

MemoryRepositoryDiscussionThreadMessage::MemoryRepositoryDiscussionThreadMessage(MemoryStoreRef store)
    : MemoryRepositoryBase(std::move(store)),
    validDiscussionMessageContentRegex(boost::make_u32regex("^[^[:space:]]+.*[^[:space:]]+$")),
    validDiscussionMessageCommentRegex(boost::make_u32regex("^[^[:space:]]+.*[^[:space:]]+$")),
    validDiscussionMessageChangeReasonRegex(boost::make_u32regex("^[^[:space:]]+.*[^[:space:]]+$"))
{
}

StatusCode MemoryRepositoryDiscussionThreadMessage::getDiscussionThreadMessagesOfUserByCreated(const IdType& id,
                                                                                               OutStream& output) const
{
    StatusWriter status(output, StatusCode::OK);
    PerformedByWithLastSeenUpdateGuard performedBy;

    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    collection().read([&](const EntityCollection& collection)
    {
        const auto& indexById = collection.usersById();
        auto it = indexById.find(id);
        if (it == indexById.end())
        {
            status = StatusCode::NOT_FOUND;
            return;
        }

        const auto& messages = (*it)->messagesByCreated();
        BoolTemporaryChanger _(serializationSettings.hideDiscussionThreadCreatedBy, true);
        BoolTemporaryChanger __(serializationSettings.hideDiscussionThreadMessageCreatedBy, true);
        BoolTemporaryChanger ___(serializationSettings.hideDiscussionThreadMessages, true);

        auto pageSize = getGlobalConfig()->discussionThreadMessage.maxMessagesPerPage;
        auto& displayContext = Context::getDisplayContext();

        status.disable();
        writeEntitiesWithPagination(messages, "messages", output, displayContext.pageNumber, pageSize,
            displayContext.sortOrder == Context::SortOrder::Ascending, [](const auto& m) { return m; });

        if ( ! Context::skipObservers())
            readEvents().onGetDiscussionThreadMessagesOfUser(createObserverContext(performedBy.get(collection, store())),
                                                             **it);
    });
    return status;
}


StatusCode MemoryRepositoryDiscussionThreadMessage::addNewDiscussionMessageInThread(const IdType& threadId, 
                                                                                    const StringView& content,
                                                                                    OutStream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! threadId)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    auto config = getGlobalConfig();
    auto validationCode = validateString(content, validDiscussionMessageContentRegex, INVALID_PARAMETERS_FOR_EMPTY_STRING,
                                         config->discussionThreadMessage.minContentLength, 
                                         config->discussionThreadMessage.maxContentLength);

    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto& threadIndex = collection.threads();
                           auto threadIt = threadIndex.find(threadId);
                           if (threadIt == threadIndex.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                 
                           const auto& createdBy = performedBy.getAndUpdate(collection);
                 
                           auto message = std::make_shared<DiscussionThreadMessage>(*createdBy);
                           message->id() = generateUUIDString();
                           message->parentThread() = *threadIt;
                           message->content() = content;
                           updateCreated(*message);
                 
                           collection.messages().insert(message);
                           collection.modifyDiscussionThread(threadIt, [&collection, &message, &threadIt]
                               (DiscussionThread& thread)
                               {
                                   thread.messages().insert(message);
                                   thread.resetVisitorsSinceLastEdit();
                                   thread.latestVisibleChange() = message->created();
                 
                                   for (auto& tagWeak : thread.tagsWeak())
                                   {
                                       if (auto tagShared = tagWeak.lock())
                                       {
                                           collection.modifyDiscussionTagById(tagShared->id(), 
                                               [&thread](auto& tag)
                                               {
                                                   tag.messageCount() += 1;
                                                   //notify the thread collection of each tag that the thread has a new message
                                                   tag.modifyDiscussionThreadById(thread.id());
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
                                                   category.modifyDiscussionThreadById(thread.id());
                                               });
                                       }
                                   }
                               });
                 
                           collection.modifyUserById(createdBy->id(), [&](User& user)
                                                                      {
                                                                          user.messages().insert(message);
                                                                      });

                           if ( ! Context::skipObservers())
                               writeEvents().onAddNewDiscussionThreadMessage(createObserverContext(*createdBy), *message);
                 
                           status.writeNow([&](auto& writer)
                                           {
                                               writer << Json::propertySafeName("id", message->id());
                                               writer << Json::propertySafeName("parentId", (*threadIt)->id());
                                               writer << Json::propertySafeName("created", message->created());
                                           });
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::deleteDiscussionMessage(const IdType& id, OutStream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto& indexById = collection.messages().get<EntityCollection::DiscussionThreadMessageCollectionById>();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           //make sure the message is not deleted before being passed to the observers
                           if ( ! Context::skipObservers())
                               writeEvents().onDeleteDiscussionThreadMessage(
                                       createObserverContext(*performedBy.getAndUpdate(collection)), **it);

                           collection.deleteDiscussionThreadMessage(it);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::changeDiscussionThreadMessageContent(const IdType& id, 
                                                                                         const StringView& newContent,
                                                                                         const StringView& changeReason, 
                                                                                         OutStream& output)
{
    StatusWriter status(output, StatusCode::OK);

    auto config = getGlobalConfig();

    auto contentValidationCode = validateString(newContent, validDiscussionMessageContentRegex, 
                                                INVALID_PARAMETERS_FOR_EMPTY_STRING,
                                                config->discussionThreadMessage.minContentLength, 
                                                config->discussionThreadMessage.maxContentLength);
    if (contentValidationCode != StatusCode::OK)
    {
        return status = contentValidationCode;
    }

    auto reasonEmptyValidation = 0 == config->discussionThreadMessage.minChangeReasonLength
                                 ? ALLOW_EMPTY_STRING : INVALID_PARAMETERS_FOR_EMPTY_STRING;
    auto reasonValidationCode = validateString(newContent, validDiscussionMessageChangeReasonRegex,
                                               reasonEmptyValidation,
                                               config->discussionThreadMessage.minChangeReasonLength, 
                                               config->discussionThreadMessage.maxChangeReasonLength);
    if (reasonValidationCode != StatusCode::OK)
    {
        return status = reasonValidationCode;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto& indexById = collection.messages()
                                   .get<EntityCollection::DiscussionThreadMessageCollectionById>();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           auto performedByPtr = performedBy.getAndUpdate(collection);
                 
                           collection.modifyDiscussionThreadMessage(it, [&](DiscussionThreadMessage& message)
                           {
                               message.content() = newContent;
                               updateLastUpdated(message, {});
                               message.lastUpdatedReason() = toString(changeReason);
                               if (&message.createdBy() != performedByPtr.get())
                               {
                                   message.lastUpdatedBy() = performedByPtr;
                               }
                               message.executeActionWithParentThreadIfAvailable([&](auto& thread)
                                   {
                                       thread.resetVisitorsSinceLastEdit();
                                       thread.latestVisibleChange() = message.lastUpdated();
                                   });
                           });

                           if ( ! Context::skipObservers())
                               writeEvents().onChangeDiscussionThreadMessage(createObserverContext(*performedByPtr), **it,
                                                                             DiscussionThreadMessage::ChangeType::Content);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::moveDiscussionThreadMessage(const IdType& messageId, 
                                                                                const IdType& intoThreadId,
                                                                                OutStream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! messageId || ! intoThreadId)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
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
                     
                           //make sure the message is not deleted before being passed to the observers
                           if ( ! Context::skipObservers())
                               writeEvents().onMoveDiscussionThreadMessage(
                                   createObserverContext(*performedBy.getAndUpdate(collection)), *messageRef, *threadIntoRef);

                           auto threadUpdateFn = [&collection](DiscussionThreadRef&& threadRef, bool insert)
                           {
                               threadRef->resetVisitorsSinceLastEdit();
                               threadRef->latestVisibleChange() = Context::getCurrentTime();
                 
                               int_fast32_t increment = insert ? 1 : -1;
                 
                               for (auto& tagWeak : threadRef->tagsWeak())
                               {
                                   if (auto tagShared = tagWeak.lock())
                                   {
                                       collection.modifyDiscussionTagById(tagShared->id(), 
                                           [increment](auto& tag)
                                           {
                                               tag.messageCount() += increment;
                                           });
                                   }
                               }
                               for (auto& categoryWeak : threadRef->categoriesWeak())
                               {
                                   if (auto categoryShared = categoryWeak.lock())
                                   {
                                       collection.modifyDiscussionCategoryById(categoryShared->id(), 
                                           [&](auto& category)
                                           {
                                               category.updateMessageCount(threadRef, increment);
                                           });
                                   }
                               }                              
                           };
                 
                           collection.modifyDiscussionThread(itInto, [&collection, &messageRef, &threadUpdateFn]
                               (DiscussionThread& thread)
                               {
                                   thread.messages().insert(messageRef);
                                   threadUpdateFn(thread.shared_from_this(), true);
                               });
                 
                           if (threadFromRef)
                           {
                               collection.modifyDiscussionThreadById(threadFromRef->id(), 
                                   [&collection, &messageRef, &threadUpdateFn](DiscussionThread& thread)
                                   {
                                       thread.deleteDiscussionThreadMessageById(messageRef->id());
                                       threadUpdateFn(thread.shared_from_this(), true);
                                   });                              
                           }
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::voteDiscussionThreadMessage(const IdType& id, OutStream& output,
                                                                                bool up)
{
    StatusWriter status(output, StatusCode::OK);
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
                 
                           UserWeakRef userWeak(currentUser);
                           if (message.hasVoted(userWeak))
                           {
                               status = StatusCode::NO_EFFECT;
                               return;
                           }
                 
                           auto timestamp = Context::getCurrentTime();
                           currentUser->registerVote(messageRef);
                 
                           if (up)
                           {
                               message.addUpVote(std::move(userWeak), timestamp);
                               if ( ! Context::skipObservers())
                                   writeEvents().onDiscussionThreadMessageUpVote(createObserverContext(*currentUser),
                                                                                 message);
                           }
                           else
                           {
                               message.addDownVote(std::move(userWeak), timestamp);
                               if ( ! Context::skipObservers())
                                   writeEvents().onDiscussionThreadMessageDownVote(createObserverContext(*currentUser),
                                                                                   message);
                           }
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::upVoteDiscussionThreadMessage(const IdType& id, OutStream& output)
{
    return voteDiscussionThreadMessage(id, output, true);
}

StatusCode MemoryRepositoryDiscussionThreadMessage::downVoteDiscussionThreadMessage(const IdType& id, OutStream& output)
{
    return voteDiscussionThreadMessage(id, output, false);
}

StatusCode MemoryRepositoryDiscussionThreadMessage::resetVoteDiscussionThreadMessage(const IdType& id, OutStream& output)
{
    StatusWriter status(output, StatusCode::OK);
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
               
                           if ( ! message.removeVote(currentUser))
                           {
                               status = StatusCode::NO_EFFECT;
                               return;
                           }

                           if ( ! Context::skipObservers())
                               writeEvents().onDiscussionThreadMessageResetVote(createObserverContext(*currentUser),
                                                                                message);
                       });
    return status;
}

template<typename Collection>
static void writeMessageComments(const Collection& collection, OutStream& output)
{
    auto pageSize = getGlobalConfig()->discussionThreadMessage.maxMessagesCommentsPerPage;
    auto& displayContext = Context::getDisplayContext();

    writeEntitiesWithPagination(collection, "message_comments", output, 
        displayContext.pageNumber, pageSize, displayContext.sortOrder == Context::SortOrder::Ascending, 
        [](const auto& c) { return c; });
}

StatusCode MemoryRepositoryDiscussionThreadMessage::getMessageComments(OutStream& output) const
{
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          writeMessageComments(collection.messageCommentsByCreated(), output);
                          if ( ! Context::skipObservers())
                              readEvents().onGetMessageComments(createObserverContext(performedBy.get(collection,
                                                                                                      store())));
                      });
    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::getMessageCommentsOfDiscussionThreadMessage(const IdType& id, 
                                                                                                OutStream& output) const
{
    StatusWriter status(output, StatusCode::OK);
    PerformedByWithLastSeenUpdateGuard performedBy;

    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    collection().read([&](const EntityCollection& collection)
                      {
                          const auto& indexById = collection.messagesById();
                          auto it = indexById.find(id);
                          if (it == indexById.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }
                          auto& message = **it;
                      
                          BoolTemporaryChanger _(serializationSettings.hideMessageCommentMessage, true);
                      
                          writeMessageComments(message.messageCommentsByCreated(), output);

                          if ( ! Context::skipObservers())
                              readEvents().onGetMessageCommentsOfMessage(
                                      createObserverContext(performedBy.get(collection, store())), message);
                      });
    return status;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::getMessageCommentsOfUser(const IdType& id, OutStream& output) const
{
    StatusWriter status(output, StatusCode::OK);
    PerformedByWithLastSeenUpdateGuard performedBy;

    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    collection().read([&](const EntityCollection& collection)
                      {
                          const auto& indexById = collection.usersById();
                          auto it = indexById.find(id);
                          if (it == indexById.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }
                          auto& user = **it;
                  
                          BoolTemporaryChanger _(serializationSettings.hideMessageCommentUser, true);
                          
                          writeMessageComments(user.messageCommentsByCreated(), output);

                          if ( ! Context::skipObservers())
                              readEvents().onGetMessageCommentsOfUser(
                                      createObserverContext(performedBy.get(collection, store())), user);
                      });
    return status;
}

StatusCode MemoryRepositoryDiscussionThreadMessage::addCommentToDiscussionThreadMessage(const IdType& messageId, 
                                                                                        const StringView& content,
                                                                                        OutStream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! messageId)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    auto config = getGlobalConfig();
    auto validationCode = validateString(content, validDiscussionMessageCommentRegex,
                                         INVALID_PARAMETERS_FOR_EMPTY_STRING,
                                         config->discussionThreadMessage.minCommentLength,
                                         config->discussionThreadMessage.maxCommentLength);
    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto& messageIndex = collection.messages();
                           auto messageIt = messageIndex.find(messageId);
                           if (messageIt == messageIndex.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                 
                           const auto& createdBy = performedBy.getAndUpdate(collection);
                 
                           auto comment = std::make_shared<MessageComment>(*createdBy);
                           comment->id() = generateUUIDString();
                           comment->content() = content;
                           updateCreated(*comment);
                 
                           collection.messageComments().insert(comment);
                 
                           collection.modifyDiscussionThreadMessageById((*messageIt)->id(), 
                               [&](auto& message)
                               {
                                   message.messageComments().insert(comment);
                               });
                 
                           collection.modifyUserById(createdBy->id(), [&](User& user)
                                                                      {
                                                                          user.messageComments().insert(comment);
                                                                      });

                           if ( ! Context::skipObservers())
                               writeEvents().onAddCommentToDiscussionThreadMessage(createObserverContext(*createdBy),
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

StatusCode MemoryRepositoryDiscussionThreadMessage::setMessageCommentToSolved(const IdType& id, OutStream& output)
{
    StatusWriter status(output, StatusCode::OK);
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
                 
                           if ( ! comment.solved())
                           {
                               status = StatusCode::NO_EFFECT;
                               return;
                           }
                 
                           comment.solved() = true;
                           comment.executeActionWithParentMessageIfAvailable(
                               [&](DiscussionThreadMessage& message)
                               {
                                   message.solvedCommentsCount() += 1;
                               });

                           if ( ! Context::skipObservers())
                               writeEvents().onSolveDiscussionThreadMessageComment(createObserverContext(*currentUser),
                                                                                   comment);
                       });
    return status;    
}
