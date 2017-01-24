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

MemoryRepository::MemoryRepository() : collection_(std::make_shared<EntityCollection>()),
    validUserNameRegex(boost::make_u32regex("^[[:alnum:]]+[ _-]*[[:alnum:]]+$")),
    validDiscussionThreadNameRegex(boost::make_u32regex("^[^[:space:]]+.*[^[:space:]]+$")),
    validDiscussionMessageContentRegex(boost::make_u32regex("^[^[:space:]]+.*[^[:space:]]+$")),
    validDiscussionTagNameRegex(boost::make_u32regex("^[^[:space:]]+.*[^[:space:]]+$"))
{
}

ReadEvents& MemoryRepository::readEvents()
{
    return readEvents_;
}

WriteEvents& MemoryRepository::writeEvents()
{
    return writeEvents_;
}

void MemoryRepository::getEntitiesCount(std::ostream& output) const
{
    auto performedBy = preparePerformedBy();

    collection_.read([&](const EntityCollection& collection)
                     {
                         EntitiesCount count;
                         count.nrOfUsers = static_cast<uint_fast32_t>(collection.usersById().size());
                         count.nrOfDiscussionThreads = static_cast<uint_fast32_t>(collection.threadsById().size());
                         count.nrOfDiscussionMessages = static_cast<uint_fast32_t>(collection.messagesById().size());
                         count.nrOfDiscussionTags = static_cast<uint_fast32_t>(collection.tagsById().size());

                         writeSingleValueSafeName(output, "count", count);

                         readEvents_.onGetEntitiesCount(createObserverContext(performedBy.get(collection)));
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
        auto mutableCollection = &repository_.collection_;
        lastSeenUpdate_ = [mutableCollection, now, userId]()
        {
            mutableCollection->write([&](EntityCollection& collectionToModify)
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

