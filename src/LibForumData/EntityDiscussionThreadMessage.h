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

        struct DiscussionThreadMessage final : public Identifiable, public CreatedMixin, public LastUpdatedMixin, 
                                               private boost::noncopyable
        {
            struct CreationDetails
            {
                IpType ip;
                UserAgentType userAgent;
            };

            struct LastUpdatedDetails : public CreationDetails
            {
                std::weak_ptr<User> by;
            };

            const std::string&        content()            const { return content_; }
                  std::string&        content()                  { return content_; }
            const User&               createdBy()          const { return createdBy_; }
                  User&               createdBy()                { return createdBy_; }
            const DiscussionThread&   parentThread()       const { return parentThread_; }
                  DiscussionThread&   parentThread()             { return parentThread_; }
            const CreationDetails&    creationDetails()    const { return creationDetails_; }
                  CreationDetails&    creationDetails()          { return creationDetails_; }
            const LastUpdatedDetails& lastUpdatedDetails() const { return lastUpdatedDetails_; }
                  LastUpdatedDetails& lastUpdatedDetails()       { return lastUpdatedDetails_; }

            enum ChangeType : uint32_t
            {
                None = 0,
                Content
            };

            DiscussionThreadMessage(User& createdBy, DiscussionThread& parentThread)
                : createdBy_(createdBy), parentThread_(parentThread) {}

        private:
            std::string content_;
            User& createdBy_;
            DiscussionThread& parentThread_;
            LastUpdatedDetails creationDetails_;
            LastUpdatedDetails lastUpdatedDetails_;
        };

        typedef std::shared_ptr<DiscussionThreadMessage> DiscussionMessageRef;
    }
}
