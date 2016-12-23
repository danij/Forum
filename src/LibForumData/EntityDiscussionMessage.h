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
            const std::string&       content()      const { return content_; }
                  std::string&       content()            { return content_; }
            const User&              createdBy()    const { return createdBy_; }
                  User&              createdBy()          { return createdBy_; }
            const DiscussionThread&  parentThread() const { return parentThread_; }
                  DiscussionThread&  parentThread()       { return parentThread_; }

            DiscussionMessage(User& createdBy, DiscussionThread& parentThread)
                : createdBy_(createdBy), parentThread_(parentThread) {};

        private:
            std::string content_;
            User& createdBy_;
            DiscussionThread& parentThread_;
        };

        typedef std::shared_ptr<DiscussionMessage> DiscussionMessageRef;
    }
}
