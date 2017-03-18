#include "MemoryRepositoryDiscussionThread.h"

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

MemoryRepositoryDiscussionThread::MemoryRepositoryDiscussionThread(MemoryStoreRef store)
    : MemoryRepositoryBase(std::move(store)),
    validDiscussionThreadNameRegex(boost::make_u32regex("^[^[:space:]]+.*[^[:space:]]+$"))
{    
}

template<typename ThreadsCollection>
static void writeDiscussionThreads(ThreadsCollection&& collection, RetrieveDiscussionThreadsBy by, 
                                   OutStream& output, const IdType& currentUserId)
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

StatusCode MemoryRepositoryDiscussionThread::getDiscussionThreads(OutStream& output, RetrieveDiscussionThreadsBy by) const
{
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, store());
                          
                          writeDiscussionThreads(collection, by, output, currentUser.id());

                          if ( ! Context::skipObservers())
                              readEvents().onGetDiscussionThreads(createObserverContext(currentUser));
                      });
    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionThread::getDiscussionThreadById(const IdType& id, OutStream& output)
{
    StatusWriter status(output, StatusCode::OK);

    PerformedByWithLastSeenUpdateGuard performedBy;
    bool addUserToVisitedSinceLastEdit = false;
    IdType userId{};

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, store());
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
                          if ( ! Context::skipObservers())
                              readEvents().onGetDiscussionThreadById(createObserverContext(currentUser), id);
                      });
    if (addUserToVisitedSinceLastEdit)
    {
        collection().write([&](EntityCollection& collection)
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


StatusCode MemoryRepositoryDiscussionThread::getDiscussionThreadsOfUser(const IdType& id, OutStream& output,
                                                                        RetrieveDiscussionThreadsBy by) const
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! id )
    {
        return status = StatusCode::INVALID_PARAMETERS;        
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

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
                 
                          BoolTemporaryChanger _(serializationSettings.hideDiscussionThreadCreatedBy, true);
                 
                          status.disable();
                          writeDiscussionThreads(user, by, output, currentUser.id());

                          if ( ! Context::skipObservers())
                              readEvents().onGetDiscussionThreadsOfUser(createObserverContext(currentUser), user);
                      });
    return status;
}

StatusCode MemoryRepositoryDiscussionThread::getDiscussionThreadsWithTag(const IdType& id, OutStream& output,
                                                                         RetrieveDiscussionThreadsBy by) const
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! id )
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, store());
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

                          if ( ! Context::skipObservers())
                              readEvents().onGetDiscussionThreadsWithTag(createObserverContext(currentUser), tag);
                      });
    return status;
}

StatusCode MemoryRepositoryDiscussionThread::getDiscussionThreadsOfCategory(const IdType& id, OutStream& output,
                                                                            RetrieveDiscussionThreadsBy by) const
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! id )
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                     {
                         auto& currentUser = performedBy.get(collection, store());
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

                         if ( ! Context::skipObservers())
                             readEvents().onGetDiscussionThreadsOfCategory(createObserverContext(currentUser), category);
                     });
    return status;
}


StatusCode MemoryRepositoryDiscussionThread::addNewDiscussionThread(const StringView& name, OutStream& output)
{
    StatusWriter status(output, StatusCode::OK);
    
    auto config = getGlobalConfig();
    auto validationCode = validateString(name, validDiscussionThreadNameRegex, INVALID_PARAMETERS_FOR_EMPTY_STRING,
                                         config->discussionThread.minNameLength, config->discussionThread.maxNameLength);
    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
    }

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           const auto& createdBy = performedBy.getAndUpdate(collection);
                 
                           auto thread = std::make_shared<DiscussionThread>(*createdBy);
                           thread->id() = generateUUIDString();
                           thread->name() = toString(name);
                           updateCreated(*thread);
                           thread->latestVisibleChange() = thread->lastUpdated() = thread->created();
                           
                           collection.insertDiscussionThread(thread);
                 
                           collection.modifyUserById(createdBy->id(), [&](User& user)
                                                                      {
                                                                        user.insertDiscussionThread(thread);
                                                                      });

                           if ( ! Context::skipObservers())
                               writeEvents().onAddNewDiscussionThread(createObserverContext(*createdBy), *thread);
                 
                           status.addExtraSafeName("id", thread->id());
                           status.addExtraSafeName("name", thread->name());
                           status.addExtraSafeName("created", thread->created());
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionThread::changeDiscussionThreadName(const IdType& id, const StringView& newName,
                                                                        OutStream& output)
{
    StatusWriter status(output, StatusCode::OK);

    auto config = getGlobalConfig();
    auto validationCode = validateString(newName, validDiscussionThreadNameRegex, INVALID_PARAMETERS_FOR_EMPTY_STRING,
                                         config->discussionThread.minNameLength, config->discussionThread.maxNameLength);
    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
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
                                                                     thread.name() = toString(newName);
                                                                     thread.latestVisibleChange() = Context::getCurrentTime();
                                                                     updateLastUpdated(thread, user);
                                                                 });
                           if ( ! Context::skipObservers())
                               writeEvents().onChangeDiscussionThread(createObserverContext(*user), **it,
                                                                      DiscussionThread::ChangeType::Name);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionThread::deleteDiscussionThread(const IdType& id, OutStream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto& indexById = collection.threads().get<EntityCollection::DiscussionThreadCollectionById>();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           //make sure the thread is not deleted before being passed to the observers
                           if ( ! Context::skipObservers())
                               writeEvents().onDeleteDiscussionThread(
                                       createObserverContext(*performedBy.getAndUpdate(collection)), **it);

                           collection.deleteDiscussionThread(it);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionThread::mergeDiscussionThreads(const IdType& fromId, const IdType& intoId,
                                                                    OutStream& output)
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

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
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
                           auto threadFromRef = *itFrom;
                           auto& threadFrom = **itFrom;
                           auto threadIntoRef = *itInto;
                           auto& threadInto = **itInto;
                   
                           //make sure the thread is not deleted before being passed to the observers
                           if ( ! Context::skipObservers())
                               writeEvents().onMergeDiscussionThreads(createObserverContext(*user), threadFrom, threadInto);
                   
                           auto updateMessageCounts = [&collection](DiscussionThreadRef& threadRef, int_fast32_t difference)
                           {
                               for (auto& tagWeak : threadRef->tagsWeak())
                               {
                                   if (auto tagShared = tagWeak.lock())
                                   {
                                       collection.modifyDiscussionTagById(tagShared->id(), 
                                           [&threadRef, difference](auto& tag)
                                           {
                                               tag.messageCount() += difference;
                                               //notify the thread collection of each tag that the thread has new messages
                                               tag.modifyDiscussionThreadById(threadRef->id());
                                           });
                                   }
                               }
                               for (auto& categoryWeak : threadRef->categoriesWeak())
                               {
                                   if (auto categoryShared = categoryWeak.lock())
                                   {
                                       collection.modifyDiscussionCategoryById(categoryShared->id(),
                                           [&threadRef, difference](auto& category)
                                           {
                                               category.updateMessageCount(threadRef, difference);
                                               //notify the thread collection of each category that the thread has new messages
                                               category.modifyDiscussionThreadById(threadRef->id());
                                           });
                                   }
                               }
                   
                           };
                   
                           collection.modifyDiscussionThread(itInto, [&](DiscussionThread& thread)
                           {
                               updateLastUpdated(thread, user);
                               thread.latestVisibleChange() = thread.lastUpdated();
                   
                               for (auto& message : threadFrom.messages())
                               {
                                   thread.messages().insert(message);
                               }
                   
                               updateMessageCounts(threadFromRef, - static_cast<int_fast32_t>(threadFrom.messages().size()));
                               updateMessageCounts(threadIntoRef,   static_cast<int_fast32_t>(threadFrom.messages().size()));
                   
                               //remove all message references from the thread so they don't get deleted
                               threadFrom.messages().clear();
                           });
                           //this will also decrease the message count on the tags the thread was part of
                           collection.deleteDiscussionThread(itFrom);
                       });
    return status;
}
