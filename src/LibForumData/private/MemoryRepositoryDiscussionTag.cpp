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

#include "MemoryRepositoryDiscussionTag.h"

#include "Configuration.h"
#include "ContextProviders.h"
#include "EntitySerialization.h"
#include "OutputHelpers.h"
#include "RandomGenerator.h"
#include "StringHelpers.h"
#include "Logging.h"

using namespace Forum;
using namespace Forum::Configuration;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;
using namespace Forum::Authorization;

MemoryRepositoryDiscussionTag::MemoryRepositoryDiscussionTag(MemoryStoreRef store,
                                                             DiscussionTagAuthorizationRef authorization)
    : MemoryRepositoryBase(std::move(store)), authorization_(std::move(authorization))
{
    if ( ! authorization_)
    {
        throw std::runtime_error("Authorization implementation not provided");
    }
}


StatusCode MemoryRepositoryDiscussionTag::getDiscussionTags(OutStream& output, RetrieveDiscussionTagsBy by) const
{
    StatusWriter status(output);
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
    {
        auto& currentUser = performedBy.get(collection, *store_);

        if ( ! (status = authorization_->getDiscussionTags(currentUser)))
        {
            return;
        }

        SerializationRestriction restriction(collection.grantedPrivileges(), collection,
                                             &currentUser, Context::getCurrentTime());

        if (Context::getDisplayContext().sortOrder == Context::SortOrder::Ascending)
        {
            switch (by)
            {
            case RetrieveDiscussionTagsBy::Name:
                writeArraySafeName(output, "tags", collection.tags().byName().begin(), collection.tags().byName().end(),
                                   restriction);
                status.disable();
                break;
            case RetrieveDiscussionTagsBy::ThreadCount:
                //collection is sorted in greater order
                writeArraySafeName(output, "tags", collection.tags().byThreadCount().rbegin(),
                                   collection.tags().byThreadCount().rend(), restriction);
                status.disable();
                break;
            case RetrieveDiscussionTagsBy::MessageCount:
                //collection is sorted in greater order
                writeArraySafeName(output, "tags", collection.tags().byMessageCount().rbegin(),
                                   collection.tags().byMessageCount().rend(), restriction);
                status.disable();
                break;
            }
        }
        else
        {
            switch (by)
            {
            case RetrieveDiscussionTagsBy::Name:
                writeArraySafeName(output, "tags", collection.tags().byName().rbegin(), collection.tags().byName().rend(),
                                   restriction);
                status.disable();
                break;
            case RetrieveDiscussionTagsBy::ThreadCount:
                //collection is sorted in greater order
                writeArraySafeName(output, "tags", collection.tags().byThreadCount().begin(),
                    collection.tags().byThreadCount().end(), restriction);
                status.disable();
                break;
            case RetrieveDiscussionTagsBy::MessageCount:
                //collection is sorted in greater order
                writeArraySafeName(output, "tags", collection.tags().byMessageCount().begin(),
                                   collection.tags().byMessageCount().end(), restriction);
                status.disable();
                break;
            }
        }

        readEvents().onGetDiscussionTags(createObserverContext(currentUser));
    });
    return status;
}


StatusCode MemoryRepositoryDiscussionTag::addNewDiscussionTag(StringView name, OutStream& output)
{
    StatusWriter status(output);

    const auto config = getGlobalConfig();
    const auto validationCode = validateString(name, INVALID_PARAMETERS_FOR_EMPTY_STRING,
                                               config->discussionTag.minNameLength,
                                               config->discussionTag.maxNameLength,
                                               &MemoryRepositoryBase::doesNotContainLeadingOrTrailingWhitespace);
    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
    }

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           if ( ! (status = authorization_->addNewDiscussionTag(*currentUser, name)))
                           {
                               return;
                           }

                           DiscussionTag::NameType nameString(name);

                           auto& indexByName = collection.tags().byName();
                           if (indexByName.find(nameString) != indexByName.end())
                           {
                               status = StatusCode::ALREADY_EXISTS;
                               return;
                           }

                           auto statusWithResource = addNewDiscussionTag(collection, generateUniqueId(),
                                                                         std::move(nameString));
                           auto& tag = statusWithResource.resource;
                           if ( ! (status = statusWithResource.status)) return;

                           writeEvents().onAddNewDiscussionTag(createObserverContext(*currentUser), *tag);

                           status.writeNow([&](auto& writer)
                                           {
                                               JSON_WRITE_PROP(writer, "id", tag->id());
                                               JSON_WRITE_PROP(writer, "name", tag->name().string());
                                           });
                       });
    return status;
}

StatusWithResource<DiscussionTagPtr> MemoryRepositoryDiscussionTag::addNewDiscussionTag(EntityCollection& collection,
                                                                                        IdTypeRef id, StringView name)
{
    return addNewDiscussionTag(collection, id, DiscussionTag::NameType(name));
}

StatusWithResource<DiscussionTagPtr> MemoryRepositoryDiscussionTag::addNewDiscussionTag(EntityCollection& collection,
                                                                                        IdTypeRef id,
                                                                                        DiscussionTag::NameType&& name)
{
    auto& indexByName = collection.tags().byName();
    if (indexByName.find(name) != indexByName.end())
    {
        FORUM_LOG_ERROR << "A discussion tag with this name already exists: " << name.string();
        return StatusCode::ALREADY_EXISTS;
    }

    //IdType id, Timestamp created, VisitDetails creationDetails
    const auto tag = collection.createDiscussionTag(id, std::move(name), Context::getCurrentTime(),
                                                    { Context::getCurrentUserIpAddress() });
    collection.insertDiscussionTag(tag);

    return tag;
}

StatusCode MemoryRepositoryDiscussionTag::changeDiscussionTagName(IdTypeRef id, StringView newName,
                                                                  OutStream& output)
{
    StatusWriter status(output);
    if ( ! id )
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    const auto config = getGlobalConfig();
    const auto validationCode = validateString(newName, INVALID_PARAMETERS_FOR_EMPTY_STRING,
                                               config->discussionTag.minNameLength,
                                               config->discussionTag.maxNameLength,
                                               &MemoryRepositoryBase::doesNotContainLeadingOrTrailingWhitespace);
    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.tags().byId();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           DiscussionTag::NameType newNameString(newName);

                           auto& indexByName = collection.tags().byName();
                           if (indexByName.find(newNameString) != indexByName.end())
                           {
                               status = StatusCode::ALREADY_EXISTS;
                               return;
                           }

                           if ( ! (status = authorization_->changeDiscussionTagName(*currentUser, **it, newName)))
                           {
                               return;
                           }

                           if ( ! (status = changeDiscussionTagName(collection, id, newName))) return;

                           writeEvents().onChangeDiscussionTag(createObserverContext(*currentUser), **it,
                                                               DiscussionTag::ChangeType::Name);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionTag::changeDiscussionTagName(EntityCollection& collection, IdTypeRef id,
                                                                  StringView newName)
{
    const auto currentUser = getCurrentUser(collection);

    auto& indexById = collection.tags().byId();
    auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion tag: " << id.toStringDashed();
        return StatusCode::NOT_FOUND;
    }

    DiscussionTag::NameType newNameString(newName);

    DiscussionTagPtr tagPtr = *it;
    DiscussionTag& tag = *tagPtr;

    tag.updateName(std::move(newNameString));
    updateLastUpdated(tag, currentUser);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionTag::changeDiscussionTagUiBlob(IdTypeRef id, StringView blob,
                                                                    OutStream& output)
{
    StatusWriter status(output);
    if ( ! id )
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    if (blob.size() > static_cast<decltype(blob.size())>(getGlobalConfig()->discussionTag.maxUiBlobSize))
    {
        return status = StatusCode::VALUE_TOO_LONG;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.tags().byId();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           if ( ! (status = authorization_->changeDiscussionTagUiBlob(*currentUser, **it, blob)))
                           {
                               return;
                           }

                           if ( ! (status = changeDiscussionTagUiBlob(collection, id, blob))) return;

                           writeEvents().onChangeDiscussionTag(createObserverContext(*currentUser),
                                                               **it, DiscussionTag::ChangeType::UIBlob);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionTag::changeDiscussionTagUiBlob(EntityCollection& collection, IdTypeRef id, StringView blob)
{
    auto& indexById = collection.tags().byId();
    const auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion tag: " << id.toStringDashed();
        return StatusCode::NOT_FOUND;
    }

    DiscussionTagPtr tagPtr = *it;
    tagPtr->uiBlob() = toString(blob);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionTag::deleteDiscussionTag(IdTypeRef id, OutStream& output)
{
    StatusWriter status(output);
    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.tags().byId();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           if ( ! (status = authorization_->deleteDiscussionTag(*currentUser, **it)))
                           {
                               return;
                           }

                           //make sure the tag is not deleted before being passed to the observers
                           writeEvents().onDeleteDiscussionTag(createObserverContext(*currentUser), **it);

                           status = deleteDiscussionTag(collection, id);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionTag::deleteDiscussionTag(EntityCollection& collection, IdTypeRef id)
{
    auto& indexById = collection.tags().byId();
    const auto it = indexById.find(id);
    if (it == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion tag: " << id.toStringDashed();
        return StatusCode::NOT_FOUND;
    }

    collection.deleteDiscussionTag(*it);
    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionTag::addDiscussionTagToThread(IdTypeRef tagId, IdTypeRef threadId,
                                                                   OutStream& output)
{
    StatusWriter status(output);
    if ( ! tagId || ! threadId)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& tagIndexById = collection.tags().byId();
                           auto tagIt = tagIndexById.find(tagId);
                           if (tagIt == tagIndexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           auto threadPtr = collection.threads().findById(threadId);
                           if ( ! threadPtr)
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           auto& tag = **tagIt;
                           auto& thread = *threadPtr;

                           if ( ! (status = authorization_->addDiscussionTagToThread(*currentUser, tag, thread)))
                           {
                               return;
                           }

                           if ( ! (status = addDiscussionTagToThread(collection, tagId, threadId))) return;

                           writeEvents().onAddDiscussionTagToThread(createObserverContext(*currentUser), tag, thread);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionTag::addDiscussionTagToThread(EntityCollection& collection, IdTypeRef tagId,
                                                                   IdTypeRef threadId)
{
    auto& tagIndexById = collection.tags().byId();
    const auto tagIt = tagIndexById.find(tagId);
    if (tagIt == tagIndexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion tag: " << tagId.toStringDashed();
        return StatusCode::NOT_FOUND;
    }

    auto threadPtr = collection.threads().findById(threadId);
    if ( ! threadPtr)
    {
        FORUM_LOG_ERROR << "Could not find discussion thread: " << threadId.toStringDashed();
        return StatusCode::NOT_FOUND;
    }

    DiscussionTagPtr tagPtr = *tagIt;
    DiscussionTag& tag = *tagPtr;
    DiscussionThread& thread = *threadPtr;

    const auto currentUser = getCurrentUser(collection);

    //the number of tags associated to a thread is much smaller than
    //the number of threads associated to a tag, so search the tag in the thread
    if ( ! thread.addTag(tagPtr))
    {
        return StatusCode::NO_EFFECT;
    }

    tag.insertDiscussionThread(threadPtr);
    updateThreadLastUpdated(thread, currentUser);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionTag::removeDiscussionTagFromThread(IdTypeRef tagId, IdTypeRef threadId,
                                                                        OutStream& output)
{
    StatusWriter status(output);
    if ( ! tagId || ! threadId)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& tagIndexById = collection.tags().byId();
                           auto tagIt = tagIndexById.find(tagId);
                           if (tagIt == tagIndexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           auto threadPtr = collection.threads().findById(threadId);
                           if ( ! threadPtr)
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           auto& tag = **tagIt;
                           auto& thread = *threadPtr;

                           if ( ! (status = authorization_->removeDiscussionTagFromThread(*currentUser, tag, thread)))
                           {
                               return;
                           }

                           if ( ! (status = removeDiscussionTagFromThread(collection, tagId, threadId))) return;

                           writeEvents().onRemoveDiscussionTagFromThread(createObserverContext(*currentUser), tag, thread);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionTag::removeDiscussionTagFromThread(EntityCollection& collection,
                                                                        IdTypeRef tagId, IdTypeRef threadId)
{

    auto& tagIndexById = collection.tags().byId();
    const auto tagIt = tagIndexById.find(tagId);
    if (tagIt == tagIndexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion tag: " << tagId.toStringDashed();
        return StatusCode::NOT_FOUND;
    }

    auto threadPtr = collection.threads().findById(threadId);
    if ( ! threadPtr)
    {
        FORUM_LOG_ERROR << "Could not find discussion thread: " << threadId.toStringDashed();
        return StatusCode::NOT_FOUND;
    }

    DiscussionTagPtr tagPtr = *tagIt;
    DiscussionTag& tag = *tagPtr;
    DiscussionThread& thread = *threadPtr;

    const auto currentUser = getCurrentUser(collection);

    if ( ! thread.removeTag(tagPtr))
    {
        //tag was not added to the thread
        return StatusCode::NO_EFFECT;
    }

    tag.deleteDiscussionThread(threadPtr, true);
    updateThreadLastUpdated(thread, currentUser);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionTag::mergeDiscussionTags(IdTypeRef fromId, IdTypeRef intoId,
                                                              OutStream& output)
{
    StatusWriter status(output);
    if ( ! fromId || ! intoId)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    if (fromId == intoId)
    {
        return status = StatusCode::NO_EFFECT;
    }

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.tags().byId();
                           auto itFrom = indexById.find(fromId);
                           if (itFrom == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           auto itInto = indexById.find(intoId);
                           if (itInto == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           auto& tagFrom = **itFrom;
                           auto& tagInto = **itInto;

                           if ( ! (status = authorization_->mergeDiscussionTags(*currentUser, tagFrom, tagInto)))
                           {
                               return;
                           }

                           //make sure the tag is not deleted before being passed to the observers
                           writeEvents().onMergeDiscussionTags(createObserverContext(*currentUser), tagFrom, tagInto);

                           status = mergeDiscussionTags(collection, fromId, intoId);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionTag::mergeDiscussionTags(EntityCollection& collection, IdTypeRef fromId,
                                                              IdTypeRef intoId)
{
    if (fromId == intoId)
    {
        FORUM_LOG_ERROR << "Cannot merge discussion tag into itself: " << fromId.toStringDashed();
        return StatusCode::NO_EFFECT;
    }

    auto& indexById = collection.tags().byId();
    const auto itFrom = indexById.find(fromId);
    if (itFrom == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion tag: " << fromId.toStringDashed();
        return StatusCode::NOT_FOUND;
    }
    const auto itInto = indexById.find(intoId);
    if (itInto == indexById.end())
    {
        FORUM_LOG_ERROR << "Could not find discussion tag: " << intoId.toStringDashed();
        return StatusCode::NOT_FOUND;
    }

    DiscussionTagPtr tagFromPtr = *itFrom;
    DiscussionTag& tagFrom = *tagFromPtr;
    DiscussionTagPtr tagIntoRef = *itInto;
    DiscussionTag& tagInto = *tagIntoRef;

    const auto currentUser = getCurrentUser(collection);

    tagFrom.threads().iterateThreads([tagIntoRef, &tagInto](DiscussionThreadPtr threadPtr)
    {
        assert(threadPtr);
        threadPtr->addTag(tagIntoRef);

        tagInto.insertDiscussionThread(threadPtr);
    });

    for (DiscussionCategoryPtr category : tagFrom.categories())
    {
        assert(category);
        category->addTag(tagIntoRef);
    }

    updateLastUpdated(tagInto, currentUser);

    collection.deleteDiscussionTag(tagFromPtr);

    return StatusCode::OK;
}
