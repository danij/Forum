#include "EntityDiscussionThreadMessageCollection.h"
#include "ContextProviders.h"

#include <algorithm>
#include <vector>

using namespace Forum::Entities;

bool DiscussionThreadMessageCollection::add(DiscussionThreadMessagePtr message)
{
    if (onPrepareCountChange_) onPrepareCountChange_();

    if ( ! std::get<1>(byId_.insert(message))) return false;

    if ( ! Context::isBatchInsertInProgress())
    {
        byCreated_.insert(message);
    }

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

void DiscussionThreadMessageCollection::stopBatchInsert()
{
    if ( ! Context::isBatchInsertInProgress()) return;

    byCreated_.clear();

    //sort the values so that insertion into byCreated_ works faster
    std::vector<DiscussionThreadMessagePtr> temp(byId_.begin(), byId_.end());

    std::sort(temp.begin(), temp.end(), [](DiscussionThreadMessagePtr first, DiscussionThreadMessagePtr second)
    {
        return first->created() < second->created();
    });

    byCreated_.insert(boost::container::ordered_range, temp.begin(), temp.end());
}
