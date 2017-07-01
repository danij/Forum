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
        auto itById = byId_.find(message->id());
        if (itById == byId_.end()) return false;
        
        byId_.erase(itById);
    }
    eraseFromNonUniqueCollection(byCreated_, message, message->created());

    if (onCountChange_) onCountChange_();
    return true;
}

void DiscussionThreadMessageCollection::clear()
{
    byId_.clear();
    byCreated_.clear();
}
