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

#include "MemoryRepositoryStatistics.h"
#include "Configuration.h"
#include "ContextProviders.h"
#include "EntitySerialization.h"
#include "OutputHelpers.h"

using namespace Forum;
using namespace Forum::Configuration;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;
using namespace Forum::Authorization;

MemoryRepositoryStatistics::MemoryRepositoryStatistics(MemoryStoreRef store, StatisticsAuthorizationRef authorization)
    : MemoryRepositoryBase(std::move(store)), authorization_(std::move(authorization)), 
    visitorCollection_{std::make_shared<VisitorCollection>(getGlobalConfig()->user.visitorOnlineForSeconds)}
{
    if ( ! authorization_)
    {
        throw std::runtime_error("Authorization implementation not provided");
    }
    Context::setVisitorCollection(visitorCollection_);
}

StatusCode MemoryRepositoryStatistics::getEntitiesCount(OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);

                          if ( ! (status = authorization_->getEntitiesCount(currentUser)))
                          {
                              return;
                          }

                          status.disable();

                          EntitiesCount count;
                          count.nrOfUsers = collection.users().count();
                          count.nrOfDiscussionThreads = collection.threads().count();
                          count.nrOfDiscussionMessages = collection.threadMessages().count();
                          count.nrOfDiscussionTags = collection.tags().count();
                          count.nrOfDiscussionCategories = collection.categories().count();
                          count.nrOfVisitors = visitorCollection_->currentNumberOfVisitors();

                          writeSingleValueSafeName(output, "count", count);

                          readEvents().onGetEntitiesCount(createObserverContext(currentUser));
                      });
    return status;
}
