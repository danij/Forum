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

#pragma once

#include "MemoryRepositoryCommon.h"
#include "Authorization.h"

namespace Forum
{
    namespace Repository
    {
        class MemoryRepositoryDiscussionThread final : public MemoryRepositoryBase,
                                                       public IDiscussionThreadRepository,
                                                       public IDiscussionThreadDirectWriteRepository
        {
        public:
            explicit MemoryRepositoryDiscussionThread(MemoryStoreRef store,
                                                      Authorization::DiscussionThreadAuthorizationRef authorization,
                                                      AuthorizationDirectWriteRepositoryRef authorizationDirectWriteRepository);

            StatusCode getDiscussionThreads(OutStream& output, RetrieveDiscussionThreadsBy by) const override;

            /**
             * Calling the function changes state:
             * - Increases the number of visits
             * - Stores that the current user has visited the discussion thread
             */
            StatusCode getDiscussionThreadById(Entities::IdTypeRef id, OutStream& output) override;

            StatusCode getDiscussionThreadsOfUser(Entities::IdTypeRef id, OutStream& output,
                                                  RetrieveDiscussionThreadsBy by) const override;
            StatusCode getSubscribedDiscussionThreadsOfUser(Entities::IdTypeRef id, OutStream& output,
                                                            RetrieveDiscussionThreadsBy by) const override;

            StatusCode getDiscussionThreadsWithTag(Entities::IdTypeRef id, OutStream& output,
                                                   RetrieveDiscussionThreadsBy by) const override;

            StatusCode getDiscussionThreadsOfCategory(Entities::IdTypeRef id, OutStream& output,
                                                      RetrieveDiscussionThreadsBy by) const override;

            StatusCode addNewDiscussionThread(StringView name, OutStream& output) override;
            StatusWithResource<Entities::DiscussionThreadPtr>
                       addNewDiscussionThread(Entities::EntityCollection& collection, Entities::IdTypeRef id,
                                              StringView name) override;

            StatusCode changeDiscussionThreadName(Entities::IdTypeRef id, StringView newName,
                                                  OutStream& output) override;
            StatusCode changeDiscussionThreadName(Entities::EntityCollection& collection,
                                                  Entities::IdTypeRef id, StringView newName) override;
            StatusCode changeDiscussionThreadPinDisplayOrder(Entities::IdTypeRef id, uint16_t newValue,
                                                             OutStream& output) override;
            StatusCode changeDiscussionThreadPinDisplayOrder(Entities::EntityCollection& collection,
                                                             Entities::IdTypeRef id, uint16_t newValue) override;
            StatusCode deleteDiscussionThread(Entities::IdTypeRef id, OutStream& output) override;
            StatusCode deleteDiscussionThread(Entities::EntityCollection& collection, Entities::IdTypeRef id) override;
            StatusCode mergeDiscussionThreads(Entities::IdTypeRef fromId, Entities::IdTypeRef intoId,
                                              OutStream& output) override;
            StatusCode mergeDiscussionThreads(Entities::EntityCollection& collection,
                                              Entities::IdTypeRef fromId, Entities::IdTypeRef intoId) override;
            StatusCode subscribeToDiscussionThread(Entities::IdTypeRef id, OutStream& output) override;
            StatusCode subscribeToDiscussionThread(Entities::EntityCollection& collection, Entities::IdTypeRef id) override;
            StatusCode unsubscribeFromDiscussionThread(Entities::IdTypeRef id, OutStream& output) override;
            StatusCode unsubscribeFromDiscussionThread(Entities::EntityCollection& collection, Entities::IdTypeRef id) override;

        private:
            Authorization::DiscussionThreadAuthorizationRef authorization_;
            AuthorizationDirectWriteRepositoryRef authorizationDirectWriteRepository_;
        };
    }
}
