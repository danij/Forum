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

void MemoryRepository::getDiscussionThreadsByName(std::ostream& output) const
{
    auto performedBy = preparePerformedBy();

    collection_.read([&](const EntityCollection& collection)
                     {
                         const auto& threads = collection.threadsByName();
                         BoolTemporaryChanger changer(serializationSettings.hideDiscussionThreadMessages, true);

                         if (Context::getDisplayContext().sortOrder == Context::SortOrder::Ascending)
                         {
                             writeSingleObjectSafeName(output, "threads", Json::enumerate(threads.begin(), threads.end()));
                         }
                         else
                         {
                             writeSingleObjectSafeName(output, "threads", Json::enumerate(threads.rbegin(), threads.rend()));
                         }
                         readEvents_.onGetDiscussionThreads(createObserverContext(performedBy.get(collection)));
                     });
}

void MemoryRepository::getDiscussionThreadsByCreated(std::ostream& output) const
{
    auto performedBy = preparePerformedBy();

    collection_.read([&](const EntityCollection& collection)
                     {
                         const auto& threads = collection.threadsByCreated();
                         BoolTemporaryChanger changer(serializationSettings.hideDiscussionThreadMessages, true);

                         if (Context::getDisplayContext().sortOrder == Context::SortOrder::Ascending)
                         {
                             writeSingleObjectSafeName(output, "threads",
                                                       Json::enumerate(threads.begin(), threads.end()));
                         }
                         else
                         {
                             writeSingleObjectSafeName(output, "threads",
                                                       Json::enumerate(threads.rbegin(), threads.rend()));
                         }
                         readEvents_.onGetDiscussionThreads(createObserverContext(performedBy.get(collection)));
                     });
}

void MemoryRepository::getDiscussionThreadsByLastUpdated(std::ostream& output) const
{
    auto performedBy = preparePerformedBy();

    collection_.read([&](const EntityCollection& collection)
                     {
                         const auto& threads = collection.threadsByLastUpdated();
                         BoolTemporaryChanger changer(serializationSettings.hideDiscussionThreadMessages, true);

                         if (Context::getDisplayContext().sortOrder == Context::SortOrder::Ascending)
                         {
                             writeSingleObjectSafeName(output, "threads",
                                                       Json::enumerate(threads.begin(), threads.end()));
                         }
                         else
                         {
                             writeSingleObjectSafeName(output, "threads",
                                                       Json::enumerate(threads.rbegin(), threads.rend()));
                         }
                         readEvents_.onGetDiscussionThreads(createObserverContext(performedBy.get(collection)));
                     });
}

void MemoryRepository::getDiscussionThreadsByMessageCount(std::ostream& output) const
{
    auto performedBy = preparePerformedBy();

    collection_.read([&](const EntityCollection& collection)
                     {
                         const auto& threads = collection.threadsByMessageCount();
                         BoolTemporaryChanger changer(serializationSettings.hideDiscussionThreadMessages, true);

                         if (Context::getDisplayContext().sortOrder == Context::SortOrder::Ascending)
                         {
                             writeSingleObjectSafeName(output, "threads",
                                                       Json::enumerate(threads.begin(), threads.end()));
                         }
                         else
                         {
                             writeSingleObjectSafeName(output, "threads",
                                                       Json::enumerate(threads.rbegin(), threads.rend()));
                         }
                         readEvents_.onGetDiscussionThreads(createObserverContext(performedBy.get(collection)));
                     });
}

void MemoryRepository::getDiscussionThreadById(const IdType& id, std::ostream& output) const
{
    auto performedBy = preparePerformedBy();

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
                         readEvents_.onGetDiscussionThreadById(createObserverContext(performedBy.get(collection)), id);
                     });
}

void MemoryRepository::getDiscussionThreadsOfUserByName(const IdType& id, std::ostream& output) const
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

                         const auto& threads = (*it)->threadsByName();
                         BoolTemporaryChanger _(serializationSettings.hideDiscussionThreadCreatedBy, true);
                         BoolTemporaryChanger __(serializationSettings.hideDiscussionThreadMessages, true);

                         if (Context::getDisplayContext().sortOrder == Context::SortOrder::Ascending)
                         {
                             writeSingleObjectSafeName(output, "threads", 
                                                       Json::enumerate(threads.begin(), threads.end()));
                         }
                         else
                         {
                             writeSingleObjectSafeName(output, "threads", 
                                                       Json::enumerate(threads.rbegin(), threads.rend()));
                         }
                         readEvents_.onGetDiscussionThreadsOfUser(createObserverContext(performedBy.get(collection)),
                                                                  **it);
                     });
}

void MemoryRepository::getDiscussionThreadsOfUserByCreated(const IdType& id, std::ostream& output) const
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

                         const auto& threads = (*it)->threadsByCreated();
                         BoolTemporaryChanger _(serializationSettings.hideDiscussionThreadCreatedBy, true);
                         BoolTemporaryChanger __(serializationSettings.hideDiscussionThreadMessages, true);
                         if (Context::getDisplayContext().sortOrder == Context::SortOrder::Ascending)
                         {
                             writeSingleObjectSafeName(output, "threads",
                                                       Json::enumerate(threads.begin(), threads.end()));
                         }
                         else
                         {
                             writeSingleObjectSafeName(output, "threads",
                                                       Json::enumerate(threads.rbegin(), threads.rend()));
                         }
                         readEvents_.onGetDiscussionThreadsOfUser(createObserverContext(performedBy.get(collection)),
                                                                  **it);
                     });
}

void MemoryRepository::getDiscussionThreadsOfUserByLastUpdated(const IdType& id, std::ostream& output) const
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

                         const auto& threads = (*it)->threadsByLastUpdated();
                         BoolTemporaryChanger _(serializationSettings.hideDiscussionThreadCreatedBy, true);
                         BoolTemporaryChanger __(serializationSettings.hideDiscussionThreadMessages, true);
                         if (Context::getDisplayContext().sortOrder == Context::SortOrder::Ascending)
                         {
                             writeSingleObjectSafeName(output, "threads",
                                                       Json::enumerate(threads.begin(), threads.end()));
                         } else
                         {
                             writeSingleObjectSafeName(output, "threads",
                                                       Json::enumerate(threads.rbegin(), threads.rend()));
                         }
                         readEvents_.onGetDiscussionThreadsOfUser(createObserverContext(performedBy.get(collection)),
                                                                  **it);
                     });
}

void MemoryRepository::getDiscussionThreadsOfUserByMessageCount(const IdType& id, std::ostream& output) const
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

                         const auto& threads = (*it)->threadsByMessageCount();
                         BoolTemporaryChanger _(serializationSettings.hideDiscussionThreadCreatedBy, true);
                         BoolTemporaryChanger __(serializationSettings.hideDiscussionThreadMessages, true);
                         if (Context::getDisplayContext().sortOrder == Context::SortOrder::Ascending)
                         {
                             writeSingleObjectSafeName(output, "threads",
                                                       Json::enumerate(threads.begin(), threads.end()));
                         } else
                         {
                             writeSingleObjectSafeName(output, "threads",
                                                       Json::enumerate(threads.rbegin(), threads.rend()));
                         }
                         readEvents_.onGetDiscussionThreadsOfUser(createObserverContext(performedBy.get(collection)),
                                                                  **it);
                     });
}

static StatusCode validateDiscussionThreadName(const std::string& name, const boost::u32regex& regex, 
                                               const ConfigConstRef& config)
{
    if (name.empty())
    {
        return StatusCode::INVALID_PARAMETERS;
    }
    try
    {
        if ( ! boost::u32regex_match(name, regex, boost::match_flag_type::format_all))
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
    auto validationCode = validateDiscussionThreadName(name, validDiscussionThreadNameRegex, getGlobalConfig());
    if (validationCode != StatusCode::OK)
    {
        status = validationCode;
        return;
    }

    auto performedBy = preparePerformedBy();

    collection_.write([&](EntityCollection& collection)
                      {
                          const auto& createdBy = performedBy.getAndUpdate(collection);

                          auto thread = std::make_shared<DiscussionThread>(*createdBy);
                          thread->id() = generateUUIDString();
                          thread->name() = name;
                          thread->created() = thread->lastUpdated() = Context::getCurrentTime();

                          collection.threads().insert(thread);
                          createdBy->threads().insert(thread);

                          writeEvents_.onAddNewDiscussionThread(createObserverContext(*createdBy), *thread);

                          status.addExtraSafeName("id", thread->id());
                          status.addExtraSafeName("name", thread->name());
                          status.addExtraSafeName("created", thread->created());
                      });
}

void MemoryRepository::changeDiscussionThreadName(const IdType& id, const std::string& newName, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    auto validationCode = validateDiscussionThreadName(newName, validDiscussionThreadNameRegex, getGlobalConfig());
    if (validationCode != StatusCode::OK)
    {
        status = validationCode;
    }
    auto performedBy = preparePerformedBy();

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
                          writeEvents_.onChangeDiscussionThread(
                                  createObserverContext(*performedBy.getAndUpdate(collection)), **it,
                                  DiscussionThread::ChangeType::Name);
                      });
}

void MemoryRepository::deleteDiscussionThread(const IdType& id, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    auto performedBy = preparePerformedBy();

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
                          writeEvents_.onDeleteDiscussionThread(
                                  createObserverContext(*performedBy.getAndUpdate(collection)), **it);
                          collection.deleteDiscussionThread(it);
                      });
}

void MemoryRepository::mergeDiscussionThreads(const IdType& fromId, const IdType& intoId, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! fromId || ! intoId)
    {
        status = StatusCode::INVALID_PARAMETERS;
        return;
    }
    if (fromId == intoId)
    {
        status = StatusCode::NO_EFFECT;
        return;
    }

    auto performedBy = preparePerformedBy();

    collection_.write([&](EntityCollection& collection)
                    {
                        auto& indexById = collection.threads().get<EntityCollection::DiscussionThreadCollectionById>();
                        auto itFrom = indexById.find(fromId);
                        if (itFrom == indexById.end())
                        {
                            status = StatusCode::NOT_FOUND;
                            return;
                        }
                        auto itInto = indexById.find(intoId);
                        if (itInto == indexById.end())
                        {
                            status = StatusCode::NOT_FOUND;
                            return;
                        }
                        //make sure the thread is not deleted before being passed to the observers
                        writeEvents_.onMergeDiscussionThreads(
                                createObserverContext(*performedBy.getAndUpdate(collection)), **itFrom, **itInto);

                        for (auto& message : (*itFrom)->messages())
                        {
                            auto& createdBy = message->createdBy();

                            auto messageClone = std::make_shared<DiscussionThreadMessage>(*message, **itInto);

                            collection.messages().insert(messageClone);
                            collection.modifyDiscussionThread(itInto, [&messageClone](DiscussionThread& thread)
                            {
                                thread.messages().insert(messageClone);
                            });

                            createdBy.messages().insert(messageClone);
                        }

                        collection.deleteDiscussionThread(itFrom);
                    });
}
