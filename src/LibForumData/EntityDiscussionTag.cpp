#include "EntityDiscussionTag.h"
#include "EntityCollection.h"

using namespace Forum;
using namespace Forum::Entities;

bool DiscussionTag::insertDiscussionThread(const DiscussionThreadRef& thread)
{
    if ( ! DiscussionThreadCollectionBase::insertDiscussionThread(thread))
    {
        return false;
    }
    messageCount() += thread->messages().size();

    for (auto& categoryWeak : categoriesWeak())
    {
        if (auto category = categoryWeak.lock())
        {
            category->insertDiscussionThread(thread);
        }
    }
    notifyChangeFn_(*this);
    return true;
}

DiscussionThreadRef DiscussionTag::deleteDiscussionThread(DiscussionThreadCollection::iterator iterator)
{
    DiscussionThreadRef result;
    if ( ! ((result = DiscussionThreadCollectionBase::deleteDiscussionThread(iterator))))
    {
        return result;
    }
    messageCount() -= result->messages().size();
    for (auto& categoryWeak : categoriesWeak())
    {
        if (auto category = categoryWeak.lock())
        {
            //called from detaching a tag from a thread
            category->deleteDiscussionThreadIfNoOtherTagsReferenceIt(result);
        }
    }
    notifyChangeFn_(*this);
    return result;
}
