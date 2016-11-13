#include <boost/regex/icu.hpp>

#include "Configuration.h"
#include "ContextProviders.h"
#include "EntitySerialization.h"
#include "MemoryRepository.h"
#include "OutputHelpers.h"
#include "RandomGenerator.h"
#include "StateHelpers.h"
#include "StringHelpers.h"

using namespace Forum;
using namespace Forum::Configuration;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;

void MemoryRepository::getDiscussionThreadsByName(std::ostream& output) const
{
    auto performedBy = preparePerformedBy(*this);

    collection_.read([&](const EntityCollection& collection)
                     {
                         const auto& threads = collection.threadsByName();
                         BoolTemporaryChanger changer(serializationSettings.hideDiscussionThreadMessages, true);

                         writeSingleObjectSafeName(output, "threads", Json::enumerate(threads.begin(), threads.end()));
                         observers_.onGetDiscussionThreads(performedBy.get(collection));
                     });
}

void MemoryRepository::getDiscussionThreadsByCreated(bool ascending, std::ostream& output) const
{
    auto performedBy = preparePerformedBy(*this);

    collection_.read([&](const EntityCollection& collection)
                     {
                         const auto& threads = collection.threadsByCreated();
                         BoolTemporaryChanger changer(serializationSettings.hideDiscussionThreadMessages, true);

                         if (ascending)
                         {
                             writeSingleObjectSafeName(output, "threads",
                                                       Json::enumerate(threads.begin(), threads.end()));
                         }
                         else
                         {
                             writeSingleObjectSafeName(output, "threads",
                                                       Json::enumerate(threads.rbegin(), threads.rend()));
                         }
                         observers_.onGetDiscussionThreads(performedBy.get(collection));
                     });
}

void MemoryRepository::getDiscussionThreadsByCreatedAscending(std::ostream& output) const
{
    getDiscussionThreadsByCreated(true, output);
}

void MemoryRepository::getDiscussionThreadsByCreatedDescending(std::ostream& output) const
{
    getDiscussionThreadsByCreated(false, output);
}

void MemoryRepository::getDiscussionThreadsByLastUpdated(bool ascending, std::ostream& output) const
{
    auto performedBy = preparePerformedBy(*this);

    collection_.read([&](const EntityCollection& collection)
                     {
                         const auto& threads = collection.threadsByLastUpdated();
                         BoolTemporaryChanger changer(serializationSettings.hideDiscussionThreadMessages, true);

                         if (ascending)
                         {
                             writeSingleObjectSafeName(output, "threads",
                                                       Json::enumerate(threads.begin(), threads.end()));
                         }
                         else
                         {
                             writeSingleObjectSafeName(output, "threads",
                                                       Json::enumerate(threads.rbegin(), threads.rend()));
                         }
                         observers_.onGetDiscussionThreads(performedBy.get(collection));
                     });
}

void MemoryRepository::getDiscussionThreadsByLastUpdatedAscending(std::ostream& output) const
{
    getDiscussionThreadsByLastUpdated(true, output);
}

void MemoryRepository::getDiscussionThreadsByLastUpdatedDescending(std::ostream& output) const
{
    getDiscussionThreadsByLastUpdated(false, output);
}

void MemoryRepository::getDiscussionThreadById(const IdType& id, std::ostream& output) const
{
    auto performedBy = preparePerformedBy(*this);

    collection_.read([&](const EntityCollection& collection)
                     {
                         const auto& index = collection.threadsById();
                         auto it = index.find(id);
                         if (it == index.end())
                         {
                             writeStatusCode(output, StatusCode::NOT_FOUND);
                         }
                         else
                         {
                             (*it)->visited().fetch_add(1);
                             BoolTemporaryChanger _(serializationSettings.hideDiscussionThreadMessageParentThread, true);
                             writeSingleObjectSafeName(output, "thread", **it);
                         }
                         observers_.onGetDiscussionThreadById(performedBy.get(collection), id);
                     });
}

void MemoryRepository::getDiscussionThreadsOfUserByName(const IdType& id, std::ostream& output) const
{
    auto performedBy = preparePerformedBy(*this);

    collection_.read([&](const EntityCollection& collection)
                     {
                         const auto& indexById = collection.usersById();
                         auto it = indexById.find(id);
                         if (it == indexById.end())
                         {
                             writeStatusCode(output, StatusCode::NOT_FOUND);
                             return;
                         }

                         const auto& threads = (*it)->threadsByName();
                         BoolTemporaryChanger _(serializationSettings.hideDiscussionThreadCreatedBy, true);
                         BoolTemporaryChanger __(serializationSettings.hideDiscussionThreadMessages, true);
                         writeSingleObjectSafeName(output, "threads", Json::enumerate(threads.begin(), threads.end()));
                         observers_.onGetDiscussionThreadsOfUser(performedBy.get(collection), **it);
                     });
}

void MemoryRepository::getDiscussionThreadsOfUserByCreated(bool ascending, const IdType& id, std::ostream& output) const
{
    auto performedBy = preparePerformedBy(*this);

    collection_.read([&](const EntityCollection& collection)
                     {
                         const auto& indexById = collection.usersById();
                         auto it = indexById.find(id);
                         if (it == indexById.end())
                         {
                             writeStatusCode(output, StatusCode::NOT_FOUND);
                             return;
                         }

                         const auto& threads = (*it)->threadsByCreated();
                         BoolTemporaryChanger _(serializationSettings.hideDiscussionThreadCreatedBy, true);
                         BoolTemporaryChanger __(serializationSettings.hideDiscussionThreadMessages, true);
                         if (ascending)
                         {
                             writeSingleObjectSafeName(output, "threads",
                                                       Json::enumerate(threads.begin(), threads.end()));
                         }
                         else
                         {
                             writeSingleObjectSafeName(output, "threads",
                                                       Json::enumerate(threads.rbegin(), threads.rend()));
                         }
                         observers_.onGetDiscussionThreadsOfUser(performedBy.get(collection), **it);
                     });
}

void MemoryRepository::getDiscussionThreadsOfUserByCreatedAscending(const IdType& id, std::ostream& output) const
{
    getDiscussionThreadsOfUserByCreated(true, id, output);
}

void MemoryRepository::getDiscussionThreadsOfUserByCreatedDescending(const IdType& id, std::ostream& output) const
{
    getDiscussionThreadsOfUserByCreated(false, id, output);
}

void MemoryRepository::getDiscussionThreadsOfUserByLastUpdated(bool ascending, const IdType& id,
                                                               std::ostream& output) const
{
    auto performedBy = preparePerformedBy(*this);

    collection_.read([&](const EntityCollection& collection)
                     {
                         const auto& indexById = collection.usersById();
                         auto it = indexById.find(id);
                         if (it == indexById.end())
                         {
                             writeStatusCode(output, StatusCode::NOT_FOUND);
                             return;
                         }

                         const auto& threads = (*it)->threadsByLastUpdated();
                         BoolTemporaryChanger _(serializationSettings.hideDiscussionThreadCreatedBy, true);
                         BoolTemporaryChanger __(serializationSettings.hideDiscussionThreadMessages, true);
                         if (ascending)
                         {
                             writeSingleObjectSafeName(output, "threads",
                                                       Json::enumerate(threads.begin(), threads.end()));
                         } else
                         {
                             writeSingleObjectSafeName(output, "threads",
                                                       Json::enumerate(threads.rbegin(), threads.rend()));
                         }
                         observers_.onGetDiscussionThreadsOfUser(performedBy.get(collection), **it);
                     });
}

void MemoryRepository::getDiscussionThreadsOfUserByLastUpdatedAscending(const IdType& id, std::ostream& output) const
{
    getDiscussionThreadsOfUserByLastUpdated(true, id, output);
}

void MemoryRepository::getDiscussionThreadsOfUserByLastUpdatedDescending(const IdType& id, std::ostream& output) const
{
    getDiscussionThreadsOfUserByLastUpdated(false, id, output);
}

static const auto validDiscussionThreadNameRegex = boost::make_u32regex("^[^[:space:]]+.*[^[:space:]]+$");
static const auto validDiscussionMessageContentRegex = boost::make_u32regex("^[^[:space:]]+.*[^[:space:]]+$");

static StatusCode validateDiscussionThreadName(const std::string& name, const ConfigConstRef& config)
{
    if (name.empty())
    {
        return StatusCode::INVALID_PARAMETERS;
    }
    try
    {
        if ( ! boost::u32regex_match(name, validDiscussionThreadNameRegex, boost::match_flag_type::format_all))
        {
            return StatusCode::INVALID_PARAMETERS;
        }
    }
    catch(...)
    {
        return StatusCode::INVALID_PARAMETERS;
    }

    auto nrCharacters = countUTF8Characters(name);
    if (nrCharacters > config->discussionThread.maxNameLength)
    {
        return StatusCode::VALUE_TOO_LONG;
    }
    if (nrCharacters < config->discussionThread.minNameLength)
    {
        return StatusCode::VALUE_TOO_SHORT;
    }

    return StatusCode::OK;
}

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

void MemoryRepository::addNewDiscussionThread(const std::string& name, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    auto validationCode = validateDiscussionThreadName(name, Configuration::getGlobalConfig());
    if (validationCode != StatusCode::OK)
    {
        status = validationCode;
        return;
    }

    auto performedBy = preparePerformedBy(*this);

    collection_.write([&](EntityCollection& collection)
                      {
                          const auto& createdBy = performedBy.getAndUpdate(collection);

                          auto thread = std::make_shared<DiscussionThread>(*createdBy);
                          thread->id() = generateUUIDString();
                          thread->name() = name;
                          thread->created() = thread->lastUpdated() = Context::getCurrentTime();

                          collection.threads().insert(thread);
                          createdBy->threads().insert(thread);

                          observers_.onAddNewDiscussionThread(*createdBy, *thread);

                          status.addExtraSafeName("id", thread->id());
                          status.addExtraSafeName("name", thread->name());
                          status.addExtraSafeName("created", thread->created());
                      });
}

void MemoryRepository::changeDiscussionThreadName(const IdType& id, const std::string& newName, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    auto validationCode = validateDiscussionThreadName(newName, Configuration::getGlobalConfig());
    if (validationCode != StatusCode::OK)
    {
        status = validationCode;
    }
    auto performedBy = preparePerformedBy(*this);

    collection_.write([&](EntityCollection& collection)
                      {
                          auto& indexById = collection.threads()
                                  .get<EntityCollection::DiscussionThreadCollectionById>();
                          auto it = indexById.find(id);
                          if (it == indexById.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }
                          collection.modifyDiscussionThread(it, [&newName](DiscussionThread& thread)
                          {
                              thread.name() = newName;
                              thread.lastUpdated() = Context::getCurrentTime();
                          });
                          observers_.onChangeDiscussionThread(*performedBy.getAndUpdate(collection), **it,
                                                            DiscussionThread::ChangeType::Name);
                      });
}

void MemoryRepository::deleteDiscussionThread(const IdType& id, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    auto performedBy = preparePerformedBy(*this);

    if ( ! id)
    {
        status = StatusCode::INVALID_PARAMETERS;
        return;
    }

    collection_.write([&](EntityCollection& collection)
                      {
                          auto& indexById = collection.threads().get<EntityCollection::DiscussionThreadCollectionById>();
                          auto it = indexById.find(id);
                          if (it == indexById.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }
                          //make sure the thread is not deleted before being passed to the observers
                          observers_.onDeleteDiscussionThread(*performedBy.getAndUpdate(collection), **it);
                          collection.deleteDiscussionThread(it);
                      });
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

    auto validationCode = validateDiscussionMessageContent(content, Configuration::getGlobalConfig());
    if (validationCode != StatusCode::OK)
    {
        status = validationCode;
        return;
    }

    auto performedBy = preparePerformedBy(*this);

    collection_.write([&](EntityCollection& collection)
                      {
                          auto& threadIndex = collection.threads();
                          auto threadIt = threadIndex.find(threadId);
                          if (threadIt == threadIndex.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }
                          auto& thread = *threadIt;

                          const auto& createdBy = performedBy.getAndUpdate(collection);

                          auto message = std::make_shared<DiscussionMessage>(*createdBy, *thread);
                          message->id() = generateUUIDString();
                          message->content() = content;
                          message->created() = Context::getCurrentTime();

                          collection.messages().insert(message);
                          thread->messages().insert(message);
                          createdBy->messages().insert(message);

                          observers_.onAddNewDiscussionMessage(*createdBy, *message);

                          status.addExtraSafeName("id", message->id());
                          status.addExtraSafeName("parentId", thread->id());
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

    auto performedBy = preparePerformedBy(*this);
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
                          observers_.onDeleteDiscussionMessage(*performedBy.getAndUpdate(collection), **it);
                          collection.deleteDiscussionMessage(it);
                      });
}
