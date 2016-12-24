#include "MemoryRepository.h"

#include "Configuration.h"
#include "ContextProviders.h"
#include "EntitySerialization.h"
#include "OutputHelpers.h"

using namespace Forum;
using namespace Forum::Configuration;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;

MemoryRepository::MemoryRepository() : collection_(std::make_shared<EntityCollection>())
{
}

void MemoryRepository::addObserver(const ReadRepositoryObserverRef& observer)
{
    observers_.addObserver(observer);
}

void MemoryRepository::addObserver(const WriteRepositoryObserverRef& observer)
{
    observers_.addObserver(observer);
}

void MemoryRepository::removeObserver(const ReadRepositoryObserverRef& observer)
{
    observers_.removeObserver(observer);
}

void MemoryRepository::removeObserver(const WriteRepositoryObserverRef& observer)
{
    observers_.removeObserver(observer);
}


void MemoryRepository::getEntitiesCount(std::ostream& output) const
{
    auto performedBy = preparePerformedBy(*this);

    collection_.read([&](const EntityCollection& collection)
                     {
                         EntitiesCount count;
                         count.nrOfUsers = collection.usersById().size();
                         count.nrOfDiscussionThreads = collection.threadsById().size();
                         count.nrOfDiscussionMessages = collection.messagesById().size();

                         writeSingleValueSafeName(output, "count", count);

                         observers_.onGetEntitiesCount(createObserverContext(performedBy.get(collection)));
                     });
}


/**
 * Retrieves the user that is performing the current action and also performs an update on the last seen if needed
 * The update is performed on the spot if a write lock is held or
 * delayed until the lock is destroyed in the case of a read lock, to avoid deadlocks
 * Do not keep references to it outside of MemoryRepository methods
 */
PerformedByWithLastSeenUpdateGuard::PerformedByWithLastSeenUpdateGuard(const MemoryRepository& repository) :
        repository_(const_cast<MemoryRepository&>(repository))
{
}

PerformedByWithLastSeenUpdateGuard::~PerformedByWithLastSeenUpdateGuard()
{
    if (lastSeenUpdate_) lastSeenUpdate_();
}

PerformedByType PerformedByWithLastSeenUpdateGuard::get(const EntityCollection& collection)
{
    const auto& index = collection.usersById();
    auto it = index.find(Context::getCurrentUserId());
    if (it == index.end())
    {
        return *AnonymousUser;
    }
    auto& result = **it;

    auto now = Context::getCurrentTime();

    if ((result.lastSeen() + getGlobalConfig()->user.lastSeenUpdatePrecision) < now)
    {
        auto& userId = result.id();
        auto collection = &(repository_.collection_);
        lastSeenUpdate_ = [collection, now, userId]()
        {
            collection->write([&](EntityCollection& collection)
                              {
                                  collection.modifyUserById(userId, [ & ](User& user)
                                  {
                                      user.lastSeen() = now;
                                  });
                              });
        };
    }
    return result;
}

UserRef PerformedByWithLastSeenUpdateGuard::getAndUpdate(EntityCollection& collection)
{
    lastSeenUpdate_ = nullptr;
    const auto& index = collection.users().get<EntityCollection::UserCollectionById>();
    auto it = index.find(Context::getCurrentUserId());
    if (it == index.end())
    {
        return AnonymousUser;
    }
    auto& result = *it;

    auto now = Context::getCurrentTime();

    if ((result->lastSeen() + getGlobalConfig()->user.lastSeenUpdatePrecision) < now)
    {
        collection.modifyUserById(result->id(), [&](User& user)
        {
            user.lastSeen() = now;
        });
    }
    return result;
}

PerformedByWithLastSeenUpdateGuard Repository::preparePerformedBy(const MemoryRepository& repository)
{
    return PerformedByWithLastSeenUpdateGuard(repository);
}
