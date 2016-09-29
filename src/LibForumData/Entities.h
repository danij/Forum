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

        struct User : public Identifiable
        {
            inline const std::string& name() const { return name_; }
            inline       std::string& name()       { return name_; }

        private:
            std::string name_;
        };

        typedef std::shared_ptr<User> UserRef;
    }
}
