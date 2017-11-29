/*
Fast Forum Backend
Copyright (C) 2016-2017 Daniel Jurcau

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

#include "MetricsRepository.h"
#include "MemoryRepositoryCommon.h"

#include "OutputHelpers.h"
#include "Version.h"

using namespace Forum::Helpers;
using namespace Forum::Repository;
using namespace Forum::Authorization;

MetricsRepository::MetricsRepository(MemoryStoreRef store, MetricsAuthorizationRef authorization)
    : MemoryRepositoryBase(std::move(store)), authorization_(std::move(authorization))
{
    if ( ! authorization_)
    {
        throw std::runtime_error("Authorization implementation not provided");
    }
}

StatusCode MetricsRepository::getVersion(OutStream& output)
{
    StatusWriter status(output);
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const Entities::EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, *store_);

                          if ( ! (status = authorization_->getVersion(currentUser)))
                          {
                              return;
                          }

                          status.disable();
                          writeSingleValueSafeName(output, "version", VERSION);
                      });
    return status;
}
