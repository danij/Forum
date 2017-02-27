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
static void writeDiscussionThreads(ThreadsCollection&& collection, RetrieveDiscussionThreadsBy by, 
                                   std::ostream& output, const IdType& currentUserId)
{
    BoolTemporaryChanger _(serializationSettings.visitedThreadSinceLastChange, false);
    BoolTemporaryChanger __(serializationSettings.hideDiscussionThreadMessages, true);

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

    switch (by)
    {
    case RetrieveDiscussionThreadsBy::Name:
        writeEntitiesWithPagination(collection.threadsByName(), "threads", output, displayContext.pageNumber, 
            pageSize, displayContext.sortOrder == Context::SortOrder::Ascending, writeInterceptor);
        break;
    case RetrieveDiscussionThreadsBy::Created:
        writeEntitiesWithPagination(collection.threadsByCreated(), "threads", output, displayContext.pageNumber, 
            pageSize, displayContext.sortOrder == Context::SortOrder::Ascending, writeInterceptor);
        break;
    case RetrieveDiscussionThreadsBy::LastUpdated:
        writeEntitiesWithPagination(collection.threadsByLastUpdated(), "threads", output, displayContext.pageNumber, 
            pageSize, displayContext.sortOrder == Context::SortOrder::Ascending, writeInterceptor);
        break;
    case RetrieveDiscussionThreadsBy::MessageCount:
        writeEntitiesWithPagination(collection.threadsByMessageCount(), "threads", output, displayContext.pageNumber, 
            pageSize, displayContext.sortOrder == Context::SortOrder::Ascending, writeInterceptor);
        break;
    }
}

StatusCode MemoryRepository::getDiscussionThreads(std::ostream& output, RetrieveDiscussionThreadsBy by) const
{
    auto performedBy = preparePerformedBy();

    collection_.read([&](const EntityCollection& collection)
    {
        auto& currentUser = performedBy.get(collection);
        
        writeDiscussionThreads(collection, by, output, currentUser.id());

        readEvents_.onGetDiscussionThreads(createObserverContext(currentUser));
    });
    return StatusCode::OK;
}

StatusCode MemoryRepository::getDiscussionThreadById(const IdType& id, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);

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
                             status = StatusCode::NOT_FOUND;
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

                             auto& displayContext = Context::getDisplayContext();
                             if (displayContext.checkNotChangedSince > 0)
                             {
                                 if (thread.latestVisibleChange() <= displayContext.checkNotChangedSince)
                                 {
                                     status = StatusCode::NOT_UPDATED_SINCE_LAST_CHECK;
                                     return;
                                 }
                             }

                             BoolTemporaryChanger _(serializationSettings.hideDiscussionThreadMessageParentThread, true);
                             BoolTemporaryChanger __(serializationSettings.hideVisitedThreadSinceLastChange, true);
                             status.disable();
                             writeSingleValueSafeName(output, "thread", thread);
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
    return status;
}


StatusCode MemoryRepository::getDiscussionThreadsOfUser(const IdType& id, std::ostream& output, 
                                                        RetrieveDiscussionThreadsBy by) const
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! id )
    {
        return status = StatusCode::INVALID_PARAMETERS;        
    }
    auto performedBy = preparePerformedBy();

    collection_.read([&](const EntityCollection& collection)
                     {
                         auto& currentUser = performedBy.get(collection);
                         const auto& indexById = collection.usersById();
                         auto it = indexById.find(id);
                         if (it == indexById.end())
                         {
                             status = StatusCode::NOT_FOUND;
                             return;
                         }
                         auto& user = **it;

                         BoolTemporaryChanger _(serializationSettings.hideDiscussionThreadCreatedBy, true);

                         status.disable();
                         writeDiscussionThreads(user, by, output, currentUser.id());

                         readEvents_.onGetDiscussionThreadsOfUser(createObserverContext(currentUser), user);
                     });
    return status;
}

StatusCode MemoryRepository::getDiscussionThreadsWithTag(const IdType& id, std::ostream& output, 
                                                         RetrieveDiscussionThreadsBy by) const
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! id )
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    auto performedBy = preparePerformedBy();

    collection_.read([&](const EntityCollection& collection)
                     {
                         auto& currentUser = performedBy.get(collection);
                         const auto& indexById = collection.tagsById();
                         auto it = indexById.find(id);
                         if (it == indexById.end())
                         {
                             status = StatusCode::NOT_FOUND;
                             return;
                         }

                         auto& tag = **it;

                         status.disable();
                         writeDiscussionThreads(tag, by, output, currentUser.id());

                         readEvents_.onGetDiscussionThreadsWithTag(createObserverContext(currentUser), tag);
                     });
    return status;
}

StatusCode MemoryRepository::getDiscussionThreadsOfCategory(const IdType& id, std::ostream& output, 
                                                            RetrieveDiscussionThreadsBy by) const
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! id )
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    auto performedBy = preparePerformedBy();

    collection_.read([&](const EntityCollection& collection)
                     {
                         auto& currentUser = performedBy.get(collection);
                         const auto& indexById = collection.categoriesById();
                         auto it = indexById.find(id);
                         if (it == indexById.end())
                         {
                             status = StatusCode::NOT_FOUND;
                             return;
                         }
                         auto& category = **it;

                         status.disable();
                         writeDiscussionThreads(category, by, output, currentUser.id());

                         readEvents_.onGetDiscussionThreadsOfCategory(createObserverContext(currentUser), category);
                     });
    return status;
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

StatusCode MemoryRepository::addNewDiscussionThread(const std::string& name, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    auto validationCode = validateDiscussionThreadName(name, validDiscussionThreadNameRegex, getGlobalConfig());
    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
    }

    auto performedBy = preparePerformedBy();

    collection_.write([&](EntityCollection& collection)
                      {
                          const auto& createdBy = performedBy.getAndUpdate(collection);

                          auto thread = std::make_shared<DiscussionThread>(*createdBy);
                          thread->id() = generateUUIDString();
                          thread->name() = name;
                          updateCreated(*thread);
                          thread->latestVisibleChange() = thread->lastUpdated() = thread->created();
                          
                          collection.insertDiscussionThread(thread);
                          createdBy->insertDiscussionThread(thread);

                          writeEvents_.onAddNewDiscussionThread(createObserverContext(*createdBy), *thread);

                          status.addExtraSafeName("id", thread->id());
                          status.addExtraSafeName("name", thread->name());
                          status.addExtraSafeName("created", thread->created());
                      });
    return status;
}

StatusCode MemoryRepository::changeDiscussionThreadName(const IdType& id, const std::string& newName, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    auto validationCode = validateDiscussionThreadName(newName, validDiscussionThreadNameRegex, getGlobalConfig());
    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
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
                              thread.latestVisibleChange() = Context::getCurrentTime();
                              updateLastUpdated(thread, user);
                          });
                          writeEvents_.onChangeDiscussionThread(createObserverContext(*user), **it,
                                  DiscussionThread::ChangeType::Name);
                      });
    return status;
}

StatusCode MemoryRepository::deleteDiscussionThread(const IdType& id, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
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
    return status;
}

StatusCode MemoryRepository::mergeDiscussionThreads(const IdType& fromId, const IdType& intoId, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! fromId || ! intoId)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    if (fromId == intoId)
    {
        return status = StatusCode::NO_EFFECT;
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
                            thread.latestVisibleChange() = thread.lastUpdated();

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
    return status;
}
