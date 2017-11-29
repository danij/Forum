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

#include "ContextProviders.h"
#include "EntityCollection.h"
#include "Observers.h"
#include "Repository.h"
#include "ResourceGuard.h"

#include <boost/core/noncopyable.hpp>
#include <boost/optional.hpp>

namespace Forum
{
    namespace Repository
    {
        struct MemoryStore final : private boost::noncopyable
        {
            explicit MemoryStore(Entities::EntityCollectionRef collection) : collection(std::move(collection))
            {}

            Helpers::ResourceGuard<Entities::EntityCollection> collection;
            ReadEvents readEvents;
            WriteEvents writeEvents;
        };
        typedef std::shared_ptr<MemoryStore> MemoryStoreRef;

        /**
        * Retrieves the user that is performing the current action and also performs an update on the last seen if needed
        * The update is performed on the spot if a write lock is held or
        * delayed until the lock is destroyed in the case of a read lock, to avoid deadlocks
        * Do not keep references to it outside of MemoryRepository methods
        */
        struct PerformedByWithLastSeenUpdateGuard final : private boost::noncopyable
        {
            ~PerformedByWithLastSeenUpdateGuard();

            /**
            * Get the current user that performs the action and optionally schedule the update of last seen
            */
            PerformedByType get(const Entities::EntityCollection& collection, const MemoryStore& store);

            /**
            * Get the current user that performs the action and optionally also perform the update of last seen
            * This method takes advantage if a write lock on the collection is already secured
            */
            Entities::UserPtr getAndUpdate(Entities::EntityCollection& collection);

        private:
            std::function<void()> lastSeenUpdate_;
        };

        Entities::UserPtr getCurrentUser(Entities::EntityCollection& collection);

        class MemoryRepositoryBase : public IObservableRepository, private boost::noncopyable
        {
        public:
            explicit MemoryRepositoryBase(MemoryStoreRef store) : store_(std::move(store))
            {}
            virtual ~MemoryRepositoryBase() = default;

            ReadEvents& readEvents()   override { return store_->readEvents; }
            WriteEvents& writeEvents() override { return store_->writeEvents; }

            ReadEvents& readEvents()      const { return store_->readEvents; }
            WriteEvents& writeEvents()    const { return store_->writeEvents; }

        protected:
            auto& collection() const { return store_->collection; }

            enum EmptyStringValidation
            {
                ALLOW_EMPTY_STRING,
                INVALID_PARAMETERS_FOR_EMPTY_STRING
            };

            static StatusCode validateString(StringView string,
                                             EmptyStringValidation emptyValidation,
                                             boost::optional<int_fast32_t> minimumLength,
                                             boost::optional<int_fast32_t> maximumLength);

            template<typename Fn>
            static StatusCode validateString(StringView string,
                                             EmptyStringValidation emptyValidation,
                                             boost::optional<int_fast32_t> minimumLength,
                                             boost::optional<int_fast32_t> maximumLength,
                                             Fn&& extraValidation)
            {
                auto initialResult = validateString(string, emptyValidation, minimumLength, maximumLength);
                if (StatusCode::OK != initialResult)
                {
                    return initialResult;
                }
                return extraValidation(string) ? StatusCode::OK : StatusCode::INVALID_PARAMETERS;
            }

            static bool doesNotContainLeadingOrTrailingWhitespace(StringView& input);

            static StatusCode validateImage(StringView content, uint_fast32_t maxBinarySize, uint_fast32_t maxWidth,
                                            uint_fast32_t maxHeight);

            MemoryStoreRef store_;
        };

        inline ObserverContext_ createObserverContext(PerformedByType performedBy)
        {
            return ObserverContext_(performedBy, Context::getCurrentTime(), Context::getDisplayContext(),
                                    Context::getCurrentUserIpAddress());
        }

        template<typename Entity, typename ByType>
        void updateLastUpdated(Entity& entity, ByType by)
        {
            entity.lastUpdated() = Context::getCurrentTime();
            entity.lastUpdatedDetails().ip = Context::getCurrentUserIpAddress();
            entity.lastUpdatedBy() = by;
        }

        inline void updateThreadLastUpdated(Entities::DiscussionThread& thread, Entities::UserPtr currentUser)
        {
            thread.updateLastUpdated(thread.latestVisibleChange() = Context::getCurrentTime());
            thread.updateLastUpdatedDetails({ Context::getCurrentUserIpAddress() });
            thread.updateLastUpdatedBy(currentUser);
        }
    }
}
