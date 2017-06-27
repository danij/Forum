#include "EntityMessageCommentCollection.h"

using namespace Forum::Entities;

bool MessageCommentCollection::add(MessageCommentPtr comment)
{
    if ( ! std::get<1>(byId_.insert(comment))) return false;
    byCreated_.insert(comment);

    if (onCountChange_) onCountChange_();
    return true;
}

bool MessageCommentCollection::remove(MessageCommentPtr comment)
{
    {
        auto itById = byId_.find(comment);
        if (itById == byId_.end()) return false;
        
        byId_.erase(itById);
    }
    byCreated_.erase(comment);

    if (onCountChange_) onCountChange_();
    return true;
}
