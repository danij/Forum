#pragma once

#include "EntityDiscussionThreadCollectionBase.h"
#include "EntityDiscussionMessageCollectionBase.h"
#include "EntityCommonTypes.h"

#include <boost/noncopyable.hpp>

#include <string>
#include <memory>

namespace Forum
{
    namespace Entities
    {
        struct User final : public Identifiable, public Creatable,
                            public DiscussionThreadCollectionBase, public DiscussionMessageCollectionBase
        {
            inline const std::string& name()     const { return name_; }
            inline       std::string& name()           { return name_; }
            inline const Timestamp    lastSeen() const { return lastSeen_; }
            inline       Timestamp&   lastSeen()       { return lastSeen_; }

            enum ChangeType : uint32_t
            {
                None = 0,
                Name
            };

            User() : lastSeen_(0) {}
            /**
             * Only used to construct the anonymous user
             */
            User(const std::string& name) : name_(name), lastSeen_(0) {}

        private:
            std::string name_;
            Timestamp lastSeen_;
        };

        typedef std::shared_ptr<User> UserRef;
        typedef std::  weak_ptr<User> UserWeakRef;
    }
}
