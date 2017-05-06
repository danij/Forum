#pragma once

#include "AuthorizationPrivileges.h"
#include "UuidString.h"

#include <ctime>

namespace Forum
{
    namespace Authorization
    {
        struct GrantedPrivilegeStore
        {
            void grantDiscussionThreadMessagePrivilege(Entities::UuidString userId, Entities::UuidString entityId, 
                                                       DiscussionThreadMessagePrivilege privilegeType, 
                                                       PrivilegeValueIntType value, time_t expiresAt)
            {
                
            }

            void grantDiscussionThreadPrivilege(Entities::UuidString userId, Entities::UuidString entityId, 
                                                DiscussionThreadPrivilege privilegeType, 
                                                PrivilegeValueIntType value, time_t expiresAt)
            {
                
            }

            void grantDiscussionTagPrivilege(Entities::UuidString userId, Entities::UuidString entityId, 
                                             DiscussionTagPrivilege privilegeType, 
                                             PrivilegeValueIntType value, time_t expiresAt)
            {
                
            }

            void grantDiscussionCategoryPrivilege(Entities::UuidString userId, Entities::UuidString entityId, 
                                                  DiscussionCategoryPrivilege privilegeType, 
                                                  PrivilegeValueIntType value, time_t expiresAt)
            {
                
            }

            void grantForumWidePrivilege(Entities::UuidString userId, Entities::UuidString entityId, 
                                         ForumWidePrivilege privilegeType, 
                                         PrivilegeValueIntType value, time_t expiresAt)
            {
                
            }
        };
    }
}