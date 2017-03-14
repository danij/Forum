#include "EntityDiscussionCategory.h"
#include "EntityCollection.h"

using namespace Forum;
using namespace Forum::Entities;

static void executeOnAllCategoryParents(DiscussionCategory& category, std::function<void(DiscussionCategory&)>&& fn)
{
    DiscussionCategoryWeakRef parentWeak = category.parentWeak();
    while (true)
    {
        if (auto parent = parentWeak.lock())
        {
            fn(*parent);
            parentWeak = parent->parentWeak();
        }
        else { break; }
    }
}

static void executeOnCategoryAndAllParents(DiscussionCategory& category, std::function<void(DiscussionCategory&)>&& fn)
{
    fn(category);
    executeOnAllCategoryParents(category, std::move(fn));
}

bool DiscussionCategory::insertDiscussionThread(const DiscussionThreadRef& thread)
{
    if ( ! DiscussionThreadCollectionBase::insertDiscussionThread(thread))
    {
        return false;
    }
    //don't use updateMessageCount() as insertDiscussionThread will take care of that for totals
    messageCount_ += thread->messages().size();
    thread->addCategory(shared_from_this());

    executeOnCategoryAndAllParents(*this, [&](auto& category)
    {
        //this category and all parents will hold separate references to the new thread
        category.totalThreads_.insertDiscussionThread(thread);
    });

    notifyChangeFn_(*this);
    return true;
}

void DiscussionCategory::modifyDiscussionThread(DiscussionThreadCollection::iterator iterator,
                                                std::function<void(DiscussionThread&)>&& modifyFunction)
{
    if (iterator == threads_.end())
    {
        return;
    }
    auto thread = *iterator;
    if ( ! thread)
    {
        return;
    }
    DiscussionThreadCollectionBase::modifyDiscussionThread(iterator, 
                                                           std::forward<std::function<void(DiscussionThread&)>>(modifyFunction));
    executeOnCategoryAndAllParents(*this, [&](auto& category)
    {
        //update separate references of this category and all parents
        category.totalThreads_.modifyDiscussionThreadById(thread->id());
    });
}

DiscussionThreadRef DiscussionCategory::deleteDiscussionThread(DiscussionThreadCollection::iterator iterator)
{
    DiscussionThreadRef result;
    if ( ! ((result = DiscussionThreadCollectionBase::deleteDiscussionThread(iterator))))
    {
        return result;
    }
    //don't use updateMessageCount() as deleteDiscussionThreadById will take care of that for totals
    messageCount_ -= static_cast<int_fast32_t>(result->messages().size());
    if ( ! result->aboutToBeDeleted())
    {
        result->removeCategory(shared_from_this());
    }

    executeOnCategoryAndAllParents(*this, [&](auto& category)
    {
        category.totalThreads_.deleteDiscussionThreadById(result->id());
    });

    notifyChangeFn_(*this);
    return result;
}

void DiscussionCategory::deleteDiscussionThreadIfNoOtherTagsReferenceIt(const DiscussionThreadRef& thread)
{
    //don't remove the thread just yet, perhaps it's also referenced by other tags
    bool referencedByOtherTags = false;
    for (auto tagWeak : thread->tagsWeak())
    {
        auto currentTag = tagWeak.lock();
        if (currentTag && containsTag(currentTag))
        {
            referencedByOtherTags = true;
            break;
        }
    }
    if ( ! referencedByOtherTags)
    {
        deleteDiscussionThreadById(thread->id());
        //release separate references held by this category and parents, if reference count drops to 0
        executeOnCategoryAndAllParents(*this, [&](auto& category)
        {
            category.totalThreads_.decreaseReferenceCount(thread);
        });
    }
}

bool DiscussionCategory::addTag(const DiscussionTagRef& tag)
{
    if ( ! std::get<1>(tags_.insert(tag)))
    {
        return false;
    }

    for (auto& thread : tag->threads().get<EntityCollection::DiscussionThreadCollectionById>())
    {
        insertDiscussionThread(thread);
    }
    return true;
}

bool DiscussionCategory::removeTag(const DiscussionTagRef& tag)
{
    if (0 == tags_.erase(tag))
    {
        return false;
    }

    for (auto& thread : tag->threads().get<EntityCollection::DiscussionThreadCollectionById>())
    {
        deleteDiscussionThreadIfNoOtherTagsReferenceIt(thread);
    }
    return true;
}

void DiscussionCategory::updateMessageCount(const DiscussionThreadRef& thread, int_fast32_t delta)
{
    messageCount_ += delta;
    //make sure totals of the current category are always updated
    totalThreads_.messageCount() += delta;

    bool stop = false;
    executeOnAllCategoryParents(*this, [&](auto& category)
    {
        if (stop)
        {
            return;
        }
        if (category.containsThread(thread))
        {
            //stop any updates on the total or else the messages will be counted multiple times
            //they will be/have already been taken care of by a call to updateMessageCount of this specific category
            stop = true;
            return;
        }
        category.totalThreads_.messageCount() += delta;
    });
}

void DiscussionCategory::resetTotals()
{
    totalThreads_.threads().clear();
    totalThreads_.messageCount() = 0;
}

void DiscussionCategory::recalculateTotals()
{
    executeOnCategoryAndAllParents(*this, [this](auto& category)
    {
        for (auto& thread : this->threads())
        {
            totalThreads_.insertDiscussionThread(thread);
        }
    });
}

//
//
//Discussion Threads Ref Counted
//
//

const DiscussionThreadMessage* DiscussionThreadRefCountedCollection::latestMessage() const
{
    auto index = threadsByLatestMessageCreated();
    if ( ! index.size())
    {
        return nullptr;
    }
    auto thread = *(index.rbegin());
    auto messageIndex = thread->messagesByCreated();
    if (messageIndex.size())
    {
        return *(messageIndex.rbegin());
    }
    return nullptr;
}

void DiscussionThreadRefCountedCollection::decreaseReferenceCount(const DiscussionThreadRef& thread)
{
    auto it = referenceCount_.find(thread);
    if (it == referenceCount_.end())
    {
        return;
    }
    if ((it->second -= 1) < 1)
    {
        referenceCount_.erase(it);
        if (thread)
        {
            deleteDiscussionThreadById(thread->id());
        }
    }
}

bool DiscussionThreadRefCountedCollection::insertDiscussionThread(const DiscussionThreadRef& thread)
{
    auto it = referenceCount_.find(thread);
    if (it == referenceCount_.end() && DiscussionThreadCollectionBase::insertDiscussionThread(thread))
    {
        referenceCount_.insert(std::make_pair(thread, 1));
        messageCount_ += thread->messagesById().size();
        return true;
    }
    it->second += 1;
    return false;
}

DiscussionThreadRef DiscussionThreadRefCountedCollection::deleteDiscussionThread(
        DiscussionThreadCollection::iterator iterator)
{
    DiscussionThreadRef result;
    if ( ! ((result = DiscussionThreadCollectionBase::deleteDiscussionThread(iterator))))
    {
        return result;
    }
    referenceCount_.erase(result);
    if (result)
    {
        messageCount_ -= result->messages().size();
    }
    return result;
}
