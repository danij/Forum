#pragma once

#include <string>
#include <memory>

#include <boost/noncopyable.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ranked_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

#include "EntityCommonTypes.h"
#include "StringHelpers.h"

namespace Forum
{
    namespace Entities
    {
        struct User;

        struct DiscussionThread final : public Identifiable, public Creatable, private boost::noncopyable
        {
            inline const std::string& name()        const { return name_; }
            inline       std::string& name()              { return name_; }
            inline const Timestamp    lastUpdated() const { return lastUpdated_; }
            inline       Timestamp&   lastUpdated()       { return lastUpdated_; }
            inline const User&        createdBy()   const { return createdBy_; }
            inline       User&        createdBy()         { return createdBy_; }

            enum ChangeType : uint32_t
            {
                None = 0,
                Name
            };

            inline DiscussionThread(User& createdBy) : createdBy_(createdBy), lastUpdated_(0) {};

        private:
            std::string name_;
            User& createdBy_;
            Timestamp lastUpdated_;
        };

        typedef std::shared_ptr<DiscussionThread> DiscussionThreadRef;
    }
}
