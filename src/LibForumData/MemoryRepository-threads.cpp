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

void MemoryRepository::getDiscussionThreadCount(std::ostream& output) const
{
    auto performedBy = preparePerformedBy(*this);

    collection_.read([&](const EntityCollection& collection)
                     {
                         auto count = collection.threadsById().size();
                         writeSingleValueSafeName(output, "count", count);

                         observers_.onGetDiscussionThreadCount(performedBy.get(collection));
                     });
}

void MemoryRepository::getDiscussionThreadsByName(std::ostream& output) const
{
    auto performedBy = preparePerformedBy(*this);

    collection_.read([&](const EntityCollection& collection)
                     {
                         const auto& threads = collection.threadsByName();
                         writeSingleObjectSafeName(output, "threads", Json::enumerate(threads.begin(), threads.end()));
                         observers_.onGetDiscussionThreads(performedBy.get(collection));
                     });
}

void MemoryRepository::getDiscussionThreadsByCreated(std::ostream& output) const
{
    auto performedBy = preparePerformedBy(*this);

    collection_.read([&](const EntityCollection& collection)
                     {
                         const auto& threads = collection.threadsByCreated();
                         writeSingleObjectSafeName(output, "threads", Json::enumerate(threads.begin(), threads.end()));
                         observers_.onGetDiscussionThreads(performedBy.get(collection));
                     });
}

void MemoryRepository::getDiscussionThreadsByLastUpdated(std::ostream& output) const
{
    auto performedBy = preparePerformedBy(*this);

    collection_.read([&](const EntityCollection& collection)
                     {
                         const auto& threads = collection.threadsByLastUpdated();
                         writeSingleObjectSafeName(output, "threads", Json::enumerate(threads.begin(), threads.end()));
                         observers_.onGetDiscussionThreads(performedBy.get(collection));
                     });
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
                         BoolTemporaryChanger settingsChanger(serializationSettings.hideDiscussionThreadCreatedBy, true);
                         writeSingleObjectSafeName(output, "threads", Json::enumerate(threads.begin(), threads.end()));
                         observers_.onGetDiscussionThreadsOfUser(performedBy.get(collection), **it);
                     });
}

void MemoryRepository::getDiscussionThreadsOfUserByCreated(const IdType& id, std::ostream& output) const
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
                         BoolTemporaryChanger settingsChanger(serializationSettings.hideDiscussionThreadCreatedBy, true);
                         writeSingleObjectSafeName(output, "threads", Json::enumerate(threads.begin(), threads.end()));
                         observers_.onGetDiscussionThreadsOfUser(performedBy.get(collection), **it);
                     });
}

void MemoryRepository::getDiscussionThreadsOfUserByLastUpdated(const IdType& id, std::ostream& output) const
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
                         BoolTemporaryChanger settingsChanger(serializationSettings.hideDiscussionThreadCreatedBy, true);
                         writeSingleObjectSafeName(output, "threads", Json::enumerate(threads.begin(), threads.end()));
                         observers_.onGetDiscussionThreadsOfUser(performedBy.get(collection), **it);
                     });
}


static const auto validThreadNameRegex = boost::make_u32regex("^[^[:space:]]+.*[^[:space:]]+$");

static StatusCode validateDiscussionThreadName(const std::string& name, const ConfigConstRef& config)
{
    if (name.empty())
    {
        return StatusCode::INVALID_PARAMETERS;
    }
    try
    {
        if ( ! boost::u32regex_match(name, validThreadNameRegex, boost::match_flag_type::format_all))
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
