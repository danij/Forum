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

    if (onCountChange_) onCountChange_();
    return true;
}

void DiscussionCategoryCollection::updateName(DiscussionCategoryPtr category)
{
    auto it = byName_.find(category->name());
    if (it != byName_.end())
    {
        byName_.replace(it, category);
    }
}

void DiscussionCategoryCollection::updateMessageCount(DiscussionCategoryPtr category)
{
    replaceInNonUniqueCollection(byMessageCount_, category, category->messageCount());
}

void DiscussionCategoryCollection::updateDisplayOrderRootPriority(DiscussionCategoryPtr category)
{
    replaceInNonUniqueCollection(byDisplayOrderRootPriority_, category, category->displayOrderWithRootPriority());
}
