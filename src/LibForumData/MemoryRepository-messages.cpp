#include "MemoryRepository.h"

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

StatusCode MemoryRepository::getDiscussionThreadMessagesOfUserByCreated(const IdType& id, std::ostream& output) const
{
    StatusWriter status(output, StatusCode::OK);
    auto performedBy = preparePerformedBy();

    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    collection_.read([&](const EntityCollection& collection)
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
            displayContext.sortOrder == Context::SortOrder::Ascending, [](auto m) { return m; });

        readEvents_.onGetDiscussionThreadMessagesOfUser(createObserverContext(performedBy.get(collection)), **it);
    });
    return status;
}


static StatusCode validateDiscussionMessageContent(const std::string& content, const boost::u32regex& regex, 
                                                   const ConfigConstRef& config)
{
    if (content.empty())
    {
        return StatusCode::INVALID_PARAMETERS;
    }

    auto nrCharacters = countUTF8Characters(content);
    if (nrCharacters > config->discussionThreadMessage.maxContentLength)
    {
        return StatusCode::VALUE_TOO_LONG;
    }
    if (nrCharacters < config->discussionThreadMessage.minContentLength)
    {
        return StatusCode::VALUE_TOO_SHORT;
    }

    try
    {
        if ( ! boost::u32regex_match(content, regex, boost::match_flag_type::format_all))
        {
            return StatusCode::INVALID_PARAMETERS;
        }
    }
    catch(...)
    {
        return StatusCode::INVALID_PARAMETERS;
    }

    return StatusCode::OK;
}


static StatusCode validateMessageCommentContent(const std::string& content, const boost::u32regex& regex,
    const ConfigConstRef& config)
{
    if (content.empty())
    {
        return StatusCode::INVALID_PARAMETERS;
    }

    auto nrCharacters = countUTF8Characters(content);
    if (nrCharacters > config->discussionThreadMessage.maxCommentLength)
    {
        return StatusCode::VALUE_TOO_LONG;
    }
    if (nrCharacters < config->discussionThreadMessage.minCommentLength)
    {
        return StatusCode::VALUE_TOO_SHORT;
    }

    try
    {
        if ( ! boost::u32regex_match(content, regex, boost::match_flag_type::format_all))
        {
            return StatusCode::INVALID_PARAMETERS;
        }
    }
    catch (...)
    {
        return StatusCode::INVALID_PARAMETERS;
    }

    return StatusCode::OK;
}

static StatusCode validateDiscussionMessageChangeReason(const std::string& reason, const boost::u32regex& regex, 
                                                        const ConfigConstRef& config)
{
    if (reason.empty() && 0 == config->discussionThreadMessage.minChangeReasonLength)
    {
        //allow an empty reason only if the minimum configured length is 0
        return StatusCode::OK;
    }

    auto nrCharacters = countUTF8Characters(reason);
    if (nrCharacters > config->discussionThreadMessage.maxChangeReasonLength)
    {
        return StatusCode::VALUE_TOO_LONG;
    }
    if (nrCharacters < config->discussionThreadMessage.minChangeReasonLength)
    {
        return StatusCode::VALUE_TOO_SHORT;
    }

    try
    {
        if ( ! boost::u32regex_match(reason, regex, boost::match_flag_type::format_all))
        {
            return StatusCode::INVALID_PARAMETERS;
        }
    }
    catch(...)
    {
        return StatusCode::INVALID_PARAMETERS;
    }

    return StatusCode::OK;
}

StatusCode MemoryRepository::addNewDiscussionMessageInThread(const IdType& threadId, const std::string& content,
                                                             std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! threadId)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    auto validationCode = validateDiscussionMessageContent(content, validDiscussionMessageContentRegex, getGlobalConfig());
    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
    }
    auto performedBy = preparePerformedBy();

    collection_.write([&](EntityCollection& collection)
                      {
                          auto& threadIndex = collection.threads();
                          auto threadIt = threadIndex.find(threadId);
                          if (threadIt == threadIndex.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }

                          const auto& createdBy = performedBy.getAndUpdate(collection);

                          auto message = std::make_shared<DiscussionThreadMessage>(*createdBy, **threadIt);
                          message->id() = generateUUIDString();
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
                                      collection.modifyDiscussionTagById(tagShared->id(), [&thread](auto& tag)
                                      {
                                          tag.messageCount() += 1;
                                          //notify the thread collection of each tag that the thread has a new message
                                          tag.modifyDiscussionThreadById(thread.id(), [](auto& _) {});
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
                                          category.modifyDiscussionThreadById(thread.id(), [](auto& _) {});
                                      });
                                  }
                              }
                          });

                          collection.modifyUserById(createdBy->id(), [&](User& user)
                          {
                              user.messages().insert(message);
                          });

                          writeEvents_.onAddNewDiscussionThreadMessage(createObserverContext(*createdBy), *message);

                          status.addExtraSafeName("id", message->id());
                          status.addExtraSafeName("parentId", (*threadIt)->id());
                          status.addExtraSafeName("created", message->created());
                      });
    return status;
}

StatusCode MemoryRepository::deleteDiscussionMessage(const IdType& id, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    auto performedBy = preparePerformedBy();

    collection_.write([&](EntityCollection& collection)
                      {
                          auto& indexById = collection.messages().get<EntityCollection::DiscussionThreadMessageCollectionById>();
                          auto it = indexById.find(id);
                          if (it == indexById.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }
                          //make sure the message is not deleted before being passed to the observers
                          writeEvents_.onDeleteDiscussionThreadMessage(
                                  createObserverContext(*performedBy.getAndUpdate(collection)), **it);
                          collection.deleteDiscussionThreadMessage(it);
                      });
    return status;
}

StatusCode MemoryRepository::changeDiscussionThreadMessageContent(const IdType& id, const std::string& newContent, 
                                                                  const std::string& changeReason, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);

    auto config = getGlobalConfig();
    auto contentValidationCode = validateDiscussionMessageContent(newContent, validDiscussionMessageContentRegex, config);
    if (contentValidationCode != StatusCode::OK)
    {
        return status = contentValidationCode;
    }
    auto reasonValidationCode = validateDiscussionMessageChangeReason(changeReason, validDiscussionMessageChangeReasonRegex, 
                                                                      config);
    if (reasonValidationCode != StatusCode::OK)
    {
        return status = reasonValidationCode;
    }
    auto performedBy = preparePerformedBy();

    collection_.write([&](EntityCollection& collection)
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
                              message.lastUpdatedReason() = changeReason;
                              if (&message.createdBy() != performedByPtr.get())
                              {
                                  message.lastUpdatedBy() = performedByPtr;
                              }
                              message.parentThread().resetVisitorsSinceLastEdit();
                              message.parentThread().latestVisibleChange() = message.lastUpdated();
                          });
                          writeEvents_.onChangeDiscussionThreadMessage(createObserverContext(*performedByPtr), **it,
                                  DiscussionThreadMessage::ChangeType::Content);
                      });
    return status;
}

StatusCode MemoryRepository::moveDiscussionThreadMessage(const IdType& messageId, const IdType& intoThreadId, 
                                                         std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! messageId || ! intoThreadId)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    auto performedBy = preparePerformedBy();

    collection_.write([&](EntityCollection& collection)
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

                          auto& message = **messageIt;
                          auto& threadIntoRef = *itInto;
                          auto& threadInto = **itInto;
                    
                          if (&(message.parentThread()) == &threadInto)
                          {
                              status = StatusCode::NO_EFFECT;
                              return;
                          }
                    
                          //make sure the message is not deleted before being passed to the observers
                          writeEvents_.onMoveDiscussionThreadMessage(
                                  createObserverContext(*performedBy.getAndUpdate(collection)), message, threadInto);
                    
                          auto& createdBy = message.createdBy();
                    
                          auto messageClone = std::make_shared<DiscussionThreadMessage>(message, threadInto);
                                              
                          collection.messages().insert(messageClone);
                          collection.modifyDiscussionThread(itInto, [&collection, &messageClone, &threadIntoRef]
                            (DiscussionThread& thread)
                          {
                              thread.messages().insert(messageClone);
                              thread.resetVisitorsSinceLastEdit();
                              thread.latestVisibleChange() = Context::getCurrentTime();

                              for (auto& tagWeak : thread.tagsWeak())
                              {
                                  if (auto tagShared = tagWeak.lock())
                                  {
                                      collection.modifyDiscussionTagById(tagShared->id(), [](auto& tag)
                                      {
                                          tag.messageCount() += 1;
                                      });
                                  }
                              }                          
                              for (auto& categoryWeak : thread.categoriesWeak())
                              {
                                  if (auto categoryShared = categoryWeak.lock())
                                  {
                                      collection.modifyDiscussionCategoryById(categoryShared->id(), 
                                          [&threadIntoRef](auto& category)
                                      {
                                          category.updateMessageCount(threadIntoRef, 1);
                                      });
                                  }
                              }
                          });
                    
                          createdBy.messages().insert(messageClone);
                          //this will also decrease the message count of all tags part of the thread the message is moved from
                          collection.deleteDiscussionThreadMessage(messageIt);
                      });
    return status;
}

StatusCode MemoryRepository::voteDiscussionThreadMessage(const IdType& id, std::ostream& output, bool up)
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
    auto performedBy = preparePerformedBy();

    collection_.write([&](EntityCollection& collection)
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
                              writeEvents_.onDiscussionThreadMessageUpVote(createObserverContext(*currentUser), message);
                          }
                          else
                          {
                              message.addDownVote(std::move(userWeak), timestamp);
                              writeEvents_.onDiscussionThreadMessageDownVote(createObserverContext(*currentUser), message);
                          }
                      });
    return status;
}

StatusCode MemoryRepository::upVoteDiscussionThreadMessage(const IdType& id, std::ostream& output)
{
    return voteDiscussionThreadMessage(id, output, true);
}

StatusCode MemoryRepository::downVoteDiscussionThreadMessage(const IdType& id, std::ostream& output)
{
    return voteDiscussionThreadMessage(id, output, false);
}

StatusCode MemoryRepository::resetVoteDiscussionThreadMessage(const IdType& id, std::ostream& output)
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
    auto performedBy = preparePerformedBy();

    collection_.write([&](EntityCollection& collection)
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

                          writeEvents_.onDiscussionThreadMessageResetVote(createObserverContext(*currentUser), message);
                      });
    return status;
}

template<typename Collection>
static void writeMessageComments(const Collection& collection, std::ostream& output)
{
    auto pageSize = getGlobalConfig()->discussionThreadMessage.maxMessagesCommentsPerPage;
    auto& displayContext = Context::getDisplayContext();

    writeEntitiesWithPagination(collection, "message_comments", output, 
        displayContext.pageNumber, pageSize, displayContext.sortOrder == Context::SortOrder::Ascending, 
        [](const auto& c) { return c; });
}

StatusCode MemoryRepository::getMessageComments(std::ostream& output) const
{
    auto performedBy = preparePerformedBy();

    collection_.read([&](const EntityCollection& collection)
    {
        writeMessageComments(collection.messageCommentsByCreated(), output);
        readEvents_.onGetMessageComments(createObserverContext(performedBy.get(collection)));
    });
    return StatusCode::OK;
}

StatusCode MemoryRepository::getMessageCommentsOfDiscussionThreadMessage(const IdType& id, std::ostream& output) const
{
    StatusWriter status(output, StatusCode::OK);
    auto performedBy = preparePerformedBy();

    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    collection_.read([&](const EntityCollection& collection)
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
        readEvents_.onGetMessageCommentsOfMessage(createObserverContext(performedBy.get(collection)), message);
    });
    return status;
}

StatusCode MemoryRepository::getMessageCommentsOfUser(const IdType& id, std::ostream& output) const
{
    StatusWriter status(output, StatusCode::OK);
    auto performedBy = preparePerformedBy();

    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    collection_.read([&](const EntityCollection& collection)
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
        readEvents_.onGetMessageCommentsOfUser(createObserverContext(performedBy.get(collection)), user);
    });
    return status;
}

StatusCode MemoryRepository::addCommentToDiscussionThreadMessage(const IdType& messageId, const std::string& content,
                                                                 std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! messageId)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    auto validationCode = validateDiscussionMessageContent(content, validDiscussionMessageContentRegex, getGlobalConfig());
    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
    }
    auto performedBy = preparePerformedBy();

    collection_.write([&](EntityCollection& collection)
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

                          collection.modifyDiscussionThreadMessageById((*messageIt)->id(), [&](auto& message)
                          {
                              message.messageComments().insert(comment);
                          });

                          collection.modifyUserById(createdBy->id(), [&](User& user)
                          {
                              user.messageComments().insert(comment);
                          });

                          writeEvents_.onAddCommentToDiscussionThreadMessage(createObserverContext(*createdBy), *comment);

                          status.addExtraSafeName("id", comment->id());
                          status.addExtraSafeName("messageId", (*messageIt)->id());
                          status.addExtraSafeName("created", comment->created());
                      });
    return status;    
}

StatusCode MemoryRepository::setMessageCommentToSolved(const IdType& id, std::ostream& output)
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
    auto performedBy = preparePerformedBy();

    collection_.write([&](EntityCollection& collection)
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
                          comment.executeActionWithParentMessageIfAvailable([&](DiscussionThreadMessage& message)
                          {
                              message.solvedCommentsCount() += 1;
                          });

                          writeEvents_.onSolveDiscussionThreadMessageComment(createObserverContext(*currentUser), comment);
                      });
    return status;    
}
