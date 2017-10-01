#include "EntityDiscussionTagCollection.h"
#include "ContextProviders.h"

using namespace Forum::Entities;

bool DiscussionTagCollection::add(DiscussionTagPtr tag)
{
    if ( ! std::get<1>(byId_.insert(tag))) return false;
    byName_.insert(tag);

    if ( ! Context::isBatchInsertInProgress())
    {
        byThreadCount_.insert(tag);
        byMessageCount_.insert(tag);
    }

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
    if ( ! Context::isBatchInsertInProgress())
    {
        eraseFromNonUniqueCollection(byThreadCount_, tag, tag->messageCount());
        eraseFromNonUniqueCollection(byMessageCount_, tag, tag->messageCount());
    }

    return true;
}

void DiscussionTagCollection::stopBatchInsert()
{
    if ( ! Context::isBatchInsertInProgress()) return;

    byThreadCount_.clear();
    byMessageCount_.clear();

    auto byIdIndex = byId_;
    byThreadCount_.insert(byId_.begin(), byId_.end());
    byMessageCount_.insert(byId_.begin(), byId_.end());
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

void DiscussionTagCollection::prepareUpdateThreadCount(DiscussionTagPtr tag)
{
    if (Context::isBatchInsertInProgress()) return;

    byThreadCountUpdateIt_ = findInNonUniqueCollection(byThreadCount_, tag, tag->threadCount());
}

void DiscussionTagCollection::updateThreadCount(DiscussionTagPtr tag)
{
    if (Context::isBatchInsertInProgress()) return;

    if (byThreadCountUpdateIt_ != byThreadCount_.end())
    {
        byThreadCount_.replace(byThreadCountUpdateIt_, tag);
    }
}

void DiscussionTagCollection::prepareUpdateMessageCount(DiscussionTagPtr tag)
{
    if (Context::isBatchInsertInProgress()) return;

    byMessageCountUpdateIt_ = findInNonUniqueCollection(byMessageCount_, tag, tag->messageCount());
}

void DiscussionTagCollection::updateMessageCount(DiscussionTagPtr tag)
{
    if (Context::isBatchInsertInProgress()) return;

    if (byMessageCountUpdateIt_ != byMessageCount_.end())
    {
        byMessageCount_.replace(byMessageCountUpdateIt_, tag);
    }
}
