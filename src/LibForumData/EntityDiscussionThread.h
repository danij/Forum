#pragma once

#include "EntityCommonTypes.h"
#include "EntityDiscussionMessageCollectionBase.h"

#include <atomic>
#include <string>
#include <memory>

namespace Forum
{
    namespace Entities
    {
        struct User;

        struct DiscussionThread final : public Identifiable, public Creatable, public DiscussionMessageCollectionBase
        {
            inline const std::string&        name()        const { return name_; }
            inline       std::string&        name()              { return name_; }
            inline const Timestamp           lastUpdated() const { return lastUpdated_; }
            inline       Timestamp&          lastUpdated()       { return lastUpdated_; }
            inline const User&               createdBy()   const { return createdBy_; }
            inline       User&               createdBy()         { return createdBy_; }
            /**
             * Thread-safe reference to the number of times the thread was visited.
             * Can be updated even for const values as it is not refenced in any index.
             * @return An atomic integer of at least 64-bits
             */
            inline std::atomic_int_fast64_t& visited()     const { return visited_; }

            enum ChangeType : uint32_t
            {
                None = 0,
                Name
            };

            inline DiscussionThread(User& createdBy) : createdBy_(createdBy), lastUpdated_(0), visited_(0) {};

        private:
            std::string name_;
            User& createdBy_;
            Timestamp lastUpdated_;
            mutable std::atomic_int_fast64_t visited_;
        };

        typedef std::shared_ptr<DiscussionThread> DiscussionThreadRef;
    }
}
