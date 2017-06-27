#include "EntityDiscussionTag.h"
#include "EntityCollection.h"

using namespace Forum;
using namespace Forum::Entities;
using namespace Forum::Authorization;

DiscussionTag::ChangeNotification DiscussionTag::changeNotifications_;

PrivilegeValueType DiscussionTag::getDiscussionThreadMessagePrivilege(DiscussionThreadMessagePrivilege privilege) const
{
    return minimumPrivilegeValue(
        DiscussionThreadMessagePrivilegeStore::getDiscussionThreadMessagePrivilege(privilege),
        forumWidePrivileges_.getDiscussionThreadMessagePrivilege(privilege));
}

PrivilegeValueType DiscussionTag::getDiscussionThreadPrivilege(DiscussionThreadPrivilege privilege) const
{
    return minimumPrivilegeValue(
        DiscussionThreadPrivilegeStore::getDiscussionThreadPrivilege(privilege),
        forumWidePrivileges_.getDiscussionThreadPrivilege(privilege));
}

PrivilegeDefaultDurationType DiscussionTag::getDiscussionThreadMessageDefaultPrivilegeDuration(DiscussionThreadMessageDefaultPrivilegeDuration privilege) const
{
    auto result = DiscussionThreadPrivilegeStore::getDiscussionThreadMessageDefaultPrivilegeDuration(privilege);
    if (result) return result;

    return forumWidePrivileges_.getDiscussionThreadMessageDefaultPrivilegeDuration(privilege);
}

PrivilegeValueType DiscussionTag::getDiscussionTagPrivilege(DiscussionTagPrivilege privilege) const
{
    return minimumPrivilegeValue(
        DiscussionTagPrivilegeStore::getDiscussionTagPrivilege(privilege),
        forumWidePrivileges_.getDiscussionTagPrivilege(privilege));
}

bool DiscussionTag::insertDiscussionThread(DiscussionThreadPtr thread)
{
    if ( ! threads_.add(thread))
    {
        return;
    }
    messageCount() += thread->messageCount();

    for (auto category : categories_)
    {
        assert(category);
        category->insertDiscussionThread(thread);
    }
    changeNotifications_.onUpdateThreadCount(*this);
    return true;
}

bool DiscussionTag::deleteDiscussionThread(DiscussionThreadPtr thread)
{
    if ( ! threads_.remove(thread))
    {
        return false;
    }
    messageCount() -= thread->messageCount();

    for (auto category : categories_)
    {
        assert(category);
        //called from detaching a tag from a thread
        category->deleteDiscussionThreadIfNoOtherTagsReferenceIt(thread);
    }
    changeNotifications_.onUpdateThreadCount(*this);
    return true;
}

bool DiscussionTag::addCategory(EntityPointer<DiscussionCategory> category)
{
    return std::get<1>(categories_.insert(std::move(category)));
}

bool DiscussionTag::removeCategory(EntityPointer<DiscussionCategory> category)
{
    return categories_.erase(category) > 0;
}
