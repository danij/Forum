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
                          auto& currentUser = performedBy.get(collection, *store_);

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
