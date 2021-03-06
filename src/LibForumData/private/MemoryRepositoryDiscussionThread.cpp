/*
Fast Forum Backend
Copyright (C) 2016-present Daniel Jurcau

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "MemoryRepositoryDiscussionThread.h"

#include "Configuration.h"
#include "ContextProviders.h"
#include "EntitySerialization.h"
#include "OutputHelpers.h"
#include "RandomGenerator.h"
#include "StateHelpers.h"
#include "StringHelpers.h"
#include "Logging.h"

#include <boost/thread/tss.hpp>

#include <utility>

using namespace Forum;
using namespace Forum::Configuration;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;
using namespace Forum::Authorization;

MemoryRepositoryDiscussionThread::MemoryRepositoryDiscussionThread(MemoryStoreRef store,
                                                                   DiscussionThreadAuthorizationRef authorization,
                                                                   AuthorizationDirectWriteRepositoryRef authorizationDirectWriteRepository)
    : MemoryRepositoryBase(std::move(store)), authorization_(std::move(authorization)),
      authorizationDirectWriteRepository_(std::move(authorizationDirectWriteRepository))
{
    if ( ! authorization_)
    {
        throw std::runtime_error("Authorization implementation not provided");
    }
    if ( ! authorizationDirectWriteRepository_)
    {
        throw std::runtime_error("Authorization direct write repository implementation not provided");
    }
}

template<typename ThreadsCollection>
void writePinnedDiscussionThreads(const ThreadsCollection&, Json::JsonWriter&, SerializationRestriction&)
{
    //do nothing
}

template<>
inline void writePinnedDiscussionThreads<DiscussionThreadCollectionWithHashedIdAndPinOrder>
        (const DiscussionThreadCollectionWithHashedIdAndPinOrder& collection, Json::JsonWriter& writer,
         SerializationRestriction& restriction)
{
    writer.newPropertyRaw(JSON_RAW_PROP_COMMA("pinned_threads"));
    writer.startArray();

    auto it = collection.byPinDisplayOrder().begin(); //collection is sorted in greater order
    auto end = collection.byPinDisplayOrder().end();

    for (; (it != end) && ((*it)->pinDisplayOrder() > 0); ++it)
    {
        serialize(writer, **it, restriction);
    }

    writer.endArray();
}

template<typename ThreadsCollection>
static void writeDiscussionThreads(const ThreadsCollection& collection, RetrieveDiscussionThreadsBy by,
                                   OutStream& output, const GrantedPrivilegeStore& privilegeStore, 
                                   const ForumWidePrivilegeStore& forumWidePrivilegeStore, const User& currentUser)
{
    BoolTemporaryChanger _(serializationSettings.visitedThreadSinceLastChange, false);
    BoolTemporaryChanger __(serializationSettings.hideDiscussionThreadMessages, true);

    auto writeFilter = [&](const DiscussionThread& currentThread)
    {
        bool visitedThreadSinceLastChange = false;
        if ( ! isAnonymousUser(currentUser))
        {
            visitedThreadSinceLastChange = currentThread.hasVisitedSinceLastEdit(currentUser.id());
        }
        serializationSettings.visitedThreadSinceLastChange = visitedThreadSinceLastChange;
        return true;
    };

    auto pageSize = getGlobalConfig()->discussionThread.maxThreadsPerPage;
    auto& displayContext = Context::getDisplayContext();

    SerializationRestriction restriction(privilegeStore, forumWidePrivilegeStore, 
                                         &currentUser, Context::getCurrentTime());

    auto ascending = displayContext.sortOrder == Context::SortOrder::Ascending;

    Json::JsonWriter writer(output);

    writer.startObject();

    switch (by)
    {
    case RetrieveDiscussionThreadsBy::Name:
        writeEntitiesWithPagination(collection.byName(), displayContext.pageNumber, pageSize, ascending, "threads",
            writer, writeFilter, restriction);
        break;
    case RetrieveDiscussionThreadsBy::Created:
        writeEntitiesWithPagination(collection.byCreated(), displayContext.pageNumber, pageSize, ascending, "threads",
            writer, writeFilter, restriction);
        break;
    case RetrieveDiscussionThreadsBy::LastUpdated:
        writeEntitiesWithPagination(collection.byLastUpdated(), displayContext.pageNumber, pageSize, ascending, "threads",
            writer, writeFilter, restriction);
        break;
    case RetrieveDiscussionThreadsBy::LatestMessageCreated:
        writeEntitiesWithPagination(collection.byLatestMessageCreated(), displayContext.pageNumber, pageSize, ascending, "threads",
            writer, writeFilter, restriction);
        break;
    case RetrieveDiscussionThreadsBy::MessageCount:
        //collection is sorted in greater order
        writeEntitiesWithPagination(collection.byMessageCount(), displayContext.pageNumber, pageSize, ! ascending, "threads",
            writer, writeFilter, restriction);
        break;
    }

    if (0 == displayContext.pageNumber)
    {
        writePinnedDiscussionThreads(collection, writer, restriction);
    }

    writer.endObject();
}

StatusCode MemoryRepositoryDiscussionThread::getDiscussionThreads(OutStream& output, RetrieveDiscussionThreadsBy by) const
{
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);

                          TemporaryChanger<UserConstPtr> _(serializationSettings.currentUser, &currentUser);

                          writeDiscussionThreads(collection.threads(), by, output, collection.grantedPrivileges(), 
                                                 collection, currentUser);

                          readEvents().onGetDiscussionThreads(createObserverContext(currentUser));
                      });
    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionThread::getDiscussionThreadById(IdTypeRef id, OutStream& output)
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    bool addUserToVisitedSinceLastEdit = false;
    IdType userId{};

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);

                          auto threadPtr = collection.threads().findById(id);
                          if ( ! threadPtr)
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }

                          auto& thread = *threadPtr;

                          if ( ! (status = authorization_->getDiscussionThreadById(currentUser, thread)))
                          {
                              return;
                          }

                          thread.visited().fetch_add(1);

                          const auto& displayContext = Context::getDisplayContext();
                          uint32_t latestPageNumberToPersist{};

                          if ( ! isAnonymousUser(currentUser))
                          {
                              if ( ! thread.hasVisitedSinceLastEdit(currentUser.id()))
                              {
                                  addUserToVisitedSinceLastEdit = true;
                                  userId = currentUser.id();
                              }

                              if (displayContext.pageNumber > 0)
                              {
                                  if (currentUser.updateLatestPageVisited(id, displayContext.pageNumber))
                                  {
                                      latestPageNumberToPersist = displayContext.pageNumber;
                                  }
                              }
                          }

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
                          BoolTemporaryChanger ___(serializationSettings.hideLatestMessage, true);
                          TemporaryChanger<UserConstPtr> ____(serializationSettings.currentUser, &currentUser);

                          status.disable();

                          SerializationRestriction restriction(collection.grantedPrivileges(), collection,
                                                               &currentUser, Context::getCurrentTime());

                          writeSingleValueSafeName(output, "thread", thread, restriction);

                          readEvents().onGetDiscussionThreadById(createObserverContext(currentUser), thread, 
                                                                 latestPageNumberToPersist);
                      });
    if (addUserToVisitedSinceLastEdit)
    {
        collection().write([&](EntityCollection& collection)
                           {
                               auto threadPtr = collection.threads().findById(id);
                               if (threadPtr)
                               {
                                   threadPtr->addVisitorSinceLastEdit(userId);
                               }
                           });
    }
    return status;
}

StatusCode MemoryRepositoryDiscussionThread::getMultipleDiscussionThreadsById(StringView ids, OutStream& output) const
{
    StatusWriter status(output);
    PerformedByWithLastSeenUpdateGuard performedBy;

    constexpr size_t MaxIdBuffer = 64;
    static boost::thread_specific_ptr<std::array<UuidString, MaxIdBuffer>> parsedIdsPtr;
    static boost::thread_specific_ptr<std::array<const DiscussionThread*, MaxIdBuffer>> threadsFoundPtr;

    if ( ! parsedIdsPtr.get())
    {
        parsedIdsPtr.reset(new std::array<UuidString, MaxIdBuffer>);
    }
    auto& parsedIds = *parsedIdsPtr;
    if ( ! threadsFoundPtr.get())
    {
        threadsFoundPtr.reset(new std::array<const DiscussionThread*, MaxIdBuffer>);
    }
    auto& threadsFound = *threadsFoundPtr;

    const auto maxThreadsToSearch = std::min(MaxIdBuffer, 
                                             static_cast<size_t>(getGlobalConfig()->discussionThread.maxThreadsPerPage));
    auto lastParsedId = parseMultipleUuidStrings(ids, parsedIds.begin(), parsedIds.begin() + maxThreadsToSearch);

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);

                          const auto& threads = collection.threads();
                          auto lastThreadFound = std::transform(parsedIds.begin(), lastParsedId, threadsFound.begin(), 
                              [&threads](auto id)
                              {
                                  return threads.findById(id);
                              });
                          
                          status = StatusCode::OK;
                          status.disable();

                          BoolTemporaryChanger _(serializationSettings.hideDiscussionThreadMessages, true);
                          TemporaryChanger<UserConstPtr> __(serializationSettings.currentUser, &currentUser);

                          SerializationRestriction restriction(collection.grantedPrivileges(), collection,
                                                               &currentUser, Context::getCurrentTime());

                          writeAllEntities(threadsFound.begin(), lastThreadFound, "threads", output, restriction);
                          
                          readEvents().onGetMultipleDiscussionThreadsById(createObserverContext(currentUser), ids);
                      });
    return status;
}

StatusCode MemoryRepositoryDiscussionThread::searchDiscussionThreadsByName(StringView name, OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    if (countUTF8Characters(name) > getGlobalConfig()->discussionThread.maxNameLength)
    {
        return status = INVALID_PARAMETERS;
    }

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);

                          readEvents().onSearchDiscussionThreadsByName(createObserverContext(currentUser), name);

                          DiscussionThread::NameType nameString(name);

                          const auto& index = collection.threads().byName();
                          auto boundIndex = index.lower_bound_rank(nameString);
                          if (boundIndex >= index.size())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }

                          status = StatusCode::OK;

                          const auto pageSize = getGlobalConfig()->discussionThread.maxThreadsPerPage;

                          status.writeNow([&](auto& writer)
                                          {
                                              JSON_WRITE_PROP(writer, "index", boundIndex);
                                              JSON_WRITE_PROP(writer, "pageSize", pageSize);
                                          });
                      });
    return status;
}

StatusCode MemoryRepositoryDiscussionThread::getDiscussionThreadsOfUser(IdTypeRef id, OutStream& output,
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
                          auto& currentUser = performedBy.get(collection, *store_);

                          const auto& indexById = collection.users().byId();
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
                          TemporaryChanger<UserConstPtr> __(serializationSettings.currentUser, &currentUser);

                          status.disable();
                          writeDiscussionThreads(user.threads(), by, output, collection.grantedPrivileges(), 
                                                 collection, currentUser);

                          readEvents().onGetDiscussionThreadsOfUser(createObserverContext(currentUser), user);
                      });
    return status;
}

StatusCode MemoryRepositoryDiscussionThread::getSubscribedDiscussionThreadsOfUser(IdTypeRef id, OutStream& output,
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
                          auto& currentUser = performedBy.get(collection, *store_);

                          const auto& indexById = collection.users().byId();
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

                          status.disable();

                          TemporaryChanger<UserConstPtr> _(serializationSettings.currentUser, &currentUser);

                          writeDiscussionThreads(user.subscribedThreads(), by, output, collection.grantedPrivileges(),
                                                 collection, currentUser);

                          readEvents().onGetDiscussionThreadsOfUser(createObserverContext(currentUser), user);
                      });
    return status;
}

StatusCode MemoryRepositoryDiscussionThread::getUsersSubscribedToDiscussionThread(IdTypeRef id, OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);

                          auto threadPtr = collection.threads().findById(id);
                          if ( ! threadPtr)
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }

                          const DiscussionThread& thread = *threadPtr;

                          if ( ! (status = authorization_->getDiscussionThreadSubscribedUsers(currentUser, thread)))
                          {
                              return;
                          }

                          status.disable();

                          SerializationRestriction restriction(collection.grantedPrivileges(), collection,
                                                               &currentUser, Context::getCurrentTime());

                          Json::JsonWriter writer(output);
                          writer.startObject();
                          writer.newPropertyRaw(JSON_RAW_PROP("users"));

                          writer.startArray();
                          for (UserConstPtr userPtr : thread.subscribedUsers())
                          {
                              if (userPtr)
                              {
                                  serialize(writer, *userPtr, restriction);
                              }
                          }
                          writer.endArray();

                          writer.endObject();

                          readEvents().onGetUsersSubscribedToDiscussionThread(createObserverContext(currentUser), thread);
                      });
    return status;
}

StatusCode MemoryRepositoryDiscussionThread::getDiscussionThreadsWithTag(IdTypeRef id, OutStream& output,
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
                          auto& currentUser = performedBy.get(collection, *store_);

                          const auto& indexById = collection.tags().byId();
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

                          TemporaryChanger<UserConstPtr> _(serializationSettings.currentUser, &currentUser);

                          writeDiscussionThreads(tag.threads(), by, output, collection.grantedPrivileges(),
                                                 collection, currentUser);

                          readEvents().onGetDiscussionThreadsWithTag(createObserverContext(currentUser), tag);
                      });
    return status;
}

StatusCode MemoryRepositoryDiscussionThread::getDiscussionThreadsOfCategory(IdTypeRef id, OutStream& output,
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
                         auto& currentUser = performedBy.get(collection, *store_);

                         const auto& indexById = collection.categories().byId();
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

                         TemporaryChanger<UserConstPtr> _(serializationSettings.currentUser, &currentUser);

                         writeDiscussionThreads(category.threads(), by, output, collection.grantedPrivileges(),
                                                collection, currentUser);

                         readEvents().onGetDiscussionThreadsOfCategory(createObserverContext(currentUser), category);
                     });
    return status;
}


StatusCode MemoryRepositoryDiscussionThread::addNewDiscussionThread(StringView name, OutStream& output)
{
    StatusWriter status(output);

    const auto config = getGlobalConfig();
    const auto validationCode = validateString(name, INVALID_PARAMETERS_FOR_EMPTY_STRING,
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

                           const bool approved = AuthorizationStatus::OK
                               == authorization_->autoApproveDiscussionThread(*currentUser);

                           auto statusWithResource = addNewDiscussionThread(collection, generateUniqueId(), name, approved);
                           auto& thread = *statusWithResource.resource;
                           if ( ! (status = statusWithResource.status)) return;

                           writeEvents().onAddNewDiscussionThread(createObserverContext(*currentUser), thread);

                           if ( ! isAnonymousUser(currentUser))
                           {
                               auto levelToGrant = collection.getForumWideDefaultPrivilegeLevel(
                                       ForumWideDefaultPrivilegeDuration::CREATE_DISCUSSION_THREAD);
                               if (levelToGrant)
                               {
                                   const auto value = levelToGrant->value;
                                   const auto duration = levelToGrant->duration;

                                   authorizationDirectWriteRepository_->assignDiscussionThreadPrivilege(
                                           collection, thread.id(), currentUser->id(), value, duration);
                                   writeEvents().onAssignDiscussionThreadPrivilege(
                                           createObserverContext(*currentUser), thread, *currentUser, value, duration);
                               }
                           }
                           status.writeNow([&](auto& writer)
                                           {
                                               JSON_WRITE_PROP(writer, "id", thread.id());
                                               JSON_WRITE_PROP(writer, "name", thread.name().string());
                                               JSON_WRITE_PROP(writer, "created", thread.created());
                                           });
                       });
    return status;
}

StatusWithResource<DiscussionThreadPtr> MemoryRepositoryDiscussionThread::addNewDiscussionThread(EntityCollection& collection,
                                                                                                 IdTypeRef id,
                                                                                                 const StringView name,
                                                                                                 const bool approved)
{
    auto currentUser = getCurrentUser(collection);

    auto thread = collection.createDiscussionThread(id, *currentUser, DiscussionThread::NameType(name),
                                                    Context::getCurrentTime(), { Context::getCurrentUserIpAddress() },
                                                    approved);
    thread->updateLastUpdated(thread->latestVisibleChange() = thread->created());

    collection.insertDiscussionThread(thread);
    currentUser->threads().add(thread);

    return thread;
}


StatusCode MemoryRepositoryDiscussionThread::changeDiscussionThreadName(IdTypeRef id, StringView newName,
                                                                        OutStream& output)
{
    StatusWriter status(output);

    const auto config = getGlobalConfig();
    const auto validationCode = validateString(newName, INVALID_PARAMETERS_FOR_EMPTY_STRING,
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

                           auto threadPtr = collection.threads().findById(id);
                           if ( ! threadPtr)
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           if ( ! (status = authorization_->changeDiscussionThreadName(*currentUser, *threadPtr, newName)))
                           {
                               return;
                           }

                           if ( ! (status = changeDiscussionThreadName(collection, id, newName))) return;

                           writeEvents().onChangeDiscussionThread(createObserverContext(*currentUser), *threadPtr,
                                                                  DiscussionThread::ChangeType::Name);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionThread::changeDiscussionThreadName(EntityCollection& collection, IdTypeRef id,
                                                                        StringView newName)
{
    auto threadPtr = collection.threads().findById(id);
    if ( ! threadPtr)
    {
        FORUM_LOG_ERROR << "Could not find discussion thread: " << id.toStringDashed();
        return StatusCode::NOT_FOUND;
    }

    const auto currentUser = getCurrentUser(collection);
    
    threadPtr->updateName(DiscussionThread::NameType(newName));
    updateThreadLastUpdated(*threadPtr, currentUser);

    return StatusCode::OK;
}


StatusCode MemoryRepositoryDiscussionThread::changeDiscussionThreadPinDisplayOrder(IdTypeRef id, uint16_t newValue,
                                                                                   OutStream& output)
{
    StatusWriter status(output);
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto threadPtr = collection.threads().findById(id);
                           if ( ! threadPtr)
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           if ( ! (status = authorization_->changeDiscussionThreadPinDisplayOrder(*currentUser, *threadPtr, newValue)))
                           {
                               return;
                           }

                           if ( ! (status = changeDiscussionThreadPinDisplayOrder(collection, id, newValue))) return;

                           writeEvents().onChangeDiscussionThread(createObserverContext(*currentUser), *threadPtr,
                                                                  DiscussionThread::ChangeType::PinDisplayOrder);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionThread::changeDiscussionThreadPinDisplayOrder(EntityCollection& collection,
                                                                                   IdTypeRef id, uint16_t newValue)
{
    DiscussionThreadPtr threadPtr = collection.threads().findById(id);
    if ( ! threadPtr)
    {
        FORUM_LOG_ERROR << "Could not find discussion thread: " << id.toStringDashed();
        return StatusCode::NOT_FOUND;
    }

    const auto currentUser = getCurrentUser(collection);

    threadPtr->updatePinDisplayOrder(newValue);
    updateThreadLastUpdated(*threadPtr, currentUser);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionThread::changeDiscussionThreadApproval(IdTypeRef id, const bool newApproval,
                                                                            OutStream& output)
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto threadPtr = collection.threads().findById(id);
                           if ( ! threadPtr)
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           if ( ! (status = authorization_->changeDiscussionThreadApproval(*currentUser, *threadPtr, newApproval)))
                           {
                               return;
                           }

                           if ( ! (status = changeDiscussionThreadApproval(collection, id, newApproval))) return;

                           const auto& write = writeEvents();
                           const auto observerContext = createObserverContext(*currentUser);

                           write.onChangeDiscussionThread(observerContext, *threadPtr, 
                                                          DiscussionThread::ChangeType::Approval);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionThread::changeDiscussionThreadApproval(EntityCollection& collection,
                                                                            IdTypeRef id, const bool newApproval)
{
    DiscussionThreadPtr threadPtr = collection.threads().findById(id);
    if ( ! threadPtr)
    {
        FORUM_LOG_ERROR << "Could not find discussion thread: " << id.toStringDashed();
        return StatusCode::NOT_FOUND;
    }

    const auto currentUser = getCurrentUser(collection);

    threadPtr->setApproved(newApproval);
    updateThreadLastUpdated(*threadPtr, currentUser);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionThread::deleteDiscussionThread(IdTypeRef id, OutStream& output)
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

                           auto threadPtr = collection.threads().findById(id);
                           if ( ! threadPtr)
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           DiscussionThread& thread = *threadPtr;
                           if ( ! (status = authorization_->deleteDiscussionThread(*currentUser, thread)))
                           {
                               return;
                           }

                           //make sure the thread is not deleted before being passed to the observers
                           writeEvents().onDeleteDiscussionThread(createObserverContext(*currentUser), thread);

                           status = deleteDiscussionThread(collection, id);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionThread::deleteDiscussionThread(EntityCollection& collection, IdTypeRef id)
{
    auto threadPtr = collection.threads().findById(id);
    if ( ! threadPtr)
    {
        FORUM_LOG_ERROR << "Could not find discussion thread: " << id.toStringDashed();
        return StatusCode::NOT_FOUND;
    }

    collection.deleteDiscussionThread(threadPtr, true);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionThread::mergeDiscussionThreads(IdTypeRef fromId, IdTypeRef intoId,
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

                           auto threadFromPtr = collection.threads().findById(fromId);
                           if ( ! threadFromPtr)
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           auto threadIntoPtr = collection.threads().findById(intoId);
                           if ( ! threadIntoPtr)
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           auto& threadFrom = *threadFromPtr;
                           auto& threadInto = *threadIntoPtr;

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

static void updateMessageCounts(DiscussionThreadPtr thread, int_fast32_t difference)
{
    for (DiscussionTagPtr tag : thread->tags())
    {
        assert(tag);
        tag->updateMessageCount(difference);
    }

    for (DiscussionCategoryPtr category : thread->categories())
    {
        assert(category);
        category->updateMessageCount(thread, difference);
    }
}

StatusCode MemoryRepositoryDiscussionThread::mergeDiscussionThreads(EntityCollection& collection,
                                                                    IdTypeRef fromId, IdTypeRef intoId)
{
    const auto currentUser = getCurrentUser(collection);

    if (fromId == intoId)
    {
        FORUM_LOG_ERROR << "Cannot merge discussion thread into self: " << fromId.toStringDashed();
        return StatusCode::NO_EFFECT;
    }

    auto threadFromPtr = collection.threads().findById(fromId);
    if ( ! threadFromPtr)
    {
        FORUM_LOG_ERROR << "Could not find discussion thread: " << fromId.toStringDashed();
        return StatusCode::NOT_FOUND;
    }

    auto threadIntoPtr = collection.threads().findById(intoId);
    if ( ! threadIntoPtr)
    {
        FORUM_LOG_ERROR << "Could not find discussion thread: " << intoId.toStringDashed();
        return StatusCode::NOT_FOUND;
    }

    DiscussionThread& threadFrom = *threadFromPtr;
    DiscussionThread& threadInto = *threadIntoPtr;

    updateThreadLastUpdated(threadInto, currentUser);

    for (DiscussionThreadMessagePtr message : threadFrom.messages().byId())
    {
        message->parentThread() = threadIntoPtr;
    }

    threadInto.insertMessages(threadFrom.messages());

    updateMessageCounts(threadFromPtr, - static_cast<int_fast32_t>(threadFrom.messageCount()));
    updateMessageCounts(threadIntoPtr,   static_cast<int_fast32_t>(threadFrom.messageCount()));

    //update subscriptions
    for (UserPtr userPtr : threadFrom.subscribedUsers())
    {
        assert(userPtr);
        userPtr->subscribedThreads().add(threadIntoPtr);
        threadInto.subscribedUsers().insert(userPtr);
    }

    //this will also decrease the message count on the tags the thread was part of
    collection.deleteDiscussionThread(threadFromPtr, false);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionThread::subscribeToDiscussionThread(IdTypeRef id, OutStream& output)
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

                           if (isAnonymousUser(currentUser))
                           {
                               status = StatusCode::NOT_ALLOWED;
                               return;
                           }

                           auto threadPtr = collection.threads().findById(id);
                           if ( ! threadPtr)
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           if ( ! (status = authorization_->subscribeToDiscussionThread(*currentUser, *threadPtr)))
                           {
                               return;
                           }

                           if ( ! (status = subscribeToDiscussionThread(collection, id))) return;

                           writeEvents().onSubscribeToDiscussionThread(createObserverContext(*currentUser), *threadPtr);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionThread::subscribeToDiscussionThread(EntityCollection& collection, IdTypeRef id)
{
    DiscussionThreadPtr threadPtr = collection.threads().findById(id);
    if ( ! threadPtr)
    {
        FORUM_LOG_ERROR << "Could not find discussion thread: " << id.toStringDashed();
        return StatusCode::NOT_FOUND;
    }

    auto currentUser = getCurrentUser(collection);

    if ( ! std::get<1>(threadPtr->subscribedUsers().insert(currentUser)))
    {
        //FORUM_LOG_WARNING << "The user " << currentUser->id().toStringDashed()
        //                  << " is already subscribed to the discussion thread " << id.toStringDashed();

        return StatusCode::NO_EFFECT;
    }

    currentUser->subscribedThreads().add(threadPtr);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionThread::unsubscribeFromDiscussionThread(IdTypeRef id, OutStream& output)
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

                           if (isAnonymousUser(currentUser))
                           {
                               status = StatusCode::NOT_ALLOWED;
                               return;
                           }

                           auto threadPtr = collection.threads().findById(id);
                           if ( ! threadPtr)
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           if ( ! (status = authorization_->unsubscribeFromDiscussionThread(*currentUser, *threadPtr)))
                           {
                               return;
                           }

                           if ( ! (status = unsubscribeFromDiscussionThread(collection, id))) return;

                           writeEvents().onUnsubscribeFromDiscussionThread(createObserverContext(*currentUser), *threadPtr);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionThread::unsubscribeFromDiscussionThread(EntityCollection& collection, IdTypeRef id)
{
    DiscussionThreadPtr threadPtr = collection.threads().findById(id);
    if ( ! threadPtr)
    {
        FORUM_LOG_ERROR << "Could not find discussion thread: " << id.toStringDashed();
        return StatusCode::NOT_FOUND;
    }

    auto currentUser = getCurrentUser(collection);

    if (0 == threadPtr->subscribedUsers().erase(currentUser))
    {
        //FORUM_LOG_WARNING << "The user " << currentUser->id().toStringDashed()
        //                  << " was not subscribed to the discussion thread " << id.toStringDashed();

        return StatusCode::NO_EFFECT;
    }

    currentUser->subscribedThreads().remove(threadPtr);
    return StatusCode::OK;
}
