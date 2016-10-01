#pragma once

#include <string>
#include <memory>

#include <boost/uuid/uuid.hpp>

namespace Forum
{
    namespace Entities
    {
        //Using a string for representing the id to prevent constant conversions between string <-> uuid
        typedef std::string IdType;

        struct Identifiable
        {
            inline const IdType& id() const { return id_; }
            inline       IdType& id()       { return id_; }

        private:
            IdType id_;
        };

        struct User final : public Identifiable
        {
            inline const std::string& name() const { return name_; }
            inline       std::string& name()       { return name_; }

            enum ChangeType : uint32_t
            {
                None = 0,
                Name
            };

            User() {}
            /**
             * Only used to construct the anonymous user
             */
            User(const std::string& name) : name_(name) {}

        private:
            std::string name_;
        };

        typedef std::shared_ptr<User> UserRef;
    }
}
