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
         typename ThreadsCollectionByMessageCount>
static void writeDiscussionThreads(ThreadsCollectionByName&& collectionByName, 
                                   ThreadsCollectionByCreated&& collectionByCreated,
                                   ThreadsCollectionByLastUpdated&& collectionByLastUpdated,
                                   ThreadsCollectionByMessageCount&& collectionByMessageCount,
                                   RetrieveDiscussionThreadsBy by, OutStream& output, const IdType& currentUserId)
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
        writeEntitiesWithPagination(collectionByName, "threads", output, displayContext.pageNumber, 
            pageSize, displayContext.sortOrder == Context::SortOrder::Ascending, writeInterceptor);
        break;
    case RetrieveDiscussionThreadsBy::Created:
        writeEntitiesWithPagination(collectionByCreated, "threads", output, displayContext.pageNumber, 
            pageSize, displayContext.sortOrder == Context::SortOrder::Ascending, writeInterceptor);
        break;
    case RetrieveDiscussionThreadsBy::LastUpdated:
        writeEntitiesWithPagination(collectionByLastUpdated, "threads", output, displayContext.pageNumber, 
            pageSize, displayContext.sortOrder == Context::SortOrder::Ascending, writeInterceptor);
        break;
    case RetrieveDiscussionThreadsBy::MessageCount:
        writeEntitiesWithPagination(collectionByMessageCount, "threads", output, displayContext.pageNumber, 
            pageSize, displayContext.sortOrder == Context::SortOrder::Ascending, writeInterceptor);
        break;
    }
}

template<typename ThreadsCollection>
static void writeDiscussionThreads(ThreadsCollection&& collection, RetrieveDiscussionThreadsBy by,
                                   OutStream& output, const IdType& currentUserId)
{
    writeDiscussionThreads(collection.threadsByName(), collection.threadsByCreated(), collection.threadsByLastUpdated(),
                           collection.threadsByMessageCount(), by, output, currentUserId);
}

StatusCode MemoryRepositoryDiscussionThread::getDiscussionThreads(OutStream& output, RetrieveDiscussionThreadsBy by) const
{
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, store());
                          
                          writeDiscussionThreads(collection, by, output, currentUser.id());

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
                          writeSingleValueSafeName(output, "thread", thread);
                          
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
                          writeDiscussionThreads(user, by, output, currentUser.id());

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
                                                 by, output, currentUser.id());

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
                          writeDiscussionThreads(tag, by, output, currentUser.id());

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
                         writeDiscussionThreads(category, by, output, currentUser.id());

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

                           auto thread = std::make_shared<DiscussionThread>(*currentUser);
                           thread->id() = generateUUIDString();
                           thread->name() = toString(name);
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

                           writeEvents().onAddNewDiscussionThread(createObserverContext(*currentUser), *thread);
                 
                           status.writeNow([&](auto& writer)
                                           {
                                               writer << Json::propertySafeName("id", thread->id());
                                               writer << Json::propertySafeName("name", thread->name());
                                               writer << Json::propertySafeName("created", thread->created());
                                           });
                       });
    return status;
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
                           
                           collection.modifyDiscussionThread(it, [&newName, &currentUser](DiscussionThread& thread)
                                                                 {
                                                                     thread.name() = toString(newName);
                                                                     thread.latestVisibleChange() = Context::getCurrentTime();
                                                                     updateLastUpdated(thread, currentUser);
                                                                 });
                           writeEvents().onChangeDiscussionThread(createObserverContext(*currentUser), **it,
                                                                  DiscussionThread::ChangeType::Name);
                       });
    return status;
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

                           collection.deleteDiscussionThread(it);
                       });
    return status;
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
                   
                           auto threadFromRef = *itFrom;
                           auto& threadFrom = **itFrom;
                           auto threadIntoRef = *itInto;
                           auto& threadInto = **itInto;
                   
                           if ( ! (status = authorization_->mergeDiscussionThreads(*currentUser, threadFrom, threadInto)))
                           {
                               return;
                           }

                           //make sure the thread is not deleted before being passed to the observers
                           writeEvents().onMergeDiscussionThreads(createObserverContext(*currentUser), 
                                                                  threadFrom, threadInto);
                   
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
                   
                           collection.modifyDiscussionThread(itInto, [&](DiscussionThread& thread)
                           {
                               updateLastUpdated(thread, currentUser);
                               thread.latestVisibleChange() = thread.lastUpdated();
                   
                               for (auto& message : threadFrom.messages())
                               {
                                   thread.insertMessage(message);
                               }
                   
                               updateMessageCounts(threadFromRef, - static_cast<int_fast32_t>(threadFrom.messages().size()));
                               updateMessageCounts(threadIntoRef,   static_cast<int_fast32_t>(threadFrom.messages().size()));
                   
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
                       });
    return status;
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

                           if ( ! (threadRef->subscribedUsers().insert(currentUser).second))
                           {
                               status = StatusCode::NO_EFFECT;
                               return;
                           }
                           
                           currentUser->subscribedThreads().insertDiscussionThread(threadRef);
                           
                           writeEvents().onSubscribeToDiscussionThread(createObserverContext(*currentUser), **it);
                       });
    return status;
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

                           if (0 == threadRef->subscribedUsers().erase(currentUser))
                           {
                               status = StatusCode::NO_EFFECT;
                               return;
                           }
                           
                           currentUser->subscribedThreads().deleteDiscussionThreadById(threadRef->id());
                           
                           writeEvents().onUnsubscribeFromDiscussionThread(createObserverContext(*currentUser), **it);
                       });
    return status;
}
