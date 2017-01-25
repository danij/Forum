#include "MemoryRepository.h"

#include "Configuration.h"
#include "ContextProviders.h"
#include "EntitySerialization.h"
#include "OutputHelpers.h"
#include "RandomGenerator.h"
#include "StateHelpers.h"
#include "StringHelpers.h"

using namespace Forum;
using namespace Forum::Configuration;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;

template<typename TagsIndexFn>
static void writeDiscussionTags(std::ostream& output, PerformedByWithLastSeenUpdateGuard&& performedBy,
    const ResourceGuard<EntityCollection>& collection_, const ReadEvents& readEvents_, TagsIndexFn&& tagsIndexFn)
{
    collection_.read([&](const EntityCollection& collection)
    {
        auto& currentUser = performedBy.get(collection);
        const auto tags = tagsIndexFn(collection);

        if (Context::getDisplayContext().sortOrder == Context::SortOrder::Ascending)
        {
            writeSingleObjectSafeName(output, "tags", Json::enumerate(tags.begin(), tags.end()));
        }
        else
        {
            writeSingleObjectSafeName(output, "tags", Json::enumerate(tags.rbegin(), tags.rend()));
        }

        readEvents_.onGetDiscussionTags(createObserverContext(currentUser));
    });
}

void MemoryRepository::getDiscussionTagsByName(std::ostream& output) const
{
    writeDiscussionTags(output, preparePerformedBy(), collection_, readEvents_, [](const auto& collection)
    {
        return collection.tagsByName();
    });
}

void MemoryRepository::getDiscussionTagsByMessageCount(std::ostream& output) const
{
    writeDiscussionTags(output, preparePerformedBy(), collection_, readEvents_, [](const auto& collection)
    {
        return collection.tagsByMessageCount();
    });
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

void MemoryRepository::addNewDiscussionTag(const std::string& name, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    auto validationCode = validateDiscussionTagName(name, validDiscussionTagNameRegex, getGlobalConfig());
    if (validationCode != StatusCode::OK)
    {
        status = validationCode;
        return;
    }

    auto performedBy = preparePerformedBy();

    collection_.write([&](EntityCollection& collection)
                      {
                          const auto& createdBy = performedBy.getAndUpdate(collection);
                          
                          auto& indexByName = collection.tags().get<EntityCollection::DiscussionTagCollectionByName>();
                          if (indexByName.find(name) != indexByName.end())
                          {
                              status = StatusCode::ALREADY_EXISTS;
                              return;
                          }

                          auto tag = std::make_shared<DiscussionTag>();
                          tag->id() = generateUUIDString();
                          tag->name() = name;
                          tag->created() = Context::getCurrentTime();

                          collection.tags().insert(tag);

                          writeEvents_.onAddNewDiscussionTag(createObserverContext(*createdBy), *tag);

                          status.addExtraSafeName("id", tag->id());
                          status.addExtraSafeName("name", tag->name());
                      });
}

void MemoryRepository::changeDiscussionTagName(const IdType& id, const std::string& newName, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    auto validationCode = validateDiscussionTagName(newName, validDiscussionTagNameRegex, getGlobalConfig());
    if (validationCode != StatusCode::OK)
    {
        status = validationCode;
        return;
    }
    auto performedBy = preparePerformedBy();

    collection_.write([&](EntityCollection& collection)
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
                          collection.modifyDiscussionTag(it, [&newName](DiscussionTag& tag)
                          {
                              tag.name() = newName;
                          });
                          writeEvents_.onChangeDiscussionTag(createObserverContext(*performedBy.getAndUpdate(collection)),
                                                             **it, DiscussionTag::ChangeType::Name);
                      });
}

void MemoryRepository::changeDiscussionTagUiBlob(const IdType& id, const std::string& blob, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    
    if (blob.size() > static_cast<std::string::size_type>(getGlobalConfig()->discussionTag.maxUiBlobSize))
    {
        status = StatusCode::VALUE_TOO_LONG;
        return;
    }
    auto performedBy = preparePerformedBy();

    collection_.write([&](EntityCollection& collection)
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
                          writeEvents_.onChangeDiscussionTag(createObserverContext(*performedBy.getAndUpdate(collection)),
                                                             **it, DiscussionTag::ChangeType::UIBlob);
                      });
}

void MemoryRepository::deleteDiscussionTag(const IdType& id, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! id)
    {
        status = StatusCode::INVALID_PARAMETERS;
        return;
    }

    auto performedBy = preparePerformedBy();

    collection_.write([&](EntityCollection& collection)
                      {
                          auto& indexById = collection.tags().get<EntityCollection::DiscussionTagCollectionById>();
                          auto it = indexById.find(id);
                          if (it == indexById.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }
                          //make sure the tag is not deleted before being passed to the observers
                          writeEvents_.onDeleteDiscussionTag(
                                  createObserverContext(*performedBy.getAndUpdate(collection)), **it);
                          collection.deleteDiscussionTag(it);
                      });
}

void MemoryRepository::addDiscussionTagToThread(const IdType& tagId, const IdType& threadId, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! tagId || ! threadId)
    {
        status = StatusCode::INVALID_PARAMETERS;
        return;
    }

    auto performedBy = preparePerformedBy();

    collection_.write([&](EntityCollection& collection)
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

                          DiscussionTagWeakRef tagWeak(*tagIt);

                          //the number of tags associated to a thread is much smaller than 
                          //the number of threads associated to a tag, so search the tag in the thread
                          if ( ! (*threadIt)->addTag(tagWeak))
                          {
                              //actually already added, but return ok
                              status = StatusCode::OK;
                              return;
                          }

                          (*tagIt)->threads().insert(*threadIt);
                          collection.modifyDiscussionTag(tagIt, [&threadIt](auto& tag)
                          {
                              tag.messageCount() += (*threadIt)->messages().size();
                          });

                          writeEvents_.onAddDiscussionTagToThread(
                                  createObserverContext(*performedBy.getAndUpdate(collection)), **tagIt, **threadIt);
                      });
}

void MemoryRepository::removeDiscussionTagFromThread(const IdType& tagId, const IdType& threadId, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! tagId || ! threadId)
    {
        status = StatusCode::INVALID_PARAMETERS;
        return;
    }

    auto performedBy = preparePerformedBy();

    collection_.write([&](EntityCollection& collection)
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

                          DiscussionTagWeakRef tagWeak(*tagIt);

                          if ( ! (*threadIt)->removeTag(tagWeak))
                          {
                              //tag was not added to the thread
                              status = StatusCode::NO_EFFECT;
                              return;
                          }

                          (*tagIt)->deleteDiscussionThreadById(threadId);
                          collection.modifyDiscussionTag(tagIt, [&threadIt](auto& tag)
                          {
                              tag.messageCount() -= (*threadIt)->messages().size();
                          });

                          writeEvents_.onRemoveDiscussionTagFromThread(
                                  createObserverContext(*performedBy.getAndUpdate(collection)), **tagIt, **threadIt);
                      });    
}

void MemoryRepository::mergeDiscussionTags(const IdType& fromId, const IdType& intoId, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! fromId || ! intoId)
    {
        status = StatusCode::INVALID_PARAMETERS;
        return;
    }
    if (fromId == intoId)
    {
        status = StatusCode::NO_EFFECT;
        return;
    }

    auto performedBy = preparePerformedBy();

    collection_.write([&](EntityCollection& collection)
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
                        //make sure the tag is not deleted before being passed to the observers
                        writeEvents_.onMergeDiscussionTags(
                                createObserverContext(*performedBy.getAndUpdate(collection)), **itFrom, **itInto);

                        DiscussionTagWeakRef fromTagWeak(*itFrom);
                        DiscussionTagWeakRef intoTagWeak(*itInto);

                        collection.modifyDiscussionTag(itInto, [&](DiscussionTag& tag)
                        {
                            tag.messageCount() = 0;
                            for (auto& thread : (*itFrom)->threads())
                            {
                                tag.threads().insert(thread);
                                thread->removeTag(fromTagWeak);
                                thread->addTag(intoTagWeak);
                                //recalculate total message count of into tag
                                tag.messageCount() += thread->messages().size();
                            }
                        });
                        collection.deleteDiscussionTag(itFrom);
                    });
}