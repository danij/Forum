#include "EntityDiscussionCategoryCollection.h"
#include "ContextProviders.h"

using namespace Forum::Entities;

bool DiscussionCategoryCollection::add(DiscussionCategoryPtr category)
{
    if ( ! std::get<1>(byId_.insert(category))) return false;
    byName_.insert(category);

    if ( ! Context::isBatchInsertInProgress())
    {
        byMessageCount_.insert(category);
        byDisplayOrderRootPriority_.insert(category);
    }

    return true;
}

bool DiscussionCategoryCollection::remove(DiscussionCategoryPtr category)
{
    {
        auto itById = byId_.find(category->id());
        if (itById == byId_.end()) return false;
        
        byId_.erase(itById);
    }
    {
        auto itByName = byName_.find(category->name());
        if (itByName != byName_.end()) byName_.erase(itByName);
    }
    if ( ! Context::isBatchInsertInProgress())
    {
        eraseFromNonUniqueCollection(byMessageCount_, category, category->messageCount());
        eraseFromNonUniqueCollection(byDisplayOrderRootPriority_, category, category->displayOrderWithRootPriority());
    }
    return true;
}

void DiscussionCategoryCollection::stopBatchInsert()
{
    if ( ! Context::isBatchInsertInProgress()) return;

    byMessageCount_.clear();
    for (DiscussionCategoryPtr category : byId_)
    {
        byMessageCount_.insert(category);
    }
        
    byDisplayOrderRootPriority_.clear();
    for (DiscussionCategoryPtr category : byId_)
    {
        byDisplayOrderRootPriority_.insert(category);
    }
}

void DiscussionCategoryCollection::prepareUpdateName(DiscussionCategoryPtr category)
{
    byNameUpdateIt_ = byName_.find(category->name());
}

void DiscussionCategoryCollection::updateName(DiscussionCategoryPtr category)
{
    if (byNameUpdateIt_ != byName_.end())
    {
        byName_.replace(byNameUpdateIt_, category);
    }
}

void DiscussionCategoryCollection::prepareUpdateMessageCount(DiscussionCategoryPtr category)
{
    if (Context::isBatchInsertInProgress()) return;

    byMessageCountUpdateIt_ = findInNonUniqueCollection(byMessageCount_, category, category->messageCount());
}

void DiscussionCategoryCollection::updateMessageCount(DiscussionCategoryPtr category)
{
    if (Context::isBatchInsertInProgress()) return;

    if (byMessageCountUpdateIt_ != byMessageCount_.end())
    {
        byMessageCount_.replace(byMessageCountUpdateIt_, category);
    }
}

void DiscussionCategoryCollection::prepareUpdateDisplayOrderRootPriority(DiscussionCategoryPtr category)
{
    if (Context::isBatchInsertInProgress()) return;

    byDisplayOrderRootPriorityUpdateIt_ = findInNonUniqueCollection(byDisplayOrderRootPriority_, category, 
                                                                    category->displayOrderWithRootPriority());
}

void DiscussionCategoryCollection::updateDisplayOrderRootPriority(DiscussionCategoryPtr category)
{
    if (Context::isBatchInsertInProgress()) return;
    
    if (byDisplayOrderRootPriorityUpdateIt_ != byDisplayOrderRootPriority_.end())
    {
        byDisplayOrderRootPriority_.replace(byDisplayOrderRootPriorityUpdateIt_, category);
    }
}
