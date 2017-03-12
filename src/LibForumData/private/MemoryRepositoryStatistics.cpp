#include "MemoryRepositoryStatistics.h"

#include "EntitySerialization.h"
#include "OutputHelpers.h"

using namespace Forum;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;

MemoryRepositoryStatistics::MemoryRepositoryStatistics(MemoryStoreRef store) : MemoryRepositoryBase(std::move(store))
{
}

StatusCode MemoryRepositoryStatistics::getEntitiesCount(std::ostream& output) const
{
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          EntitiesCount count;
                          count.nrOfUsers = static_cast<uint_fast32_t>(collection.usersById().size());
                          count.nrOfDiscussionThreads = static_cast<uint_fast32_t>(collection.threadsById().size());
                          count.nrOfDiscussionMessages = static_cast<uint_fast32_t>(collection.messagesById().size());
                          count.nrOfDiscussionTags = static_cast<uint_fast32_t>(collection.tagsById().size());
                          count.nrOfDiscussionCategories = static_cast<uint_fast32_t>(collection.categoriesById().size());
                 
                          writeSingleValueSafeName(output, "count", count);

                          if ( ! Context::skipObservers())
                              readEvents().onGetEntitiesCount(createObserverContext(performedBy.get(collection, store())));
                      });
    return StatusCode::OK;
}
