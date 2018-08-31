/*
Fast Forum Backend
Copyright (C) 2016-present Daniel Jurcau

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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
User* Private::getEntityFromGlobalCollection<User>(const size_t index)
{
    return getGlobalEntityCollectionInline().getUserPoolRoot()[index].get();
}

template<>
DiscussionThread* Private::getEntityFromGlobalCollection<DiscussionThread>(const size_t index)
{
    return getGlobalEntityCollectionInline().getDiscussionThreadPoolRoot()[index].get();
}

template<>
DiscussionThreadMessage* Private::getEntityFromGlobalCollection<DiscussionThreadMessage>(const size_t index)
{
    return getGlobalEntityCollectionInline().getDiscussionThreadMessagePoolRoot()[index].get();
}

template<>
DiscussionTag* Private::getEntityFromGlobalCollection<DiscussionTag>(const size_t index)
{
    return getGlobalEntityCollectionInline().getDiscussionTagPoolRoot()[index].get();
}

template<>
DiscussionCategory* Private::getEntityFromGlobalCollection<DiscussionCategory>(const size_t index)
{
    return getGlobalEntityCollectionInline().getDiscussionCategoryPoolRoot()[index].get();
}

template<>
MessageComment* Private::getEntityFromGlobalCollection<MessageComment>(const size_t index)
{
    return getGlobalEntityCollectionInline().getMessageCommentPoolRoot()[index].get();
}
