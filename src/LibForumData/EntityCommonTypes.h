#pragma once

#include "UuidString.h"

#include <cstdint>
#include <memory>

#include <boost/flyweight.hpp>

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
         * Representing an IPv4 or IPv6 address
         */
        typedef boost::flyweight<std::string> IpType;

        /**
         * Representing a browser user agent
         */
        typedef boost::flyweight<std::string> UserAgentType;

        struct Identifiable
        {
            const IdType& id() const { return id_; }
                  IdType& id()       { return id_; }

        private:
            IdType id_;
        };

        struct IndicateDeletionInProgress
        {
            bool& aboutToBeDeleted() { return aboutToBeDeleted_; }

        private:
            bool aboutToBeDeleted_ = false;
        };

        struct VisitDetails
        {
            IpType ip;
            UserAgentType userAgent;
        };

        struct CreatedMixin
        {
                     Timestamp  created()         const { return created_; }
                     Timestamp& created()               { return created_; }

            const VisitDetails& creationDetails() const { return creationDetails_; }
                  VisitDetails& creationDetails()       { return creationDetails_; }

            CreatedMixin() : created_(0) {}

        private:
            Timestamp created_;
            VisitDetails creationDetails_;
        };

        template<typename ByType>
        struct LastUpdatedMixin
        {
                     Timestamp  lastUpdated()        const { return lastUpdated_; }
                     Timestamp& lastUpdated()              { return lastUpdated_; }

            const VisitDetails& lastUpdatedDetails() const { return lastUpdatedDetails_; }
                  VisitDetails& lastUpdatedDetails()       { return lastUpdatedDetails_; }
                        
                   std::string  lastUpdatedReason()  const { return lastUpdatedReason_; }
                   std::string& lastUpdatedReason()        { return lastUpdatedReason_; }

            std::weak_ptr<ByType>& lastUpdatedBy() { return lastUpdatedBy_; }

            template<typename Action>
            void executeActionWithLastUpdatedByIfAvailable(Action&& action) const
            {
                if (auto updatedByShared = lastUpdatedBy_.lock())
                {
                    action(const_cast<const ByType&>(*updatedByShared));
                }
            }

            LastUpdatedMixin() : lastUpdated_(0) {}

            typedef std::weak_ptr<ByType> ByTypeRef;

        private:
            Timestamp lastUpdated_;
            VisitDetails lastUpdatedDetails_;
            ByTypeRef lastUpdatedBy_;
            std::string lastUpdatedReason_;
        };
    }
}
