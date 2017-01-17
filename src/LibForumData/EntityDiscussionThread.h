#pragma once

#include "EntityCommonTypes.h"
#include "EntityDiscussionThreadMessageCollectionBase.h"

#include <atomic>
#include <string>
#include <memory>

namespace Forum
{
    namespace Entities
    {
        struct User;

        struct DiscussionThread final : public Identifiable, public CreatedMixin, public LastUpdatedMixin, 
                                        public DiscussionThreadMessageCollectionBase
        {
            const std::string& name()        const { return name_; }
                  std::string& name()              { return name_; }
            const User&        createdBy()   const { return createdBy_; }
                  User&        createdBy()         { return createdBy_; }
            /**
             * Thread-safe reference to the number of times the thread was visited.
             * Can be updated even for const values as it is not refenced in any index.
             * @return An atomic integer of at least 64-bits
             */
            std::atomic_int_fast64_t& visited()     const { return visited_; }

            enum ChangeType : uint32_t
            {
                None = 0,
                Name
            };

            explicit DiscussionThread(User& createdBy) : createdBy_(createdBy), visited_(0) {};

        private:
            std::string name_;
            User& createdBy_;
            mutable std::atomic_int_fast64_t visited_;
        };

        typedef std::shared_ptr<DiscussionThread> DiscussionThreadRef;
    }
}
