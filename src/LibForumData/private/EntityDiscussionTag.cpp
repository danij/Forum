#include "EntityDiscussionTag.h"
#include "EntityCollection.h"

using namespace Forum;
using namespace Forum::Entities;
using namespace Forum::Authorization;

PrivilegeValueType DiscussionTag::getDiscussionThreadMessagePrivilege(DiscussionThreadMessagePrivilege privilege) const
{
    return MinimumPrivilegeValue(
        DiscussionThreadMessagePrivilegeStore::getDiscussionThreadMessagePrivilege(privilege),
        forumWidePrivileges_.getDiscussionThreadMessagePrivilege(privilege));
}

PrivilegeValueType DiscussionTag::getDiscussionThreadPrivilege(DiscussionThreadPrivilege privilege) const
{
    return MinimumPrivilegeValue(
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
    return MinimumPrivilegeValue(
        DiscussionTagPrivilegeStore::getDiscussionTagPrivilege(privilege),
        forumWidePrivileges_.getDiscussionTagPrivilege(privilege));
}

bool DiscussionTag::insertDiscussionThread(const DiscussionThreadRef& thread)
{
    if ( ! DiscussionThreadCollectionBase::insertDiscussionThread(thread))
    {
        return false;
    }
    messageCount() += thread->messages().size();

    for (auto& categoryWeak : categoriesWeak())
    {
        if (auto category = categoryWeak.lock())
        {
            category->insertDiscussionThread(thread);
        }
    }
    notifyChangeFn_(*this);
    return true;
}

DiscussionThreadRef DiscussionTag::deleteDiscussionThread(DiscussionThreadCollection::iterator iterator)
{
    DiscussionThreadRef result;
    if ( ! ((result = DiscussionThreadCollectionBase::deleteDiscussionThread(iterator))))
    {
        return result;
    }
    messageCount() -= result->messages().size();
    for (auto& categoryWeak : categoriesWeak())
    {
        if (auto category = categoryWeak.lock())
        {
            //called from detaching a tag from a thread
            category->deleteDiscussionThreadIfNoOtherTagsReferenceIt(result);
        }
    }
    notifyChangeFn_(*this);
    return result;
}
