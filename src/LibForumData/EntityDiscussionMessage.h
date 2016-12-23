#pragma once

#include "EntityCommonTypes.h"

#include <boost/noncopyable.hpp>

#include <memory>
#include <string>

namespace Forum
{
    namespace Entities
    {
        struct User;
        struct DiscussionThread;

        struct DiscussionMessage final : public Identifiable, public Creatable, private boost::noncopyable
        {
            inline const std::string&       content()      const { return content_; }
            inline       std::string&       content()            { return content_; }
            inline const User&              createdBy()    const { return createdBy_; }
            inline       User&              createdBy()          { return createdBy_; }
            inline const DiscussionThread&  parentThread() const { return parentThread_; }
            inline       DiscussionThread&  parentThread()       { return parentThread_; }

            inline DiscussionMessage(User& createdBy, DiscussionThread& parentThread)
                : createdBy_(createdBy), parentThread_(parentThread) {};

        private:
            std::string content_;
            User& createdBy_;
            DiscussionThread& parentThread_;
        };

        typedef std::shared_ptr<DiscussionMessage> DiscussionMessageRef;
    }
}
