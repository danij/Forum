#pragma once

#include <string>
#include <memory>
#include <unordered_set>

#include <boost/uuid/uuid.hpp>

namespace Forum
{
    namespace Entities
    {
        struct Identifiable
        {
            inline const boost::uuids::uuid& id() const { return id_; }
            inline       boost::uuids::uuid& id()       { return id_; }

        private:
            boost::uuids::uuid id_;
        };

        struct User : public Identifiable
        {
            inline const std::string& name() const { return name_; }
            inline       std::string& name()       { return name_; }

        private:
            std::string name_;
        };

        typedef std::shared_ptr<User> UserRef;
        typedef std::shared_ptr<const User> UserConstRef;

        struct EntityCollection
        {
            std::unordered_set<UserRef> users_;
        };
    }
}
