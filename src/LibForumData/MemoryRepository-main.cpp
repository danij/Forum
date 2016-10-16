#include "Configuration.h"
#include "ContextProviders.h"
#include "MemoryRepository.h"

using namespace Forum;
using namespace Forum::Configuration;
using namespace Forum::Entities;
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
    auto it = index.find(Forum::Context::getCurrentUserId());
    if (it == index.end())
    {
        return AnonymousUser;
    }
    auto& result = **it;

    auto now = Forum::Context::getCurrentTime();

    if ((result.lastSeen() + getGlobalConfig()->user.lastSeenUpdatePrecision) < now)
    {
        auto& userId = result.id();
        auto collection = &(repository_.collection_);
        lastSeenUpdate_ = [collection, now, userId]()
        {
            collection->write([&](EntityCollection& collection)
                              {
                                  collection.modifyUser(userId, [ & ](User& user)
                                  {
                                      user.lastSeen() = now;
                                  });
                              });
        };
    }
    return result;
}

PerformedByType PerformedByWithLastSeenUpdateGuard::getAndUpdate(EntityCollection& collection)
{
    lastSeenUpdate_ = nullptr;
    const auto& index = collection.usersById();
    auto it = index.find(Forum::Context::getCurrentUserId());
    if (it == index.end())
    {
        return AnonymousUser;
    }
    auto& result = **it;

    auto now = Forum::Context::getCurrentTime();

    if ((result.lastSeen() + getGlobalConfig()->user.lastSeenUpdatePrecision) < now)
    {
        collection.modifyUser(result.id(), [&](User& user)
        {
            user.lastSeen() = now;
        });
    }
    return result;
}

PerformedByWithLastSeenUpdateGuard Forum::Repository::preparePerformedBy(const MemoryRepository& repository)
{
    return PerformedByWithLastSeenUpdateGuard(repository);
}
