#include "MemoryRepositoryDiscussionTag.h"

#include "Configuration.h"
#include "ContextProviders.h"
#include "EntitySerialization.h"
#include "OutputHelpers.h"
#include "RandomGenerator.h"
#include "StringHelpers.h"

using namespace Forum;
using namespace Forum::Configuration;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;

MemoryRepositoryDiscussionTag::MemoryRepositoryDiscussionTag(MemoryStoreRef store)
    : MemoryRepositoryBase(std::move(store)),
    validDiscussionTagNameRegex(boost::make_u32regex("^[^[:space:]]+.*[^[:space:]]+$"))
{
}


StatusCode MemoryRepositoryDiscussionTag::getDiscussionTags(std::ostream& output, RetrieveDiscussionTagsBy by) const
{
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
    {
        auto& currentUser = performedBy.get(collection, store());

        if (Context::getDisplayContext().sortOrder == Context::SortOrder::Ascending)
        {
            switch (by)
            {
            case RetrieveDiscussionTagsBy::Name: 
                writeSingleValueSafeName(output, "tags", Json::enumerate(collection.tagsByName().begin(), 
                                                                         collection.tagsByName().end()));
                break;
            case RetrieveDiscussionTagsBy::MessageCount: 
                writeSingleValueSafeName(output, "tags", Json::enumerate(collection.tagsByMessageCount().begin(),
                                                                         collection.tagsByMessageCount().end()));
                break;
            }
        }
        else
        {
            switch (by)
            {
            case RetrieveDiscussionTagsBy::Name: 
                writeSingleValueSafeName(output, "tags", Json::enumerate(collection.tagsByName().rbegin(), 
                                                                         collection.tagsByName().rend()));
                break;
            case RetrieveDiscussionTagsBy::MessageCount: 
                writeSingleValueSafeName(output, "tags", Json::enumerate(collection.tagsByMessageCount().rbegin(),
                                                                         collection.tagsByMessageCount().rend()));
                break;
            }
        }

        readEvents().onGetDiscussionTags(createObserverContext(currentUser));
    });
    return StatusCode::OK;
}

static StatusCode validateDiscussionTagName(const std::string& name, const boost::u32regex& regex,
    const ConfigConstRef& config)
{
    if (name.empty())
    {
        return StatusCode::INVALID_PARAMETERS;
    }
    
    auto nrCharacters = countUTF8Characters(name);
    if (nrCharacters > config->discussionTag.maxNameLength)
    {
        return StatusCode::VALUE_TOO_LONG;
    }
    if (nrCharacters < config->discussionTag.minNameLength)
    {
        return StatusCode::VALUE_TOO_SHORT;
    }

    try
    {
        if ( ! boost::u32regex_match(name, regex, boost::match_flag_type::format_all))
        {
            return StatusCode::INVALID_PARAMETERS;
        }
    }
    catch (...)
    {
        return StatusCode::INVALID_PARAMETERS;
    }

    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionTag::addNewDiscussionTag(const std::string& name, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    auto validationCode = validateDiscussionTagName(name, validDiscussionTagNameRegex, getGlobalConfig());
    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
    }

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           const auto& createdBy = performedBy.getAndUpdate(collection);
                           
                           auto& indexByName = collection.tags().get<EntityCollection::DiscussionTagCollectionByName>();
                           if (indexByName.find(name) != indexByName.end())
                           {
                               status = StatusCode::ALREADY_EXISTS;
                               return;
                           }
                 
                           auto tag = std::make_shared<DiscussionTag>();
                           tag->notifyChange() = collection.notifyTagChange();
                           tag->id() = generateUUIDString();
                           tag->name() = name;
                           updateCreated(*tag);
                 
                           collection.tags().insert(tag);
                 
                           writeEvents().onAddNewDiscussionTag(createObserverContext(*createdBy), *tag);
                 
                           status.addExtraSafeName("id", tag->id());
                           status.addExtraSafeName("name", tag->name());
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionTag::changeDiscussionTagName(const IdType& id, const std::string& newName, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! id )
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    auto validationCode = validateDiscussionTagName(newName, validDiscussionTagNameRegex, getGlobalConfig());
    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto& indexById = collection.tags().get<EntityCollection::DiscussionTagCollectionById>();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           auto& indexByName = collection.tags().get<EntityCollection::DiscussionTagCollectionByName>();
                           if (indexByName.find(newName) != indexByName.end())
                           {
                               status = StatusCode::ALREADY_EXISTS;
                               return;
                           }
                 
                           auto user = performedBy.getAndUpdate(collection);
                 
                           collection.modifyDiscussionTag(it, [&newName, &user](DiscussionTag& tag)
                                                              {
                                                                  tag.name() = newName;
                                                                  updateLastUpdated(tag, user);
                                                              });
                 
                           writeEvents().onChangeDiscussionTag(createObserverContext(*user), **it, 
                                                              DiscussionTag::ChangeType::Name);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionTag::changeDiscussionTagUiBlob(const IdType& id, const std::string& blob, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! id )
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    if (blob.size() > static_cast<std::string::size_type>(getGlobalConfig()->discussionTag.maxUiBlobSize))
    {
        return status = StatusCode::VALUE_TOO_LONG;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto& indexById = collection.tags().get<EntityCollection::DiscussionTagCollectionById>();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           collection.modifyDiscussionTag(it, [&blob](DiscussionTag& tag)
                                                              {
                                                                  tag.uiBlob() = blob;
                                                              });
                           writeEvents().onChangeDiscussionTag(createObserverContext(*performedBy.getAndUpdate(collection)),
                                                               **it, DiscussionTag::ChangeType::UIBlob);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionTag::deleteDiscussionTag(const IdType& id, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto& indexById = collection.tags().get<EntityCollection::DiscussionTagCollectionById>();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           //make sure the tag is not deleted before being passed to the observers
                           writeEvents().onDeleteDiscussionTag(
                                   createObserverContext(*performedBy.getAndUpdate(collection)), **it);
                           collection.deleteDiscussionTag(it);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionTag::addDiscussionTagToThread(const IdType& tagId, const IdType& threadId, 
                                                                   std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! tagId || ! threadId)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto& tagIndexById = collection.tags().get<EntityCollection::DiscussionTagCollectionById>();
                           auto tagIt = tagIndexById.find(tagId);
                           if (tagIt == tagIndexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                 
                           auto& threadIndexById = collection.threads().get<EntityCollection::DiscussionThreadCollectionById>();
                           auto threadIt = threadIndexById.find(threadId);
                           if (threadIt == threadIndexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                 
                           auto& tagRef = *tagIt;
                           auto& tag = **tagIt;
                           auto& threadRef = *threadIt;
                           auto& thread = **threadIt;
                 
                           //the number of tags associated to a thread is much smaller than 
                           //the number of threads associated to a tag, so search the tag in the thread
                           if ( ! thread.addTag(tagRef))
                           {
                               //actually already added, but return ok
                               status = StatusCode::OK;
                               return;
                           }
                 
                           auto user = performedBy.getAndUpdate(collection);
                 
                           tag.insertDiscussionThread(threadRef);
                           updateLastUpdated(thread, user);
                 
                           writeEvents().onAddDiscussionTagToThread(createObserverContext(*user), tag, thread);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionTag::removeDiscussionTagFromThread(const IdType& tagId, const IdType& threadId, 
                                                                        std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! tagId || ! threadId)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto& tagIndexById = collection.tags().get<EntityCollection::DiscussionTagCollectionById>();
                           auto tagIt = tagIndexById.find(tagId);
                           if (tagIt == tagIndexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                 
                           auto& threadIndexById = collection.threads().get<EntityCollection::DiscussionThreadCollectionById>();
                           auto threadIt = threadIndexById.find(threadId);
                           if (threadIt == threadIndexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                 
                           auto& tagRef = *tagIt;
                           auto& tag = **tagIt;
                           auto& thread = **threadIt;
                 
                           if ( ! thread.removeTag(tagRef))
                           {
                               //tag was not added to the thread
                               status = StatusCode::NO_EFFECT;
                               return;
                           }
                 
                           auto user = performedBy.getAndUpdate(collection);
                 
                           tag.deleteDiscussionThreadById(threadId);
                           updateLastUpdated(thread, user);
                 
                           writeEvents().onRemoveDiscussionTagFromThread(createObserverContext(*user), tag, thread);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionTag::mergeDiscussionTags(const IdType& fromId, const IdType& intoId, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
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
                           auto& indexById = collection.tags().get<EntityCollection::DiscussionTagCollectionById>();
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
                   
                           auto user = performedBy.getAndUpdate(collection);
                           auto& tagFrom = **itFrom;
                           auto& tagIntoRef = *itInto;
                           auto& tagInto = **itInto;
                   
                           //make sure the tag is not deleted before being passed to the observers
                           writeEvents().onMergeDiscussionTags(createObserverContext(*user), tagFrom, tagInto);
                           
                           for (auto& thread : tagFrom.threads())
                           {
                               thread->addTag(tagIntoRef);
                               updateLastUpdated(*thread, user);
                               tagInto.insertDiscussionThread(thread);
                           }
                           for (auto& categoryWeak : tagFrom.categoriesWeak())
                           {
                               if (auto category = categoryWeak.lock())
                               {
                                   category->addTag(tagIntoRef);
                                   updateLastUpdated(*category, user);
                               }
                           }
                   
                           updateLastUpdated(tagInto, user);
                   
                           collection.deleteDiscussionTag(itFrom);
                       });
    return status;
}
