#include "EntityDiscussionThreadMessage.h"
#include "EntityDiscussionThread.h"

using namespace Forum::Entities;
using namespace Forum::Authorization;

PrivilegeValueType DiscussionThreadMessage::getDiscussionThreadMessagePrivilege(DiscussionThreadMessagePrivilege privilege) const
{
    assert(parentThread_);
    
    return getDiscussionThreadMessagePrivilege(privilege, parentThread_->getDiscussionThreadMessagePrivilege(privilege));
}
