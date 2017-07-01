#include "MemoryRepositoryStatistics.h"

#include "EntitySerialization.h"
#include "OutputHelpers.h"

using namespace Forum;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;
using namespace Forum::Authorization;

MemoryRepositoryStatistics::MemoryRepositoryStatistics(MemoryStoreRef store, StatisticsAuthorizationRef authorization)
    : MemoryRepositoryBase(std::move(store)), authorization_(std::move(authorization))
{
    if ( ! authorization_)
    {
        throw std::runtime_error("Authorization implementation not provided");
    }
}

StatusCode MemoryRepositoryStatistics::getEntitiesCount(OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const Entities::EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, store());

                          if ( ! (status = authorization_->getEntitiesCount(currentUser)))
                          {
                              return;
                          }

                          status.disable();

                          EntitiesCount count;
                          count.nrOfUsers = static_cast<uint_fast32_t>(collection.users().count());
                          count.nrOfDiscussionThreads = static_cast<uint_fast32_t>(collection.threads().count());
                          count.nrOfDiscussionMessages = static_cast<uint_fast32_t>(collection.threadMessages().count());
                          count.nrOfDiscussionTags = static_cast<uint_fast32_t>(collection.tags().count());
                          count.nrOfDiscussionCategories = static_cast<uint_fast32_t>(collection.categories().count());

                          writeSingleValueSafeName(output, "count", count);

                          readEvents().onGetEntitiesCount(createObserverContext(currentUser));
                      });
    return status;
}
