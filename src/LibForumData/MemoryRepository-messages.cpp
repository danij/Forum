#include "MemoryRepository.h"

#include "Configuration.h"
#include "ContextProviders.h"
#include "EntitySerialization.h"
#include "OutputHelpers.h"
#include "RandomGenerator.h"
#include "StateHelpers.h"
#include "StringHelpers.h"

#include <boost/regex/icu.hpp>

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


static const auto validDiscussionMessageContentRegex = boost::make_u32regex("^[^[:space:]]+.*[^[:space:]]+$");

static StatusCode validateDiscussionMessageContent(const std::string& content, const ConfigConstRef& config)
{
    if (content.empty())
    {
        return StatusCode::INVALID_PARAMETERS;
    }
    try
    {
        if ( ! boost::u32regex_match(content, validDiscussionMessageContentRegex, boost::match_flag_type::format_all))
        {
            return StatusCode::INVALID_PARAMETERS;
        }
    }
    catch(...)
    {
        return StatusCode::INVALID_PARAMETERS;
    }

    auto nrCharacters = countUTF8Characters(content);
    if (nrCharacters > config->discussionMessage.maxContentLength)
    {
        return StatusCode::VALUE_TOO_LONG;
    }
    if (nrCharacters < config->discussionMessage.minContentLength)
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

    auto validationCode = validateDiscussionMessageContent(content, getGlobalConfig());
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

                          auto message = std::make_shared<DiscussionMessage>(*createdBy, **threadIt);
                          message->id() = generateUUIDString();
                          message->content() = content;
                          message->created() = Context::getCurrentTime();

                          collection.messages().insert(message);
                          collection.modifyDiscussionThread(threadIt, [&message](DiscussionThread& thread)
                          {
                              thread.messages().insert(message);
                          });

                          createdBy->messages().insert(message);

                          writeEvents_.onAddNewDiscussionMessage(createObserverContext(*createdBy), *message);

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
                          auto& indexById = collection.messages().get<EntityCollection::DiscussionMessageCollectionById>();
                          auto it = indexById.find(id);
                          if (it == indexById.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }
                          //make sure the message is not deleted before being passed to the observers
                          writeEvents_.onDeleteDiscussionMessage(
                                  createObserverContext(*performedBy.getAndUpdate(collection)), **it);
                          collection.deleteDiscussionMessage(it);
                      });
}
