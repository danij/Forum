#pragma once

#include <cstdint>
#include <string>

namespace Forum
{
    namespace Entities
    {
        /**
         * Using a string for representing the id to prevent constant conversions between string <-> uuid
         */
        typedef std::string IdType;

        /**
         * Representing a timestamp as the number of seconds since the UNIX EPOCH
         */
        typedef uint64_t Timestamp;
    }
}
