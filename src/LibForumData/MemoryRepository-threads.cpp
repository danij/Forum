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

template<typename ThreadsCollection>
static void writeDiscussionThreads(ThreadsCollection&& threads, std::ostream& output, const IdType& currentUserId)
{
    BoolTemporaryChanger _(serializationSettings.visitedThreadSinceLastChange, false);

    auto writeInterceptor = [&](auto currentThread)
    {
        bool visitedThreadSinceLastChange = false;
        if (currentThread && (currentUserId != AnonymousUserId))
        {
            visitedThreadSinceLastChange = currentThread->hasVisitedSinceLastEdit(currentUserId);
        }
        serializationSettings.visitedThreadSinceLastChange = visitedThreadSinceLastChange;
        return currentThread;
    };

    auto pageSize = getGlobalConfig()->discussionThread.maxThreadsPerPage;
    auto& displayContext = Context::getDisplayContext();

    writeEntitiesWithPagination(threads, "threads", output, displayContext.pageNumber, pageSize,
        displayContext.sortOrder == Context::SortOrder::Ascending, writeInterceptor);
}

template<typename ThreadsIndexFn>
static void writeDiscussionThreads(std::ostream& output, PerformedByWithLastSeenUpdateGuard&& performedBy,
    const ResourceGuard<EntityCollection>& collection_, const ReadEvents& readEvents_, ThreadsIndexFn&& threadsIndexFn)
{
    collection_.read([&](const EntityCollection& collection)
                     {
                         auto& currentUser = performedBy.get(collection);
                         const auto threads = threadsIndexFn(collection);

                         BoolTemporaryChanger _(serializationSettings.hideDiscussionThreadMessages, true);

                         writeDiscussionThreads(threads, output, currentUser.id());

                         readEvents_.onGetDiscussionThreads(createObserverContext(currentUser));
                     });
    
}

void MemoryRepository::getDiscussionThreadsByName(std::ostream& output) const
{
    writeDiscussionThreads(output, preparePerformedBy(), collection_, readEvents_, [](const auto& collection)
    {
        return collection.threadsByName();
    });
}

void MemoryRepository::getDiscussionThreadsByCreated(std::ostream& output) const
{
    writeDiscussionThreads(output, preparePerformedBy(), collection_, readEvents_, [](const auto& collection)
    {
        return collection.threadsByCreated();
    });
}

void MemoryRepository::getDiscussionThreadsByLastUpdated(std::ostream& output) const
{
    writeDiscussionThreads(output, preparePerformedBy(), collection_, readEvents_, [](const auto& collection)
    {
        return collection.threadsByLastUpdated();
    });
}

void MemoryRepository::getDiscussionThreadsByMessageCount(std::ostream& output) const
{
    writeDiscussionThreads(output, preparePerformedBy(), collection_, readEvents_, [](const auto& collection)
    {
        return collection.threadsByMessageCount();
    });
}

void MemoryRepository::getDiscussionThreadById(const IdType& id, std::ostream& output)
{
    auto performedBy = preparePerformedBy();
    bool addUserToVisitedSinceLastEdit = false;
    IdType userId{};

    collection_.read([&](const EntityCollection& collection)
                     {
                         auto& currentUser = performedBy.get(collection);
                         const auto& index = collection.threadsById();
                         auto it = index.find(id);
                         if (it == index.end())
                         {
                             writeStatusCode(output, StatusCode::NOT_FOUND);
                             return;
                         }
                         else
                         {
                             auto& thread = **it;
                             thread.visited().fetch_add(1);

                             if (currentUser.id() != AnonymousUserId)
                             if ( ! thread.hasVisitedSinceLastEdit(currentUser.id()))
                             {
                                 addUserToVisitedSinceLastEdit = true;
                                 userId = currentUser.id();
                             }

                             BoolTemporaryChanger _(serializationSettings.hideDiscussionThreadMessageParentThread, true);
                             BoolTemporaryChanger __(serializationSettings.hideVisitedThreadSinceLastChange, true);
                             writeSingleObjectSafeName(output, "thread", thread);
                         }
                         readEvents_.onGetDiscussionThreadById(createObserverContext(currentUser), id);
                     });
    if (addUserToVisitedSinceLastEdit)
    {
        collection_.write([&](EntityCollection& collection)
        {
            auto& index = collection.threads().get<EntityCollection::DiscussionThreadCollectionById>();
            auto it = index.find(id);
            if (it != index.end())
            {
                (*it)->addVisitorSinceLastEdit(userId);
            }
        });
    }
}


template<typename ThreadsIndexFn>
static void writeDiscussionThreadsOfUser(const IdType& id, std::ostream& output, 
    PerformedByWithLastSeenUpdateGuard&& performedBy, const ResourceGuard<EntityCollection>& collection_, 
    const ReadEvents& readEvents_, ThreadsIndexFn&& threadsIndexFn)
{
    if ( ! id )
    {
        writeStatusCode(output, StatusCode::INVALID_PARAMETERS);
        return;
    }
    collection_.read([&](const EntityCollection& collection)
                     {
                         auto& currentUser = performedBy.get(collection);
                         const auto& indexById = collection.usersById();
                         auto it = indexById.find(id);
                         if (it == indexById.end())
                         {
                             writeStatusCode(output, StatusCode::NOT_FOUND);
                             return;
                         }

                         auto& user = **it;
                         const auto& threads = threadsIndexFn(user);
                         BoolTemporaryChanger _(serializationSettings.hideDiscussionThreadCreatedBy, true);
                         BoolTemporaryChanger __(serializationSettings.hideDiscussionThreadMessages, true);

                         writeDiscussionThreads(threads, output, currentUser.id());

                         readEvents_.onGetDiscussionThreadsOfUser(createObserverContext(currentUser), user);
                     });
}

void MemoryRepository::getDiscussionThreadsOfUserByName(const IdType& id, std::ostream& output) const
{
    writeDiscussionThreadsOfUser(id, output, preparePerformedBy(), collection_, readEvents_, [](const auto& user)
    {
        return user.threadsByName();
    });
}

void MemoryRepository::getDiscussionThreadsOfUserByCreated(const IdType& id, std::ostream& output) const
{
    writeDiscussionThreadsOfUser(id, output, preparePerformedBy(), collection_, readEvents_, [](const auto& user)
    {
        return user.threadsByCreated();
    });
}

void MemoryRepository::getDiscussionThreadsOfUserByLastUpdated(const IdType& id, std::ostream& output) const
{
    writeDiscussionThreadsOfUser(id, output, preparePerformedBy(), collection_, readEvents_, [](const auto& user)
    {
        return user.threadsByLastUpdated();
    });
}

void MemoryRepository::getDiscussionThreadsOfUserByMessageCount(const IdType& id, std::ostream& output) const
{
    writeDiscussionThreadsOfUser(id, output, preparePerformedBy(), collection_, readEvents_, [](const auto& user)
    {
        return user.threadsByMessageCount();
    });
}


template<typename ThreadsIndexFn>
static void writeDiscussionThreadsWithTag(const IdType& id, std::ostream& output, 
    PerformedByWithLastSeenUpdateGuard&& performedBy, const ResourceGuard<EntityCollection>& collection_, 
    const ReadEvents& readEvents_, ThreadsIndexFn&& threadsIndexFn)
{
    if ( ! id )
    {
        writeStatusCode(output, StatusCode::INVALID_PARAMETERS);
        return;
    }    
    collection_.read([&](const EntityCollection& collection)
                     {
                         auto& currentUser = performedBy.get(collection);
                         const auto& indexById = collection.tagsById();
                         auto it = indexById.find(id);
                         if (it == indexById.end())
                         {
                             writeStatusCode(output, StatusCode::NOT_FOUND);
                             return;
                         }

                         auto& tag = **it;
                         const auto& threads = threadsIndexFn(tag);
                         BoolTemporaryChanger _(serializationSettings.hideDiscussionThreadMessages, true);

                         writeDiscussionThreads(threads, output, currentUser.id());

                         readEvents_.onGetDiscussionThreadsWithTag(createObserverContext(currentUser), tag);
                     });
}

void MemoryRepository::getDiscussionThreadsWithTagByName(const IdType& id, std::ostream& output) const
{
    writeDiscussionThreadsWithTag(id, output, preparePerformedBy(), collection_, readEvents_, [](const auto& tag)
    {
        return tag.threadsByName();
    });
}

void MemoryRepository::getDiscussionThreadsWithTagByCreated(const IdType& id, std::ostream& output) const
{
    writeDiscussionThreadsWithTag(id, output, preparePerformedBy(), collection_, readEvents_, [](const auto& tag)
    {
        return tag.threadsByCreated();
    });
}

void MemoryRepository::getDiscussionThreadsWithTagByLastUpdated(const IdType& id, std::ostream& output) const
{
    writeDiscussionThreadsWithTag(id, output, preparePerformedBy(), collection_, readEvents_, [](const auto& tag)
    {
        return tag.threadsByLastUpdated();
    });
}

void MemoryRepository::getDiscussionThreadsWithTagByMessageCount(const IdType& id, std::ostream& output) const
{
    writeDiscussionThreadsWithTag(id, output, preparePerformedBy(), collection_, readEvents_, [](const auto& tag)
    {
        return tag.threadsByMessageCount();
    });
}



template<typename ThreadsIndexFn>
static void writeDiscussionThreadsOfCategory(const IdType& id, std::ostream& output, 
    PerformedByWithLastSeenUpdateGuard&& performedBy, const ResourceGuard<EntityCollection>& collection_, 
    const ReadEvents& readEvents_, ThreadsIndexFn&& threadsIndexFn)
{
    if ( ! id )
    {
        writeStatusCode(output, StatusCode::INVALID_PARAMETERS);
        return;
    }
    collection_.read([&](const EntityCollection& collection)
                     {
                         auto& currentUser = performedBy.get(collection);
                         const auto& indexById = collection.categoriesById();
                         auto it = indexById.find(id);
                         if (it == indexById.end())
                         {
                             writeStatusCode(output, StatusCode::NOT_FOUND);
                             return;
                         }
                         auto& category = **it;
                         const auto& threads = threadsIndexFn(category);
                         BoolTemporaryChanger _(serializationSettings.hideDiscussionThreadMessages, true);

                         writeDiscussionThreads(threads, output, currentUser.id());

                         readEvents_.onGetDiscussionThreadsOfCategory(createObserverContext(currentUser), category);
                     });
}

void MemoryRepository::getDiscussionThreadsOfCategoryByName(const IdType& id, std::ostream& output) const
{
    writeDiscussionThreadsOfCategory(id, output, preparePerformedBy(), collection_, readEvents_, [](const auto& category)
    {
        return category.threadsByName();
    });
}

void MemoryRepository::getDiscussionThreadsOfCategoryByCreated(const IdType& id, std::ostream& output) const
{
    writeDiscussionThreadsOfCategory(id, output, preparePerformedBy(), collection_, readEvents_, [](const auto& category)
    {
        return category.threadsByCreated();
    });
}

void MemoryRepository::getDiscussionThreadsOfCategoryByLastUpdated(const IdType& id, std::ostream& output) const
{
    writeDiscussionThreadsOfCategory(id, output, preparePerformedBy(), collection_, readEvents_, [](const auto& category)
    {
        return category.threadsByLastUpdated();
    });
}

void MemoryRepository::getDiscussionThreadsOfCategoryByMessageCount(const IdType& id, std::ostream& output) const
{
    writeDiscussionThreadsOfCategory(id, output, preparePerformedBy(), collection_, readEvents_, [](const auto& category)
    {
        return category.threadsByMessageCount();
    });
}


static StatusCode validateDiscussionThreadName(const std::string& name, const boost::u32regex& regex, 
                                               const ConfigConstRef& config)
{
    if (name.empty())
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
                          updateCreated(*thread);
                          thread->lastUpdated() = thread->created();
                          
                          collection.insertDiscussionThread(thread);
                          createdBy->insertDiscussionThread(thread);

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
        return;
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

                          auto user = performedBy.getAndUpdate(collection);

                          collection.modifyDiscussionThread(it, [&newName, &user](DiscussionThread& thread)
                          {
                              thread.name() = newName;
                              updateLastUpdated(thread, user);
                          });
                          writeEvents_.onChangeDiscussionThread(createObserverContext(*user), **it,
                                  DiscussionThread::ChangeType::Name);
                      });
}

void MemoryRepository::deleteDiscussionThread(const IdType& id, std::ostream& output)
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

                        auto user = performedBy.getAndUpdate(collection);
                        auto& threadFromRef = *itFrom;
                        auto& threadFrom = **itFrom;
                        auto& threadInto = **itInto;

                        //make sure the thread is not deleted before being passed to the observers
                        writeEvents_.onMergeDiscussionThreads(createObserverContext(*user), threadFrom, threadInto);

                        collection.modifyDiscussionThread(itInto, [&](DiscussionThread& thread)
                        {
                            updateLastUpdated(thread, user);

                            for (auto& message : threadFrom.messages())
                            {
                                auto& createdBy = message->createdBy();

                                auto messageClone = std::make_shared<DiscussionThreadMessage>(*message, threadInto);

                                collection.messages().insert(messageClone);
                                thread.messages().insert(messageClone);
                                createdBy.messages().insert(messageClone);
                            }
                            for (auto& tagWeak : thread.tagsWeak())
                            {
                                if (auto tagShared = tagWeak.lock())
                                {
                                    collection.modifyDiscussionTagById(tagShared->id(), [&threadFrom, &thread](auto& tag)
                                    {
                                        tag.messageCount() += threadFrom.messages().size();
                                        //notify the thread collection of each tag that the thread has new messages
                                        tag.modifyDiscussionThreadById(thread.id(), [](auto& _) {});
                                    });
                                }
                            }
                            for (auto& categoryWeak : thread.categoriesWeak())
                            {
                                if (auto categoryShared = categoryWeak.lock())
                                {
                                    collection.modifyDiscussionCategoryById(categoryShared->id(), 
                                        [&threadFrom, &threadFromRef, &thread](auto& category)
                                    {
                                        category.updateMessageCount(threadFromRef, threadFrom.messages().size());
                                        //notify the thread collection of each category that the thread has new messages
                                        category.modifyDiscussionThreadById(thread.id(), [](auto& _) {});
                                    });
                                }
                            }
                        });
                        //this will also decrease the message count on the tags the thread was part of
                        collection.deleteDiscussionThread(itFrom);
                    });
}
