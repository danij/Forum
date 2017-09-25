#include "MemoryRepositoryDiscussionThread.h"

#include "Configuration.h"
#include "ContextProviders.h"
#include "EntitySerialization.h"
#include "OutputHelpers.h"
#include "RandomGenerator.h"
#include "StateHelpers.h"
#include "StringHelpers.h"
#include "Logging.h"

#include <utility>

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

template<typename ThreadsCollection>
inline void writePinnedDiscussionThreads(const ThreadsCollection&, Json::JsonWriter&, SerializationRestriction&)
{
    //do nothing
}

template<>
inline void writePinnedDiscussionThreads<DiscussionThreadCollectionWithHashedIdAndPinOrder>
        (const DiscussionThreadCollectionWithHashedIdAndPinOrder& collection, Json::JsonWriter& writer,
         SerializationRestriction& restriction)
{
    writer.newPropertyWithSafeName("pinned_threads");
    writer.startArray();

    auto it = collection.byPinDisplayOrder().rbegin();
    auto end = collection.byPinDisplayOrder().rend();

    for (; (it != end) && ((*it)->pinDisplayOrder() > 0); ++it)
    {
        serialize(writer, **it, restriction);
    }

    writer.endArray();
}

template<typename ThreadsCollection>
static void writeDiscussionThreads(const ThreadsCollection& collection, RetrieveDiscussionThreadsBy by,
                                   OutStream& output, const GrantedPrivilegeStore& privilegeStore, const User& currentUser)
{
    BoolTemporaryChanger _(serializationSettings.visitedThreadSinceLastChange, false);
    BoolTemporaryChanger __(serializationSettings.hideDiscussionThreadMessages, true);

    auto writeFilter = [&](const DiscussionThread& currentThread)
    {
        bool visitedThreadSinceLastChange = false;
        if (currentUser.id() != anonymousUserId())
        {
            visitedThreadSinceLastChange = currentThread.hasVisitedSinceLastEdit(currentUser.id());
        }
        serializationSettings.visitedThreadSinceLastChange = visitedThreadSinceLastChange;
        return true;
    };

    auto pageSize = getGlobalConfig()->discussionThread.maxThreadsPerPage;
    auto& displayContext = Context::getDisplayContext();

    SerializationRestriction restriction(privilegeStore, currentUser.id(), Context::getCurrentTime());

    auto ascending = displayContext.sortOrder == Context::SortOrder::Ascending;

    Json::JsonWriter writer(output);

    writer.startObject();

    if (0 == displayContext.pageNumber)
    {
        writePinnedDiscussionThreads(collection, writer, restriction);
    }

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
    case RetrieveDiscussionThreadsBy::MessageCount:
        writeEntitiesWithPagination(collection.byMessageCount(), displayContext.pageNumber, pageSize, ascending, "threads",
            writer, writeFilter, restriction);
        break;
    }

    writer.endObject();
}

StatusCode MemoryRepositoryDiscussionThread::getDiscussionThreads(OutStream& output, RetrieveDiscussionThreadsBy by) const
{
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);

                          writeDiscussionThreads(collection.threads(), by, output, collection.grantedPrivileges(), currentUser);

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

                          const auto& index = collection.threads().byId();
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

                          if (currentUser.id() != anonymousUserId())
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

                          SerializationRestriction restriction(collection.grantedPrivileges(), currentUser.id(),
                                                               Context::getCurrentTime());

                          writeSingleValueSafeName(output, "thread", thread, restriction);

                          readEvents().onGetDiscussionThreadById(createObserverContext(currentUser), thread);
                      });
    if (addUserToVisitedSinceLastEdit)
    {
        collection().write([&](EntityCollection& collection)
                           {
                               auto& index = collection.threads().byId();
                               auto it = index.find(id);
                               if (it != index.end())
                               {
                                   DiscussionThreadPtr threadPtr = *it;
                                   threadPtr->addVisitorSinceLastEdit(userId);
                               }
                           });
    }
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

                          status.disable();
                          writeDiscussionThreads(user.threads(), by, output, collection.grantedPrivileges(), currentUser);

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

                          BoolTemporaryChanger _(serializationSettings.hideDiscussionThreadCreatedBy, true);

                          status.disable();
                          writeDiscussionThreads(user.subscribedThreads(), by, output, collection.grantedPrivileges(),
                                                 currentUser);

                          readEvents().onGetDiscussionThreadsOfUser(createObserverContext(currentUser), user);
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
                          writeDiscussionThreads(tag.threads(), by, output, collection.grantedPrivileges(), currentUser);

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
                         writeDiscussionThreads(category.threads(), by, output, collection.grantedPrivileges(), currentUser);

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

                           auto statusWithResource = addNewDiscussionThread(collection, generateUniqueId(), name);
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

StatusWithResource<DiscussionThreadPtr> MemoryRepositoryDiscussionThread::addNewDiscussionThread(EntityCollection& collection,
                                                                                                 IdTypeRef id,
                                                                                                 StringView name)
{
    auto currentUser = getCurrentUser(collection);

    auto thread = collection.createDiscussionThread(id, *currentUser, DiscussionThread::NameType(name),
                                                    Context::getCurrentTime(), { Context::getCurrentUserIpAddress() });
    thread->updateLastUpdated(thread->latestVisibleChange() = thread->created());

    collection.insertDiscussionThread(thread);
    currentUser->threads().add(thread);

    //add privileges for the user that created the message
    auto changePrivilegeDuration = optionalOrZero(collection.getForumWideDefaultPrivilegeDuration(
            ForumWideDefaultPrivilegeDuration::CHANGE_DISCUSSION_THREAD_NAME));

    if (changePrivilegeDuration > 0)
    {
        auto privilege = DiscussionThreadPrivilege::CHANGE_NAME;
        auto valueNeeded = optionalOrZero(collection.getDiscussionThreadPrivilege(privilege));

        if (valueNeeded > 0)
        {
            auto expiresAt = calculatePrivilegeExpires(thread->created(), changePrivilegeDuration);

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
            auto expiresAt = calculatePrivilegeExpires(thread->created(), changePrivilegeDuration);

            collection.grantedPrivileges().grantDiscussionThreadPrivilege(
                currentUser->id(), thread->id(), privilege, valueNeeded, expiresAt);
        }
    }

    return thread;
}


StatusCode MemoryRepositoryDiscussionThread::changeDiscussionThreadName(IdTypeRef id, StringView newName,
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

                           auto& indexById = collection.threads().byId();
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
    auto& indexById = collection.threads().byId();
    auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion thread: " << static_cast<std::string>(id);
        return StatusCode::NOT_FOUND;
    }

    auto currentUser = getCurrentUser(collection);

    DiscussionThreadPtr thread = *it;

    thread->updateName(DiscussionThread::NameType(newName));
    updateThreadLastUpdated(*thread, currentUser);

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

                           auto& indexById = collection.threads().byId();
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
    auto& indexById = collection.threads().byId();
    auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion thread: " << static_cast<std::string>(id);
        return StatusCode::NOT_FOUND;
    }

    auto currentUser = getCurrentUser(collection);

    DiscussionThreadPtr thread = *it;
    thread->updatePinDisplayOrder(newValue);
    updateThreadLastUpdated(*thread, currentUser);

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

                           auto& indexById = collection.threads().byId();
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
    auto& indexById = collection.threads().byId();
    auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion thread: " << static_cast<std::string>(id);
        return StatusCode::NOT_FOUND;
    }

    collection.deleteDiscussionThread(*it);

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

                           auto& indexById = collection.threads().byId();
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
    auto currentUser = getCurrentUser(collection);

    if (fromId == intoId)
    {
        FORUM_LOG_ERROR << "Cannot merge discussion thread into self: " << static_cast<std::string>(fromId);
        return StatusCode::NO_EFFECT;
    }

    auto& indexById = collection.threads().byId();
    auto itFrom = indexById.find(fromId);
    if (itFrom == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion thread: " << static_cast<std::string>(fromId);
        return StatusCode::NOT_FOUND;
    }
    auto itInto = indexById.find(intoId);
    if (itInto == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion thread: " << static_cast<std::string>(intoId);
        return StatusCode::NOT_FOUND;
    }

    DiscussionThreadPtr threadFromPtr = *itFrom;
    DiscussionThread& threadFrom = *threadFromPtr;
    DiscussionThreadPtr threadIntoPtr = *itInto;
    DiscussionThread& threadInto = *threadIntoPtr;

    updateThreadLastUpdated(threadInto, currentUser);

    for (DiscussionThreadMessagePtr message : threadFrom.messages().byId())
    {
        message->parentThread() = threadIntoPtr;
        threadInto.insertMessage(message);
    }

    updateMessageCounts(threadFromPtr, - static_cast<int_fast32_t>(threadFrom.messageCount()));
    updateMessageCounts(threadIntoPtr,   static_cast<int_fast32_t>(threadFrom.messageCount()));

    //remove all message references from the thread so they don't get deleted
    threadFrom.messages().clear();

    //update subscriptions
    for (UserPtr user : threadFrom.subscribedUsers())
    {
        assert(user);
        user->subscribedThreads().add(threadIntoPtr);
        threadInto.subscribedUsers().insert(user);
    }

    //this will also decrease the message count on the tags the thread was part of
    collection.deleteDiscussionThread(threadFromPtr);

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

                           auto& indexById = collection.threads().byId();
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
    auto& indexById = collection.threads().byId();
    auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion thread: " << static_cast<std::string>(id);
        return StatusCode::NOT_FOUND;
    }

    auto thread = *it;
    auto currentUser = getCurrentUser(collection);

    if ( ! std::get<1>(thread->subscribedUsers().insert(currentUser)))
    {
        FORUM_LOG_WARNING << "The user " << static_cast<std::string>(currentUser->id())
                          << " is already subscribed to the discussion thread " << static_cast<std::string>(id);

        return StatusCode::NO_EFFECT;
    }

    currentUser->subscribedThreads().add(thread);

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

                           auto& indexById = collection.threads().byId();
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
    auto& indexById = collection.threads().byId();
    auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion thread: " << static_cast<std::string>(id);
        return StatusCode::NOT_FOUND;
    }

    auto thread = *it;
    auto currentUser = getCurrentUser(collection);

    if (0 == thread->subscribedUsers().erase(currentUser))
    {
        FORUM_LOG_WARNING << "The user " << static_cast<std::string>(currentUser->id())
                          << " was not subscribed to the discussion thread " << static_cast<std::string>(id);

        return StatusCode::NO_EFFECT;
    }

    currentUser->subscribedThreads().remove(thread);
    return StatusCode::OK;
}
