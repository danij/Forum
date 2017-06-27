#include "EntityDiscussionTagCollection.h"

using namespace Forum::Entities;

bool DiscussionTagCollection::add(DiscussionTagPtr tag)
{
    if ( ! std::get<1>(byId_.insert(tag))) return false;
    byName_.insert(tag);
    byMessageCount_.insert(tag);

    if (onCountChange_) onCountChange_();
    return true;
}

bool DiscussionTagCollection::remove(DiscussionTagPtr tag)
{
    {
        auto itById = byId_.find(tag);
        if (itById == byId_.end()) return false;
        
        byId_.erase(itById);
    }
    {
        auto itByName = byName_.find(tag);
        if (itByName != byName_.end()) byName_.erase(itByName);
    }
    byMessageCount_.erase(tag);

    if (onCountChange_) onCountChange_();
    return true;
}

void DiscussionTagCollection::updateName(DiscussionTagPtr tag)
{
    auto it = byName_.find(tag);
    if (it != byName_.end())
    {
        byName_.replace(it, tag);
    }
}

void DiscussionTagCollection::updateMessageCount(DiscussionTagPtr tag)
{
    auto it = byMessageCount_.find(tag);
    if (it != byMessageCount_.end())
    {
        byMessageCount_.replace(it, tag);
    }
}
