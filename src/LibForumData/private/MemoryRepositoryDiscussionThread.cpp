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
using namespace Forum::Authorization;

MemoryRepositoryDiscussionThread::MemoryRepositoryDiscussionThread(MemoryStoreRef store, 
                                                                   DiscussionThreadAuthorizationRef authorization)
    : MemoryRepositoryBase(std::move(store)), authorization_(std::move(authorization))
{
    if ( ! authorization_)
    {
        throw std::runtime_error("Authorization implementation not provided");
    }
}

template<typename ThreadsCollectionByName, typename ThreadsCollectionByCreated, typename ThreadsCollectionByLastUpdated,
         typename ThreadsCollectionByMessageCount, typename ThreadsCollectionByPinDisplayOrder>
static void writeDiscussionThreads(ThreadsCollectionByName&& collectionByName,
                                   ThreadsCollectionByCreated&& collectionByCreated,
                                   ThreadsCollectionByLastUpdated&& collectionByLastUpdated,
                                   ThreadsCollectionByMessageCount&& collectionByMessageCount,
                                   ThreadsCollectionByPinDisplayOrder&& collectionByPinDisplayOrder,
                                   RetrieveDiscussionThreadsBy by, OutStream& output, 
                                   const GrantedPrivilegeStore& privilegeStore, const User& currentUser)
{
    BoolTemporaryChanger _(serializationSettings.visitedThreadSinceLastChange, false);
    BoolTemporaryChanger __(serializationSettings.hideDiscussionThreadMessages, true);

    auto writeFilter = [&](const DiscussionThread& currentThread)
                          {
                              bool visitedThreadSinceLastChange = false;
                              if (currentUser.id() != AnonymousUserId)
                              {
                                  visitedThreadSinceLastChange = currentThread.hasVisitedSinceLastEdit(currentUser.id());
                              }
                              serializationSettings.visitedThreadSinceLastChange = visitedThreadSinceLastChange;
                              return true;
                          };

    auto pageSize = getGlobalConfig()->discussionThread.maxThreadsPerPage;
    auto& displayContext = Context::getDisplayContext();

    SerializationRestriction restriction(privilegeStore, currentUser, Context::getCurrentTime());
    
    auto ascending = displayContext.sortOrder == Context::SortOrder::Ascending;

    Json::JsonWriter writer(output);

    writer.startObject();

    if (0 == displayContext.pageNumber)
    {
        writer.newPropertyWithSafeName("pinned_threads");
        writer.startArray();

        auto it = collectionByPinDisplayOrder.rbegin();
        auto end = collectionByPinDisplayOrder.rend();

        for (; (it != end) && ((*it)->pinDisplayOrder() > 0); ++it)
        {
            serialize(writer, **it, restriction);
        }

        writer.endArray();
    }
    
    switch (by)
    {
    case RetrieveDiscussionThreadsBy::Name:
        writeEntitiesWithPagination(collectionByName, displayContext.pageNumber, pageSize, ascending, "threads", 
            writer, writeFilter, restriction);
        break;
    case RetrieveDiscussionThreadsBy::Created:
        writeEntitiesWithPagination(collectionByCreated, displayContext.pageNumber, pageSize, ascending, "threads",
            writer, writeFilter, restriction);
        break;
    case RetrieveDiscussionThreadsBy::LastUpdated:
        writeEntitiesWithPagination(collectionByLastUpdated, displayContext.pageNumber, pageSize, ascending, "threads",
            writer, writeFilter, restriction);
        break;
    case RetrieveDiscussionThreadsBy::MessageCount:
        writeEntitiesWithPagination(collectionByMessageCount, displayContext.pageNumber, pageSize, ascending, "threads",
            writer, writeFilter, restriction);
        break;
    }

    writer.endObject();
}

template<typename ThreadsCollection>
static void writeDiscussionThreads(ThreadsCollection&& collection, RetrieveDiscussionThreadsBy by,
                                   OutStream& output, const GrantedPrivilegeStore& privilegeStore, const User& currentUser)
{
    writeDiscussionThreads(collection.threadsByName(), collection.threadsByCreated(), collection.threadsByLastUpdated(),
                           collection.threadsByMessageCount(), collection.threadsByPinDisplayOrder(), by, output, 
                           privilegeStore, currentUser);
}

StatusCode MemoryRepositoryDiscussionThread::getDiscussionThreads(OutStream& output, RetrieveDiscussionThreadsBy by) const
{
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, store());
        
                          writeDiscussionThreads(collection, by, output, collection.grantedPrivileges(), currentUser);

                          readEvents().onGetDiscussionThreads(createObserverContext(currentUser));
                      });
    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionThread::getDiscussionThreadById(const IdType& id, OutStream& output)
{
    StatusWriter status(output);

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

                          auto& thread = **it;

                          if ( ! (status = authorization_->getDiscussionThreadById(currentUser, thread)))
                          {
                              return;
                          }

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

                          SerializationRestriction restriction(collection.grantedPrivileges(), currentUser, Context::getCurrentTime());

                          writeSingleValueSafeName(output, "thread", thread, restriction);
                          
                          readEvents().onGetDiscussionThreadById(createObserverContext(currentUser), thread);
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
    StatusWriter status(output);
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

                          if ( ! (status = authorization_->getDiscussionThreadsOfUser(currentUser, user)))
                          {
                              return;
                          }
                 
                          BoolTemporaryChanger _(serializationSettings.hideDiscussionThreadCreatedBy, true);
                 
                          status.disable();
                          writeDiscussionThreads(user, by, output, collection.grantedPrivileges(), currentUser);

                          readEvents().onGetDiscussionThreadsOfUser(createObserverContext(currentUser), user);
                      });
    return status;
}

StatusCode MemoryRepositoryDiscussionThread::getSubscribedDiscussionThreadsOfUser(const IdType& id, OutStream& output,
                                                                                  RetrieveDiscussionThreadsBy by) const
{
    StatusWriter status(output);
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
                 
                          if ( ! (status = authorization_->getSubscribedDiscussionThreadsOfUser(currentUser, user)))
                          {
                              return;
                          }

                          BoolTemporaryChanger _(serializationSettings.hideDiscussionThreadCreatedBy, true);
                 
                          status.disable();
                          writeDiscussionThreads(user.subscribedThreadsByName(), user.subscribedThreadsByCreated(),
                                                 user.subscribedThreadsByLastUpdated(), 
                                                 user.subscribedThreadsByMessageCount(),
                                                 user.subscribedThreadsByPinDisplayOrder(),
                                                 by, output, collection.grantedPrivileges(), currentUser);

                          readEvents().onGetDiscussionThreadsOfUser(createObserverContext(currentUser), user);
                      });
    return status;
}

StatusCode MemoryRepositoryDiscussionThread::getDiscussionThreadsWithTag(const IdType& id, OutStream& output,
                                                                         RetrieveDiscussionThreadsBy by) const
{
    StatusWriter status(output);
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
                                                    
                          if ( ! (status = authorization_->getDiscussionThreadsWithTag(currentUser, tag)))
                          {
                              return;
                          }
                 
                          status.disable();
                          writeDiscussionThreads(tag, by, output, collection.grantedPrivileges(), currentUser);

                          readEvents().onGetDiscussionThreadsWithTag(createObserverContext(currentUser), tag);
                      });
    return status;
}

StatusCode MemoryRepositoryDiscussionThread::getDiscussionThreadsOfCategory(const IdType& id, OutStream& output,
                                                                            RetrieveDiscussionThreadsBy by) const
{
    StatusWriter status(output);
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

                         if ( ! (status = authorization_->getDiscussionThreadsOfCategory(currentUser, category)))
                         {
                             return;
                         }

                         status.disable();
                         writeDiscussionThreads(category, by, output, collection.grantedPrivileges(), currentUser);

                         readEvents().onGetDiscussionThreadsOfCategory(createObserverContext(currentUser), category);
                     });
    return status;
}


StatusCode MemoryRepositoryDiscussionThread::addNewDiscussionThread(StringView name, OutStream& output)
{
    StatusWriter status(output);
    
    auto config = getGlobalConfig();
    auto validationCode = validateString(name, INVALID_PARAMETERS_FOR_EMPTY_STRING,
                                         config->discussionThread.minNameLength, 
                                         config->discussionThread.maxNameLength,
                                         &MemoryRepositoryBase::doesNotContainLeadingOrTrailingWhitespace);
    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
    }

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);
                 
                           if ( ! (status = authorization_->addNewDiscussionThread(*currentUser, name)))
                           {
                               return;
                           }

                           auto statusWithResource = addNewDiscussionThread(collection, name);
                           auto& thread = statusWithResource.resource;
                           if ( ! (status = statusWithResource.status)) return;

                           writeEvents().onAddNewDiscussionThread(createObserverContext(*currentUser), *thread);
                 
                           status.writeNow([&](auto& writer)
                                           {
                                               writer << Json::propertySafeName("id", thread->id());
                                               writer << Json::propertySafeName("name", thread->name().string());
                                               writer << Json::propertySafeName("created", thread->created());
                                           });
                       });
    return status;
}

StatusWithResource<DiscussionThreadRef> MemoryRepositoryDiscussionThread::addNewDiscussionThread(EntityCollection& collection,
                                                                                                 StringView name)
{
    auto currentUser = getCurrentUser(collection);

    auto thread = std::make_shared<DiscussionThread>(*currentUser);
    thread->id() = generateUUIDString();
    thread->name() = name;
    updateCreated(*thread);
    thread->latestVisibleChange() = thread->lastUpdated() = thread->created();
                           
    collection.insertDiscussionThread(thread);
                 
    collection.modifyUserById(currentUser->id(), [&](User& user)
                                                 {
                                                     user.insertDiscussionThread(thread);
                                                 });

    //add privileges for the user that created the message
    auto changePrivilegeDuration = optionalOrZero(
            collection.getForumWideDefaultPrivilegeDuration(
                    ForumWideDefaultPrivilegeDuration::CHANGE_DISCUSSION_THREAD_NAME));
    if (changePrivilegeDuration > 0)
    {
        auto privilege = DiscussionThreadPrivilege::CHANGE_NAME;
        auto valueNeeded = optionalOrZero(collection.getDiscussionThreadPrivilege(privilege));

        if (valueNeeded > 0)
        {
            auto expiresAt = thread->created() + changePrivilegeDuration;

            collection.grantedPrivileges().grantDiscussionThreadPrivilege(
                currentUser->id(), thread->id(), privilege, valueNeeded, expiresAt);
        }
    }

    auto deletePrivilegeDuration = optionalOrZero(
            collection.getForumWideDefaultPrivilegeDuration(
                    ForumWideDefaultPrivilegeDuration::DELETE_DISCUSSION_THREAD));
    if (deletePrivilegeDuration > 0)
    {
        auto privilege = DiscussionThreadPrivilege::DELETE;
        auto valueNeeded = optionalOrZero(collection.getDiscussionThreadPrivilege(privilege));

        if (valueNeeded)
        {
            auto expiresAt = thread->created() + changePrivilegeDuration;

            collection.grantedPrivileges().grantDiscussionThreadPrivilege(
                currentUser->id(), thread->id(), privilege, valueNeeded, expiresAt);
        }
    }
    
    return thread;
}


StatusCode MemoryRepositoryDiscussionThread::changeDiscussionThreadName(const IdType& id, StringView newName,
                                                                        OutStream& output)
{
    StatusWriter status(output);

    auto config = getGlobalConfig();
    auto validationCode = validateString(newName, INVALID_PARAMETERS_FOR_EMPTY_STRING,
                                         config->discussionThread.minNameLength, 
                                         config->discussionThread.maxNameLength,
                                         &MemoryRepositoryBase::doesNotContainLeadingOrTrailingWhitespace);
    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.threads()
                                   .get<EntityCollection::DiscussionThreadCollectionById>();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           
                           if ( ! (status = authorization_->changeDiscussionThreadName(*currentUser, **it, newName)))
                           {
                               return;
                           }

                           if ( ! (status = changeDiscussionThreadName(collection, id, newName))) return;
                           
                           writeEvents().onChangeDiscussionThread(createObserverContext(*currentUser), **it,
                                                                  DiscussionThread::ChangeType::Name);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionThread::changeDiscussionThreadName(EntityCollection& collection, IdTypeRef id, 
                                                                        StringView newName)
{
    auto& indexById = collection.threads().get<EntityCollection::DiscussionThreadCollectionById>();
    auto it = indexById.find(id);
    if (it == indexById.end())
    {
        return StatusCode::NOT_FOUND;
    }

    auto currentUser = getCurrentUser(collection);

    collection.modifyDiscussionThread(it, [&newName, &currentUser](DiscussionThread& thread)
                                          {
                                              thread.name() = newName;
                                              thread.latestVisibleChange() = Context::getCurrentTime();
                                              updateLastUpdated(thread, currentUser);
                                          });
    return StatusCode::OK;
}


StatusCode MemoryRepositoryDiscussionThread::changeDiscussionThreadPinDisplayOrder(const IdType& id, uint16_t newValue,
                                                                                   OutStream& output)
{
    StatusWriter status(output);
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.threads()
                                   .get<EntityCollection::DiscussionThreadCollectionById>();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           
                           if ( ! (status = authorization_->changeDiscussionThreadPinDisplayOrder(*currentUser, **it, newValue)))
                           {
                               return;
                           }

                           if ( ! (status = changeDiscussionThreadPinDisplayOrder(collection, id, newValue))) return;
                                                      
                           writeEvents().onChangeDiscussionThread(createObserverContext(*currentUser), **it,
                                                                  DiscussionThread::ChangeType::PinDisplayOrder);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionThread::changeDiscussionThreadPinDisplayOrder(EntityCollection& collection,
                                                                                   IdTypeRef id, uint16_t newValue)
{
    auto& indexById = collection.threads().get<EntityCollection::DiscussionThreadCollectionById>();
    auto it = indexById.find(id);
    if (it == indexById.end())
    {
        return StatusCode::NOT_FOUND;
    }

    auto currentUser = getCurrentUser(collection);

    collection.modifyDiscussionThread(it, [newValue, &currentUser](DiscussionThread& thread)
                                          {
                                              thread.pinDisplayOrder() = newValue;
                                              thread.latestVisibleChange() = Context::getCurrentTime();
                                              updateLastUpdated(thread, currentUser);
                                          });
    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionThread::deleteDiscussionThread(const IdType& id, OutStream& output)
{
    StatusWriter status(output);
    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.threads().get<EntityCollection::DiscussionThreadCollectionById>();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           if ( ! (status = authorization_->deleteDiscussionThread(*currentUser, **it)))
                           {
                               return;
                           }

                           //make sure the thread is not deleted before being passed to the observers
                           writeEvents().onDeleteDiscussionThread(createObserverContext(*currentUser), **it);

                           status = deleteDiscussionThread(collection, id);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionThread::deleteDiscussionThread(EntityCollection& collection, IdTypeRef id)
{
    auto& indexById = collection.threads().get<EntityCollection::DiscussionThreadCollectionById>();
    auto it = indexById.find(id);
    if (it == indexById.end())
    {
        return StatusCode::NOT_FOUND;
    }
    
    collection.deleteDiscussionThread(it);
        
    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionThread::mergeDiscussionThreads(const IdType& fromId, const IdType& intoId,
                                                                    OutStream& output)
{
    StatusWriter status(output);
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
                           auto currentUser = performedBy.getAndUpdate(collection);

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
                   
                           auto& threadFrom = **itFrom;
                           auto& threadInto = **itInto;
                   
                           if ( ! (status = authorization_->mergeDiscussionThreads(*currentUser, threadFrom, threadInto)))
                           {
                               return;
                           }

                           //make sure the thread is not deleted before being passed to the observers
                           writeEvents().onMergeDiscussionThreads(createObserverContext(*currentUser), 
                                                                  threadFrom, threadInto);
                           status = mergeDiscussionThreads(collection, fromId, intoId);
                       });                          
    return status;
}

static void updateMessageCounts(EntityCollection& collection, DiscussionThreadRef& threadRef, int_fast32_t difference)
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
                                                   tag.modifyDiscussionThreadById(threadRef->id(), {});
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
                                                        category.modifyDiscussionThreadById(threadRef->id(), {});
                                                    });
        }
    }
};

StatusCode MemoryRepositoryDiscussionThread::mergeDiscussionThreads(EntityCollection& collection,
                                                                    IdTypeRef fromId, IdTypeRef intoId)
{
    auto currentUser = getCurrentUser(collection);

    auto& indexById = collection.threads().get<EntityCollection::DiscussionThreadCollectionById>();
    auto itFrom = indexById.find(fromId);
    if (itFrom == indexById.end())
    {
        return StatusCode::NOT_FOUND;
    }
    auto itInto = indexById.find(intoId);
    if (itInto == indexById.end())
    {
        return StatusCode::NOT_FOUND;
    }

    auto threadFromRef = *itFrom;
    auto& threadFrom = **itFrom;
    auto threadIntoRef = *itInto;
                       
    collection.modifyDiscussionThread(itInto, [&](DiscussionThread& thread)
    {
        updateLastUpdated(thread, currentUser);
        thread.latestVisibleChange() = thread.lastUpdated();
                   
        for (auto& message : threadFrom.messages())
        {
            thread.insertMessage(message);
        }
                   
        updateMessageCounts(collection, threadFromRef, - static_cast<int_fast32_t>(threadFrom.messages().size()));
        updateMessageCounts(collection, threadIntoRef,   static_cast<int_fast32_t>(threadFrom.messages().size()));
                   
        //remove all message references from the thread so they don't get deleted
        threadFrom.messages().clear();

        //update subscriptions
        for (auto& userWeak : threadFromRef->subscribedUsers())
        {
            if (auto userShared = userWeak.lock())
            {
                userShared->subscribedThreads().insertDiscussionThread(threadIntoRef);
                thread.subscribedUsers().insert(userWeak);
            }
        }
    });
    //this will also decrease the message count on the tags the thread was part of
    collection.deleteDiscussionThread(itFrom);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionThread::subscribeToDiscussionThread(const IdType& id, OutStream& output)
{
    StatusWriter status(output);
    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.threads().get<EntityCollection::DiscussionThreadCollectionById>();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           auto threadRef = *it;

                           if ( ! (status = authorization_->subscribeToDiscussionThread(*currentUser, *threadRef)))
                           {
                               return;
                           }

                           if ( ! (status = subscribeToDiscussionThread(collection, id))) return;
                           
                           writeEvents().onSubscribeToDiscussionThread(createObserverContext(*currentUser), *threadRef);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionThread::subscribeToDiscussionThread(EntityCollection& collection, IdTypeRef id)
{
    auto& indexById = collection.threads().get<EntityCollection::DiscussionThreadCollectionById>();
    auto it = indexById.find(id);
    if (it == indexById.end())
    {
        return StatusCode::NOT_FOUND;
    }

    auto threadRef = *it;
    auto currentUser = getCurrentUser(collection);

    if ( ! (threadRef->subscribedUsers().insert(currentUser).second))
    {
        return StatusCode::NO_EFFECT;
    }

    currentUser->subscribedThreads().insertDiscussionThread(threadRef);
    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionThread::unsubscribeFromDiscussionThread(const IdType& id, OutStream& output)
{
    StatusWriter status(output);
    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.threads().get<EntityCollection::DiscussionThreadCollectionById>();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           auto threadRef = *it;

                           if ( ! (status = authorization_->unsubscribeFromDiscussionThread(*currentUser, *threadRef)))
                           {
                               return;
                           }

                           if ( ! (status = unsubscribeFromDiscussionThread(collection, id))) return;

                           writeEvents().onUnsubscribeFromDiscussionThread(createObserverContext(*currentUser), *threadRef);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionThread::unsubscribeFromDiscussionThread(EntityCollection& collection, IdTypeRef id)
{
    auto& indexById = collection.threads().get<EntityCollection::DiscussionThreadCollectionById>();
    auto it = indexById.find(id);
    if (it == indexById.end())
    {
        return StatusCode::NOT_FOUND;
    }

    auto threadRef = *it;
    auto currentUser = getCurrentUser(collection);

    if (0 == threadRef->subscribedUsers().erase(currentUser))
    {
        return StatusCode::NO_EFFECT;
    }

    currentUser->subscribedThreads().deleteDiscussionThreadById(threadRef->id());
    return StatusCode::OK;
}