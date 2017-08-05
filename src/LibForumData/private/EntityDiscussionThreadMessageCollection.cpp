#include "EntityDiscussionThreadMessageCollection.h"

using namespace Forum::Entities;

bool DiscussionThreadMessageCollection::add(DiscussionThreadMessagePtr message)
{
    if (onPrepareCountChange_) onPrepareCountChange_();

    if ( ! std::get<1>(byId_.insert(message))) return false;
    byCreated_.insert(message);
    
    if (onCountChange_) onCountChange_();
    return true;
}

bool DiscussionThreadMessageCollection::remove(DiscussionThreadMessagePtr message)
{
    if (onPrepareCountChange_) onPrepareCountChange_();
    {
        auto itById = byId_.find(message->id());
        if (itById == byId_.end()) return false;
        
        byId_.erase(itById);
    }
    eraseFromFlatMultisetCollection(byCreated_, message);

    if (onCountChange_) onCountChange_();
    return true;
}

void DiscussionThreadMessageCollection::clear()
{
    byId_.clear();
    byCreated_.clear();
}
