#include "EntityDiscussionCategoryCollection.h"

using namespace Forum::Entities;

bool DiscussionCategoryCollection::add(DiscussionCategoryPtr category)
{
    if ( ! std::get<1>(byId_.insert(category))) return false;
    byName_.insert(category);
    byMessageCount_.insert(category);
    byDisplayOrderRootPriority_.insert(category);

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
    eraseFromNonUniqueCollection(byMessageCount_, category, category->messageCount());
    eraseFromNonUniqueCollection(byDisplayOrderRootPriority_, category, category->displayOrderWithRootPriority());

    return true;
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
    byMessageCountUpdateIt_ = findInNonUniqueCollection(byMessageCount_, category, category->messageCount());
}

void DiscussionCategoryCollection::updateMessageCount(DiscussionCategoryPtr category)
{
    if (byMessageCountUpdateIt_ != byMessageCount_.end())
    {
        byMessageCount_.replace(byMessageCountUpdateIt_, category);
    }
}

void DiscussionCategoryCollection::prepareUpdateDisplayOrderRootPriority(DiscussionCategoryPtr category)
{
    byDisplayOrderRootPriorityUpdateIt_ = findInNonUniqueCollection(byDisplayOrderRootPriority_, category, 
                                                                    category->displayOrderWithRootPriority());
}

void DiscussionCategoryCollection::updateDisplayOrderRootPriority(DiscussionCategoryPtr category)
{
    if (byDisplayOrderRootPriorityUpdateIt_ != byDisplayOrderRootPriority_.end())
    {
        byDisplayOrderRootPriority_.replace(byDisplayOrderRootPriorityUpdateIt_, category);
    }
}
