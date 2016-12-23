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
            inline const IdType& id() const { return id_; }
            inline       IdType& id()       { return id_; }

        private:
            IdType id_;
        };

        struct Creatable
        {
            inline const Timestamp  created() const { return created_; }
            inline       Timestamp& created()       { return created_; }

            Creatable() : created_(0) {}

        private:
            Timestamp created_;
        };
    }
}
