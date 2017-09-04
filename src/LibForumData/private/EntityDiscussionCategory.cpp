#include "EntityDiscussionCategory.h"
#include "EntityCollection.h"

#include <map>

using namespace Forum;
using namespace Forum::Entities;
using namespace Forum::Authorization;

DiscussionCategory::ChangeNotification DiscussionCategory::changeNotifications_;

static void executeOnAllCategoryParents(DiscussionCategory& category, std::function<void(DiscussionCategory&)>&& fn)
{
    auto parent = category.parent();
    while (true)
    {
        if (parent)
        {
            fn(*parent);
            parent = parent->parent();
        }
        else { break; }
    }
}

static void executeOnCategoryAndAllParents(DiscussionCategory& category, std::function<void(DiscussionCategory&)>&& fn)
{
    fn(category);
    executeOnAllCategoryParents(category, std::move(fn));
}

bool DiscussionCategory::addChild(EntityPointer<DiscussionCategory> category)
{
    return std::get<1>(children_.insert(std::move(category)));
}

bool DiscussionCategory::removeChild(EntityPointer<DiscussionCategory> category)
{
    return children_.erase(category) > 0;
}

bool DiscussionCategory::hasAncestor(EntityPointer<DiscussionCategory> ancestor)
{
    auto currentParent = parent_;
    while (currentParent)
    {
        if (currentParent == ancestor) return true;
        currentParent = currentParent->parent_;
    }
    return false;
}

bool DiscussionCategory::insertDiscussionThread(DiscussionThreadPtr thread)
{
    assert(thread);
    changeNotifications_.onPrepareUpdateMessageCount(*this);

    if ( ! threads_.add(thread))
    {
        return false;
    }

    //don't use updateMessageCount() as insertDiscussionThread will take care of that for totals
    messageCount_ += thread->messageCount();
    thread->addCategory(pointer());

    changeNotifications_.onUpdateMessageCount(*this);

    executeOnCategoryAndAllParents(*this, [&](auto& category)
    {
        //this category and all parents will hold separate references to the new thread
        category.totalThreads_.add(thread);
    });

    return true;
}

bool DiscussionCategory::deleteDiscussionThread(DiscussionThreadPtr thread)
{
    assert(thread);
    changeNotifications_.onPrepareUpdateMessageCount(*this);

    if ( ! threads_.remove(thread))
    {
        return false;
    }
    //don't use updateMessageCount() as deleteDiscussionThreadById will take care of that for totals
    messageCount_ -= static_cast<int_fast32_t>(thread->messageCount());
    if ( ! thread->aboutToBeDeleted())
    {
        thread->removeCategory(pointer());
    }

    changeNotifications_.onUpdateMessageCount(*this);
    
    executeOnCategoryAndAllParents(*this, [&](auto& category)
    {
        category.totalThreads_.remove(thread);
    });

    return true;
}

void DiscussionCategory::deleteDiscussionThreadIfNoOtherTagsReferenceIt(DiscussionThreadPtr thread)
{
    assert(thread);
    //don't remove the thread just yet, perhaps it's also referenced by other tags
    bool referencedByOtherTags = false;
    for (auto tag : thread->tags())
    {
        assert(tag);
        if (containsTag(tag))
        {
            referencedByOtherTags = true;
            break;
        }
    }
    if ( ! referencedByOtherTags)
    {
        deleteDiscussionThread(thread);
        
        //release separate references held by this category and parents, if reference count drops to 0
        executeOnCategoryAndAllParents(*this, [&](auto& category)
        {
            category.totalThreads_.decreaseReferenceCount(thread);
        });
    }
}

bool DiscussionCategory::addTag(DiscussionTagPtr tag)
{
    assert(tag);
    if ( ! std::get<1>(tags_.insert(tag)))
    {
        return false;
    }

    for (DiscussionThreadPtr thread : tag->threads().byId())
    {
        assert(thread);
        insertDiscussionThread(thread);
    }
    return true;
}

bool DiscussionCategory::removeTag(DiscussionTagPtr tag)
{
    assert(tag);
    if (0 == tags_.erase(tag))
    {
        return false;
    }

    for (auto& thread : tag->threads().byId())
    {
        assert(thread);
        deleteDiscussionThreadIfNoOtherTagsReferenceIt(thread);
    }
    return true;
}

bool DiscussionCategory::containsTag(DiscussionTagPtr tag) const
{
    return tags_.find(tag) != tags_.end();
}

void DiscussionCategory::updateMessageCount(DiscussionThreadPtr thread, int_fast32_t delta)
{
    assert(thread);
    changeNotifications_.onPrepareUpdateMessageCount(*this);

    messageCount_ += delta;
    //make sure totals of the current category are always updated
    totalThreads_.messageCount() += delta;

    bool stop = false;
    executeOnAllCategoryParents(*this, [&](DiscussionCategory& category)
    {
        if (stop)
        {
            return;
        }
        if (category.threads().contains(thread))
        {
            //stop any updates on the total or else the messages will be counted multiple times
            //they will be/have already been taken care of by a call to updateMessageCount of this specific category
            stop = true;
            return;
        }
        category.totalThreads_.messageCount() += delta;
    });

    changeNotifications_.onUpdateMessageCount(*this);
}

void DiscussionCategory::removeTotalsFromChild(const DiscussionCategory& childCategory)
{
    executeOnCategoryAndAllParents(*this, [&childCategory](DiscussionCategory& category)
    {
        category.totalThreads_.decreaseReferenceCount(childCategory.totalThreads_);
    });
}

void DiscussionCategory::addTotalsFromChild(const DiscussionCategory& childCategory)
{
    executeOnCategoryAndAllParents(*this, [&childCategory](DiscussionCategory& category)
    {
        category.totalThreads_.add(childCategory.totalThreads_);
    });
}

PrivilegeValueType DiscussionCategory::getDiscussionCategoryPrivilege(DiscussionCategoryPrivilege privilege) const
{
    auto result = DiscussionCategoryPrivilegeStore::getDiscussionCategoryPrivilege(privilege);
    if (result) return result;

    return forumWidePrivileges_.getDiscussionCategoryPrivilege(privilege);
}
