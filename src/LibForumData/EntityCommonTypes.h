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

        struct Identifiable
        {
            const IdType& id() const { return id_; }
                  IdType& id()       { return id_; }

        private:
            IdType id_;
        };

        struct Creatable
        {
            const Timestamp& created() const { return created_; }
                  Timestamp& created()       { return created_; }

            Creatable() : created_(0) {}

        private:
            Timestamp created_;
        };
    }
}
