#pragma once

#include "UuidString.h"

#include <cstdint>
#include <memory>

#include "IpAddress.h"
#include "TypeHelpers.h"

namespace Forum
{
    namespace Entities
    {
        /**
         * Using a string for representing the id to prevent constant conversions between string <-> uuid
         */
        typedef UuidString IdType;

        /**
         * Representing a timestamp as the number of seconds since the UNIX EPOCH
         */
        typedef int_fast64_t Timestamp;
        
        /**
         * Specify that a collection is to order entities by id using a hash table
         */
        struct HashIndexForId {};

        /**
         * Specify that a colelction is to order entities by id using a tree
         */
        struct OrderedIndexForId {};

        struct Identifiable
        {
            DECLARE_ABSTRACT_MANDATORY(Identifiable)

            const IdType& id() const { return id_; }
                  IdType& id()       { return id_; }

        private:
            IdType id_;
        };

        struct IndicateDeletionInProgress
        {
            DECLARE_ABSTRACT_MANDATORY(IndicateDeletionInProgress)

            bool& aboutToBeDeleted() { return aboutToBeDeleted_; }

        private:
            bool aboutToBeDeleted_ = false;
        };

        struct VisitDetails
        {
            Helpers::IpAddress ip;
        };

        struct CreatedMixin
        {
            DECLARE_ABSTRACT_MANDATORY(CreatedMixin)

                  Timestamp     created()         const { return created_; }
                  Timestamp&    created()               { return created_; }

            const VisitDetails& creationDetails() const { return creationDetails_; }
                  VisitDetails& creationDetails()       { return creationDetails_; }

        private:
            Timestamp created_ = 0;
            VisitDetails creationDetails_;
        };

        template<typename ByType>
        struct LastUpdatedMixin
        {
            DECLARE_ABSTRACT_MANDATORY(LastUpdatedMixin)

                  Timestamp     lastUpdated()        const { return lastUpdated_; }
                  Timestamp&    lastUpdated()              { return lastUpdated_; }

            const VisitDetails& lastUpdatedDetails() const { return lastUpdatedDetails_; }
                  VisitDetails& lastUpdatedDetails()       { return lastUpdatedDetails_; }
                        
                  std::string   lastUpdatedReason()  const { return lastUpdatedReason_; }
                  std::string&  lastUpdatedReason()        { return lastUpdatedReason_; }

            std::weak_ptr<ByType>& lastUpdatedBy() { return lastUpdatedBy_; }

            template<typename Action>
            void executeActionWithLastUpdatedByIfAvailable(Action&& action) const
            {
                if (auto updatedByShared = lastUpdatedBy_.lock())
                {
                    action(const_cast<const ByType&>(*updatedByShared));
                }
            }

            typedef std::weak_ptr<ByType> ByTypeRef;

        private:
            Timestamp lastUpdated_ = 0;
            VisitDetails lastUpdatedDetails_;
            ByTypeRef lastUpdatedBy_;
            std::string lastUpdatedReason_;
        };
    }
}
