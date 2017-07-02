#include "EntityDiscussionTagCollection.h"

using namespace Forum::Entities;

bool DiscussionTagCollection::add(DiscussionTagPtr tag)
{
    if ( ! std::get<1>(byId_.insert(tag))) return false;
    byName_.insert(tag);
    byMessageCount_.insert(tag);

    return true;
}

bool DiscussionTagCollection::remove(DiscussionTagPtr tag)
{
    {
        auto itById = byId_.find(tag->id());
        if (itById == byId_.end()) return false;
        
        byId_.erase(itById);
    }
    {
        auto itByName = byName_.find(tag->name());
        if (itByName != byName_.end()) byName_.erase(itByName);
    }
    eraseFromNonUniqueCollection(byMessageCount_, tag, tag->messageCount());

    return true;
}

void DiscussionTagCollection::prepareUpdateName(DiscussionTagPtr tag)
{
    byNameUpdateIt_ = byName_.find(tag->name());
}

void DiscussionTagCollection::updateName(DiscussionTagPtr tag)
{
    if (byNameUpdateIt_ != byName_.end())
    {
        byName_.replace(byNameUpdateIt_, tag);
    }
}

void DiscussionTagCollection::prepareUpdateMessageCount(DiscussionTagPtr tag)
{
    byMessageCountUpdateIt_ = findInNonUniqueCollection(byMessageCount_, tag, tag->messageCount());
}

void DiscussionTagCollection::updateMessageCount(DiscussionTagPtr tag)
{
    if (byMessageCountUpdateIt_ != byMessageCount_.end())
    {
        byMessageCount_.replace(byMessageCountUpdateIt_, tag);
    }
}

void DiscussionTagCollection::refreshByMessageCount()
{
    byMessageCount_.clear();
    for (DiscussionTagPtr tag : byId_)
    {
        byMessageCount_.insert(tag);
    }
}
