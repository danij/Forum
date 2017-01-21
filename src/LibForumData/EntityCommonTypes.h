#pragma once

#include "UuidString.h"

#include <cstdint>

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
        typedef std::string IpType;

        /**
         * Representing a browser user agent
         */
        typedef std::string UserAgentType;

        struct Identifiable
        {
            const IdType& id() const { return id_; }
                  IdType& id()       { return id_; }

        private:
            IdType id_;
        };

        struct CreatedMixin
        {
            const Timestamp& created() const { return created_; }
                  Timestamp& created()       { return created_; }

            CreatedMixin() : created_(0) {}

        private:
            Timestamp created_;
        };

        struct LastUpdatedMixin
        {
            const Timestamp& lastUpdated() const { return lastUpdated_; }
                  Timestamp& lastUpdated()       { return lastUpdated_; }

            LastUpdatedMixin() : lastUpdated_(0) {}

        private:
            Timestamp lastUpdated_;
        };
    }
}
