#include "MemoryRepositoryCommon.h"

#include "Configuration.h"
#include "ContextProviders.h"
#include "EntitySerialization.h"
#include "OutputHelpers.h"

using namespace Forum;
using namespace Forum::Configuration;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;

/**
 * Retrieves the user that is performing the current action and also performs an update on the last seen if needed
 * The update is performed on the spot if a write lock is held or
 * delayed until the lock is destroyed in the case of a read lock, to avoid deadlocks
 * Do not keep references to it outside of MemoryRepository methods
 */
PerformedByWithLastSeenUpdateGuard::~PerformedByWithLastSeenUpdateGuard()
{
    if (lastSeenUpdate_) lastSeenUpdate_();
}

PerformedByType PerformedByWithLastSeenUpdateGuard::get(const EntityCollection& collection, const MemoryStore& store)
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
        auto& mutableCollection = const_cast<MemoryStore&>(store).collection;
        lastSeenUpdate_ = [&mutableCollection, now, &userId]()
        {
            mutableCollection.write([&](EntityCollection& collectionToModify)
                                    {
                                        collectionToModify.modifyUserById(userId, [ & ](User& user)
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
