#include "EntityDiscussionTag.h"
#include "EntityCollection.h"

using namespace Forum;
using namespace Forum::Entities;
using namespace Forum::Authorization;

DiscussionTag::ChangeNotification DiscussionTag::changeNotifications_;

PrivilegeValueType DiscussionTag::getDiscussionThreadMessagePrivilege(DiscussionThreadMessagePrivilege privilege) const
{
    auto result = DiscussionThreadMessagePrivilegeStore::getDiscussionThreadMessagePrivilege(privilege);
    if (result) return result;

    return forumWidePrivileges_.getDiscussionThreadMessagePrivilege(privilege);
}

PrivilegeValueType DiscussionTag::getDiscussionThreadPrivilege(DiscussionThreadPrivilege privilege) const
{
    auto result = DiscussionThreadPrivilegeStore::getDiscussionThreadPrivilege(privilege);
    if (result) return result;

    return forumWidePrivileges_.getDiscussionThreadPrivilege(privilege);
}

PrivilegeDefaultDurationType DiscussionTag::getDiscussionThreadMessageDefaultPrivilegeDuration(DiscussionThreadMessageDefaultPrivilegeDuration privilege) const
{
    auto result = DiscussionThreadPrivilegeStore::getDiscussionThreadMessageDefaultPrivilegeDuration(privilege);
    if (result) return result;

    return forumWidePrivileges_.getDiscussionThreadMessageDefaultPrivilegeDuration(privilege);
}

PrivilegeValueType DiscussionTag::getDiscussionTagPrivilege(DiscussionTagPrivilege privilege) const
{
    auto result = DiscussionTagPrivilegeStore::getDiscussionTagPrivilege(privilege);
    if (result) return result;

    return forumWidePrivileges_.getDiscussionTagPrivilege(privilege);
}

bool DiscussionTag::insertDiscussionThread(DiscussionThreadPtr thread)
{
    assert(thread);
    changeNotifications_.onPrepareUpdateThreadCount(*this);

    if ( ! threads_.add(thread))
    {
        return false;
    }
    messageCount_ += thread->messageCount();

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
    assert(thread);
    changeNotifications_.onPrepareUpdateThreadCount(*this);

    if ( ! threads_.remove(thread))
    {
        return false;
    }
    messageCount_ -= thread->messageCount();

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
    assert(category);
    return std::get<1>(categories_.insert(std::move(category)));
}

bool DiscussionTag::removeCategory(EntityPointer<DiscussionCategory> category)
{
    assert(category);
    return categories_.erase(category) > 0;
}
