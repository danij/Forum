#include "EntityDiscussionThreadMessage.h"
#include "EntityDiscussionThread.h"

using namespace Forum::Entities;
using namespace Forum::Authorization;

PrivilegeValueType DiscussionThreadMessage::getDiscussionThreadMessagePrivilege(
        DiscussionThreadMessagePrivilege privilege) const
{
    auto result = DiscussionThreadMessagePrivilegeStore::getDiscussionThreadMessagePrivilege(privilege);
    if (result) return result;
    
    assert(parentThread_);
    return parentThread_->getDiscussionThreadMessagePrivilege(privilege);
}

PrivilegeValueType DiscussionThreadMessage::getDiscussionThreadMessagePrivilege(
        DiscussionThreadMessagePrivilege privilege, PrivilegeValueType discussionThreadLevelValue) const
{
    auto result = DiscussionThreadMessagePrivilegeStore::getDiscussionThreadMessagePrivilege(privilege);

    return result ? result : discussionThreadLevelValue;
}
