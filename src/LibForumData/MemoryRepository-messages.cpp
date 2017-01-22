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

void MemoryRepository::getDiscussionThreadMessagesOfUserByCreated(const IdType& id, std::ostream& output) const
{
    auto performedBy = preparePerformedBy();

    collection_.read([&](const EntityCollection& collection)
    {
        const auto& indexById = collection.usersById();
        auto it = indexById.find(id);
        if (it == indexById.end())
        {
            writeStatusCode(output, StatusCode::NOT_FOUND);
            return;
        }

        const auto& messages = (*it)->messagesByCreated();
        BoolTemporaryChanger _(serializationSettings.hideDiscussionThreadCreatedBy, true);
        BoolTemporaryChanger __(serializationSettings.hideDiscussionThreadMessageCreatedBy, true);
        BoolTemporaryChanger ___(serializationSettings.hideDiscussionThreadMessages, true);
        if (Context::getDisplayContext().sortOrder == Context::SortOrder::Ascending)
        {
            writeSingleObjectSafeName(output, "messages",
                Json::enumerate(messages.begin(), messages.end()));
        }
        else
        {
            writeSingleObjectSafeName(output, "messages",
                Json::enumerate(messages.rbegin(), messages.rend()));
        }
        readEvents_.onGetDiscussionThreadMessagesOfUser(createObserverContext(performedBy.get(collection)), **it);
    });
}


static StatusCode validateDiscussionMessageContent(const std::string& content, const boost::u32regex& regex, 
                                                   const ConfigConstRef& config)
{
    if (content.empty())
    {
        return StatusCode::INVALID_PARAMETERS;
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

    auto nrCharacters = countUTF8Characters(content);
    if (nrCharacters > config->discussionThreadMessage.maxContentLength)
    {
        return StatusCode::VALUE_TOO_LONG;
    }
    if (nrCharacters < config->discussionThreadMessage.minContentLength)
    {
        return StatusCode::VALUE_TOO_SHORT;
    }

    return StatusCode::OK;
}

void MemoryRepository::addNewDiscussionMessageInThread(const IdType& threadId, const std::string& content,
                                                       std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! threadId)
    {
        status = StatusCode::INVALID_PARAMETERS;
        return;
    }

    auto validationCode = validateDiscussionMessageContent(content, validDiscussionMessageContentRegex, getGlobalConfig());
    if (validationCode != StatusCode::OK)
    {
        status = validationCode;
        return;
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
                          message->created() = Context::getCurrentTime();
                          message->creationDetails().ip = Context::getCurrentUserIpAddress();
                          message->creationDetails().userAgent = Context::getCurrentUserBrowserUserAgent();

                          collection.messages().insert(message);
                          collection.modifyDiscussionThread(threadIt, [&message](DiscussionThread& thread)
                          {
                              thread.messages().insert(message);
                          });

                          createdBy->messages().insert(message);

                          writeEvents_.onAddNewDiscussionThreadMessage(createObserverContext(*createdBy), *message);

                          status.addExtraSafeName("id", message->id());
                          status.addExtraSafeName("parentId", (*threadIt)->id());
                          status.addExtraSafeName("created", message->created());
                      });
}

void MemoryRepository::deleteDiscussionMessage(const IdType& id, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! id)
    {
        status = StatusCode::INVALID_PARAMETERS;
        return;
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
}

void MemoryRepository::changeDiscussionThreadMessageContent(const IdType& id, const std::string& newContent, 
                                                            std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    auto validationCode = validateDiscussionMessageContent(newContent, validDiscussionMessageContentRegex, getGlobalConfig());
    if (validationCode != StatusCode::OK)
    {
        status = validationCode;
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
                              message.lastUpdated() = Context::getCurrentTime();
                              message.lastUpdatedDetails().ip = Context::getCurrentUserIpAddress();
                              message.lastUpdatedDetails().userAgent = Context::getCurrentUserBrowserUserAgent();
                              if (&message.createdBy() != performedByPtr.get())
                              {
                                  message.lastUpdatedDetails().by = performedByPtr;
                              }
                          });
                          writeEvents_.onChangeDiscussionThreadMessage(createObserverContext(*performedByPtr), **it,
                                  DiscussionThreadMessage::ChangeType::Content);
                      });
}

void MemoryRepository::moveDiscussionThreadMessage(const IdType& messageId, const IdType& intoThreadId, 
                                                   std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! messageId || ! intoThreadId)
    {
        status = StatusCode::INVALID_PARAMETERS;
        return;
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

                        if (&((**messageIt).parentThread()) == (*itInto).get())
                        {
                            status = StatusCode::NO_EFFECT;
                            return;
                        }

                        //make sure the message is not deleted before being passed to the observers
                        writeEvents_.onMoveDiscussionThreadMessage(
                                createObserverContext(*performedBy.getAndUpdate(collection)), **messageIt, **itInto);

                        auto& createdBy = (*messageIt)->createdBy();

                        auto messageClone = std::make_shared<DiscussionThreadMessage>(**messageIt, **itInto);

                        collection.messages().insert(messageClone);
                        collection.modifyDiscussionThread(itInto, [&messageClone](DiscussionThread& thread)
                        {
                            thread.messages().insert(messageClone);
                        });

                        createdBy.messages().insert(messageClone);

                        collection.deleteDiscussionThreadMessage(messageIt);
                    });
}
