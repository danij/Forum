#include "EntityDiscussionCategoryCollection.h"

using namespace Forum::Entities;

bool DiscussionCategoryCollection::add(DiscussionCategoryPtr category)
{
    if ( ! std::get<1>(byId_.insert(category))) return false;
    byName_.insert(category);
    byMessageCount_.insert(category);
    byDisplayOrderRootPriority_.insert(category);

    if (onCountChange_) onCountChange_();
    return true;
}

bool DiscussionCategoryCollection::remove(DiscussionCategoryPtr category)
{
    {
        auto itById = byId_.find(category);
        if (itById == byId_.end()) return false;
        
        byId_.erase(itById);
    }
    {
        auto itByName = byName_.find(category);
        if (itByName != byName_.end()) byName_.erase(itByName);
    }
    byMessageCount_.erase(category);
    byDisplayOrderRootPriority_.erase(category);

    if (onCountChange_) onCountChange_();
    return true;
}

void DiscussionCategoryCollection::updateName(DiscussionCategoryPtr category)
{
    auto it = byName_.find(category);
    if (it != byName_.end())
    {
        byName_.replace(it, category);
    }
}

void DiscussionCategoryCollection::updateMessageCount(DiscussionCategoryPtr category)
{
    auto it = byMessageCount_.find(category);
    if (it != byMessageCount_.end())
    {
        byMessageCount_.replace(it, category);
    }
}

void DiscussionCategoryCollection::updateDisplayOrderRootPriority(DiscussionCategoryPtr category)
{
    auto it = byDisplayOrderRootPriority_.find(category);
    if (it != byDisplayOrderRootPriority_.end())
    {
        byDisplayOrderRootPriority_.replace(it, category);
    }
}
