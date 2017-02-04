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

template<typename CategoriesIndexFn>
static void writeDiscussionCategories(std::ostream& output, PerformedByWithLastSeenUpdateGuard&& performedBy,
    const ResourceGuard<EntityCollection>& collection_, const ReadEvents& readEvents_, CategoriesIndexFn&& categoriesIndexFn)
{
    collection_.read([&](const EntityCollection& collection)
    {
        auto& currentUser = performedBy.get(collection);
        const auto categories = categoriesIndexFn(collection);

        if (Context::getDisplayContext().sortOrder == Context::SortOrder::Ascending)
        {
            writeSingleObjectSafeName(output, "categories", Json::enumerate(categories.begin(), categories.end()));
        }
        else
        {
            writeSingleObjectSafeName(output, "categories", Json::enumerate(categories.rbegin(), categories.rend()));
        }

        readEvents_.onGetDiscussionCategories(createObserverContext(currentUser));
    });
}

void MemoryRepository::getDiscussionCategoriesByName(std::ostream& output) const
{
    writeDiscussionCategories(output, preparePerformedBy(), collection_, readEvents_, [](const auto& collection)
    {
        return collection.categoriesByName();
    });
}

void MemoryRepository::getDiscussionCategoriesByMessageCount(std::ostream& output) const
{
    writeDiscussionCategories(output, preparePerformedBy(), collection_, readEvents_, [](const auto& collection)
    {
        return collection.categoriesByMessageCount();
    });
}

void MemoryRepository::getDiscussionCategoriesFromRoot(std::ostream& output) const
{
    auto performedBy = preparePerformedBy();
    collection_.read([&](const EntityCollection& collection)
    {
        auto& currentUser = performedBy.get(collection);
        auto index = collection.categoriesByDisplayOrderRootPriority();
        auto indexBegin = index.begin();
        auto indexEnd = index.end();

        auto indexRootEnd = indexBegin;
        while ((indexRootEnd != indexEnd) && ((*indexRootEnd)->isRootCategory()))
        {
            ++indexRootEnd;
        }
        
        BoolTemporaryChanger _(serializationSettings.showDiscussionCategoryChildren, true);

        writeSingleObjectSafeName(output, "categories", Json::enumerate(indexBegin, indexRootEnd));

        readEvents_.onGetRootDiscussionCategories(createObserverContext(currentUser));
    });
}

void MemoryRepository::getDiscussionCategoryById(const IdType& id, std::ostream& output) const
{
    if ( ! id)
    {
        writeStatusCode(output, StatusCode::INVALID_PARAMETERS);
        return;
    }
    auto performedBy = preparePerformedBy();

    collection_.read([&](const EntityCollection& collection)
    {
        auto& currentUser = performedBy.get(collection);
        const auto& index = collection.categoriesById();
        auto it = index.find(id);
        if (it == index.end())
        {
            writeStatusCode(output, StatusCode::NOT_FOUND);
        }
        else
        {
            BoolTemporaryChanger _(serializationSettings.showDiscussionCategoryChildren, true);
            writeSingleObjectSafeName(output, "category", **it);
        }
        readEvents_.onGetDiscussionCategory(createObserverContext(currentUser), **it);
    });
}

static StatusCode validateDiscussionCategoryName(const std::string& name, const boost::u32regex& regex,
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

void MemoryRepository::addNewDiscussionCategory(const std::string& name, const IdType& parentId, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    auto validationCode = validateDiscussionCategoryName(name, validDiscussionCategoryNameRegex, getGlobalConfig());
    if (validationCode != StatusCode::OK)
    {
        status = validationCode;
        return;
    }

    auto performedBy = preparePerformedBy();

    collection_.write([&](EntityCollection& collection)
                      {
                          const auto& createdBy = performedBy.getAndUpdate(collection);
                          
                          auto& indexByName = collection.categories().get<EntityCollection::DiscussionCategoryCollectionByName>();
                          if (indexByName.find(name) != indexByName.end())
                          {
                              status = StatusCode::ALREADY_EXISTS;
                              return;
                          }

                          auto category = std::make_shared<DiscussionCategory>();
                          category->notifyChange() = collection.notifyCategoryChange();
                          category->id() = generateUUIDString();
                          category->name() = name;
                          updateCreated(*category);

                          auto setParentId = IdType::empty;
                          if (parentId)
                          {
                              auto& indexById = collection.categories().get<EntityCollection::DiscussionCategoryCollectionById>();
                              auto it = indexById.find(parentId);
                              if (it != indexById.end())
                              {
                                  collection.modifyDiscussionCategory(it, [&category](DiscussionCategory& parent)
                                  {
                                      parent.addChild(category);
                                  });
                                  category->parentWeak() = *it;
                                  setParentId = parentId;
                              }
                          }

                          collection.categories().insert(category);

                          writeEvents_.onAddNewDiscussionCategory(createObserverContext(*createdBy), *category);

                          status.addExtraSafeName("id", category->id());
                          status.addExtraSafeName("name", category->name());
                          status.addExtraSafeName("parentId", setParentId);
                      });
}

void MemoryRepository::changeDiscussionCategoryName(const IdType& id, const std::string& newName, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! id )
    {
        status = StatusCode::INVALID_PARAMETERS;
        return;
    }
    auto validationCode = validateDiscussionCategoryName(newName, validDiscussionCategoryNameRegex, getGlobalConfig());
    if (validationCode != StatusCode::OK)
    {
        status = validationCode;
        return;
    }
    auto performedBy = preparePerformedBy();

    collection_.write([&](EntityCollection& collection)
                      {
                          auto& indexById = collection.categories().get<EntityCollection::DiscussionCategoryCollectionById>();
                          auto it = indexById.find(id);
                          if (it == indexById.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }
                          auto& indexByName = collection.categories().get<EntityCollection::DiscussionCategoryCollectionByName>();
                          if (indexByName.find(newName) != indexByName.end())
                          {
                              status = StatusCode::ALREADY_EXISTS;
                              return;
                          }

                          auto user = performedBy.getAndUpdate(collection);

                          collection.modifyDiscussionCategory(it, [&newName, &user](DiscussionCategory& category)
                          {
                              category.name() = newName;
                              updateLastUpdated(category, user);
                          });
                          writeEvents_.onChangeDiscussionCategory(createObserverContext(*user), **it,
                                                                  DiscussionCategory::ChangeType::Name);
                      });
}

void MemoryRepository::changeDiscussionCategoryDescription(const IdType& id, const std::string& newDescription, 
                                                           std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! id )
    {
        status = StatusCode::INVALID_PARAMETERS;
        return;
    }
    auto maxLength = static_cast<int_fast32_t>(getGlobalConfig()->discussionCategory.maxDescriptionLength);
    if (countUTF8Characters(newDescription) > maxLength)
    {
        status = StatusCode::VALUE_TOO_LONG;
        return;
    }
    auto performedBy = preparePerformedBy();

    collection_.write([&](EntityCollection& collection)
                      {
                          auto& indexById = collection.categories().get<EntityCollection::DiscussionCategoryCollectionById>();
                          auto it = indexById.find(id);
                          if (it == indexById.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }
                          collection.modifyDiscussionCategory(it, [&newDescription](DiscussionCategory& category)
                          {
                              category.description() = newDescription;
                          });

                          writeEvents_.onChangeDiscussionCategory(createObserverContext(*performedBy.getAndUpdate(collection)),
                                                                  **it, DiscussionCategory::ChangeType::Description);
                      });
}

void MemoryRepository::changeDiscussionCategoryParent(const IdType& id, const IdType& newParentId, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if (( ! id) || (id == newParentId))
    {
        status = StatusCode::INVALID_PARAMETERS;
        return;
    }
    auto performedBy = preparePerformedBy();

    collection_.write([&](EntityCollection& collection)
                      {
                          auto& indexById = collection.categories().get<EntityCollection::DiscussionCategoryCollectionById>();
                          auto it = indexById.find(id);
                          if (it == indexById.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }

                          auto& threadRef = *it;
                          auto& thread = **it;
                          auto newParentIt = indexById.find(newParentId);
                          DiscussionCategoryRef newParentRef; //might be empty

                          if (newParentIt != indexById.end())
                          {
                              newParentRef = *newParentIt;
                              //check that the new parent is not a child of the current category
                              if (newParentRef->hasAncestor(threadRef))
                              {
                                  status = StatusCode::CIRCULAR_REFERENCE_NOT_ALLOWED;
                                  return;
                              }
                          }
                          if (auto currentParent = thread.parentWeak().lock())
                          {
                              if (currentParent->id() == newParentId)
                              {
                                  status = StatusCode::NO_EFFECT;
                                  return;
                              }
                              collection.modifyDiscussionCategoryById(currentParent->id(), 
                                  [&threadRef](DiscussionCategory& parent)
                              {
                                  //remove the current category from it's parent child list
                                  parent.removeChild(threadRef);
                              });
                          }
                          
                          auto user = performedBy.getAndUpdate(collection);
                          
                          collection.modifyDiscussionCategory(it, [&newParentRef, &user](DiscussionCategory& category)
                          {
                              category.parentWeak() = newParentRef;
                              updateLastUpdated(category, user);
                          });

                          //changing a parent requires updating totals
                          //until there's a visible performance penalty, simply update all totals
                          for (auto& category : collection.categories())
                          {
                              category->resetTotals();
                          }
                          for (auto& category : collection.categories())
                          {
                              category->recalculateTotals();
                          }

                          writeEvents_.onChangeDiscussionCategory(createObserverContext(*user), thread, 
                                                                  DiscussionCategory::ChangeType::Parent);
                      });
}
void MemoryRepository::changeDiscussionCategoryDisplayOrder(const IdType& id, int_fast16_t newDisplayOrder, 
                                                            std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! id )
    {
        status = StatusCode::INVALID_PARAMETERS;
        return;
    }

    if (newDisplayOrder < 0)
    {
        status = StatusCode::INVALID_PARAMETERS;
        return;
    }
    auto performedBy = preparePerformedBy();

    collection_.write([&](EntityCollection& collection)
                      {
                          auto& indexById = collection.categories().get<EntityCollection::DiscussionCategoryCollectionById>();
                          auto it = indexById.find(id);
                          if (it == indexById.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }

                          auto user = performedBy.getAndUpdate(collection);

                          collection.modifyDiscussionCategory(it, [&newDisplayOrder, &user](DiscussionCategory& category)
                          {
                              category.displayOrder() = newDisplayOrder;
                              updateLastUpdated(category, user);
                          });

                          writeEvents_.onChangeDiscussionCategory(createObserverContext(*user), **it, 
                                                                  DiscussionCategory::ChangeType::DisplayOrder);
                      });
}

void MemoryRepository::deleteDiscussionCategory(const IdType& id, std::ostream& output)
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
                          auto& indexById = collection.categories().get<EntityCollection::DiscussionCategoryCollectionById>();
                          auto it = indexById.find(id);
                          if (it == indexById.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }
                          //make sure the category is not deleted before being passed to the observers
                          writeEvents_.onDeleteDiscussionCategory(
                                  createObserverContext(*performedBy.getAndUpdate(collection)), **it);
                          collection.deleteDiscussionCategory(it);
                      });
}

void MemoryRepository::addDiscussionTagToCategory(const IdType& tagId, const IdType& categoryId, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! tagId || ! categoryId)
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

                          auto& categoryIndexById = collection.categories().get<EntityCollection::DiscussionCategoryCollectionById>();
                          auto categoryIt = categoryIndexById.find(categoryId);
                          if (categoryIt == categoryIndexById.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }
                          
                          auto& tagRef = *tagIt;
                          auto& tag = **tagIt;
                          auto& categoryRef = *categoryIt;
                          auto& category = **categoryIt;

                          //the number of categories associated to a tag is smaller than 
                          //the number of tags associated to a category, so search the category in the tag
                          if ( ! tag.addCategory(categoryRef))
                          {
                              //actually already added, but return ok
                              status = StatusCode::OK;
                              return;
                          }

                          auto user = performedBy.getAndUpdate(collection);

                          collection.modifyDiscussionCategory(categoryIt, [&tagRef, &user](auto& category)
                          {
                              category.addTag(tagRef);
                              updateLastUpdated(category, user);
                          });

                          writeEvents_.onAddDiscussionTagToCategory(createObserverContext(*user), tag, category);
                      });
}

void MemoryRepository::removeDiscussionTagFromCategory(const IdType& tagId, const IdType& categoryId, std::ostream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! tagId || ! categoryId)
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

                          auto& categoryIndexById = collection.categories().get<EntityCollection::DiscussionCategoryCollectionById>();
                          auto categoryIt = categoryIndexById.find(categoryId);
                          if (categoryIt == categoryIndexById.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }

                          auto& tagRef = *tagIt;
                          auto& tag = **tagIt;
                          auto& categoryRef = *categoryIt;
                          auto& category = **categoryIt;
                          
                          //the number of categories associated to a tag is smaller than 
                          //the number of tags associated to a category, so search the category in the tag
                          if ( ! tag.removeCategory(categoryRef))
                          {
                              status = StatusCode::NO_EFFECT;
                              return;
                          }

                          auto user = performedBy.getAndUpdate(collection);

                          collection.modifyDiscussionCategory(categoryIt, [&tagRef, &user](auto& category)
                          {
                              category.removeTag(tagRef);
                              updateLastUpdated(category, user);
                          });

                          writeEvents_.onRemoveDiscussionTagFromCategory(createObserverContext(*user), tag, category);
                      });    
}
