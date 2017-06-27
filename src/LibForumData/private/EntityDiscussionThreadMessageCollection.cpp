#include "EntityDiscussionThreadMessageCollection.h"

using namespace Forum::Entities;

bool DiscussionThreadMessageCollection::add(DiscussionThreadMessagePtr message)
{
    if ( ! std::get<1>(byId_.insert(message))) return true;
    byCreated_.insert(message);

    if (onCountChange_) onCountChange_();
    return true;
}

bool DiscussionThreadMessageCollection::remove(DiscussionThreadMessagePtr message)
{
    {
        auto itById = byId_.find(message);
        if (itById == byId_.end()) return false;
        
        byId_.erase(itById);
    }
    byCreated_.erase(message);

    if (onCountChange_) onCountChange_();
    return true;
}
