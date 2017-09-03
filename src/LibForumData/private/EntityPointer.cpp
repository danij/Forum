#include "EntityPointer.h"
#include "EntityCollection.h"

using namespace Forum::Entities;
using namespace Forum::Entities::Private;

/**
 * Only one EntityCollection is used at a time during the runtime of the application or it's tests
 * Thus memory can be saved by keeping only an index to entities stored in vector collections.
 * 
 * Memory management also becomes easier than with std::shared/weak_ptrs
 */
static EntityCollection* globalEntityCollection = nullptr;

inline EntityCollection& getGlobalEntityCollectionInline()
{
    assert(globalEntityCollection);
    return *globalEntityCollection;
}

EntityCollection& Private::getGlobalEntityCollection()
{
    return getGlobalEntityCollectionInline();
}

void Private::setGlobalEntityCollection(EntityCollection* collection)
{
    globalEntityCollection = collection;
}

template<>
User* Private::getEntityFromGlobalCollection<User>(size_t index)
{
    return getGlobalEntityCollectionInline().getUserPoolRoot()[index].get();
}

template<>
DiscussionThread* Private::getEntityFromGlobalCollection<DiscussionThread>(size_t index)
{
    return getGlobalEntityCollectionInline().getDiscussionThreadPoolRoot()[index].get();
}

template<>
DiscussionThreadMessage* Private::getEntityFromGlobalCollection<DiscussionThreadMessage>(size_t index)
{
    return getGlobalEntityCollectionInline().getDiscussionThreadMessagePoolRoot()[index].get();
}

template<>
DiscussionTag* Private::getEntityFromGlobalCollection<DiscussionTag>(size_t index)
{
    return getGlobalEntityCollectionInline().getDiscussionTagPoolRoot()[index].get();
}

template<>
DiscussionCategory* Private::getEntityFromGlobalCollection<DiscussionCategory>(size_t index)
{
    return getGlobalEntityCollectionInline().getDiscussionCategoryPoolRoot()[index].get();
}

template<>
MessageComment* Private::getEntityFromGlobalCollection<MessageComment>(size_t index)
{
    return getGlobalEntityCollectionInline().getMessageCommentPoolRoot()[index].get();
}
