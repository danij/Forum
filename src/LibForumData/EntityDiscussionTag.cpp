#include "EntityDiscussionTag.h"
#include "EntityCollection.h"

using namespace Forum;
using namespace Forum::Entities;

bool DiscussionTag::insertDiscussionThread(const DiscussionThreadRef& thread)
{
    bool result = false;
    modifyWithNotificationFn_(*this, [&](auto& _)
    {
        if ( ! DiscussionThreadCollectionBase::insertDiscussionThread(thread))
        {
            result = false;
            return;
        }
        messageCount() += thread->messages().size();

        for (auto& categoryWeak : categoriesWeak())
        {
            if (auto category = categoryWeak.lock())
            {
                category->insertDiscussionThread(thread);
            }
        }
        result = true;
    });
    return result;
}

DiscussionThreadRef DiscussionTag::deleteDiscussionThread(DiscussionThreadCollection::iterator iterator)
{
    DiscussionThreadRef result;
    modifyWithNotificationFn_(*this, [&](auto& _)
    {
        if ( ! ((result = DiscussionThreadCollectionBase::deleteDiscussionThread(iterator))))
        {
            return;
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
    });
    return result;
}
