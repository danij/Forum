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
    StatusWriter status(output, StatusCode::OK);

    PerformedByWithLastSeenUpdateGuard performedBy;
    AuthorizationStatus authorizationStatus;

    collection().read([&](const Entities::EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, store());
                      
                          authorizationStatus = authorization_->getEntitiesCount(currentUser);
                          if (AuthorizationStatusCode::OK != authorizationStatus.code)
                          {
                              status = authorizationStatus.code;
                              return;
                          }
                      
                          status.disable();

                          EntitiesCount count;
                          count.nrOfUsers = static_cast<uint_fast32_t>(collection.usersById().size());
                          count.nrOfDiscussionThreads = static_cast<uint_fast32_t>(collection.threadsById().size());
                          count.nrOfDiscussionMessages = static_cast<uint_fast32_t>(collection.messagesById().size());
                          count.nrOfDiscussionTags = static_cast<uint_fast32_t>(collection.tagsById().size());
                          count.nrOfDiscussionCategories = static_cast<uint_fast32_t>(collection.categoriesById().size());
                 
                          writeSingleValueSafeName(output, "count", count);

                          if ( ! Context::skipObservers())
                              readEvents().onGetEntitiesCount(createObserverContext(currentUser));
                      });
    return status;
}
