#pragma once

#include <string>
#include <memory>

#include <boost/uuid/uuid.hpp>

#include "EntityCommonTypes.h"

namespace Forum
{
    namespace Entities
    {
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

        struct User final : public Identifiable, public Creatable
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

        struct DiscussionThread final : public Identifiable, public Creatable
        {
            inline const std::string& name()        const { return name_; }
            inline       std::string& name()              { return name_; }
            inline const Timestamp    lastUpdated() const { return lastUpdated_; }
            inline       Timestamp&   lastUpdated()       { return lastUpdated_; }

        private:
            std::string name_;
            Timestamp lastUpdated_;
        };

        typedef std::shared_ptr<User> UserRef;
        typedef std::shared_ptr<DiscussionThread> DiscussionThreadRef;
    }
}
